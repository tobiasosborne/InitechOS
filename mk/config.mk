# mk/config.mk -- shared settings. Include first from every component
# Makefile, after setting TOP (path to the repo root) and COMPONENT
# (this component's path from the root, e.g. apps/initech123).
#
# Everything generated goes under $(TOP)/build/<component>/.

ifndef TOP
$(error set TOP before including mk/config.mk)
endif
ifndef COMPONENT
$(error set COMPONENT before including mk/config.mk)
endif

SHELL := /bin/sh
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
MAKEFLAGS += --no-builtin-rules --no-builtin-variables

# Override only for isolated builds (for example the mutation runner).
OUT ?= $(TOP)/build/$(COMPONENT)

# Host tools: tests and factory programs run on the build machine.
HOSTCC      ?= cc
HOST_CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L

# Target tools: freestanding 32-bit x86 for the artifact. Host gcc in
# freestanding mode for now; point TARGET_CC at an i686-elf cross
# compiler when one is installed. -march=i386: no cmov, no SSE (a 486
# has neither; Bochs faults on them).
TARGET_CC      ?= gcc
TARGET_CFLAGS  ?= -m32 -march=i386 -std=c11 -Os -ffreestanding -nostdlib \
                  -fno-stack-protector -fno-pic -fno-asynchronous-unwind-tables \
                  -Wall -Wextra -Werror
TARGET_LD      ?= ld
TARGET_LDFLAGS ?= -m elf_i386 --build-id=none
NASM           ?= nasm

# Reference corpora captured from real period software live outside the
# repository until they are pinned under goldens/. Override
# INITECH_CORPUS, or the individual variables, to relocate them.
INITECH_CORPUS       ?= $(HOME)/Projects
SYSTEM7_DECOMP       ?= $(INITECH_CORPUS)/system7-decomp
WIN31_DECOMP         ?= $(INITECH_CORPUS)/win31-decomp
DBASE3_DECOMP        ?= $(INITECH_CORPUS)/dbase3-decomp
LOTUS123_DECOMP      ?= $(INITECH_CORPUS)/lotus123-decomp
WORDPERFECT51_DECOMP ?= $(INITECH_CORPUS)/wordperfect51-decomp
TURBOPASCAL_DECOMP   ?= $(INITECH_CORPUS)/turbopascal-decomp

$(OUT):
	@mkdir -p $@

# A target a component has not implemented must fail, never succeed
# silently. Components declare their own targets .PHONY.
.PHONY: clean
clean:
	rm -rf "$(OUT)"

# Missing dependency files are normal on a first build.
%.d: ;

.DEFAULT:
	@echo "$(COMPONENT): '$@' is not implemented (or a file is missing)" >&2; exit 1
