TOP := $(abspath ../..)
COMPONENT := flair/region
include $(TOP)/mk/config.mk

TARGET_OUT := $(OUT)/target
TARGET_SRCS := region.c
TARGET_INCLUDES := -Iinclude
TARGET_LIB := $(OUT)/libregion.a
include $(TOP)/mk/target.mk
include $(TOP)/mk/import-check.mk

HOST_TESTS := region region-gdi
EXTRA_HOST_TESTS := region-asan
HOST_INCLUDES := -Iinclude -I$(TOP)/factory/testlib
region_SRCS := tests/test_region.c region.c
region-gdi_SRCS := tests/test_region_gdi.c tests/gdi_ref_wine.c region.c
region-gdi_CFLAGS := -Itests/gdi_ref_winestubs
region-asan_SRCS := $(region_SRCS)
region-asan_CFLAGS := -fsanitize=address -fno-pie -no-pie
region-asan_ENV := ASAN_OPTIONS=abort_on_error=0:detect_leaks=0

include $(TOP)/mk/host-test.mk
include $(TOP)/mk/mutants.mk
TEST_BUDGET_S := 30
CORPORA := REPO
include $(TOP)/mk/corpus.mk

.PHONY: target
target: all
