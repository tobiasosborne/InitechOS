# Set TARGET_SRCS (C/ASM), TARGET_INCLUDES; optional TARGET_LIB (full path).
# Existing explicit object rules and TARGET_CFLAGS remain supported.
TARGET_OBJS ?= $(addprefix $(OUT)/,$(addsuffix .o,$(basename $(TARGET_SRCS))))
TARGET_ASFLAGS ?= -f elf32
TARGET_BIN_ASFLAGS ?= -f bin
$(OUT)/%.o: %.c FORCE
	$(call run-command,$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) $(TARGET_EXTRA_CFLAGS) -MMD -MP -c $< -o $@)
$(OUT)/%.o: %.asm FORCE
	$(call run-command,$(NASM) $(TARGET_ASFLAGS) -MD $@.d -MP -MT $@ $< -o $@)
$(OUT)/%.bin: %.asm FORCE
	$(call run-command,$(NASM) $(TARGET_BIN_ASFLAGS) -MD $@.d -MP -MT $@ $< -o $@)
ifneq ($(strip $(TARGET_LIB)),)
$(TARGET_LIB): private command = rm -f $@ && $(TARGET_AR) rcsD $@ $(TARGET_OBJS)
$(TARGET_LIB): $(TARGET_OBJS) FORCE
	$(run-command)
all: $(TARGET_LIB)
else
all: $(TARGET_OBJS)
endif
-include $(wildcard $(OUT)/*.d $(OUT)/*/*.d)
