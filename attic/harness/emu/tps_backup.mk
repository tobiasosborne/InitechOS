# initech-56ch: replacement must retain the latest report if backup rotation
# fails. Ref: spec/tps_ritual.json file_rules; IBM DOS 3.30 TR 6-70/71,
# 6-74, 6-79/80; real-DOS FCBX golden T15/T16 (rename collision / RO delete).
# The hidden .BAK forces AH=17h failure independently of AH=13h: a standard
# FCB cannot find/delete it, but its name still blocks the rename.
TPS_BACKUP_BAT := $(TPS_DIR)/autoexec_backup.bat
TPS_BACKUP_MUT := $(BUILD)/tpsrep_program_unchecked_backup.bin
TPS_BACKUP_BASE := AUTOEXEC.BAT TPSREP.COM TPS.DBF TPSPARM.DAT

$(TPS_BACKUP_MUT): $(TPSREP_ASM) | $(BUILD)
	$(NASM) -f bin -DTPS_MUTATE_UNCHECKED_BACKUP $< -o $@

# $(1) run name, $(2) program, $(3) case. All fixture bytes are independent
# pre-existing inputs; successful report bytes remain the real-DOS golden.
define tps-backup-run
	@dd if=/dev/zero of=$(BUILD)/$(1).img bs=512 count=2880 status=none
	@mformat -i $(BUILD)/$(1).img -f 1440 ::
	@mcopy -i $(BUILD)/$(1).img $(2) ::TPSREP.COM
	@mcopy -i $(BUILD)/$(1).img $(TPS_BACKUP_BAT) ::AUTOEXEC.BAT
	@mcopy -i $(BUILD)/$(1).img $(TPS_DIR)/tps.dbf ::TPS.DBF
	@mcopy -i $(BUILD)/$(1).img $(TPS_DIR)/tpsparm.dat ::TPSPARM.DAT
	@case $(3) in ro|hidden|hiddentxt|nobak) mcopy -i $(BUILD)/$(1).img $(TPS_DIR)/old_tpsrept.txt ::TPSREPT.TXT;; esac
	@case $(3) in ro|hidden|hiddentxt|orphan) mcopy -i $(BUILD)/$(1).img $(TPS_DIR)/old_tpsrept.bak ::TPSREPT.BAK;; esac
	@case $(3) in ro|orphan) mattrib -i $(BUILD)/$(1).img +r ::TPSREPT.BAK;; hidden) mattrib -i $(BUILD)/$(1).img +h ::TPSREPT.BAK;; hiddentxt) mattrib -i $(BUILD)/$(1).img +h '::TPSREPT.*';; esac
	@$(call fixture-check,recipe,$(BUILD)/$(1).img)
	@case $(3) in fresh) : > $(BUILD)/$(1).attrs-before;; *) mattrib -i $(BUILD)/$(1).img '::TPSREPT.*' > $(BUILD)/$(1).attrs-before;; esac
	@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(BUILD)/$(1).img" \
		--name "$(1)" --out "$(BUILD)" --timeout-ms 40000 \
		--keys "$(call tps-keys,$(TPS_DIR)/keys_a.txt),$(TPS_EXIT_KEYS)" --keys-after "SHELL-READY" \
		$(SHELL_OS_COMPLETION) 2> "$(BUILD)/$(1).report" || true
	@tr -d '\r' < $(BUILD)/$(1).serial > $(BUILD)/$(1).got
	@mtype -i $(BUILD)/$(1).img ::TPSREPT.TXT > $(BUILD)/$(1).rept
	@mattrib -i $(BUILD)/$(1).img '::TPSREPT.*' > $(BUILD)/$(1).attrs-after
endef

define tps-backup-check-boot
grep -qxF 'SHELL-EXIT' $(BUILD)/$(1).got && \
grep -qxF 'FORM ALIGNED.' $(BUILD)/$(1).got && \
! grep -qE '^PANIC|triple_fault=1' $(BUILD)/$(1).got $(BUILD)/$(1).report \
|| { printf 'TPSBAK: guest did not finish the ritual and return to the shell\n'; exit 1; }
endef

define tps-backup-check-refused
$(call tps-backup-check-boot,$(1)); \
cmp -s $(TPS_DIR)/old_tpsrept.txt $(BUILD)/$(1).rept \
|| { printf 'TPSBAK: TPSREPT.TXT changed after failed rotation\n'; exit 1; }; \
mtype -i $(BUILD)/$(1).img ::TPSREPT.BAK | cmp -s $(TPS_DIR)/old_tpsrept.bak - \
|| { printf 'TPSBAK: TPSREPT.BAK changed after failed rotation\n'; exit 1; }; \
cmp -s $(BUILD)/$(1).attrs-before $(BUILD)/$(1).attrs-after \
|| { printf 'TPSBAK: report attributes changed after failed rotation\n'; exit 1; }; \
grep -qxF 'TPS-012 PREVIOUS REPORT COULD NOT BE RETAINED AS TPSREPT.BAK.' $(BUILD)/$(1).got && \
grep -qxF 'TPS REPORT GENERATOR TERMINATED. RETURN CODE 10.' $(BUILD)/$(1).got && \
grep -qxF 'TPS-ERRORLEVEL-GE-10' $(BUILD)/$(1).got && \
! grep -qE '^TPS-ERRORLEVEL-GE-11|^TPS REPORT WRITTEN|^PREVIOUS REPORT RETAINED' $(BUILD)/$(1).got \
|| { printf 'TPSBAK: missing TPS-012 / ERRORLEVEL 10 refusal, or false success\n'; exit 1; }
endef

.PHONY: test-tps-backup test-tps-backup-mutant
test-tps-backup: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(TPSREP_BIN) $(TPS_BACKUP_BAT) $(TPS_FIXTURES) $(TPS_GOLDENS)
	$(call tps-backup-run,tps_backup_ro,$(TPSREP_BIN),ro)
	@( $(call tps-backup-check-refused,tps_backup_ro) )
	@printf '>>> test-tps-backup: read-only .BAK: AH=13h refused, both files/attributes preserved, TPS-012 / ERRORLEVEL 10\n'
	$(call tps-backup-run,tps_backup_hidden,$(TPSREP_BIN),hidden)
	@( $(call tps-backup-check-refused,tps_backup_hidden) )
	@printf '>>> test-tps-backup: hidden .BAK: AH=17h refused, both files/attributes preserved, TPS-012 / ERRORLEVEL 10\n'
	$(call tps-backup-run,tps_backup_hiddentxt,$(TPSREP_BIN),hiddentxt)
	@( $(call tps-backup-check-refused,tps_backup_hiddentxt) )
	@printf '>>> test-tps-backup: hidden .TXT is an existing report, both files/attributes preserved\n'
	$(call tps-backup-run,tps_backup_nobak,$(TPSREP_BIN),nobak)
	$(call tps-backup-run,tps_backup_fresh,$(TPSREP_BIN),fresh)
	$(call tps-backup-run,tps_backup_orphan,$(TPSREP_BIN),orphan)
	@set -e; for c in nobak fresh orphan; do \
	  ( $(call tps-backup-check-boot,tps_backup_$$c) ); \
	  cmp $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/tps_backup_$$c.rept; \
	  grep -qxF 'TPS REPORT GENERATOR TERMINATED. RETURN CODE 0.' $(BUILD)/tps_backup_$$c.got; \
	  ! grep -q '^TPS-ERRORLEVEL-GE-' $(BUILD)/tps_backup_$$c.got; \
	done
	@mtype -i $(BUILD)/tps_backup_nobak.img ::TPSREPT.BAK | cmp $(TPS_DIR)/old_tpsrept.txt -
	@grep -qxF 'PREVIOUS REPORT RETAINED AS TPSREPT.BAK.' $(BUILD)/tps_backup_nobak.got
	@! mdir -a -b -i $(BUILD)/tps_backup_fresh.img ::TPSREPT.BAK >/dev/null 2>&1
	@mtype -i $(BUILD)/tps_backup_orphan.img ::TPSREPT.BAK | cmp $(TPS_DIR)/old_tpsrept.bak -
	@$(call fixture-check,harness,$(BUILD)/tps_backup_orphan.img,TPSREPT.BAK=AR)
	@! grep -q '^PREVIOUS REPORT RETAINED' $(BUILD)/tps_backup_fresh.got $(BUILD)/tps_backup_orphan.got
	@printf '>>> test-tps-backup: green (3 refusal legs + missing .BAK / first report / orphan read-only .BAK controls; QEMU)\n'

test-tps-backup-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(TPS_BACKUP_MUT) $(TPS_BACKUP_BAT) $(TPS_FIXTURES)
	$(call tps-backup-run,tps_backup_mut_ro,$(TPS_BACKUP_MUT),ro)
	$(call tps-backup-run,tps_backup_mut_hidden,$(TPS_BACKUP_MUT),hidden)
	@set -e; for c in ro hidden; do \
	  ( $(call tps-backup-check-boot,tps_backup_mut_$$c) ); \
	  out=$$( ( $(call tps-backup-check-refused,tps_backup_mut_$$c) ) 2>&1 ) && { printf '!!! test-tps-backup-mutant FAIL: unchecked rotation passed\n'; exit 1; }; \
	  printf '%s\n' "$$out" | grep -qxF 'TPSBAK: TPSREPT.TXT changed after failed rotation'; \
	  grep -qxF 'TPS REPORT GENERATOR TERMINATED. RETURN CODE 0.' $(BUILD)/tps_backup_mut_$$c.got; \
	  printf '>>> test-tps-backup-mutant: unchecked rotation correctly RED (%s): %s\n' $$c "$$out"; \
	done

TEST_EMU_GATES += test-tps-backup test-tps-backup-mutant
