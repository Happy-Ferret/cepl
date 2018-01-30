/*
 * cepl.c - REPL translation unit
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

/* silence linter */
#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#include "compile.h"
#include "errs.h"
#include "hist.h"
#include "parseopts.h"
#include "readline.h"
#include "vars.h"
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * program source strucs (program_state.src[0] is
 * truncated for interactive printing)
 */
static struct program program_state;
/* SIGINT buffer for non-local goto */
static sigjmp_buf jmp_env;
/* TODO: change history filename to a non-hardcoded string */
static char hist_name[] = "./.cepl_history";
/* output file buffer and cc args */
static char **cc_argv;

/* string to compile */
extern char eval_arg[];
/* output filenames */
extern char *input_src[3];
extern char const *prelude, *prog_start, *prog_start_user, *prog_end;
extern enum asm_type asm_dialect;

static inline char *read_line(char **restrict ln)
{
	/* false while waiting for input */
	program_state.exec_flag = false;
	/* return early if executed with `-e` argument */
	if (program_state.eval_flag) {
		*ln = eval_arg;
		return *ln;
	}
	/* use an empty prompt if stdin is a pipe */
	if (isatty(STDIN_FILENO)) {
		*ln = readline(">>> ");
		return *ln;
	}

	/* redirect stdout to /dev/null */
	FILE *bitbucket;
	if (!(bitbucket = fopen("/dev/null", "r+b")))
		ERR("%s", "read_line() fopen()");
	rl_outstream = bitbucket;
	*ln = readline(NULL);
	rl_outstream = NULL;
	fclose(bitbucket);
	return *ln;
}

static inline void init_vars(void)
{
	if (program_state.type_list.list) {
		free(program_state.type_list.list);
		program_state.type_list.list = NULL;
	}
	if (program_state.var_list.list) {
		for (size_t i = 0; i < program_state.var_list.cnt; i++)
			free(program_state.var_list.list[i].id);
		free(program_state.var_list.list);
	}
	init_var_list(&program_state.var_list);
}

static inline void undo_last_line(void)
{
	/* break early if no history to pop */
	if (program_state.src[0].flags.cnt < 1 || program_state.src[1].flags.cnt < 1)
		return;
	for (size_t i = 0; i < 2; i++)
		pop_history(&program_state.src[i]);
	/* break early if tracking disabled */
	if (!program_state.track_flag)
		return;
	init_vars();
	/* add vars from previous lines */
	for (size_t i = 1; i < program_state.src[0].lines.cnt; i++) {
		if (program_state.src[0].lines.list[i] && program_state.src[0].flags.list[i] == IN_MAIN) {
			if (find_vars(&program_state, program_state.src[0].lines.list[i]))
				gen_var_list(&program_state);
		}
	}
}

/* exit handler registration */
static inline void free_bufs(void)
{
	free_buffers(&program_state);
	cleanup(&program_state);
}

/* general signal handling function */
static void sig_handler(int sig)
{
	/* abort current input line */
	if (sig == SIGINT) {
		rl_clear_visible_line();
		rl_reset_line_state();
		rl_free_line_state();
		fputc('\n', stderr);
		if (program_state.exec_flag) {
			undo_last_line();
			program_state.exec_flag = false;
		}
		siglongjmp(jmp_env, 1);
	}
	free_buffers(&program_state);
	cleanup(&program_state);
	raise(sig);
}

/* register signal handlers to make sure that history is written out */
static void reg_handlers(void)
{
	/* signals to trap */
	struct { int sig; char *sig_name; } sigs[] = {
		{SIGHUP, "SIGHUP"}, {SIGINT, "SIGINT"},
		{SIGQUIT, "SIGQUIT"}, {SIGILL, "SIGILL"},
		{SIGABRT, "SIGABRT"}, {SIGFPE, "SIGFPE"},
		{SIGSEGV, "SIGSEGV"}, {SIGPIPE, "SIGPIPE"},
		{SIGALRM, "SIGALRM"}, {SIGTERM, "SIGTERM"},
		{SIGBUS, "SIGBUS"}, {SIGSYS, "SIGSYS"},
		{SIGVTALRM, "SIGVTALRM"}, {SIGXCPU, "SIGXCPU"},
		{SIGXFSZ, "SIGXFSZ"},
	};
	struct sigaction sa[ARRLEN(sigs)];
	for (size_t i = 0; i < ARRLEN(sigs); i++) {
		sa[i].sa_handler = &sig_handler;
		sigemptyset(&sa[i].sa_mask);
		sa[i].sa_flags = (sigs[i].sig == SIGINT)
			? SA_RESTART
			: SA_RESETHAND|SA_RESTART;
		/* don't reset `SIGINT` handler */
		if (sigaction(sigs[i].sig, &sa[i], NULL) == -1)
			ERR("%s %s", sigs[i].sig_name, "sigaction()");
	}
	if (at_quick_exit(&free_bufs))
		WARN("%s", "at_quick_exit(&free_bufs)");
}

static void eval_line(int argc, char **restrict argv, char const *restrict optstring, char ***restrict cc_args)
{
	char *ln = NULL, *ln_save = program_state.cur_line;
	char const *const term = getenv("TERM");
	struct program prg;
	struct str_list temp = strsplit(program_state.cur_line);
	bool has_color = term
		&& isatty(STDOUT_FILENO)
		&& isatty(STDERR_FILENO)
		&& strcmp(term, "")
		&& strcmp(term, "dumb");
	char const *const ln_beg = has_color
		? "printf(\"" GREEN "%s%lld\\n" RST "\", \"result = \", (long long)("
		: "printf(\"%s%lld\\n\", \"result = \", (long long)(";
	char const *const ln_end = "));";
	bool flags[] = {
		program_state.warn_flag, program_state.track_flag, program_state.parse_flag,
		program_state.out_flag, program_state.eval_flag, program_state.asm_flag,
	};

	/* save and close stderr */
	int saved_fd = dup(STDERR_FILENO);
	close(STDERR_FILENO);
	init_buffers(&prg);
	build_final(&prg, argv);
	for (size_t i = 0; i < temp.cnt; i++) {
		size_t sz = strlen(ln_beg) + strlen(ln_end) + strlen(temp.list[i]) + 1;
		/* initialize source buffers */
		xcalloc(char, &ln, 1, strlen(temp.list[i]) + sz, "eval_line() calloc");
		strmv(0, ln, ln_beg);
		strmv(CONCAT, ln, temp.list[i]);
		strmv(CONCAT, ln, ln_end);
#ifdef _DEBUG
		puts(ln);
#endif

		for (size_t j = 0; j < 2; j++) {
			rsz_buf(&prg, &prg.src[j].body, &prg.src[j].body_size, &prg.src[j].body_max, sz);
			rsz_buf(&prg, &prg.src[j].total, &prg.src[j].total_size, &prg.src[j].total_max, sz);
		}
		/* extract identifiers and types */
		if (temp.list[i][0] != ';' && !find_vars(&prg, temp.list[i])) {
			build_body(&prg);
			build_final(&prg, argv);
		}
		free(ln);
		ln = NULL;
	}
	/* print generated source code unless stdin is a pipe */
	compile(prg.src[1].total, cc_argv, argv);
	free_buffers(&prg);
	free_str_list(&temp);
	dup2(saved_fd, STDERR_FILENO);
	program_state.cur_line = ln_save;

	/* reset to defaults */
	program_state.warn_flag = false;
	program_state.track_flag = true;
	program_state.parse_flag = true;
	program_state.out_flag = false;
	program_state.eval_flag = false;
	program_state.asm_flag = false;
	/* re-initiatalize compiler arg array */
	*cc_args = parse_opts(&program_state, argc, argv, optstring);
	/* restore old values */
	program_state.warn_flag = flags[0];
	program_state.track_flag = flags[1];
	program_state.parse_flag = flags[2];
	program_state.out_flag = flags[3];
	program_state.eval_flag = flags[4];
	program_state.asm_flag = flags[5];
}

static inline void toggle_att(int argc, char **argv, char const *optstring, char *tbuf)
{
	/* if file was open, flip it and break early */
	if (program_state.asm_flag) {
		program_state.asm_flag ^= true;
		return;
	}
	program_state.asm_flag ^= true;
	tbuf = strpbrk(program_state.cur_line, " \t");
	/* return if file name empty */
	if (!tbuf || strspn(tbuf, " \t") == strlen(tbuf)) {
		/* reset flag */
		program_state.asm_flag ^= true;
		return;
	}
	/* increment pointer to start of definition */
	tbuf += strspn(tbuf, " \t");
	if (program_state.asm_filename) {
		free(program_state.asm_filename);
		program_state.asm_filename = NULL;
	}
	if (!(program_state.asm_filename = calloc(1, strlen(tbuf) + 1)))
		ERR("%s", "error during program_state.asm_filename calloc()");
	strmv(0, program_state.asm_filename, tbuf);
	free_buffers(&program_state);
	init_buffers(&program_state);
	asm_dialect = ATT;
	/* reset to defaults */
	program_state.warn_flag = false;
	program_state.track_flag = true;
	program_state.parse_flag = true;
	program_state.out_flag = false;
	program_state.eval_flag = false;
	/* re-initiatalize compiler arg array */
	cc_argv = parse_opts(&program_state, argc, argv, optstring);
}

static inline void toggle_intel(int argc, char **argv, char const *optstring, char *tbuf)
{
	/* if file was open, flip it and break early */
	if (program_state.asm_flag) {
		program_state.asm_flag ^= true;
		return;
	}
	program_state.asm_flag ^= true;
	tbuf = strpbrk(program_state.cur_line, " \t");
	/* return if file name empty */
	if (!tbuf || strspn(tbuf, " \t") == strlen(tbuf)) {
		/* reset flag */
		program_state.asm_flag ^= true;
		return;
	}
	/* increment pointer to start of definition */
	tbuf += strspn(tbuf, " \t");
	if (program_state.asm_filename) {
		free(program_state.asm_filename);
		program_state.asm_filename = NULL;
	}
	if (!(program_state.asm_filename = calloc(1, strlen(tbuf) + 1)))
		ERR("%s", "error during program_state.asm_filename calloc()");
	strmv(0, program_state.asm_filename, tbuf);
	free_buffers(&program_state);
	init_buffers(&program_state);
	asm_dialect = INTEL;
	/* reset to defaults */
	program_state.warn_flag = false;
	program_state.track_flag = true;
	program_state.parse_flag = true;
	program_state.out_flag = false;
	program_state.eval_flag = false;
	/* re-initiatalize compiler arg array */
	cc_argv = parse_opts(&program_state, argc, argv, optstring);
}

static inline void toggle_output_file(int argc, char **argv, char const *optstring, char *tbuf)
{
	/* if file was open, flip it and break early */
	if (program_state.out_flag) {
		program_state.out_flag ^= true;
		return;
	}
	program_state.out_flag ^= true;
	tbuf = strpbrk(program_state.cur_line, " \t");
	/* return if file name empty */
	if (!tbuf || strspn(tbuf, " \t") == strlen(tbuf)) {
		/* reset flag */
		program_state.out_flag ^= true;
		return;
	}
	/* increment pointer to start of definition */
	tbuf += strspn(tbuf, " \t");
	if (program_state.out_filename) {
		free(program_state.out_filename);
		program_state.out_filename = NULL;
	}
	if (!(program_state.out_filename = calloc(1, strlen(tbuf) + 1)))
		ERR("%s", "error during program_state.out_filename calloc()");
	strmv(0, program_state.out_filename, tbuf);
	free_buffers(&program_state);
	init_buffers(&program_state);
	printf("%d", program_state.out_flag);
	/* reset to defaults */
	program_state.warn_flag = false;
	program_state.track_flag = true;
	program_state.parse_flag = true;
	program_state.eval_flag = false;
	program_state.asm_flag = false;
	/* re-initiatalize compiler arg array */
	cc_argv = parse_opts(&program_state, argc, argv, optstring);
}

static inline void parse_macro(char *tbuf)
{
	/* remove trailing ' ' and '\t' */
	for (size_t i = strlen(program_state.cur_line) - 1; i > 0; i--) {
		if (program_state.cur_line[i] != ' ' && program_state.cur_line[i] != '\t')
			break;
		program_state.cur_line[i] = '\0';
	}
	tbuf = strpbrk(program_state.cur_line, " \t");
	/* return if function definition empty */
	if (!tbuf || strspn(tbuf, " \t") == strlen(tbuf))
		return;
	/* increment pointer to start of definition */
	tbuf += strspn(tbuf, " \t");
	/* re-allocate enough memory for program_state.cur_line + '\n' + '\n' + '\0' */
	size_t sz = strlen(tbuf) + 3;
	for (size_t i = 0; i < 2; i++) {
		struct program *prg = &program_state;
		rsz_buf(prg, &prg->src[i].funcs, &prg->src[i].funcs_size, &prg->src[i].funcs_max, sz);
	}

	switch (tbuf[0]) {
	/* dont append ';' for preprocessor directives */
	case '#':
		/* remove trailing ' ' and '\t' */
		for (size_t i = strlen(tbuf) - 1; i > 0; i--) {
			if (tbuf[i] != ' ' && tbuf[i] != '\t')
				break;
			tbuf[i] = '\0';
		}
		build_funcs(&program_state);
		for (size_t i = 0; i < 2; i++)
			strmv(CONCAT, program_state.src[i].funcs, "\n");
		break;

	default:
		/* remove trailing ' ' and '\t' */
		for (size_t i = strlen(tbuf) - 1; i > 0; i--) {
			if (tbuf[i] != ' ' && tbuf[i] != '\t')
				break;
			tbuf[i] = '\0';
		}

		switch(tbuf[strlen(tbuf) - 1]) {
		case '{': /* fallthough */
		case '}': /* fallthough */
		case ';': /* fallthough */
		case '\\': {
				/* remove extra trailing ';' */
				for (size_t j = strlen(tbuf) - 1; j > 0; j--) {
					if (tbuf[j] != ';' || tbuf[j - 1] != ';')
						break;
					tbuf[j] = '\0';
				}
				build_funcs(&program_state);
				for (size_t i = 0; i < 2; i++)
					strmv(CONCAT, program_state.src[i].funcs, "\n");

				struct str_list tmp = strsplit(program_state.cur_line);
				for (size_t i = 0; i < tmp.cnt; i++) {
					/* extract identifiers and types */
					if (program_state.track_flag && find_vars(&program_state, tmp.list[i]))
						gen_var_list(&program_state);
				}
				free_str_list(&tmp);
				break;
			}

		default: {
				build_funcs(&program_state);
				/* append ';' if no trailing '}', ';', or '\' */
				for (size_t i = 0; i < 2; i++)
					strmv(CONCAT, program_state.src[i].funcs, ";\n");
				struct str_list tmp = strsplit(program_state.cur_line);
				for (size_t i = 0; i < tmp.cnt; i++) {
					/* extract identifiers and types */
					if (program_state.track_flag && find_vars(&program_state, tmp.list[i]))
						gen_var_list(&program_state);
				}
				free_str_list(&tmp);
			 }
		}
	}
}

static inline void parse_normal(void)
{
	/* remove trailing ' ' and '\t' */
	for (size_t i = strlen(program_state.cur_line) - 1; i > 0; i--) {
		if (program_state.cur_line[i] != ' ' && program_state.cur_line[i] != '\t')
			break;
		program_state.cur_line[i] = '\0';
	}
	switch(program_state.cur_line[strlen(program_state.cur_line) - 1]) {
	case '{': /* fallthough */
	case '}': /* fallthough */
	case ';': /* fallthough */
	case '\\': {
			build_body(&program_state);
			for (size_t i = 0; i < 2; i++) {
				struct program *prg = &program_state;
				/* remove extra trailing ';' */
				size_t b_len = strlen(prg->src[i].body) - 1;
				for (size_t j = b_len; j > 0; j--) {
					if (prg->src[i].body[j] != ';' || prg->src[i].body[j - 1] != ';')
						break;
					prg->src[i].body[j] = '\0';
				}
				strmv(CONCAT, prg->src[i].body, "\n");
			}
			struct str_list tmp = strsplit(program_state.cur_line);
			for (size_t i = 0; i < tmp.cnt; i++) {
				/* extract identifiers and types */
				if (program_state.track_flag && find_vars(&program_state, tmp.list[i]))
					gen_var_list(&program_state);
			}
			free_str_list(&tmp);
			break;
		   }

	default: {
			build_body(&program_state);
			/* append ';' if no trailing '}', ';', or '\' */
			for (size_t i = 0; i < 2; i++)
				strmv(CONCAT, program_state.src[i].body, ";\n");
			struct str_list tmp = strsplit(program_state.cur_line);
			for (size_t i = 0; i < tmp.cnt; i++) {
				/* extract identifiers and types */
				if (program_state.track_flag && find_vars(&program_state, tmp.list[i]))
					gen_var_list(&program_state);
			}
			free_str_list(&tmp);
		}
	}
}

static inline void scan_input_file(void)
{
	init_vars();
	/* parse input file if one is specified */
	struct str_list tmp = strsplit(prog_start_user);
	for (size_t i = 0; i < tmp.cnt; i++) {
		/* extract identifiers and types */
		if (program_state.track_flag && find_vars(&program_state, tmp.list[i]))
			gen_var_list(&program_state);
	}
	free_str_list(&tmp);
}

static inline void build_hist_name(void)
{
	struct stat hist_stat;
	size_t buf_sz = sizeof hist_name, hist_len = 0;
	char const *const home_env = getenv("HOME");
	FILE *make_hist = NULL;

	/* add hist_length of “$HOME/” if home_env is non-NULL */
	if (home_env && strcmp(home_env, ""))
		buf_sz += strlen(home_env) + 1;
	/* prepend "~/" to history fihist_lename ("~/.cepl_history" by default) */
	if (!(program_state.hist_file = calloc(1, buf_sz)))
		ERR("%s", "program_state.hist_file malloc()");
	/* check if home_env is non-NULL */
	if (home_env && strcmp(home_env, "")) {
		hist_len = strlen(home_env);
		strmv(0, program_state.hist_file, home_env);
		program_state.hist_file[hist_len++] = '/';
	}
	strmv(hist_len, program_state.hist_file, hist_name);

	/* enable completion */
	rl_completion_entry_function = &generator;
	rl_attempted_completion_function = &completer;
	rl_basic_word_break_characters = " \t\n\"\\'`@$><=|&{}()[]";
	rl_completion_suppress_append = 1;
	rl_bind_key('\t', &rl_complete);

	/* initialize history sesssion */
	using_history();
	/* create history file if it doesn't exsit */
	if (!(make_hist = fopen(program_state.hist_file, "ab"))) {
		WARN("%s", "error creating history file with fopen()");
	} else {
		fclose(make_hist);
		program_state.has_hist = true;
	}
	/* read program_state.hist_file if size is non-zero */
	stat(program_state.hist_file, &hist_stat);
	if (hist_stat.st_size > 0) {
		if (read_history(program_state.hist_file)) {
			char hist_pre[] = "error reading history from ";
			char hist_full[sizeof hist_pre + strlen(program_state.hist_file)];
			char *hist_ptr = hist_full;
			strmv(0, hist_ptr, hist_pre);
			strmv(sizeof hist_pre - 1, hist_ptr, program_state.hist_file);
			WARN("%s", hist_ptr);
		}
	}
}

int main(int argc, char **argv)
{
	char const optstring[] = "hptvwc:a:f:e:i:l:I:o:";
	/* token buffers */
	char *lbuf = NULL, *tbuf = NULL;

	/* build history filename from environment */
	build_hist_name();

	/* initiatalize compiler arg array */
	cc_argv = parse_opts(&program_state, argc, argv, optstring);
	program_state.asm_flag = false;
	program_state.eval_flag = false;
	program_state.exec_flag = true;
	program_state.in_flag = false;
	program_state.out_flag = false;
	program_state.parse_flag = true;
	program_state.track_flag = true;
	program_state.warn_flag = false;

	/* initialize source buffers */
	init_buffers(&program_state);

	/* scan input source file if applicable */
	if (program_state.in_flag)
		scan_input_file();

	/* initialize program_state.src[0].total and program_state.src[1].total then print version */
	build_final(&program_state, argv);
	if (isatty(STDIN_FILENO) && !program_state.eval_flag)
		printf("%s\n", VERSION_STRING);
	reg_handlers();
	rl_set_signals();
	sigsetjmp(jmp_env, 1);

	/* loop readline() until EOF is read */
	while (read_line(&lbuf)) {
		/* if no input then do nothing */
		if (!*lbuf)
			continue;

		/* point global at line */
		program_state.cur_line = lbuf;
		/* program_state.cur_line newlines */
		if ((tbuf = strpbrk(program_state.cur_line, "\f\r\n")))
			tbuf[0] = '\0';
		/* add and dedup history */
		dedup_history(&lbuf);
		/* re-enable completion if disabled */
		rl_bind_key('\t', &rl_complete);

		/* re-allocate enough memory for line + '\t' + ';' + '\n' + '\0' */
		for (size_t i = 0; i < 2; i++) {
			struct program *prg = &program_state;
			rsz_buf(prg, &prg->src[i].body, &prg->src[i].body_size, &prg->src[i].body_max, 3);
			rsz_buf(prg, &prg->src[i].total, &prg->src[i].total_size, &prg->src[i].total_max, 3);
		}

		/* strip leading whitespace */
		program_state.cur_line += strspn(program_state.cur_line, " \t");

		eval_line(argc, argv, optstring, &cc_argv);

		/* control sequence and preprocessor directive parsing */
		switch (program_state.cur_line[0]) {
		case ';':
			switch(program_state.cur_line[1]) {
			/* pop last history statement */
			case 'u':
				undo_last_line();
				break;

			/* toggle writing at&t-dialect asm output */
			case 'a':
				toggle_att(argc, argv, optstring, tbuf);
				break;

			/* toggle writing intel-dialect asm output */
			case 'i':
				toggle_intel(argc, argv, optstring, tbuf);
				break;

			/* toggle output file writing */
			case 'o':
				toggle_output_file(argc, argv, optstring, tbuf);
				break;

			/* toggle library parsing */
			case 'p':
				program_state.parse_flag ^= true;
				free_buffers(&program_state);
				init_buffers(&program_state);
				/* reset to defaults */
				program_state.warn_flag = false;
				program_state.track_flag = true;
				program_state.out_flag = false;
				program_state.eval_flag = false;
				program_state.asm_flag = false;
				/* re-initiatalize compiler arg array */
				cc_argv = parse_opts(&program_state, argc, argv, optstring);
				break;

			/* toggle variable tracking */
			case 't':
				program_state.track_flag ^= true;
				free_buffers(&program_state);
				init_buffers(&program_state);
				/* reset to defaults */
				program_state.warn_flag = false;
				program_state.parse_flag = true;
				program_state.out_flag = false;
				program_state.eval_flag = false;
				program_state.asm_flag = false;
				/* re-initiatalize compiler arg array */
				cc_argv = parse_opts(&program_state, argc, argv, optstring);
				break;

			/* toggle warnings */
			case 'w':
				program_state.warn_flag ^= true;
				free_buffers(&program_state);
				init_buffers(&program_state);
				/* reset to defaults */
				program_state.track_flag = true;
				program_state.parse_flag = true;
				program_state.out_flag = false;
				program_state.eval_flag = false;
				program_state.asm_flag = false;
				/* re-initiatalize compiler arg array */
				cc_argv = parse_opts(&program_state, argc, argv, optstring);
				break;

			/* reset state */
			case 'r':
				free_buffers(&program_state);
				init_buffers(&program_state);
				/* reset to defaults */
				program_state.warn_flag = false;
				program_state.track_flag = true;
				program_state.parse_flag = true;
				program_state.out_flag = false;
				program_state.eval_flag = false;
				program_state.asm_flag = false;
				/* re-initiatalize compiler arg array */
				cc_argv = parse_opts(&program_state, argc, argv, optstring);
				break;

			/* define an include/macro/function */
			case 'm': /* fallthrough */
			case 'f':
				parse_macro(tbuf);
				break;

			/* show usage information */
			case 'h':
				fprintf(stderr, "%s %s %s\n", "Usage:", argv[0], USAGE_STRING);
				break;

			/* clean up and exit program */
			case 'q':
				free_buffers(&program_state);
				cleanup(&program_state);
				exit(EXIT_SUCCESS);
				/* unused break */
				break;

			/* unknown command becomes a noop */
			default:;
			}
			break;

		/* dont append ';' for preprocessor directives */
		case '#':
			/* remove trailing ' ' and '\t' */
			for (size_t i = strlen(program_state.cur_line) - 1; i > 0; i--) {
				if (program_state.cur_line[i] != ' ' && program_state.cur_line[i] != '\t')
					break;
				program_state.cur_line[i] = '\0';
			}
			/* start building program source */
			build_body(&program_state);
			for (size_t i = 0; i < 2; i++)
				strmv(CONCAT, program_state.src[i].body, "\n");
			break;

		default:
			parse_normal();
		}

		/* set to true before compiling */
		program_state.exec_flag = true;
		/* finalize source */
		build_final(&program_state, argv);
		/* fix buffering issues */
		sync();
		usleep(5000);
		/* print generated source code unless stdin is a pipe */
		if (isatty(STDIN_FILENO) && !program_state.eval_flag)
			printf("%s:\n==========\n%s\n==========\n", argv[0], program_state.src[0].total);
		int ret = compile(program_state.src[1].total, cc_argv, argv);
		/* fix buffering issues */
		sync();
		usleep(5000);
		/* print output and exit code if non-zero */
		if (ret || (isatty(STDIN_FILENO) && !program_state.eval_flag))
			printf("[exit status: %d]\n", ret);

		/* exit if executed with `-e` argument */
		if (program_state.eval_flag) {
			lbuf = NULL;
			break;
		}
		/* cleanup old buffer */
		free(program_state.cur_line);
		program_state.cur_line = NULL;
	}

	free_buffers(&program_state);
	cleanup(&program_state);
	return 0;
}
