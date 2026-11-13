TOP := $(abspath ../..)
COMPONENT := apps/fonts
include $(TOP)/mk/config.mk
HOST_INCLUDES := -I$(TOP)/sdk/include -I$(TOP)/sdk/host -I$(TOP)/factory/testlib
HOST_TESTS := test_route
test_route_SRCS := tests/test_route.c
test_route_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_route_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
include $(TOP)/mk/host-test.mk
$(TOP)/build/sdk/host/libtbx_host.a:
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
TENANT := fonts
TENANT_EXE := FONTS.EXE
TENANT_SRCS := fonts.c
include $(TOP)/mk/tenant.mk
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke:
	@echo 'No emulator journey imported for this SDK example' >&2; exit 1
# Optional legacy pixel checker; font-engine host suites remain owned by FLAIR.
$(HOST_OUT)/pixels: tests/ppm_flair_fonts_check.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -I$(TOP)/build/flair/generated -I$(TOP)/flair/include/assets -I$(TOP)/flair/tests/goldens $< -o $@)
