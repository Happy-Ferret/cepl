/*
 * parser.h - c11 parsing and lexing functions
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#ifndef PARSER_H
#define PARSER_H 1

#include "defs.h"
#include "vars.h"

static inline struct str_list pre_process(char const *restrict src)
{
	size_t token_cnt = 0;
	struct str_list tokens = {0};

	(void)src, (void)token_cnt;

#ifdef _DEBUG
	FOR_EACH_IN(tokens)
		puts(tokens.list[i]);
#endif

	return tokens;
}

#endif
