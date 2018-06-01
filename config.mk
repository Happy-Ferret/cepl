#
# config.mk - Makefile configuration variables
#
# AUTHOR: Joey Pabalinas <joeypabalinas@gmail.com>
# See LICENSE.md file for copyright and license details.

# add -Wrestrict if using gcc7 or higher
DEBUG != gcc -v 2>&1 | awk '/version/ {if (substr($$3, 1, 1) > 6) {print "-Wrestrict"}}'

# optional
DESTDIR ?=
PREFIX ?= /usr/local
CC ?= gcc
OLVL ?= -O3
LIBS ?= $(READLINE)/lib/libreadline.a $(READLINE)/lib/libhistory.a 		\
	$(shell pkg-config ncursesw --libs --cflags 2>/dev/null			\
		|| pkg-config ncurses --libs --cflags 2>/dev/null		\
		|| echo '-D_GNU_SOURCE -D_DEFAULT_SOURCE -lncursesw -ltinfo')
ifneq "$(origin LIBS)" "command line"
	CFLAGS += -I$(READLINE)/include
else
	CFLAGS += -DRL_OVERRIDE
endif

# mandatory
LD = $(CC)
DEBUG_CFLAGS = $(CFLAGS) $(DEBUG)
DEBUG_LDFLAGS = $(LDFLAGS) $(DEBUG)
MKALL = $(MKCFG) $(DEP)
OBJ = $(SRC:.c=.o)
TOBJ = $(TSRC:.c=.o)
DEP = $(SRC:.c=.d) $(TSRC:.c=.d)
TEST = $(filter-out $(TAP),$(TSRC:.c=))
UTEST = $(filter-out src/$(TARGET).o,$(SRC:.c=.o))
SRC := $(wildcard src/*.c)
TSRC := $(wildcard t/*.c)
HDR := $(wildcard src/*.h) $(wildcard t/*.h)
ASAN := -fsanitize=address,alignment,leak,undefined
CPPFLAGS := -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -MMD -MP
TARGET := cepl
MANPAGE := cepl.1
COMPLETION := _cepl
TAP := t/tap
BINDIR := bin
MANDIR := share/man/man1
COMPDIR := share/zsh/site-functions
READLINE := readline
ELF_LIBS := -lelf
WARNINGS := -Wall -Wextra -pedantic					\
		-Wcast-align -Wfloat-equal -Wmissing-declarations	\
		-Wmissing-prototypes -Wnested-externs -Wpointer-arith	\
		-Wshadow -Wstrict-overflow -Wwrite-strings
IGNORES := -Wno-conversion -Wno-cpp -Wno-discarded-qualifiers		\
		-Wno-implicit-fallthrough -Wno-inline -Wno-long-long	\
		-Wno-missing-field-initializers -Wno-redundant-decls	\
		-Wno-sign-conversion -Wno-strict-prototypes		\
		-Wno-unused-variable
MKALL += Makefile asan.mk
DEBUG += -O1 -no-pie -D_DEBUG
DEBUG += -fno-builtin -fno-inline -fverbose-asm
CFLAGS += -march=native -g3 -std=c11
CFLAGS += -fPIC -fuse-ld=gold -fuse-linker-plugin
CFLAGS +=  -fno-common -fno-strict-aliasing
CFLAGS += $(WARNINGS) $(IGNORES)
LDFLAGS += -Wl,-O3,-z,relro,-z,now,-z,noexecstack
LDFLAGS += -Wl,--sort-common,--as-needed,--warn-common
LDFLAGS += $(filter-out $(WARNINGS),$(CFLAGS))

# vi:ft=make:
