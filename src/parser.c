/*
 * parser.c - c11 parsing and lexing functions
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>

/* globals */
struct var_table objects;

static char *word_list[] = {
	"auto", "bool", "break", "case", "char", "const", "continue", "default",
	"do", "double", "else", "enum", "extern", "float", "for", "goto",
	"if", "inline", "int", "long", "register", "restrict", "return",
	"short", "signed", "size_t", "sizeof", "static", "struct", "switch",
	"typedef", "union", "unsigned", "void", "volatile", "while",
	"true", "false", "intptr_t", "ptrdiff_t", "intmax_t", "uintmax_t",
	"uint8_t", "uint16_t", "uint32_t", "uint64_t", "char16_t", "char32_t",
	"wchar_t", "alignas", "alignof", "_Atomic", "_Complex", "_Imaginary",
	"_Generic", "_Noreturn", "_Static_assert", "_Thread_local",
	"#pragma", "#include", "#define", "#if", "#ifdef", "#else", "#endif",
	"__VA_ARGS__", "__asm__(", "__attribute__(", "malloc(", "calloc(",
	"free(", "memcpy(", "memset(", "memcmp(", "fread(", "fwrite(",
	"strcat(", "strtok(", "strcpy(", "strlen(", "puts(", "system(",
	"fopen(", "fclose(", "sprintf(", "printf(", "scanf(",
	";att", ";function", ";help", ";intel", ";macro", ";output",
	";parse", ";quit", ";reset", ";tracking", ";undo", ";warnings", NULL
};

size_t pre_process(char *restrict output, char const *restrict src)
{
	/* initial and final preprocessing token count */
	size_t token_cnt[2] = {0};

	(void)word_list, (void)token_cnt;

	return 0;
}
