<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0033 -- SAMIR (InitechBase) RUNS INSIDE InitechOS: S8.2 milestone (soft-float + disjoint arena + in-place loader + read/write round-trip)

**Milestone:** M6 Phase 8 (SAMIR <-> InitechDOS integration), steps S8.1 + S8.2 + the write-round-trip deepening.
**Governing decision:** **ADR-0009** (ratified this session, ADR-by-committee).
**Date:** 2026-06-19.

## Context

SAMIR was host-complete (176 host gates, `make test-dbase` GREEN) but **host-only** -- it had never run on the booted OS. The operator directed an orchestrated push to "get the samir dbase working in the initechos," authorizing new InitechDOS work as needed, with an ADR-by-committee for serious decisions. The load-bearing question was the on-target **numeric mechanism** (the engine emits pervasive x87; the kernel has no FPU init and no `spec/hardware.json`) and the **memory model** for a large app.

## What changed

**Governance.** Convened the committee (Platform/Kernel + SAMIR/Engine architects [opus] + Fidelity/Law steward [sonnet]). The operator's period-authenticity question ("how would dBASE/DOS have done it") reframed it: **dBASE III+ did software FP -- the 8087 was optional and DOS 3.3 did nothing with the coprocessor**; dBASE shipped as `DBASE.EXE`+`DBASE.OVL` (MZ + overlays) in 640 KB conventional. Ratified **ADR-0009**: SOFT-FLOAT engine (no kernel FPU), flat `.COM` in conventional memory now (MZ/overlay = authentic end-state, bead `dtw`), kernel x87 deferred (`zj6w`).

**Wave 1 -- foundations (176 -> 182 host gates).**
- `pal_milton.c` (`ax9.1`): the 15-slot PAL bound to INT 21h -- the SOLE `int 0x21` TU (Law 3).
- AH=48h arena DISJOINT from the loaded program (`1q4u`/DEC-04): the arena was bound flat-base == PROGRAM_BASE, overlapping the running program; now `int21_mcb_bind_program` places it ABOVE image+BSS, below env/stack, left FREE.
- `softfp.c` (`ap5g`/DEC-02): vendored libgcc soft-float + 64-bit int helpers (no static i386 libgcc on the host) incl. `__udivmoddi4` (needed at -Os); `test-samir-softfp` 23858 checks vs host double + mutant.
- `samir_crt0.asm` + `samir.ld` (DEC-05): flat `.COM` @0x30100, `.bss` NOLOAD zeroed at runtime by the entry stub.
- `FLOW_MAX_REGISTRY` knob (`qucm`/DEC-03): Milton build =1 -> engine BSS 418 KiB -> 26 KiB.
- `spec/hardware.json` (`nh0m`/DEC-07): `fpu=optional`, `init_by_kernel=false`, cpu=386+, mem window, libgcc pin.

**Wave 2a -- loader (`za4m`, 182 -> 184 host gates).** `SAMIR.COM` is 77 KiB but `load_program_from_fat` staged FAT loads in a 64 KiB buffer (`LOAD_STAGING_MAX`) -- rejected. Fix: read the `.COM` DIRECTLY into PROGRAM_IMAGE, no staging copy; the sole bound is now PROGRAM_IMAGE_MAX (~188 KiB). Shared `loader_run_plan(do_copy)`: baked path copies, FAT path runs in place. Benefits all future large apps (TPS, the Pascal compiler).

**Wave 2b -- S8.2 MILESTONE (`hdlb`, +2 emu gates).** `boot -> EXEC SAMIR.COM -> USE CLIENTS.DBF -> LIST 3 records -> QUIT` on QEMU, no triple-fault. The dot prompt + LIST table render on the seafoam LFB console (Law 4) and on serial. The end-to-end gate caught a real runtime bug (Law 2) that the host arena oracle missed: `int21_mcb_bind_program` called `int21_mcb_reset()`, claiming the arena for the PSP -> AH=48h OOM -> panic; fixed to leave it FREE. Fixture `CLIENTS.DBF` minted deterministically by the SHIPPED dbf writer.

**Wave 3 -- write round-trip (`g6wx`, +2 emu gates).** `USE -> REPLACE BAL WITH 9999.99 -> APPEND BLANK -> REPLACE BAL WITH 5555.55 -> LIST -> QUIT`, then `mcopy` the `.dbf` OUT and verify with the independent `dbf_ref.py`: **record_count 3->4, rec0 BAL=9999.99, rec3 BAL=5555.55 ON DISK.** First runtime exercise of pal_milton's write/seek/close path (no bug found). Mutation-proven (drop-write -> stale on-disk `.dbf` -> RED). `ppm_text_check` gained purely-additive `bg_y0/bg_x0` args (scroll-immune right-margin seafoam sampling).

## Why

Soft-float is the most period-authentic (dBASE did software math; DOS ignored the FPU), the most deterministic (Rule 11 -- no 80-bit x87 intermediates, no control-word drift), and tri-emulator-safe (Rule 5 -- one code path). The libgcc finding (the engine already needs `__divdi3` et al.) made soft-float nearly free, refuting the only argument for kernel-x87.

## Frictions / lessons

- **Parallel agents drift on shared seams.** pal_milton (written against the OLD whole-window arena model) and the arena fix (the NEW disjoint model) disagreed; the orchestrator caught it at integration (dropped pal_milton's stale `AH=4Ah SETBLOCK`). Then the e2e emu gate caught a SECOND arena bug (`int21_mcb_reset` claiming the arena) that no host oracle bit. **Law 2: only the end-to-end oracle is truth.**
- **-O level matters for soft-float helpers.** `-Os` emits `__udivmoddi4`; `-O0` emits separate `__udivdi3/__umoddi3`. A proof built at one -O can miss a helper needed at another.
- **A >64 KiB app exposed a latent loader cap.** Always re-derive size assumptions for the first big consumer.
- `bd dolt push` has no configured dolt remote (prints a usage hint, exit 0); beads persist locally + via git. Code pushes via `git push` as normal.

## Acceptance

- `make test-unit`: **ALL GREEN (184 host gates)** from clean.
- `make test-emu`: **ALL GREEN (31 QEMU gates)** incl. `test-samir-boot(+mutant)` and `test-samir-write(+mutant)`.
- emu regression (the shared int21 arena change): `test-exec`/`test-shell`/`test-mcb-emu` PASS.
- All committed (`bf54c91`, `6f06411`, `8afa37d`, `6336efb`) and pushed to `origin/command-com-default`.

## Pointers

- `docs/adr/ADR-0009-SAMIR-Milton-Integration-Numeric-Memory-Packaging.md` -- the ratified DECs.
- `os/samir/boot/{samir_crt0.asm,samir.ld,softfp.c}`, `os/samir/pal/pal_milton.c` -- the artifact binding.
- `make samir-com` -> `build/SAMIR.COM` (77792 bytes, soft-float, x87=0, org 0x30100).
- Gates: `test-samir-boot`, `test-samir-write`, `test-samir-softfp`, `test-arena-disjoint`, `test-loader-big`, `test-hardware-spec`.

## Remaining (filed)

- `g6wx`-style deepening continues: SEEK/`.ndx` index + DELETE/PACK in-emulator; a CANON app (Y2K accounting `586.1` / Bolton's salami `586.2`) running INSIDE InitechOS via a `DO <file>` REPL feature -- the Law-4 showpiece.
- `7az.13` transcendentals (SQRT/LOG/EXP) -- now tractable on the soft-float base.
- `ax9.3` (S8.3 FLAIR window) + `0tl`/`0tl.1` (S8.4 @SAY/GET forms) -- HARD-GATED on M4 (FLAIR).
- `zj6w` (kernel x87 end-state), `dtw` (MZ/overlay authentic packaging), the flow_state-arena refactor -- deferred quality/end-state.
