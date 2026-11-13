TOP := $(abspath ../..)
COMPONENT := apps/initechcalc
include $(TOP)/mk/config.mk
# Temporary: remove this entire block when the Initech 123 owner publishes
# its target worksheet archive. The orchestrator switches this at wave merge.
# Exact files currently required, compiled only because no library exists yet.
CORE := $(TOP)/apps/initech123/core/sheet.c $(TOP)/apps/initech123/core/fmt.c $(TOP)/apps/initech123/fs/wk1.c $(TOP)/apps/initech123/core/formula.c $(TOP)/apps/initech123/core/range.c $(TOP)/apps/initech123/core/eval.c $(TOP)/apps/initech123/core/recalc.c
CORE_INC := -I$(TOP)/apps/initech123/include
TENANT_SRCS := calc.c $(CORE)
# End temporary worksheet-core block.
HOST_INCLUDES := -Iinclude $(CORE_INC) -I$(TOP)/sdk/include -I$(TOP)/sdk/host
HOST_TESTS := test_calc_tenant test_calc_file_copy
$(foreach t,$(HOST_TESTS),$(eval $(t)_SRCS := tests/$(t).c $(CORE)))
$(foreach t,$(HOST_TESTS),$(eval $(t)_LIBS := $(TOP)/build/sdk/host/libtbx_host.a))
$(foreach t,$(HOST_TESTS),$(eval $(t)_DEPS := $(TOP)/build/sdk/host/libtbx_host.a))
test_calc_tenant_COUNT_PATTERN := ^test_calc_tenant: ([0-9]+) checks.*$$
test_calc_file_copy_COUNT_PATTERN := ^test_calc_file_copy: ([0-9]+) checks.*$$
include $(TOP)/mk/host-test.mk
TENANT := calc
TENANT_EXE := CALC.EXE
TENANT_CPPFLAGS := -Iinclude $(CORE_INC)
include $(TOP)/mk/tenant.mk
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all
	sh tests/smoke/journey.sh
all: $(OUT)/LEDGER.TXT $(OUT)/MICHAEL.CFG
$(OUT)/LEDGER.TXT: data/LEDGER.TXT | $(OUT)
	cp $< $@
$(OUT)/MICHAEL.CFG: data/MICHAEL.CFG | $(OUT)
	cp $< $@
