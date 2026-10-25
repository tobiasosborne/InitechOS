# B05 / initech-c8w7: new regression traces, leaving the R4.5 locked traces intact.
# Coordinates: cpanels/core.c authored layout; PRD 6.3; DAS audit L027.
CP_BARS_RTC := --rtc-base 1980-01-01T00:56:00 --rtc-deterministic
CP_BARS_PARK := $(CP_RESET_POINTER),m120:100,m120:100,m120:100,m120:100,m120:60,w300
# CPANELS owns File in band 2. Select its actual Quit item with the mouse.
CP_BARS_QUIT := $(CP_RESET_POINTER),m60:30,l1,w200,m0:20,w200,l0,w300
CP_BARS_QUIT_FAST := $(CP_RESET_POINTER),m30:15,l1,w200,m0:10,w200,l0,w300
# After Quit, activate the retained system Apple > About, dismiss its modal,
# then compare both complete strips. The pointer starts at (60,50).
CP_BARS_ABOUT := m-40:-40,l1,w200,m40:17,l0,w200,kesc,w200
CP_BARS_ABOUT_FAST := m-20:-20,l1,w200,m20:9,l0,w200,kesc,w200
CP_BARS_BASE_CP := $(CP_LAUNCH_SPEC),$(CP_BARS_PARK)
CP_BARS_BASE_HELLO := $(CP_LAUNCH_SPEC),$(CP_BARS_QUIT),$(CP_BARS_PARK)
CP_BARS_PATTERN := $(CP_PATTERNS_SPEC),kret,w300,$(CP_BARS_PARK)
CP_BARS_PATTERN_EXIT := $(CP_PATTERNS_SPEC),kret,w300,$(CP_BARS_QUIT),$(CP_BARS_ABOUT),$(CP_BARS_PARK)
# Chooser -> Date & Time -> Menu Bar Clock tab -> checkbox -> OK.
CP_BARS_CLOCK_PANEL := $(CP_RESET_POINTER),m120:86,m120:86,l1,l0,w200,m90:-44,l1,l0,w200,m-127:84,m-43:84,l1,l0,w200,kret,w300
CP_BARS_HIDE := $(CP_LAUNCH_SPEC),$(CP_BARS_CLOCK_PANEL),$(CP_BARS_PARK)
CP_BARS_HIDE_EXIT := $(CP_LAUNCH_SPEC),$(CP_BARS_CLOCK_PANEL),$(CP_BARS_QUIT),$(CP_BARS_ABOUT),$(CP_BARS_PARK)
CP_BARS_SHOW_EXIT := $(CP_LAUNCH_SPEC),$(CP_BARS_CLOCK_PANEL),$(CP_BARS_CLOCK_PANEL),$(CP_BARS_QUIT),$(CP_BARS_ABOUT),$(CP_BARS_PARK)
CP_BARS_MOUSE_EXIT := $(CP_MOUSE_APPLY_SPEC),$(CP_BARS_QUIT_FAST),$(CP_BARS_ABOUT_FAST),$(CP_BARS_PARK)
CP_BARS_SOUND_EXIT := $(CP_SOUND_SPEC),kesc,w300,$(CP_BARS_QUIT),$(CP_BARS_ABOUT),$(CP_BARS_PARK)

CP_BARS_CHECK := $(BUILD)/ppm_flair_cpanels_bars_check
CP_BARS_GOLDEN := fixtures/flair-cpanels-bars
# Two clock transactions plus actual menu exit outlast the original 1200-tick
# single-panel gate. Bound this new trace image at 1800 ticks; no oracle changes.
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_TEN_TICK_BUDGET=1800,cp_bars_gate))
CP_BARS_IMG := $(BUILD)/flair_tenants_mut_cp_bars_gate.img
$(CP_BARS_CHECK): tools/ppm_flair_cpanels_bars_check.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lpng

define cp-bars-boot
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/$(1)_data.img
@$(HARNESS_BIN) --disk $(2) --disk2 $(BUILD)/$(1)_data.img --name $(1) --out $(BUILD) \
	$(CP_BARS_RTC) --mouse "$($(3))" --keys-after FLAIR-LIVE-READY \
	--quit-after FLAIR-LIVE-OK --timeout-ms 40000 --screendump --screendump-after FLAIR-LIVE-OK 2>$(BUILD)/$(1).report
@grep -qx 'FLAIR-LIVE-OK' $(BUILD)/$(1).serial && ! grep -q '^PANIC\|^TENANT-CRASH' $(BUILD)/$(1).serial
endef

define cp-bars-leg
.PHONY: test-flair-cpanels-bars-$(1)
test-flair-cpanels-bars-$(1): $(HARNESS_BIN) $(CP_BARS_IMG) $(FLAIR_DATA_IMG) $(CP_BARS_CHECK) $(BUILD)/ppm_flair_cpanels_check
	$$(call cp-bars-boot,cp_bars_$(1),$$(CP_BARS_IMG),CP_BARS_$(2))
	$(if $(findstring exit,$(1)),@grep -qx 'TENANT-MENU menu=129 item=1 src=mouse' $(BUILD)/cp_bars_$(1).serial,@:)
	$(if $(findstring exit,$(1)),@grep -qx 'FLAIR-MENU menu=1 item=1 (sel=0x00010001)' $(BUILD)/cp_bars_$(1).serial && grep -qx 'APPLE-ABOUT closed' $(BUILD)/cp_bars_$(1).serial,@:)
	@$(CP_BARS_CHECK) $(BUILD)/cp_bars_$(1).ppm $(CP_BARS_GOLDEN)/$(3).png $(4)
	$(if $(findstring pattern,$(1)),@grep -qx 'CPANELS-APPLY pattern=3 tracking=1 double=1 ticks=20' $(BUILD)/cp_bars_$(1).serial && $(BUILD)/ppm_flair_cpanels_check pattern $(BUILD)/cp_bars_$(1).ppm "$(SYSTEM7_DECOMP)/goldens/resources/PAT#_0.bin" 3,@:)
	$(if $(findstring hide,$(1)),@grep -qx 'CPANELS-RTC hour=0 minute=56' $(BUILD)/cp_bars_$(1).serial,@:)
	$(if $(findstring show,$(1)),@test "$$$$(grep -cx 'CPANELS-RTC hour=0 minute=56' $(BUILD)/cp_bars_$(1).serial)" = 2,@:)
	$(if $(findstring mouse,$(1)),@grep -qx 'CPANELS-APPLY pattern=0 tracking=2 double=2 ticks=10' $(BUILD)/cp_bars_$(1).serial,@:)
	$(if $(findstring sound,$(1)),@grep -qx 'CPANELS-BEEP hz=660' $(BUILD)/cp_bars_$(1).serial && grep -qx 'CPANELS-BEEP off' $(BUILD)/cp_bars_$(1).serial,@:)
	@printf '>>> test-flair-cpanels-bars-$(1): green\n'
endef
$(eval $(call cp-bars-leg,pattern,PATTERN,cpanels,shown))
$(eval $(call cp-bars-leg,pattern-exit,PATTERN_EXIT,hello,shown))
$(eval $(call cp-bars-leg,hide,HIDE,cpanels,hidden))
$(eval $(call cp-bars-leg,hide-exit,HIDE_EXIT,hello,hidden))
$(eval $(call cp-bars-leg,show-exit,SHOW_EXIT,hello,shown))
$(eval $(call cp-bars-leg,mouse-exit,MOUSE_EXIT,hello,shown))
$(eval $(call cp-bars-leg,sound-exit,SOUND_EXIT,hello,shown))
test-flair-cpanels-settings: test-flair-cpanels-bars-pattern test-flair-cpanels-bars-hide test-flair-cpanels-bars-hide-exit test-flair-cpanels-bars-show-exit test-flair-cpanels-bars-mouse-exit test-flair-cpanels-bars-sound-exit
test-flair-cpanels-persist: test-flair-cpanels-bars-pattern-exit

$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_CP_NO_SYSTEM_BAR -DFLAIR_TEN_TICK_BUDGET=1200,cp_no_system_bar))
.PHONY: test-flair-cpanels-bars-mutant
test-flair-cpanels-bars-mutant: $(HARNESS_BIN) $(BUILD)/flair_tenants_mut_cp_no_system_bar.img $(FLAIR_DATA_IMG) $(CP_BARS_CHECK)
	$(call cp-bars-boot,cp_bars_no_system_bar,$(BUILD)/flair_tenants_mut_cp_no_system_bar.img,CP_BARS_PATTERN)
	@if $(CP_BARS_CHECK) $(BUILD)/cp_bars_no_system_bar.ppm $(CP_BARS_GOLDEN)/cpanels.png shown > $(BUILD)/cp_bars_no_system_bar.pixels 2>&1; then printf 'FAIL system-bar repaint mutant survived\n'; exit 1; fi
	@grep -E '^FAIL CP_BARS system=[1-9][0-9]*/12800 foreground=0/12800$$' $(BUILD)/cp_bars_no_system_bar.pixels
	@printf '>>> test-flair-cpanels-bars-mutant: green (omitted system bar correctly RED)\n'
test-flair-cpanels-settings-mutant: test-flair-cpanels-bars-mutant
