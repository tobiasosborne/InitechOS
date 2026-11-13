# Shared disk tenant build. Include config.mk first.
# TENANT: intermediate basename; TENANT_SRCS: component-relative C files;
# TENANT_EXE: on-disk name; TENANT_CPPFLAGS: semantic defines/local includes;
# TENANT_RESOURCES: full output paths, with app-supplied rules (e.g. 123.MNU).
# No size-driven tuning and no OS-side file-size guard here.
TARGET_OBJCOPY ?= objcopy
TARGET_NM ?= nm
# Low byte nonzero for the diff; aligned to 128 so normal C data alignment
# produces the same section layout at both link bases.
TENANT_PROBE := 0x01010180
TENANT_OBJ := $(addprefix $(OUT)/tenant/,$(TENANT_SRCS:.c=.o)) $(OUT)/tenant/tbx_rt.o
TENANT_HEADERS := $(wildcard $(TOP)/sdk/include/*.h)
MZ_OUT := $(TOP)/build/factory/mz
TENANT_IMAGE := $(OUT)/$(TENANT_EXE)

all: $(TENANT_IMAGE) $(TENANT_RESOURCES)
$(OUT)/tenant/%.o: %.c $(TENANT_HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TENANT_CPPFLAGS) -I$(TOP)/sdk/include -MMD -MP -c $< -o $@
$(OUT)/tenant/tbx_rt.o: $(TOP)/sdk/rt/tbx_rt.c $(TENANT_HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -I$(TOP)/sdk/include -MMD -MP -c $< -o $@
$(OUT)/$(TENANT).base.elf: $(TENANT_OBJ) $(TOP)/sdk/rt/tenant.ld $(MAKEFILE_LIST)
	$(TARGET_LD) $(TARGET_LDFLAGS) -T $(TOP)/sdk/rt/tenant.ld --defsym=TBX_BASE=0 -o $@ $(TENANT_OBJ)
$(OUT)/$(TENANT).probe.elf: $(TENANT_OBJ) $(TOP)/sdk/rt/tenant.ld $(MAKEFILE_LIST)
	$(TARGET_LD) $(TARGET_LDFLAGS) -T $(TOP)/sdk/rt/tenant.ld --defsym=TBX_BASE=$(TENANT_PROBE) -o $@ $(TENANT_OBJ)
$(OUT)/$(TENANT).%.bin: $(OUT)/$(TENANT).%.elf
	$(TARGET_OBJCOPY) -O binary $< $@
.PHONY: tenant-tools tenant-check
tenant-tools:
	+$(MAKE) -C $(TOP)/factory/mz all
$(TENANT_IMAGE): $(OUT)/$(TENANT).base.bin $(OUT)/$(TENANT).probe.bin $(wildcard $(TOP)/factory/mz/*.c $(TOP)/factory/mz/*.h) $(TOP)/factory/mz/Makefile | tenant-tools
	@set -eu; \
	entry=$$($(TARGET_NM) $(OUT)/$(TENANT).base.elf | awk '$$3=="tbx_entry"{print $$1}'); \
	test "$$entry" = 00000000 || { echo "tbx_entry is $$entry, expected 0" >&2; exit 1; }; \
	bss=$$($(TARGET_NM) $(OUT)/$(TENANT).base.elf | awk '$$3=="__bss_len"{print $$1}'); \
	paras=$$(( (0x$$bss+15)/16 )); \
	$(MZ_OUT)/mzlink $(OUT)/$(TENANT).base.bin $@ --reloc-diff $(OUT)/$(TENANT).probe.bin $(TENANT_PROBE) --header-relocs --entry 0 --minalloc $$paras; \
	$(MZ_OUT)/mzcheck $@ $(OUT)/$(TENANT).base.bin $$((0x$$bss)); \
	echo "$(TENANT_EXE): $$(wc -c < $@) bytes (OS loader limits documented in sdk/README.md)"
tenant-check: $(TENANT_IMAGE) | tenant-tools
	$(MZ_OUT)/mzcheck $(TENANT_IMAGE) $(OUT)/$(TENANT).base.bin $$((0x$$($(TARGET_NM) $(OUT)/$(TENANT).base.elf | awk '$$3=="__bss_len"{print $$1}')))
-include $(TENANT_OBJ:.o=.d)
# Retain the paired binary/ELF evidence, including on failed wrapping.
.SECONDARY: $(OUT)/$(TENANT).base.bin $(OUT)/$(TENANT).probe.bin
