# Local journey frontend; shared driver files stay owned by the factory owner.
TOP := $(abspath .)
COMPONENT := factory
include $(TOP)/mk/config.mk
all: $(OUT)/desktop-bochs
$(OUT)/desktop-bochs: factory/desktop-bochs.c $(wildcard factory/emu/bochs*) factory/emu/rfb_port.h FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
all: $(OUT)/desktop-primitives-test
$(OUT)/desktop-primitives-test: factory/desktop-primitives-test.c factory/desktop-bochs.c $(wildcard factory/emu/bochs*) factory/emu/rfb_port.h FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
.PHONY: test
test: all
	@$(OUT)/desktop-primitives-test
	@$(OUT)/desktop-wait-test
	@$(OUT)/desktop-key-test
all: $(OUT)/desktop-wait-test
$(OUT)/desktop-wait-test: factory/desktop-wait-test.c factory/desktop-bochs.c $(wildcard factory/emu/bochs*) factory/emu/rfb_port.h FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
all: $(OUT)/desktop-key-test
$(OUT)/desktop-key-test: factory/desktop-key-test.c factory/desktop-bochs.c $(wildcard factory/emu/bochs*) factory/emu/rfb_port.h FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
