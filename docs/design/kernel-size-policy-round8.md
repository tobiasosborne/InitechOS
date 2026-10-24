# Kernel Size Policy, Round 8

Bead: initech-hztg. Base: 2808f8b5. Branch: lane/ksize.
Status: PARTIAL; the requested r3/r4 green round has not been achieved.

## Measurement

Ref: round-2 policy Sec 4 lever C; PRD Sec 5/6.1/6.3/7/8;
CODEX-OUT.md contains the full per-object/symbol measurements and trial tables.
All arrival measurements preceded artifact/policy edits. GCC 13.3.0,
binutils 2.42, NASM 2.16.01. Loaded means objcopy -O binary, excluding BSS.

| Tree / trial | Flagship loaded B | Flagship headroom B | Binding loaded B | Binding headroom B |
| --- | ---: | ---: | ---: | ---: |
| Rebuilt round 7, e3e5277d | 176756 | 19852 | - | - |
| Arrival, all 309 kernels | 183956 | 12652 | 185292 | 11316 |
| LTO sections, isolated link measurement | 183700 | 12908 | - | - |
| Final scan, all 309 kernels | 183700 | 12908 | 185068 | 11540 |

The arrival binding image is kernel_flairtenants_mut_itext_g12row. Growth
since round 7 is 7200 B: text +4912 B, rodata +2290 B, data unchanged,
plus a -2 B net change in section padding. Important stable-symbol growth:
tbx_gate_dispatch +841 B, new finder_shell_delete_item 623 B, new
panic_screen_lines 622 B, flair_live_do_surface +583 B. The new packed
Geneva-12 data/metrics contribute 1462 B of named rodata. Native diagnostic
object growth includes discarded raw font rows and is not additive to LTO.

## Lever taken

Keep -ffunction-sections/-fdata-sections at final LTO codegen. The existing
kernel-only reachability policy already uses those flags on C compilation
and --gc-sections at link. They were missing on the LTO codegen invocation.
A native caller in a discarded section can nevertheless make the plugin emit
an LTO callee; a monolithic emitted text section then keeps that callee alive.
The new native-reference fixture makes this failure observable independently
of the flag values. Without the fix:

    !!! kernel-gc FAIL: dead gc_dead_function retained

With final LTO sections, GC removes the dead callee and KEEP(.text._start)
places the C fixture entry at KERNEL_BASE. A no_lto_sections mutant restores
the original failure. The ordinary raw-ld GC oracle remains in place, and
test-kernel-gc now also exercises the actual LTO link driver.

The r3 cap+1 mutant now retains the real flagship's transient imports, as
the r4 mutant already did. Dropping those roots would test a smaller image
without required services. The cap, exact cap+1 check and rejection stay fixed.

## Levers parked

The existing cold LTO set already covers nearly every large safe manager.
The clean cold-set -Os, reduced-inlining, frame-pointer, IPA and pass-policy
trials mostly grew the image; CODEX-OUT.md records the byte counts. No hot
blitter/region/text, IRQ/port driver or loader/event boundary was reflagged.
No behavior, ABI, ISA, boot source, geometry, spec or cap was changed.

-malign-data=abi on the cold compile set saved 740 B in isolation but was not
selected; it is far short of the required recovery and would broaden the
changed compile-flag families. SUBALIGN(4) in the linker script saved 1144 B
with LTO sections but was rejected because it overrides alignment promised
by input objects without changing the compiler's assumptions. Link-only
alignment flags did not recover extra bytes.

No payload-compression mechanism was implemented. Kernel startup/link-layout
changes require a scope decision. No claim is made that all possible source
refactors have been exhausted.

## Acceptance

The final command outputs are recorded in CODEX-OUT.md. The round stays open
until the unchanged r3 all-kernel cap and r4 flagship headroom gate are green.
Bochs legs belong to the PM after merge, per LANE-BRIEF.md; none is run here.

Final output from this worktree (complete command-by-command output is in
CODEX-OUT.md):

```text
>>> test-kernel-headroom: 309 kernels measured; window 196608 B (384 sectors); floor 4096 B
    flagship kernel_flairtenants.elf                              183700 B loaded, headroom  12908 B
    LARGEST  kernel_flairtenants_mut_itext_g12row.elf             185068 B loaded, headroom  11540 B (binding)
>>> test-kernel-headroom: green
>>> test-kernel-headroom-mutant: green (floor-1 pad kernel RED, floor pad kernel GREEN)
!!! kernel-size-r3 FAIL: build/kernel_alert_chrome_finder_press.elf loaded 183180 B > 172492 B (10 KiB reclaim required)
!!! kernel-size-r4 FAIL: loaded 183700 B > 180224 B (the flagship must keep 16 KiB headroom -- size round 5 is due)
>>> test-kernel-gc: green (entry/live code/data retained; dead code/data removed)
>>> test-kernel-gc-roots: green (entry, IDT, all exception/IRQ/DOS/Toolbox stubs retained)
>>> test-kernel-cold-lto: green (cross-TU call folded to 22; entry fixed)
>>> test-kernel-cold-lto-mutant: green (no_lto RED)
>>> test-kernel-gc-lto-mutant: green (no_lto_sections RED)
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 50e5a1c4d68aeac39f51137a18bd7173a0f5c0353b1a920bebc48e1acb1942bc)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 2575e0c9ad4ef0ca24482659e62e7fa57dff32dd1d8a4b3bd9495cc6fdacc83c)
>>> test-kernel-size-r3-emu: green (desktop pixels, text trace, DOS/FCB/FAT differential)
>>> test-kernel-size-r3-emu-mutant: green (desktop/FCB behaviour mutants RED)
```

Desktop, desktop-mutant, appswitch, appswitch-mutant, exec, itext and boot
also passed in QEMU; their exact verdicts are in CODEX-OUT.md. The native
boundary gate has exactly the same 16 test_mzload.c failures as arrival,
confirmed by a byte comparison of its FAIL-line files; no other assertion
fails there (known initech-zxzi). Both size-mutant commands stop at their
red real-size prerequisites, so their padding bodies are not certified.

Remaining gap: the binding kernel is 12576 B over r3; the flagship is 3476 B
over r4. This is a measured partial improvement, not a completed green round.
