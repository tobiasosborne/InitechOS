TOP := $(abspath ../..)
COMPONENT := apps/initechtext
include $(TOP)/mk/config.mk
FLAIR_INC := -I$(TOP)/flair/include -I$(TOP)/build/flair/generated -I$(TOP)/flair/include/assets -I$(TOP)/flair/region/include
HOST_INCLUDES := -I. -Iinclude -I$(TOP)/sdk/include -I$(TOP)/sdk/host $(FLAIR_INC)
HOST_TESTS := test_itext_route
# Temporary HOST binding: removed when FLAIR publishes its hosted TextEdit library.
# Exact files: flair/textedit.c, flair/scrap.c. Target uses libflair.a below.
test_itext_route_SRCS := tests/test_itext_route.c $(TOP)/flair/textedit.c $(TOP)/flair/scrap.c
test_itext_route_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_itext_route_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
test_itext_route_COUNT_PATTERN := ^test_itext_route: ([0-9]+) checks.*$$
include $(TOP)/mk/host-test.mk
TENANT := text
TENANT_EXE := TEXT.EXE
TENANT_SRCS := itext.c
TENANT_CPPFLAGS := $(FLAIR_INC)
include $(TOP)/mk/tenant.mk
TENANT_OBJ += $(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o
$(OUT)/$(TENANT).base.elf $(OUT)/$(TENANT).probe.elf: $(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o
.PHONY: dependencies
dependencies:
	+$(MAKE) -C $(TOP)/flair OUT=$(TOP)/build/flair MAKEOVERRIDES= all
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
all test-build: dependencies
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all
	+$(MAKE) -C $(TOP) image
	sh $(TOP)/factory/smoke-desktop.sh
$(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o $(TOP)/build/sdk/host/libtbx_host.a: | dependencies
$(TARGET_OBJS) $(foreach t,$(HOST_TESTS),$($(t)_C_OBJS)): | dependencies
