# Optional hosted-i386 builds; target layout assertions are always mandatory.
TOP := $(abspath ..)
COMPONENT := sdk
include $(TOP)/mk/config.mk
$(OUT)/probe: $(OUT)/probe.c FORCE
	$(call run-command,($(HOSTCC) -m32 $< -o $@ > $(OUT)/probe.log 2>&1 || rm -f $@))
$(OUT)/layout: $(OUT)/layout-main.c FORCE
	$(call run-command,$(HOSTCC) -m32 -std=c11 -Wall -Wextra -Werror -Iinclude -Itests -MMD -MP $< -o $@)
$(OUT)/host-test.o: tests/host.c FORCE
	$(call run-command,$(HOSTCC) -m32 $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP -c $< -o $@)
$(OUT)/host-fake.o: host/tbx_host.c FORCE
	$(call run-command,$(HOSTCC) -m32 $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP -c $< -o $@)
$(OUT)/host: $(OUT)/host-test.o $(OUT)/host-fake.o FORCE
	$(call run-command,$(HOSTCC) -m32 $(HOST_CFLAGS) $(filter %.o,$^) -o $@)
.PHONY: available
available: $(OUT)/layout $(OUT)/host
-include $(wildcard $(OUT)/*.d)
