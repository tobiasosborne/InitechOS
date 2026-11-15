TOP := $(abspath ../..)
COMPONENT := turbo/tests
include $(TOP)/mk/config.mk
HOST_TESTS := verdict
verdict_DEPS := verdict-test.sh entry.sh checks.sh wrap.sh
include $(TOP)/mk/host-test.mk
include $(TOP)/mk/mutants.mk
.PHONY: seed-tools
seed-tools:
	+@$(MAKE) --no-print-directory -C $(TOP)/turbo/seed OUT=$(TOP)/build/turbo/seed TESTS= MAKEOVERRIDES= all
test-build: seed-tools
$(HOST_OUT)/verdict: private command = sh wrap.sh $@ $(TOP) $(CURDIR) $(OUT) $(HOST_OUT)
all: test-build
smoke:
	@echo 'Factory assertion controls are host-only; no emulator target.' >&2; exit 1
