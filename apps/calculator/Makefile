TOP := $(abspath ../..)
COMPONENT := apps/calculator
include $(TOP)/mk/config.mk
HOST_INCLUDES := -I. -Iinclude -I$(TOP)/sdk/include -I$(TOP)/sdk/host
HOST_TESTS := test_calcda test_calcda_route
EXTRA_HOST_TESTS := pixels
test_calcda_SRCS := tests/test_calcda.c calc_core.c calc_view.c
test_calcda_LIBS := -lpng -lm
test_calcda_ARGS := include/calcda/calcda_vectors.tsv $(SYSTEM7_DECOMP)
test_calcda_COUNT_PATTERN := ^test_calcda: ([0-9]+) checks.*$$
test_calcda_route_SRCS := tests/test_calcda_route.c calc_core.c calc_view.c
test_calcda_route_LIBS := $(TOP)/build/sdk/host/libtbx_host.a
test_calcda_route_DEPS := $(TOP)/build/sdk/host/libtbx_host.a
pixels_SRCS := tests/ppm_flair_calcda_check.c
pixels_LIBS := -lpng
include $(TOP)/mk/host-test.mk
$(TOP)/build/sdk/host/libtbx_host.a:
	+$(MAKE) -C $(TOP)/sdk host
TENANT := calculator
TENANT_EXE := CALCULAT.EXE
TENANT_SRCS := calcda.c calc_core.c calc_view.c
TENANT_CPPFLAGS := -Iinclude
include $(TOP)/mk/tenant.mk
TARGET_ARTIFACTS := $(TENANT_IMAGE)
include $(TOP)/mk/import-check.mk
include $(TOP)/mk/mutants.mk
smoke: all $(HOST_OUT)/pixels
	sh tests/smoke/journey.sh
.PHONY: references
references:
	@cd $(SYSTEM7_DECOMP) && sha256sum -c $(CURDIR)/tests/REFERENCES.sha256 > /dev/null
test-build: references
