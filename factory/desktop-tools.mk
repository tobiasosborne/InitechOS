# Local journey frontend; shared driver files stay owned by the factory owner.
TOP := $(abspath .)
COMPONENT := factory
include $(TOP)/mk/config.mk
all: $(OUT)/desktop-bochs
$(OUT)/desktop-bochs: factory/desktop-bochs.c $(wildcard factory/emu/bochs*) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
