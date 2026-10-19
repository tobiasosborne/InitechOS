# WL-0168 -- Kernel size round 5: 11,040 B reclaimed by calling-convention, pass policy and packed strike storage

Lane ksize5 (branch lane/ksize5, worktree lane-ksize5): an Opus lane left
uncommitted scaffolding when the PM paused everything; this shard is the
Fable takeover that committed it as WIP, merged main (the SAMIR size round
2 + DPB + routing merges, no conflicts) and finished the round. Nothing
pushed; CLAUDE.md, HANDOFF and beads untouched. Every QEMU gate ran one at
a time with INITECH_QMP_STDIO=1 (scratchpad env.sh); `make test` was not
run (brief).

## Context

test-kernel-size-r4 was RED on main after the boot screen, the DEC-18 DOS
volumes and the SAMIR hotkey drain landed:

    !!! kernel-size-r4 FAIL: loaded 186196 B > 180224 B (the flagship must keep 16 KiB headroom -- size round 5 is due)

On the merged lane tree (main at 3c32c4df + the WIP scaffolding) the
flagship measured 187,788 B: 7,564 B over the floor, with seven kernel
lanes still to merge. The r4 gate (harness/proptest/kernel_size_r4.mk) is
a HEADROOM floor -- loaded bytes <= KERNEL_SECTORS * 512 - 16384 = 180,224
B -- so the target is a reclaim large enough to leave margin under it.
KERNEL_SECTORS and the 16 KiB floor were not touched (brief stop
condition).

## Measure first (merged tree, build/kernel_flairtenants.elf)

objcopy raw loaded bytes 187,788: .text 164,105 B (750 symbols, zero
inter-symbol padding), .rodata 22,754 B (120 named symbols = 14,821 B; the
unnamed remainder is 7,444 B of string literals, almost all of them canon
user-visible text or locked serial markers the traces grade), .data 876 B.
Every symbol > 256 B, by section, is reproduced in the appendix; the head
of it:

| B | .text symbol | B | .rodata symbol |
| ---: | --- | ---: | --- |
| 12040 | kernel_main | 1856 | boot_logo_rle |
| 7739 | tbx_gate_dispatch | 1045 | geneva9 |
| 6515 | int21_dispatch | 783 | chicago_pack_rows |
| 5667 | flair_live_do_surface | 768 | flair_dac |
| 4486 | paint_window_chrome | 768 | finder_small_art |
| 3508 | DrawControl | 632 | flair_part_map |
| 2900 | fat12_write_partial | 552 | FINDER_COMMANDS |
| 2418 | flair_live_tenant_sf | 528 x 7 | FLAIR_*_ICON_* strikes |

The brief's source-level candidates, measured: the boot_logo strike IS in
the loaded image (1,856 B, .rodata; the splash precedes the FAT mount, so
it cannot move to the data volume); the serial trace pack is already
indexed/deduplicated (round 4); the DOS message catalogue is not in the
flagship (`dos_messages` has no flagship symbol); there is no duplicated
glyph strike (one chicago_pack_rows, one geneva9); -fno-asynchronous-
unwind-tables / -fno-ident / --gc-sections were already in force; the
section alignment paddings total 23 B. The mass is .text, so the first
lever is codegen policy (the WL-0171 SAMIR precedent) and the second is
packed storage for the 5,320 B of strike tables (the WL-0152 chicago_pack
precedent).

## Method: one flag at a time, then the kept subset

Each row is a clean `make -B build/kernel_flairtenants.elf` with the flag
set passed through the KERNEL_R5_FLAGS seam (compile AND LTO link) in a
detached trial worktree, GCC 13.3.0, ~40 s per build (scratchpad try.sh).
Baseline 187,788 B (the WIP -flto-partition=one -fno-jump-tables already
in the tree; the Opus measurement of those two was 187,988 -> 187,212).

Round 1, each flag alone on the baseline:

| Flag | B | delta |
| --- | ---: | ---: |
| -mregparm=3 -mrtd | 183916 | -3872 |
| -mregparm=3 | 184844 | -2944 |
| -mrtd | 186604 | -1184 |
| -fno-tree-sra | 187084 | -704 |
| -fno-tree-dominator-opts | 187500 | -288 |
| -fno-shrink-wrap | 187628 | -160 |
| -fno-tree-tail-merge | 187628 | -160 |
| -fno-stack-clash-protection | 187788 | 0 |
| -fno-ipa-sra | 187852 | +64 |
| -fno-defer-pop | 187884 | +96 |
| -fno-forward-propagate | 187980 | +192 |
| -mtune=generic | 188588 | +800 |
| all eleven WL-0171 flags together | 182956 | -4832 |

Round 2, on top of -mregparm=3 -mrtd -fno-tree-sra -fno-tree-dominator-opts
-fno-shrink-wrap -fno-tree-tail-merge (182,476 B):

| Added | B | delta |
| --- | ---: | ---: |
| -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 | 181100 | -1376 |
| -fno-tree-loop-im | 182060 | -416 |
| -fno-partial-inlining | 182284 | -192 |
| --param=max-completely-peel-times=0 | 182316 | -160 |
| -fno-optimize-sibling-calls (not taken, the WL-0171 refusal: changes stack depth) | 182348 | -128 |
| -fno-code-hoisting | 182348 | -128 |
| -fno-stack-clash-protection | 182444 | -32 |
| -fno-tree-pre, -fno-defer-pop, -falign-*=1, -fno-ipa-cp-clone, crossjump params | 182476 | 0 |
| -fno-tree-slsr | 182540 | +64 |
| -fno-tree-forwprop | 182572 | +96 |
| -fno-ipa-sra | 182604 | +128 |
| -fno-forward-propagate | 182700 | +224 |
| -fno-tree-switch-conversion | 182732 | +256 |
| -fno-tree-ch | 182988 | +512 |
| -fno-inline-functions-called-once | 184012 | +1536 |

Round 3, on top of round 2 + the six keepers (180,364 B): each keeper
removed costs 96-1,312 B (sra 800, psb 1312, loop-im 384, peel/hoist/
partial 96 each; clash 0 but kept: it removes the Ubuntu default probes
nothing in the image can use); then

| Added | B | delta |
| --- | ---: | ---: |
| -fno-move-loop-invariants | 179916 | -448 |
| -fno-optimize-sibling-calls (not taken) | 180108 | -256 |
| -fno-tree-scev-cprop | 180236 | -128 |
| -mno-align-stringops, -fno-tree-loop-ivcanon | 180364 | 0 |
| -mstringop-strategy=byte_loop | 180396 | +32 |
| -fno-tree-dse | 180524 | +160 |
| -fno-tree-sink | 180620 | +256 |

## What changed, per commit (loaded flagship bytes before -> after)

| Commit | Lever | Before | After | Reclaim |
| --- | --- | ---: | ---: | ---: |
| 81ae47a8 (WIP, Opus scaffolding) | -flto-partition=one -fno-jump-tables on the LTO set + link; asset_lz packer/decoder/data, kernel_size_r5.mk, the part-map golden (all unwired) | 187988 (Opus) | 187788 (merged) | - |
| step 1 (fd7baeb6) | -mregparm=3 -mrtd for the whole image (KERNEL_ABI_FLAGS), the asm boundary pinned by KERNEL_ASM_ABI | 187788 | 183948 | 3840 |
| step 2 (12a1a191) | the pass policy (KERNEL_R5_FLAGS, twelve switches) on every kernel object + the link | 183948 | 179916 | 4032 |
| step 3 (d4ca3fe3, landed with step 4) | packed strike storage wired: the six 32x32 desk/Finder strikes, the CAUTION strike, the three 16x16 cells, the boot logo RLE stream (asset_lz.h + the committed asset_lz_data.h; test-asset-lz / -mutant) | 179916 | 177100 | 2816 |
| step 4 (d4ca3fe3) | flair_part_map rows 8 B -> 3 B + the four authored derived colours once (test-flair-part-map / -mutant) | 177100 | 176748 | 352 |
| **total** | | **187788** | **176748** | **11040** |

Window 196,608 B; r4 floor 180,224 B; headroom after = 19,860 B
(3,476 B under the floor for the seven kernel lanes still to merge).
BSS grew by 6,320 B of decode buffers (fd_strike_store 3,168, the CAUTION
record 528, the small cells 768, logo_store 1,856) plus three ready flags;
_kernel_end stays far below TRANSIENT_BASE (0x580000), which the
transient link checks on every image.

## Why each step is behaviour-neutral (Law 1 citations)

Step 1, calling convention. GCC 13 manual, x86 Options: -mregparm=NUM
passes the first NUM integer arguments in EAX/EDX/ECX; -mrtd makes the
callee pop its arguments (`ret N`); "you must provide function prototypes
for all functions that take variable numbers of arguments" -- the one
documented hazard. Audit: no varargs function in os/milton, os/flair,
os/apps/tbx or spec (grep for `...)` / va_list); a full flagship compile
under -Wstrict-prototypes -Wold-style-definition -Werror is clean (no
unprototyped declaration or definition); no memcpy/memset/memmove/memcmp
symbol exists in the image (GCC emitted no libcall, so no libcall ABI).
The asm boundaries, each read in full: kstart.asm `call kernel_main`
(void); isr.asm irq0/1/12 call pit_irq_handler / kbd_irq_handler /
mouse_irq_handler / irq_enter / irq_leave (all void), isr_selftest calls
selftest_report_c (void); isr.asm's int21_entry and its siblings
(int20/22/23/24/13/25/26) and isr_common `push` the frame pointer, `call`
the C dispatcher and `add esp, 4` -- cdecl, so those nine C functions
carry KERNEL_ASM_ABI (cdecl + regparm(0): GCC x86 Function Attributes,
"cdecl ... useful to override the effects of the -mrtd switch", "regparm
(number)") on both prototype and definition; tbx_gate.asm's
tbx_gate_entry does the same for tbx_gate_dispatch; tbx_tenant_call reads
fn/psp from [ebp+8]/[ebp+12] and returns with a bare `ret`, so its C
prototype carries the attribute too; tbx_tenant_abort is void. A probe
object (scratchpad abi.c) confirmed the attribute form: stack argument,
bare `ret`, while an unattributed function took EAX/EDX/ECX. Tenant code
is entered by `call eax` with EBX = PSP (ADR-0013 Sec 3.4, no C ABI) and
reaches the kernel only through INT 81h (args read from the trapped
frame) -- the disk tenants keep plain cdecl (TENANT_CFLAGS). The
transient COMMAND.COM (command/env/batch objects) is compiled with
KERNEL_CFLAGS and linked by address against the kernel (--just-symbols),
so both sides of command_repl(int) and the five imports share the
convention; the kernel-link ABI audit (scratchpad abi_audit.py over `make
-n -B` of all 246 kernel ELFs) found no object compiled without the
flags. CHKDSK is the one utility that links a kernel object (mcb.o) and
keeps KERNEL_CFLAGS; chkdsk_start.asm calls chkdsk_entry(void). FDISK,
FORMAT/SYS, EDLIN, XCOPY and the DOS utilities are separate images and
keep cdecl via KERNEL_CFLAGS_CDECL. test-kernel-native-boundaries still
compares like with like (both sides compile under KERNEL_CFLAGS) and
stays green: the ring publications and the loader transfer are
byte-identical to their native -O0 policy under the new convention.
Period precedent: Borland's `register` (-pr) and `pascal` conventions.

Step 2, pass policy: every switch is a GCC optimiser pass or the stack
alignment preference; none changes the ISA (-march=i386 still pins no
cmov, the WL-0087 Bochs rule), the memory layout of any object, or the
asm contracts. -mpreferred-stack-boundary=2: GCC manual "x86 Options":
the default 4 (16 B) exists for SSE spills; a 386 has none, GCC realigns
any local that declares a larger alignment, and the interrupt stubs
already enter C on a 4-byte-aligned stack (so -mincoming-stack-boundary=2
states what was always true). The gate families and the byte-identical
screendump goldens are the proof (Law 2).

Step 3, packed storage: tools/asset_lz.c (factory C, host) packs the
UNCHANGED locked spec/assets tables into os/flair/asset_lz_data.h (LZSS,
deterministic shortest-path parse, no timestamps, Rule 11); the kernel's
decoder os/flair/asset_lz.h unpacks into BSS on first use; a hosted build
reads the locked tables directly (the chicago_packed.h __STDC_HOSTED__
seam, WL-0152). test-asset-lz regenerates the committed header from the
locked headers and compares byte for byte (drift tooth), then decodes
every block with the kernel's own decoder and compares with the locked
bytes (equality tooth); test-asset-lz-mutant flips one decoded byte ->
RED. The strikes' appearance stays graded independently: test-finder-
icon-art (8 strikes, zero tolerance), test-boot-screen (the logo cell ==
strike), test-dialog, and the emu pixel gates test-flair-finder-icons /
test-flair-boot-screen / test-flair-desktop. Rule 8: no locked asset
edited.

Step 4, part map: a storage transformation of flair_look.c's table
(the 4-byte derived_rgb field of 79 rows -> an ordinal into a 4-entry
table); no 0xRRGGBB literal is introduced (ARB-5: the entries are the
canon authority's macros). test-flair-part-map prints every PART (plus
both out-of-range neighbours) at every depth and through every registry
skin and diffs against harness/proptest/fixtures/flair_part_map.golden,
captured ONCE from the 8-byte-row implementation at 4d907b1f -- a
differential against the original, not a value recomputed from the new
table (Law 2); test-flair-part-map-mutant names the wrong ordinal for the
BEVEL_SHADOW row -> RED.

## Acceptance (final tree 2de01da8, loaded 176,748 B)

Every family the brief names ran on every commit (step 1 fd7baeb6, step 2
12a1a191, steps 3+4 2de01da8), one QEMU at a time; the table is the final
run (scratchpad fam-D/*.log), plus the round's own host gates and the
mutant teeth. The screendump goldens are the pixel gates' own fixtures:
unchanged, and every pixel gate is green against them.

| Target | Verdict | Tail |
| --- | --- | --- |
| image | GREEN | >>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT) |
| test-kernel-headroom | GREEN | >>> test-kernel-headroom: green |
| test-kernel-size-r4 | GREEN | >>> test-kernel-size-r4: green (flagship keeps >= 16 KiB headroom in the 384-sector window) |
| test-kernel-native-boundaries | GREEN | >>> test-kernel-native-boundaries: green (rings, keyboard IRQ and loader transfer byte-identical to native -O0) |
| test-serial-trace-pack | GREEN | >>> test-serial-trace-pack: green (original C calls == indexed packed trace bytes) |
| test-chicago-pack | GREEN | >>> test-chicago-pack: green (all 1568 decoded rows == unchanged locked strike) |
| test-makefile-vars | GREEN | >>> test-makefile-vars: green |
| test-boot | GREEN | >>> test-boot [4/4]: screendump banner-text check |
| test-shell | GREEN | >>> test-shell [7/7]: screendump shows the "A:\>" COMMAND.COM prompt rendered on the seafoam desktop |
| test-int21 | GREEN | >>> test-int21: green |
| test-dos-volumes | GREEN | GREEN test-dos-volumes emu: real FAT/ATA, mtype bytes, mdir both images, fsck.fat clean |
| test-flair-desktop | GREEN | >>> test-flair-desktop [+]: HER-02 boundary -- structure value-blind (color VALUES graded by test-color-canon, not by co |
| test-flair-appswitch | GREEN | >>> test-flair-appswitch [4/4]: grader PASS (DISTINCT-CHIMERA + POST-DISTINCT + TIER-A overlap + TIER-B accent + BAR1-ST |
| test-flair-finder-icons | GREEN | >>> test-flair-finder-icons: green (7 cells: VOLUME sel, TRASH, DOC sel, FOLDER, 2 x APP, DOC -- 0/1024 differing each) |
| test-flair-list-view | GREEN | >>> test-flair-list-view: green (Name/Size header cells, sort shading, Restart kind=4) |
| test-flair-boot-screen | GREEN | >>> test-flair-boot-screen: green (live splash == Mac OS 8.1 Welcome panel cell for cell + the Initech strike, 32bpp AND |
| test-samir-lifetime-desktop | GREEN | >>> test-samir-lifetime-desktop: green |
| test-asset-lz | GREEN | >>> test-asset-lz: green (4 packed blocks decode == the unchanged locked strikes) |
| test-asset-lz-mutant | GREEN | >>> test-asset-lz-mutant: green (asset_lz_byte RED) |
| test-flair-part-map | GREEN | >>> test-flair-part-map: green (405 PART x depth x skin resolutions == the pre-change table) |
| test-flair-part-map-mutant | GREEN | >>> test-flair-part-map-mutant: green (part_map_derived RED) |
| test-finder-icon-art | GREEN | >>> test-finder-icon-art: green (8 strikes, every state captured, 0 differing cells, zero tolerance) |
| test-boot-screen | GREEN | >>> test-boot-screen: green (host render == s8_startup_welcome.png furniture; logo cell == strike) |
| test-dialog | GREEN | >>> test-dialog: green |
| test-dialog-mutant | GREEN | >>> test-dialog-mutant: green (DIALOG_MUT_ALERT_SLAB correctly RED: ALERT-BANDS) |
| test-flair-trash-full-mutant | GREEN | >>> DIALOG_MUT_NO_ICON: RED -- the CAUTION cell 572/1024 (every opaque cell missing), the FULL Trash still 0/1024 |
| test-kernel-size-r4-mutant | GREEN | >>> test-kernel-size-r4-mutant: green (size_pad_one RED) |
| test-kernel-native-boundaries-mutant | GREEN | >>> test-kernel-native-boundaries-mutant: green (whole_tu_oz RED) |

Reproducible builds (Rule 11): two clean `make image` runs (build/*.o,
*.elf, *.bin removed between them) produced byte-identical flagships and
images:

    b9af28b66c29104e8929bd3c04c9f318734650955c095efe0d8bdc948140450a  kernel_flairtenants (objcopy -O binary, 176748 B)
    d4400f583b0ffd9c5fcc4c27a16ec2c50dca487be13d5394b75cb02777151d9e  build/initech.img

Not run: `make test` (brief), the Bochs legs (none named; the ISA pin is
unchanged, -march=i386 throughout), 86Box (not built).

## Frictions

- The step-3 helper dlg_caution_strike was unused under the
  DIALOG_MUT_NO_ICON mutant object (the kernel-headroom gate builds every
  mutant kernel): -Werror=unused-function. The helper is compiled out with
  the mutant; the emu mutant test-flair-trash-full-mutant still goes RED
  for its named reason (the CAUTION cell 572/1024 missing). Amended into
  the steps-3+4 commit before the family run that accepted it.
- test-kernel-headroom relinks ~245 kernel variants single-threaded after
  every clean rebuild (~15 min per family run); three family runs were
  needed (steps 1, 2, 3+4).
- `tail --pid` on a pid that has already exited returns at once -- the
  first waits on the steps-3+4 family run returned early; harmless.
- The decoder is `static noinline` per TU; LTO folded the three
  identical bodies to one address but kept a constprop copy (82 B) for
  the boot-screen call -- a shared extern decoder would save it; not
  worth a fourth family run tonight.
- -fno-optimize-sibling-calls (-256 B) is again left on the table, on
  the WL-0171 principle (changes stack depth), not measurement.
- lane worktree .beads is 0775 (bd warning on every commit); not touched.
- The r4 gate was green by 308 B after step 2 alone; the storage steps
  are what buy the margin for the seven kernel lanes still to merge.

## Pointers

- Makefile: KERNEL_ABI_FLAGS / KERNEL_CFLAGS_CDECL (step 1),
  KERNEL_R5_FLAGS (step 2), `include harness/proptest/kernel_size_r5.mk`
  (steps 3+4); harness/diff/fat_diff/{fdisk,formatsys,xcopy_tree,dosutil}.mk
  and harness/edlin/edlin.mk take KERNEL_CFLAGS_CDECL.
- os/milton/idt.h KERNEL_ASM_ABI (+ the guarded copy in os/flair/tbxgate.h);
  os/milton/{int21.h,int21.c,block_device.h,block_device.c,panic.c},
  os/flair/tbxgate.c: the attributed boundary functions.
- tools/asset_lz.c, os/flair/asset_lz.h, os/flair/asset_lz_data.h
  (generated; `make gen-asset-lz` after a locked-strike edit),
  harness/proptest/test_asset_lz.c; the readers in os/flair/finder_desktop.c,
  os/flair/dialog.c, os/flair/finder_icon.c, os/milton/boot_screen.h.
- os/flair/flair_look.c flair_derived_rgb[] / FLAIR_DERIVED_*;
  harness/proptest/test_flair_part_map.c + fixtures/flair_part_map.golden.
- harness/proptest/kernel_size_r4.mk: the floor, unchanged.
- docs/worklog/WL-0152-kernel-size-round4.md (the tools and the floor
  redefinition), docs/worklog/WL-0171-samir-size-round-2.md (the policy
  this round ports).
- Scratchpad (session-local): trials1/2/3.txt, abi_audit.py, try.sh,
  families.sh, fam-A/B/C/D logs.

## Appendix: sorted symbol table of the merged tree (every symbol > 256 B, by section)

## .data: 0 symbols > 256 B, total 0 B

| Bytes | Symbol |
| ---: | --- |

## .rodata: 15 symbols > 256 B, total 10388 B

| Bytes | Symbol |
| ---: | --- |
| 1856 | `boot_logo_rle` |
| 1045 | `geneva9` |
| 783 | `chicago_pack_rows` |
| 768 | `flair_dac` |
| 768 | `finder_small_art` |
| 632 | `flair_part_map` |
| 552 | `FINDER_COMMANDS` |
| 528 | `FLAIR_FINDER_ICON_FOLDER` |
| 528 | `FLAIR_FINDER_ICON_DOC` |
| 528 | `FLAIR_FINDER_ICON_APP` |
| 528 | `FLAIR_DESK_ICON_VOLUME` |
| 528 | `FLAIR_DESK_ICON_TRASH_FULL` |
| 528 | `FLAIR_DESK_ICON_TRASH` |
| 528 | `FLAIR_ALERT_ICON_CAUTION` |
| 288 | `flair_skin_registry` |

## .text: 128 symbols > 256 B, total 111557 B

| Bytes | Symbol |
| ---: | --- |
| 12040 | `kernel_main` |
| 7739 | `tbx_gate_dispatch` |
| 6515 | `int21_dispatch` |
| 5667 | `flair_live_do_surface` |
| 4486 | `paint_window_chrome` |
| 3508 | `DrawControl` |
| 2900 | `fat12_write_partial` |
| 2418 | `flair_live_tenant_sf` |
| 1923 | `flair_draw_menu_panel.constprop.0` |
| 1844 | `flair_desktop_run` |
| 1565 | `fw_paint_list` |
| 1548 | `DrawMenuBar` |
| 1535 | `do_fcb` |
| 1359 | `flair_tenant_mount_volume` |
| 1294 | `flair_live_tenant_filecopy` |
| 1253 | `finder_fat_move` |
| 1138 | `DrawDialog` |
| 1065 | `fat12_mkdir` |
| 968 | `draw_titlebar_band` |
| 947 | `DrawControlDefaultRing.part.0` |
| 935 | `flair_exit_to_dos` |
| 924 | `v_dialog` |
| 900 | `finder_win_open.constprop.0` |
| 888 | `isr_dispatch_c` |
| 865 | `sf_draw_dynamic.constprop.0` |
| 826 | `fat12_write_file` |
| 791 | `flair_live_do_menu_at` |
| 777 | `conin_cooked_line.constprop.0` |
| 759 | `finder_shell_empty_trash` |
| 756 | `int13_dispatch` |
| 729 | `flair_live_tenant_service` |
| 728 | `draw_platinum_widget` |
| 726 | `text_draw_run` |
| 691 | `finder_desk_persist` |
| 671 | `finder_win_sync_geometry` |
| 658 | `finder_report_outcome` |
| 649 | `fat12_create` |
| 647 | `finder_shutdown_screen` |
| 643 | `fat12_read_partial` |
| 635 | `flair_live_alert_open_icon.constprop.0` |
| 635 | `cook_raw` |
| 622 | `loader_run_plan` |
| 614 | `structure_scratch` |
| 603 | `flair_live_track_box` |
| 601 | `tbxf_call` |
| 590 | `region_normalize` |
| 576 | `fat12_read_file` |
| 574 | `fat12_resolve_path_from.constprop.0` |
| 561 | `mount_primary` |
| 560 | `fat12_grow_dir.constprop.0` |
| 551 | `finder_desk_load_positions` |
| 534 | `do_open` |
| 533 | `fat_chmod` |
| 530 | `icon_draw` |
| 516 | `FindWindow.constprop.0` |
| 514 | `finder_desk_paint` |
| 508 | `fw_sync_list` |
| 500 | `fat_resolve_dir` |
| 495 | `region_merge` |
| 495 | `finder_fat_trash_name` |
| 492 | `open_common` |
| 482 | `fat12_scan_dir.constprop.0` |
| 474 | `loader_load_tenant` |
| 468 | `load_program_from_fat` |
| 462 | `fat_rename` |
| 451 | `console_putc` |
| 450 | `set_content` |
| 447 | `set_document_frame` |
| 446 | `fat12_read_dir` |
| 437 | `region_from_rects` |
| 436 | `finder_dispatch.constprop.0` |
| 434 | `fat12_rmdir` |
| 431 | `block_device_map` |
| 429 | `draw_arrow_tile` |
| 423 | `rtc_decode` |
| 422 | `mz_parse_header` |
| 420 | `shell_render` |
| 402 | `FlairProcess_launch` |
| 401 | `fcb_xfer` |
| 399 | `cursor_draw` |
| 396 | `WindowScrollFindPart` |
| 396 | `loader_prepare_tenant` |
| 389 | `do_getdpb` |
| 378 | `psp_parse_fcb` |
| 377 | `NewWindow` |
| 375 | `finder_ops_resolve` |
| 367 | `finder_power_ask_apps` |
| 364 | `flair_live_erase_menu_panel` |
| 351 | `DialogHandleEvent.part.0.constprop.0` |
| 349 | `flair_route_updates` |
| 349 | `fat12_unlink` |
| 341 | `find_scan` |
| 339 | `console_cursor_wait` |
| 333 | `finder_win_populate` |
| 333 | `rtc_encode` |
| 326 | `dev_clk_read_adapter` |
| 322 | `flair_desktop_present` |
| 306 | `NewDialog.constprop.0` |
| 300 | `sf_draw_static.constprop.0` |
| 300 | `fat12_dir_live` |
| 298 | `flair_live_outline_walk.constprop.0` |
| 298 | `event_common` |
| 294 | `ata_write_sectors` |
| 293 | `desktop_paint_damage` |
| 292 | `fat12_next_cluster` |
| 289 | `tbx_reap` |
| 288 | `desktop_fill` |
| 287 | `WindowScrollBand` |
| 283 | `dev_aux_handler` |
| 281 | `finder_desk_text` |
| 280 | `fat12_read_fat_sector` |
| 280 | `dev_route_rw.constprop.0` |
| 278 | `console_init.part.0` |
| 275 | `psp_build` |
| 272 | `finder_zoom_back` |
| 272 | `advance_line` |
| 270 | `sf_rebuild_list.constprop.0` |
| 270 | `draw_sb_thumb` |
| 269 | `switch_foreground` |
| 269 | `dev_open_lookup` |
| 267 | `MenuInfo_panel_rect` |
| 266 | `tbx_launch.isra.0` |
| 266 | `blitter_fill_rect_clipped` |
| 264 | `kbd_irq_handler` |
| 262 | `stamp_entry` |
| 261 | `loader_prepare_mz` |
| 257 | `sft_write` |
| 257 | `fat12_subdir_slot_lba` |
