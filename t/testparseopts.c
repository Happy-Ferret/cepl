/*
 * t/testparseopts.c - unit-test for parseopts.c
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE file for copyright and license details.
 */

#include "tap.h"
#include "../src/parseopts.h"

int main (void)
{
	FILE *ofile = NULL;
	int argc = 0;
	char *argv[] = {
		"cepl", "-lssl", "-I.",
		"-o/tmp/test", NULL
	};
	char *const optstring = "hvwpl:I:o:";
	char *libs[] = {"cepl", "ssl", "readline", NULL};
	char *const *result;

	for (; argv[argc]; argc++);
	result = parse_opts(argc, argv, optstring, &ofile);
	printf("%s\n%s", "# generated compiler string: ", "# ");
	for (int i = 0; result[i]; (printf("%s ", result[i]), i++));
	putchar('\n');

	plan(3);

	is(result[0], "gcc", "test option parsing.");
	like(result[0], "^gcc$", "test cc_argv[5] matches \"-O2\"");
	ok(parse_libs(libs) != NULL, "test library parsing.");

	done_testing();

	free_argv((char **)result);

}
