# factory

Retain emulator drivers, image/fixture builders, independent differential readers and generic assertion helpers. Retire lane certificates, merge trains, sentinels, gate-selection process and variant-wide size policing. Test source files that are app/component-specific appear in those component manifests, even though they remain physically under harness/.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

305 production/source-support files; 37,345 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`harness/conkeys/extended_check.sh:1`|17|initech-wuxo0; IBM PC AT TR Sep 1985, System BIOS pp.5-18--5-19.|
|`harness/conkeys/f3template.sh:1`|25|not the echoed input (which also contains REPEAT), after SHELL-READY.|
|`harness/console_text.c:1`|193|read the 80x25 InitechDOS console back out of a QEMU|
|`harness/def_dependency_check.sh:1`|64|Reproduce the stale COLOR oracle with the current source tree in a temp copy.|
|`harness/diff/dbf_diff/console_review_emu.sh:1`|79|initech-0spyf/fzsvo; PRD 6.6. Safety invariants, extended-pair regression,|
|`harness/diff/dbf_diff/corpus_pristine.sh:1`|80|- the dBASE III PLUS sample corpus|
|`harness/diff/dbf_diff/datatype_mint.sh:1`|55|Real dBASE III PLUS mint for the SAMIR data-typing lane (initech-ylqs;|
|`harness/diff/dbf_diff/date_contract_mint.sh:1`|41|initech-bkmv: re-verify real III+ date evidence, never mint from SAMIR.|
|`harness/diff/dbf_diff/dbf_ref.py:1`|828|- independent dBASE III+ .dbf/.dbt reference reader.|
|`harness/diff/dbf_diff/dml_extra_mint.sh:1`|27|Real III+ probe companion. See dml_probe.c and the DML evidence ledger.|
|`harness/diff/dbf_diff/dml_mint.sh:1`|138|Real III+ only. Harness recipe: dbase3-decomp/re/harness-setup.md.|
|`harness/diff/dbf_diff/dml_probe.c:1`|31|Factory-only extra III+ probes: label comma spacing, block copies, pubs ?.|
|`harness/diff/dbf_diff/do_file_mint.sh:1`|26|initech-104j3: real III+ 1.1 only; re/harness-setup.md and mint_capture.sh.|
|`harness/diff/dbf_diff/do_file_test.sh:1`|48|The gate never mints. Every expected byte comes from real III+ GOLD.OUT.|
|`harness/diff/dbf_diff/fns_mint.sh:1`|48|PRD 6.6: real III PLUS 1.1 only; independent bytes, never SAMIR output.|
|`harness/diff/dbf_diff/mint_capture.sh:1`|50|Shared reference-only DOSBox capture and exact ordered error validation.|
|`harness/diff/dbf_diff/mint_clients_dbf.c:1`|144|- deterministic CLIENTS.DBF minter|
|`harness/diff/dbf_diff/mint_invoice_dbf.c:1`|171|- deterministic INVOICE.DBF minter|
|`harness/diff/dbf_diff/mzlink_close_fault.c:1`|12|Factory-only fault injection: a real close followed by an I/O error.|
|`harness/diff/dbf_diff/ndx_ref.py:1`|904|- independent dBASE III+ .ndx reference reader.|
|`harness/diff/dbf_diff/numeric_format_ref.py:1`|106|"""Independent LIST field oracle, Using III+ U5-144 (PDF 256).|
|`harness/diff/dbf_diff/prog_mint.sh:1`|47|Independent replacement of the old authored Tier-0 transcripts.|
|`harness/diff/dbf_diff/samir_core_ref.c:1`|78|Independent real III+ 1.1 mint/differential. No SAMIR engine/spec includes.|
|`harness/diff/dbf_diff/samir_install.h:1`|41|how an emulator harness puts SAMIR on a FAT image|
|`harness/diff/dbf_diff/scratch_lock.h:1`|46|serialize test binaries that share a fixed scratch dir.|
|`harness/diff/dbf_diff/screen_capture.c:1`|179|Capture the live SAMIR text console, without OCR or artifact changes.|
|`harness/diff/dbf_diff/screen_file.h:1`|147|Locked W4.0 screen format; PRD 6.6, samirctl-mint/scrdump.asm,|
|`harness/diff/dbf_diff/screen_forms_emu.sh:1`|73|W3 forms emu oracle (initech-p3nn0): replay a real III+ screen-mint case's KEYS|
|`harness/diff/dbf_diff/screen_mint.sh:1`|78|W4.0 / PRD 6.6: real III+ bytes only. Factory C + thin shell, no Python.|
|`harness/diff/dbf_diff/screen_mint_mutants.sh:1`|138|PM review reproducers. Every RED must be the expected rejection, not a timeout.|
|`harness/diff/dbf_diff/screen_mint_oracle.sh:1`|30|Recompute the current independent oracle; never trust a recorded hash alone.|
|`harness/diff/dbf_diff/screen_mint_repro.sh:1`|30|Independent fresh DOS drives; compare screens and both provenance manifests.|
|`harness/diff/dbf_diff/screen_mint_samir.sh:1`|39|Focused SAMIR emu proof. No manuals, full test vector or Bochs dependencies.|
|`harness/diff/dbf_diff/screen_mint_test.sh:1`|64|tool=build/screen_tool|
|`harness/diff/dbf_diff/screen_status_pending.sh:1`|24|PRD 6.6; initech-ebwsw. Real III+ cells/attributes/cursor, without masks.|
|`harness/diff/dbf_diff/screen_tool.c:1`|170|Factory C, PRD 6.6 / W4.0: compile timed BIOS events and grade real pages. */|
|`harness/diff/dbf_diff/screen_tsr.asm:1`|269|W4.0 guest capture shim. Extends samirctl-mint/scrdump.asm's B800 copy.|
|`harness/diff/dbf_diff/test_mzlink_io.sh:1`|21|ok=$1; close_fault=$2; root=$3|
|`harness/diff/dbf_diff/test_scratch_lock.sh:1`|13|root=$(mktemp -d build/scratch-lock-test.XXXXXX)|
|`harness/diff/dbf_diff/trace_complete.h:1`|28|PRD 8 / Rules 2/6: do not confuse launch/trace failure with a mutant bite. */|
|`harness/diff/dbf_diff/w2_fixture.c:1`|50|Factory inputs only; expected output is minted by real III+.|
|`harness/diff/dbf_diff/w2_host.c:1`|131|W2 factory adapter: DOSBox C:/D: fixture mounts, explicit free bytes and|
|`harness/diff/dbf_diff/w2_host.h:1`|29|Factory inputs matching mint_capture.sh mounts; never host disk capacity. */|
|`harness/diff/dbf_diff/w2_mint.sh:1`|67|PRD 6.6: independent real III+ bytes. Shared runner: mint_capture.sh.|
|`harness/diff/dbf_diff/w4_accept.sh:1`|17|Future application acceptance, DEC-03a.10. No not-started app instantiated.|
|`harness/diff/dbf_diff/w4_files_paths_test.sh:1`|31|Exercise the real file gate under both corpus spellings and missing refs.|
|`harness/diff/dbf_diff/w4_files_test.sh:1`|78|Real III+ samples are the oracle; no generated substitute. DEC-03a.10.|
|`harness/diff/dbf_diff/w4_inventory_test.sh:1`|39|out=build/w4-inventory-mutants|
|`harness/diff/dbf_diff/w4_mutate.c:1`|24|Deterministic negative controls, factory C. initech-9nzpr, DEC-03a.10. */|
|`harness/diff/dbf_diff/w4_retained_mint.sh:1`|80|Fresh real III+ retained screens. Same guest SCREEN.COM + screen_tool route|
|`harness/diff/dbf_diff/w4_retained_test.sh:1`|42|Validate newly minted cheap exact references and prove all three screen-byte|
|`harness/diff/dbf_diff/w4_smoke_mutants.sh:1`|57|Mutate observed behavior / injected trace, never the asserted contract.|
|`harness/diff/dbf_diff/w4_trace.sh:1`|104|L2 locked replay, DEC-03a.10 / initech-9nzpr. Pattern: screen_forms_emu.sh.|
|`harness/diff/dbf_diff/w4_unchanged.sh:1`|8|L1 read-only contract (HELP / smoke), DEC-03a.10, initech-9nzpr.|
|`harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/mint_run.py:1`|41|"""Run one samirctl mint case under dosbox-x headless; print the ALTERNATE|
|`harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/mint_setup.py:1`|114|"""samirctl mint: build the dosbox-x C: drive + per-case confs for the real|
|`harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/scrdump.asm:1`|24|mint-only helper: copy the 80x25 colour text page B800:0000|
|`harness/diff/fat_diff/ata_no_drive.sh:1`|15|bcg.12: real port-I/O failure without a primary slave; shell stays alive.|
|`harness/diff/fat_diff/batch2.sh:1`|187|Manual oracle, independent literals: MS-DOS 3.3 Reference pp.41-42,|
|`harness/diff/fat_diff/batch_break.sh:1`|108|Independent real DOS 3.30 nested behavior is in batch_break_mint/. QEMU|
|`harness/diff/fat_diff/batch_break_host.h:1`|77|Factory adapters for the real command.c batch state machine. The physical|
|`harness/diff/fat_diff/batch_break_mint/exec_child.asm:1`|39|Same DOS-call sequence for real DOS 3.30 and the flat InitechDOS ABI.|
|`harness/diff/fat_diff/batch_break_mint/mint.sh:1`|69|Re-mint into build/ only. Never overwrite the checked-in independent goldens.|
|`harness/diff/fat_diff/block_device.sh:1`|34|Raw ATA oracle vs independent mtools-created MBR and host readback.|
|`harness/diff/fat_diff/blockdev_file.c:1`|189|- host file-backed blockdev (FACTORY).|
|`harness/diff/fat_diff/blockdev_file.h:1`|98|- host file-backed blockdev (FACTORY).|
|`harness/diff/fat_diff/boot_system.sh:1`|59|Oracle first, initech-kod. Manual sources: boot_system.c, boot_system.mk.|
|`harness/diff/fat_diff/chkdsk.sh:1`|125|Oracle first: mtools mints each image, fsck.fat judges the identical bytes.|
|`harness/diff/fat_diff/chkdsk_fixture.c:1`|149|Independent corruption + reference-diff tool, FACTORY C only.|
|`harness/diff/fat_diff/chkdsk_label.sh:1`|36|ug8l: compare classification with genuine DOS 3.30, not artifact output.|
|`harness/diff/fat_diff/chkdsk_label_mint.sh:1`|43|Mint with genuine DOS 3.3, never InitechDOS. Reference media stays read-only.|
|`harness/diff/fat_diff/command_c.sh:1`|42|MS-DOS 3.3 Reference pp.46-47: copied environment, /C then parent returns.|
|`harness/diff/fat_diff/command_file.sh:1`|17|initech-e9sx independent mtools / dosfstools differential on ONE image.|
|`harness/diff/fat_diff/conkeys.sh:1`|37|Keyboard semantics use printed manual vectors; FAT extraction/allocation is|
|`harness/diff/fat_diff/copy_stamp.sh:1`|102|initech-qqrz: independent mcopy -m / mdir / fsck.fat on identical seeds.|
|`harness/diff/fat_diff/del_b023.sh:1`|27|Replay the ORIGINAL audit batch, answering the newly supported question.|
|`harness/diff/fat_diff/del_directory.sh:1`|71|MS-DOS 3.3 Reference p.56 / PDF141: a directory operand deletes its files.|
|`harness/diff/fat_diff/dir_match.sh:1`|74|MS-DOS 3.3 User's Reference printed pp.57-58 (PDF142-143), wildcards|
|`harness/diff/fat_diff/diskcopy_emu.sh:1`|27|One honest QMP medium replacement attempt, fixed ATA --disk2 hardware.|
|`harness/diff/fat_diff/dos_break.sh:1`|47|Written before C-6 implementation. Ref: DEC-16 C-6, MS-DOS 3.3 p.38.|
|`harness/diff/fat_diff/dos_break_program.asm:1`|45|Test fixture, never shipped. DEC-16 C-6; MS-DOS 3.3 Reference p.38.|
|`harness/diff/fat_diff/dos_folder.sh:1`|61|Factory only. Independent roster + mtools/dosfstools on the identical image,|
|`harness/diff/fat_diff/dos_not_ready.sh:1`|29|Independent literal + rendered console, not strings/image-presence testing.|
|`harness/diff/fat_diff/dos_parity.sh:1`|232|Four manual-derived parity gates. No expected text is read from command.c.|
|`harness/diff/fat_diff/dos_resident_break.sh:1`|68|initech-c80y: replay audit controlshort/gaps2 on PRIVATE disk copies.|
|`harness/diff/fat_diff/dos_safety.sh:1`|305|Thin mtools/QEMU differential. References: Microsoft MS-DOS 3.3 User's|
|`harness/diff/fat_diff/dos_safety_fixture.c:1`|53|Factory-only deterministic fixtures and printable-text QMP key encoder.|
|`harness/diff/fat_diff/dos_volumes.sh:1`|80|Two independent reference images: C has an MBR, A is the DEC-18 slave.|
|`harness/diff/fat_diff/dosmsg_fault.c:1`|23|Factory-only disconnected-device fixture. The live ATA read remains intact;|
|`harness/diff/fat_diff/dosmsg_mutants.sh:1`|50|violation. Never edit shipped source or a shared generated header here.|
|`harness/diff/fat_diff/dosutil_diff.sh:1`|101|Expected attribute bytes: mattrib; labels: mlabel + mdir + fsck.fat.|
|`harness/diff/fat_diff/dosutil_emu.sh:1`|56|Real FAT-loaded COMs + COMMAND.COM VOL, m0dc injected-trace pattern.|
|`harness/diff/fat_diff/dosutil_fixture.sh:1`|13|Deterministic source image, cloned before every differential. Factory only.|
|`harness/diff/fat_diff/drive_operands.sh:1`|138|initech-q45q: drive operands of CD and the external commands on the DEC-18|
|`harness/diff/fat_diff/edlin.sh:1`|121|Same-image mtools/dosfstools differential, m0dc injected-console pattern.|
|`harness/diff/fat_diff/fat12_ref.py:1`|570|- independent FAT12 reference reader.|
|`harness/diff/fat_diff/fat16_ref.py:1`|250|- independent FAT16 reference reader.|
|`harness/diff/fat_diff/fat_dump.c:1`|333|- FAT12 manifest/content dumper (FACTORY).|
|`harness/diff/fat_diff/fat_spc.sh:1`|53|PRD 6.1 differential: real mformat -c images, mtools + independent Python.|
|`harness/diff/fat_diff/fdisk.sh:1`|54|Identical-image MBR differential, mpartition plus util-linux sfdisk.|
|`harness/diff/fat_diff/fdisk_emu.sh:1`|107|Menus driven by injected keys; pixels decoded by harness/console_text.c.|
|`harness/diff/fat_diff/formatsys.sh:1`|230|Authored before diskutil.c. Law 1: MS-DOS UG pp74-76,119; IBM TR|
|`harness/diff/fat_diff/formatsys_emu.sh:1`|205|Injected manual prompts + console_text pixels and mtools on scratch disks.|
|`harness/diff/fat_diff/int21_dpb.sh:1`|171|INT 21h AH=1Fh / AH=32h differential (WL-0167, bead initech-dpb1).|
|`harness/diff/fat_diff/int21_dpb_program.asm:1`|154|Test-only flat program, not shipped (WL-0167). Drives the real INT 21h|
|`harness/diff/fat_diff/kernel_hygiene.sh:1`|55|Law 1: independent mtools operations on IDENTICAL starting images.|
|`harness/diff/fat_diff/kernel_hygiene_emu.sh:1`|25|Injected shell trace + independent mtools readback, same crafted layout.|
|`harness/diff/fat_diff/locked_trash.sh:1`|63|L118: stopped-image mtools/fsck judgment, independent of Finder's model.|
|`harness/diff/fat_diff/mz_entry_emu.sh:1`|22|ADR-0003 DEC-08a.2: reject entry at module end through the real FAT loader.|
|`harness/diff/fat_diff/partition.sh:1`|120|Identical-image differential: real kernel FAT mount vs mtools/dosfstools.|
|`harness/diff/fat_diff/redir_full.sh:1`|128|initech-66oc: COMMAND.COM redirection and pipes on a full or protected|
|`harness/diff/fat_diff/resident_boot_write.asm:1`|28|Rule-6 oracle mutant: change the private C: boot sector's OEM byte.|
|`harness/diff/fat_diff/shell_dir_look.sh:1`|51|DOS 3.3 manual pp.28-29 / PDF 41-42, p.45 / PDF 130.|
|`harness/diff/fat_diff/shell_files.sh:1`|126|Independent file bytes + mcopy/mren inventories on identical FAT seeds.|
|`harness/diff/fat_diff/shell_files_host.h:1`|71|Factory bridge for command.c's actual builtins. ADR-0003 App A/DEC-04a.|
|`harness/diff/fat_diff/shell_permanent.sh:1`|44|IBM 6138519_DOS_3.10_Reference_Feb85.pdf pp. 7-53/7-54.|
|`harness/diff/fat_diff/trash_hidden_emu.sh:1`|18|Runtime service attribute, independently read back by mtools.|
|`harness/diff/fat_diff/trash_purge_views.sh:1`|113|- the emulator gate for bead|
|`harness/diff/fat_diff/truncate.c:1`|93|AH=40h CX=0 oracle. IBM DOS 3.30 Technical Reference p.6-140;|
|`harness/diff/fat_diff/truncate.sh:1`|71|Identical FAT12 seeds, independent mcopy replacements and free-space counts.|
|`harness/diff/fat_diff/truncate_program.asm:1`|112|Test-only flat program, not shipped. IBM DOS 3.30 TR p.6-140; ADR-0003 A.|
|`harness/diff/fat_diff/xcopy_cwd.sh:1`|33|Reference pp.7-8,127: relative source is resolved from the working directory.|
|`harness/diff/fat_diff/xcopy_tree.sh:1`|220|Thin mtools/dosfstools driver; factory logic and rendering stay in C.|
|`harness/diff/wk1_diff/mint_i123_copy_steps.c:1`|30|Factory-only translation of mint_drive steps to DOSBox-X AUTOTYPE.|
|`harness/diff/wk1_diff/mint_i123_numeric.c:1`|57|Input-only builder for genuine Lotus R2.2 numeric mints. The output is|
|`harness/diff/wk1_diff/wk1_canon.py:1`|128|the INDEPENDENT side of test-i123-wk1-roundtrip (factory).|
|`harness/diff/wk1_diff/wk1_formula_ref.py:1`|66|the INDEPENDENT side of test-i123-formula (factory).|
|`harness/diff/wp5_diff/iword_keys.h:1`|148|- drive the InitechWord core with the|
|`harness/diff/wp5_diff/replace.sh:1`|15|Reuse the independent MILTON/mtools identical-image input oracle. The|
|`harness/diff/wp5_diff/search.sh:1`|33|Read-only FAT differential: identical images, MILTON --cat versus mtools|
|`harness/diff/wp5_diff/settings.sh:1`|18|ADR-0003 Appendix A READ; IBM DOS 3.30 6-137..6-140. Captured tenant|
|`harness/diff/wp5_diff/wp51_headless.c:1`|99|headless SDL2 driver for the REAL WP 5.1 mint.|
|`harness/edlin/host.c:1`|28|Native execution of the artifact through a DOS-I/O seam; NOT the oracle.|
|`harness/edlin/model.c:1`|195|Independent, heap/vector EDLIN command model (never linked into the OS).|
|`harness/emu/bochs.c:1`|1081|InitechOS Bochs oracle harness (the Bochs leg of the|
|`harness/emu/bochs.h:1`|167|InitechOS Bochs oracle harness (public API)|
|`harness/emu/bochs_leg_check.sh:1`|68|WL-0170: the ONE Bochs boot-leg serial verdict (Rule 5; PRD Sec 8). Every|
|`harness/emu/bochs_main.c:1`|168|CLI wrapper around the Bochs oracle harness (initech-564).|
|`harness/emu/bochs_pixel_leg.sh:1`|55|WL-0170 (initech-u9gf): the Bochs PIXEL leg. Boots the golden image on QEMU|
|`harness/emu/bochs_reboot_check.sh:1`|10|Independent serial oracle for the factory reset fixture, PRD Sec 8.|
|`harness/emu/bochs_trace.h:1`|194|Factory RFB input vocabulary. RFC 6143 Sec 7.5.4/5. The m/l/w/k forms|
|`harness/emu/calcda_power_check.sh:1`|16|Independent lifecycle golden: guide p.125; IM VI p.345; B04 audit L022.|
|`harness/emu/command_file.sh:1`|34|initech-e9sx negative oracle: initial interpreter load (manual printed p310),|
|`harness/emu/copybreak_check.sh:1`|20|Audit DAS L002; Macintosh System Software User's Guide pp.125,152;|
|`harness/emu/copybreak_seed.c:1`|19|Independent one-clip fixture: spec/notepad/store.json. Factory only. */|
|`harness/emu/drive_flair.py:1`|152|"""Drive the live FLAIR tenants desktop through interaction storyboards.|
|`harness/emu/exec_high_check.sh:1`|14|DEC-03a.2-.5: same independent oracle for production and runtime mutants.|
|`harness/emu/fixtures/bochs_reboot.asm:1`|111|Factory-only boot fixture, initech-cqej / PRD Sec 8. Same 8042 pulse as|
|`harness/emu/fixtures/hang.asm:1`|34|harness self-test fixture: a guest that never finishes.|
|`harness/emu/fixtures/multiboot.ld:1`|32|linker script for the harness self-test fixtures.|
|`harness/emu/fixtures/serial_hello.asm:1`|65|harness self-test fixture: boot + serial proof.|
|`harness/emu/fixtures/triple_fault.asm:1`|49|harness self-test fixture: deliberate triple fault.|
|`harness/emu/ovl_panic.sh:1`|40|one leg of the kernel-overlay refusal / context gates|
|`harness/emu/panic_reasons.sh:1`|55|CLAUDE Rule 2; ADR-0003 DEC-08a.5; DOS L030 / DAS L001 audit evidence.|
|`harness/emu/panic_screen.sh:1`|43|initech-s25. Locked injected trace: COMMAND.COM -> FAT PANIC.COM -> #UD/#GP.|
|`harness/emu/power_notice_check.sh:1`|14|B04 / IM VI p.345: cancelling a dirty document keeps it and the Finder alive.|
|`harness/emu/qemu.c:1`|2776|InitechOS QEMU oracle harness (implementation)|
|`harness/emu/qemu.h:1`|354|InitechOS QEMU oracle harness (public API)|
|`harness/emu/qemu_main.c:1`|341|CLI wrapper around the QEMU oracle harness|
|`harness/emu/realdos_run.sh:1`|29|run DOS programs under a REAL MS-DOS boot diskette in|
|`harness/emu/require_fault_trace.sh:1`|30|Explicit deliberate-fault oracle, NOT a successful guest completion.|
|`harness/emu/require_trace.sh:1`|10|PRD 8 / Rule 6: absence means regression only AFTER a real completed trace.|
|`harness/emu/rfb_unblock.py:1`|74|headless-Bochs DIAGNOSIS helper (beads initech-6pj/564).|
|`harness/emu/samir_boot_check.sh:1`|23|ADR-0009 DEC-08 / PRD Sec 6.6; the SAME CLIENTS value fixture and M005U/M005N|
|`harness/emu/samir_high_stack.sh:1`|36|DEC-03a.5, initech-jarzu. The real III+ by-value depth capture must complete;|
|`harness/emu/samir_link_reserve.asm:1`|14|Independent DEC-03a.8 link-bound fixture. No application code or packer.|
|`harness/emu/shutdown_cancel_check.sh:1`|38|initech-cqej: same locked FLAIR_SHUTDOWN_APP_SPEC as the consenting leg.|
|`harness/emu/snapshot_write.asm:1`|43|Factory oracle only. BIOS INT 13h changes one private raw-disk sector.|
|`harness/emu/test_dos_control.sh:1`|48|K12 oracle: real PS/2 -> cooked CON -> COMMAND.COM, independent mcopy.|
|`harness/emu/test_qemu_log.sh:1`|34|initech-tfn6r; PRD Sec 8 / Rule 6. Real QEMU fixtures, not a fake logger.|
|`harness/emu/test_snapshot.sh:1`|31|harness=$1; sector=$2; debug_sector=$3|
|`harness/emu/tsr_exec_check.sh:1`|29|Independent serial oracle shared by the clean kernel and every mutant.|
|`harness/emu/vesa8.sh:1`|48|initech-2gva; oracle written before the VBE/DAC changes. Reuse the SAME|
|`harness/factory_smoke.c:1`|29|InitechOS factory toolchain smoke stub|
|`harness/fixtures/dos33_overlay/exit.asm:1`|5|exit.asm; source/contract input (role inferred from path)|
|`harness/fixtures/dos33_overlay/mint.sh:1`|44|Independent DOS33 mint; never changes the reference disk or saved captures.|
|`harness/fixtures/dos33_overlay/overlay.asm:1`|6|dw 0x5a4d,64,1,1,3,0,0,0,0,0,0,0,32,0,0,0|
|`harness/fixtures/dos33_overlay/probe.asm:1`|74|Independent DOS 3.30 capture; never executed as InitechDOS artifact code.|
|`harness/menubar_clock_oracle.c:1`|84|cap rows5..13, right ink592); goldens/resources/NFNT_5478.bin (Chicago12).|
|`harness/menubar_clock_oracle.h:1`|14|Factory only, tdnl.13: independent Mac OS 8 bar + original Chicago NFNT.|
|`harness/missing_reference.sh:1`|20|Thin shell counterpart of oracle_policy.h; called only for absent references.|
|`harness/oracle_policy.h:1`|48|Factory only. Ref: CLAUDE.md Law 2 / Rule 2; initech-iv8c.|
|`harness/proptest/expect_abort.h:1`|63|(void)fflush(NULL); \|
|`harness/proptest/fixtures/mech_policy_mutant.c:1`|30|- the MECH_POLICY_MUTANT|
|`harness/proptest/flair_look_sentinel_stub.c:1`|60|the SENTINEL policy stub for|
|`harness/proptest/flair_routing_stubs.c:1`|83|the link seams test-flair-routing does NOT grade.|
|`harness/proptest/font_styles_golden.h:1`|198|- INDEPENDENT golden crops of styled|
|`harness/proptest/gdi_ref_wine.c:1`|116|- the wine-only translation unit for the|
|`harness/proptest/gdi_ref_wine.h:1`|57|- the host-only INDEPENDENT-GOLDEN interface.|
|`harness/proptest/gdi_ref_wine_shim.h:1`|177|- host-only extraction shim that lets|
|`harness/proptest/gdi_ref_winestubs/ntstatus.h:1`|9|- EMPTY host-only stub.|
|`harness/proptest/gdi_ref_winestubs/request.h:1`|9|- EMPTY host-only stub.|
|`harness/proptest/gdi_ref_winestubs/user.h:1`|9|- EMPTY host-only stub.|
|`harness/proptest/gdi_ref_winestubs/winternl.h:1`|9|- EMPTY host-only stub.|
|`harness/proptest/kernel_cold_answer.c:1`|2|Factory-only cross-TU size oracle; never linked into a shipped image. */ **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`harness/proptest/kernel_cold_entry.c:1`|9|Factory-only cross-TU size oracle. Ref: PRD Sec 5/7; Rule 6/11. **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`harness/proptest/kernel_df_check.sh:1`|42|Intel SDM Vol. 1 Sec. 3.4.3.2 (DF flag): C entry needs forward strings.|
|`harness/proptest/kernel_gc_fixture.c:1`|24|Kernel-only section-GC oracle, initech-tode.|
|`harness/proptest/kernel_gc_native.c:1`|10|Round 8: a dead native section must not root an LTO callee permanently.|
|`harness/proptest/kernel_transfer_check.sh:1`|48|Round-2 Sec 4 / loader_context_t offset assertions. Check the actual linked|
|`harness/proptest/kernel_transfer_mutants.sh:1`|46|Mutate real linked instructions: unsafe ESP source and an extra transfer|
|`harness/proptest/ovl_ptr_check.sh:1`|56|ADR-0014 DEC-14-6.2 / DEC-14-9(c): "every code pointer|
|`harness/proptest/ovl_reader.c:1`|147|INDEPENDENT host reader of C:\FLAIR.OVL (ADR-0014 DEC-14-9:|
|`harness/proptest/serial_trace_fixture.c:1`|34|storage-only serial transform; cover both numeric limits and string syntax. */|
|`harness/proptest/tbx_trace_fixture.c:1`|21|Original C transcript oracle for the Toolbox's callback-based serial sink.|
|`harness/proptest/test_asset_lz.c:1`|87|- storage equality for the packed strikes|
|`harness/proptest/test_chicago_pack.c:1`|20|Storage oracle: the LOCKED Chicago row table stays the independent input;|
|`harness/proptest/test_flair_part_map.c:1`|42|- the PART resolution table, before|
|`harness/proptest/test_geneva12_pack.c:1`|23|Storage oracle (bead initech-uelk): the LOCKED Geneva 12 row table|
|`harness/proptest/test_kmain_cold.c:1`|23|The original About-dialog text/units, independent of its moved formatter. **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`harness/pubs/aux_check.sh:1`|62|GC-07 and CLAUDE.md Rules 1/6. Use real LuaLaTeX, never a stub golden.|
|`harness/pubs/check_allowlist.sh:1`|13|GC-01: compare both uncommitted changes and the previous committed state.|
|`harness/pubs/check_flair.sh:1`|57|the FLAIR books' figure and operation proofs (bead|
|`harness/pubs/check_screens.sh:1`|24|GC-03/04: every printed figure has replay input and expected text.|
|`harness/pubs/cover_check.c:1`|109|the identity leg of test-pubs-families (PUB-REF-0001|
|`harness/pubs/dos_surface_inventory.c:1`|39|PUB-REF-0001 GC-01 (bead initech-ipu5.7 integration): the batch|
|`harness/pubs/entry_index_check.sh:1`|67|Exercise the real pubs.mk discovery/rule in an isolated source tree.|
|`harness/pubs/families.sh:1`|94|Compile every PG-10..26 family branch, not just the IBM pilot branch.|
|`harness/pubs/flair_alerts.sh:1`|45|the SHIPPED FLAIR alerts and dialogs, for the FLAIR|
|`harness/pubs/flair_app_menus.c:1`|59|- the menu bar of ONE shipped FLAIR disk|
|`harness/pubs/flair_grey.c:1`|414|- FLAIR screen reproductions for the manuals|
|`harness/pubs/flair_inventory.c:1`|414|- the SHIPPED FLAIR surface, for the|
|`harness/pubs/flair_mutants.sh:1`|108|Rule 6 for the FLAIR manuals' gates (bead|
|`harness/pubs/int21_inventory.c:1`|11|PUB-REF-0001 GC-01 / CP-08. Call the actual dispatcher's controlled|
|`harness/pubs/iword_check.sh:1`|110|the InitechWord manuals' correctness gate (bead|
|`harness/pubs/iword_features.sh:1`|15|CP-17 leg of test-pubs-iword (bead initech-ipu5.10):|
|`harness/pubs/iword_mutants.sh:1`|97|Rule 6 for test-pubs-iword (bead initech-ipu5.10): the|
|`harness/pubs/iword_probe.c:1`|973|the SHIPPED SURFACE of InitechWord and the REPLAY ENGINE|
|`harness/pubs/label_check.sh:1`|34|initech-77am: MS-DOS 3.3 User's Guide, LABEL pp.86-87 / PDF171-172;|
|`harness/pubs/label_mutants.sh:1`|21|gate=$1; entry=$2; keys=$3; screen=$4; out=$5|
|`harness/pubs/mutants.sh:1`|187|No tracked publication, executable or allow-list is ever changed here.|
|`harness/pubs/ppm_grey_png.c:1`|217|the PIXEL ROUTE of a FLAIR screen figure (PUB-REF-0001|
|`harness/pubs/pubs_gate.c:1`|850|Publications correctness oracle. Ref: PUB-REF-0001 GC-01/03/05/06,|
|`harness/pubs/railroad.c:1`|553|the Turbo Initech Programmer's Guide grammar: railroad|
|`harness/pubs/replay.sh:1`|41|Replay every emitted Transcript and DotExample (PUB-REF-0001 GC-03).|
|`harness/pubs/samir_candidates.sh:1`|31|PUB-REF-0001 GC-01 / CP-11 (beads initech-ipu5.8, initech-u01i): the|
|`harness/pubs/samir_inventory.c:1`|212|PUB-REF-0001 GC-01 / CP-11 (bead initech-ipu5.8): the shipped InitechBase|
|`harness/pubs/samir_programs.sh:1`|31|GC-11 for InitechBase (bead initech-ipu5.8): the month-end programs of the|
|`harness/pubs/samir_replay.c:1`|135|PUB-REF-0001 GC-03. Same host PAL/interpreter route as dbf_diff.|
|`harness/pubs/samir_sample_disk.c:1`|378|The InitechBase "Sample Programs and Utilities" diskette (PUB-REF-0001|
|`harness/pubs/samir_tables.c:1`|217|Generated tables of the InitechBase manuals (bead initech-ipu5.8;|
|`harness/pubs/slip_mutants.sh:1`|75|VO-30 mutants (CLAUDE.md Rule 6; bead initech-ipu5.16): each perturbs a|
|`harness/pubs/slip_places.sh:1`|46|VO-30 (PUB-REF-0001 Sec 9.3; bead initech-ipu5.16): the class reports where|
|`harness/pubs/srclockstep.h:1`|562|Source grammar for the InitechBase merge gate (initech-j29).|
|`harness/pubs/srclockstep_mutants.sh:1`|215|Rule 6 for `pubs_gate srclockstep` (bead|
|`harness/pubs/tps_inventory.c:1`|394|the SHIPPED inventory of the TPS Report Generator's|
|`harness/pubs/tps_mutants.sh:1`|71|Rule 6 for the TPS Report Generator manual's gates|
|`harness/pubs/turbo_asmblock.sh:1`|11|print the lines of a Turbo Initech TPSOUT.S|
|`harness/pubs/turbo_gate.c:1`|532|the Turbo Initech manuals' shipped inventory and its|
|`harness/pubs/turbo_gates_cited.sh:1`|19|GC-09 for the Turbo Initech manuals (bead|
|`harness/pubs/turbo_grammar.sh:1`|37|the printed Turbo Initech grammar against the shipped|
|`harness/pubs/turbo_mutants.sh:1`|56|Rule 6 for the Turbo Initech manual gates (bead|
|`harness/pubs/turbo_replay.sh:1`|157|GC-03 for the Turbo Initech manuals (bead initech-ipu5.12;|
|`harness/registry_mutants.sh:1`|106|checker=$(realpath "$1")|
|`harness/render/alert_capture.h:1`|105|Independent runtime oracle input for tdnl.99/.100 and L023 (Law 2).|
|`harness/render/cursor_capture.h:1`|74|M009 independent per-cell oracle (PRD Sec 6.3; CLAUDE.md Laws 1/2).|
|`harness/render/icon_capture.h:1`|229|- the INDEPENDENT Finder-icon golden|
|`harness/render/progress_golden.h:1`|174|Independent runtime oracle input for initech-bf2 (Law 2): the Mac OS 8.1|
|`harness/render/render.c:1`|288|- the FLAIR host-render skeleton (FACTORY, host-only).|
|`harness/render/render.h:1`|167|- the FLAIR host-render skeleton (FACTORY, host-only).|
|`harness/ssim.c:1`|262|Factory-only SSIM guide, initech-w06 (PRD Sec 3/8/14; Law 3/4).|
|`harness/verify_fixture_volume.sh:1`|45|Expectations are explicit Makefile contracts, not inferred from an image.|
|`tools/asset_check.c:1`|333|asset-extraction v0 gate checker (factory, C11-only).|
|`tools/asset_lz.c:1`|124|- build-time STORAGE packing of locked strike data for|
|`tools/bin2c.c:1`|74|embed a binary file as a C byte array (deterministic).|
|`tools/boot_logo_gen.c:1`|235|- factory-only generator for spec/assets/boot_logo.h,|
|`tools/bootstrap/doctor.sh:1`|117|is this machine ready to build, test and mint InitechOS?|
|`tools/bootstrap/fetch-siblings.sh:1`|42|clone the six ground-truth sibling repos next to this|
|`tools/bootstrap/snapshot-sibling.sh:1`|50|refresh a sibling's private release asset: tar every|
|`tools/calc_ledger_gen.c:1`|67|C factory generator; locked flat schema, PRD Appendix A/B (tdnl.20).|
|`tools/cert-check.sh:1`|103|export LC_ALL|
|`tools/cert-sharded-selftest.sh:1`|94|Independent fake make gates exercise the real runner, without emulators.|
|`tools/cert-sharded.sh:1`|189|Parallel certificate: docs/design/parallel-certificate.md Sec 3, initech-9bgg6.|
|`tools/cert-summary.sh:1`|34|Certificate evidence grammar, PRD 8/9; initech-0spyf/fzsvo.|
|`tools/check-samir-parallel.sh:1`|31|Acceptance evidence, PRD 8 / Rule 11. Never normalize a failing gate away.|
|`tools/chicago_pack.c:1`|65|Build-time storage packing, not a new font. Ref: PRD Sec 6.4;|
|`tools/color_canon_extract.c:1`|737|FLAIR Color Canon generator (factory, C11-only).|
|`tools/culprit.sh:1`|91|Local first-parent regression search (initech-3nc3v); PRD Sec 8/9/14.|
|`tools/da_store_check.c:1`|33|Independent FAT file oracle, factory C, tdnl.18. No artifact/spec headers.|
|`tools/egg_jtc_pack.c:1`|70|Factory C: locked JSON -> compact data -> bin2c. Ref: initech-2ter (a).|
|`tools/flair_dac_gen.c:1`|127|initech-2gva: factory-only, deterministic C. Ref: color_canon.json entries|
|`tools/flairovl.c:1`|108|the FLAIR.OVL factory packer (ADR-0014 DEC-14-2; epic|
|`tools/font_art2h.c:1`|230|- transcribe a hand-authored ASCII-art strike|
|`tools/gate_map.c:1`|836|Locked gate selection: initech-fw3x0; PRD Sec 8/9/14, CLAUDE.md Rules 1/6/8.|
|`tools/gen_block_device_spec.c:1`|40|Factory C. DEC-17 JSON -> checked C header, not an oracle of correctness. */|
|`tools/gen_drive_letters.c:1`|17|Factory C; DEC-18 JSON -> header. Equality is checked independently. */|
|`tools/geneva12_art2h.c:1`|189|- transcribe the hand-authored Geneva 12 strike|
|`tools/iword_fkeys_gen.c:1`|47|Deterministic compiler for the LOCKED, line-record fkeys.json schema.|
|`tools/lane-cert.sh:1`|159|export LC_ALL|
|`tools/lane-env.sh:1`|29|- source this in a lane / PM shell before building.|
|`tools/merge-train.sh:1`|181|apply lane diffs onto main, run the per-merge SAMIR gate|
|`tools/mint_finder_small_icons.c:1`|40|tdnl.44 factory transcriber. Both independent Mac OS8.1 list captures|
|`tools/mk_boot_system.sh:1`|63|Reproducible two-file FAT12 BOOT volume, initech-kod.|
|`tools/mk_ovl_tables.sh:1`|122|GENERATE the kernel-overlay tables (ADR-0014 DEC-14-1,|
|`tools/mzlink.c:1`|373|the InitechMZ (.EXE) container linker (host factory tool).|
|`tools/palette_extract.c:1`|486|desktop palette extraction tool (factory, C11-only).|
|`tools/ppm_region_count.c:1`|79|- count the pixels of a rectangle of a 640x480 P6|
|`tools/proof-replay.sh:1`|49|export LC_ALL|
|`tools/pubs_msgtable.c:1`|187|emit the InitechDOS message list for the manuals from|
|`tools/revert-merge.sh:1`|225|one-command recovery from a bad lane merge. Reverts a|
|`tools/samir_memory_ld.c:1`|14|Factory: DEC-03a.4/.8, initech-fxolx. One contract feeds ld, emitter, PAL.|
|`tools/samir_msg_pack.c:1`|69|Deterministic byte-pair storage for the locked III+ DBASE.MSG catalogue.|
|`tools/samir_ovl_pack.c:1`|397|split the SAMIR link into SAMIR.EXE's resident module|
|`tools/sentinel.sh:1`|201|PM-local post-push sentinel (initech-3nc3v); PRD Sec 8/9/14, Rule 10.|
|`tools/serial_trace_pack.c:1`|246|Deterministic serial marker storage transform (initech-tode round 4).|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/CANON-MANIFEST.md:1`|330|INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->|
|`spec/README.md:1`|39|spec/ — LOCKED specs-as-data (JSON / C headers)|
|`spec/gate_map.json:1`|4462|"families": {|
|`spec/pubs_inconsistencies.json:1`|178|"_comment": "LOCKED spec-data (CLAUDE.md Rule 8): the house register of tolerated inconsistencies, PUB-REF-0001 VO-30 (docs/pubs/DESIGN-REFERENCE.md Sec 9.3). Every slip a manual carries is |
|`spec/samir/README.md:1`|52|Provenance Ledger|
|`spec/win95ism_guardrails.md:1`|247|INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`harness/diff/dbf_diff/mint_clients_dbf.c:60`|`samir/dbf.h` → `os/samir/include/samir/dbf.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/mint_clients_dbf.c:61`|`samir/value.h` → `os/samir/include/samir/value.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/mint_invoice_dbf.c:64`|`samir/dbf.h` → `os/samir/include/samir/dbf.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/mint_invoice_dbf.c:65`|`samir/value.h` → `os/samir/include/samir/value.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/mint_invoice_dbf.c:66`|`samir/rt.h` → `os/samir/include/samir/rt.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/samir_core_ref.c:10`|`test_assert.h` → `seed/test_assert.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/w2_fixture.c:7`|`samir/dbf.h` → `os/samir/include/samir/dbf.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/w2_fixture.c:8`|`samir/dbt.h` → `os/samir/include/samir/dbt.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/w2_fixture.c:9`|`samir/value.h` → `os/samir/include/samir/value.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/w2_host.c:14`|`../../../os/samir/samir_main.c` → `os/samir/samir_main.c`|INTERNAL LEAK|
|`harness/diff/dbf_diff/w2_host.h:3`|`samir/pal.h` → `os/samir/include/samir/pal.h`|INTERNAL LEAK|
|`harness/diff/dbf_diff/w2_host.h:4`|`samir/interp.h` → `os/samir/include/samir/interp.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/blockdev_file.h:20`|`blockdev.h` → `os/milton/blockdev.h`|PUBLIC INTERFACE|
|`harness/diff/fat_diff/dosmsg_fault.c:4`|`ata.h` → `os/milton/ata.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/fat_dump.c:43`|`fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/truncate.c:7`|`fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/truncate.c:8`|`int21.h` → `os/milton/int21.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/truncate.c:9`|`fileio_fat.h` → `os/milton/fileio_fat.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/truncate.c:10`|`psp.h` → `os/milton/psp.h`|INTERNAL LEAK|
|`harness/diff/fat_diff/truncate.c:11`|`sft.h` → `os/milton/sft.h`|INTERNAL LEAK|
|`harness/diff/wk1_diff/mint_i123_numeric.c:8`|`i123.h` → `os/i123/core/i123.h`|INTERNAL LEAK|
|`harness/diff/wp5_diff/iword_keys.h:50`|`iword.h` → `os/iword/core/iword.h`|INTERNAL LEAK|
|`harness/edlin/host.c:10`|`edlin.h` → `os/milton/edlin.h`|INTERNAL LEAK|
|`harness/proptest/flair_look_sentinel_stub.c:27`|`flair_look.h` → `os/flair/flair_look.h`|INTERNAL LEAK|
|`harness/proptest/flair_look_sentinel_stub.c:28`|`flair_skins.h` → `spec/flair_skins.h`|PUBLIC INTERFACE|
|`harness/proptest/flair_look_sentinel_stub.c:29`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:21`|`blitter.h` → `os/flair/blitter.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:22`|`control.h` → `os/flair/control.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:23`|`desktop.h` → `os/flair/desktop.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:24`|`dialog.h` → `os/flair/dialog.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:25`|`event.h` → `os/flair/event.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:26`|`scrap.h` → `os/flair/scrap.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:27`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:28`|`text.h` → `os/flair/text.h`|INTERNAL LEAK|
|`harness/proptest/flair_routing_stubs.c:29`|`int21.h` → `os/milton/int21.h`|INTERNAL LEAK|
|`harness/proptest/test_asset_lz.c:19`|`desk_icons.h` → `spec/assets/desk_icons.h`|PUBLIC INTERFACE|
|`harness/proptest/test_asset_lz.c:20`|`finder_icons.h` → `spec/assets/finder_icons.h`|PUBLIC INTERFACE|
|`harness/proptest/test_asset_lz.c:21`|`alert_icons.h` → `spec/assets/alert_icons.h`|PUBLIC INTERFACE|
|`harness/proptest/test_asset_lz.c:22`|`finder_small_icons.h` → `spec/assets/finder_small_icons.h`|PUBLIC INTERFACE|
|`harness/proptest/test_asset_lz.c:23`|`boot_logo.h` → `spec/assets/boot_logo.h`|PUBLIC INTERFACE|
|`harness/proptest/test_asset_lz.c:24`|`asset_lz.h` → `os/flair/asset_lz.h`|INTERNAL LEAK|
|`harness/proptest/test_asset_lz.c:25`|`asset_lz_data.h` → `os/flair/asset_lz_data.h`|INTERNAL LEAK|
|`harness/proptest/test_chicago_pack.c:5`|`chicago12.h` → `spec/assets/chicago12.h`|PUBLIC INTERFACE|
|`harness/proptest/test_chicago_pack.c:6`|`chicago_packed.h` → `os/flair/chicago_packed.h`|INTERNAL LEAK|
|`harness/proptest/test_flair_part_map.c:16`|`flair_look.h` → `os/flair/flair_look.h`|INTERNAL LEAK|
|`harness/proptest/test_flair_part_map.c:17`|`flair_skins.h` → `spec/flair_skins.h`|PUBLIC INTERFACE|
|`harness/proptest/test_geneva12_pack.c:7`|`geneva12.h` → `spec/assets/geneva12.h`|PUBLIC INTERFACE|
|`harness/proptest/test_geneva12_pack.c:8`|`geneva12_packed.h` → `os/flair/geneva12_packed.h`|INTERNAL LEAK|
|`harness/proptest/test_kmain_cold.c:6`|`kmain_cold.h` → `os/milton/kmain_cold.h`|INTERNAL LEAK|
|`harness/pubs/dos_surface_inventory.c:15`|`../../os/milton/batch.c` → `os/milton/batch.c`|INTERNAL LEAK|
|`harness/pubs/dos_surface_inventory.c:16`|`../../os/milton/config_sys.c` → `os/milton/config_sys.c`|INTERNAL LEAK|
|`harness/pubs/flair_inventory.c:53`|`../../os/flair/finder_menu.c` → `os/flair/finder_menu.c`|INTERNAL LEAK|
|`harness/pubs/flair_inventory.c:54`|`../../os/flair/finder_cmd.c` → `os/flair/finder_cmd.c`|INTERNAL LEAK|
|`harness/pubs/flair_inventory.c:55`|`../../os/flair/apple_menu.c` → `os/flair/apple_menu.c`|INTERNAL LEAK|
|`harness/pubs/flair_inventory.c:56`|`shell_menus.h` → `os/flair/shell_menus.h`|INTERNAL LEAK|
|`harness/pubs/flair_inventory.c:57`|`menu_canon.h` → `spec/assets/menu_canon.h`|PUBLIC INTERFACE|
|`harness/pubs/int21_inventory.c:5`|`../../os/milton/int21.c` → `os/milton/int21.c`|INTERNAL LEAK|
|`harness/pubs/iword_probe.c:93`|`../../os/iword/app/face_a.c` → `os/iword/app/face_a.c`|INTERNAL LEAK|
|`harness/pubs/samir_inventory.c:52`|`samir/interp.h` → `os/samir/include/samir/interp.h`|INTERNAL LEAK|
|`harness/pubs/samir_inventory.c:53`|`samir/dbf.h` → `os/samir/include/samir/dbf.h`|INTERNAL LEAK|
|`harness/pubs/samir_inventory.c:54`|`../../os/samir/samir_main.c` → `os/samir/samir_main.c`|INTERNAL LEAK|
|`harness/pubs/samir_replay.c:41`|`samir/interp.h` → `os/samir/include/samir/interp.h`|INTERNAL LEAK|
|`harness/pubs/samir_sample_disk.c:67`|`samir/dbf.h` → `os/samir/include/samir/dbf.h`|INTERNAL LEAK|
|`harness/pubs/samir_sample_disk.c:68`|`samir/dbt.h` → `os/samir/include/samir/dbt.h`|INTERNAL LEAK|
|`harness/pubs/samir_sample_disk.c:69`|`samir/ndx.h` → `os/samir/include/samir/ndx.h`|INTERNAL LEAK|
|`harness/pubs/samir_sample_disk.c:70`|`samir/value.h` → `os/samir/include/samir/value.h`|INTERNAL LEAK|
|`harness/pubs/samir_sample_disk.c:71`|`samir/rt.h` → `os/samir/include/samir/rt.h`|INTERNAL LEAK|
|`harness/pubs/samir_tables.c:35`|`../../os/samir/cmd/flow.c` → `os/samir/cmd/flow.c`|INTERNAL LEAK|
|`harness/pubs/samir_tables.c:36`|`../../os/samir/cmd/proc.c` → `os/samir/cmd/proc.c`|INTERNAL LEAK|
|`harness/pubs/samir_tables.c:37`|`../../os/samir/cmd/report.c` → `os/samir/cmd/report.c`|INTERNAL LEAK|
|`harness/pubs/samir_tables.c:38`|`../../os/samir/samir_main.c` → `os/samir/samir_main.c`|INTERNAL LEAK|
|`harness/pubs/samir_tables.c:39`|`dbf_format.h` → `spec/samir/dbf_format.h`|PUBLIC INTERFACE|
|`harness/pubs/samir_tables.c:40`|`ndx_format.h` → `spec/samir/ndx_format.h`|PUBLIC INTERFACE|
|`harness/pubs/turbo_gate.c:47`|`lexer.h` → `seed/lexer.h`|INTERNAL LEAK|
|`harness/render/render.c:25`|`palette.h` → `spec/assets/palette.h`|PUBLIC INTERFACE|
|`harness/render/render.c:26`|`color_canon.h` → `spec/assets/color_canon.h`|PUBLIC INTERFACE|
|`harness/render/render.h:41`|`grafport.h` → `spec/grafport.h`|PUBLIC INTERFACE|
|`harness/render/render.h:42`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`harness/render/render.h:43`|`heap.h` → `os/flair/heap.h`|INTERNAL LEAK|
|`harness/ssim.c:22`|`ssim_params.h` → `spec/ssim_params.h`|PUBLIC INTERFACE|
|`tools/asset_check.c:35`|`../spec/assets/chicago12.h` → `spec/assets/chicago12.h`|PUBLIC INTERFACE|
|`tools/asset_lz.c:25`|`desk_icons.h` → `spec/assets/desk_icons.h`|PUBLIC INTERFACE|
|`tools/asset_lz.c:26`|`finder_icons.h` → `spec/assets/finder_icons.h`|PUBLIC INTERFACE|
|`tools/asset_lz.c:27`|`alert_icons.h` → `spec/assets/alert_icons.h`|PUBLIC INTERFACE|
|`tools/asset_lz.c:28`|`finder_small_icons.h` → `spec/assets/finder_small_icons.h`|PUBLIC INTERFACE|
|`tools/asset_lz.c:29`|`boot_logo.h` → `spec/assets/boot_logo.h`|PUBLIC INTERFACE|
|`tools/chicago_pack.c:11`|`geneva12.h` → `spec/assets/geneva12.h`|PUBLIC INTERFACE|
|`tools/chicago_pack.c:18`|`chicago12.h` → `spec/assets/chicago12.h`|PUBLIC INTERFACE|
|`tools/egg_jtc_pack.c:7`|`egg_jtc_model.h` → `os/flair/egg_jtc_model.h`|INTERNAL LEAK|
|`tools/flairovl.c:26`|`overlay_format.h` → `spec/overlay_format.h`|PUBLIC INTERFACE|
|`tools/mint_finder_small_icons.c:9`|`color_canon.h` → `spec/assets/color_canon.h`|PUBLIC INTERFACE|
|`tools/mzlink.c:85`|`../spec/samir/memory_contract.h` → `spec/samir/memory_contract.h`|PUBLIC INTERFACE|
|`tools/samir_memory_ld.c:4`|`../spec/memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`tools/samir_memory_ld.c:5`|`../spec/samir/memory_contract.h` → `spec/samir/memory_contract.h`|PUBLIC INTERFACE|
|`tools/samir_ovl_pack.c:49`|`samir/ovl_format.h` → `spec/samir/ovl_format.h`|PUBLIC INTERFACE|

### Generated inputs

|Header|Generator inputs / recipe origin|
|---|---|
|`spec/assets/palette.h`|`spec/assets/palette.json build/palette_extract`; #  recipe to execute (from 'Makefile', line 12063):|

### Checked-in generated header provenance

These generation facts supplement section 2; checked-in output is an input during the current build, not necessarily regenerated by make.

|Header|Generator|Generator inputs|
|---|---|---|
|`os/flair/asset_lz_data.h:1`|`tools/asset_lz.c:1`|spec/assets/{desk_icons,finder_icons,alert_icons,finder_small_icons,boot_logo}.h|
|`spec/assets/boot_logo.h:1`|`tools/boot_logo_gen.c:1`|compiled-in geometry; generator:4–22 names pubs/common/initech-logo.tex and docs/pubs/DESIGN-REFERENCE.md|
|`spec/assets/chicago12.h:1`|`tools/font_art2h.c:1`|spec/assets/chicago12.art|
|`spec/assets/geneva12.h:1`|`tools/geneva12_art2h.c:1`|spec/assets/geneva12.art|
|`spec/assets/finder_small_icons.h:1`|`tools/mint_finder_small_icons.c:1`|system7 captures s8_finder_list_view{,_bysize}.png + spec/assets/color_canon.h|
|`spec/assets/color_canon.h:1`|`tools/color_canon_extract.c:1`|spec/assets/color_canon.json|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-factory.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/boot_system build/qemu_harness build/bochs_harness build/mzlink
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
harness/diff/fat_diff/boot_system.mk:9: update target 'build/boot_system' due to: harness/diff/fat_diff/boot_system.c
cc -std=c11 -O2 -Wall -Wextra -Werror harness/diff/fat_diff/boot_system.c -o build/boot_system
Makefile:12114: update target 'build/qemu_harness' due to: harness/emu/qemu_main.c harness/emu/qemu.c harness/emu/qemu.h
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/qemu_harness harness/emu/qemu_main.c harness/emu/qemu.c
Makefile:12189: update target 'build/bochs_harness' due to: harness/emu/bochs_main.c harness/emu/bochs.c harness/emu/bochs.h harness/emu/bochs_trace.h
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/bochs_harness harness/emu/bochs_main.c harness/emu/bochs.c
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c

# stderr

```

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 0; UNIT 0; META 26; SIZE/LAYOUT 1.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-factory.md](gates-factory.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/check_adr_citations.c — META

`harness/check_adr_citations.c:1`; test-adr-citations (initech-lk29, initech-0ni0).. tests the factory/checker/process contract itself.
Build target(s): `build/check_adr_citations`.
Run/assert gate(s): `test-adr-citations`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/check_adr_citations`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/check_adr_citations harness/check_adr_citations.c
```

### harness/check_fixture_volumes.c — META

`harness/check_fixture_volumes.c:1`; Read GNU make's database, then ask make to EXPAND (never execute) recipes.. tests the factory/checker/process contract itself.
Build target(s): `build/check_fixture_volumes`.
Run/assert gate(s): `test-fixture-volumes`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/adr_citations.mk`, `harness/bochs_legs.mk`, `harness/cert_runner.mk`, `harness/conkeys/conkeys.mk`, `harness/conkeys/f3template.mk`, `harness/diff/dbf_diff/console_review.mk`, `harness/diff/dbf_diff/do_file.mk`, `harness/diff/dbf_diff/samir_ovl.mk`, `harness/diff/dbf_diff/samir_size.mk`, `harness/diff/dbf_diff/samir_w2_history.mk`, `harness/diff/dbf_diff/screen_mint.mk`, `harness/diff/dbf_diff/w2_set_fixed.mk`, `harness/diff/dbf_diff/w2_set_unique.mk`, `harness/diff/dbf_diff/w4.mk`, `harness/diff/fat_diff/batch2.mk`, `harness/diff/fat_diff/block_device.mk`, `harness/diff/fat_diff/boot_system.mk`, `harness/diff/fat_diff/chkdsk.mk`, `harness/diff/fat_diff/command_file.mk`, `harness/diff/fat_diff/copy_stamp.mk`, `harness/diff/fat_diff/dos_folder.mk`, `harness/diff/fat_diff/dos_parity.mk`, `harness/diff/fat_diff/dos_volumes.mk`, `harness/diff/fat_diff/doscmd_round2.mk`, `harness/diff/fat_diff/doslook.mk`, `harness/diff/fat_diff/dosmsg.mk`, `harness/diff/fat_diff/dosutil.mk`, `harness/diff/fat_diff/drive_operands.mk`, `harness/diff/fat_diff/fat_spc.mk`, `harness/diff/fat_diff/fdisk.mk`, `harness/diff/fat_diff/formatsys.mk`, `harness/diff/fat_diff/int21_dpb.mk`, `harness/diff/fat_diff/kernel_hygiene.mk`, `harness/diff/fat_diff/partition.mk`, `harness/diff/fat_diff/shell_files.mk`, `harness/diff/fat_diff/xcopy_tree.mk`, `harness/diff/wk1_diff/i123_numeric.mk`, `harness/diff/wp5_diff/p3.mk`, `harness/diff/wp5_diff/p3_listfiles.mk`, `harness/diff/wp5_diff/p3_part2.mk`, `harness/edlin/edlin.mk`, `harness/emu/ansi.mk`, `harness/emu/calcda_power.mk`, `harness/emu/copybreak.mk`, `harness/emu/cpanels_bars.mk`, `harness/emu/dos_control.mk`, `harness/emu/exec_high.mk`, `harness/emu/execguard.mk`, `harness/emu/exit_dos.mk`, `harness/emu/fcb_tps.mk`, `harness/emu/flair_launchkey.mk`, `harness/emu/fullsave.mk`, `harness/emu/notepad.mk`, `harness/emu/panic_reasons.mk`, `harness/emu/panic_screen.mk`, `harness/emu/power_cancel.mk`, `harness/emu/qemu_log.mk`, `harness/emu/tps_backup.mk`, `harness/emu/tsr_exec.mk`, `harness/emu/vesa8.mk`, `harness/exec_overlay.mk`, `harness/film_scene.mk`, `harness/finder_alerts.mk`, `harness/finder_trash_edges.mk`, `harness/fixture_volumes.mk`, `harness/frameapps.mk`, `harness/gate_map.mk`, `harness/gate_pairs.mk`, `harness/lane_cert.mk`, `harness/merge_samir.mk`, `harness/proptest/egg_jtc.mk`, `harness/proptest/finder_overlay.mk`, `harness/proptest/geneva12.mk`, `harness/proptest/kernel_size.mk`, `harness/proptest/kernel_size_r4.mk`, `harness/proptest/kernel_size_r5.mk`, `harness/proptest/kernel_size_r7.mk`, `harness/proptest/overlay.mk`, `harness/proptest/power_overlay.mk`, `harness/registries.mk`, `harness/revert_merge.mk`, `harness/samir_boot_bochs.mk`, `harness/sentinel.mk`, `harness/toolbox_hardening.mk`, `harness/verify_fixture_volume.sh`, `spec/dos_folder_traces.mk`, `spec/flair_app_launch_traces.mk`, `spec/flair_appswitch_trace.mk`, `spec/flair_calc_traces.mk`, `spec/flair_calcda_traces.mk`, `spec/flair_cpanels_traces.mk`, `spec/flair_ctenant_traces.mk`, `spec/flair_desktop_icons_traces.mk`, `spec/flair_disk_windows_traces.mk`, `spec/flair_egg_jtc_traces.mk`, `spec/flair_exit_dos_traces.mk`, `spec/flair_fg_close_traces.mk`, `spec/flair_file_ops_traces.mk`, `spec/flair_file_traces.mk`, `spec/flair_film_scene_traces.mk`, `spec/flair_finder_alerts_trace.mk`, `spec/flair_finder_cmds_traces.mk`, `spec/flair_finder_follow_traces.mk`, `spec/flair_finder_icons_traces.mk`, `spec/flair_finder_open_traces.mk`, `spec/flair_fonts_traces.mk`, `spec/flair_fullsave_traces.mk`, `spec/flair_grow_box_trace.mk`, `spec/flair_i123_copy_traces.mk`, `spec/flair_i123_entry_traces.mk`, `spec/flair_i123_file_traces.mk`, `spec/flair_i123_hit_traces.mk`, `spec/flair_i123_keys_traces.mk`, `spec/flair_i123_menu_traces.mk`, `spec/flair_i123_quit_traces.mk`, `spec/flair_i123_resize_traces.mk`, `spec/flair_i123_traces.mk`, `spec/flair_i123_wide_traces.mk`, `spec/flair_input_traces.mk`, `spec/flair_itext_traces.mk`, `spec/flair_iword_traces.mk`, `spec/flair_launchkey_traces.mk`, `spec/flair_list_view_traces.mk`, `spec/flair_menu_look_traces.mk`, `spec/flair_menubar_clock_traces.mk`, `spec/flair_notepad_traces.mk`, `spec/flair_power_traces.mk`, `spec/flair_protection_traces.mk`, `spec/flair_scroll_look_traces.mk`, `spec/flair_scroll_traces.mk`, `spec/flair_solid_traces.mk`, `spec/flair_toolbox_hardening_trace.mk`, `spec/flair_trash_edges_traces.mk`, `spec/flair_trash_traces.mk`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/check_fixture_volumes`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/check_fixture_volumes harness/check_fixture_volumes.c
```

### harness/check_registries.c — META

`harness/check_registries.c:1`; Union merges preserve text, not registry invariants. Validate the actual. tests the factory/checker/process contract itself.
Build target(s): `build/check_registries`.
Run/assert gate(s): `test-samir-registries`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/dbf_diff/xbase_prog_diff/fns-mint/`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/check_registries`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/check_registries harness/check_registries.c
```

### harness/diff/fat_diff/boot_system.c — META

`harness/diff/fat_diff/boot_system.c:1`; Independent factory layout oracle and fault injector, initech-kod.. tests the factory/checker/process contract itself.
Build target(s): `build/boot_system`.
Run/assert gate(s): `test-boot-system`, `test-boot-system-emu`, `test-boot-system-emu-mutant`, `test-boot-system-negative`, `test-boot-system-repro`, `test-boot-vesa-images`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/boot_system.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/boot_system`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror harness/diff/fat_diff/boot_system.c -o build/boot_system
```

### harness/diff/fat_diff/test_kernel_hygiene.c — SIZE/LAYOUT

`harness/diff/fat_diff/test_kernel_hygiene.c:1`; MILTON FAT regressions, on images independently exercised by mtools.. memory/layout, byte/size policy or legacy map constraints; preserve functional safety invariants separately.
Build target(s): `build/test_kernel_hygiene`, `build/test_kernel_hygiene_grow`, `build/test_kernel_hygiene_hidden`, `build/test_kernel_hygiene_rename`.
Run/assert gate(s): `test-fat-hygiene-grow`, `test-fat-hygiene-grow-mutant`, `test-fat-hygiene-hidden`, `test-fat-hygiene-hidden-mutant`, `test-fat-hygiene-rename`, `test-fat-hygiene-rename-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/kernel_hygiene.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_kernel_hygiene`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/test_kernel_hygiene harness/diff/fat_diff/test_kernel_hygiene.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/desktop_db.c
```

### harness/emu/test_bochs_completion.c — META

`harness/emu/test_bochs_completion.c:1`; Factory oracle, initech-cqej. Ref: PRD Sec 8, qemu.h quit_after_count,. tests the factory/checker/process contract itself.
Build target(s): `build/test_bochs_completion`, `build/test_bochs_completion_mut`, `build/test_bochs_completion_partial`, `build/test_bochs_completion_substring`.
Run/assert gate(s): `test-bochs-completion`, `test-bochs-completion-mutant`, `test-bochs-completion-partial-mutant`, `test-bochs-completion-substring-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/flair_power_traces.mk`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/bochs.c`, `harness/emu/bochs.h`, `harness/emu/bochs_trace.h`.
Canonical `make -n` compile/link (`build/test_bochs_completion`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu harness/emu/test_bochs_completion.c -o build/test_bochs_completion
```

### harness/emu/test_bochs_legs.c — META

`harness/emu/test_bochs_legs.c:1`; Factory inventory oracle, written before the cqej second-half legs and. tests the factory/checker/process contract itself.
Build target(s): `build/test_bochs_legs`.
Run/assert gate(s): `test-bochs-legs`, `test-bochs-legs-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/bochs_leg_check.sh`, `harness/emu/samir_boot_check.sh`, `harness/emu/test_bochs_legs.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_bochs_legs`):
```sh
cc -std=c11 -Wall -Wextra -Werror harness/emu/test_bochs_legs.c -o build/test_bochs_legs
```

### harness/emu/test_bochs_legs.sh — META

`harness/emu/test_bochs_legs.sh:1`; Factory make-wiring differential. Mutate actual factory recipes in build,. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-bochs-legs`, `test-bochs-legs-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/bochs_legs.mk`, `harness/emu/test_bochs_legs.sh`, `harness/samir_boot_bochs.mk`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
@sh harness/emu/test_bochs_legs.sh $(BUILD) green
@sh harness/emu/test_bochs_legs.sh $(BUILD) mutants
```

### harness/emu/test_bochs_trace.c — META

`harness/emu/test_bochs_trace.c:1`; Factory vocabulary, independent expected RFB keysyms and pointer steps. */. tests the factory/checker/process contract itself.
Build target(s): `build/test_bochs_trace`, `build/test_bochs_trace_capture_mut`, `build/test_bochs_trace_mut`, `build/test_bochs_trace_origin_mut`, `build/test_bochs_trace_sync_mut`.
Run/assert gate(s): `test-bochs-trace`, `test-bochs-trace-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/bochs_trace.h`.
Canonical `make -n` compile/link (`build/test_bochs_trace`):
```sh
cc -std=c11 -Wall -Wextra -Werror -DCOPYBREAK_OPEN='"m-100:-100,m-100:-100,m-100:-30,l1,w200,m40:88,l0,w600"' -DCOPYBREAK_SPEC='"m-100:-100,m-100:-100,m-100:-30,l1,w200,m40:88,l0,w600,kctrl-c,w2200,kctrl-v,w400,m100:-8,m100:0,m60:0,l1,m60:40,l0,w300,m-100:0,m-50:0,l1,l0,w400,m-100:-100,m-100:-20,m-10:0,l1,w200,m40:88,l0,w600"' -DCOPYBREAK_TICK_BUDGET=1500 -Iharness/emu harness/emu/test_bochs_trace.c -o build/test_bochs_trace
```

### harness/emu/test_qemu_key_ack.c — META

`harness/emu/test_qemu_key_ack.c:1`; initech-ptzy; PRD Sec 6.3/8; spec/event_model.h Sec 5; Rule 6.. tests the factory/checker/process contract itself.
Build target(s): `build/test_qemu_key_ack`, `build/test_qemu_key_ack_mut`.
Run/assert gate(s): `test-harness-key-ack`, `test-harness-key-ack-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/event_model.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/qemu.c`, `harness/emu/qemu.h`.
Canonical `make -n` compile/link (`build/test_qemu_key_ack`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/test_qemu_key_ack harness/emu/test_qemu_key_ack.c
```

### harness/emu/test_qemu_key_pace.c — META

`harness/emu/test_qemu_key_pace.c:1`; initech-1x6x2.1; PRD Sec 8; Rule 6. key_pace_wait against real processes:. tests the factory/checker/process contract itself.
Build target(s): `build/test_qemu_key_pace`, `build/test_qemu_key_pace_mut`.
Run/assert gate(s): `test-harness-key-pace`, `test-harness-key-pace-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/qemu.c`, `harness/emu/qemu.h`.
Canonical `make -n` compile/link (`build/test_qemu_key_pace`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/test_qemu_key_pace harness/emu/test_qemu_key_pace.c
```

### harness/emu/test_qemu_paths.c — META

`harness/emu/test_qemu_paths.c:1`; initech-nstg; PRD Sec 8 + CLAUDE.md Rule 2. Host-only preflight test.. tests the factory/checker/process contract itself.
Build target(s): `build/test_qemu_paths`, `build/test_qemu_paths_mut`.
Run/assert gate(s): `test-qemu-paths`, `test-qemu-paths-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/qemu.c`, `harness/emu/qemu.h`.
Canonical `make -n` compile/link (`build/test_qemu_paths`):
```sh
cc -std=c11 -Wall -Wextra -Werror harness/emu/test_qemu_paths.c -o build/test_qemu_paths
```

### harness/emu/test_qemu_progress.c — META

`harness/emu/test_qemu_progress.c:1`; initech-7r2; PRD Sec 8 serial milestones; CLAUDE.md Rules 1, 2, 6.. tests the factory/checker/process contract itself.
Build target(s): `build/test_qemu_progress`, `build/test_qemu_progress_mut`.
Run/assert gate(s): `test-harness-progress`, `test-harness-progress-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/qemu.c`, `harness/emu/qemu.h`.
Canonical `make -n` compile/link (`build/test_qemu_progress`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/test_qemu_progress harness/emu/test_qemu_progress.c
```

### harness/emu/test_qemu_screen_helper.c — META

`harness/emu/test_qemu_screen_helper.c:1`; W4.0 review item 4: a stalled optional helper cannot outlive the harness.. tests the factory/checker/process contract itself.
Build target(s): `build/test_qemu_screen_helper`.
Run/assert gate(s): `test-screen-mint-mutants`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/dbf_diff/screen_mint_mutants.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/emu/qemu.c`, `harness/emu/qemu.h`.
Canonical `make -n` compile/link (`build/test_qemu_screen_helper`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/test_qemu_screen_helper harness/emu/test_qemu_screen_helper.c
```

### harness/fixtures/dos33_overlay/check.c — META

`harness/fixtures/dos33_overlay/check.c:1`; Independent DOS 3.30 captures, not values computed by the artifact.. tests the factory/checker/process contract itself.
Build target(s): `build/overlay_dos33_check`, `build/overlay_dos33_check_mut`.
Run/assert gate(s): `test-overlay-dos33-reference`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/fixtures/dos33_overlay/check.c`, `harness/fixtures/dos33_overlay/factor-different.bin`, `harness/fixtures/dos33_overlay/factor-equal.bin`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/overlay_dos33_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/overlay_dos33_check harness/fixtures/dos33_overlay/check.c
```

### harness/proptest/check_win95isms.c — META

`harness/proptest/check_win95isms.c:1`; the era-boundary guardrail gate (check-win95isms).. tests the factory/checker/process contract itself.
Build target(s): `build/check_win95isms`, `build/check_win95isms_mutant`.
Run/assert gate(s): no direct run gate found; support program or unregistered.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/check_win95isms.c`, `spec/assets/color_canon.h`, `spec/assets/color_canon.json`, `spec/flair_skins.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/check_win95isms`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -o build/check_win95isms harness/proptest/check_win95isms.c
```

### harness/proptest/test_ssim.c — META

`harness/proptest/test_ssim.c:1`; Independent analytic oracle, written before harness/ssim.c (initech-w06).. tests the factory/checker/process contract itself.
Build target(s): `build/test_ssim`.
Run/assert gate(s): `test-ssim`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/ssim.c`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_ssim`):
```sh
cc -std=c11 -Wall -Wextra -Werror -O2 -o build/test_ssim harness/proptest/test_ssim.c -lpng -lm
```

### harness/pubs/check_log.sh — META

`harness/pubs/check_log.sh:1`; Shared by the build gate and the OVERFULL mutant (GC-07).. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-pubs-build`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/console_text.c`, `harness/pubs/check_log.sh`, `harness/pubs/pubs_gate.c`, `harness/pubs/slip_mutants.sh`, `harness/pubs/slip_places.sh`, `harness/pubs/srclockstep.h`, `harness/pubs/tps_inventory.c`, `spec/pubs_inconsistencies.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
@printf '>>> test-pubs-build: every manual typesets clean (initech-ipu5.5; D-11 LuaLaTeX)\n'
@for t in $(LUALATEX) $(LATEXMK); do command -v $$t >/dev/null 2>&1 || { \
	printf '!!! test-pubs-build FAIL: %s not found (TeX Live is required to typeset the manuals). A skipped oracle is worse than a red one (Law 2). Set SKIP_PUBS=1 to explicitly (and loudly) opt out.\n' "$$t"; exit 1; }; done
@bad=$$(LC_ALL=C grep -l -P '[^\x00-\x7F]' $(PUBS_SOURCES) harness/console_text.c tools/pubs_msgtable.c harness/pubs/tps_inventory.c \
	harness/pubs/pubs_gate.c harness/pubs/srclockstep.h harness/pubs/slip_places.sh harness/pubs/slip_mutants.sh spec/pubs_inconsistencies.json || true); \
if [ -n "$$bad" ]; then printf '!!! test-pubs-build FAIL: non-ASCII bytes in (Rule 12): %s\n' "$$bad"; exit 1; fi
$(call pubs-build-into,$(PUBS_BUILD)/gate)
@set -e; for log in $(PUBS_BUILD)/gate/*/*.log; do \
	doc=$${log%.log}; name=$${doc#$(PUBS_BUILD)/gate/}; \
	over=$$(grep -c '^Overfull \\[hv]box' $$log || true); \
	sh harness/pubs/check_log.sh "$$log" "$(PUBS_OVERFULL_MAX)"; \
	if grep -E -q 'There were undefined references|Reference .* undefined|Label\(s\) may have changed' $$log; then \
		printf '!!! test-pubs-build FAIL: %s: undefined or unsettled references\n' "$$name"; exit 1; fi; \
	if grep -q 'Missing character' $$log; then grep 'Missing character' $$log | head -5; \
		printf '!!! test-pubs-build FAIL: %s: glyphs missing from the fonts\n' "$$name"; exit 1; fi; \
	if grep -q 'No index file' $$log; then \
		printf '!!! test-pubs-build FAIL: %s: the index was not generated\n' "$$name"; exit 1; fi; \
	pages=$$(pdfinfo $$doc.pdf | sed -n 's/^Pages: *//p'); \
	drafts=$$(sed -n 's/^initech-manual: \([0-9]*\) DRAFT markers$$/\1/p' $$log); \
	case $$name in */main) \
		[ $$((pages % 4)) -eq 0 ] || { printf '!!! test-pubs-build FAIL: %s: %s pages is not a multiple of 4 (printer signatures)\n' "$$name" "$$pages"; exit 1; } ;; esac; \
	printf '>>> test-pubs-build: %s.pdf %s pages, %s overfull, %s DRAFT markers\n' "$$name" "$$pages" "$$over" "$${drafts:-0}"; \
done
@printf '>>> test-pubs-build: green\n'
```

### harness/test_cert_summaries.sh — META

`harness/test_cert_summaries.sh:1`; PRD 8/9; initech-0spyf/fzsvo. Real local cert/check integration.. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-lane-cert-summaries`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/test_cert_summaries.sh`, `spec/assets/`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
@sh harness/test_cert_summaries.sh $(abspath tools/lane-cert.sh) $(abspath tools/cert-check.sh) $(abspath $(BUILD))/lane-cert/summaries
```

### harness/test_cert_summaries_mutant.sh — META

`harness/test_cert_summaries_mutant.sh:1`; mkdir -p "$out/tools". tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-lane-cert-summaries-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/test_cert_summaries.sh`, `harness/test_cert_summaries_mutant.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
@sh harness/test_cert_summaries_mutant.sh $(abspath $(BUILD))/lane-cert/summaries-mutant
```

### harness/test_gate_honesty.sh — META

`harness/test_gate_honesty.sh:1`; mkdir -p "$out/empty". tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-gate-honesty`, `test-gate-honesty-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/adr_citations.mk`, `harness/bochs_legs.mk`, `harness/cert_runner.mk`, `harness/conkeys/conkeys.mk`, `harness/conkeys/f3template.mk`, `harness/diff/dbf_diff/console_review.mk`, `harness/diff/dbf_diff/do_file.mk`, `harness/diff/dbf_diff/samir_ovl.mk`, `harness/diff/dbf_diff/samir_size.mk`, `harness/diff/dbf_diff/samir_w2_history.mk`, `harness/diff/dbf_diff/screen_mint.mk`, `harness/diff/dbf_diff/w2_set_fixed.mk`, `harness/diff/dbf_diff/w2_set_unique.mk`, `harness/diff/dbf_diff/w4.mk`, `harness/diff/fat_diff/batch2.mk`, `harness/diff/fat_diff/block_device.mk`, `harness/diff/fat_diff/boot_system.mk`, `harness/diff/fat_diff/chkdsk.mk`, `harness/diff/fat_diff/command_file.mk`, `harness/diff/fat_diff/copy_stamp.mk`, `harness/diff/fat_diff/dos_folder.mk`, `harness/diff/fat_diff/dos_parity.mk`, `harness/diff/fat_diff/dos_volumes.mk`, `harness/diff/fat_diff/doscmd_round2.mk`, `harness/diff/fat_diff/doslook.mk`, `harness/diff/fat_diff/dosmsg.mk`, `harness/diff/fat_diff/dosutil.mk`, `harness/diff/fat_diff/drive_operands.mk`, `harness/diff/fat_diff/fat_spc.mk`, `harness/diff/fat_diff/fdisk.mk`, `harness/diff/fat_diff/formatsys.mk`, `harness/diff/fat_diff/int21_dpb.mk`, `harness/diff/fat_diff/kernel_hygiene.mk`, `harness/diff/fat_diff/partition.mk`, `harness/diff/fat_diff/shell_files.mk`, `harness/diff/fat_diff/xcopy_tree.mk`, `harness/diff/wk1_diff/i123_numeric.mk`, `harness/diff/wp5_diff/p3.mk`, `harness/diff/wp5_diff/p3_listfiles.mk`, `harness/diff/wp5_diff/p3_part2.mk`, `harness/edlin/edlin.mk`, `harness/emu/ansi.mk`, `harness/emu/calcda_power.mk`, `harness/emu/copybreak.mk`, `harness/emu/cpanels_bars.mk`, `harness/emu/dos_control.mk`, `harness/emu/exec_high.mk`, `harness/emu/execguard.mk`, `harness/emu/exit_dos.mk`, `harness/emu/fcb_tps.mk`, `harness/emu/flair_launchkey.mk`, `harness/emu/fullsave.mk`, `harness/emu/notepad.mk`, `harness/emu/panic_reasons.mk`, `harness/emu/panic_screen.mk`, `harness/emu/power_cancel.mk`, `harness/emu/qemu_log.mk`, `harness/emu/tps_backup.mk`, `harness/emu/tsr_exec.mk`, `harness/emu/vesa8.mk`, `harness/exec_overlay.mk`, `harness/film_scene.mk`, `harness/finder_alerts.mk`, `harness/finder_trash_edges.mk`, `harness/fixture_volumes.mk`, `harness/frameapps.mk`, `harness/gate_map.mk`, `harness/gate_pairs.mk`, `harness/lane_cert.mk`, `harness/merge_samir.mk`, `harness/proptest/egg_jtc.mk`, `harness/proptest/finder_overlay.mk`, `harness/proptest/geneva12.mk`, `harness/proptest/kernel_size.mk`, `harness/proptest/kernel_size_r4.mk`, `harness/proptest/kernel_size_r5.mk`, `harness/proptest/kernel_size_r7.mk`, `harness/proptest/overlay.mk`, `harness/proptest/power_overlay.mk`, `harness/registries.mk`, `harness/revert_merge.mk`, `harness/samir_boot_bochs.mk`, `harness/sentinel.mk`, `harness/test_gate_honesty.sh`, `harness/toolbox_hardening.mk`, `spec/dos_folder_traces.mk`, `spec/flair_app_launch_traces.mk`, `spec/flair_appswitch_trace.mk`, `spec/flair_calc_traces.mk`, `spec/flair_calcda_traces.mk`, `spec/flair_cpanels_traces.mk`, `spec/flair_ctenant_traces.mk`, `spec/flair_desktop_icons_traces.mk`, `spec/flair_disk_windows_traces.mk`, `spec/flair_egg_jtc_traces.mk`, `spec/flair_exit_dos_traces.mk`, `spec/flair_fg_close_traces.mk`, `spec/flair_file_ops_traces.mk`, `spec/flair_file_traces.mk`, `spec/flair_film_scene_traces.mk`, `spec/flair_finder_alerts_trace.mk`, `spec/flair_finder_cmds_traces.mk`, `spec/flair_finder_follow_traces.mk`, `spec/flair_finder_icons_traces.mk`, `spec/flair_finder_open_traces.mk`, `spec/flair_fonts_traces.mk`, `spec/flair_fullsave_traces.mk`, `spec/flair_grow_box_trace.mk`, `spec/flair_i123_copy_traces.mk`, `spec/flair_i123_entry_traces.mk`, `spec/flair_i123_file_traces.mk`, `spec/flair_i123_hit_traces.mk`, `spec/flair_i123_keys_traces.mk`, `spec/flair_i123_menu_traces.mk`, `spec/flair_i123_quit_traces.mk`, `spec/flair_i123_resize_traces.mk`, `spec/flair_i123_traces.mk`, `spec/flair_i123_wide_traces.mk`, `spec/flair_input_traces.mk`, `spec/flair_itext_traces.mk`, `spec/flair_iword_traces.mk`, `spec/flair_launchkey_traces.mk`, `spec/flair_list_view_traces.mk`, `spec/flair_menu_look_traces.mk`, `spec/flair_menubar_clock_traces.mk`, `spec/flair_notepad_traces.mk`, `spec/flair_power_traces.mk`, `spec/flair_protection_traces.mk`, `spec/flair_scroll_look_traces.mk`, `spec/flair_scroll_traces.mk`, `spec/flair_solid_traces.mk`, `spec/flair_toolbox_hardening_trace.mk`, `spec/flair_trash_edges_traces.mk`, `spec/flair_trash_traces.mk`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults), `WIN31_DECOMP` → `../win31-decomp` (Makefile:39–43 defaults), `DBASE3_DECOMP` → `../dbase3-decomp` (Makefile:39–43 defaults), `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults), `WORDPERFECT51_DECOMP` → `../wordperfect51-decomp` (Makefile:39–43 defaults).
Shell program; no compilation. Exact invocation:
```make
@sh harness/test_gate_honesty.sh "$(MAKE)" "$(abspath $(firstword $(MAKEFILE_LIST)))" "$(abspath $(BUILD)/gate-honesty)"
@sh harness/test_gate_honesty.sh "$(MAKE)" "$(abspath $(firstword $(MAKEFILE_LIST)))" "$(abspath $(BUILD)/gate-honesty-mutant)" "CFLAGS=$(CFLAGS) -DGATE_HONESTY_MUTATE_SKIP" > $(BUILD)/gate-honesty-mutant.log 2>&1; rc=$$?; \
[ "$$rc" -ne 0 ] && grep -qF 'test-gate-honesty FAIL: test-chicago-metrics accepted missing SYSTEM7_DECOMP' $(BUILD)/gate-honesty-mutant.log \
	|| { cat $(BUILD)/gate-honesty-mutant.log; printf '!!! test-gate-honesty-mutant FAIL: old skip-to-green policy was not caught for the named reason\n'; exit 1; }; \
printf '>>> test-gate-honesty-mutant: green (old Chicago LOUD-SKIP-to-green correctly RED on missing SYSTEM7_DECOMP)\n'
```

### harness/test_revert_merge.sh — META

`harness/test_revert_merge.sh:1`; oracle for tools/revert-merge.sh (initech-ror2g, F10. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-revert-merge`, `test-revert-merge-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/test_revert_merge.sh`, `harness/test_revert_merge_mutant.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
@sh $(REVERT_MERGE_TEST) $(REVERT_MERGE_TOOL) $(BUILD)/revert-merge
@sh $(REVERT_MERGE_MUTANT) $(REVERT_MERGE_TOOL) $(REVERT_MERGE_TEST) $(BUILD)/revert-merge-mut
```

### harness/test_revert_merge_mutant.sh — META

`harness/test_revert_merge_mutant.sh:1`; mutation proof for test_revert_merge.sh. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-revert-merge-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/test_revert_merge.sh`, `harness/test_revert_merge_mutant.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
@sh $(REVERT_MERGE_MUTANT) $(REVERT_MERGE_TOOL) $(REVERT_MERGE_TEST) $(BUILD)/revert-merge-mut
```

### tools/ppm_bochs_check.c — META

`tools/ppm_bochs_check.c:1`; grade a Bochs capture against a REAL QEMU QMP capture of. tests the factory/checker/process contract itself.
Build target(s): `build/ppm_bochs_check`.
Run/assert gate(s): `test-flair-appswitch-bochs`, `test-flair-desktop-8bpp-bochs-pixel`, `test-flair-desktop-bochs`, `test-flair-desktop-icons-bochs-pixel`, `test-samir-boot-bochs`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/bochs.c`, `harness/emu/bochs_leg_check.sh`, `harness/emu/samir_boot_check.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/ppm_bochs_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror tools/ppm_bochs_check.c -o build/ppm_bochs_check
```

### tools/ppm_seafoam_check.c — META

`tools/ppm_seafoam_check.c:1`; RETIRED (ADR-0010 CD-6 / CD-7; epic initech-qipc).. tests the factory/checker/process contract itself.
Build target(s): `build/ppm_seafoam_check`.
Run/assert gate(s): no direct run gate found; support program or unregistered.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest//`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/ppm_seafoam_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/ppm_seafoam_check tools/ppm_seafoam_check.c
```

### tools/test-emu-trace.sh — META

`tools/test-emu-trace.sh:1`; Reject missing/failed/partial observations before any absence oracle.. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-samir-trace`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/require_trace.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
+@MAKE="$(MAKE)" TRACE_ASSUME_OLD="$(TRACE_ASSUME_OLD)" sh tools/test-emu-trace.sh
```

### tools/test-merge-train.sh — META

`tools/test-merge-train.sh:1`; "$(dirname "$0")" && pwd)/merge-train.sh. tests the factory/checker/process contract itself.
Build target(s): none found; source is orphan/unregistered in parsed rules.
Run/assert gate(s): `test-merge-train`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Shell program; no compilation. Exact invocation:
```make
+@sh tools/test-merge-train.sh
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-factory.md](gates-factory.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**13 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 18 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-harness

#  recipe to execute (from 'Makefile', line 19145):

Boot/data images named by direct prerequisites or expanded recipe: passed by recursive helper; see full recipe in gate inventory.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	|| { printf "!!! test-harness FAIL: serial_hello did not satisfy the oracle (marker missing, fault, or timeout)\n"; exit 1; }
	echo "$$rep" | grep -q 'triple_fault=1' \
		|| { printf "!!! test-harness FAIL: triple_fault was NOT detected on the bad fixture\n"; exit 1; }; \
	echo "$$rep" | grep -q 'marker_found=0' \
		|| { printf "!!! test-harness FAIL: HARNESS-OK marker unexpectedly present on the bad fixture\n"; exit 1; }; \
	echo "$$rep" | grep -q 'timed_out=0' \
		|| { printf "!!! test-harness FAIL: bad fixture timed out instead of exiting on the fault\n"; exit 1; }
```

### test-bochs-completion

#  recipe to execute (from 'harness/bochs_legs.mk', line 266):

Boot/data images named by direct prerequisites or expanded recipe: passed by recursive helper; see full recipe in gate inventory.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

### test-harness-key-ack

#  recipe to execute (from 'Makefile', line 12149):

Boot/data images named by direct prerequisites or expanded recipe: passed by recursive helper; see full recipe in gate inventory.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

The remaining legacy emulator entries cover emulator harness completion/pacing/locking, snapshot/reboot/report honesty and process negative controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`harness/conkeys/extended_check.sh:1`|0|0|none|
|`harness/conkeys/f3template.sh:1`|0|0|none|
|`harness/console_text.c:1`|0|0|none|
|`harness/def_dependency_check.sh:1`|0|0|none|
|`harness/diff/dbf_diff/console_review_emu.sh:1`|0|0|none|
|`harness/diff/dbf_diff/corpus_pristine.sh:1`|0|0|none|
|`harness/diff/dbf_diff/datatype_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/date_contract_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/dbf_ref.py:1`|0|0|none|
|`harness/diff/dbf_diff/dml_extra_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/dml_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/dml_probe.c:1`|0|0|none|
|`harness/diff/dbf_diff/do_file_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/do_file_test.sh:1`|0|0|none|
|`harness/diff/dbf_diff/fns_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/mint_capture.sh:1`|0|0|none|
|`harness/diff/dbf_diff/mint_clients_dbf.c:1`|0|0|none|
|`harness/diff/dbf_diff/mint_invoice_dbf.c:1`|0|0|none|
|`harness/diff/dbf_diff/mzlink_close_fault.c:1`|0|0|none|
|`harness/diff/dbf_diff/ndx_ref.py:1`|0|0|none|
|`harness/diff/dbf_diff/numeric_format_ref.py:1`|0|0|none|
|`harness/diff/dbf_diff/prog_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/samir_core_ref.c:1`|0|0|none|
|`harness/diff/dbf_diff/samir_install.h:1`|0|0|none|
|`harness/diff/dbf_diff/scratch_lock.h:1`|0|0|none|
|`harness/diff/dbf_diff/screen_capture.c:1`|0|0|none|
|`harness/diff/dbf_diff/screen_file.h:1`|0|0|none|
|`harness/diff/dbf_diff/screen_forms_emu.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_mint_mutants.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_mint_oracle.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_mint_repro.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_mint_samir.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_mint_test.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_status_pending.sh:1`|0|0|none|
|`harness/diff/dbf_diff/screen_tool.c:1`|0|0|none|
|`harness/diff/dbf_diff/screen_tsr.asm:1`|0|0|none|
|`harness/diff/dbf_diff/test_mzlink_io.sh:1`|0|0|none|
|`harness/diff/dbf_diff/test_scratch_lock.sh:1`|0|0|none|
|`harness/diff/dbf_diff/trace_complete.h:1`|0|0|none|
|`harness/diff/dbf_diff/w2_fixture.c:1`|0|0|none|
|`harness/diff/dbf_diff/w2_host.c:1`|0|0|none|
|`harness/diff/dbf_diff/w2_host.h:1`|0|0|none|
|`harness/diff/dbf_diff/w2_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_accept.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_files_paths_test.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_files_test.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_inventory_test.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_mutate.c:1`|0|0|none|
|`harness/diff/dbf_diff/w4_retained_mint.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_retained_test.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_smoke_mutants.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_trace.sh:1`|0|0|none|
|`harness/diff/dbf_diff/w4_unchanged.sh:1`|0|0|none|
|`harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/mint_run.py:1`|0|0|none|
|`harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/mint_setup.py:1`|0|0|none|
|`harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/scrdump.asm:1`|0|0|none|
|`harness/diff/fat_diff/ata_no_drive.sh:1`|0|0|none|
|`harness/diff/fat_diff/batch2.sh:1`|0|0|none|
|`harness/diff/fat_diff/batch_break.sh:1`|0|0|none|
|`harness/diff/fat_diff/batch_break_host.h:1`|0|0|none|
|`harness/diff/fat_diff/batch_break_mint/exec_child.asm:1`|0|0|none|
|`harness/diff/fat_diff/batch_break_mint/mint.sh:1`|0|0|none|
|`harness/diff/fat_diff/block_device.sh:1`|0|0|none|
|`harness/diff/fat_diff/blockdev_file.c:1`|0|0|none|
|`harness/diff/fat_diff/blockdev_file.h:1`|0|0|none|
|`harness/diff/fat_diff/boot_system.sh:1`|0|0|none|
|`harness/diff/fat_diff/chkdsk.sh:1`|0|0|none|
|`harness/diff/fat_diff/chkdsk_fixture.c:1`|0|0|none|
|`harness/diff/fat_diff/chkdsk_label.sh:1`|0|0|none|
|`harness/diff/fat_diff/chkdsk_label_mint.sh:1`|0|0|none|
|`harness/diff/fat_diff/command_c.sh:1`|0|0|none|
|`harness/diff/fat_diff/command_file.sh:1`|0|0|none|
|`harness/diff/fat_diff/conkeys.sh:1`|0|0|none|
|`harness/diff/fat_diff/copy_stamp.sh:1`|0|0|none|
|`harness/diff/fat_diff/del_b023.sh:1`|0|0|none|
|`harness/diff/fat_diff/del_directory.sh:1`|0|0|none|
|`harness/diff/fat_diff/dir_match.sh:1`|0|0|none|
|`harness/diff/fat_diff/diskcopy_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_break.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_break_program.asm:1`|0|0|none|
|`harness/diff/fat_diff/dos_folder.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_not_ready.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_parity.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_resident_break.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_safety.sh:1`|0|0|none|
|`harness/diff/fat_diff/dos_safety_fixture.c:1`|0|0|none|
|`harness/diff/fat_diff/dos_volumes.sh:1`|0|0|none|
|`harness/diff/fat_diff/dosmsg_fault.c:1`|0|0|none|
|`harness/diff/fat_diff/dosmsg_mutants.sh:1`|0|0|none|
|`harness/diff/fat_diff/dosutil_diff.sh:1`|0|0|none|
|`harness/diff/fat_diff/dosutil_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/dosutil_fixture.sh:1`|0|0|none|
|`harness/diff/fat_diff/drive_operands.sh:1`|0|0|none|
|`harness/diff/fat_diff/edlin.sh:1`|0|0|none|
|`harness/diff/fat_diff/fat12_ref.py:1`|0|0|none|
|`harness/diff/fat_diff/fat16_ref.py:1`|0|0|none|
|`harness/diff/fat_diff/fat_dump.c:1`|0|0|none|
|`harness/diff/fat_diff/fat_spc.sh:1`|0|0|none|
|`harness/diff/fat_diff/fdisk.sh:1`|0|0|none|
|`harness/diff/fat_diff/fdisk_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/formatsys.sh:1`|0|0|none|
|`harness/diff/fat_diff/formatsys_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/int21_dpb.sh:1`|0|0|none|
|`harness/diff/fat_diff/int21_dpb_program.asm:1`|0|0|none|
|`harness/diff/fat_diff/kernel_hygiene.sh:1`|0|0|none|
|`harness/diff/fat_diff/kernel_hygiene_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/locked_trash.sh:1`|0|0|none|
|`harness/diff/fat_diff/mz_entry_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/partition.sh:1`|0|0|none|
|`harness/diff/fat_diff/redir_full.sh:1`|0|0|none|
|`harness/diff/fat_diff/resident_boot_write.asm:1`|0|0|none|
|`harness/diff/fat_diff/shell_dir_look.sh:1`|0|0|none|
|`harness/diff/fat_diff/shell_files.sh:1`|0|0|none|
|`harness/diff/fat_diff/shell_files_host.h:1`|0|0|none|
|`harness/diff/fat_diff/shell_permanent.sh:1`|0|0|none|
|`harness/diff/fat_diff/trash_hidden_emu.sh:1`|0|0|none|
|`harness/diff/fat_diff/trash_purge_views.sh:1`|0|0|none|
|`harness/diff/fat_diff/truncate.c:1`|0|0|none|
|`harness/diff/fat_diff/truncate.sh:1`|0|0|none|
|`harness/diff/fat_diff/truncate_program.asm:1`|0|0|none|
|`harness/diff/fat_diff/xcopy_cwd.sh:1`|0|0|none|
|`harness/diff/fat_diff/xcopy_tree.sh:1`|0|0|none|
|`harness/diff/wk1_diff/mint_i123_copy_steps.c:1`|0|0|none|
|`harness/diff/wk1_diff/mint_i123_numeric.c:1`|0|0|none|
|`harness/diff/wk1_diff/wk1_canon.py:1`|0|0|none|
|`harness/diff/wk1_diff/wk1_formula_ref.py:1`|0|0|none|
|`harness/diff/wp5_diff/iword_keys.h:1`|0|0|none|
|`harness/diff/wp5_diff/replace.sh:1`|0|0|none|
|`harness/diff/wp5_diff/search.sh:1`|0|0|none|
|`harness/diff/wp5_diff/settings.sh:1`|0|0|none|
|`harness/diff/wp5_diff/wp51_headless.c:1`|0|0|none|
|`harness/edlin/host.c:1`|0|0|none|
|`harness/edlin/model.c:1`|0|0|none|
|`harness/emu/bochs.c:1`|4|4|`harness/emu/bochs.c:121` `BOCHS_MUTATE_MARKER_SUBSTRING`; `harness/emu/bochs.c:129` `BOCHS_MUTATE_PARTIAL_LINE`; `harness/emu/bochs.c:137` `BOCHS_MUTATE_NO_REBOOT_WAIT`; `harness/emu/bochs.c:621` `BOCHS_MUTATE_NO_HOST_LOCK`|
|`harness/emu/bochs.h:1`|0|0|none|
|`harness/emu/bochs_leg_check.sh:1`|0|0|none|
|`harness/emu/bochs_main.c:1`|0|0|none|
|`harness/emu/bochs_pixel_leg.sh:1`|0|0|none|
|`harness/emu/bochs_reboot_check.sh:1`|0|0|none|
|`harness/emu/bochs_trace.h:1`|4|4|`harness/emu/bochs_trace.h:57` `BOCHS_TRACE_MUT_SYNC_NONE`; `harness/emu/bochs_trace.h:88` `BOCHS_TRACE_MUT_NO_CAPTURE`; `harness/emu/bochs_trace.h:91` `BOCHS_TRACE_MUT_ORIGIN`; `harness/emu/bochs_trace.h:184` `BOCHS_KEYS_MUT_RETURN`|
|`harness/emu/calcda_power_check.sh:1`|0|0|none|
|`harness/emu/command_file.sh:1`|0|0|none|
|`harness/emu/copybreak_check.sh:1`|0|0|none|
|`harness/emu/copybreak_seed.c:1`|0|0|none|
|`harness/emu/drive_flair.py:1`|0|0|none|
|`harness/emu/exec_high_check.sh:1`|0|0|none|
|`harness/emu/fixtures/bochs_reboot.asm:1`|0|0|none|
|`harness/emu/fixtures/hang.asm:1`|0|0|none|
|`harness/emu/fixtures/multiboot.ld:1`|0|0|none|
|`harness/emu/fixtures/serial_hello.asm:1`|0|0|none|
|`harness/emu/fixtures/triple_fault.asm:1`|0|0|none|
|`harness/emu/ovl_panic.sh:1`|0|0|none|
|`harness/emu/panic_reasons.sh:1`|0|0|none|
|`harness/emu/panic_screen.sh:1`|0|0|none|
|`harness/emu/power_notice_check.sh:1`|0|0|none|
|`harness/emu/qemu.c:1`|9|7|`harness/emu/qemu.c:1115` `HARNESS_MUTATE_NO_QEMU_SLOTS`; `harness/emu/qemu.c:1159` `HARNESS_MUTATE_NO_KEY_PACE`; `harness/emu/qemu.c:1192` `HARNESS_MUTATE_NO_KEY_PACE`; `harness/emu/qemu.c:1234` `HARNESS_MUTATE_NO_KEY_ACK`; `harness/emu/qemu.c:1727` `HARNESS_MUTATE_NO_PROGRESS_RESET`; `harness/emu/qemu.c:2018` `HARNESS_MUTATE_NO_QUIT_WAIT`; `harness/emu/qemu.c:2330` `HARNESS_MUTATE_NO_QEMU_LOG_CAP`; `harness/emu/qemu.c:2362` `HARNESS_MUTATE_NO_QEMU_LOG_CAP`; `harness/emu/qemu.c:2430` `QEMU_MUTATE_PATH_PREFLIGHT`|
|`harness/emu/qemu.h:1`|0|0|none|
|`harness/emu/qemu_main.c:1`|2|2|`harness/emu/qemu_main.c:259` `HARNESS_MUTATE_NO_BARE_KEYS_CHECK`; `harness/emu/qemu_main.c:332` `HARNESS_MUTATE_KEEP_GREEN_LOG`|
|`harness/emu/realdos_run.sh:1`|0|0|none|
|`harness/emu/require_fault_trace.sh:1`|0|0|none|
|`harness/emu/require_trace.sh:1`|0|0|none|
|`harness/emu/rfb_unblock.py:1`|0|0|none|
|`harness/emu/samir_boot_check.sh:1`|0|0|none|
|`harness/emu/samir_high_stack.sh:1`|0|0|none|
|`harness/emu/samir_link_reserve.asm:1`|0|0|none|
|`harness/emu/shutdown_cancel_check.sh:1`|0|0|none|
|`harness/emu/snapshot_write.asm:1`|0|0|none|
|`harness/emu/test_dos_control.sh:1`|0|0|none|
|`harness/emu/test_qemu_log.sh:1`|0|0|none|
|`harness/emu/test_snapshot.sh:1`|0|0|none|
|`harness/emu/tsr_exec_check.sh:1`|0|0|none|
|`harness/emu/vesa8.sh:1`|0|0|none|
|`harness/factory_smoke.c:1`|0|0|none|
|`harness/fixtures/dos33_overlay/exit.asm:1`|0|0|none|
|`harness/fixtures/dos33_overlay/mint.sh:1`|0|0|none|
|`harness/fixtures/dos33_overlay/overlay.asm:1`|0|0|none|
|`harness/fixtures/dos33_overlay/probe.asm:1`|0|0|none|
|`harness/menubar_clock_oracle.c:1`|0|0|none|
|`harness/menubar_clock_oracle.h:1`|0|0|none|
|`harness/missing_reference.sh:1`|0|0|none|
|`harness/oracle_policy.h:1`|0|0|none|
|`harness/proptest/expect_abort.h:1`|0|0|none|
|`harness/proptest/fixtures/mech_policy_mutant.c:1`|0|0|none|
|`harness/proptest/flair_look_sentinel_stub.c:1`|0|0|none|
|`harness/proptest/flair_routing_stubs.c:1`|0|0|none|
|`harness/proptest/font_styles_golden.h:1`|0|0|none|
|`harness/proptest/gdi_ref_wine.c:1`|0|0|none|
|`harness/proptest/gdi_ref_wine.h:1`|0|0|none|
|`harness/proptest/gdi_ref_wine_shim.h:1`|0|0|none|
|`harness/proptest/gdi_ref_winestubs/ntstatus.h:1`|0|0|none|
|`harness/proptest/gdi_ref_winestubs/request.h:1`|0|0|none|
|`harness/proptest/gdi_ref_winestubs/user.h:1`|0|0|none|
|`harness/proptest/gdi_ref_winestubs/winternl.h:1`|0|0|none|
|`harness/proptest/kernel_cold_answer.c:1`|0|0|none|
|`harness/proptest/kernel_cold_entry.c:1`|0|0|none|
|`harness/proptest/kernel_df_check.sh:1`|0|0|none|
|`harness/proptest/kernel_gc_fixture.c:1`|0|0|none|
|`harness/proptest/kernel_gc_native.c:1`|0|0|none|
|`harness/proptest/kernel_transfer_check.sh:1`|0|0|none|
|`harness/proptest/kernel_transfer_mutants.sh:1`|0|0|none|
|`harness/proptest/ovl_ptr_check.sh:1`|0|0|none|
|`harness/proptest/ovl_reader.c:1`|1|1|`harness/proptest/ovl_reader.c:129` `OVL_READER_MUT_NO_CRC`|
|`harness/proptest/serial_trace_fixture.c:1`|0|0|none|
|`harness/proptest/tbx_trace_fixture.c:1`|0|0|none|
|`harness/proptest/test_asset_lz.c:1`|0|0|none|
|`harness/proptest/test_chicago_pack.c:1`|0|0|none|
|`harness/proptest/test_flair_part_map.c:1`|0|0|none|
|`harness/proptest/test_geneva12_pack.c:1`|0|0|none|
|`harness/proptest/test_kmain_cold.c:1`|0|0|none|
|`harness/pubs/aux_check.sh:1`|0|0|none|
|`harness/pubs/check_allowlist.sh:1`|0|0|none|
|`harness/pubs/check_flair.sh:1`|0|0|none|
|`harness/pubs/check_screens.sh:1`|0|0|none|
|`harness/pubs/cover_check.c:1`|0|0|none|
|`harness/pubs/dos_surface_inventory.c:1`|0|0|none|
|`harness/pubs/entry_index_check.sh:1`|0|0|none|
|`harness/pubs/families.sh:1`|0|0|none|
|`harness/pubs/flair_alerts.sh:1`|0|0|none|
|`harness/pubs/flair_app_menus.c:1`|0|0|none|
|`harness/pubs/flair_grey.c:1`|0|0|none|
|`harness/pubs/flair_inventory.c:1`|0|0|none|
|`harness/pubs/flair_mutants.sh:1`|0|0|none|
|`harness/pubs/int21_inventory.c:1`|0|0|none|
|`harness/pubs/iword_check.sh:1`|0|0|none|
|`harness/pubs/iword_features.sh:1`|0|0|none|
|`harness/pubs/iword_mutants.sh:1`|0|0|none|
|`harness/pubs/iword_probe.c:1`|0|0|none|
|`harness/pubs/label_check.sh:1`|0|0|none|
|`harness/pubs/label_mutants.sh:1`|0|0|none|
|`harness/pubs/mutants.sh:1`|0|0|none|
|`harness/pubs/ppm_grey_png.c:1`|0|0|none|
|`harness/pubs/pubs_gate.c:1`|0|0|none|
|`harness/pubs/railroad.c:1`|0|0|none|
|`harness/pubs/replay.sh:1`|0|0|none|
|`harness/pubs/samir_candidates.sh:1`|0|0|none|
|`harness/pubs/samir_inventory.c:1`|0|0|none|
|`harness/pubs/samir_programs.sh:1`|0|0|none|
|`harness/pubs/samir_replay.c:1`|0|0|none|
|`harness/pubs/samir_sample_disk.c:1`|0|0|none|
|`harness/pubs/samir_tables.c:1`|0|0|none|
|`harness/pubs/slip_mutants.sh:1`|0|0|none|
|`harness/pubs/slip_places.sh:1`|0|0|none|
|`harness/pubs/srclockstep.h:1`|0|0|none|
|`harness/pubs/srclockstep_mutants.sh:1`|0|0|none|
|`harness/pubs/tps_inventory.c:1`|0|0|none|
|`harness/pubs/tps_mutants.sh:1`|0|0|none|
|`harness/pubs/turbo_asmblock.sh:1`|0|0|none|
|`harness/pubs/turbo_gate.c:1`|0|0|none|
|`harness/pubs/turbo_gates_cited.sh:1`|0|0|none|
|`harness/pubs/turbo_grammar.sh:1`|0|0|none|
|`harness/pubs/turbo_mutants.sh:1`|0|0|none|
|`harness/pubs/turbo_replay.sh:1`|0|0|none|
|`harness/registry_mutants.sh:1`|0|0|none|
|`harness/render/alert_capture.h:1`|0|0|none|
|`harness/render/cursor_capture.h:1`|0|0|none|
|`harness/render/icon_capture.h:1`|0|0|none|
|`harness/render/progress_golden.h:1`|0|0|none|
|`harness/render/render.c:1`|0|0|none|
|`harness/render/render.h:1`|0|0|none|
|`harness/ssim.c:1`|5|5|`harness/ssim.c:117` `SSIM_MUT_LUMA`; `harness/ssim.c:128` `SSIM_MUT_WINDOW_9`; `harness/ssim.c:150` `SSIM_MUT_SIGMA`; `harness/ssim.c:153` `SSIM_MUT_K1`; `harness/ssim.c:156` `SSIM_MUT_K2`|
|`harness/verify_fixture_volume.sh:1`|0|0|none|
|`tools/asset_check.c:1`|0|0|none|
|`tools/asset_lz.c:1`|0|0|none|
|`tools/bin2c.c:1`|0|0|none|
|`tools/boot_logo_gen.c:1`|0|0|none|
|`tools/bootstrap/doctor.sh:1`|0|0|none|
|`tools/bootstrap/fetch-siblings.sh:1`|0|0|none|
|`tools/bootstrap/snapshot-sibling.sh:1`|0|0|none|
|`tools/calc_ledger_gen.c:1`|0|0|none|
|`tools/cert-check.sh:1`|0|0|none|
|`tools/cert-sharded-selftest.sh:1`|0|0|none|
|`tools/cert-sharded.sh:1`|0|0|none|
|`tools/cert-summary.sh:1`|0|0|none|
|`tools/check-samir-parallel.sh:1`|0|0|none|
|`tools/chicago_pack.c:1`|0|0|none|
|`tools/color_canon_extract.c:1`|0|0|none|
|`tools/culprit.sh:1`|0|0|none|
|`tools/da_store_check.c:1`|0|0|none|
|`tools/egg_jtc_pack.c:1`|0|0|none|
|`tools/flair_dac_gen.c:1`|1|1|`tools/flair_dac_gen.c:119` `DAC_GEN_MUT_OMIT`|
|`tools/flairovl.c:1`|0|0|none|
|`tools/font_art2h.c:1`|0|0|none|
|`tools/gate_map.c:1`|0|0|none|
|`tools/gen_block_device_spec.c:1`|0|0|none|
|`tools/gen_drive_letters.c:1`|0|0|none|
|`tools/geneva12_art2h.c:1`|0|0|none|
|`tools/iword_fkeys_gen.c:1`|0|0|none|
|`tools/lane-cert.sh:1`|0|0|none|
|`tools/lane-env.sh:1`|0|0|none|
|`tools/merge-train.sh:1`|0|0|none|
|`tools/mint_finder_small_icons.c:1`|0|0|none|
|`tools/mk_boot_system.sh:1`|0|0|none|
|`tools/mk_ovl_tables.sh:1`|0|0|none|
|`tools/mzlink.c:1`|0|0|none|
|`tools/palette_extract.c:1`|0|0|none|
|`tools/ppm_region_count.c:1`|0|0|none|
|`tools/proof-replay.sh:1`|0|0|none|
|`tools/pubs_msgtable.c:1`|0|0|none|
|`tools/revert-merge.sh:1`|0|0|none|
|`tools/samir_memory_ld.c:1`|0|0|none|
|`tools/samir_msg_pack.c:1`|0|0|none|
|`tools/samir_ovl_pack.c:1`|0|0|none|
|`tools/sentinel.sh:1`|0|0|none|
|`tools/serial_trace_pack.c:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `harness/diff/dbf_diff/mint_clients_dbf.c:60` → `os/samir/include/samir/dbf.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/mint_clients_dbf.c:61` → `os/samir/include/samir/value.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/mint_invoice_dbf.c:64` → `os/samir/include/samir/dbf.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/mint_invoice_dbf.c:65` → `os/samir/include/samir/value.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/mint_invoice_dbf.c:66` → `os/samir/include/samir/rt.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/samir_core_ref.c:10` → `seed/test_assert.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/w2_fixture.c:7` → `os/samir/include/samir/dbf.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/w2_fixture.c:8` → `os/samir/include/samir/dbt.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/w2_fixture.c:9` → `os/samir/include/samir/value.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/w2_host.c:14` → `os/samir/samir_main.c`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/w2_host.h:3` → `os/samir/include/samir/pal.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/dbf_diff/w2_host.h:4` → `os/samir/include/samir/interp.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/dosmsg_fault.c:4` → `os/milton/ata.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/fat_dump.c:43` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/truncate.c:7` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/truncate.c:8` → `os/milton/int21.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/truncate.c:9` → `os/milton/fileio_fat.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/truncate.c:10` → `os/milton/psp.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/fat_diff/truncate.c:11` → `os/milton/sft.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/wk1_diff/mint_i123_numeric.c:8` → `os/i123/core/i123.h`: literal cross-component include; evaluate in its #if context.
- `harness/diff/wp5_diff/iword_keys.h:50` → `os/iword/core/iword.h`: literal cross-component include; evaluate in its #if context.
- `harness/edlin/host.c:10` → `os/milton/edlin.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_look_sentinel_stub.c:27` → `os/flair/flair_look.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_look_sentinel_stub.c:29` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:21` → `os/flair/blitter.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:22` → `os/flair/control.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:23` → `os/flair/desktop.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:24` → `os/flair/dialog.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:25` → `os/flair/event.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:26` → `os/flair/scrap.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:27` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:28` → `os/flair/text.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/flair_routing_stubs.c:29` → `os/milton/int21.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/test_asset_lz.c:24` → `os/flair/asset_lz.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/test_asset_lz.c:25` → `os/flair/asset_lz_data.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/test_chicago_pack.c:6` → `os/flair/chicago_packed.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/test_flair_part_map.c:16` → `os/flair/flair_look.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/test_geneva12_pack.c:8` → `os/flair/geneva12_packed.h`: literal cross-component include; evaluate in its #if context.
- `harness/proptest/test_kmain_cold.c:6` → `os/milton/kmain_cold.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/dos_surface_inventory.c:15` → `os/milton/batch.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/dos_surface_inventory.c:16` → `os/milton/config_sys.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/flair_inventory.c:53` → `os/flair/finder_menu.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/flair_inventory.c:54` → `os/flair/finder_cmd.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/flair_inventory.c:55` → `os/flair/apple_menu.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/flair_inventory.c:56` → `os/flair/shell_menus.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/int21_inventory.c:5` → `os/milton/int21.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/iword_probe.c:93` → `os/iword/app/face_a.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_inventory.c:52` → `os/samir/include/samir/interp.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_inventory.c:53` → `os/samir/include/samir/dbf.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_inventory.c:54` → `os/samir/samir_main.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_replay.c:41` → `os/samir/include/samir/interp.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_sample_disk.c:67` → `os/samir/include/samir/dbf.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_sample_disk.c:68` → `os/samir/include/samir/dbt.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_sample_disk.c:69` → `os/samir/include/samir/ndx.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_sample_disk.c:70` → `os/samir/include/samir/value.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_sample_disk.c:71` → `os/samir/include/samir/rt.h`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_tables.c:35` → `os/samir/cmd/flow.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_tables.c:36` → `os/samir/cmd/proc.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_tables.c:37` → `os/samir/cmd/report.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/samir_tables.c:38` → `os/samir/samir_main.c`: literal cross-component include; evaluate in its #if context.
- `harness/pubs/turbo_gate.c:47` → `seed/lexer.h`: literal cross-component include; evaluate in its #if context.
- `harness/render/render.h:43` → `os/flair/heap.h`: literal cross-component include; evaluate in its #if context.
- `tools/egg_jtc_pack.c:7` → `os/flair/egg_jtc_model.h`: literal cross-component include; evaluate in its #if context.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Gates/process wrappers to retire: `test-adr-citations`, `test-adr-citations-mutant`, `test-gate-honesty`, `test-gate-honesty-mutant`, `test-gate-map`, `test-gate-map-mutant`, `test-kernel-repro`, `test-kernel-repro-flagship`, `test-kernel-repro-flagship-mutant`, `test-kernel-repro-mutant`, `test-lane-cert`, `test-lane-cert-mutant`, `test-lane-cert-summaries`, `test-lane-cert-summaries-mutant`, `test-merge-train`, `test-ovl-repro`, `test-ovl-repro-mutant`, `test-sentinel`, `test-sentinel-mutant`.
Retire `tools/ppm_seafoam_check.c:2` (explicitly RETIRED, revoked background canon) and `harness/factory_smoke.c:1` (initial skeleton smoke stub), rather than making them part of the reboot test contract.
Specific legacy process implementation/support files: `harness/adr_citations.mk:1`, `harness/cert_runner.mk:1`, `harness/check_adr_citations.c:1`, `harness/gate_map.mk:1`, `harness/lane_cert.mk:1`, `harness/merge_samir.mk:1`, `harness/proptest/flair_look_sentinel_stub.c:1`, `harness/registry_mutants.sh:1`, `harness/revert_merge.mk:1`, `harness/sentinel.mk:1`, `harness/test_cert_summaries.sh:1`, `harness/test_cert_summaries_mutant.sh:1`, `harness/test_gate_honesty.sh:1`, `harness/test_revert_merge.sh:1`, `harness/test_revert_merge_mutant.sh:1`, `tools/cert-check.sh:1`, `tools/cert-sharded-selftest.sh:1`, `tools/cert-sharded.sh:1`, `tools/cert-summary.sh:1`, `tools/gate_map.c:1`, `tools/lane-cert.sh:1`, `tools/lane-env.sh:1`, `tools/merge-train.sh:1`, `tools/proof-replay.sh:1`, `tools/revert-merge.sh:1`, `tools/sentinel.sh:1`, `tools/test-merge-train.sh:1`. `tools/bootstrap/doctor.sh` should be replaced by a small tool/corpus check: its beads/agent/quota and full-pubs requirements are not component build dependencies.
