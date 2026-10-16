# WL-0124 -- Kernel size round 3: GC reclaim, stopped at arrival fixture failure

Lane: initech-tode. Partial result only; the requested 20 KB recovery and
complete regression acceptance were not achieved. No commit or beads edit.

## Context and sources

Read CLAUDE.md in full, PRD Sec 5, 6.1-6.5, 7, 8, WL-0091, WL-0104, and
docs/design/kernel-size-policy-round2.md. `bd show initech-tode` and
`bd show initech-8z9j` fail with "no beads database found" in this clone;
the checked-in .beads/issues.jsonl supplies 8z9j/opms/m0dc but has no tode row.
Beads were not initialized or edited.

Law 1: round2 Sec 4 lever C specifies kernel-only section GC and the symbol
audit. PRD Sec 5 and ADR-0003 DEC-08 specify a flat kernel at the boot handoff;
ADR-0003 Appendix A remains the DOS API contract. The local IBM DOS 3.30
Technical Reference, printed 6-200 / PDF page 323, was rendered at 110 dpi
and viewed: AH=4Ch returns to the invoking process and closes its files.
The existing FCB gate independently checks the DOS transcript, mtools FAT
listing/attributes/sizes, and surviving file bytes (its recorded real-DOS
golden is MS-DOS 6.22; this is not a newly minted DOS 3.3 golden).

## Implemented lever

Kernel C recipes use separate KERNEL_SECTION_FLAGS, leaving the inherited
TENANT_CFLAGS base intact. Kernel links use --gc-sections. kernel.ld keeps
the entry first, complete assembly ISR stubs, exception pointer table, IDT
and IDT descriptor. The kernel headroom pad mutant now names its test blob
and roots it explicitly with --undefined; otherwise GC would remove the
test's intentionally unused bytes.

The Makefile/harness audit found kernel placement lookups for _kernel_end
and defined-symbol minima; these remain. Other executable nm lookups grade
tenant/seed objects. The QEMU harness offers a GDB stub but does not drive
symbol probes. Assembly interrupt and Toolbox references use relocations.

All 179 kernels were measured with the unchanged 196608-byte window and
4096-byte floor:

| Lever | Flagship loaded / headroom | Binding mutant loaded / headroom |
| --- | --- | --- |
| Arrival (RED) | 191912 / 4696 | 193032 / 3576 |
| Kernel-only sections + GC (GREEN) | 181644 / 14964 | 182732 / 13876 |

Flagship reclaim: 10268 B. Binding reclaim: 10300 B. Binding remains
kernel_flairtenants_mut_no_raise_on_title. Large scrap storage is allocated
from the heap (FLAIR-SCRAP-READY bytes=32808), not 32 KB of loaded kernel.
The linked font strikes already have one definition each. No artifact source
behaviour, optimisation level, boot source, window, or floor was changed.

Exploratory scratch measurements only (not implemented or accepted): -Oz
on the measured optimised objects saves about 3 KB of object sections;
no-inline often grows them. A partial LTO pilot on the optimised objects
plus text.c loads a flagship of 172808 B at -Os, or 170568 B at -Oz. Those
pilots have no binding-mutant measurement or regression certificate. They
are in build/ksize-{oz,noinline,stack4,lto,lto-oz}; the loader, event ring,
and drivers were not selected for LTO. Pilot oracle files were removed
when this lane stopped; no incomplete LTO gate is in the test vectors.

## RED and GREEN evidence

New test-kernel-gc first failed with:

    !!! kernel-gc FAIL: dead gc_dead_function retained

New gates are included in TEST_UNIT_GATES / TEST_EMU_GATES:

    >>> test-kernel-gc: green (entry/live code/data retained; dead code/data removed)
    >>> test-kernel-gc-mutant: green (no_gc/no_sections both RED for dead code)
    >>> test-kernel-gc-roots: green (entry, IDT, all exception/IRQ/DOS/Toolbox stubs retained)
    >>> test-kernel-gc-emu: green (boot/desktop pixels/FCB transcript and FAT differential)
    >>> test-kernel-gc-emu-mutant: green (desktop and FCB behaviour mutants rejected)
    >>> test-kernel-flair-repro: green (two clean builds byte-identical; sha256 af7e3a53db4a37e33dbea0066a5fb4680ada8bf5e1ce9a8d35992859fdd89fcb)

Named new mutants no_gc and no_sections both emit:

    !!! kernel-gc FAIL: dead gc_dead_function retained

Existing headroom boundary mutant, with its explicit GC root:

    !!! khr-mutant FAIL: largest kernel khr_mut_pad.elf leaves 4095 B < KERNEL_HEADROOM_FLOOR 4096 B -- a size-policy decision is due (bead initech-8z9j) before any slice grows the kernel
    >>> test-kernel-headroom-mutant: green (floor-1 pad kernel RED, floor pad kernel GREEN)

The new emulator wrapper runs the existing mutation-proven gates. Actual REDs:

    ignore_refcon: FLAIR-DISPATCH app=NOTES never fired (no marker-gated dump)
    skip_activate: ppm_flair_appswitch_check: FAIL TIER-B: POST NOTES accent block (centre) is the active accent [POST]
    drop_update: ppm_flair_appswitch_check: FAIL TIER-A: POST overlap probe is NOTES_FILL (NOTES raised + updateEvt-repainted) [POST]
    no_menubar_swap: ppm_flair_appswitch_check: FAIL MENU-BAND (0 differing pixels)
    notes_bar_sys: ppm_flair_appswitch_check: FAIL POST-DISTINCT (0 differing pixels)
    seq: < T03 SEQWRITE 00 00 00 BLK=0000 REC=03 SIZE=00000030
    noattr: < T14 HIDDEN 00 00 FF FF 00 X=FF0201 FCBHID  DAT A=22
    nozero: < T09 PARTIAL 03 SUM=15B4 B75=4B B76=00 BLK=0000 REC=01

Other GREEN runs: make image; test-flair-desktop; test-flair-appswitch;
test-flair-app-launch (all 10 legs); test-boot; test-fcb-api;
test-kernel-headroom (179 kernels); test-kernel-headroom-mutant;
test-makefile-vars; test-kernel-repro (DOS kernel sha256
af8bbdf5202a464b5e169092071916a2c9648e5a310f9b7bbe36c18bce3357ac).
The flagship gate actually removes all flagship objects before EACH of two
builds and compares the complete padded binaries. Boot binaries compared
identical before/after the GC regression runs.

## Stop: pre-existing locked InitechText trace misses TEXT

The sequential requested-family run stopped at test-flair-itext:

    !!! test-flair-itext FAIL [1/5] UP: the scrap / TEXT did not come up, or the desktop died

spec/flair_itext_traces.mk says TEXT.EXE is APPS slot 5 and double-clicks
(143,174). The actual shipped directory puts CHKDSK.COM at slot 5 and
TEXT.EXE at slot 12. The guest serial proves the wrong selection:

    FINDER-WIN-SELECT win=1 name=CHKDSK.COM count=1
    FINDER-OPEN-ITEM NYI name=CHKDSK.COM

This is independently reproduced with the arrival flags (no function/data
sections, no GC, original linker layout reconstructed in build/ksize-arrival.ld):

    INITECH_QMP_STDIO=1 make BUILD=build/ksize-arrival KERNEL_LD=build/ksize-arrival.ld KERNEL_SECTION_FLAGS= KERNEL_GC_LDFLAGS= FLAIR_DATA_IMG=build/flair_data.img test-flair-itext

It fails identically and records the same CHKDSK selection. The locked trace
and artifact behaviour were not changed to get past it. Stopped per the lane's
explicit blocker instruction; the 20 KB target is unfinished. The later
requested Calc, IWord, I123, Finder service, locked purge, shutdown, fonts,
menubar clock, shell and command gates were not run. No full make test or
Bochs run was attempted. QEMU runs all exported INITECH_QMP_STDIO=1.

Evidence: build/ksize-baseline.log, ksize-gc-headroom.log,
ksize-gc-{red,green,emu}.log, ksize-gc-test-flair-*.log,
ksize-arrival-itext.log, ksize-arrival/flair_itext.serial,
ksize-partial-final-{host,emu-mutant,other}.log, kernel-flair-repro-{1,2}.bin.
