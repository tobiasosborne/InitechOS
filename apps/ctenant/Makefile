TOP := $(abspath ../..)
COMPONENT := apps/ctenant
include $(TOP)/mk/config.mk
HOST_INCLUDES := -I$(TOP)/sdk/include -I$(TOP)/sdk/host -I$(TOP)/factory/testlib
HOST_TESTS := test_route test_files
test_files_SRCS := tests/test_files.c
test_files_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_files_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
test_route_SRCS := tests/test_route.c
test_route_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_route_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
include $(TOP)/mk/host-test.mk
$(TOP)/build/sdk/host/libtbx_host.a:
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
TENANT := ctenant
TENANT_EXE := CTENANT.EXE
TENANT_SRCS := ctenant.c
ifneq ($(FILE_GATE),)
TENANT := filegate
TENANT_EXE := FILETEN.EXE
TENANT_SRCS := tests/file_gate.c
endif
include $(TOP)/mk/tenant.mk
ifeq ($(FILE_GATE),)
.PHONY: file-example
all: file-example
file-example:
	+$(MAKE) OUT=$(OUT)/filegate FILE_GATE=1 all
endif
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke:
	@echo 'No emulator journey imported for this SDK example' >&2; exit 1
