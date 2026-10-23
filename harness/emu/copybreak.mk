# DAS L002 / initech-kxrk. Locked regression: Copy must survive the next
# AH=2Ch poll (100 PIT ticks); w2200 crosses two polls before Paste.
# Initial mouse (320,240); Apple Scrapbook row (60,98), title (320,90).
COPYBREAK_OPEN := m-100:-100,m-100:-100,m-100:-30,l1,w200,m40:88,l0,w600
COPYBREAK_SPEC := $(COPYBREAK_OPEN),kctrl-c,w2200,kctrl-v,w400,m100:-8,m100:0,m60:0,l1,m60:40,l0,w300,m-100:0,m-50:0,l1,l0,w400,m-100:-100,m-100:-20,m-10:0,l1,w200,m40:88,l0,w600
COPYBREAK_IMAGE ?= $(BUILD)/flair_tenants_mut_iword_gate.img
COPYBREAK_TAG ?= copybreak
COPYBREAK_DATA := $(BUILD)/copybreak_seed_data.img
$(BUILD)/copybreak_seed: harness/emu/copybreak_seed.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(COPYBREAK_DATA): $(FLAIR_DATA_IMG) $(BUILD)/copybreak_seed
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $< $@
	$(BUILD)/copybreak_seed $(BUILD)/copybreak.dat
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/copybreak.dat ::SCRAPBK.DAT
	@$(call fixture-check,recipe,$@)

.PHONY: test-flair-copybreak
test-flair-copybreak: $(HARNESS_BIN) $(COPYBREAK_IMAGE) $(COPYBREAK_DATA) $(BUILD)/da_store_check
	$(call appl-boot,$(COPYBREAK_TAG),$(COPYBREAK_IMAGE),$(COPYBREAK_DATA),COPYBREAK_SPEC,)
	@sh harness/emu/copybreak_check.sh $(BUILD)/$(COPYBREAK_TAG).serial
	@$(call APPL_ALIVE,$(BUILD)/$(COPYBREAK_TAG).serial)
	@mcopy -o -i $(BUILD)/$(COPYBREAK_TAG)_data.img ::SCRAPBK.DAT $(BUILD)/$(COPYBREAK_TAG).dat
	@$(DA_DISK) scrapbook $(BUILD)/$(COPYBREAK_TAG).dat 1 alpha alpha

RECORD_SCRIPTS += copybreak
RECORD_SPEC_copybreak = $(COPYBREAK_SPEC)
RECORD_IMAGE_copybreak = $(FLAIRTENANTS_RECORDDBL_IMG)
RECORD_DATA_copybreak = $(COPYBREAK_DATA)
RECORD_MARKER_copybreak = DA-LOADED SCRAPBK.DAT
RECORD_SETTLE_copybreak = 400

$(BUILD)/kbd_copybreak_leak.o: $(KERNEL_KBD_C) $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/io.h spec/tsr_contract.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_PURE_DECODE_FLAGS) -DKBD_MUT_DUAL_RING -I$(KERNEL_DIR) -c $< -o $@
$(BUILD)/kernel_copybreak_leak.elf: $(filter-out $(KERNEL_KBD_OBJ) $(KERNEL_FLAIRTENANTS_MAIN_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/kmain_flairtenants_mut_iword_gate.o $(BUILD)/kbd_copybreak_leak.o $(KERNEL_LD) $(TRANSIENT_IMPORTS_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_KBD_OBJ) $(KERNEL_FLAIRTENANTS_MAIN_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/kmain_flairtenants_mut_iword_gate.o $(BUILD)/kbd_copybreak_leak.o @$(TRANSIENT_IMPORTS_LD)
$(BUILD)/kernel_copybreak_leak.bin: $(BUILD)/kernel_copybreak_leak.elf
	$(OBJCOPY) -O binary $< $@
	@test "$$(wc -c < $@)" -le $$(( $(KERNEL_SECTORS) * 512 ))
	@truncate -s $$(( $(KERNEL_SECTORS) * 512 )) $@
	$(call kernel-end-guard,$<,copybreak-leak)
$(BUILD)/copybreak_leak.img: $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_copybreak_leak.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) $(BUILD)/transient_kernel_copybreak_leak.bin
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_copybreak_leak.bin $(BOOT_SYSTEM_TOOL) $(BUILD)/transient_kernel_copybreak_leak.bin

$(BUILD)/test_kbd_dual_ring: $(TEST_KBD_SRC) $(TEST_KBD_DEPS) $(TEST_KBD_HDRS)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DKBD_MUT_DUAL_RING -I$(MILTON_DIR) -Iseed -o $@ $(TEST_KBD_SRC) $(TEST_KBD_DEPS)
$(BUILD)/test_conin_dual_ring: $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DKBD_MUT_DUAL_RING -Ispec -I$(MILTON_DIR) -Iseed -Ibuild -o $@ $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS)
$(BUILD)/test_conin_owner_pb: $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUT_KEEP_INPUT_LOOKAHEAD -Ispec -I$(MILTON_DIR) -Iseed -Ibuild -o $@ $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS)
.PHONY: test-input-owner-mutant test-flair-copybreak-mutant test-flair-copybreak-record
test-input-owner-mutant: $(BUILD)/test_kbd_dual_ring $(BUILD)/test_conin_dual_ring $(BUILD)/test_conin_owner_pb
	@set -eu; for t in kbd conin; do \
	 if $(BUILD)/test_$${t}_dual_ring >$(BUILD)/$${t}_dual_ring.log 2>&1; then echo 'FAIL dual-ring mutant survived'; exit 1; fi; \
	 grep 'FAIL.*ownership: GUI' $(BUILD)/$${t}_dual_ring.log; done
	@if $(BUILD)/test_conin_owner_pb >$(BUILD)/conin_owner_pb.log 2>&1; then echo 'FAIL old lookahead survived handoff'; exit 1; fi
	@grep 'FAIL.*ownership: GUI entry clears DOS lookahead' $(BUILD)/conin_owner_pb.log
test-flair-copybreak-mutant: $(HARNESS_BIN) $(BUILD)/copybreak_leak.img $(COPYBREAK_DATA)
	cp -f $(COPYBREAK_DATA) $(BUILD)/copybreak_leak_data.img
	@$(HARNESS_BIN) --disk $(BUILD)/copybreak_leak.img --disk2 $(BUILD)/copybreak_leak_data.img --name copybreak_leak --out $(BUILD) --mouse '$(COPYBREAK_SPEC)' --keys-after FLAIR-LIVE-READY --quit-after 'INT21-EXIT rc=0' --timeout-ms 30000 2>$(BUILD)/copybreak_leak.report || true
	@grep -q 'triple_fault=0' $(BUILD)/copybreak_leak.report
	@if sh harness/emu/copybreak_check.sh $(BUILD)/copybreak_leak.serial >$(BUILD)/copybreak_leak.check 2>&1; then echo 'FAIL dual-ring mutant survived QEMU'; exit 1; fi
	@grep -qx 'RED copybreak: GUI key reached DOS break/exit or input dropped' $(BUILD)/copybreak_leak.check
	@grep -qx 'TENANT-SCRAP put n=5 count=1' $(BUILD)/copybreak_leak.serial
	@grep -qx 'INT23-BREAK' $(BUILD)/copybreak_leak.serial && grep -qx 'INT21-EXIT rc=0' $(BUILD)/copybreak_leak.serial
	@echo 'GREEN copybreak leak mutant: copied alpha, then INT23 terminated the desktop'
test-flair-copybreak-record:
	+@$(MAKE) --no-print-directory record-flair SCRIPT=copybreak
	@sh harness/emu/copybreak_check.sh $(RECORD_CLIPS_DIR)/rec_copybreak.serial

.PHONY: test-flair-copybreak-bochs
test-flair-copybreak-bochs: $(BOCHS_BIN) $(COPYBREAK_IMAGE) $(COPYBREAK_DATA) $(BUILD)/da_store_check
	cp -f $(COPYBREAK_DATA) $(BUILD)/copybreak_bochs_data.img
	@$(BOCHS_BIN) --disk $(COPYBREAK_IMAGE) --disk2 $(BUILD)/copybreak_bochs_data.img --name copybreak_bochs --out $(BUILD) --keys '$(COPYBREAK_SPEC)' --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK --timeout-ms 180000 2>$(BUILD)/copybreak_bochs.report
	@sh harness/emu/copybreak_check.sh $(BUILD)/copybreak_bochs.serial
	@mcopy -o -i $(BUILD)/copybreak_bochs_data.img ::SCRAPBK.DAT $(BUILD)/copybreak_bochs.dat
	@$(DA_DISK) scrapbook $(BUILD)/copybreak_bochs.dat 1 alpha alpha

$(BUILD)/test_event_owner_mut: $(TEST_EVENT_SRC) $(TEST_EVENT_DEPS)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DEVENT_MUT_NO_KEY_OBSERVER $(EVENT_INC) -o $@ $(TEST_EVENT_SRC) os/flair/event.c
.PHONY: test-event-owner-mutant test-bochs-trace test-bochs-trace-mutant
test-event-owner-mutant: $(BUILD)/test_event_owner_mut
	@if $< >$(BUILD)/event_owner_mut.log 2>&1; then echo 'FAIL missing observer survived'; exit 1; fi
	@grep 'FAIL.*ownership: observer' $(BUILD)/event_owner_mut.log
$(BUILD)/test_bochs_trace: harness/emu/test_bochs_trace.c harness/emu/bochs_trace.h
	$(CC) $(CFLAGS) -Iharness/emu $< -o $@
$(BUILD)/test_bochs_trace_mut: harness/emu/test_bochs_trace.c harness/emu/bochs_trace.h
	$(CC) $(CFLAGS) -DBOCHS_KEYS_MUT_RETURN -Iharness/emu $< -o $@
test-bochs-trace: $(BUILD)/test_bochs_trace
	@$<
test-bochs-trace-mutant: $(BUILD)/test_bochs_trace_mut
	@if $< >$(BUILD)/bochs_trace_mut.log 2>&1; then echo 'FAIL Return mutant survived'; exit 1; fi
	@grep -qx 'RED bochs trace parser' $(BUILD)/bochs_trace_mut.log

$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_INPUT_HANDOFF_GATE -DFLAIR_TEN_TICK_BUDGET=3000,input_handoff))
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_NO_INPUT_REPLAY -DKMAIN_INPUT_HANDOFF_GATE -DFLAIR_TEN_TICK_BUDGET=3000,no_input_replay))
HANDOFF_IMAGE ?= $(BUILD)/flair_tenants_mut_input_handoff.img
HANDOFF_TAG ?= input_handoff
HANDOFF_QUIT ?= FLAIR-LIVE-OK
.PHONY: test-input-handoff test-input-handoff-mutant
test-input-handoff: $(HARNESS_BIN) $(HANDOFF_IMAGE) $(LAUNCHKEY_DATA)
	cp -f $(LAUNCHKEY_DATA) $(BUILD)/$(HANDOFF_TAG)_data.img
	@$(HARNESS_BIN) --disk $(HANDOFF_IMAGE) --disk2 $(BUILD)/$(HANDOFF_TAG)_data.img --name $(HANDOFF_TAG) --out $(BUILD) --mouse '$(FLAIR_LAUNCHKEY_RESUME_SPEC)' --keys-after FLAIR-LIVE-READY --quit-after '$(HANDOFF_QUIT)' --timeout-ms 40000 2>$(BUILD)/$(HANDOFF_TAG).report || true
	@grep -qx 'FLAIR-HANDOFF-QUEUED' $(BUILD)/$(HANDOFF_TAG).serial
	@$(call LAUNCHKEY_RECOVERY,$(BUILD)/$(HANDOFF_TAG).serial) || { echo 'RED input-handoff: queued QUIT did not resume the sheet'; exit 1; }
	@echo 'GREEN input-handoff: queued QUIT resumes the sheet'
test-input-handoff-mutant: $(BUILD)/flair_tenants_mut_no_input_replay.img
	@if $(MAKE) --no-print-directory test-input-handoff HANDOFF_IMAGE=$(BUILD)/flair_tenants_mut_no_input_replay.img HANDOFF_TAG=handoff_mut HANDOFF_QUIT=FLAIR-HANDOFF-QUEUED >$(BUILD)/input_handoff_mut.log 2>&1; then echo 'FAIL no-replay mutant survived'; exit 1; fi
	@grep -Fx 'RED input-handoff: queued QUIT did not resume the sheet' $(BUILD)/input_handoff_mut.log
	@grep -qx 'FLAIR-HANDOFF-QUEUED' $(BUILD)/handoff_mut.serial
	@! grep -q '^FLAIR-RESUME' $(BUILD)/handoff_mut.serial
