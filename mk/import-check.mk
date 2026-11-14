ifndef INITECH_IMPORT_CHECK_INCLUDED
INITECH_IMPORT_CHECK_INCLUDED := 1
# Opt in after target.mk: include $(TOP)/mk/import-check.mk
# TARGET_ARTIFACTS lists real outputs reachable from all; default TARGET_LIB.
TARGET_ARTIFACTS ?= $(TARGET_LIB)
IMPORT_MUT_PATTERN ?= _(MUT|MUTATE|MUTANT)_
.PHONY: import-check
import-check: all
	@mkdir -p $(OUT)
	@cd $(TOP) && sh factory/testlib/dependency-query.sh $(TOP) $(COMPONENT) '$(MAKE)' $(OUT)/import.make
	@IMPORT_ROOT=$(TOP) IMPORT_COMPONENT=$(COMPONENT) IMPORT_OUT=$(OUT)/interface-audit \
	 sh $(TOP)/factory/testlib/import-check.sh $(call shquote,$(IMPORT_MUT_PATTERN)) \
	  $(OUT)/import.make.parent $(call shquote,$(TARGET_ARTIFACTS)) \
	  $(call shquote,$(TARGET_INCLUDES) $(HOST_INCLUDES) $(TARGET_CFLAGS) $(TARGET_EXTRA_CFLAGS) $(HOST_CFLAGS) $(foreach t,$(HOST_TESTS),$($(t)_CFLAGS)) $(MAKEFILE_LIST)) $(call shquote,$(IMPORT_PRODUCT_SRCS)) $(MAKEFILE_LIST)
test: all import-check
endif
