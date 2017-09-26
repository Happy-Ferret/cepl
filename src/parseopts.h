/*
 * parseopts.h - option parsing
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#ifndef PARSEOPTS_H
#define PARSEOPTS_H

#include "defs.h"
#include "errs.h"
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* prototypes */
char **parse_opts(int argc, char *argv[], char const optstring[], FILE volatile **ofile);
void read_syms(struct str_list *tokens, char const *elf_file);
void parse_libs(struct str_list *symbols, char *libs[]);

#endif
