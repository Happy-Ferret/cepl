/*
 * hist.c - history handling functions
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#include "hist.h"

/* externs */
extern bool track_flag;
/* compiler argument list */
extern char **cc_argv;
/* completion list of generated symbols */
extern struct str_list comp_list;

/* globals */
bool has_hist = false;
/* filenames */
char *out_filename;
char *hist_file;
/* `-o` flag output file */
FILE *volatile ofile;
/* program source strucs (prg[0] is truncated for interactive printing) */
struct prog_src prg[2];
/* type, identifier, and var lists */
struct type_list tl;
struct str_list il;
struct var_list vl;

void cleanup(void)
{
	/* free generated completions */
	free_str_list(&comp_list);
	/* append history to history file */
	if (has_hist) {
		if (write_history(hist_file)) {
			WARN("write_history()");
		}
	}
	if (hist_file) {
		free(hist_file);
		hist_file = NULL;
	}
	if (isatty(STDIN_FILENO))
		printf("\n%s\n\n", "Terminating program.");
}

void write_file(FILE *volatile *out_file, struct prog_src (*restrict prgm)[])
{
	/* return early if no file open */
	if (!out_file || !*out_file || !(*prgm) || !(*prgm)[1].total)
		return;
	/* write out program to file */
	fwrite((*prgm)[1].total, strlen((*prgm)[1].total), 1, *out_file);
	fputc('\n', *out_file);
	fflush(NULL);
	fclose(*out_file);
	*out_file = NULL;
}

void free_buffers(struct var_list *restrict vlist, struct type_list *restrict tlist, struct str_list *restrict ilist, struct prog_src (*restrict prgm)[], char **restrict ln)
{
	/* write out history before freeing buffers */
	write_file(&ofile, prgm);
	if (out_filename) {
		free(out_filename);
		out_filename = NULL;
	}
	free_str_list(ilist);
	/* clean up user data */
	if (*ln) {
		free(*ln);
		*ln = NULL;
	}
	if (tlist) {
		free(tlist->list);
		tlist->list = NULL;
	}
	/* free vectors */
	if (cc_argv)
		free_argv(&cc_argv);
	if (vlist->list) {
		for (size_t i = 0; i < vlist->cnt; i++) {
			if (vlist->list[i].key)
				free(vlist->list[i].key);
		}
		free(vlist->list);
	}
	/* free program structs */
	for (size_t i = 0; i < 2; i++) {
		if ((*prgm)[i].f)
			free((*prgm)[i].f);
		if ((*prgm)[i].b)
			free((*prgm)[i].b);
		if ((*prgm)[i].total)
			free((*prgm)[i].total);
		if ((*prgm)[i].flags.list)
			free((*prgm)[i].flags.list);
		free_str_list(&(*prgm)[i].hist);
		free_str_list(&(*prgm)[i].lines);
		(*prgm)[i].b_sz = (*prgm)[i].f_sz = (*prgm)[i].t_sz = 0;
		(*prgm)[i].b_max = (*prgm)[i].f_max = (*prgm)[i].t_max = 1;
		/* `(void *)` casts needed to chain diff ptr types */
		(*prgm)[i].b = (*prgm)[i].f = (*prgm)[i].total = (void *)((*prgm)[i].flags.list = NULL);
	}
}

void init_buffers(struct var_list *restrict vlist, struct type_list *restrict tlist, struct str_list *restrict ilist, struct prog_src (*restrict prgm)[], char **restrict ln)
{
	/* user is truncated source for display */
	(*prgm)[0].f = calloc(1, 1);
	(*prgm)[0].b = calloc(1, strlen(prog_start_user) + 1);
	(*prgm)[0].total = calloc(1, strlen(prelude) + strlen(prog_start_user) + strlen(prog_end) + 3);
	(*prgm)[0].f_sz = (*prgm)[0].f_max = 1;
	(*prgm)[0].b_sz = (*prgm)[0].b_max = strlen(prog_start_user) + 1;
	(*prgm)[0].t_sz = (*prgm)[0].t_max = strlen(prelude) + strlen(prog_start_user) + strlen(prog_end) + 3;
	/* actual is source passed to compiler */
	(*prgm)[1].f = calloc(1, strlen(prelude) + 1);
	(*prgm)[1].b = calloc(1, strlen(prog_start) + 1);
	(*prgm)[1].total = calloc(1, strlen(prelude) + strlen(prog_start) + strlen(prog_end) + 3);
	(*prgm)[1].f_sz = (*prgm)[1].f_max = strlen(prelude) + 1;
	(*prgm)[1].b_sz = (*prgm)[1].b_max = strlen(prog_start) + 1;
	(*prgm)[1].t_sz = (*prgm)[1].t_max = strlen(prelude) + strlen(prog_start) + strlen(prog_end) + 3;
	/* sanity check */
	for (size_t i = 0; i < 2; i++) {
		if (!(*prgm)[i].f || !(*prgm)[i].b || !(*prgm)[i].total) {
			free_buffers(vlist, tlist, ilist, prgm, ln);
			cleanup();
			ERR("prgm[2] calloc()");
		}
	}
	/* no memcpy for prgm[0].f */
	strmv(0, (*prgm)[0].b, prog_start_user);
	strmv(0, (*prgm)[1].f, prelude);
	strmv(0, (*prgm)[1].b, prog_start);
	/* init source history and flag lists */
	for (size_t i = 0; i < 2; i++) {
		init_list(&(*prgm)[i].lines, "FOOBARTHISVALUEDOESNTMATTERTROLLOLOLOL");
		init_list(&(*prgm)[i].hist, "FOOBARTHISVALUEDOESNTMATTERTROLLOLOLOL");
		init_flag_list(&(*prgm)[i].flags);
	}
	init_vlist(vlist);
	init_tlist(tlist);
	init_list(ilist, "FOOBARTHISVALUEDOESNTMATTERTROLLOLOLOL");
}

size_t rsz_buf(char **restrict buf, size_t *restrict buf_sz, size_t *restrict b_max, size_t off, struct var_list *restrict vlist, struct type_list *restrict tlist, struct str_list *restrict ilist, struct prog_src (*restrict prgm)[], char **restrict ln)
{
	/* sanity check */
	if (!buf || !*buf || !ln)
		return 0;
	char *tmp;
	size_t alloc_sz = strlen(*buf) + strlen(*ln) + off + 1;
	if (!buf_sz || !b_max) {
		/* current length + line length + extra characters + \0 */
		if (!(tmp = realloc(*buf, alloc_sz))) {
			free_buffers(vlist, tlist, ilist, prgm, ln);
			cleanup();
			ERR("rsz_buf()");
		}
		*buf = tmp;
		return alloc_sz;
	}
	*buf_sz += alloc_sz;
	/* realloc only if b_max is less than current size */
	if (*buf_sz < *b_max)
		return 0;
	/* check if size too large */
	if (*buf_sz > MAX)
		ERRX("*buf_sz > (SIZE_MAX / 2 - 1)");
	/* double until size is reached */
	while ((*b_max *= 2) < *buf_sz);
	/* current length + line length + extra characters + \0 */
	if (!(tmp = realloc(*buf, *b_max))) {
		free_buffers(vlist, tlist, ilist, prgm, ln);
		cleanup();
		ERR("rsz_buf()");
	}
	*buf = tmp;
	return *buf_sz;
}

void pop_history(struct prog_src *restrict prgm)
{
	switch(prgm->flags.list[--prgm->flags.cnt]) {
	case NOT_IN_MAIN:
		prgm->hist.cnt = prgm->lines.cnt = prgm->flags.cnt;
		strmv(0, prgm->f, prgm->hist.list[prgm->hist.cnt]);
		free(prgm->hist.list[prgm->hist.cnt]);
		free(prgm->lines.list[prgm->lines.cnt]);
		prgm->hist.list[prgm->hist.cnt] = prgm->lines.list[prgm->lines.cnt] = NULL;
		break;
	case IN_MAIN:
		prgm->hist.cnt = prgm->lines.cnt = prgm->flags.cnt;
		strmv(0, prgm->b, prgm->hist.list[prgm->hist.cnt]);
		free(prgm->hist.list[prgm->hist.cnt]);
		free(prgm->lines.list[prgm->lines.cnt]);
		prgm->hist.list[prgm->hist.cnt] = prgm->lines.list[prgm->lines.cnt] = NULL;
		break;
	case EMPTY: /* fallthrough */
	default:
		/* revert decrement */
		prgm->flags.cnt++;
	}
}

/* look for current ln in readln history */
void dedup_history(char **restrict ln)
{
	/* return early on empty input */
	if (!ln || !*ln)
		return;
	/* strip leading whitespace */
	char *strip = *ln;
	strip += strspn(strip, " \t");
	/* current entry and forward/backward function pointers  */
	HIST_ENTRY *(*seek_hist[])() = {&previous_history, &next_history};
	/* save current position */
	int hpos = where_history();
	for (size_t i = 0; i < 2; i++) {
		while (history_search_prefix(strip, i - 1) != -1) {
			/* if this line is already in the history, remove the earlier entry */
			HIST_ENTRY *ent = current_history();
			/* HIST_ENTRY *ent = history_get(hpos[1]); */
			if (!ent || !ent->line || strcmp(*ln, ent->line)) {
				/* break if at end of list */
				if (!seek_hist[i]())
					break;
				continue;
			}
			/* free application data */
			ent = remove_history(where_history());
			histdata_t data = free_history_entry(ent);
			if (data)
				free(data);
		}
		history_set_pos(hpos);
	}
	history_set_pos(hpos);
	add_history(strip);
}

void build_body(struct prog_src (*restrict prgm)[], char *restrict ln)
{
	/* sanity check */
	if (!prgm || !ln) {
		WARNX("NULL pointer passed to build_body()");
		return;
	}
	for (size_t i = 0; i < 2; i++) {
		append_str(&(*prgm)[i].lines, ln, 0);
		append_str(&(*prgm)[i].hist, (*prgm)[i].b, 0);
		append_flag(&(*prgm)[i].flags, IN_MAIN);
		strmv(CONCAT, (*prgm)[i].b, "\t");
		strmv(CONCAT, (*prgm)[i].b, ln);
	}
}

void build_funcs(struct prog_src (*restrict prgm)[], char *restrict ln)
{
	/* sanity check */
	if (!prgm || !ln) {
		WARNX("NULL pointer passed to build_funcs()");
		return;
	}
	for (size_t i = 0; i < 2; i++) {
		append_str(&(*prgm)[i].lines, ln, 0);
		append_str(&(*prgm)[i].hist, (*prgm)[i].f, 0);
		append_flag(&(*prgm)[i].flags, NOT_IN_MAIN);
		/* generate function buffers */
		strmv(CONCAT, (*prgm)[i].f, ln);
	}
}

void build_final(struct prog_src (*restrict prgm)[], struct var_list *restrict vlist, char *argv[])
{
	/* sanity check */
	if (!prgm || !argv) {
		WARNX("NULL pointer passed to build_final()");
		return;
	}
	/* finish building current iteration of source code */
	for (size_t i = 0; i < 2; i++) {
		strmv(0, (*prgm)[i].total, (*prgm)[i].f);
		strmv(CONCAT, (*prgm)[i].total, (*prgm)[i].b);
		/* print variable values */
		if (track_flag && i == 1)
			print_vars(vlist, (*prgm)[i].total, cc_argv, argv);
		strmv(CONCAT, (*prgm)[i].total, prog_end);
	}
}
