TOP := ../..
COMPONENT := flair/desktop
include $(TOP)/mk/config.mk
INCLUDES := -I. -I../include -I../include/assets -I../region/include -I$(TOP)/sdk/include -I$(TOP)/kernel/include -I$(TOP)/build/flair/generated
SOURCES := platform desktop scene input tracking finder_glue launch volumes dialogs start finder_actions finder_cmd finder_desktop finder_icon finder_menu finder_ops finder_windows desktop_db shell apple_menu power about_box about_format
OBJECTS := $(addprefix $(OUT)/,$(addsuffix .o,$(SOURCES))) $(OUT)/tbx_gate.o
all: $(OUT)/libdesktop.a
$(OUT)/%.o: %.c $(MAKEFILE_LIST) | $(OUT)
	$(TARGET_CC) $(TARGET_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
$(OUT)/tbx_gate.o: tbx_gate.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@
$(OUT)/libdesktop.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)
AR := ar
-include $(OBJECTS:.o=.d)
# Host suites retained from legacy-final; code dependencies are public Toolbox
# managers and the pure desktop modules, never the target pump/platform.
HOST_INCLUDES := $(INCLUDES) -I$(TOP)/factory/testlib -I../tests/render -I../tests/goldens -I../tests
HOST_DESKTOP := desktop shell finder_cmd finder_desktop finder_icon finder_menu finder_ops finder_windows desktop_db apple_menu egg_jtc_model
HOST_TOOLBOX := blitter chrome control cursor dialog event flair_look heap list menu process scrap stdfile surface text textedit window
HOST_SHARED := $(addprefix $(OUT)/host-common/,$(addsuffix .o,$(HOST_DESKTOP))) $(addprefix $(OUT)/host-toolbox/,$(addsuffix .o,$(HOST_TOOLBOX))) $(OUT)/host-region.o $(OUT)/host-render.o
HOST_TESTS := $(addprefix test_,finder_cmd finder_desktop finder_windows finder_ops finder_menu finder_input finder_list_view finder_replace finder_limits finder_notices finder_restore desk_icons apple_menu shell drag power_notice flair_heap_ram)
$(foreach t,$(HOST_TESTS),$(eval $(t)_SRCS := tests/$(t).c))
$(foreach t,$(HOST_TESTS),$(eval $(t)_DEPS := $(OUT)/libdesktop-host.a))
$(foreach t,$(HOST_TESTS),$(eval $(t)_LIBS := $(OUT)/libdesktop-host.a -lpng))
test_apple_menu_ARGS := $(SYSTEM7_DECOMP)
test_desk_icons_ARGS := $(SYSTEM7_DECOMP)/goldens/captures
test_shell_ARGS := $(OUT)/host/desktop_scene.ppm
test_drag_ARGS := $(OUT)/host/drag_before.ppm $(OUT)/host/drag_after.ppm
$(OUT)/host-common/%.o: %.c $(wildcard *.h) | $(OUT)
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -MMD -MP -c $< -o $@
$(OUT)/host-toolbox/%.o: ../%.c $(wildcard ../include/*.h ../*.h) | $(OUT)
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -MMD -MP -c $< -o $@
$(OUT)/host-region.o: ../region/region.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -c $< -o $@
$(OUT)/host-render.o: ../tests/render/render.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -c $< -o $@
$(OUT)/libdesktop-host.a: $(HOST_SHARED)
	rm -f $@
	$(AR) rcs $@ $^
include $(TOP)/mk/host-test.mk
-include $(HOST_SHARED:.o=.d)
.PHONY: target toolbox import-check smoke mutants
target: $(OUT)/libdesktop.a
toolbox:
	+$(MAKE) -C .. all
$(OBJECTS) $(HOST_SHARED): | toolbox
import-check: target
	sh tests/import.sh $(addsuffix .c,$(SOURCES))
test: import-check
smoke:
	+$(MAKE) -C $(TOP) smoke
mutants:
	@echo 'Desktop mutation patch conversion pending: tests/MUTANTS-TODO.md' >&2; exit 1
