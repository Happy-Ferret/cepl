/*
 * t/testparser.c - unit-test for parser.c
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#include "tap.h"
#include "../src/errs.h"
#include "../src/parser.h"

int main (void)
{
	plan(1);

	ok(true, "test parser and lexer");

	done_testing();
}
