TOP := $(abspath ../..)
COMPONENT := turbo/seed
include $(TOP)/mk/config.mk
FPC ?= fpc
LIB := token.c lexer.c ast.c parser.c typecheck.c codegen.c
OBJS := $(addprefix $(OUT)/host/,$(LIB:.c=.o))
HOST_TESTS := lexer parser repro fileio-build fileio-fpc fpc-shared
TESTS ?= $(HOST_TESTS)
ifneq ($(filter-out $(HOST_TESTS),$(TESTS)),)
$(error unknown TESTS: $(filter-out $(HOST_TESTS),$(TESTS)))
endif
all: $(OUT)/initechc $(addprefix $(OUT)/rt/,start.o start_dos.o fileio.o)
$(OUT)/host/%.o: %.c $(wildcard *.h) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	@$(HOSTCC) $(HOST_CFLAGS) -I. -c $< -o $@
$(OUT)/initechc: initechc.c $(OBJS)
	@$(HOSTCC) $(HOST_CFLAGS) -I. -o $@ $^
$(OUT)/test_%: tests/test_%.c $(OBJS) $(TOP)/factory/testlib/test_assert.h
	@$(HOSTCC) $(HOST_CFLAGS) -I. -I$(TOP)/factory/testlib -o $@ $< $(OBJS)
$(OUT)/rt/%.o: rt/%.asm
	@mkdir -p $(@D)
	@$(NASM) -f elf32 $< -o $@
.PHONY: tools test-build test-run
tools:
	@command -v $(FPC) >/dev/null 2>&1 || { echo 'FAIL: fpc is required (no skipped reference tests)' >&2; exit 1; }
test-build: tools all $(OUT)/test_lexer $(OUT)/test_parser
test: test-run
test-run: test-build
	@sh $(TOP)/factory/testlib/run-entries.sh $(COMPONENT) "$(OUT)" tests/run.sh $(TESTS)
export FPC NASM TARGET_LD
mutants:
	@echo '17 seed hooks await patch conversion; see tests/MUTANTS-TODO.md' >&2; exit 1
smoke:
	@echo 'Tier 1 requires the product image; see tests/SMOKES.md' >&2; exit 1
