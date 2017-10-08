# CEPL - *C Read-Eval-Print Loop*

An interactive C (ISO/IEC 9899:2011) read–eval–print loop (REPL) currently supporting multiple compilers, shell-esque readline key-bindings/tab-completion, and incremental undo.

## Dependencies

* gcc (GNU C Compiler)
* readline library
* libelf ELF access library

## Usage
```bash
./cepl [-hptvw] [-c<compiler>] [-l<library name>] [-I<include dir>] [-o<output.c>]
```

Run `make` then `./cepl` to start the interactive REPL.

Command history is read from and saved to `~/.cepl_history`.

#### CEPL understands the following control sequences:

Input lines prefixed with a `;` are used to control internal state.

	;f[unction]:		Define a function (e.g. “;f void foo(void) { … }”)
	;h[elp]:		Show help
	;i[nclude]:		Define an include (e.g. “;i #include <crypt.h>”)
	;m[acro]:		Define a macro (e.g. “;m #define ZERO(x) (x ^ x)”)
	;o[utput]:		Toggle -o (C code file) flag
	;p[arse]:		Toggle -p (shared library parsing) flag
	;q[uit]:		Exit CEPL
	;s[ource]:		Toggle -s (assembler code file) flag
	;r[eset]:		Reset CEPL to its initial program state
	;t[racking]:		Toggle variable tracking.
	;u[ndo]:		Incremental undo (can be repeated).
	;w[arnings]:		Toggle -w (warnings) flag

#### CEPL understands the following options:

	-h,--help:		Show help/usage information
	-p,--parse:		Disable addition of dynamic library symbols to readline completion
	-t,--tracking:		Toggle variable tracking
	-v,--version:		Show version information
	-w,--warnings:		Compile with “-pedantic-errors -Wall -Wextra” flags
	-c,--compiler:		Specify alternate compiler
	-l:			Link against specified library (flag can be repeated)
	-I:			Search directory for header files (flag can be repeated)
	-o:			Name of the file to write C code to
	-s:			Name of the file to write assembler code to

## Libraries used:

* libtap ([zorgnax/libtap](https://github.com/zorgnax/libtap))
* libelf
* libreadline
