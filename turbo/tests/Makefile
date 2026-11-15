TOP := $(abspath ../..)
COMPONENT := turbo/tests
include $(TOP)/mk/config.mk
HOST_TESTS := verdict progress-plan
verdict_DEPS := verdict-test.sh entry.sh checks.sh wrap.sh
progress-plan_DEPS := progress-plan-test.sh execution/keys.sh checks.sh wrap.sh
include $(TOP)/mk/host-test.mk
include $(TOP)/mk/mutants.mk
.PHONY: seed-tools
seed-tools:
	+@$(MAKE) --no-print-directory -C $(TOP)/turbo/seed OUT=$(TOP)/build/turbo/seed TESTS= MAKEOVERRIDES= all
test-build: seed-tools
$(HOST_OUT)/verdict: private command = sh wrap.sh $@ $(TOP) $(CURDIR) $(OUT) $(HOST_OUT)
$(HOST_OUT)/progress-plan: private command = sh wrap.sh $@ $(TOP) $(CURDIR) $(OUT) $(HOST_OUT) progress-plan
all: test-build
smoke:
	@echo 'Factory assertion controls are host-only; no emulator target.' >&2; exit 1

.PHONY: controls
controls: seed-tools
	@env TOP="$(TOP)" sh noop-control.sh "$(OUT)" "$(CURDIR)"
