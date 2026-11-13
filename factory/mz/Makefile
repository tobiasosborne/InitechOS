TOP := $(abspath ../..)
COMPONENT := factory/mz
include $(TOP)/mk/config.mk
all: $(OUT)/mzlink $(OUT)/mzcheck
$(OUT)/mzlink: mzlink.c $(TOP)/sdk/include/initechmz_extended.h $(MAKEFILE_LIST) | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) -I$(TOP)/sdk/include $< -o $@
$(OUT)/mzcheck: mzcheck.c $(MAKEFILE_LIST) | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) -I$(TOP)/sdk/include $< -o $@
$(OUT)/close-fault: mzlink.c tests/mzlink_close_fault.c $(TOP)/sdk/include/initechmz_extended.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) -I$(TOP)/sdk/include mzlink.c tests/mzlink_close_fault.c -Wl,--wrap=fclose -o $@
test: all $(OUT)/close-fault
	sh tests/test_mzlink_io.sh $(OUT)/mzlink $(OUT)/close-fault $(OUT)/io
	sh tests/structure.sh $(OUT)/mzlink $(OUT)/mzcheck $(OUT)/structure
