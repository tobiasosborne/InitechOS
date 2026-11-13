ifndef INITECH_IMPORT_CHECK_INCLUDED
INITECH_IMPORT_CHECK_INCLUDED := 1
# Opt in after target.mk: include $(TOP)/mk/import-check.mk
# TARGET_ARTIFACTS lists real outputs reachable from all; default TARGET_LIB.
TARGET_ARTIFACTS ?= $(TARGET_LIB)
IMPORT_MUT_PATTERN ?= _(MUT|MUTATE|MUTANT)_
.PHONY: import-check
import-check:
	@mkdir -p $(OUT)
	@$(MAKE) -qp all > $(OUT)/import.make 2>/dev/null || test $$? -eq 1
	@sh $(TOP)/factory/testlib/import-check.sh $(call shquote,$(IMPORT_MUT_PATTERN)) \
	  $(OUT)/import.make $(call shquote,$(TARGET_ARTIFACTS)) \
	  $(call shquote,$(TARGET_INCLUDES) $(HOST_INCLUDES) $(TARGET_CFLAGS) $(TARGET_EXTRA_CFLAGS) $(HOST_CFLAGS) $(foreach t,$(HOST_TESTS),$($(t)_CFLAGS)) $(MAKEFILE_LIST)) $(MAKEFILE_LIST)
test: all import-check
endif
