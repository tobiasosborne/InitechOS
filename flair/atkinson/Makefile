TOP := ../..
COMPONENT := flair/atkinson
include $(TOP)/mk/config.mk

HOST_TESTS := region region-gdi
EXTRA_HOST_TESTS := region-asan
HOST_INCLUDES := -Iinclude -I$(TOP)/factory/testlib
TEST_HDRS := $(shell find include tests -name '*.h') $(wildcard $(TOP)/factory/testlib/*.h)
region_SRCS := tests/test_region.c
region_OBJS := $(OUT)/region.o
region-gdi_SRCS := tests/test_region_gdi.c tests/gdi_ref_wine.c
region-gdi_OBJS := $(OUT)/region.o
region-gdi_CFLAGS := -Itests/gdi_ref_winestubs -DWIN31_DECOMP=$(abspath $(WIN31_DECOMP))
region-gdi_DEPS := $(wildcard $(WIN31_DECOMP)/oracles/wine/server/*.h) \
                   $(wildcard $(WIN31_DECOMP)/oracles/wine/server/region.c)
region-asan_SRCS := $(region_SRCS)
region-asan_OBJS := $(OUT)/region-asan.o
region-asan_CFLAGS := -fsanitize=address -fno-pie -no-pie
region-asan_ENV := ASAN_OPTIONS=abort_on_error=0:detect_leaks=0

$(OUT)/region.o $(OUT)/region-asan.o: region.c $(wildcard include/*.h) $(MAKEFILE_LIST) | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) -I. -Iinclude $(if $(filter %/region-asan.o,$@),-fsanitize=address -fno-pie) -c region.c -o $@

include $(TOP)/mk/host-test.mk
include $(TOP)/mk/mutants.mk
