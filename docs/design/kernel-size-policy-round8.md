# Kernel Size Policy, Round 8

Bead: initech-hztg. Base: 2808f8b5. Branch: lane/ksize.
Status: Part 1 was PARTIAL. Part 2 achieves r4 GREEN; r3 remains RED by
9076 B and is explicitly operator-owned under the part-2 brief.

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

## Part 2 -- audited parked levers (lane/ksize2)

Arrival is the part-1 merge 6b02c678: flagship 183700 loaded bytes. The r4
bar for this lane is the unchanged 180224 B flagship cap. The all-kernel
172492 B r3 cap is reported separately; its policy decision is with the
operator. Bochs legs are the PM's, per LANE-BRIEF.md.

The arrival source supersedes two details of the historical round-2 estimate:
loader_context_t already saves/restores ESI/EDI, and all the listed hardware
drivers except keyboard already run native -Oz. Savings already taken in
round 4 are not counted again. Arrival host loader/MZ and native-boundary
checks are green (the earlier part-1 MZ fixture failures do not reproduce).

### Lever 1 -- loader transfer and whole-object cold optimization

All callee-saved GPRs are captured in the context with compile-time offset
assertions, and the non-returning hook restores EBX/EBP/ESI/EDI, ESP, then
jumps to the saved label. The entry asm must also declare ECX and EDX: the
child can destroy these caller-saved registers even though the entry sequence
itself only uses EAX. The existing memory/condition-code clobbers remain.
Every input memory operand is read before the ESP switch; the entry jump
uses EAX, so frame-pointer omission cannot address the new stack by mistake.

With that contract explicit, the complete loader joins KERNEL_COLD_LTO_FLAGS;
the validation-only pragmas are redundant and removed. The actual linked ELF
is checked for the ordered save/entry/restore instructions, including the LTO
clone. A real ELF mutant changes the EDI restore displacement from 20 to 16
and must fail at restore step 4. Native event/keyboard byte-policy checks and
the host validation models remain. This structural audit complements the
injected EXEC/DOS/MZ behavior oracles; it does not replace them. Every loader
mutant recipe now uses the same optimization policy as the real object.

Native -Os recovered 576 B; native -Oz with frame-pointer omission recovered
864 B. The selected full cold policy recovers 1696 B: 183700 -> 182004 B.

Lever-1 proof: all 31 gates in build/ksize8p2/lever1-lto/status exited zero:
exec-unit; loader (+mutant, big +mutant, nested-mutant); mzload (+mutant);
exec (+mutant, nested +mutant); mzexec (+mutant); mz (+mutant); zs24-exec
(+mutant); baked-exec-guard (+mutant); batch-exec (+mutant); autoexec
(+mutant); flair-execguard (+mutant); mzload-entry-emu (+mutant);
kernel-native-boundaries (+mutant); kernel-repro. QEMU used
INITECH_QMP_STDIO=1. Complete final verdict lines are in CODEX-OUT.md.

```text
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 45ac1b447730ebfaa13be8411238dd9ef0a15019f55b1dfaed57fc6b7c4f2bc5)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 fac488e41d2e6e1bc004f14e8f13865f877d8b37d0d1c9d7403e34a9eef8da82)
```

### Lever 2 -- event-ring publication and consumption

The ISR may interrupt a partly completed pump copy. Volatile indices force
index accesses but do not order ordinary slot memory. Each side now has an
empty volatile asm memory barrier after observing the indices and another
before advancing its owned index. The complete slot is therefore written
before head is published, and copied out before tail allows reuse. This is a
compiler-ordering contract on the existing single x86 core, with no CPU fence,
new instruction-set requirement or change to the locked ring layout.

The entire event object now uses native -Oz, no strict aliasing, with frame
pointer omission. The obsolete synthesis-only pragmas are removed. Every
kernel event recipe, recording variant and mutant uses KERNEL_EVENT_OPT; the
host event replay and mutation tests also run optimized. Disassembly confirms
all three dwords are copied by REP MOVSL before the head/tail store. The
boundary gate compares the new audited native policy and rejects a different
policy; the ELF loader restore mutant continues to bite independently.

Flagship: 182004 -> 181524 B (-480 B). The historical -996 B estimate included
synthesis code already optimized in round 4. The evaluated TEST_EMU_GATES
vector contains no additional gate with pump or event in its name; the named
desktop/appswitch/itext/live gates and keyboard/modifier traces are exercised.

Lever-2 proof: all 19 commands in build/ksize8p2/lever2/status exited zero:
event (+mutant), mouse-producer (+mutant), event-owner-mutant,
kernel-native-boundaries (+mutant), flair-desktop (+mutant), flair-appswitch
(+mutant), flair-itext, flair-live (+mutant), flair-key (+mutant),
flair-modifier-release (+mutant), kernel-repro. QEMU used INITECH_QMP_STDIO=1.

```text
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 45ac1b447730ebfaa13be8411238dd9ef0a15019f55b1dfaed57fc6b7c4f2bc5)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 b0463f5842a77680c510113ceb4d35c48c9777ffd14f4c1395dab7997454e91d)
```

### Lever 3 -- hardware audit, one changed object per commit

Every changed driver faces the same 23-gate QEMU/native/repro family, before
the next object: boot; boot-system-mutant and boot-system-emu-mutant (there is
no test-boot-mutant target); kbd; flair-key (+mutant); flair-live (+mutant);
int21-irqstorm (+mutant); flair-mouse (+mutant); flair-cpanels-mouse (+mutant,
apply-mutant); panic; panic-screen (+mutant); panic-reasons (+mutant);
kernel-native-boundaries (+mutant); kernel-repro. This includes every expanded
TEST_EMU_GATES name containing kbd/mouse/pit/irq/panic, excluding Bochs.
Complete per-step verdicts are in CODEX-OUT.md and build/ksize8p2/lever3*/.

ATA (3a): complete register operands, no arithmetic-flag effects; add memory
clobbers to all four local IN/OUT primitives to order PIO buffers against I/O.
Polls always read volatile port instructions, including all four settle reads;
BSY/DRQ and retry budgets remain finite. The existing native -Oz is retained:
the -Os trial grows 64 B. Flagship 181524 -> 181524 B (0); no old savings are
claimed. All 33 gates pass: the common 23 plus ata, prewait/flush/lba/errors
mutants, no-drive-emu (+mutant), fs, block-device-emu (+mutant). Rule-11 hashes:

```text
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 eb00fa63d35d89c7f79a90c90f4189fe87551e0a71a3e805a0bcfadd1f6bfffe)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 ad0c090a1a5bb3c7900ebed1801d4f3819dbe27e09d80cc5f241e32fe77e4f38)
```

Keyboard (3b): volatile head/tail keep their existing layout. Acquire/publish
barriers cover ordinary ring slots and the extended-key two-byte pair, and
memory clobbers keep the consumer inside CLI/STI. Port reads remain volatile;
the 16-read drain is unchanged. Whole native -Os retains no strict aliasing
and frame-pointer omission; the prior Set-1 -Oz subpolicy remains. Every
kernel/host keyboard mutant uses the same policy. Native boundary sections
are now compared against the actual event.o/kbd.o kernel objects.

Flagship 181524 -> 181076 B (-448). All 36 command invocations in lever3b/status
pass: the common 23 plus kbd-unit (+mutant), j1lu-kbd-mutant, an extra native
boundary check, conkeys (+emu-mutant), tsr (+mutant), flair-modifier-release
(+mutant), input-owner-mutant, flair-copybreak (+mutant). Rule-11 hashes:

```text
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 552527fad847073cf84533a2f61887f24ee5b94f271378101eb3b43236571967)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 1b2d192f7443d98b2ff6804f68ed604d88857e4e1890bae607198ad4928dfe2d)
```

IDT (3g): LIDT names its descriptor but also consumes the pointed-to gate
table. A memory clobber publishes all initialized gates before loading it.
Operands are complete; LIDT does not change arithmetic flags. Init loops stay
bounded at 256 defaults and 32 exceptions. Native -Os retains the prior
frame-pointer omission/no strict aliasing: 181076 -> 181076 B (0). Dropping
frame-pointer omission grew 32 B and was rejected despite green proof.
All 25 selected-policy gates in lever3g-final/status pass: the common 23 and
idt (+mutant). Rule-11 hashes:

```text
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 f2e76ec16ae22f68e9e2151c1ed00cbe69f61239594a9621cd4962e94efdf6a0)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 21ddca097ce7a2b8e9d1279b8d9d474b46449222abfea529033ffd969f3fc4c4)
```

The other native drivers are audited and left at their existing round-4
policy: mouse (3c), PIC (3d), IRQ (3e), PIT (3f). Individual -Os trials recovered
0 loaded bytes each. Mouse status polls are volatile, bounded at 100000 reads;
ACK/drain counts remain bounded, packet state is ISR-owned, and slave-before-
master EOI is intact. The active (last) mouse recipe uses KERNEL_LOWLEVEL_OPT;
the earlier -Os recipe is shadowed, so that variable is not mistaken for the
effective policy. PIC operands fully name their ports/values; volatile OUT
also preserves the port-0x80 delays. PIC has no ordinary shared RAM state;
its native call boundaries order caller initialization before unmasking.
IRQ depth and PIT ticks are volatile, naturally aligned counters. UART loops
are bounded volatile IN reads; the terminal INT3 loop intentionally never
returns. Tick/mouse hooks are installed through native calls, and their
ISR-only state does not require a new RAM/port ordering assumption. None of
these zero-saving policies is reflagged just to count a historical saving.

Panic (3h): publish the fault-hook store before caller IRQ enable/DOS handoff;
order the completed DAC port sequence before physical pixel stores; make
terminal CLI/HLT a memory barrier. UART polls retain volatile IN and their
100000-read bound. Exception entry retains KERNEL_ASM_ABI (cdecl/regparm(0));
no harness lookup relies on a newly internalized symbol. Native -Os recovered
0 B here; the full house cold policy, including effective -Oz/LTO and matching
cross-jump/alias options, recovers 128 B: 181076 -> 180948 B. Every normal,
debug, reason and mutant recipe uses KERNEL_PANIC_OPT. This adds no port,
state or raster behavior; the rasterizer source stays unchanged.

All 29 gates in lever3h/status pass: the common 23, panic-layout (+mutant),
panic-reason-map (+mutant), flair-app-launch (+all six mutants). Crash triage
kills only the crashing tenant and the desktop survives. All ten saved
positive panic/reason PPM captures compare byte-identical before/after.
Bochs accuracy is still the explicitly delegated PM leg, not certified here.

```text
GREEN panic byte comparison: all ten before/after PPM files identical
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 d2ef56676ac8c5686667e688ab06fe11ae477b5b582da8e9f5b574f38a76ae26)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 8c9dcb57bc2a9888b599d3d6fe1f823d6f897877ca605a7c3e57344d71fe971a)
```

### Lever 4 -- kernel-only ABI global-data alignment

KERNEL_COLD_DATA_FLAGS adds -malign-data=abi only to KERNEL_COLD_LTO_FLAGS.
It is absent from KERNEL_CFLAGS, its cdecl derivative and TENANT_CFLAGS; the
expanded HELLO.EXE recipe confirms the boundary. Natural type layouts and
packed/explicitly aligned records are unchanged: a standing compile-only
probe compares 22 sizeof/alignment pairs across boot, PSP/DOS, IDT/frame,
loader/tenant, keyboard/event and graphics records. Both 176-byte vectors
hash d6f6141387ab6eba3518e6324082329c182942f220d2ff8ceae9b62de70442e2;
explicit align 16 remains 16. Existing locked-header assertions also compile.
A real -fpack-struct=1 counterexample must fail assertions or vector equality.
No spec, ABI, ISA, hot-object policy, linker root or boot geometry changes.

Flagship 180948 -> 180240 B (-708). This intermediate r4 preflight is still
RED by 16 B; no cap is loosened. The final acceptance after the named rodata/
panic-rendering lever is reported below. Earlier isolated trials without
this complete cold set measured -740 B; the selected tree's measured reclaim
is 708 B. Complete preflight verdicts are in CODEX-OUT.md.

Lever-4 affected-family proof: all 89 commands in lever4-family/status exit
zero, covering the union of the selected loader, event and driver families
above, including all behavior mutants. Layout (+packing mutant), native
boundaries (+mutants) and makefile-vars are independently green in preflight.
The real r4/r4-mutant prerequisites remain RED at this checkpoint, explicitly
recorded at 180240 B; lever 5 supplies the remaining recovery. Rule-11 hashes:

```text
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 dd53e4d9c0d20af2153a4fac62a5bd8d78779a620ad62d2a310c8b07c08bb849)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 65ca20ae19bb06a4bb2aacf3b39741eeb9bdd8df4dbc826bd78b0d0207342d21)
```

### Lever 5 -- linked strikes and compact panic raster

Chicago, Geneva 9 and Geneva 12 all link through text.c's FONT_TABLES_DEFINE.
Geneva 12 uses the same generated row-dictionary storage as Chicago; leave its
1462 B of named data linked (762 rows + 192 offsets + 124 dictionary + four
96-byte first/count/lb/aw arrays). FONT.BIN in the panic fixture is a separate
BIOS VGA font dump for the independent oracle, not a Toolbox volume-font route.

The historical 622 B panic_screen_lines symbol is text, not rodata. A shorter
raster recovers 64 loaded bytes. Every supported black pixel is all zero bytes,
so clear exactly w*bytes visible bytes per row in the same byte order. The
existing guard proves that product <= pitch and prevents overflow/padding
writes. Scales are 1 or 2, so unsigned pixel divisions are exactly shifts by
0 or 1. Foreground masks, coordinates, clipping and bytes stay identical.
An include-order-preserving final-policy control measured 180240 B; shifts
alone 180208 B; byte clear plus shifts 180176 B. The production build confirms
180176 B: r4 green, 48 B under the unchanged cap. No font or locked spec header is changed.

Preflight r4 and its exact cap+1 mutant pass; host panic-layout (+mutant) covers
8/24/32bpp, clipping, odd sizes, row padding and framebuffer guards. All ten
before/after positive panic/reason PPM captures are byte-identical under QEMU.
Final family/acceptance output and the all-kernel r3 measurement follow below;
Bochs remains with the PM per the brief.

### Part-2 final acceptance and handoff

Flagship 183700 -> 180176 B: **3524 B reclaimed**, 16432 B headroom, 48 B
under the unchanged r4 cap. The aspirational 5 KB margin was not achieved.
All 309 kernels meet the unchanged 4096 B global floor in the 196608 B
window. The new binding kernel is kernel_flairtenants_db_write_skip.elf:
181568 B loaded, 15040 B headroom. The remaining all-kernel r3 gap is exactly
**9076 B** (181568 - 172492), not the first failure's smaller 7140 B gap.
The former G12ROW binding now loads 181512 B.

| Lever | Flagship before B | After B | Reclaimed B |
| --- | ---: | ---: | ---: |
| 1 loader | 183700 | 182004 | 1696 |
| 2 event | 182004 | 181524 | 480 |
| 3a ATA ordering | 181524 | 181524 | 0 |
| 3b keyboard | 181524 | 181076 | 448 |
| 3c-f mouse/PIC/IRQ/PIT | - | - | 0 (already optimized; retained) |
| 3g IDT ordering | 181076 | 181076 | 0 |
| 3h panic cold policy | 181076 | 180948 | 128 |
| 4 cold ABI data alignment | 180948 | 180240 | 708 |
| 5 compact panic raster | 180240 | 180176 | 64 |
| 5 linked Geneva 12 | - | - | 0 (1462 B retained) |

The complete per-lever and final command verdicts are in CODEX-OUT.md;
build/ksize8p2/final-kernel-sizes.tsv contains all 309 independent measurements.
Final acceptance has 45 zero exits and only the explicitly expected r3 size
failure; both r3 emulator behavior gates are green. The final panic/reason
captures remain byte-identical, and the host guards include 24bpp.

```text
>>> test-kernel-headroom: 309 kernels measured; window 196608 B (384 sectors); floor 4096 B
    flagship kernel_flairtenants.elf                              180176 B loaded, headroom  16432 B
    LARGEST  kernel_flairtenants_db_write_skip.elf                181568 B loaded, headroom  15040 B (binding)
>>> test-kernel-headroom: green
>>> test-kernel-headroom-mutant: green (floor-1 pad kernel RED, floor pad kernel GREEN)
>>> test-kernel-size-r4: green (flagship keeps >= 16 KiB headroom in the 384-sector window)
!!! kernel-size-r4 FAIL: loaded 180225 B > 180224 B (the flagship must keep 16 KiB headroom -- size round 5 is due)
>>> test-kernel-size-r4-mutant: green (size_pad_one RED)
>>> test-kernel-repro: green -- build/kernel.bin reproducible (sha256 58e8311c5f0dd4ac2e3eba2553f4d41e40bccf16320181482370dada6619d277)
>>> test-kernel-repro: green -- build/kernel_flairtenants.bin reproducible (sha256 14cf8db0b07e2c10a6e6f41d5e2cea40401350d07c8ce9a7fe540edf459c0e76)
>>> test-kernel-gc: green (entry/live code/data retained; dead code/data removed)
>>> test-kernel-gc-mutant: green (no_gc/no_sections both RED for dead code)
>>> test-kernel-gc-lto: green (dead native reference discarded after LTO; entry fixed)
>>> test-kernel-gc-lto-mutant: green (no_lto_sections RED)
>>> test-kernel-cold-lto: green (cross-TU call folded to 22; entry fixed)
>>> test-kernel-cold-lto-mutant: green (no_lto RED)
>>> test-makefile-vars: green
!!! kernel-size-r3 FAIL: build/kernel_alert_chrome_finder_press.elf loaded 179632 B > 172492 B (10 KiB reclaim required)
>>> test-kernel-size-r3-emu: green (desktop pixels, text trace, DOS/FCB/FAT differential)
>>> test-kernel-size-r3-emu-mutant: green (desktop/FCB behaviour mutants RED)
```

No Bochs gate was run; the PM retains accuracy acceptance. Local followups
are initech-7sp (operator r3 decision) and initech-3kc (PM Bochs acceptance).
The original initech-hztg stays in progress for those parent-level decisions;
the coding lane's part-2 r4 bar and all requested QEMU/host proofs are complete.
Dolt origin is absent in this worktree; no remote endpoint is invented.

### Part 2 review fixes

DF ground truth: Intel SDM Vol. 1 Sec. 3.4.3.2, "DF Flag", describes
STD/CLD and the direction of MOVS/STOS. Interrupt delivery saves EFLAGS;
it does not establish the C ABI forward-string convention. This is the
8086 convention: clear live DF before C, preserve the interrupted FLAGS
image for IRET. Reference acquired before the fix:
https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf

F1: add CLD to all 14 previously unguarded paths into C in isr.asm,
including IRQ0, DOS/TSR services and the selftest as well as the five
reported paths. Exception/spurious vector wrappers share isr_common;
the bare TSR idle IRETD calls no C. INT 13h and the Toolbox gate already
clear DF. Add one CLD at the loader resume label: 15 one-byte instructions.
Measured .text grows 16 B; loaded bytes grow 32 B after section padding:
180176 -> 180208 B, 16400 B headroom, 16 B r4 margin.

The current EXEC harness cannot faithfully combine a child held at STD,
an injected IRQ under the live FLAIR hook, and inspection of the raw payload:
EXEC images lack that hook, while live images have no child/ring observation
protocol. A faithful behavior probe is unavailable in the current harness.
Use the brief's static fallback, not a claimed IRQ payload proof.
test-kernel-df inspects all 16 linked C-entry stubs for CLD before every
CALL and follows the saved loader resume address to its first instruction.
Before the fix it is RED for the missing entry/resume CLDs; after the fix
it is GREEN. test-kernel-df-mutant replaces the real ELF IRQ1 CLD with NOP
and is RED for calling C before CLD. Logs: build/ksize8p2-review/df-before.log
and f1.log. test-kernel-size-r4 remains GREEN.

F2: psp_ptr is now a "b" input. GCC establishes EBX before the asm;
the context's EBX save records that input and GCC preserves its own caller's
EBX. Removing the undeclared in-asm EBX write removes the hazard of addressing
the subsequent stack_top operand through an overwritten EBX. The transfer
checker accepts only an ESP/EBP-based ESP source, scans all executable ELF
sections for the save sequence (including unnamed/inlined copies), and
requires exactly one transfer and one restore. It is RED on the pre-fix
transfer at save step 9 and GREEN on the constrained version. New real-ELF
mutants change the stack source base to EBX and insert a second copy of the
actual transfer bytes outside the loader symbol; both are RED for their
intended reasons. The existing wrong-EDI-restore mutant also remains RED.
Proof: build/ksize8p2-review/f2-before.log, f2.log and f2-mutants.log.
Flagship remains 180208 B (0 B delta); .text and section sizes are unchanged.
