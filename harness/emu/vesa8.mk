# initech-2gva -- gates FIRST. See docs/research/vesa8-ground-truth.md.
# FORCE_VBE_101 is ONLY in these independently named IO.SYS/stage2 images.
VESA8_DIR := $(BUILD)/vesa8
VESA8_GEN := $(BUILD)/flair_dac_gen
VESA8_HEADER := $(BUILD)/flair_dac.h
VESA8_MUTANTS := omit119 omit165 rowcopy dac6 chan
KERNEL_CFLAGS += -I$(BUILD)
# Pattern-specific extra prerequisites also cover literal/generated mutant
# names. private prevents propagating this dependency into unrelated objects.
$(BUILD)/kmain%.o: private .EXTRA_PREREQS := $(VESA8_HEADER)
# PM 2026-10-17: .EXTRA_PREREQS on the pattern did not stop an explicitly named mutant
# kmain object (kmain_flairtenants_mut_iword_gate.o) from compiling before the header
# existed in a fresh build dir. Belt and braces: an included makefile that depends on
# the header is remade at parse time, so the header exists before any recipe runs.
-include $(BUILD)/flair_dac.stamp.mk
$(BUILD)/flair_dac.stamp.mk: $(VESA8_HEADER)
	@printf '# generated at parse time: $(VESA8_HEADER) is current\n' > $@
$(VESA8_DIR): | $(BUILD)
	@mkdir -p $@
$(VESA8_GEN): tools/flair_dac_gen.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 $< -o $@
$(VESA8_HEADER): $(VESA8_GEN) spec/assets/clut.json spec/assets/color_canon.json
	@$(VESA8_GEN) spec/assets/clut.json spec/assets/color_canon.json > $@.tmp
	@mv -f $@.tmp $@
$(BUILD)/test_flair_dac: harness/proptest/test_flair_dac.c $(VESA8_HEADER) spec/assets/color_canon.h
	$(CC) $(CFLAGS) -I$(BUILD) -Ispec/assets $< -o $@
$(BUILD)/ppm_flair_depth_check: tools/ppm_flair_depth_check.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: test-flair-dac test-flair-dac-gen test-flair-dac-mutant test-flair-desktop-8bpp test-flair-desktop-8bpp-mutant
test-flair-dac-gen: $(VESA8_HEADER)
	@$(VESA8_GEN) spec/assets/clut.json spec/assets/color_canon.json > $(BUILD)/flair_dac.again.h
	@cmp $(VESA8_HEADER) $(BUILD)/flair_dac.again.h
	@echo 'GREEN test-flair-dac-gen: deterministic generated header equality'
test-flair-dac: test-flair-dac-gen test-color-canon-gen test-color-canon test-clut $(BUILD)/test_flair_dac
	@$(BUILD)/test_flair_dac
test-flair-dac-mutant: test-flair-dac $(VESA8_HEADER) | $(VESA8_DIR)
	@set -eu; for i in 119 165; do \
	 mkdir -p $(VESA8_DIR)/mut$$i; \
	 $(CC) $(CFLAGS) -DDAC_GEN_MUT_OMIT=$$i tools/flair_dac_gen.c -o $(VESA8_DIR)/mut$$i/gen; \
	 $(VESA8_DIR)/mut$$i/gen spec/assets/clut.json spec/assets/color_canon.json > $(VESA8_DIR)/mut$$i/flair_dac.h; \
	 $(CC) $(CFLAGS) -I$(VESA8_DIR)/mut$$i -Ispec/assets harness/proptest/test_flair_dac.c -o $(VESA8_DIR)/mut$$i/check; \
	 if $(VESA8_DIR)/mut$$i/check > $(VESA8_DIR)/mut$$i/red.log 2>&1; then echo 'FAIL DAC generator mutant survived'; exit 1; fi; \
	 grep "^RED flair-dac idx=$$i " $(VESA8_DIR)/mut$$i/red.log; done
	@echo 'GREEN test-flair-dac-mutant: both generated-table omissions RED'

$(VESA8_DIR)/IO.SYS: $(BOOT_DIR)/io_sys.asm Makefile harness/emu/vesa8.mk | $(VESA8_DIR)
	$(NASM) -f bin -DFORCE_VBE_101 -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) $< -o $@
$(VESA8_DIR)/stage2.bin: $(STAGE2_ASM) $(VESA8_DIR)/IO.SYS
	$(NASM) -f bin -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) -DIO_SYS_PATH='"$(VESA8_DIR)/IO.SYS"' $< -o $@
# Copy the ACTUAL shipped/scene images; alter only IO.SYS and its embedded
# legacy copy. INITDOS.SYS and the flagship transient remain byte-identical.
define vesa8-image
$(VESA8_DIR)/$(1).img: $(2) $(VESA8_DIR)/stage2.bin harness/emu/vesa8.mk
	cp -f $$< $$@
	dd if=$(VESA8_DIR)/stage2.bin of=$$@ bs=512 seek=1 conv=notrunc status=none
	cp -f $(VESA8_DIR)/IO.SYS $$@.io
	touch -d '@315532800' $$@.io
	mattrib -i $$@@@32256 -r -s -h ::IO.SYS
	SOURCE_DATE_EPOCH=315532800 TZ=UTC mcopy -m -o -i $$@@@32256 $$@.io ::IO.SYS
	mattrib -i $$@@@32256 +r +s +h -a ::IO.SYS
	rm -f $$@.io
	@$$(call fixture-check,recipe,$$@)
endef
$(eval $(call vesa8-image,scene,$(FLAIRSHELL_IMG)))
$(eval $(call vesa8-image,flagship,$(FLAIRTENANTS_IMG)))

.PHONY: test-boot-vesa-images
test-boot-vesa-images: $(VESA8_DIR)/scene.img $(VESA8_DIR)/flagship.img $(BUILD)/boot_system
	@set -eu; for pair in scene:$(KERNEL_FLAIRSHELL_BIN) flagship:$(INITDOS_SYS_BIN); do \
	 tag=$${pair%%:*}; kernel=$${pair#*:}; img=$(VESA8_DIR)/$$tag.img; \
	 $(BUILD)/boot_system $$img check; \
	 mdir -a -i $$img@@32256 :: > $(VESA8_DIR)/$$tag.mdir; \
	 mattrib -i $$img@@32256 ::IO.SYS ::INITDOS.SYS > $(VESA8_DIR)/$$tag.attrs; \
	 mcopy -i $$img@@32256 ::IO.SYS - > $(VESA8_DIR)/$$tag.IO.SYS; \
	 mcopy -i $$img@@32256 ::INITDOS.SYS - > $(VESA8_DIR)/$$tag.INITDOS.SYS; \
	 cmp $(VESA8_DIR)/IO.SYS $(VESA8_DIR)/$$tag.IO.SYS; \
	 cmp $$kernel $(VESA8_DIR)/$$tag.INITDOS.SYS; \
	 dd if=$$img of=$(VESA8_DIR)/$$tag.volume.img bs=512 skip=63 count=2880 status=none; \
	 fsck.fat -n $(VESA8_DIR)/$$tag.volume.img > $(VESA8_DIR)/$$tag.fsck 2>&1; \
	 echo "GREEN test-boot-vesa-images $$tag: mtools bytes/order/HSR + dosfstools on identical forced image"; done

define vesa8-capture
	@sh harness/emu/vesa8.sh $(BUILD) $(1) $(2) $(3) $(FLAIR_DATA_IMG) '$(FLAIR_APPSWITCH_SPEC)'
endef
test-flair-desktop-8bpp: test-flair-dac test-boot-vesa-images $(HARNESS_BIN) $(VESA8_DIR)/scene.img $(VESA8_DIR)/flagship.img $(FLAIR_DATA_IMG) $(PPM_FLAIR_CHECK_BIN) $(PPM_FLAIR_APPSWITCH_CHECK_BIN) $(BUILD)/ppm_flair_depth_check
	$(call vesa8-capture,scene,$(FLAIRSHELL_IMG),scene32)
	$(call vesa8-capture,scene,$(VESA8_DIR)/scene.img,scene8)
	@$(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-scene32.ppm $(VESA8_DIR)/vesa8-scene8.ppm
	$(call vesa8-capture,pre,$(FLAIRTENANTS_IMG),pre32)
	$(call vesa8-capture,pre,$(VESA8_DIR)/flagship.img,pre8)
	$(call vesa8-capture,post,$(FLAIRTENANTS_IMG),post32)
	$(call vesa8-capture,post,$(VESA8_DIR)/flagship.img,post8)
	@$(PPM_FLAIR_APPSWITCH_CHECK_BIN) $(VESA8_DIR)/vesa8-pre8.ppm $(VESA8_DIR)/vesa8-post8.ppm
	@$(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-pre32.ppm $(VESA8_DIR)/vesa8-pre8.ppm
	@$(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-post32.ppm $(VESA8_DIR)/vesa8-post8.ppm
	@echo 'GREEN test-flair-desktop-8bpp: canonical checker + flagship locked app switch + exact RGB equality'

# Use the established isolated kmain-mutant template; never overwrite the
# shipped object. The mutant image still contains the real FAT-loaded kernel.
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_DAC_MUT_OMIT119,dac_omit119))
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_DAC_MUT_OMIT165,dac_omit165))
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_DAC_MUT_ROWCOPY,dac_rowcopy))
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_DAC_MUT_6BIT,dac_dac6))
# Size round 7 (initech-5qsm): the DAC loads through flair_palette_rgb; a
# channel-order slip in that unpacking must go RED here.
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_DAC_MUT_CHANNEL,dac_chan))
$(foreach m,$(VESA8_MUTANTS),$(eval $(call vesa8-image,mut_$(m),$(BUILD)/flair_tenants_mut_dac_$(m).img)))
test-flair-desktop-8bpp-mutant: test-flair-desktop-8bpp $(foreach m,$(VESA8_MUTANTS),$(VESA8_DIR)/mut_$(m).img)
	@set -eu; for m in $(VESA8_MUTANTS); do \
	 sh harness/emu/vesa8.sh $(BUILD) pre $(VESA8_DIR)/mut_$$m.img mut_$$m $(FLAIR_DATA_IMG); \
	 if $(BUILD)/ppm_flair_depth_check $(VESA8_DIR)/vesa8-pre32.ppm $(VESA8_DIR)/vesa8-mut_$$m.ppm > $(VESA8_DIR)/$$m.red 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 printf '%s: ' "$$m"; grep '^RED flair-depth RGB ' $(VESA8_DIR)/$$m.red; done
	@echo 'GREEN test-flair-desktop-8bpp-mutant: omissions 119/165, row copy, DAC precision and channel order each RED'

# Boot selection rules have their own real IO.SYS mutants. The first removes
# 0101h from the fallback; the second wrongly prefers indexed over direct.
define vesa8-boot-mutant
$(VESA8_DIR)/IO_$(1).SYS: $(BOOT_DIR)/io_sys.asm harness/emu/vesa8.mk | $(VESA8_DIR)
	$(NASM) -f bin $(2) -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) $$< -o $$@
$(VESA8_DIR)/stage2_$(1).bin: $(STAGE2_ASM) $(VESA8_DIR)/IO_$(1).SYS
	$(NASM) -f bin -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) -DIO_SYS_PATH='"$(VESA8_DIR)/IO_$(1).SYS"' $$< -o $$@
$(VESA8_DIR)/boot_$(1).img: $(MBR_BIN) $(VESA8_DIR)/stage2_$(1).bin $(KERNEL_FLAIRSHELL_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(VESA8_DIR)/stage2_$(1).bin $(VESA8_DIR)/IO_$(1).SYS $(KERNEL_FLAIRSHELL_BIN) $(BOOT_SYSTEM_TOOL)
endef
$(eval $(call vesa8-boot-mutant,no101,-DFORCE_VBE_101 -DVBE_MUT_NO_101))
$(eval $(call vesa8-boot-mutant,priority32,-DVBE_MUT_8_FIRST))
.PHONY: test-boot-vesa-mutant
test-boot-vesa-mutant: test-flair-desktop-8bpp $(VESA8_DIR)/boot_no101.img $(VESA8_DIR)/boot_priority32.img
	@set -eu; for m in no101 priority32; do \
	 if sh harness/emu/vesa8.sh $(BUILD) mode $(VESA8_DIR)/boot_$$m.img $$m > $(VESA8_DIR)/$$m.red 2>&1; then echo "FAIL boot mutant $$m survived"; exit 1; fi; \
	 why=expected-8bpp; [ $$m != priority32 ] || why=expected-32bpp; \
	 grep "^RED vesa8 $$m $$why$$" $(VESA8_DIR)/$$m.red; done
	@echo 'GREEN test-boot-vesa-mutant: missing 0101 fallback and inverted depth preference each RED'

TEST_UNIT_GATES += test-flair-dac test-flair-dac-gen test-flair-dac-mutant test-boot-vesa-images
TEST_EMU_GATES += test-flair-desktop-8bpp test-flair-desktop-8bpp-mutant test-boot-vesa-mutant
