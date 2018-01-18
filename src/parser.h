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

struct str_list pre_process(char const *restrict src);

#endif
