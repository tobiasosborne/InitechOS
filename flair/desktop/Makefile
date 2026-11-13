TOP := ../..
COMPONENT := flair/desktop
include $(TOP)/mk/config.mk
INCLUDES := -I. -I../include -I../include/assets -I../region/include -I$(TOP)/sdk/include -I$(TOP)/kernel/include -I$(TOP)/build/flair/generated
SOURCES := platform desktop
OBJECTS := $(addprefix $(OUT)/,$(addsuffix .o,$(SOURCES))) $(OUT)/tbx_gate.o
all: $(OUT)/libdesktop.a
$(OUT)/%.o: %.c $(MAKEFILE_LIST) | $(OUT)
	$(TARGET_CC) $(TARGET_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
$(OUT)/tbx_gate.o: tbx_gate.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@
$(OUT)/libdesktop.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)
AR := ar
-include $(OBJECTS:.o=.d)
