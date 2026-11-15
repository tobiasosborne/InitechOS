TOP := $(abspath ../..)
COMPONENT := turbo/ide
include $(TOP)/mk/config.mk
FPC ?= fpc
SOURCES := tests/head.pas screen.pas buffer.pas editor.pas tests/dos.pas tests/host.pas
FPC_SOURCES := tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/fpc.pas tests/host.pas
TOOLS := $(OUT)/idehost.pas $(OUT)/idehost $(OUT)/keys $(OUT)/screens
# Each entry is tests/run.sh ENTRY (README.md); the wrapper keeps the shared
# host-test status protocol, so mutants are graded like any host program.
HOST_TESTS := journey paging page-failure save-failure save-identity seed turbo one-program
$(foreach t,$(HOST_TESTS),$(eval $(t)_DEPS := tests/run.sh tests/wrap.sh $$(TOOLS)))
CORPORA := TURBOPASCAL_DECOMP
include $(TOP)/mk/host-test.mk
include $(TOP)/mk/corpus.mk
export TOP
include $(TOP)/mk/mutants.mk

.PHONY: tools
tools:
	@command -v $(FPC) >/dev/null 2>&1 || { echo 'FAIL: fpc is required' >&2; exit 1; }
$(OUT)/idehost.pas: $(SOURCES)
	@mkdir -p $(@D)
	cat $(SOURCES) > $@
$(OUT)/idehost-fpc.pas: $(FPC_SOURCES)
	@mkdir -p $(@D)
	cat $(FPC_SOURCES) > $@
$(OUT)/idehost: $(OUT)/idehost-fpc.pas | tools
	@$(FPC) -B -O- -v0 -FU$(OUT) -FE$(OUT) -o$@ $< > $(OUT)/fpc.log 2>&1 || { cat $(OUT)/fpc.log; exit 1; }
$(OUT)/keys: tests/keys.c
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/screens: tests/screens.c
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
all: $(TOOLS)
$(addprefix $(HOST_OUT)/,$(HOST_TESTS)): | $(HOST_OUT)
$(addprefix $(HOST_OUT)/,$(HOST_TESTS)): private command = sh tests/wrap.sh $@ $(TOP) $(TURBOPASCAL_DECOMP) $(CURDIR) $(OUT) $(notdir $@)
test-build: pinned-corpus
