# ===========================================================================
# KERNEL OVERLAYS -- the ADR-0014 DEC-14-9 gates (RATIFIED 2026-10-25; epic
# initech-x8gsi lane 1: the JTC easter-egg mat and About This Computer are
# section COLD of C:\FLAIR.OVL).
# Each gate was written RED first and is mutation-proven (Rule 6); every
# expected value is a literal or comes from an INDEPENDENT reader / golden,
# never from the overlay manager's own parser (Law 2, ADR-0010 HER-02).
#   (a) test-ovl-behaviour(-mutant)      moved family graded on its goldens
#   (b) test-ovl-panic(-mutant)          MISSING / FOREIGN / CORRUPT / magic /
#                                         wrong volume, PC LOAD LETTER raster
#   (c) test-ovl-context(-mutant)         runtime CONTEXT guards + section fault
#       test-ovl-context-static(-mutant)  ELF stub / hook / pointer rules
#   (d) test-kernel-size-ovl(-mutant)     169,984 B resident cap (lane 2 GREEN;
#                                         now in the default vector)
#   (e) test-ovl-window(-mutant)          per-section budget + window size
#   (f) test-ovl-repro(-mutant)           FLAIR.OVL + INITDOS.SYS + image
#   (g) test-ovl-absence(-mutant)         moved symbols gone from the kernel
#   (h) OVL-OK in test-flair-appswitch-bochs (Makefile)
#       test-ovl-format(-mutant)          the independent host reader
# ===========================================================================
OVL_GATE_DIR := $(BUILD)/ovl-gates
OVL_FLAG     := $(BUILD)/flairovl_kernel_flairtenants
OVL_JTC_KERNEL := kernel_flairtenants_mut_jtc_gate
$(OVL_GATE_DIR): | $(BUILD)
	@mkdir -p $@

# --- the independent host reader (DEC-14-9 last paragraph) -----------------
OVL_READER     := $(BUILD)/ovl_reader
OVL_READER_MUT := $(BUILD)/ovl_reader_mut_nocrc
$(OVL_READER): harness/proptest/ovl_reader.c | $(BUILD)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
$(OVL_READER_MUT): harness/proptest/ovl_reader.c | $(BUILD)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -DOVL_READER_MUT_NO_CRC $< -o $@
# $(call ovl-read,<reader>,<ovl>,<kernel stem>,<overlay elf>)
define ovl-read
nm $(BUILD)/$(3).elf > $(OVL_GATE_DIR)/$(3).knm; nm $(4) > $(OVL_GATE_DIR)/$(3).onm; \
$(1) $(2) $(BUILD)/$(3).bin $(BUILD)/flairovl_$(3).sys $(OVL_GATE_DIR)/$(3).knm $(OVL_GATE_DIR)/$(3).onm $(BUILD)/ovl_imports.txt $(BUILD)/ovl_exports.txt
endef

.PHONY: test-ovl-format test-ovl-format-mutant
test-ovl-format: $(OVL_READER) $(OVL_FLAG).ovl $(OVL_FLAG).sys $(FLAIRTENANTS_IMG) $(INITDOS_SYS_BIN) | $(OVL_GATE_DIR)
	@$(call ovl-read,$(OVL_READER),$(OVL_FLAG).ovl,kernel_flairtenants,$(OVL_FLAG).elf)
	@mcopy -i $(FLAIRTENANTS_IMG)@@32256 ::FLAIR.OVL - | cmp - $(OVL_FLAG).ovl
	@mcopy -i $(FLAIRTENANTS_IMG)@@32256 ::INITDOS.SYS - | cmp - $(OVL_FLAG).sys
	@cmp $(INITDOS_SYS_BIN) $(OVL_FLAG).sys
	@mattrib -i $(FLAIRTENANTS_IMG)@@32256 ::FLAIR.OVL | grep -qx '  A          ::/FLAIR.OVL' || { printf '!!! test-ovl-format FAIL: C:\\FLAIR.OVL is not archive-only (DEC-14-4)\n'; exit 1; }
	@mdir -i $(FLAIRTENANTS_IMG)@@32256 :: | grep -Eq '^FLAIR +OVL +[0-9]+ 1980-01-01 +0:00' || { printf '!!! test-ovl-format FAIL: C:\\FLAIR.OVL directory date is not 1980-01-01 00:00 (DEC-14-4, Rule 11)\n'; exit 1; }
	@printf '>>> test-ovl-format: green (independent reader; C:\\FLAIR.OVL and INITDOS.SYS on the image == the packed pair; archive-only, 1980-01-01 00:00)\n'
# Rule 6: flip one payload byte. The real reader must refuse it on the CRC
# property; the reader with that property disabled must accept it, so the CRC
# property (not something incidental) is what bit.
test-ovl-format-mutant: test-ovl-format $(OVL_READER_MUT)
	@cp -f $(OVL_FLAG).ovl $(OVL_GATE_DIR)/crcflip.ovl
	@set -eu; len=$$(od -An -tu4 -j28 -N4 $(OVL_FLAG).ovl | tr -d ' '); \
	 b=$$(od -An -tu1 -j$$(( len - 1 )) -N1 $(OVL_FLAG).ovl | tr -d ' '); \
	 printf "$$(printf '\\%03o' $$(( b ^ 1 )))" | dd of=$(OVL_GATE_DIR)/crcflip.ovl bs=1 seek=$$(( len - 1 )) conv=notrunc status=none
	@if ( $(call ovl-read,$(OVL_READER),$(OVL_GATE_DIR)/crcflip.ovl,kernel_flairtenants,$(OVL_FLAG).elf) ) > $(OVL_GATE_DIR)/crcflip.log 2>&1; then \
		printf '!!! test-ovl-format-mutant FAIL: a payload flip PASSED the reader\n'; exit 1; fi
	@grep -x 'RED ovl-reader: crc32 field != CRC-32 of every other byte' $(OVL_GATE_DIR)/crcflip.log
	@$(call ovl-read,$(OVL_READER_MUT),$(OVL_GATE_DIR)/crcflip.ovl,kernel_flairtenants,$(OVL_FLAG).elf) > /dev/null
	@printf '>>> test-ovl-format-mutant: green (payload flip RED on the CRC property; the no-CRC reader accepts it)\n'
TEST_UNIT_GATES += test-ovl-format test-ovl-format-mutant

# --- (e) overlay-window budget ----------------------------------------------
OVL_COLD_BUDGET := $(shell sed -n 's/^OVL_COLD_BUDGET = \(0x[0-9A-Fa-f]*\);/\1/p' $(OVL_LD))
.PHONY: test-ovl-window test-ovl-window-mutant
test-ovl-window: $(OVL_FLAG).elf $(OVL_FLAG).ovl | $(OVL_GATE_DIR)
	@set -eu; s=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_start"{print "0x"$$1}'); e=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_bss_end"{print "0x"$$1}'); \
	 used=$$(( e - s )); sz=$$(wc -c < $(OVL_FLAG).ovl); win=$$(( 0x5E0000 - 0x5C0000 )); \
	 [ "$$used" -le $$(( $(OVL_COLD_BUDGET) )) ] || { printf '!!! test-ovl-window FAIL: section COLD %s B > budget %s B\n' "$$used" "$$(( $(OVL_COLD_BUDGET) ))"; exit 1; }; \
	 [ "$$sz" -le "$$win" ] || { printf '!!! test-ovl-window FAIL: FLAIR.OVL %s B > window %s B\n' "$$sz" "$$win"; exit 1; }; \
	 printf '>>> test-ovl-window: green (section COLD %s of %s B budget; FLAIR.OVL %s of %s B window)\n' "$$used" "$$(( $(OVL_COLD_BUDGET) ))" "$$sz" "$$win"
# Rule 6 at the exact boundary. Re-link the real overlay with a .bss pad
# object LAST (NOBITS, align 1: it moves only bss_end, never the alignment of
# anything before it): budget exactly must link, budget + 1 must be refused by
# the link ASSERT.
test-ovl-window-mutant: test-ovl-window
	@set -eu; s=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_start"{print "0x"$$1}'); e=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_bss_end"{print "0x"$$1}'); \
	 for extra in 0 1; do pad=$$(( $(OVL_COLD_BUDGET) - (e - s) + extra )); \
	  printf 'section .bss nobits alloc noexec write align=1\nglobal ovl_budget_pad\novl_budget_pad: resb %s\nsection .note.GNU-stack noalloc noexec nowrite progbits\n' "$$pad" > $(OVL_GATE_DIR)/pad.asm; \
	  $(NASM) -f elf32 $(OVL_GATE_DIR)/pad.asm -o $(OVL_GATE_DIR)/pad.o; \
	  if $(LD) -m elf_i386 -T $(OVL_LD) --just-symbols=$(OVL_FLAG).elf.syms -o $(OVL_GATE_DIR)/pad.elf $(OVL_HDR_OBJ) $(OVL_SECTION_OBJS) $(OVL_GATE_DIR)/pad.o > $(OVL_GATE_DIR)/pad$$extra.log 2>&1; then \
		[ "$$extra" -eq 0 ] || { printf '!!! test-ovl-window-mutant FAIL: budget+1 linked\n'; exit 1; }; \
		u=$$(nm $(OVL_GATE_DIR)/pad.elf | awk '$$3=="_ovl_cold_bss_end"{print "0x"$$1}'); \
		[ $$(( u - s )) -eq $$(( $(OVL_COLD_BUDGET) )) ] || { printf '!!! test-ovl-window-mutant: pad layout %s != budget\n' "$$(( u - s ))"; exit 1; }; \
	  else [ "$$extra" -eq 1 ] || { cat $(OVL_GATE_DIR)/pad0.log; printf '!!! test-ovl-window-mutant FAIL: exactly the budget was refused (off by one)\n'; exit 1; }; fi; \
	 done; grep 'FLAIR.OVL section COLD busts its budget' $(OVL_GATE_DIR)/pad1.log
	@printf '>>> test-ovl-window-mutant: green (exactly the budget links; budget+1 refused by the link ASSERT)\n'
TEST_UNIT_GATES += test-ovl-window test-ovl-window-mutant

# --- (g) absence: the moved family is gone from the resident ELF -------------
# The family = the text symbols egg_jtc.o defines, clone suffixes stripped,
# minus header static inlines (os/flair/*.h). An export may remain only as its
# generated 11-byte stub: call ovl_check; jmp [export slot].
define ovl-absence-check
fam=$$(nm --defined-only $(OVL_SECTION_OBJS) | awk '$$2 ~ /^[tT]$$/{sub(/\..*/,"",$$3); print $$3}' | LC_ALL=C sort -u); \
checked=; for f in $$fam; do grep -Eq "static inline[^(]*[ *]$$f\(" os/flair/*.h spec/*.h spec/assets/*.h && continue; checked="$$checked $$f"; \
	hits=$$(nm --defined-only $(1) | awk -v f="$$f" '{n=$$3; sub(/\..*/,"",n); sub(/_kept$$/,"",n)} n==f' | wc -l); \
	if grep -qx "$$f" $(BUILD)/ovl_exports.txt; then \
		a=$$(nm $(1) | awk -v f="$$f" '$$3==f{print $$1}'); \
		[ "$$hits" -eq 1 ] && [ -n "$$a" ] || { printf '!!! ovl-absence FAIL: export %s resident %s times\n' "$$f" "$$hits"; exit 1; }; \
		objdump -d --no-show-raw-insn --start-address=0x$$a --stop-address=$$(( 0x$$a + 11 )) $(1) | grep -A2 "<$$f>:" | tail -n 2 > $(OVL_GATE_DIR)/stub.dis; \
		grep -q 'call.*<ovl_check>' $(OVL_GATE_DIR)/stub.dis && grep -q 'jmp *\*0x5c0' $(OVL_GATE_DIR)/stub.dis || { cat $(OVL_GATE_DIR)/stub.dis; printf '!!! ovl-absence FAIL: resident %s is not a generated stub\n' "$$f"; exit 1; }; \
	else [ "$$hits" -eq 0 ] || { printf '!!! ovl-absence FAIL: moved symbol %s still resident\n' "$$f"; exit 1; }; fi; \
done; printf '%s\n' $$checked > $(OVL_GATE_DIR)/family.txt
endef
.PHONY: test-ovl-absence test-ovl-absence-mutant
test-ovl-absence: $(KERNEL_FLAIRTENANTS_ELF) $(OVL_SECTION_OBJS) $(TRANSIENT_IMPORTS_LD) | $(OVL_GATE_DIR)
	@$(call ovl-absence-check,$(KERNEL_FLAIRTENANTS_ELF))
	@printf '>>> test-ovl-absence: green (COLD families, including Finder [%s], absent from the resident ELF; exports are stubs only)\n' "$$(tr '\n' ' ' < $(OVL_GATE_DIR)/family.txt)"
# Rule 6: LTO keeps a resident copy. Link the flagship with egg_jtc.c compiled
# INTO the kernel (entry points renamed so the stubs still link, kept by -u).
$(OVL_GATE_DIR)/egg_jtc_kept.o: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h | $(OVL_GATE_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) $(JTC_INC) -Degg_jtc_open=egg_jtc_open_kept -Degg_jtc_event=egg_jtc_event_kept -c $< -o $@
test-ovl-absence-mutant: test-ovl-absence $(OVL_GATE_DIR)/egg_jtc_kept.o
	$(KERNEL_LINK) -u egg_jtc_open_kept -u egg_jtc_event_kept -o $(OVL_GATE_DIR)/kept.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/egg_jtc_kept.o @$(TRANSIENT_IMPORTS_LD)
	@if ( $(call ovl-absence-check,$(OVL_GATE_DIR)/kept.elf) ) > $(OVL_GATE_DIR)/kept.log 2>&1; then printf '!!! test-ovl-absence-mutant FAIL: a resident copy survived\n'; exit 1; fi
	@grep -E '!!! ovl-absence FAIL: (moved symbol|export .* resident 2 times)' $(OVL_GATE_DIR)/kept.log
	@printf '>>> test-ovl-absence-mutant: green (resident copy of the moved family RED)\n'
TEST_UNIT_GATES += test-ovl-absence test-ovl-absence-mutant

# Lane 2: independently prove the Finder family check catches a resident body.
$(OVL_GATE_DIR)/finder_ops_kept.o: os/flair/finder_ops_cold.c os/flair/finder_ops_private.h | $(OVL_GATE_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Dfinder_ops_drop=finder_ops_drop_kept -Dfinder_ops_replace=finder_ops_replace_kept -Dfinder_ops_reason=finder_ops_reason_kept -Dfinder_ops_put_away=finder_ops_put_away_kept -c $< -o $@
.PHONY: test-ovl-absence-finder-mutant
test-ovl-absence-finder-mutant: test-ovl-absence $(OVL_GATE_DIR)/finder_ops_kept.o
	$(KERNEL_LINK) -u finder_ops_drop_kept -o $(OVL_GATE_DIR)/finder_kept.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/finder_ops_kept.o @$(TRANSIENT_IMPORTS_LD)
	@if ( $(call ovl-absence-check,$(OVL_GATE_DIR)/finder_kept.elf) ) > $(OVL_GATE_DIR)/finder_kept.log 2>&1; then printf '!!! Finder resident body survived\n'; exit 1; fi
	@grep -E '!!! ovl-absence FAIL: (export finder_ops_drop resident 2 times|moved symbol fo_)' $(OVL_GATE_DIR)/finder_kept.log
	@printf '>>> test-ovl-absence-finder-mutant: green (resident Finder body RED)\n'
test-ovl-absence-mutant: test-ovl-absence-finder-mutant
TEST_UNIT_GATES += test-ovl-absence-finder-mutant

# --- (c) static ELF rules (DEC-14-6.2/.5) ------------------------------------
# 1. no IRQ / driver / INT 21h object references a stub; 2. the overlay imports
# no IRQ-hook setter or idt_*; 3. no resident .data/.rodata dword points into
# [OVERLAY_BASE, OVERLAY_CEIL) (stored code pointers are stub addresses);
# 4. the tenant crash triage range (the FLAIR heap) stays disjoint from the
# overlay window (spec/memory_map.h asserts; mutant below).
OVL_NOSTUB_OBJS := $(KERNEL_ISR_OBJ) $(KERNEL_IRQ_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_KBD_OBJ) $(KERNEL_MOUSE_OBJ) $(KERNEL_EVENT_OBJ) $(KERNEL_INT21_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) $(KERNEL_BLOCK_DEVICE_OBJ) $(KERNEL_ATA_OBJ)
define ovl-nostub-check
for o in $(1); do if gcc-nm -u $$o 2>/dev/null | awk '{print $$NF}' | grep -qxF -f $(BUILD)/ovl_exports.txt; then \
	printf '!!! ovl-context-static FAIL: %s references an overlay stub (DEC-14-6.5)\n' "$$o"; exit 1; fi; done
endef
# The resident side relinks the flagship objects with --emit-relocs so only
# real (relocated) pointers are graded, never coincidental table words.
OVL_RELOC_ELF := $(OVL_GATE_DIR)/kernel_relocs.elf
.PHONY: test-ovl-context-static test-ovl-context-static-mutant
test-ovl-context-static: $(KERNEL_FLAIRTENANTS_ELF) $(OVL_NOSTUB_OBJS) $(TRANSIENT_IMPORTS_LD) | $(OVL_GATE_DIR)
	@$(call ovl-nostub-check,$(OVL_NOSTUB_OBJS))
	@if grep -Eq '^(pit_set_tick_hook|kbd_set_scancode_hook|mouse_set_event_hook|panic_set_fault_hook|idt_.*)$$' $(BUILD)/ovl_imports.txt; then printf '!!! ovl-context-static FAIL: overlay imports a hook setter\n'; exit 1; fi
	$(KERNEL_LINK) -Wl,--emit-relocs -o $(OVL_RELOC_ELF) $(KERNEL_FLAIRTENANTS_OBJS) @$(TRANSIENT_IMPORTS_LD)
	@sh harness/proptest/ovl_ptr_check.sh resident $(OVL_RELOC_ELF) $(OVL_GATE_DIR)/ptr
	@sh harness/proptest/ovl_ptr_check.sh overlay $(OVL_SECTION_OBJS)
	@printf '>>> test-ovl-context-static: green (no IRQ/driver/INT21 object references a stub; no hook-setter import; no resident data pointer into the window)\n'
test-ovl-context-static-mutant: test-ovl-context-static
	@printf 'bits 32\nextern egg_jtc_event\nsection .text\nglobal pit_ovl_ref\npit_ovl_ref: call egg_jtc_event\n' > $(OVL_GATE_DIR)/pitref.asm
	@$(NASM) -f elf32 $(OVL_GATE_DIR)/pitref.asm -o $(OVL_GATE_DIR)/pit.o
	@if ( $(call ovl-nostub-check,$(OVL_GATE_DIR)/pit.o) ) > $(OVL_GATE_DIR)/nostub.log 2>&1; then printf '!!! FAIL: stub reference from pit.o survived\n'; exit 1; fi
	@grep 'references an overlay stub' $(OVL_GATE_DIR)/nostub.log
	@printf 'bits 32\nextern pit_set_tick_hook\nsection .text\nglobal egg_jtc_hookme\negg_jtc_hookme: call pit_set_tick_hook\n' > $(OVL_GATE_DIR)/hook.asm
	@$(NASM) -f elf32 $(OVL_GATE_DIR)/hook.asm -o $(OVL_GATE_DIR)/hook.o
	@mkdir -p $(OVL_GATE_DIR)/hookgen
	@if sh tools/mk_ovl_tables.sh $(OVL_GATE_DIR)/hookgen $(NASM) $(TRANSIENT_IMPORTS_LD).kdef $(TRANSIENT_IMPORTS_LD).kund $(OVL_SECTION_OBJS) $(OVL_GATE_DIR)/hook.o > $(OVL_GATE_DIR)/hook.log 2>&1; then printf '!!! FAIL: hook-setter import survived\n'; exit 1; fi
	@grep 'DEC-14-6.5 overlay imports an IRQ/IDT hook setter: pit_set_tick_hook' $(OVL_GATE_DIR)/hook.log
	@printf 'extern ovl_window_sym\nsection .rodata align=4\nglobal ovl_bad_ptr\novl_bad_ptr: dd ovl_window_sym\n' > $(OVL_GATE_DIR)/badptr.asm
	@$(NASM) -f elf32 $(OVL_GATE_DIR)/badptr.asm -o $(OVL_GATE_DIR)/badptr.o
	$(KERNEL_LINK) -Wl,--emit-relocs -Wl,--defsym=ovl_window_sym=0x005C0100 -u ovl_bad_ptr -o $(OVL_GATE_DIR)/badptr.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/badptr.o @$(TRANSIENT_IMPORTS_LD)
	@if sh harness/proptest/ovl_ptr_check.sh resident $(OVL_GATE_DIR)/badptr.elf $(OVL_GATE_DIR)/ptrmut > $(OVL_GATE_DIR)/badptr.log 2>&1; then printf '!!! FAIL: resident pointer into the window survived\n'; exit 1; fi
	@grep 'resident relocated pointer(s) into the overlay window' $(OVL_GATE_DIR)/badptr.log
	@printf 'section .text\nglobal egg_jtc_addr_taken\negg_jtc_addr_taken: mov eax, here\nhere: ret\n' > $(OVL_GATE_DIR)/addr.asm
	@$(NASM) -f elf32 $(OVL_GATE_DIR)/addr.asm -o $(OVL_GATE_DIR)/addr.o
	@if sh harness/proptest/ovl_ptr_check.sh overlay $(OVL_SECTION_OBJS) $(OVL_GATE_DIR)/addr.o > $(OVL_GATE_DIR)/addr.log 2>&1; then printf '!!! FAIL: overlay address-taken code survived\n'; exit 1; fi
	@grep 'takes the address of overlay code' $(OVL_GATE_DIR)/addr.log
	@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(JTC_INC) -DEGG_MUT_OVL_LEAK -c os/flair/egg_jtc.c -o $(OVL_GATE_DIR)/egg_jtc_mut_leak.o
	@if sh harness/proptest/ovl_ptr_check.sh overlay $(OVL_GATE_DIR)/egg_jtc_mut_leak.o $(filter-out $(BUILD)/egg_jtc.o,$(OVL_SECTION_OBJS)) > $(OVL_GATE_DIR)/leak.log 2>&1; then printf '!!! FAIL: a global overlay function address (reproducer B) survived\n'; exit 1; fi
	@grep 'takes the address of overlay code' $(OVL_GATE_DIR)/leak.log
	@mkdir -p $(OVL_GATE_DIR)/heapmap
	@sed -e 's/^#define OVERLAY_BASE        0x[0-9A-F]*u$$/#define OVERLAY_BASE        0x004C0000u/' -e 's/^#define OVERLAY_CEIL        0x[0-9A-F]*u$$/#define OVERLAY_CEIL        0x004E0000u/' spec/memory_map.h > $(OVL_GATE_DIR)/heapmap/memory_map.h
	@test "$$(grep -cE '^#define OVERLAY_(BASE|CEIL) +0x004[CE]0000u$$' $(OVL_GATE_DIR)/heapmap/memory_map.h)" -eq 2
	@if $(CC) $(CFLAGS) -I$(OVL_GATE_DIR)/heapmap -I$(KERNEL_DIR) -o $(OVL_GATE_DIR)/heapmap/t $(TEST_MEMMAP_SRC) > $(OVL_GATE_DIR)/heapmap/cc.log 2>&1; then printf '!!! FAIL: overlay window inside the FLAIR heap compiled\n'; exit 1; fi
	@grep -q 'memory map windows overlap: OVERLAY vs FLAIR_HEAP' $(OVL_GATE_DIR)/heapmap/cc.log
	@printf '>>> test-ovl-context-static-mutant: green (stub ref from pit.o, hook-setter import, resident window pointer, overlay address-taken code (local and global), window-in-heap: all RED)\n'
TEST_UNIT_GATES += test-ovl-context-static test-ovl-context-static-mutant

# --- (d) the resident-size gate (DEC-14-9(d)) ---------------------------------
# Every production (non-mutant) kernel loads at or under 196,608 - 4,096 -
# 2 x 11,264 = 169,984 B. RED in lane 1; GREEN in lane 2, hence part of
# `make test` (operator resolution 2026-10-25, item 3).
KERNEL_OVL_RESIDENT_MAX := 169984
define kernel-ovl-grade
$(OBJCOPY) -O binary $(1) $(OVL_GATE_DIR)/resident.bin; \
loaded=$$(wc -c < $(OVL_GATE_DIR)/resident.bin); \
printf '    kernel-size-ovl %s loaded %s B; cap %s B\n' "$(1)" "$$loaded" '$(KERNEL_OVL_RESIDENT_MAX)'; \
if [ "$$loaded" -gt "$(KERNEL_OVL_RESIDENT_MAX)" ]; then \
	printf '!!! kernel-size-ovl FAIL: %s loaded %s B > %s B (DEC-14-9(d): 196608 - 4096 - 2 x 11264)\n' "$(notdir $(1))" "$$loaded" '$(KERNEL_OVL_RESIDENT_MAX)'; exit 1; \
fi
endef
.PHONY: test-kernel-size-ovl test-kernel-size-ovl-mutant
test-kernel-size-ovl: | $(OVL_GATE_DIR)
	@$(kernel-headroom-enum); \
	prod=$$(for b in $$bins; do case "$$b" in *mut*|*_record*) ;; *) printf '%s ' "$$b";; esac; done); \
	$(MAKE) --no-print-directory $$prod > $(OVL_GATE_DIR)/ksize.log 2>&1 || { cat $(OVL_GATE_DIR)/ksize.log; exit 1; }; \
	n=0; bad=0; for b in $$prod; do e=$${b%.bin}.elf; n=$$((n+1)); ( $(call kernel-ovl-grade,$$e) ) || bad=$$((bad+1)); done; \
	$(OBJCOPY) -O binary $(KERNEL_FLAIRTENANTS_ELF) $(OVL_GATE_DIR)/resident.bin; \
	printf '    flagship %s loaded %s B; cap %s B; %s of %s production kernels over\n' kernel_flairtenants.elf "$$(wc -c < $(OVL_GATE_DIR)/resident.bin)" '$(KERNEL_OVL_RESIDENT_MAX)' "$$bad" "$$n"; \
	[ "$$bad" -eq 0 ] || exit 1; \
	printf '>>> test-kernel-size-ovl: green (%s production kernels <= %s B)\n' "$$n" '$(KERNEL_OVL_RESIDENT_MAX)'
# Rule 6 at the exact boundary, on a tiny fixture kernel (the flagship is above
# the cap until lane 2, so the r4-style "real gate green first" cannot run):
# fixture padded to exactly the cap must pass, cap + 1 must fail.
test-kernel-size-ovl-mutant: $(KGC_DIR)/probe.o | $(OVL_GATE_DIR)
	@set -eu; $(LD) -m elf_i386 -T $(KERNEL_LD) -o $(OVL_GATE_DIR)/probe0.elf $(KGC_DIR)/probe.o; \
	 base=$$($(OBJCOPY) -O binary $(OVL_GATE_DIR)/probe0.elf /dev/stdout | wc -c); \
	 for extra in 0 1; do pad=$$(( $(KERNEL_OVL_RESIDENT_MAX) + extra - base )); \
		printf 'section .data align=1\nglobal ovl_size_pad\novl_size_pad: times %s db 0xA5\n' "$$pad" > $(OVL_GATE_DIR)/szpad.asm; \
		$(NASM) -f elf32 $(OVL_GATE_DIR)/szpad.asm -o $(OVL_GATE_DIR)/szpad.o; \
		$(LD) -m elf_i386 -T $(KERNEL_LD) -o $(OVL_GATE_DIR)/szpad$$extra.elf $(KGC_DIR)/probe.o $(OVL_GATE_DIR)/szpad.o; \
		got=$$($(OBJCOPY) -O binary $(OVL_GATE_DIR)/szpad$$extra.elf /dev/stdout | wc -c); \
		[ "$$got" -eq $$(( $(KERNEL_OVL_RESIDENT_MAX) + extra )) ] || { printf '!!! test-kernel-size-ovl-mutant: pad layout %s != %s\n' "$$got" "$$(( $(KERNEL_OVL_RESIDENT_MAX) + extra ))"; exit 1; }; \
	 done
	@( $(call kernel-ovl-grade,$(OVL_GATE_DIR)/szpad0.elf) ) || { printf '!!! test-kernel-size-ovl-mutant FAIL: exactly the cap went RED (off by one)\n'; exit 1; }
	@if ( $(call kernel-ovl-grade,$(OVL_GATE_DIR)/szpad1.elf) ) > $(OVL_GATE_DIR)/szpad1.log 2>&1; then printf '!!! test-kernel-size-ovl-mutant FAIL: cap+1 survived\n'; exit 1; fi
	@grep '!!! kernel-size-ovl FAIL: szpad1.elf loaded 169985 B' $(OVL_GATE_DIR)/szpad1.log
	@printf '>>> test-kernel-size-ovl-mutant: green (cap passes, cap+1 RED)\n'
TEST_UNIT_GATES += test-kernel-size-ovl test-kernel-size-ovl-mutant

# --- (f) reproducibility: FLAIR.OVL, INITDOS.SYS, the minted image -----------
# Two complete builds in DIFFERENT build directories under DIFFERENT locales
# (the host offers C, C.utf8 and POSIX; the generators pin LC_ALL=C, research
# gap G-4). The second tree is a fresh BUILD= directory: every object, both
# links, the tables, the packer and the image mint are redone there.
OVL_REPRO_B := $(BUILD)/ovl-repro-b
.PHONY: test-ovl-repro test-ovl-repro-mutant
test-ovl-repro: | $(OVL_GATE_DIR)
	@set -eu; LC_ALL=C $(MAKE) --no-print-directory $(OVL_FLAG).ovl $(INITDOS_SYS_BIN) $(FLAIRTENANTS_IMG) > $(OVL_GATE_DIR)/repro-a.log 2>&1; \
	 rm -rf $(OVL_REPRO_B); LC_ALL=C.utf8 LANG=C.utf8 $(MAKE) --no-print-directory BUILD=$(OVL_REPRO_B) $(OVL_REPRO_B)/flairovl_kernel_flairtenants.ovl $(OVL_REPRO_B)/INITDOS.SYS $(OVL_REPRO_B)/flair_tenants.img > $(OVL_GATE_DIR)/repro-b.log 2>&1 || { tail -20 $(OVL_GATE_DIR)/repro-b.log; exit 1; }; \
	 for f in flairovl_kernel_flairtenants.ovl INITDOS.SYS flair_tenants.img; do \
		a=$$(sha256sum < $(BUILD)/$$f | cut -d' ' -f1); b=$$(sha256sum < $(OVL_REPRO_B)/$$f | cut -d' ' -f1); \
		[ "$(OVL_REPRO_MUTATE)" != 1 ] || [ "$$f" != flairovl_kernel_flairtenants.ovl ] || b=$$(printf X | cat $(OVL_REPRO_B)/$$f - | sha256sum | cut -d' ' -f1); \
		[ "$$a" = "$$b" ] || { printf 'RED test-ovl-repro: %s differs across directories/locales\n' "$$f"; exit 1; }; \
		printf '>>> test-ovl-repro: green -- %s reproducible (sha256 %s)\n' "$$f" "$$a"; \
	 done
test-ovl-repro-mutant:
	@if $(MAKE) --no-print-directory test-ovl-repro OVL_REPRO_MUTATE=1 > $(OVL_GATE_DIR)/repro-mut.log 2>&1; then printf '!!! test-ovl-repro-mutant FAIL: a changed FLAIR.OVL passed\n'; exit 1; fi
	@grep 'RED test-ovl-repro: flairovl_kernel_flairtenants.ovl differs' $(OVL_GATE_DIR)/repro-mut.log
	@printf '>>> test-ovl-repro-mutant: green (a one-byte FLAIR.OVL change is detected)\n'
TEST_UNIT_GATES += test-ovl-repro test-ovl-repro-mutant

# --- emulator legs ------------------------------------------------------------
# Gate overlays: a header with the last import skewed (the FOREIGN leg), and two
# mutant JTC sections (no pad labels; ud2 on open). Each is linked and packed
# against a REAL kernel by the same recipe as the shipped overlay.
$(BUILD)/overlay_hdr_skew.o: $(OVL_HDR_ASM) $(TRANSIENT_IMPORTS_LD) | $(BUILD)
	$(NASM) -f elf32 -I$(BUILD)/ -DOVL_LAST_IMPORT_SKEW=4 $< -o $@
$(BUILD)/egg_jtc_mut_nolabel.o: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(JTC_INC) -DEGG_MUT_OVL_ONE_LABEL -c $< -o $@
$(BUILD)/egg_jtc_mut_ud2.o: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(JTC_INC) -DEGG_MUT_OVL_UD2 -c $< -o $@
$(eval $(call flairovl-rules,_skew,$(BUILD)/overlay_hdr_skew.o,$(OVL_SECTION_OBJS)))
$(eval $(call flairovl-rules,_nolabel,$(OVL_HDR_OBJ),$(BUILD)/egg_jtc_mut_nolabel.o $(filter-out $(BUILD)/egg_jtc.o,$(OVL_SECTION_OBJS))))
$(eval $(call flairovl-rules,_ud2,$(OVL_HDR_OBJ),$(BUILD)/egg_jtc_mut_ud2.o $(filter-out $(BUILD)/egg_jtc.o,$(OVL_SECTION_OBJS))))

# $(call ovl-gate-image,<img>,<kernel stem>,<overlay tag>): mint a boot image
# whose INITDOS.SYS / C:\FLAIR.OVL are the <tag> pair for kernel <stem>.
define ovl-gate-image
$(1): $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/flairovl$(3)_$(2).sys $(BUILD)/flairovl$(3)_$(2).ovl $(BUILD)/transient_$(2).bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(OVL_GATE_DIR)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/flairovl$(3)_$(2).sys $(BOOT_SYSTEM_TOOL) $(BUILD)/transient_$(2).bin $(BUILD)/flairovl$(3)_$(2).ovl
endef

# Kernel mutants (each disables exactly one check; never in a real build).
OVL_MUT_TAGS := ovl_no_missing ovl_no_imports ovl_no_crc ovl_no_magic ovl_data_volume
$(foreach t,$(OVL_MUT_TAGS),$(eval $(call flair-tenants-kmain-mutant-rules,$(shell echo $(t) | tr a-z A-Z | sed 's/^OVL_/OVL_MUT_/'),$(t))))
$(eval $(call ovl-gate-image,$(OVL_GATE_DIR)/skew.img,kernel_flairtenants,_skew))
$(eval $(call ovl-gate-image,$(OVL_GATE_DIR)/skew_mut.img,kernel_flairtenants_mut_ovl_no_imports,_skew))

# $(call ovl-derive,<src img>,<dst img>,<action>): copy a minted image and
# author ONE defect into C:\FLAIR.OVL at build time (the DESKTOP.DB precedent).
#   missing: delete it; crcflip: flip the last payload byte; magicflip: 'L'->'M'.
define ovl-derive
	@mkdir -p $(OVL_GATE_DIR)
	cp -f $(1) $(2)
	@set -eu; v=$(2)@@32256; case $(3) in \
	 missing) mdel -i $$v ::FLAIR.OVL ;; \
	 crcflip|magicflip) mcopy -i $$v ::FLAIR.OVL $(2).ovl; \
		if [ $(3) = magicflip ]; then off=7; else off=$$(( $$(od -An -tu4 -j28 -N4 $(2).ovl | tr -d ' ') - 1 )); fi; \
		b=$$(od -An -tu1 -j$$off -N1 $(2).ovl | tr -d ' '); \
		printf "$$(printf '\\%03o' $$(( b ^ 1 )))" | dd of=$(2).ovl bs=1 seek=$$off conv=notrunc status=none; \
		touch -d @315532800 $(2).ovl; mcopy -o -m -i $$v $(2).ovl ::FLAIR.OVL ;; \
	esac
endef
OVL_DATA      := $(OVL_GATE_DIR)/data.img
OVL_DATA_OVL  := $(OVL_GATE_DIR)/data_with_ovl.img
# $(call ovl-leg,<image>,<data>,<tag>,<REASON>,<v|->) -- one positive leg.
ovl-leg = sh harness/emu/ovl_panic.sh $(BUILD) $(1) $(2) $(3) $(4) $(5) HALTED
# $(call ovl-mutant-leg,<image>,<data>,<tag>,<REASON>,<v|->,<quit>,<expected RED line>[,<ready-ok>])
define ovl-mutant-leg
	@if sh harness/emu/ovl_panic.sh $(BUILD) $(1) $(2) $(3) $(4) $(5) $(6) $(8) > $(OVL_GATE_DIR)/$(3).red 2>&1; then \
		cat $(OVL_GATE_DIR)/$(3).red; printf '!!! %s survived\n' '$(3)'; exit 1; fi
	@grep -F '$(7)' $(OVL_GATE_DIR)/$(3).red
endef

# --- (b) the panic gate --------------------------------------------------------
OVL_N_IMPORTS_LAST = $(shell printf '%08X' $$(( $$(wc -l < $(BUILD)/ovl_imports.txt) - 1 )))
$(OVL_GATE_DIR)/missing.img: $(FLAIRTENANTS_IMG)
	$(call ovl-derive,$<,$@,missing)
$(OVL_GATE_DIR)/crcflip.img: $(FLAIRTENANTS_IMG)
	$(call ovl-derive,$<,$@,crcflip)
$(OVL_GATE_DIR)/magicflip.img: $(FLAIRTENANTS_IMG)
	$(call ovl-derive,$<,$@,magicflip)
$(OVL_GATE_DIR)/missing_mut.img: $(BUILD)/flair_tenants_mut_ovl_no_missing.img
	$(call ovl-derive,$<,$@,missing)
$(OVL_GATE_DIR)/crcflip_mut.img: $(BUILD)/flair_tenants_mut_ovl_no_crc.img
	$(call ovl-derive,$<,$@,crcflip)
$(OVL_GATE_DIR)/magicflip_mut.img: $(BUILD)/flair_tenants_mut_ovl_no_magic.img
	$(call ovl-derive,$<,$@,magicflip)
# The wrong-volume mutant gets ITS OWN overlay on A:, so disabling the C:-only
# rule shows as acceptance, not as an incidental identity mismatch.
OVL_DATA_OVL_MUT := $(OVL_GATE_DIR)/data_with_mut_ovl.img
$(OVL_DATA_OVL_MUT): $(FLAIR_DATA_IMG) $(BUILD)/flairovl_kernel_flairtenants_mut_ovl_data_volume.ovl | $(OVL_GATE_DIR)
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $< $@
	@mcopy -m -i $@ $(BUILD)/flairovl_kernel_flairtenants_mut_ovl_data_volume.ovl ::FLAIR.OVL
$(OVL_GATE_DIR)/wrongvol_mut.img: $(BUILD)/flair_tenants_mut_ovl_data_volume.img
	$(call ovl-derive,$<,$@,missing)
$(OVL_DATA): $(FLAIR_DATA_IMG) | $(OVL_GATE_DIR)
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $< $@
# Wrong volume (DEC-14-4): the overlay copied to the DATA volume (A:) only.
$(OVL_DATA_OVL): $(FLAIR_DATA_IMG) $(OVL_FLAG).ovl | $(OVL_GATE_DIR)
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $< $@
	@mcopy -m -i $@ $(OVL_FLAG).ovl ::FLAIR.OVL
.PHONY: test-ovl-panic test-ovl-panic-mutant
OVL_PANIC_IMGS := $(addprefix $(OVL_GATE_DIR)/,missing.img skew.img crcflip.img magicflip.img)
OVL_PANIC_MUT_IMGS := $(addprefix $(OVL_GATE_DIR)/,missing_mut.img skew_mut.img crcflip_mut.img magicflip_mut.img wrongvol_mut.img)
test-ovl-panic: test-panic-screen $(HARNESS_BIN) $(BUILD)/ppm_panic_check $(OVL_PANIC_IMGS) $(OVL_DATA) $(OVL_DATA_OVL)
	@$(call ovl-leg,$(OVL_GATE_DIR)/missing.img,$(OVL_DATA),missing,MISSING,-)
	@$(call ovl-leg,$(OVL_GATE_DIR)/skew.img,$(OVL_DATA),skew,FOREIGN,$(OVL_N_IMPORTS_LAST))
	@$(call ovl-leg,$(OVL_GATE_DIR)/crcflip.img,$(OVL_DATA),crcflip,CORRUPT,-)
	@$(call ovl-leg,$(OVL_GATE_DIR)/magicflip.img,$(OVL_DATA),magicflip,FOREIGN,00000007)
	@$(call ovl-leg,$(OVL_GATE_DIR)/missing.img,$(OVL_DATA_OVL),wrongvol,MISSING,-)
	@printf '>>> test-ovl-panic: green (missing, foreign import skew, CRC flip, magic flip, wrong volume: each refused by name + PC LOAD LETTER, never READY)\n'
test-ovl-panic-mutant: test-ovl-panic $(OVL_PANIC_MUT_IMGS) $(OVL_DATA_OVL_MUT)
	$(call ovl-mutant-leg,$(OVL_GATE_DIR)/missing_mut.img,$(OVL_DATA),missing_mut,MISSING,-,HALTED,RED ovl-panic missing_mut: wrong refusal)
	$(call ovl-mutant-leg,$(OVL_GATE_DIR)/skew_mut.img,$(OVL_DATA),skew_mut,FOREIGN,-,FLAIR-TENANTS-READY,reached FLAIR-TENANTS-READY)
	$(call ovl-mutant-leg,$(OVL_GATE_DIR)/crcflip_mut.img,$(OVL_DATA),crcflip_mut,CORRUPT,-,FLAIR-TENANTS-READY,reached FLAIR-TENANTS-READY)
	$(call ovl-mutant-leg,$(OVL_GATE_DIR)/magicflip_mut.img,$(OVL_DATA),magicflip_mut,FOREIGN,00000007,HALTED,RED ovl-panic magicflip_mut: wrong refusal)
	$(call ovl-mutant-leg,$(OVL_GATE_DIR)/wrongvol_mut.img,$(OVL_DATA_OVL_MUT),wrongvol_mut,MISSING,-,FLAIR-TENANTS-READY,reached FLAIR-TENANTS-READY)
	@printf '>>> test-ovl-panic-mutant: green (each check disabled in turn: all five legs RED)\n'
TEST_EMU_GATES += test-ovl-panic test-ovl-panic-mutant

# --- (c) runtime context guards + the section fault (DEC-14-6.4/.6) -----------
OVL_CTX_TAGS := ovl_ctx_irq ovl_ctx_panic ovl_ctx_busy
$(eval $(call flair-tenants-kmain-mutant-rules,OVL_TEST_CTX_IRQ,ovl_ctx_irq))
$(eval $(call flair-tenants-kmain-mutant-rules,OVL_TEST_CTX_PANIC,ovl_ctx_panic))
$(eval $(call flair-tenants-kmain-mutant-rules,OVL_TEST_CTX_BUSY,ovl_ctx_busy))
$(eval $(call flair-tenants-kmain-mutant-rules,OVL_TEST_CTX_IRQ -DOVL_MUT_NO_IRQ_GUARD,ovl_ctx_irq_mut))
$(eval $(call flair-tenants-kmain-mutant-rules,OVL_TEST_CTX_PANIC -DOVL_MUT_NO_PANIC_GUARD,ovl_ctx_panic_mut))
$(eval $(call flair-tenants-kmain-mutant-rules,OVL_TEST_CTX_BUSY -DOVL_MUT_NO_BUSY_GUARD,ovl_ctx_busy_mut))
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_TEN_TICK_BUDGET=1000 -DOVL_MUT_NO_FAULT_NAME,ovl_fault_mut))
$(eval $(call ovl-gate-image,$(OVL_GATE_DIR)/fault.img,$(OVL_JTC_KERNEL),_ud2))
$(eval $(call ovl-gate-image,$(OVL_GATE_DIR)/fault_mut.img,kernel_flairtenants_mut_ovl_fault_mut,_ud2))
# The section-fault leg: the JTC open trace runs a ud2 in the overlaid open.
define ovl-fault-leg
	cp -f $(OVL_DATA) $(OVL_GATE_DIR)/$(2)_data.img
	@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(1) --disk2 $(OVL_GATE_DIR)/$(2)_data.img --name $(2) --out $(OVL_GATE_DIR) \
	 --mouse "$(FLAIR_EGG_JTC_OPEN_SPEC)" --keys-after FLAIR-LIVE-READY --quit-after HALTED --timeout-ms 40000 2> $(OVL_GATE_DIR)/$(2).report || true
	@tr -d '\r' < $(OVL_GATE_DIR)/$(2).serial > $(OVL_GATE_DIR)/$(2).norm
endef
.PHONY: test-ovl-context test-ovl-context-mutant
test-ovl-context: test-ovl-context-static test-panic-screen $(HARNESS_BIN) $(BUILD)/ppm_panic_check $(OVL_DATA) $(foreach t,$(OVL_CTX_TAGS),$(BUILD)/flair_tenants_mut_$(t).img) $(OVL_GATE_DIR)/fault.img
	@sh harness/emu/ovl_panic.sh $(BUILD) $(BUILD)/flair_tenants_mut_ovl_ctx_irq.img $(OVL_DATA) ctx_irq CONTEXT 00000001 HALTED 1
	@$(call ovl-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_panic.img,$(OVL_DATA),ctx_panic,CONTEXT,00000100)
	@$(call ovl-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_busy.img,$(OVL_DATA),ctx_busy,CONTEXT,00010000)
	$(call ovl-fault-leg,$(OVL_GATE_DIR)/fault.img,fault)
	@grep -qx 'PANIC FLAIR.OVL section=COLD' $(OVL_GATE_DIR)/fault.norm && grep -q '^PANIC vec=06 ' $(OVL_GATE_DIR)/fault.norm && grep -qx HALTED $(OVL_GATE_DIR)/fault.norm \
	 && ! grep -q '^TENANT-CRASH' $(OVL_GATE_DIR)/fault.norm || { cat $(OVL_GATE_DIR)/fault.norm | tail -5; printf '!!! test-ovl-context FAIL: an overlay #UD was not a kernel panic naming section COLD\n'; exit 1; }
	@printf '>>> test-ovl-context: green (loads from IRQ0, the panic path and under a busy ATA channel refused CONTEXT; an overlay #UD panics naming section COLD, never a tenant crash)\n'
test-ovl-context-mutant: test-ovl-context $(foreach t,$(OVL_CTX_TAGS),$(BUILD)/flair_tenants_mut_$(t)_mut.img) $(OVL_GATE_DIR)/fault_mut.img
	$(call ovl-mutant-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_irq_mut.img,$(OVL_DATA),ctx_irq_mut,CONTEXT,00000001,HALTED,no PANIC FLAIR.OVL line,1)
	@grep -qx OVL-TEST-SURVIVED $(OVL_GATE_DIR)/ctx_irq_mut.norm
	$(call ovl-mutant-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_panic_mut.img,$(OVL_DATA),ctx_panic_mut,CONTEXT,00000100,HALTED,no PANIC FLAIR.OVL line)
	@grep -qx OVL-TEST-SURVIVED $(OVL_GATE_DIR)/ctx_panic_mut.norm
	$(call ovl-mutant-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_busy_mut.img,$(OVL_DATA),ctx_busy_mut,CONTEXT,00010000,HALTED,no PANIC FLAIR.OVL line)
	@grep -qx OVL-TEST-SURVIVED $(OVL_GATE_DIR)/ctx_busy_mut.norm
	$(call ovl-fault-leg,$(OVL_GATE_DIR)/fault_mut.img,fault_mut)
	@if grep -qx 'PANIC FLAIR.OVL section=COLD' $(OVL_GATE_DIR)/fault_mut.norm; then printf '!!! OVL_MUT_NO_FAULT_NAME survived\n'; exit 1; fi
	@grep -q '^PANIC vec=06 ' $(OVL_GATE_DIR)/fault_mut.norm
	@printf '>>> test-ovl-context-mutant: green (each guard disabled: the load proceeds -- OVL-TEST-SURVIVED; unnamed section fault RED)\n'
TEST_EMU_GATES += test-ovl-context test-ovl-context-mutant

# --- (a) behaviour equivalence of the moved family ----------------------------
# The family's grading stays its existing INDEPENDENT goldens
# (fixtures/easter/jtc_mat.png, jtc_alert.png via test-flair-egg-jtc), now with
# the mat running from C:\FLAIR.OVL. Mutants: (1) a mutant JTC section whose
# pads all show pad 0's label, packed as a mutant overlay on the REAL gate kernel, must fail the
# authored still; (2) every stub resolved to the WRONG export must fail.
$(eval $(call ovl-gate-image,$(OVL_GATE_DIR)/nolabel.img,$(OVL_JTC_KERNEL),_nolabel))
# $(call ovl-stub-mutant,<tag>,<generator env>,<kmain obj>): a kernel
# build/kernel_mut_ovl<tag>.{elf,bin} whose GENERATED stub object is a mutant
# (tools/mk_ovl_tables.sh knob), everything else real, plus its gate image
# $(OVL_GATE_DIR)/<tag>.img carrying the real overlay linked against it.
define ovl-stub-mutant
$(OVL_GATE_DIR)/$(1)/ovl_kernel.o: $(TRANSIENT_IMPORTS_LD) tools/mk_ovl_tables.sh $(OVL_SECTION_OBJS)
	@mkdir -p $(OVL_GATE_DIR)/$(1)
	@$(2) sh tools/mk_ovl_tables.sh $(OVL_GATE_DIR)/$(1) $(NASM) $(TRANSIENT_IMPORTS_LD).kdef $(TRANSIENT_IMPORTS_LD).kund $(OVL_SECTION_OBJS)
	@sed 's|$(OVL_KERNEL_OBJ)|$(OVL_GATE_DIR)/$(1)/ovl_kernel.o|' $(TRANSIENT_IMPORTS_LD) > $(OVL_GATE_DIR)/$(1)/link.rsp
$(BUILD)/kernel_mut_ovl$(1).elf: $(filter-out $(KERNEL_FLAIRTENANTS_MAIN_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(3) $(OVL_GATE_DIR)/$(1)/ovl_kernel.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $$@ $(filter-out $(KERNEL_FLAIRTENANTS_MAIN_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(3) @$(OVL_GATE_DIR)/$(1)/link.rsp
$(BUILD)/kernel_mut_ovl$(1).bin: $(BUILD)/kernel_mut_ovl$(1).elf
	$(OBJCOPY) -O binary $$< $$@
	@sz=$$$$(wc -c < $$@); dd if=/dev/zero of=$$@ bs=1 seek=$$$$sz count=$$$$(( $(KERNEL_SECTORS) * 512 - sz )) conv=notrunc status=none
$$(eval $$(call ovl-gate-image,$(OVL_GATE_DIR)/$(1).img,kernel_mut_ovl$(1),))
endef
$(eval $(call ovl-stub-mutant,swap,OVL_MUT_SWAP_STUBS=1,$(BUILD)/kmain_flairtenants_mut_jtc_gate.o))
.PHONY: test-ovl-behaviour test-ovl-behaviour-mutant
test-ovl-behaviour: test-flair-egg-jtc test-ovl-absence test-flair-file-ops test-flair-trash-edges test-flair-finder-alerts test-flair-file-copy
	@grep -qx 'OVL-OK' $(BUILD)/egg_jtc_mat.serial
	@printf '>>> test-ovl-behaviour: green (the JTC mat ran from C:\\FLAIR.OVL against its authored goldens; resident copy absent)\n'
test-ovl-behaviour-mutant: test-ovl-behaviour test-flair-file-ops-mutant test-flair-trash-edges-mutant test-flair-file-copy-mutant $(OVL_GATE_DIR)/nolabel.img $(OVL_GATE_DIR)/swap.img $(JTC_CHECK) $(OVL_DATA)
	$(call jtc-boot,ovl_nolabel,$(OVL_GATE_DIR)/nolabel.img,FLAIR_EGG_JTC_OPEN_SPEC,EGG-JTC-OPEN click=3)
	@if $(JTC_CHECK) $(BUILD)/ovl_nolabel.ppm fixtures/easter/jtc_mat.png > $(OVL_GATE_DIR)/nolabel.red 2>&1; then printf '!!! mutant overlay (one label) survived the authored still\n'; exit 1; fi
	@cat $(OVL_GATE_DIR)/nolabel.red | head -n 2
	@cp -f $(OVL_DATA) $(OVL_GATE_DIR)/swap_data.img
	@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(OVL_GATE_DIR)/swap.img --disk2 $(OVL_GATE_DIR)/swap_data.img --name ovl_swap --out $(OVL_GATE_DIR) \
	 --mouse "$(FLAIR_EGG_JTC_OPEN_SPEC)" --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK --timeout-ms 40000 \
	 --screendump --screendump-after 'EGG-JTC-OPEN click=3' 2> $(OVL_GATE_DIR)/ovl_swap.report || true
	@if tr -d '\r' < $(OVL_GATE_DIR)/ovl_swap.serial | grep -qx 'PANIC JTC launch'; then printf 'RED wrong-export stubs: PANIC JTC launch (open ran another entry)\n'; \
	 elif ! tr -d '\r' < $(OVL_GATE_DIR)/ovl_swap.serial | grep -qx 'EGG-JTC-OPEN click=3'; then printf 'RED wrong-export stubs: the mat never opened\n'; \
	 elif [ -f $(OVL_GATE_DIR)/ovl_swap.ppm ] && ! $(JTC_CHECK) $(OVL_GATE_DIR)/ovl_swap.ppm fixtures/easter/jtc_mat.png > $(OVL_GATE_DIR)/swap.red 2>&1; then head -n 1 $(OVL_GATE_DIR)/swap.red; \
	 else printf '!!! wrong-export stubs survived\n'; exit 1; fi
	@printf '>>> test-ovl-behaviour-mutant: green (mutant overlay on a real kernel and wrong-export stubs both RED)\n'
TEST_EMU_GATES += test-ovl-behaviour test-ovl-behaviour-mutant

# --- the LAZY slow path (DEC-14-6.1; review of 61fa5720, Rule 6) ------------
# Under 5A the desktop-start preload always wins, so the stubs' slow path
# (ovl_check -> pushad; call ovl_need; popad) never ran in any gate. This gate
# kernel skips the preload (-DOVL_TEST_NO_PRELOAD, gate-only): the FIRST stub
# call -- the JTC mat's open, or Apple > About -- must load the window
# (OVL-OK only AFTER FLAIR-TENANTS-READY) and behave exactly as the preloaded
# desktop: the mat against its authored golden, the About alert byte-identical
# to test-flair-apple-menu's preloaded capture. Mutants: the slow path without
# pushad/popad (the regparm arguments clobbered) and an ovl_check that never
# loads must both go RED on BOTH traces.
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_TEN_TICK_BUDGET=1000 -DOVL_TEST_NO_PRELOAD,ovl_lazy))
$(eval $(call ovl-stub-mutant,nosave,OVL_MUT_STUB_NOSAVE=1,$(BUILD)/kmain_flairtenants_mut_ovl_lazy.o))
$(eval $(call ovl-stub-mutant,noload,OVL_MUT_STUB_NOLOAD=1,$(BUILD)/kmain_flairtenants_mut_ovl_lazy.o))
OVL_LAZY_IMG := $(BUILD)/flair_tenants_mut_ovl_lazy.img
# $(call ovl-lazy-mat,<image>,<tag>) / $(call ovl-lazy-about,<image>,<tag>):
# one trace each; exit 1 on any departure from the preloaded behaviour.
define ovl-lazy-mat
$(call calcda-boot,$(2)_mat,$(1),$(FLAIR_DATA_IMG),FLAIR_EGG_JTC_OPEN_SPEC,EGG-JTC-OPEN click=3)
@grep -qx 'EGG-JTC-OPEN click=3' $(BUILD)/$(2)_mat.serial || { printf 'RED %s: the mat never opened\n' '$(2)'; exit 1; }
@$(JTC_CHECK) $(BUILD)/$(2)_mat.ppm fixtures/easter/jtc_mat.png || { printf 'RED %s: mat pixels\n' '$(2)'; exit 1; }
@tr -d '\r' < $(BUILD)/$(2)_mat.serial | awk '/^FLAIR-TENANTS-READY/{r=1} /^OVL-OK$$/{n++; if(!r) early=1} END{exit !(n==1 && !early)}' \
 || { printf 'RED %s: OVL-OK was not loaded lazily exactly once after FLAIR-TENANTS-READY\n' '$(2)'; exit 1; }
endef
define ovl-lazy-about
$(call calcda-boot,$(2)_about,$(1),$(FLAIR_DATA_IMG),FLAIR_APPLE_ABOUT_SPEC,APPLE-ABOUT up)
@grep -qx 'APPLE-ABOUT up' $(BUILD)/$(2)_about.serial && grep -q '^APPLE-ABOUT mem_k=130944 ' $(BUILD)/$(2)_about.serial || { printf 'RED %s: no About alert\n' '$(2)'; exit 1; }
@cmp -s $(BUILD)/$(2)_about.ppm $(BUILD)/flair_apple_about.ppm || { printf 'RED %s: About alert pixels differ from the preloaded capture\n' '$(2)'; exit 1; }
@tr -d '\r' < $(BUILD)/$(2)_about.serial | awk '/^FLAIR-TENANTS-READY/{r=1} /^OVL-OK$$/{n++; if(!r) early=1} END{exit !(n==1 && !early)}' \
 || { printf 'RED %s: About did not load the overlay lazily\n' '$(2)'; exit 1; }
endef
.PHONY: test-ovl-lazy test-ovl-lazy-mutant
test-ovl-lazy: test-flair-apple-menu $(OVL_LAZY_IMG) $(FLAIR_DATA_IMG) $(JTC_CHECK)
	$(call ovl-lazy-mat,$(OVL_LAZY_IMG),ovl_lazy)
	$(call ovl-lazy-about,$(OVL_LAZY_IMG),ovl_lazy)
	@printf '>>> test-ovl-lazy: green (no preload: the first stub call loads C:\\FLAIR.OVL once, after READY; JTC mat == authored golden; About == preloaded capture)\n'
test-ovl-lazy-mutant: test-ovl-lazy $(OVL_GATE_DIR)/nosave.img $(OVL_GATE_DIR)/noload.img
	@set -e; for m in nosave noload; do for leg in mat about; do \
	 if ( $(MAKE) --no-print-directory ovl-lazy-leg-$$leg OVL_LAZY_LEG_IMG=$(OVL_GATE_DIR)/$$m.img OVL_LAZY_LEG_TAG=ovl_lazy_$$m ) > $(OVL_GATE_DIR)/lazy_$${m}_$$leg.log 2>&1; then \
		printf '!!! test-ovl-lazy-mutant FAIL: stub mutant %s survived the %s trace\n' $$m $$leg; exit 1; fi; \
	 printf '    %s %s: %s\n' $$m $$leg "$$(grep -E '^(RED|!!!)' $(OVL_GATE_DIR)/lazy_$${m}_$$leg.log | head -n 1)"; done; done
	@printf '>>> test-ovl-lazy-mutant: green (slow path without pushad/popad, and an ovl_check that never loads: RED on both traces)\n'
.PHONY: ovl-lazy-leg-mat ovl-lazy-leg-about
ovl-lazy-leg-mat:
	$(call ovl-lazy-mat,$(OVL_LAZY_LEG_IMG),$(OVL_LAZY_LEG_TAG))
ovl-lazy-leg-about:
	$(call ovl-lazy-about,$(OVL_LAZY_LEG_IMG),$(OVL_LAZY_LEG_TAG))
TEST_EMU_GATES += test-ovl-lazy test-ovl-lazy-mutant
