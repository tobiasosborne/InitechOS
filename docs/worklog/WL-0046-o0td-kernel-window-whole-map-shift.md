<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0046 -- initech-o0td: the InitechDOS conventional-memory-map redesign (whole-map shift +0x8000) that re-opens kernel growth

**Type:** Locked-spec change (CLAUDE.md Rule 8) -- the principled fix for the EXHAUSTED kernel
window. **Date:** 2026-06-20. **Branch:** command-com-default (continues WL-0045, the hsct
negative-result log that escalated o0td to P1). **Bead:** initech-o0td (P1).

## Context

After the DOS-3.3 parity capstone (WL-0040..0045), the kernel window `[0x10000, 0x30000)` was FULL
(`_kernel_end = 0x2f8e0`, ~1.5 KiB under `PROGRAM_BASE = 0x30000`). The Wave-6 hsct I/O-redirection
driver (~1.9 KiB .text) could not fit; it was reverted (WL-0045) and o0td escalated to P1 as the
gating prerequisite for ALL further kernel-resident growth.

The bead PRESCRIBED a fix (raise `PROGRAM_BASE` 0x30000->0x34000 PAIRED WITH cutting
`PROGRAM_BSS_RESERVE` 0x10000->0x8000). **That prescription was UNSOUND** and was disproven by
ground-truth measurement before any code was written (Law 1 / Law 4).

## The disproven premise (Law 4 -- verify the report, not the doc)

`spec/memory_map.h` and ADR-0009 claimed SAMIR is "~81 KiB text + ~26 KiB BSS." Measured from
`build/samir_com/SAMIR.elf`: SAMIR's **.bss is 0xBD20 = 48,416 bytes = 47.3 KiB**, NOT 26 KiB. A
32 KiB `PROGRAM_BSS_RESERVE` cannot cover a 47.3 KiB .bss -> the loader's heap `arena_base` would land
INSIDE SAMIR's live .bss -> the SAME corruption that reverted the earlier raise (y206). The prescribed
numbers were wrong; the HANDOFF's own guard ("verify SAMIR BSS fits 32 KiB") FAILS.

## The decision (ADR-by-committee -- 4 lenses + opus chair; gridlock=false)

A committee was convened (per the operator's "convene the committee for serious decisions"). Three
operator steers framed it: (1) OK to break SAMIR if easily fixed; (2) SAMIR can be an MZ .EXE now;
(3) SAMIR/FLAIR are RAPID PROTOTYPES against the incomplete DOS -- design the map for DOS's OWN
stability/runway; finalise SAMIR/FLAIR later.

The chair RATIFIED **PATH 2 -- whole-map shift UP by delta = 0x8000**, OVERRULING a 3-lens PATH-1
majority on dispositive arithmetic:
- PATH-1A (shrink reserve to 0xC000, arena preserved) left only 736 B between `arena_base` and
  SAMIR's real .bss end -> one byte of .bss growth re-creates y206.
- PATH-1B (keep reserve 0x10000, arena shrinks to 31.2 KiB) PANICS SAMIR: `pal_milton.c`
  `PAL_MILTON_HEAP_PARAS = 2048` requests EXACTLY 0x8000 = 32,768 B via AH=48h with `milton_panic()`
  on CF=1, and 31,968 < 32,768. (Verified in source.)
- PATH 2 keeps SAMIR's arena byte-identical (47.2 KiB) and gains +32 KiB of kernel window by
  reclaiming half the dead `[0x90000, 0xA0000)` conventional gap. Reserve held at 0x10000 (covers
  47.3 KiB .bss with 16.7 KiB slack). MZ .EXE conversion (PATH 3) unanimously DEFERRED (operator
  steer 3; big-C InitechMZ emission is unproven; the MZ loader still uses the flat reserve anyway).

## Orchestrator amendment (Law 4 -- the chair's report had a flaw)

The chair specified "kstart ESP unchanged (0x8FFFC)." Grading caught that moving `LOAD_STAGING` to
`[0x78000, 0x88000)` would then OVERLAP the kernel-stack region `[0x80000, 0x90000)`. The correct
geometry-preserving translation moves the kernel stack up by delta too: `kstart.asm` ESP
`0x8FFFC -> 0x97FFC`, kernel stack `[0x88000, 0x98000)`, disjoint from staging, 32 KiB gap to VGA.
Verified safe: `boot_info` @0x500 and `FONT_STASH` @0x1000 are low-memory (untouched); the stage2
stack anchor @0x90000 is dead after handoff.

## What changed (the new map; delta = +0x8000)

`spec/memory_map.h` (7 constants; `PROGRAM_BSS_RESERVE` HELD at 0x10000; all FLAIR_* HELD):

| const | old | new |
|---|---|---|
| PROGRAM_BASE | 0x30000 | 0x38000 |
| PROGRAM_IMAGE | 0x30100 | 0x38100 |
| ENV_BLOCK (== arena ceil) | 0x5F000 | 0x67000 |
| PROGRAM_STACK_BOT | 0x60000 | 0x68000 |
| PROGRAM_STACK_TOP | 0x6FFFC | 0x77FFC |
| PROGRAM_ALLOC_END | 0x70000 | 0x78000 |
| LOAD_STAGING_BASE | 0x70000 | 0x78000 |
| kernel stack ESP (kstart.asm) | 0x8FFFC | 0x97FFC |

Kernel window `[0x10000, 0x38000)` = 160 KiB (+32 KiB). New `_kernel_end = 0x300e0` (incl. the
restored BATCH buffer) -> ~31.5 KiB of runway under the 0x37f00 guard.

Consumers shifted in lockstep (NASM/ld cannot read the C header -- duplicated constants that MUST
track it):
- 13 `os/milton/*_program.asm`: `org 0x30100 -> 0x38100`. The self-audit caught MORE than the
  enumerated scratch pages: vect `BOGUS_24`/`R_CRITAL`, datetime's FULL `R_*` block, absdisk
  `PATBUF`/`RDBUF`/`R_AX`, plus `LBUF 0x31000->0x39000` / `R_* 0x32000->0x3A000`. (mzexec_fixture.asm
  stays `org 0` -- a deliberate reloc test.)
- `os/samir/boot/samir.ld` link base `0x30100 -> 0x38100`; `samir_crt0.asm` STACK_TOP `0x6FFFC -> 0x77FFC`.
- `os/milton/command.c`: `BATCH_FILE_MAX 2048 -> 4096` (restore the tactical .BAT cut; +2 KiB .bss,
  now covered).
- `os/milton/test_exec.c`: a LATENT test bug the shift exposed (Law 4). It hardcoded
  `kEnvAddr = 0x5F000` and asserted it threads through AH=4Bh -- but `do_exec` sanitizes env_block to
  honor ONLY `{0, ENV_BLOCK}` (int21.c). The literal passed before only by coincidence (== old
  ENV_BLOCK). Fixed to use the `ENV_BLOCK` macro (+ added the `memory_map.h` include) so it tracks
  spec and correctly exercises the sanitizer.

NOT changed (confirmed): `KERNEL_SECTORS`/`IMG_SECTORS` (kernel on-disk ends 0x2C000 < 0x38000); the
Makefile kernel-end guard (it parses `PROGRAM_BASE` from the header -> auto-rebinds to 0x38000); the
six self-contained host PSP fixtures that set `alloc_end_linear = 0x70000` (arbitrary valid ceiling,
0x70000>>4 = 0x7000 regardless of map -> stay green); `test_loader.c`/`test_mz*.c`/`test_arena_disjoint.c`
assertions (macro-based -> auto-track). `test_hardware_spec.c` and a `hardware.json` literal were
y206-claimed phantom edit sites -- they do not exist / are cited-by-reference.

## Correctness (at the final numbers)

For SAMIR (image_len 0x13220, .bss 0xBD20, relinked @0x38100 -- verified `.bss` size unchanged,
`__bss_end = 0x57040`):
- `arena_base = roundup_para(0x38100 + 0x13220 + 0x10000) = 0x5B320`; `arena = [0x5B320, 0x67000) =
  47.2 KiB`; `arena_base(0x5B320) >= .bss end(0x57040)`, margin 0x42E0 = 16.7 KiB -> NO overlap.
- arena (0x8160 = 33,120 B) >= SAMIR's 32 KiB request -> AH=48h CF=0, no `milton_panic`.
- env [0x67000,...) at the exclusive ceiling; program stack [0x68000, 0x77FFC] above it; staging
  [0x78000, 0x88000) adjacent-disjoint to kernel stack [0x88000, 0x98000); kstack top 0x97FFC < VGA
  0xA0000 (32 KiB gap). All regions disjoint.

## Acceptance (graded independently -- Law 2/Law 4)

- kernel-end guard re-bound to `PROGRAM_BASE=0x38000` and PASSED (`_kernel_end=0x300e0`); clean `-Werror`.
- SAMIR relinked @0x38100, `.bss` still 0xBD20, `__bss_end=0x57040`.
- `test-loader` 29/0; `test-arena-disjoint` 1321/0; `test-exec-unit` 64/0 (after the fix).
- **`test-samir-boot` PASS** -- SAMIR booted in InitechOS at the new map, AH=48h 32 KiB alloc with NO
  panic, opened a .dbf and LISTed PESTON/WADDAMS/LUMBERGH. (This is the EXACT oracle that went RED at
  y206's 16 KiB arena.)
- `make clean && make test` = **240 host + 39 emu gates ALL GREEN** (unchanged counts -- the map
  change adds no new gates). Bochs boot leg PASS (Rule 5 -- the protected-mode load address moved;
  `triple_fault=0` at the new base). Took THREE clean runs to reach all-green: run 1 aborted at
  `test-exec-unit` (the latent 0x5F000 literal), run 2 aborted at `test-hardware-spec` (the factory
  regression guards `harness/diff/dbf_diff/test_hardware_spec.c` + `hardware.json` base/ceiling hex
  -- a real blast-radius site my first audit missed because it greps `0x00030000` 8-digit-padded, not
  `0x30000`). Run 3 (after both fixes + the comment sweep): clean green.

## Frictions / lessons

- **The doc lied about a load-bearing number** (26 vs 47.3 KiB BSS). Always MEASURE the artifact;
  never size a spec invariant off a stale comment. This single measurement overturned the prescribed
  fix and the prior committee (y206).
- **The committee chair's report had a real flaw** (staging/kernel-stack overlap). The orchestrator
  graded it (Law 4) and amended it rather than implementing the report verbatim. Independent
  verification of the committee is not optional.
- **A latent test bug hid behind a macro coincidence** (test_exec's 0x5F000 == old ENV_BLOCK). Only
  the FULL clean `make test` caught it (the targeted gates passed) -- `make clean && make test` at
  integration is non-negotiable. The grep-audit MUST NOT be truncated (`| head` hid the hit).
- The subagent self-audit ("grep your own files for ANY absolute [0x30000,0x38000) address") caught
  scratch/result equs beyond the enumerated list -- delegate with a completeness instruction, not just
  a checklist.

## Pointers

- `spec/memory_map.h` (the new map + the corrected 47.3 KiB BSS note + the Rule-8 deliberate-act note).
- `initech-o0td` (bead: ratified PATH 2 + the orchestrator kstart amendment + the grading record).
- `initech-y206` (CLOSED/superseded -- the earlier raise that broke SAMIR), WL-0045 (the hsct blocker).
- Unblocks: hsct I/O-redirection redo (now has window room) + the new "route AH=09h/02h/06h CON
  output through handle 1" bead it also needs.

<!-- Tedium certified compliant with NFR-7. Verify revision before use. -->
