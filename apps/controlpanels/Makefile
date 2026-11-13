TOP := $(abspath ../..)
COMPONENT := apps/controlpanels
include $(TOP)/mk/config.mk
HOST_INCLUDES := -I. -Iinclude -I$(TOP)/sdk/include -I$(TOP)/sdk/host -I$(TOP)/flair/include -I$(TOP)/flair/region/include -I$(TOP)/factory/include
HOST_TESTS := test_cpanels test_route
test_cpanels_SRCS := tests/test_cpanels.c core.c
test_cpanels_LIBS := $(TOP)/build/flair/host/libdialog.a -lpng
test_cpanels_DEPS := $(TOP)/build/flair/host/libdialog.a
test_cpanels_ARGS := $(SYSTEM7_DECOMP)
test_cpanels_COUNT_PATTERN := ^test_cpanels: ([0-9]+) checks.*$$
test_route_SRCS := tests/test_route.c core.c
test_route_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_route_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
include $(TOP)/mk/host-test.mk
TENANT := cpanels
TENANT_EXE := CPANELS.EXE
TENANT_SRCS := cpanels.c core.c
include $(TOP)/mk/tenant.mk
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all
	+$(MAKE) -C $(TOP) image
	sh $(TOP)/factory/smoke-desktop.sh
CORPORA := SYSTEM7_DECOMP
include $(TOP)/mk/corpus.mk
.PHONY: references
references:
	@cd $(SYSTEM7_DECOMP) && sha256sum -c $(CURDIR)/tests/REFERENCES.sha256 > /dev/null
test-build: references
$(TOP)/build/sdk/host/libtbx_host.a:
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
.PHONY: host-dialog boundary-check
host-dialog:
	+$(MAKE) -C $(TOP)/flair OUT=$(TOP)/build/flair TESTS= MAKEOVERRIDES= host-dialog
$(TOP)/build/flair/host/libdialog.a: | host-dialog
boundary-check: test-build
	sh tests/boundary.sh "$(TOP)" "$(HOST_CFLAGS) $(HOST_INCLUDES) $(TARGET_CFLAGS) $(TENANT_CPPFLAGS) $(foreach t,$(HOST_TESTS),$($(t)_CFLAGS))" $(foreach t,$(HOST_TESTS),$($(t)_C_OBJS:.o=.d))
test: boundary-check
