/*
 * parseopts.h - option parsing
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE file for copyright and license details.
 */

#ifndef PARSEOPTS_H
#define PARSEOPTS_H 1

#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CEPL_VERSION "CEPL v0.6.1"
#define USAGE "[-hpvw] [-c<compiler>] [-l<library>] [-I<include dir>] [-o<output.c>]\n\n\t-h,--help:\t\tShow help/usage information.\n\t-p:\t\t\tAdd symbols from dynamic libraries to readline completion.\n\t-v,--version:\t\tShow version information.\n\t-w:\t\t\tCompile with ”-pedantic-errors -Wall -Wextra” flags.\n\t-c,--compiler:\t\tSpecify alternate compiler.\n\t-l:\t\t\tLink against specified library (flag can be repeated).\n\t-I:\t\t\tSearch directory for header files (flag can be repeated).\n\t-o:\t\t\tName of the file to output source to.\n\nInput lines prefixed with a “;” are used to control internal state.\n\n\t;p[arse]:\t\tToggle -p (shared library parsing) flag\n\t;q[uit]:\t\tExit CEPL\n\t;r[eset]:\t\tReset CEPL to its initial program state\n\t:w[arnings]:\t\tToggle -w (warnings) flag"
/* perl script to pull symbols from shared libraries */
#define ELF_SCRIPT "./elfsyms"
/* pipe buffer size */
#define COUNT sysconf(_SC_PAGESIZE)

/* silence linter */
int getopt(int argc, char * const argv[], const char *optstring);
FILE *fdopen(int fd, const char *mode);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

char *const *parse_opts(int argc, char *argv[], char *const optstring, FILE **ofile);
char **parse_libs(char *libs[]);

static inline int free_argv(char **argv)
{
	int count;
	if (!argv || !argv[0])
		return -1;
	for (count = 0; argv[count]; count++)
		free(argv[count]);
	free(argv);
	return count;
}

#endif
