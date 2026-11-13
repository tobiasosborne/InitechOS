TOP := $(abspath ../..)
COMPONENT := apps/notepad
include $(TOP)/mk/config.mk
FLAIR_INC := -I$(TOP)/flair/include -I$(TOP)/build/flair/generated -I$(TOP)/flair/include/assets -I$(TOP)/flair/region/include
HOST_INCLUDES := -I. -Iinclude -I$(TOP)/sdk/include -I$(TOP)/sdk/host $(FLAIR_INC)
HOST_TESTS := test_notepad_route test_scrapbook_route
# Temporary HOST binding: removed when FLAIR publishes its hosted TextEdit library.
# Exact files: flair/textedit.c, flair/scrap.c. Target uses libflair.a below.
HOST_TEXTEDIT := $(TOP)/flair/textedit.c $(TOP)/flair/scrap.c
test_notepad_route_SRCS := tests/test_notepad_route.c $(HOST_TEXTEDIT)
test_scrapbook_route_SRCS := $(test_notepad_route_SRCS)
test_scrapbook_route_CFLAGS := -DSCRAPBOOK
$(foreach t,$(HOST_TESTS),$(eval $(t)_LIBS := $(TOP)/build/sdk/host/libtbx_host.a))
$(foreach t,$(HOST_TESTS),$(eval $(t)_DEPS := $(TOP)/build/sdk/host/libtbx_host.a))
$(foreach t,$(HOST_TESTS),$(eval $(t)_COUNT_PATTERN := ^test_notepad_route[^:]*: ([0-9]+) checks.*$$))
include $(TOP)/mk/host-test.mk
TENANT := notepad
TENANT_EXE := NOTEPAD.EXE
TENANT_SRCS := notepad.c
TENANT_CPPFLAGS := -Iinclude $(FLAIR_INC) $(if $(SCRAPBOOK),-DSCRAPBOOK)
ifneq ($(SCRAPBOOK),)
TENANT := scrapbook
TENANT_EXE := SCRAPBK.EXE
endif
include $(TOP)/mk/tenant.mk
TENANT_OBJ += $(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o
$(OUT)/$(TENANT).base.elf $(OUT)/$(TENANT).probe.elf: $(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o
.PHONY: dependencies scrapbook
# Libraries are built by their owners; never recompile their target sources.
dependencies:
	+$(MAKE) -C $(TOP)/flair OUT=$(TOP)/build/flair MAKEOVERRIDES= all
	+$(MAKE) -C $(TOP)/sdk OUT=$(TOP)/build/sdk MAKEOVERRIDES= host
ifeq ($(DEPENDENCIES_READY),)
all test-build: dependencies
endif
ifeq ($(SCRAPBOOK),)
all: scrapbook
scrapbook: dependencies
	+$(MAKE) SCRAPBOOK=1 DEPENDENCIES_READY=1 OUT=$(OUT)/scrapbook all
endif
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all
	sh tests/smoke/journey.sh
$(HOST_OUT)/readback: tests/readback.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
smoke: $(HOST_OUT)/readback
