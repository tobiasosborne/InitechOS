# WL-0128 -- Kernel size round 3 part 2: cold optimisation

Lane: initech-tode, continuation of WL-0127. Changes left uncommitted for PM
review. No beads writes, git mutation, full make test, or Bochs run.
`bd show initech-tode` returned "no issue found matching initech-tode".

## Ground truth and scope

Read CLAUDE.md top to bottom, PRD Sec 5, 6.1-6.5, 7-8, the bead lookup,
WL-0127, the HEAD Makefile/kernel.ld diff and the round-2 size policy.
PRD Sec 5 and ADR-0003 DEC-08 specify the flat kernel; ADR-0003 Appendix A
defines the unchanged DOS API. The IBM DOS 3.30 Technical Reference scans
were rendered with pdftoppm at 110 dpi and inspected:

- Printed 6-67 / PDF 190: AH=0Fh FCB open, default-drive resolution, current
  block zero and default record size 80h.
- Printed 6-200 / PDF 323: AH=4Ch return code, return to parent, close files.

Reference file: ../initech-manuals-ref/dos33/
80X0945_DOS_3.30_Technical_Reference_Apr87.pdf. Existing fcb_tps.mk uses an
independent real MS-DOS 6.22 transcript (not a new DOS 3.3 golden), mtools
names/attributes/sizes and file-byte comparisons on the guest-written image.
The InitechText gate independently compares saved bytes using mtools, runs
fsck.fat and grades pixels. No DOS behaviour, diagnostic text, artifact source
logic, fonts, boot geometry or safety floor changed.

The PM's exact trace re-key was applied to BOTH FLAIR_ITEXT_SPEC and
FLAIR_ITEXT_EDIT_SPEC. TEXT.EXE is fixture entry 13 / cell 12, centre (75,278);
the added return moves restore (143,174) before the original continuation.
Comments now agree with harness/fixture_volumes.mk. The arrival-flags text
gate passed after this authorized re-key, before compiler changes.

## Measured levers

Every row ran make test-kernel-headroom over all 179 kernel targets, with the
unchanged 196608-byte window and 4096-byte floor. Each -Oz file was applied
and measured separately, in the order below. Loaded bytes exclude NOBITS BSS.

| Lever | Flagship loaded / headroom B | Binding loaded / headroom B | Binding gain B |
| --- | --- | --- | --- |
| Arrival, part 1 committed | 181644 / 14964 | 182732 / 13876 | - |
| finder_ops -Oz | 181612 / 14996 | 182700 / 13908 | 32 |
| dialog -Oz | 181580 / 15028 | 182668 / 13940 | 32 |
| stdfile -Oz | 181548 / 15060 | 182636 / 13972 | 32 |
| list -Oz | 181516 / 15092 | 182604 / 14004 | 32 |
| int21 -Oz | 181228 / 15380 | 182316 / 14292 | 288 |
| config_sys -Oz | 181228 / 15380 | 182316 / 14292 | 0 |
| Selected cold TUs -flto -Oz | 170820 / 25788 | 171588 / 25020 | 10728 |

Net binding gain: 11144 B, exceeding the 10240-byte target by 904 B.
Binding moved from kernel_flairtenants_mut_no_raise_on_title to
kernel_flairtenants_db_write_skip. Flagship gain: 10824 B.

The six ordinary flag probes recovered only 416 B. Cold cross-TU optimisation
is the additional lever: INT 21h/FAT and pure storage/configuration services,
Toolbox managers, Finder, tenant orchestration and platform-service models.
The GCC driver now performs kernel-only links so the LTO plugin runs; every
kernel/mutant link recipe uses KERNEL_LINK, including included .mk recipes.
The process mutants use the same tenants optimisation policy as process.o.
Per-TU alias policies, -march=i386 and all FLAIR serial markers are retained.
TENANT_CFLAGS remains separate. Native mcb.o is shared with CHKDSK, so it is
excluded from LTO. Native loader, event ring, hardware/IRQ drivers, blitter,
ATKINSON and text drawing are also excluded. The three hot object files and
MBR/stage2 compared byte-identical to saved arrival copies.

nm --size-sort -S and size -A were inspected before/after. Final flagship:
.text 150729, .rodata 19236, .data 804, .bss 54136. The loaded image is 170820
bytes including alignment. The large font strikes occur once each; command.o
is absent from the flagship link. Icon tables already use packed strikes.
The generated DOS message header contains macros rather than a second
allocated string table. No font, icon, table schema, panic text or serial
marker was removed. Scratch --sort-section=alignment saved only 56 B
(170764 B flagship); it was not adopted. Broader no-inline probes grew several
objects; no FCB source refactor or constant-address merging was needed.

## Oracles and mutation evidence

New test-kernel-size-r3 failed before compiler changes:

    !!! kernel-size-r3 FAIL: build/kernel_alert_chrome_fringe.elf loaded 181904 B > 172492 B (10 KiB reclaim required)

Its fixed bound is the independently recorded arrival binding 182732 minus
10240, not a value derived from the optimised kernel. All enumerated kernels
are graded via objcopy raw loaded bytes. size_pad_one adds rooted data to the
real flagship and proves rejection exactly one byte past that bound:

    !!! kernel-size-r3 FAIL: build/kernel_gc/r3-pad.elf loaded 172493 B > 172492 B (10 KiB reclaim required)

New test-kernel-cold-lto failed before enabling LTO; the independently fixed
answer 22 must replace a cross-TU call, with entry still at 0x00500000. Its
no_lto mutant emits the same RED:

    !!! kernel-cold-lto FAIL: cross-object callee retained

Both gates and both mutants are in TEST_UNIT_GATES. New emulator wrappers
test-kernel-size-r3-emu and test-kernel-size-r3-emu-mutant are in TEST_EMU_GATES;
they reuse independent injected desktop/FCB/text traces and their graders.
Normal boot, app-switch pixels, FCB transcript/FAT and InitechText passed.
The five desktop and three FCB mutants were all rejected:

- ignore_refcon: FLAIR-DISPATCH app=NOTES never fired.
- skip_activate: FAIL TIER-B: POST NOTES accent block (centre) is the active accent.
- drop_update: FAIL TIER-A: POST overlap probe is NOTES_FILL.
- no_menubar_swap: FAIL MENU-BAND, 0 differing pixels.
- notes_bar_sys: FAIL POST-DISTINCT, 0 differing pixels.
- seq: T03 SEQWRITE 00 00 00 BLK=0000 REC=03 SIZE=00000030.
- noattr: T14 HIDDEN 00 00 FF FF 00 X=FF0201 FCBHID DAT A=22.
- nozero: T09 PARTIAL 03 SUM=15B4 B75=4B B76=00 BLK=0000 REC=01.

Part-1 GC, roots and no_gc/no_sections mutations remain green; each GC mutant
emits "kernel-gc FAIL: dead gc_dead_function retained". The explicitly rooted
headroom pad goes RED at 4095 B and GREEN at 4096 B. test-makefile-vars passed.
The DOS kernel clean rebuild and byte-change mutant passed (SHA256
9f29274af0ee2efcb0041eb8f48d96c469f23ae9be54e5f3cb52a6ade42bab10).

All QEMU runs export INITECH_QMP_STDIO=1 and run sequentially. free -g and
df -h / were checked before heavy runs (25 GB available RAM, 129 GB free
disk at the last check). Evidence is under build/ksize-p2/, including
oz-*.log, lto-headroom.log, lto-fixture-green.log, red.log, lto-red.log,
test-*.log and final-*.log; direct mutant output is in build/kernel_gc/.

## Final acceptance

All 19 requested targets and make image passed, in the requested order (image
was built after flagship reproducibility). The final headroom run again
measured 179 kernels: flagship 170820 / 25788 B, binding db_write_skip
171588 / 25020 B. The flagship gate removed and rebuilt ALL flagship objects
for EACH of two passes; the complete padded binaries are byte-identical:

    >>> test-kernel-flair-repro: green (two clean builds byte-identical; sha256 f7d9d197ad9a222a45637c26e8dc7c80b67bcb93c311fc75b5571c7e8be31496)

Final gate tails (full output: build/ksize-p2/final-<target>.log):

| Target | GREEN / PASS tail |
| --- | --- |
| test-kernel-headroom | test-kernel-headroom: green |
| test-kernel-flair-repro | two clean builds byte-identical, SHA256 above |
| image | image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS) |
| test-boot | PASS -- InitechDOS banner printed (serial == spec, rendered on screen), no triple-fault |
| test-shell | PASS -- booted InitechDOS, got an A:\> prompt, ran DIR/TYPE/a program/EXIT via COMMAND.COM |
| test-command | test_command: 182 checks, 0 failures; test-command: green |
| test-fcb-api | PASS -- the FCB API agrees with real DOS |
| test-tps-report | PASS -- the TPS Report Generator works from the prompt through the FCB API |
| test-flair-desktop | PASS -- the LIVE FLAIR chimera desktop is the Office Space frame |
| test-flair-appswitch | the booted FLAIR App Contract honours O-5 (ADR-0013) |
| test-flair-app-launch | test-flair-app-launch: green (all 10 legs) |
| test-flair-itext | test-flair-itext: green; MEMO.TXT 43 B, mtools/fsck.fat and independent pixels |
| test-flair-calc | green (disk launch, two windows, PRD text, historical chrome pixels, edit, quit) |
| test-flair-iword | test-flair-iword: green; MEMO 404 B, round-trip and screen checks |
| test-flair-i123 | test-flair-i123: green; quit frame byte-identical to never-launched baseline |
| test-flair-finder-service | test-flair-finder-service: green (both clean and damaged volumes) |
| test-flair-locked-purge | PASS -- locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat |
| test-flair-shutdown | test-flair-shutdown: green; DESKTOP-DB-SAVE n=4, FLAIR-SHUTDOWN-SAFE |
| test-flair-fonts | test-flair-fonts: green |
| test-flair-menubar-clock | green (pinned RTC crossed a minute, exactly two paints, independent1600-pixel golden) |

All six added targets passed:

    >>> test-kernel-size-r3: green (all kernels reclaim >= 10240 B against arrival binding)
    >>> test-kernel-size-r3-mutant: green (size_pad_one RED)
    >>> test-kernel-size-r3-emu: green (desktop pixels, text trace, DOS/FCB/FAT differential)
    >>> test-kernel-size-r3-emu-mutant: green (desktop/FCB behaviour mutants RED)
    >>> test-kernel-cold-lto: green (cross-TU call folded to 22; entry fixed)
    >>> test-kernel-cold-lto-mutant: green (no_lto RED)

After the clean rebuilds, blitter.o, region.o, text.o, mbr.bin and stage2.bin
again compared identical to the arrival copies. Source diff whitespace and
ASCII checks passed. No requested acceptance remains. Bochs/86Box, full make
test, commits/pushes and beads writes were not performed, per lane scope.
