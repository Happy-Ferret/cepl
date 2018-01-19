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
	START_RULE = 0,
	TRANSLATION_UNIT, COMMENT, EXTERNAL_DECLARATION,
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

/* current symbol */
static size_t sym_idx;
static struct str_list tok_list;

static void next_sym(void)
{
	sym_idx++;
#ifdef _DEBUG
	printf("prev/cur: (%s, %s)\n", tok_list.list[sym_idx - 1], tok_list.list[sym_idx]);
#endif
}

static bool accept(enum sym_type type)
{
	if (type) {
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

static void start_rule(void);
static void translation_unit(void)
{
	(void)tok_list, (void)expect, (void)start_rule;
}

static void start_rule(void)
{
	translation_unit();
}
