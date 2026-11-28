$(OUT)/bochs-rfb.so: $(TOP)/factory/emu/bochs-rfb.c $(TOP)/factory/emu/rfb_port.h | $(OUT)
	@set -e; tmp=$@.$$$$.tmp; trap 'rm -f "$$tmp"' EXIT; \
	$(HOSTCC) $(HOST_CFLAGS) -fPIC -shared $< -o "$$tmp" -ldl; mv "$$tmp" $@
