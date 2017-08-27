/*
 * vars.c - variable tracking
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#include "compile.h"
#include "vars.h"

/* silence linter */
long syscall(long number, ...);
int fexecve(int mem_fd, char *const argv[], char *const envp[]);
size_t strnlen(char const *s, size_t maxlen);
void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

enum var_type extract_type(char const *line, char const *id)
{
	regex_t reg;
	regmatch_t match[6];
	/* return early if passed NULL pointers */
	if (!line || !id)
		ERRX("NULL pointer passed to extract_type()");

	/* first/fourth captures are ignored */
	char *regex, *type;
	char beg[] = "(^|.*[\\(\\{\\;[:blank:]]+)"
		"(bool|_Bool|_Complex|_Imaginary|struct|union|char|double|float|int|long|short|unsigned|void)"
		"(.*)[[:blank:]](";
	char end[] = ")(\\[*)";

	/* append identifier to regex */
	if ((regex = malloc(strlen(id) + sizeof beg + sizeof end - 1)) == NULL)
		ERR("failed to allocate space for regex");
	memset(regex, 0, strlen(id) + sizeof beg + 5);
	memcpy(regex, beg, sizeof beg - 1);
	memcpy(regex + sizeof beg - 1, id, strlen(id));
	memcpy(regex + sizeof beg - 1 + strlen(id), end, sizeof end);

	if (regcomp(&reg, regex, REG_EXTENDED|REG_NEWLINE))
		ERR("failed to compile regex");

	/* non-zero return or -1 value in rm_so means no captures */
	if (regexec(&reg, line, 6, match, 0) || match[3].rm_so == -1) {
		free(regex);
		return T_ERR;
	}
	if ((type = malloc(match[3].rm_eo - match[2].rm_so + match[5].rm_eo - match[5].rm_so + 1)) == NULL)
		ERR("failed to allocate space for captured type");

	/* copy matched string */
	memset(type, 0, match[3].rm_eo - match[2].rm_so + match[5].rm_eo - match[5].rm_so + 1);
	memcpy(type, line + match[2].rm_so, match[3].rm_eo - match[2].rm_so);
	memcpy(type + match[3].rm_eo - match[2].rm_so, line + match[5].rm_so, match[5].rm_eo - match[5].rm_so);

	/* string */
	if (regcomp(&reg, "char \\*", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_STR;
	}

	/* pointer */
	if (regcomp(&reg, "(\\*|\\[)", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_PTR;
	}

	/* char */
	if (regcomp(&reg, "char", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_CHR;
	}

	/* long double */
	if (regcomp(&reg, "long double", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_LDBL;
	}

	/* double */
	if (regcomp(&reg, "(float|double)", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_DBL;
	}

	/* unsigned integral */
	if (regcomp(&reg, "unsigned", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_UINT;
	}

	/* signed integral */
	if (regcomp(&reg, "(bool|_Bool|short|int|long)", REG_EXTENDED|REG_NOSUB|REG_NEWLINE))
		ERR("failed to compile regex");
	if (!regexec(&reg, type, 1, 0, 0)) {
		free(regex);
		free(type);
		return T_INT;
	}

	/* return fallback type */
	free(regex);
	free(type);
	return T_OTHER;
}

size_t extract_id(char const *line, char **id, size_t *offset)
{
	regex_t reg;
	regmatch_t match[2];
	/* second capture is ignored */
	char regex[] = ".*[^[:alnum:]]+"
		"([[:alpha:]_][[:alnum:]_]*)"
		"([^[:alnum:]=!<>]*=|[^[:alnum:]=!<>]*[<>]{2}=*|)"
		"[^=]*";

	/* return early if passed NULL pointers */
	if (!line || !id || !offset)
		ERRX("NULL pointer passed to extract_id()");
	if (regcomp(&reg, regex, REG_EXTENDED|REG_ICASE|REG_NEWLINE))
		ERR("failed to compile regex");
	/* non-zero return or -1 value in rm_so means no captures */
	if (regexec(&reg, line, 2, match, 0) || match[1].rm_so == -1)
		return 0;
	if ((*id= malloc(match[1].rm_eo - match[1].rm_so + 1)) == NULL)
		ERR("failed to allocate space for captured id");

	/* set the output parameter and return the offset */
	memset(*id, 0, match[1].rm_eo - match[1].rm_so + 1);
	memcpy(*id, line + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
	*offset = match[1].rm_so;
	return match[1].rm_so;
}

int find_vars(char const *line, struct str_list *id_list, enum var_type **type_list)
{
	size_t off;
	char *line_tmp[2], *id_tmp;

	if (!line || !id_list || !type_list)
		ERRX("NULL pointer passed to find_vars()");
	if ((line_tmp[0] = malloc(strlen(line) + 1)) == NULL)
		ERR("error allocating line_tmp");

	size_t count = id_list->cnt;
	memcpy(line_tmp[0], line, strlen(line) + 1);
	line_tmp[1] = line_tmp[0];
	/* extract all identifiers from the line */
	while (extract_id(line_tmp[0], &id_tmp, &off) != 0) {
		append_str(id_list, id_tmp, 0);
		free(id_tmp);
		line_tmp[0] += off;
		count++;
	}
	line_tmp[0] = line_tmp[1];
	/* get the type of each identifier */
	enum var_type type_tmp[id_list->cnt];
	for (size_t i = count; i < id_list->cnt; i++) {
		if ((type_tmp[i] = extract_type(line_tmp[0], id_list->list[i])) == T_ERR)
			WARNXGEN(id_list->list[i]);
	}

	/* copy it into the output parameter */
	if ((*type_list = malloc(sizeof type_tmp)) == NULL)
		ERR("failed to allocate memory for type_list");
	memcpy(*type_list, type_tmp, sizeof type_tmp);
	free(line_tmp[1]);
	return id_list->cnt;
}

int print_vars(char const *src, char *const cc_args[], char *const exec_args[], struct var_list *list)
{
	int mem_fd, status;
	int pipe_cc[2], pipe_ld[2], pipe_exec[2];
	char src_buffer[strnlen(src, COUNT) + 1];
	char prog_end[] = "\n\treturn 0;\n}\n";
	char print_beg[] = "\n\tprintf(\"%s = %s, \",";
	char println_beg[] = "\n\tprintf(\"%s = %s\n \",";
	char func_end[] = ");";
	char *src_tmp = NULL, *id_tmp = NULL;
	size_t off = 0;

	/* sanity checks */
	if (!src || !cc_args || !exec_args || !list)
		ERRX("NULL pointer passed to print_vars()");
	if (sizeof src_buffer < 2)
		ERRX("empty source string passed to print_vars()");

	/* add trailing '\n' */
	memcpy(src_buffer, src, sizeof src_buffer);
	src_tmp = src_buffer;

	char final[sizeof src_buffer + sizeof prog_end];
	memset(final, 0, sizeof final);
	memcpy(final, src_buffer, sizeof src_buffer - 1);
	memcpy(final + sizeof src_buffer - 1, prog_end, sizeof prog_end);
	final[sizeof src_buffer + sizeof prog_end - 1] = '\n';

	/* create pipes */
	/* if (pipe(pipe_cc) == -1) */
	/*         ERR("error making pipe_cc pipe"); */
	/* if (pipe(pipe_ld) == -1) */
	/*         ERR("error making pipe_ld pipe"); */
	/* if (pipe(pipe_exec) == -1) */
	/*         ERR("error making pipe_exec pipe"); */
	/* set close-on-exec for pipe fds */
	/* set_cloexec(pipe_cc); */
	/* set_cloexec(pipe_ld); */
	/* set_cloexec(pipe_exec); */

	/* fork compiler */
	/* switch (fork()) { */
	/* error */
	/* case -1: */
	/*         close(pipe_cc[0]); */
	/*         close(pipe_cc[1]); */
	/*         close(pipe_ld[0]); */
	/*         close(pipe_ld[1]); */
	/*         close(pipe_exec[0]); */
	/*         close(pipe_exec[1]); */
	/*         ERR("error forking compiler"); */
	/*         break; */

	/* child */
	/* case 0: */
	/*         dup2(pipe_cc[0], 0); */
	/*         dup2(pipe_ld[1], 1); */
	/*         execvp(cc_args[0], cc_args); */
	/*         [> execvp() should never return <] */
	/*         ERR("error forking compiler"); */
	/*         break; */

	/* parent */
	/* default: */
	/*         close(pipe_cc[0]); */
	/*         close(pipe_ld[1]); */
	/*         if (write(pipe_cc[1], final, sizeof final) == -1) */
	/*                 ERR("error writing to pipe_cc[1]"); */
	/*         close(pipe_cc[1]); */
	/*         wait(&status); */
	/*         if (WIFEXITED(status) && WEXITSTATUS(status)) { */
	/*                 WARNX("compiler returned non-zero exit code"); */
	/*                 return WEXITSTATUS(status); */
		/* } */
	/* } */

	return 1;
}
