# WL-0152 -- Kernel size round 4

Lane: initech-tode, sibling ksize4. Changes left uncommitted for PM review.
No git mutation, beads writes, full make test, Bochs or boot-geometry change.

## Ground truth

Read CLAUDE.md top to bottom, PRD Sec 5, 6.1-6.5, 7-8, WL-0127/WL-0128,
round-2 size policy, ADR-0003 Appendix A / DEC-08 and amendment DEC-08a.
`bd show initech-tode` reports no beads database; the JSONL has no tode row.
The user-supplied in-progress lane scope is the task authority. No database
was initialized or modified.

Rendered at 110 dpi and viewed the local scanned manuals:

- IBM DOS 3.30 Technical Reference, printed 6-200 / PDF 323, AH=4Ch:
  return to the invoking process, return code, close its files.
- MS-DOS 3.3 User's Guide, printed 28 / PDF 41: DIR listing, including
  COMMAND.COM as a visible disk file.

The existing FCB oracle compares its independent real-DOS transcript and
mtools attributes, names, sizes and surviving bytes on the guest-written FAT
image. Its real-DOS transcript is MS-DOS 6.22, not a newly minted 3.3 golden.
InitechText and the other app gates also use mtools/fsck.fat and independent
pixels. No DOS API, message, switch, console layout or user-visible text was
redesigned. Flat kernel/flat-32 applications stay as specified by DEC-08/08a.

Font storage authority: unchanged spec/assets/chicago12.h, PRD Sec 6.4.
Every decoded row is compared with the original locked strike; historical
pixel gates independently grade its appearance. This equality gate grades
storage, not the correctness of the original font drawing.

Native code boundaries: event.c's documented task-owned synthesis versus
SPSC publication, kbd.h Set-1 state contract, and the round-2 size policy's
loader_run_plan/loader_exit_hook warning. Fresh native comparison builds
prove the publication, drain, keyboard IRQ/consumer and loader transfer
functions remain byte-identical to their original -O0 policy. Optimized
host loader/MZ/tenant models, keyboard model and event replay also execute.

## Byte accounting

All sizes below are objcopy raw loaded bytes, excluding NOBITS BSS and the
padding to 384 sectors. Window 196608 B; safety floor 4096 B. Physical load
base, KERNEL_SECTORS, disk geometry and TRANSIENT_BASE are unchanged.

| Cumulative lever | Flagship loaded B | Headroom B | Reclaim B |
| --- | ---: | ---: | ---: |
| Arrival | 188552 | 8056 | - |
| Cold frame-pointer omission / crossjump + indexed main serial records | 182024 | 14584 | 6528 |
| Native -Oz set, pure native managers, selected additional LTO | 176164 | 20444 | 5860 |
| Chicago row dictionary / trimmed blank rows | 174500 | 22108 | 1664 |
| Cold rare shell functions + loader validation region | 173652 | 22956 | 848 |
| Toolbox serial records | 173460 | 23148 | 192 |
| Mergeable packed-record suffix storage | 172948 | 23660 | 512 |
| Task-owned decode regions + non-executing tenant-load wrapper | 171988 | 24620 | 960 |
| About text path split into cold TU | 171988 | 24620 | 0 |
| **Total** | **171988** | **24620** | **16564** |

The last row exceeds the required 16384 B by 180 B. The About split is a
structural separation, not an invented byte saving. Its bounded formatter,
real memory units and unbound `none` text retain their original behavior.

All normal/mutant kmain and Toolbox builds use the same preprocessing and
packing house macros with their own -D flags. GCC -Oz on this host retains
frame pointers by default; explicit omission is confined to the selected
optimized routines. The loader's run/resume region and both rings remain
native -O0. Low-level ATA/PIC/IDT/mouse/PIT/panic/IRQ code stays native (no
LTO); its volatile I/O instructions, four ATA settle reads and bounded poll
counters remain. kbd/event only optimize the isolated synthesis/state
functions, not their publication/consumer paths. Their boundary gate rejects
blanket TU optimization.

No active -O2 recipe remained to downgrade. -fno-asynchronous-unwind-tables
and -fno-ident are kernel-only; discarded metadata is not counted as reclaimed
loaded bytes. -fmerge-all-constants was not adopted: pointer identity of
arbitrary immutable objects was not part of this storage transformation.
Font/palette tables already had one definition. A byte-level .rodata scan
found small overlapping runs in packed strikes (largest 113 B), not a second
whole font or palette table. No locked asset was edited. The new font generator
uses deterministic first-seen row indices and omits blank leading/trailing
rows; unpacking restores all 1568 original uint16 rows.

Rejected scratch probes: broad no-inline policies grew the kernel by 4-6 KiB;
whole-program/points-to options did not help; explicit four-byte preferred
stack alignment grew the near-final kernel by 320 B. Struct-return ABI probes
were not adopted. None changes the final build policy.

## Audit method and evidence

Baseline: build/ksize4/arrival.elf and baseline-top40.txt. Per-TU compiler
recipes were captured with make -Bn, then compiled separately with -fno-lto
into audit/ and audit-final/. The tables below measure native object
.text*/.rodata*/.data* sections, not .eh_frame or BSS. They are diagnostic
attribution probes, not additive ELF byte accounting: LTO crosses TU boundaries
and section GC discards unreferenced functions/data. COMMAND/ENV/BATCH are
transient imports, excluded from the flagship TU table. The authoritative
loaded-size accounting above always uses the linked ELF.

Full logs, original/packed transcripts, mutant output, images and before/after
kernel measurements are under build/ksize4/. Manual renderings were viewed
from /tmp/ksize4-{ibm-323,ms-041}.png. RAM/disk were checked before heavy runs;
22 GB available RAM and 117 GB free disk at acceptance start. QEMU gates run
sequentially with INITECH_QMP_STDIO=1; kernel compilation uses at most four
make jobs. No full vector or other emulator is run by this lane.

## Arrival top 40 loaded symbols

| Symbol | Bytes |
| --- | ---: |
| `tbxf_call` | 640 |
| `region_op` | 649 |
| `draw_platinum_widget.lto_priv.0` | 659 |
| `flair_live_track_box.lto_priv.0` | 707 |
| `flair_draw_rdoc_window` | 714 |
| `fw_do_new_folder` | 728 |
| `fat12_write_file` | 732 |
| `finder_desk_paint` | 732 |
| `finder_desk_persist.lto_priv.0` | 734 |
| `flair_dac` | 768 |
| `region_normalize` | 772 |
| `int13_dispatch` | 813 |
| `isr_dispatch_c` | 835 |
| `sf_draw_dynamic.constprop.0` | 840 |
| `DrawControlDefaultRing.part.0` | 864 |
| `finder_win_open.constprop.0` | 888 |
| `text_draw_run` | 906 |
| `DrawDialog` | 919 |
| `finder_desk_load_positions.lto_priv.0` | 933 |
| `flair_live_do_menu_at.lto_priv.0` | 977 |
| `fat12_mkdir` | 1015 |
| `cook_raw` | 1032 |
| `draw_titlebar_band` | 1039 |
| `geneva9` | 1045 |
| `v_dialog` | 1115 |
| `flair_desktop_run.lto_priv.0` | 1129 |
| `flair_tenant_mount_volume.lto_priv.0` | 1422 |
| `finder_fat_move.lto_priv.0` | 1430 |
| `DrawMenuBar` | 1468 |
| `fat12_write_partial` | 1477 |
| `flair_draw_menu_panel.constprop.0` | 1793 |
| `flair_draw_document_window_scroll` | 3002 |
| `chicago12_rows` | 3136 |
| `DrawControl` | 3179 |
| `finder_report_outcome` | 4565 |
| `flair_live_tenant_service.lto_priv.0` | 5530 |
| `flair_live_do_surface.lto_priv.0` | 6818 |
| `int21_dispatch` | 7849 |
| `tbx_gate_dispatch` | 8346 |
| `kernel_main` | 12689 |

## Final top 40 loaded symbols

| Symbol | Bytes |
| --- | ---: |
| `finder_shutdown_screen` | 650 |
| `fat12_create` | 651 |
| `finder_win_sync_geometry` | 654 |
| `finder_report_outcome` | 676 |
| `finder_desk_persist.lto_priv.0` | 707 |
| `fw_do_new_folder` | 714 |
| `text_draw_run` | 726 |
| `draw_platinum_widget.lto_priv.0` | 728 |
| `flair_live_tenant_service.lto_priv.0` | 729 |
| `conin_cooked_line.constprop.0` | 759 |
| `flair_dac` | 768 |
| `chicago_pack_rows` | 783 |
| `finder_desk_paint` | 791 |
| `flair_live_do_menu_at.lto_priv.0` | 791 |
| `fat12_write_file` | 826 |
| `int13_dispatch` | 853 |
| `sf_draw_dynamic.constprop.0` | 874 |
| `DrawDialog` | 901 |
| `finder_win_open.constprop.0` | 910 |
| `v_dialog` | 924 |
| `DrawControlDefaultRing.part.0` | 947 |
| `flair_exit_to_dos` | 958 |
| `draw_titlebar_band` | 968 |
| `geneva9` | 1045 |
| `fat12_mkdir` | 1072 |
| `finder_empty_trash` | 1168 |
| `flair_tenant_mount_volume.lto_priv.0` | 1208 |
| `finder_fat_move.lto_priv.0` | 1253 |
| `flair_live_tenant_filecopy.lto_priv.0` | 1294 |
| `DrawMenuBar` | 1548 |
| `fat12_write_partial` | 1659 |
| `flair_desktop_run.lto_priv.0` | 1833 |
| `flair_draw_menu_panel.constprop.0` | 1923 |
| `flair_live_tenant_sf` | 2415 |
| `DrawControl` | 3490 |
| `paint_window_chrome` | 4486 |
| `flair_live_do_surface.lto_priv.0` | 5679 |
| `int21_dispatch` | 6629 |
| `tbx_gate_dispatch` | 7606 |
| `kernel_main` | 11614 |

## Per-TU native audit

| TU | Before B | After B |
| --- | ---: | ---: |
| `kmain_flairtenants.o` | 42419 | 37348 |
| `int21.o` | 18436 | 17289 |
| `fat12.o` | 15735 | 17147 |
| `tbxgate.o` | 17319 | 16069 |
| `finder_windows.o` | 11092 | 10872 |
| `chrome.o` | 8031 | 8663 |
| `window_live.o` | 7048 | 7113 |
| `text.o` | 6541 | 7078 |
| `finder_desktop.o` | 6851 | 6867 |
| `control.o` | 6404 | 6817 |
| `menu.o` | 5899 | 5979 |
| `loader.o` | 5089 | 4076 |
| `dialog.o` | 3624 | 3752 |
| `region.o` | 4222 | 3748 |
| `fileio_fat.o` | 3239 | 3238 |
| `finder_ops.o` | 2871 | 3077 |
| `process.o` | 2511 | 2558 |
| `sysinit.o` | 2396 | 2423 |
| `console.o` | 2313 | 2304 |
| `event.o` | 2640 | 2025 |
| `finder_cmd.o` | 1587 | 1585 |
| `ref_tenant.o` | 1510 | 1539 |
| `rtc.o` | 1473 | 1473 |
| `devices.o` | 1458 | 1437 |
| `desktop_db.o` | 1240 | 1293 |
| `panic.o` | 1928 | 1273 |
| `finder_menu.o` | 1262 | 1258 |
| `flair_shell.o` | 1217 | 1247 |
| `stdfile.o` | 1317 | 1224 |
| `desktop.o` | 1255 | 1213 |
| `mcb.o` | 1309 | 1182 |
| `cursor.o` | 1200 | 1144 |
| `config_sys.o` | 1163 | 1144 |
| `ata.o` | 2566 | 1096 |
| `kbd.o` | 1272 | 1052 |
| `block_device.o` | 1052 | 1016 |
| `flair_look.o` | 953 | 942 |
| `psp.o` | 846 | 797 |
| `list.o` | 833 | 794 |
| `blitter.o` | 778 | 775 |
| `sft.o` | 703 | 666 |
| `finder_icon.o` | 567 | 598 |
| `apple_menu.o` | 483 | 477 |
| `idt.o` | 552 | 439 |
| `scrap.o` | 366 | 357 |
| `surface.o` | 352 | 353 |
| `mouse.o` | 755 | 310 |
| `heap.o` | 302 | 289 |
| `kmain_cold.o` | - | 273 |
| `type_prog_blob.o` | 193 | 193 |
| `irq.o` | 346 | 184 |
| `dir_prog_blob.o` | 159 | 159 |
| `pit.o` | 265 | 118 |
| `pic.o` | 386 | 100 |
| `test_prog_blob.o` | 53 | 53 |

## RED -> GREEN and Rule 6

The fixed r4 budget was added before implementation and rejected arrival:

    !!! kernel-size-r4 FAIL: loaded 188552 B > 172168 B (16 KiB reclaim required)

The formatter oracle also caught an actual implementation defect during
bootstrap: byte 916 / line 23 differed when an unbraced else absorbed later
statements. The parser now groups only standalone statements; control-arm
calls retain their original scope. Side-effecting argument calls remain
original C. The fixture performs no interactive input.

All five host gate pairs are in TEST_UNIT_GATES; the two injected emulator
wrappers are in TEST_EMU_GATES. The exact new target tails from the host run:

    >>> test-kernel-size-r4: green (flagship reclaims >= 16384 B; window/floor unchanged)
    >>> test-kernel-size-r4-mutant: green (size_pad_one RED)
    >>> test-serial-trace-pack: green (original C calls == indexed packed trace bytes)
    >>> test-serial-trace-pack-mutant: green (trace_byte RED)
    >>> test-chicago-pack: green (all 1568 decoded rows == unchanged locked strike)
    >>> test-chicago-pack-mutant: green (packed_row RED)
    >>> test-kernel-native-boundaries: green (rings, keyboard IRQ and loader transfer byte-identical to native -O0)
    >>> test-kernel-native-boundaries-mutant: green (whole_tu_oz RED)
    >>> test-kernel-cold-about: green (memory units, unbound text, uint32 limits, bounds)
    >>> test-kernel-cold-about-mutant: green (about_none RED)

Named new mutants and actual RED lines:

    size_pad_one: !!! kernel-size-r4 FAIL: loaded 172169 B > 172168 B (16 KiB reclaim required)
    trace_byte: !!! serial-trace-pack FAIL: trace_byte changes the original transcript
    packed_row: !!! chicago-pack FAIL: glyph=1 row=3 got=C001 expected=C000
    whole_tu_oz: !!! kernel-native-boundary FAIL: flair_raw_post changed
    about_none: !!! cold-about FAIL: case 1 text/units/bounds differ

The native boundary gate also executes the selected-policy host models:
loader 29, MZ load 23, tenant load 25, event replay 322, keyboard 325 checks:
724 total, zero failures. test-makefile-vars, its blank-line lint, new-source
ASCII and trailing-whitespace checks pass. Both record packing modes compare
against independently compiled original C output, including signed/unsigned
limits, fixed hex width, quotes, adjacent literals and control-flow scope.



## Final acceptance

All 24 targets in build/ksize4/acceptance.log are GREEN, including every
requested family and both new emulator wrappers. The final host gate run,
after the build-dependency correction, is build/ksize4/final-host-verified.log:
all ten new host targets, their five named mutants, test-makefile-vars and
test-kernel-gc-roots pass. Native models total 724 checks, zero failures.

The codegen tools are prerequisites of the OBJECT rules, not rules attached
to their C source files. Attaching them to source targets combined with the
project-wide .EXTRA_PREREQS stamp caused unnecessary subsequent rebuilds.
That dependency defect was corrected before final acceptance. A final
`make -n build/kernel_flairtenants.elf` reports up to date.

Two clean flagship object builds and the current padded binary are identical:

    >>> test-kernel-flair-repro: green (two clean builds byte-identical; sha256 f61c3d07b697eb8c80347045f2f4143756be14207669eb9870821cde294b6ccd)
    GREEN test-boot-system-repro: two fresh FAT builds == each other == shipped boot image, byte for byte

MBR and stage2 binaries compare identical to arrival. A strings scan of the
shipped kernel finds no /home/ or /tmp/ paths. New source is ASCII-clean and
has no trailing whitespace. No implementation changes followed these checks.

| Target | Exact GREEN/PASS tail |
| --- | --- |
| `test-kernel-headroom` | >>> test-kernel-headroom: green |
| `test-kernel-headroom-mutant` | >>> test-kernel-headroom-mutant: green (floor-1 pad kernel RED, floor pad kernel GREEN) |
| `test-kernel-flair-repro` | >>> test-kernel-flair-repro: green (two clean builds byte-identical; sha256 f61c3d07b697eb8c80347045f2f4143756be14207669eb9870821cde294b6ccd) |
| `image` | >>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT) |
| `test-flair-desktop` | VERDICT   : PASS -- the LIVE FLAIR chimera desktop is the Office Space frame |
| `test-flair-appswitch` | VERDICT   : PASS -- clicking the background tenant RAISES + ACTIVATES it and swaps |
| `test-flair-app-launch` | >>> test-flair-app-launch: green |
| `test-flair-app-launch-mutant` | >>> test-flair-app-launch-mutant: green (all six mutants RED for their named reasons) |
| `test-flair-exit-to-dos` | >>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed) |
| `test-flair-tps-journey` | >>> test-flair-tps-journey: green (QEMU; Bochs leg owed) |
| `test-boot` | VERDICT   : PASS -- InitechDOS banner printed (serial == spec, rendered on screen), no triple-fault |
| `test-boot-system` | GREEN test-boot-system: mdir order + mattrib HSR + mcopy/cmp both files + fsck.fat clean |
| `test-boot-system-repro` | GREEN test-boot-system-repro: two fresh FAT builds == each other == shipped boot image, byte for byte |
| `test-shell` | VERDICT   : PASS -- booted InitechDOS, got an A:\> prompt, ran DIR/TYPE/a program/EXIT via COMMAND.COM |
| `test-flair-locked-purge` | VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat) |
| `test-flair-finder-service` | >>> test-flair-finder-service: green |
| `test-flair-itext` | >>> test-flair-itext: green |
| `test-flair-calcda` | >>> test-flair-calcda: green |
| `test-flair-cpanels` | >>> test-flair-cpanels-memory: green (locked trace, dialog routing, independent button/ring chrome) |
| `test-flair-notepad` | >>> test-flair-notepad: green (Apple launch; dog-ear; TEXT Scrap; turn/close/reopen; independent FAT + PNG) |
| `test-fat-partition` | GREEN test-fat-partition |
| `test-block-device-emu` | GREEN test-block-device-emu: QEMU INT13 MBR 55AA/type01, scratch LBA32 host readback, every other sector intact |
| `test-kernel-size-r4-emu` | >>> test-kernel-size-r4-emu: green (DOS/FAT differential; injected desktop/text traces and pixels) |
| `test-kernel-size-r4-emu-mutant` | >>> test-kernel-size-r4-emu-mutant: green (FCB and desktop behaviour mutants RED) |

The new emulator mutant wrapper rejects the inherited independent FCB and
desktop mutants with their actual witnesses:

    seq: < T03 SEQWRITE 00 00 00 BLK=0000 REC=03 SIZE=00000030
    noattr: < T14 HIDDEN 00 00 FF FF 00 X=FF0201 FCBHID  DAT A=22
    nozero: < T09 PARTIAL 03 SUM=15B4 B75=4B B76=00 BLK=0000 REC=01
    ignore_refcon: FLAIR-DISPATCH app=NOTES never fired
    skip_activate: FAIL TIER-B: POST NOTES accent block (centre) is the active accent [POST]
    drop_update: FAIL TIER-A: POST overlap probe is NOTES_FILL (NOTES raised + updateEvt-repainted) [POST]
    no_menubar_swap: FAIL MENU-BAND -- foreground menubar UNCHANGED (0 differing px < 8)
    notes_bar_sys: FAIL POST-DISTINCT -- stacked menu bars IDENTICAL (0 differing px < 8)

The requested app-launch mutant gate also rejects all six:

    NO_REGISTER: no TENANT-REGISTER, golden trace diverges, desktop intact
    DOUBLE_LAUNCH: second REGISTER installed, no TENANT-SLOT-BUSY
    EXIT_LEAK: TENANT-HEAPAVAIL drifted 3490784 -> 3486672
    CLOSE_STALE_FOCUS: no TENANT-EXIT; relaunch hit TENANT-SLOT-BUSY on windowless zombie
    MBAR_NOT_SWAPPED: SETMBAR said 0, foreground tenant still has shell fallback bar
    MBAR_NO_BOUNDS: PSP MenuBar accepted -> 0; no TENANT-SETMBAR-BAD; out-of-image bar reached band 2

The headroom boundary mutant still rejects 4095 B and accepts exactly 4096 B.
All five app-switch mutants and all three FCB mutants are run by the new
emulator wrapper; no synthetic expected transcript or pixel golden was added.

## All-kernel accounting and remaining limitation

219 kernels are measured before and after. Arrival binding:
`kernel_flairtenants_db_write_skip` 189416 B / 7192 B headroom. Final binding:
the same kernel 173140 B / 23468 B headroom. Binding recovery is 16276 B;
flagship recovery is 16564 B. The requested 16384 B target is for the flagship.

At round-4 acceptance, test-kernel-size-r3 had a fixed 172492 B cap for
EVERY kernel.
The arrival build already exceeded it; the final binding remains 648 B above
it. That historical gate was not weakened or removed. It was not run as part
of this lane, and this report does not claim the full vector is green. Its
absolute cap was left as a PM policy follow-up; operator ruling 2026-10-24
(initech-oiygj, WL-0181) has since retired it and its mutant. No beads were
edited. Bochs, 86Box and full make test were not run, as instructed. No
commit or push.
All requested round-4 QEMU acceptance and determinism checks are complete.

| Kernel ELF | Before B | After B | Reclaim B |
| --- | ---: | ---: | ---: |
| `kernel.elf` | 63244 | 57580 | 5664 |
| `kernel_absdisk.elf` | 63812 | 58148 | 5664 |
| `kernel_alert_chrome_fringe.elf` | 188820 | 172272 | 16548 |
| `kernel_alert_chrome_label.elf` | 188500 | 171824 | 16676 |
| `kernel_alert_chrome_slab.elf` | 188528 | 171884 | 16644 |
| `kernel_ansi_ANSIWIRE_MUTATE_DROP_DSR.elf` | 98073 | 92985 | 5088 |
| `kernel_ansi_ANSIWIRE_MUTATE_DROP_REL.elf` | 98329 | 93177 | 5152 |
| `kernel_block_device_relative.elf` | 98301 | 93209 | 5092 |
| `kernel_chkdsk_cache.elf` | 98265 | 93177 | 5088 |
| `kernel_conin.elf` | 63544 | 57848 | 5696 |
| `kernel_datetime.elf` | 63984 | 58288 | 5696 |
| `kernel_doslook_mut_cursor.elf` | 99805 | 94813 | 4992 |
| `kernel_doslook_mut_date.elf` | 97872 | 92752 | 5120 |
| `kernel_doslook_mut_home.elf` | 99805 | 94781 | 5024 |
| `kernel_doslook_mut_joined.elf` | 98384 | 93264 | 5120 |
| `kernel_doslook_mut_lower.elf` | 98320 | 93200 | 5120 |
| `kernel_doslook_mut_volume.elf` | 98256 | 93104 | 5152 |
| `kernel_echo.elf` | 63372 | 57740 | 5632 |
| `kernel_exec.elf` | 63788 | 57996 | 5792 |
| `kernel_exith.elf` | 63724 | 57932 | 5792 |
| `kernel_exith_mut.elf` | 63596 | 57900 | 5696 |
| `kernel_fault.elf` | 63340 | 57676 | 5664 |
| `kernel_flairlive.elf` | 90200 | 83192 | 7008 |
| `kernel_flairlive_int.elf` | 162316 | 151528 | 10788 |
| `kernel_flairlive_mut.elf` | 90125 | 83093 | 7032 |
| `kernel_flairlive_mut_drag.elf` | 88749 | 81269 | 7480 |
| `kernel_flairlive_mut_eoi.elf` | 90168 | 83192 | 6976 |
| `kernel_flairlive_mut_kbd.elf` | 90093 | 83061 | 7032 |
| `kernel_flairlive_mut_modal_restore.elf` | 90125 | 83093 | 7032 |
| `kernel_flairlive_mut_mouse.elf` | 89997 | 82965 | 7032 |
| `kernel_flairlive_nomodal.elf` | 90189 | 83157 | 7032 |
| `kernel_flairlive_nomodal_mut_menu.elf` | 85677 | 77953 | 7724 |
| `kernel_flairlive_nomodal_mut_norehit.elf` | 90189 | 83157 | 7032 |
| `kernel_flairshell.elf` | 68084 | 62004 | 6080 |
| `kernel_flairshell_mut_modalbehind.elf` | 68052 | 62228 | 5824 |
| `kernel_flairshell_mut_nomodal.elf` | 62708 | 55060 | 7648 |
| `kernel_flairshell_mut_onebar.elf` | 67860 | 61940 | 5920 |
| `kernel_flairtenants.elf` | 188552 | 171988 | 16564 |
| `kernel_flairtenants_calcda_apple_dead.elf` | 188608 | 171916 | 16692 |
| `kernel_flairtenants_calcda_rdoc_no_close.elf` | 188564 | 171884 | 16680 |
| `kernel_flairtenants_db_write_skip.elf` | 189416 | 173140 | 16276 |
| `kernel_flairtenants_int.elf` | 187772 | 171180 | 16592 |
| `kernel_flairtenants_mut_calc_geneva_ignored.elf` | 188552 | 171956 | 16596 |
| `kernel_flairtenants_mut_calc_token_alias.elf` | 188552 | 171956 | 16596 |
| `kernel_flairtenants_mut_calcda_gate.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_clock_stuck.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_clock_wrong_hour.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_close_hide_only.elf` | 188360 | 171796 | 16564 |
| `kernel_flairtenants_mut_command_magic.elf` | 188560 | 171884 | 16676 |
| `kernel_flairtenants_mut_command_message.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_command_volume.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_cp_mouse.elf` | 188540 | 171928 | 16612 |
| `kernel_flairtenants_mut_cp_radio.elf` | 188660 | 172016 | 16644 |
| `kernel_flairtenants_mut_cp_ring.elf` | 188244 | 171632 | 16612 |
| `kernel_flairtenants_mut_cpanels_gate.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_cursor_squared.elf` | 188616 | 172020 | 16596 |
| `kernel_flairtenants_mut_cwdreset.elf` | 188488 | 172948 | 15540 |
| `kernel_flairtenants_mut_dac_dac6.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_dac_omit119.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_dac_omit165.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_dac_rowcopy.elf` | 188592 | 171916 | 16676 |
| `kernel_flairtenants_mut_desk_dbltick_off.elf` | 187592 | 171156 | 16436 |
| `kernel_flairtenants_mut_desk_no_hilite.elf` | 188488 | 171860 | 16628 |
| `kernel_flairtenants_mut_desk_no_underlay.elf` | 188424 | 171860 | 16564 |
| `kernel_flairtenants_mut_drop_update.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_exit_no_teardown.elf` | 188400 | 171820 | 16580 |
| `kernel_flairtenants_mut_fg_follows_front.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_file_leak.elf` | 188520 | 171924 | 16596 |
| `kernel_flairtenants_mut_file_write.elf` | 188584 | 171988 | 16596 |
| `kernel_flairtenants_mut_filecopy_no_copy.elf` | 188456 | 171860 | 16596 |
| `kernel_flairtenants_mut_filecopy_rate_ignored.elf` | 188200 | 171636 | 16564 |
| `kernel_flairtenants_mut_film_busy_arrow.elf` | 163468 | 153128 | 10340 |
| `kernel_flairtenants_mut_film_scene.elf` | 163052 | 152488 | 10564 |
| `kernel_flairtenants_mut_fonts_clip_ignore.elf` | 188328 | 172692 | 15636 |
| `kernel_flairtenants_mut_fonts_smear_off.elf` | 188520 | 172884 | 15636 |
| `kernel_flairtenants_mut_fonts_trunc_period.elf` | 188568 | 172020 | 16548 |
| `kernel_flairtenants_mut_goaway_on_press.elf` | 187644 | 171020 | 16624 |
| `kernel_flairtenants_mut_grow_figure.elf` | 188568 | 172020 | 16548 |
| `kernel_flairtenants_mut_grow_no_commit.elf` | 188264 | 171796 | 16468 |
| `kernel_flairtenants_mut_hold.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_hold_timeout.elf` | 188496 | 171884 | 16612 |
| `kernel_flairtenants_mut_i123_close_noask.elf` | 188464 | 171852 | 16612 |
| `kernel_flairtenants_mut_i123_file_gate.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_i123_menu_gate.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_ignore_refcon.elf` | 188456 | 171892 | 16564 |
| `kernel_flairtenants_mut_iword_gate.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_iword_ullow.elf` | 188572 | 171992 | 16580 |
| `kernel_flairtenants_mut_l002.elf` | 187424 | 170868 | 16556 |
| `kernel_flairtenants_mut_locked_notice.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_locked_purge.elf` | 188328 | 171892 | 16436 |
| `kernel_flairtenants_mut_locked_stage.elf` | 188392 | 171796 | 16596 |
| `kernel_flairtenants_mut_menu2_bar_sys.elf` | 188592 | 171948 | 16644 |
| `kernel_flairtenants_mut_menu2_dead.elf` | 187388 | 171468 | 15920 |
| `kernel_flairtenants_mut_menu_dead_item.elf` | 188552 | 171988 | 16564 |
| `kernel_flairtenants_mut_menu_look_flat.elf` | 188556 | 171960 | 16596 |
| `kernel_flairtenants_mut_menu_look_gate.elf` | 188552 | 171924 | 16628 |
| `kernel_flairtenants_mut_menu_no_restore.elf` | 188720 | 172044 | 16676 |
| `kernel_flairtenants_mut_menu_old_divider_inset.elf` | 188588 | 171960 | 16628 |
| `kernel_flairtenants_mut_menu_restore_shell.elf` | 188284 | 171628 | 16656 |
| `kernel_flairtenants_mut_menu_stub_enabled.elf` | 188552 | 171988 | 16564 |
| `kernel_flairtenants_mut_menubar_clock_clip.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_menubar_clock_gate.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_no_activate_inval.elf` | 188520 | 171732 | 16788 |
| `kernel_flairtenants_mut_no_desk_activate.elf` | 188432 | 171756 | 16676 |
| `kernel_flairtenants_mut_no_drag_clamp.elf` | 188336 | 171660 | 16676 |
| `kernel_flairtenants_mut_no_menubar_swap.elf` | 188560 | 171916 | 16644 |
| `kernel_flairtenants_mut_no_pulse.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_no_raise_on_title.elf` | 188552 | 172020 | 16532 |
| `kernel_flairtenants_mut_no_rebuild.elf` | 188592 | 171948 | 16644 |
| `kernel_flairtenants_mut_no_route_on_chrome.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_notes_bar_sys.elf` | 188424 | 171748 | 16676 |
| `kernel_flairtenants_mut_ops_trash_no_stage.elf` | 188584 | 172020 | 16564 |
| `kernel_flairtenants_mut_power_no_ask.elf` | 188188 | 171596 | 16592 |
| `kernel_flairtenants_mut_power_no_flush.elf` | 188592 | 171948 | 16644 |
| `kernel_flairtenants_mut_progress_flat.elf` | 188104 | 172116 | 15988 |
| `kernel_flairtenants_mut_purge_keeps_views.elf` | 188232 | 171764 | 16468 |
| `kernel_flairtenants_mut_record.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_recordlong.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_mut_recordxlong.elf` | 188720 | 172044 | 16676 |
| `kernel_flairtenants_mut_scroll_drag.elf` | 188520 | 171732 | 16788 |
| `kernel_flairtenants_mut_scroll_no_min.elf` | 188520 | 171732 | 16788 |
| `kernel_flairtenants_mut_scroll_paint_only.elf` | 188680 | 172116 | 16564 |
| `kernel_flairtenants_mut_scroll_ramp_flat.elf` | 188568 | 172020 | 16548 |
| `kernel_flairtenants_mut_scroll_stale_resize.elf` | 188392 | 171604 | 16788 |
| `kernel_flairtenants_mut_set1.elf` | 188520 | 171988 | 16532 |
| `kernel_flairtenants_mut_shutdown_returns.elf` | 188464 | 171852 | 16612 |
| `kernel_flairtenants_mut_skip_activate.elf` | 188392 | 171828 | 16564 |
| `kernel_flairtenants_mut_tbx_draw_not_presented.elf` | 188488 | 171924 | 16564 |
| `kernel_flairtenants_mut_tbx_drawmbar_noop.elf` | 188552 | 171956 | 16596 |
| `kernel_flairtenants_mut_tbx_exit_leak.elf` | 188520 | 171956 | 16564 |
| `kernel_flairtenants_mut_tbx_mbar_no_bounds.elf` | 188328 | 171764 | 16564 |
| `kernel_flairtenants_mut_tbx_mbar_not_swapped.elf` | 188488 | 171924 | 16564 |
| `kernel_flairtenants_mut_tbx_menu_key_plain.elf` | 188456 | 171828 | 16628 |
| `kernel_flairtenants_mut_tbx_menu_not_delivered.elf` | 188264 | 171700 | 16564 |
| `kernel_flairtenants_mut_tbx_no_slot_guard.elf` | 188456 | 171924 | 16532 |
| `kernel_flairtenants_mut_tbx_smoke.elf` | 105624 | 87604 | 18020 |
| `kernel_flairtenants_mut_tbx_smoke_i123.elf` | 104472 | 86548 | 17924 |
| `kernel_flairtenants_mut_tenant_close_hide.elf` | 188624 | 171980 | 16644 |
| `kernel_flairtenants_mut_trackcap.elf` | 188672 | 171980 | 16692 |
| `kernel_flairtenants_mut_win_abs_coords.elf` | 187432 | 170900 | 16532 |
| `kernel_flairtenants_mut_win_empty_noconfirm.elf` | 188552 | 172084 | 16468 |
| `kernel_flairtenants_mut_win_service_shown.elf` | 188520 | 171956 | 16564 |
| `kernel_flairtenants_mut_win_singleton_dup.elf` | 187560 | 171124 | 16436 |
| `kernel_flairtenants_mut_win_trash_root.elf` | 188520 | 171988 | 16532 |
| `kernel_flairtenants_mut_win_vollabel_shown.elf` | 188520 | 171956 | 16564 |
| `kernel_flairtenants_recorddbl.elf` | 188592 | 171884 | 16708 |
| `kernel_flairtenants_recordlongdbl.elf` | 188624 | 171948 | 16676 |
| `kernel_flairtenants_recordxlongdbl.elf` | 188720 | 172012 | 16708 |
| `kernel_fpu_absent.elf` | 31693 | 28421 | 3272 |
| `kernel_fpu_absent_mut_em.elf` | 31693 | 28421 | 3272 |
| `kernel_fpu_mf.elf` | 32109 | 28773 | 3336 |
| `kernel_fpu_mf_mut_skip.elf` | 31725 | 28389 | 3336 |
| `kernel_fpu_mut_cw.elf` | 31981 | 28709 | 3272 |
| `kernel_fpu_mut_skip.elf` | 31629 | 28325 | 3304 |
| `kernel_fpu_test.elf` | 31981 | 28709 | 3272 |
| `kernel_irqstorm.elf` | 64147 | 58423 | 5724 |
| `kernel_irqstorm_muta.elf` | 64147 | 58423 | 5724 |
| `kernel_irqstorm_mutb.elf` | 64147 | 58423 | 5724 |
| `kernel_memtest.elf` | 64044 | 58284 | 5760 |
| `kernel_memtest_mut.elf` | 64012 | 58252 | 5760 |
| `kernel_multiopen.elf` | 64040 | 58344 | 5696 |
| `kernel_mzexec.elf` | 63532 | 57804 | 5728 |
| `kernel_mzforeign.elf` | 63481 | 57785 | 5696 |
| `kernel_partition_EXTENDED.elf` | 107525 | 102117 | 5408 |
| `kernel_partition_IGNORE_BASE.elf` | 107525 | 102117 | 5408 |
| `kernel_partition_SIGNATURE.elf` | 107493 | 102085 | 5408 |
| `kernel_shell.elf` | 98300 | 92796 | 5504 |
| `kernel_shell_fcb_noattr.elf` | 98265 | 93113 | 5152 |
| `kernel_shell_fcb_nozero.elf` | 98329 | 93209 | 5120 |
| `kernel_shell_fcb_seq.elf` | 98265 | 93145 | 5120 |
| `kernel_shell_j1lu_abort.elf` | 98329 | 93177 | 5152 |
| `kernel_shell_j1lu_batch2ctrlc.elf` | 98329 | 93209 | 5120 |
| `kernel_shell_j1lu_batch2ctrlz.elf` | 98329 | 93145 | 5184 |
| `kernel_shell_j1lu_batch2cwd.elf` | 98201 | 93145 | 5056 |
| `kernel_shell_j1lu_ctrl.elf` | 98249 | 93321 | 4928 |
| `kernel_shell_j1lu_dosbreak.elf` | 98425 | 93209 | 5216 |
| `kernel_shell_j1lu_eof.elf` | 98297 | 93145 | 5152 |
| `kernel_shell_j1lu_raw.elf` | 98329 | 93177 | 5152 |
| `kernel_shell_j1lu_restore.elf` | 98288 | 93168 | 5120 |
| `kernel_shell_j1lu_stamp.elf` | 97881 | 92761 | 5120 |
| `kernel_shell_mut_autoexec.elf` | 98288 | 93168 | 5120 |
| `kernel_shell_mut_no_jft_inherit.elf` | 98265 | 93177 | 5088 |
| `kernel_shell_mut_nolt.elf` | 98352 | 93200 | 5152 |
| `kernel_shell_mut_nostdin.elf` | 98160 | 93040 | 5120 |
| `kernel_shell_mut_ojxn.elf` | 97616 | 92464 | 5152 |
| `kernel_shell_mut_protection_copy.elf` | 107493 | 102085 | 5408 |
| `kernel_shell_mut_protection_del.elf` | 107493 | 102085 | 5408 |
| `kernel_shell_mut_redirbypass.elf` | 98320 | 93168 | 5152 |
| `kernel_shell_mut_safety_batch2body.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_batch2date.elf` | 98288 | 93136 | 5152 |
| `kernel_shell_mut_safety_batch2dollar.elf` | 98352 | 93200 | 5152 |
| `kernel_shell_mut_safety_batch2for.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_batch2shift.elf` | 98320 | 93168 | 5152 |
| `kernel_shell_mut_safety_batch2wild.elf` | 98064 | 92944 | 5120 |
| `kernel_shell_mut_safety_copydate.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_copytime.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_denied.elf` | 98288 | 93168 | 5120 |
| `kernel_shell_mut_safety_dirargs.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_k02.elf` | 97968 | 92848 | 5120 |
| `kernel_shell_mut_safety_k03.elf` | 98320 | 93168 | 5152 |
| `kernel_shell_mut_safety_k04.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_k15.elf` | 98140 | 92988 | 5152 |
| `kernel_shell_mut_safety_pathcmd.elf` | 98316 | 93164 | 5152 |
| `kernel_shell_mut_safety_permanent.elf` | 98320 | 92752 | 5568 |
| `kernel_shell_mut_safety_refusey.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_safety_rostop.elf` | 98256 | 93136 | 5120 |
| `kernel_shell_mut_safety_silent.elf` | 98288 | 93168 | 5120 |
| `kernel_shell_mut_safety_yesall.elf` | 98320 | 93200 | 5120 |
| `kernel_shell_mut_ut6d.elf` | 98040 | 92920 | 5120 |
| `kernel_shell_mut_winh.elf` | 98288 | 93168 | 5120 |
| `kernel_shell_mut_zs24reject.elf` | 98361 | 93241 | 5120 |
| `kernel_shell_mut_zs24rootonly.elf` | 98321 | 93937 | 4384 |
| `kernel_shell_rdnoop_ut6d.elf` | 98168 | 93048 | 5120 |
| `kernel_shell_secondary.elf` | 98297 | 93177 | 5120 |
| `kernel_spurious.elf` | 63372 | 57708 | 5664 |
| `kernel_startup_mut.elf` | 97776 | 92240 | 5536 |
| `kernel_sysi.elf` | 63564 | 57836 | 5728 |
| `kernel_vect.elf` | 63712 | 57984 | 5728 |
| `kernel_write.elf` | 63684 | 58020 | 5664 |

## Changed files

- Makefile
- harness/diff/fat_diff/block_device.mk
- os/milton/kmain.c, kbd.c, loader.c, kmain_cold.c, kmain_cold.h
- os/flair/event.c, text.c, text.h, chicago_packed.h
- tools/serial_trace_pack.c, tools/chicago_pack.c
- harness/proptest/kernel_size_r4.mk, serial_trace_fixture.c,
  tbx_trace_fixture.c, test_chicago_pack.c, test_kmain_cold.c
- docs/worklog/WL-0152-kernel-size-round4.md
