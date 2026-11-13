# Set TARGET_SRCS (C/ASM), TARGET_INCLUDES; optional TARGET_LIB (full path).
# Existing explicit object rules and TARGET_CFLAGS remain supported.
TARGET_OUT ?= $(OUT)
TARGET_OBJS ?= $(addprefix $(TARGET_OUT)/,$(addsuffix .o,$(basename $(TARGET_SRCS))))
TARGET_ASFLAGS ?= -f elf32
TARGET_BIN_ASFLAGS ?= -f bin
$(TARGET_OUT)/%.o: %.c FORCE
	$(call run-command,$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) $(TARGET_EXTRA_CFLAGS) -MMD -MP -c $< -o $@)
$(TARGET_OUT)/%.o: %.asm FORCE
	$(call run-command,$(NASM) $(TARGET_ASFLAGS) -MD $@.d -MP -MT $@ $< -o $@)
$(TARGET_OUT)/%.bin: %.asm FORCE
	$(call run-command,$(NASM) $(TARGET_BIN_ASFLAGS) -MD $@.d -MP -MT $@ $< -o $@)
ifneq ($(strip $(TARGET_LIB)),)
$(TARGET_LIB): private command = rm -f $@ && $(TARGET_AR) rcsD $@ $(TARGET_OBJS)
$(TARGET_LIB): $(TARGET_OBJS) FORCE
	$(run-command)
all: $(TARGET_LIB)
else
all: $(TARGET_OBJS)
endif
-include $(TARGET_OBJS:.o=.d)
-include $(wildcard $(TARGET_OUT)/*.d $(TARGET_OUT)/*/*.d)
