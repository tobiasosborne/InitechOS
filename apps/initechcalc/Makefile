TOP := $(abspath ../..)
COMPONENT := apps/initechcalc
include $(TOP)/mk/config.mk
# Host-only worksheet sources; target code links the owner's public archive.
CORE := $(TOP)/apps/initech123/core/sheet.c $(TOP)/apps/initech123/core/fmt.c $(TOP)/apps/initech123/fs/wk1.c $(TOP)/apps/initech123/core/formula.c $(TOP)/apps/initech123/core/range.c $(TOP)/apps/initech123/core/eval.c $(TOP)/apps/initech123/core/recalc.c
CORE_INC := -I$(TOP)/apps/initech123/include
TENANT_SRCS := calc.c
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
TENANT_OBJ += $(TOP)/build/apps/initech123/libi123.a
$(OUT)/$(TENANT).base.elf $(OUT)/$(TENANT).probe.elf: $(TOP)/build/apps/initech123/libi123.a
.PHONY: worksheet-library
worksheet-library:
	+$(MAKE) -C $(TOP)/apps/initech123 OUT=$(TOP)/build/apps/initech123 MAKEOVERRIDES= all
$(TOP)/build/apps/initech123/libi123.a: | worksheet-library
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all
	+$(MAKE) -C $(TOP) image
	sh $(TOP)/factory/smoke-desktop.sh
all: $(OUT)/LEDGER.TXT $(OUT)/MICHAEL.CFG
$(OUT)/LEDGER.TXT: data/LEDGER.TXT | $(OUT)
	cp $< $@
$(OUT)/MICHAEL.CFG: data/MICHAEL.CFG | $(OUT)
	cp $< $@
$(HOST_OUT)/ledgergen: tests/calc_ledger_gen.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
.PHONY: ledger-data
ledger-data: $(HOST_OUT)/ledgergen
	@$< include/calc_ledger.json > $(OUT)/ledger.generated.h
	@cmp include/calc_ledger.h $(OUT)/ledger.generated.h
test-build: ledger-data
$(TOP)/build/sdk/host/libtbx_host.a:
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
