TOP := $(abspath ../..)
COMPONENT := apps/initechpaint
include $(TOP)/mk/config.mk
HOST_INCLUDES := -I$(TOP)/flair/include -I$(TOP)/apps/initechcalc/include -I$(TOP)/sdk/include -I$(TOP)/sdk/host -I$(TOP)/factory/testlib
HOST_TESTS := test_paint_tenant
test_paint_tenant_COUNT_PATTERN := ^test_paint_tenant: ([0-9]+) checks.*$$
test_paint_tenant_SRCS := tests/test_paint_tenant.c
test_paint_tenant_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_paint_tenant_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
EXTRA_HOST_TESTS := pixels
pixels_SRCS := tests/ppm_flair_paint_check.c
pixels_CFLAGS := -I$(TOP)/flair/tests/goldens
include $(TOP)/mk/host-test.mk
$(TOP)/build/sdk/host/libtbx_host.a:
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
TENANT := initechpaint
TENANT_EXE := PAINT.EXE
TENANT_SRCS := paint.c
TENANT_CPPFLAGS := -I$(TOP)/flair/include
include $(TOP)/mk/tenant.mk
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all $(HOST_OUT)/pixels
	+$(MAKE) -C $(TOP) image
	sh $(TOP)/factory/smoke-desktop.sh
