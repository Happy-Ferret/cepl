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
	START_RULE, TRANSLATION_UNIT, COMMENT, EXTERNAL_DECLARATION,
	FUNCTION_DEFINITION, DECLARATION, DECLARATION_SPECIFIERS,
	DECLARATOR, COMPOUND_STATEMENT, BLOCK_ITEM_LIST, BLOCK_ITEM,
	STATEMENT, STATEMENT_LIST, JUMP_STATEMENT, ITERATION_STATEMENT,
	SELECTION_STATEMENT, LABELED_STATEMENT, EXPRESSION_STATEMENT,
};

/* globals */
struct var_table objects;

/* tables */
char const *symbols[] = {
	[START_RULE] = "START_RULE",
	/*
	 *   TRANSLATION_UNIT
	 * + START_RULE (?)
	 */
	[TRANSLATION_UNIT] = "TRANSLATION_UNIT",
	/*
	 *   COMMENT
	 * | EXTERNAL_DECLARATION
	 * | FUNCTION_DEFINITION
	 */
	[COMMENT] = "COMMENT",
	[EXTERNAL_DECLARATION] = "EXTERNAL_DECLARATION",
	/*
	 *   DECLARATION
	 */
	[FUNCTION_DEFINITION] = "FUNCTION_DEFINITION",
	/*
	 *   DECLARATION_SPECIFIERS[CONTEXT => 'FUNCTION DEFINITION'](?)
	 * + DECLARATOR[CONTEXT => 'FUNCTION DEFINITION']
	 * + COMPOUND_STATEMENT[CONTEXT => 'FUNCTION DEFINITION STATEMENT'](?)
	 */
	[DECLARATION_SPECIFIERS] = "DECLARATION_SPECIFIERS",
	[DECLARATOR] = "DECLARATOR",
	[COMPOUND_STATEMENT] = "COMPOUND_STATEMENT",
	/*
	 *   '{'
	 * + block_item_list(s?)
	 * + '}'
	 */
	[DECLARATION] = "DECLARATION",
	[BLOCK_ITEM_LIST] = "BLOCK_ITEM_LIST",
	/*
	 *   BLOCK_ITEM(s)
	 */
	[BLOCK_ITEM] = "BLOCK_ITEM",
	/*
	 *   DECLARATION
	 * | STATEMENT[CONTEXT => "$ARG{CONTEXT}|BLOCK ITEM"]
	 * | COMMENT
	 */
	[STATEMENT] = "STATEMENT",
	/*
	 *   JUMP_STATEMENT
	 * | COMPOUND_STATEMENT
	 * | ITERATION_STATEMENT
	 * | SELECTION_STATEMENT
	 * | LABELED_STATEMENT
	 * | EXPRESSION_STATEMENT
	 */
	[STATEMENT_LIST] = "STATEMENT_LIST",
	/*
	 *   COMMENT(?)
	 * + STATEMENT[CONTEXT => UNDEF]
	 * + STATEMENT_LIST(?)
	 */
	[JUMP_STATEMENT] = "JUMP_STATEMENT",
	[ITERATION_STATEMENT] = "ITERATION_STATEMENT",
	[SELECTION_STATEMENT] = "SELECTION_STATEMENT",
	[LABELED_STATEMENT] = "LABELED_STATEMENT",
	[EXPRESSION_STATEMENT] = "EXPRESSION_STATEMENT",
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

/* current symbol */
static char **sym_list;
/* symbol indices */
static size_t sym_idx, max_idx;

static void next_sym(void)
{
#ifdef _DEBUG
	puts(sym_list[sym_idx]);
#endif
	sym_idx++;
}

static bool accept(enum sym_type type)
{
	if (true) {
		next_sym();
		return true;
	}
	return false;
}

static bool expect(enum sym_type type)
{
	if (accept(type))
		return true;
	WARNX("%s: %s", "failure parsing symbol", symbols[type]);
	return false;
}

static void translation_unit(void)
{
	(void)word_list, (void)expect;
}

static void start_rule(struct str_list *restrict tok_list)
{
	sym_idx = 0;
	max_idx = tok_list->cnt;
	sym_list = tok_list->list;
	translation_unit();
}
