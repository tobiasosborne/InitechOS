TOP := $(abspath ../..)
COMPONENT := apps/controlpanels
include $(TOP)/mk/config.mk
HOST_INCLUDES := -I$(TOP)/factory/testlib -I. -I$(TOP)/sdk/include -I$(TOP)/sdk/host -I$(TOP)/flair/include -I$(TOP)/flair/region/include -I$(TOP)/build/flair/generated -I$(TOP)/flair/include/assets
HOST_TESTS := test_cpanels test_route
# Temporary hosted library/adapter access: remove when FLAIR publishes a hosted
# Dialog library and cp_dialog_items.h. Exact sources are listed here.
HOST_INCLUDES += -I$(TOP)/flair -I$(TOP)/flair/tests/render
HOST_FLAIR := event dialog control window surface blitter text chrome heap flair_look
HOST_DIALOG := $(addprefix $(TOP)/flair/,$(addsuffix .c,$(HOST_FLAIR))) $(TOP)/flair/region/region.c $(TOP)/flair/tests/render/render.c
test_cpanels_SRCS := tests/test_cpanels.c core.c $(HOST_DIALOG)
test_cpanels_LIBS := -lpng
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
.PHONY: host-dialog-assets
host-dialog-assets:
	+$(MAKE) -C $(TOP)/flair OUT=$(TOP)/build/flair MAKEOVERRIDES= all
$(foreach t,$(HOST_TESTS),$($(t)_C_OBJS)): | host-dialog-assets
