/*
 * parser.c - c11 parsing and lexing functions
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>

/* symbol types */
enum sym_type {
	TRANSLATION_UNIT,
};

/* globals */
struct var_table objects;

/* tables */
char const *symbols[] = {
	[TRANSLATION_UNIT] = "translation_unit",
};

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

static void next_sym(struct str_list *restrict tok_list)
{
	(void)tok_list;
}

static bool accept(struct str_list *restrict tok_list, enum sym_type type)
{
	if (true) {
		next_sym(tok_list);
		return true;
	}
	return false;
}

static bool expect(struct str_list *restrict tok_list, enum sym_type type)
{
	if (accept(tok_list, type))
		return true;
	WARNX("%s: %s", "failure parsing symbol", symbols[type]);
	return false;
}

struct str_list pre_process(char const *restrict src)
{
	size_t token_cnt = 0;
	struct str_list tokens = {0};

	(void)word_list, (void)src, (void)token_cnt, (void)expect;

#ifdef _DEBUG
	FOR_EACH(tokens)
		puts(tokens.list[i]);
#endif

	return tokens;
}
