<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0009 -- SAMIR <-> InitechDOS Integration: the numeric mechanism, the memory model, and app packaging (Phase 8 S8.1/S8.2)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0009 |
| Title | ADR-0009: SAMIR <-> InitechDOS Integration (numeric / memory / packaging) |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED 2026-06-19 (operator-delegated; ADR-by-committee, chair-synthesized; all load-bearing claims repo-verified)** |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Related Documents | `docs/adr/ADR-0008-SAMIR-InitechBase-Architecture.md` (this is its integration-layer companion; ADR-0009 settles the §2.F numeric mechanism ADR-0008 DEC-07 deliberately deferred to Phase 8); `docs/plans/SAMIR-implementation-plan.md` §2.F/§5-Phase 8/§7; ADR-0003 DEC-08 (flat `.COM` now / MZ `.EXE` deferred); ADR-0001 (386+, 32-bit flat); PRD §5/§6.6 |
| Related Issues | beads `initech-ax9` (Phase-8 epic) + `ax9.1`/`ax9.2`; `initech-nh0m` (hardware.json), `initech-zj6w` (kernel x87 -- DEFERRED by this ADR), `initech-dtw` (MZ/overlay end-state) |

### Approval & Sign-Off Matrix (ADR-by-committee, 2026-06-19)

| Role | Disposition | Note |
|---|---|---|
| Platform / Kernel Architect | Ratify -- soft-float primary | Found the decisive fact: the engine **already** links libgcc (`__divdi3`/`__udivdi3`/`__udivmoddi4`); flagged the arena/program overlap as the biggest risk |
| SAMIR / Engine Architect | Ratify-with-dissent (preferred kernel-x87-now as a fast path) | Dissent overruled: the "soft-float is extra work" basis is refuted by the libgcc finding; accepted soft-float as the authentic end-state regardless |
| Fidelity / Period-Authenticity & Law Steward | Ratify -- soft-float #1 on authenticity | "DOS did nothing with the FPU; dBASE did software math" -- kernel-x87 is a Law-3 (kernel period-plausibility) deviation, invisible to the Law-4 judge but real to a source reviewer |
| Chair (operator-delegated) | **Ratified** | All four load-bearing claims independently re-verified against the repo before ratification (Law 2 / Rule 4) |

---

## 1. Context

ADR-0008 ratified the SAMIR engine architecture (the PAL split, dBASE III+ 1.1-only, IEEE-double internal rep) but **deliberately deferred** the on-target numeric *mechanism* to Phase 8: DEC-07 / plan §2.F flagged "a small PAL/kernel 'FPU ready' step + a hardware-contract line" as GATED pending a hardware contract that did not exist. The engine is now host-complete (176 host gates green) and the task is to make it **run inside InitechOS**: plan Phase 8 `S8.1` (`pal_milton.c` + numeric-on-target) -> `S8.2` (SAMIR as a loadable app on the booted kernel; the boot->USE->LIST screendump gate). `S8.3` (FLAIR window) is hard-gated on M4 and out of scope.

Repo-verified ground truth driving this ADR (measured 2026-06-19):

- The 17-file engine compiles cleanly freestanding: **~81 KiB text, ~439 KiB BSS** -- but the BSS is ~95% a *host-test convenience*: `flow.c`'s `FLOW_MAX_REGISTRY=16` interp registry (`s_arena` 256 KiB + `s_pool` 162 KiB). Milton runs **one** interpreter -> the genuine footprint collapses to ~81 KiB text + ~26 KiB BSS, which **fits** the locked program window.
- The engine emits **pervasive x87** for its `double` math; the kernel has **no FPU init**; `#NM`(7)/`#MF`(16) route to the `PC LOAD LETTER` panic. So a SAMIR numeric op on the booted kernel would panic unless the FPU is initialized **or** no x87 is emitted.
- The engine **already** references libgcc 64-bit integer helpers (`__divdi3`/`__udivdi3`/`__udivmoddi4`) -- so libgcc linkage must be resolved for the freestanding `SAMIR.COM` **regardless** of float strategy.
- The AH=48h MCB arena is bound over `[PROGRAM_BASE, PROGRAM_ALLOC_END)` with **flat base == PROGRAM_BASE** (`sysinit.c:118`) -- the exact window the loaded program image + PSP + stack occupy. Latent today (toy `.COM`s never call AH=48h; the emulator zero-inits RAM), but SAMIR is the first app with a real heap.

**Period grounding (operator's question, 2026-06-19 -- the spine of DEC-01).** dBASE III+ targeted 8088/286 PC/XT/AT where the 8087/287 was an *optional* coprocessor most business PCs lacked. **DOS 3.3 did nothing with the FPU** (no init, no state mgmt -- single-tasking). Period apps brought their own floating point: the compiler runtime auto-detected the 8087 and fell back to a **software 80x87 emulator** when absent; dBASE did software math, IEEE-double-equivalent in result. dBASE shipped as **`DBASE.EXE` + `DBASE.OVL`** -- a relocatable MZ `.EXE` with on-demand overlays, in 640 KB conventional real mode; DOS handed it the largest MCB block (AH=48h). It was **not** a flat `.COM`; loading high in extended memory is anachronistic for the III+ 1.1 period.

---

## 2. Decisions

### DEC-01 -- Numeric mechanism: SOFT-FLOAT (software IEEE-754 double); kernel x87 DEFERRED.

The SAMIR engine (`core/`/`fs/`/`cmd/`) ships compiled `-msoft-float -mno-80387`: no x87 instructions are emitted; `double`/JDN arithmetic runs through the libgcc soft-float helpers (`__adddf3`/`__subdf3`/`__muldf3`/`__divdf3`/`__fixdf{si,di}`/`__float{si,di}df`/`__{eq,ne,lt,le,gt}df2`, verified the full set). This is:

- **Period-authentic** -- exactly what dBASE III+ did (software FP; the 8087 was optional; DOS uninvolved). Directly answers the operator's "how would dBASE/DOS have done it."
- **Deterministic (Rule 11)** -- strict 64-bit IEEE at every step; **no 80-bit x87 intermediates**, no FP-control-word-drift risk, no host/target divergence. The round-trip and program-diff goldens bite bit-for-bit.
- **Tri-emulator-safe (Rule 5)** -- one code path on QEMU/Bochs/86Box; sidesteps the "does 86Box's 386 present a 387 / does Bochs panic where QEMU silently succeeds" hazard.
- **Zero kernel boot change** -- SAMIR stays decoupled from the kernel exactly as ADR-0008 DEC-02 promises.

The "soft-float is extra work" objection is refuted: the freestanding `SAMIR.COM` must link libgcc **anyway** (the engine already needs `__divdi3` et al.), so the marginal cost is the soft-float objects from the *same* libgcc.

**Kernel x87 (CR0.EM=0/MP=1/NE=1 + FNINIT + a pinned control word) is the correct 386+387/486 platform end-state** and is DEFERRED to a post-M6 bead (`initech-zj6w`, re-scoped) for when FLAIR/Turbo-Initech want hardware FP. It is **not** used for M6. Reason it is not primary now: it couples SAMIR's landing to a kernel boot change, is least period-authentic (a Law-3 deviation -- DOS never touched the FPU), and reopens the 80-bit-intermediate computed-key divergence ADR-0008 DEC-07-rev flagged GATED.

### DEC-02 -- libgcc linkage for the freestanding artifact (Rule 11 pin).

The `SAMIR.COM` link resolves the libgcc helpers (integer + soft-float) from a **pinned** `-m32` libgcc (provenance recorded in the build manifest / `spec/hardware.json`). A floating libgcc version is a reproducibility hole (Rule 11). Implementation may extract the i386-multilib objects from the host libgcc or, as a fallback, vendor a small audited `softfp.c`/`softint.c` -- whichever is chosen is recorded and mutation-proven. Hand-rolling IEEE rounding from scratch is disfavored (it would itself need a differential oracle; libgcc's soft-float is well-tested).

### DEC-03 -- Memory model: flat `.COM` in conventional memory now; `FLOW_MAX_REGISTRY=1`; MZ/overlay is the authentic end-state.

For S8.2: ship SAMIR as a **flat `.COM`-equivalent** (ADR-0003 DEC-08; MZ deferred) in the locked program window `[0x30100, 0x5F000)`, with a **Milton build profile** that sets `FLOW_MAX_REGISTRY=1` (a pure dimensioning knob, default 16 for the host gates -- **no target-only behavior**, Law 3). This collapses the engine BSS to ~26 KiB, fitting the 188 KiB window with room. **Loading high in extended memory (>1 MB) is REJECTED** as anachronistic for a III+-era app.

The **authentic end-state is the MZ `.EXE` + overlay loader** (`DBASE.EXE`/`DBASE.OVL` pattern; bead `initech-dtw`). The flat `.COM` is an honest, labeled interim -- not "how dBASE shipped." `initech-dtw` remains open and is **raised to P2** (it is the period-faithful packaging, and the "corporate software accretes / never deletes" stance wants the vestigial overlay-dispatch machinery present when it lands).

### DEC-04 -- InitechDOS correctness fix: the AH=48h arena MUST be disjoint from the loaded program.

The arena's free region must start **above** the loaded program image + BSS (paragraph-rounded) and end at/below `PROGRAM_STACK_BOT`, so `AH=48h` can never return memory inside the running program or its stack -- the authentic DOS model (DOS hands the program the block *after* its image). This is a genuine latent-corruption fix (operator-authorized InitechDOS work), not a SAMIR special-case; it is documented in `spec/memory_map.h`'s arena commentary (Rule 8 -- a deliberate locked-data act, computed base rather than the literal `PROGRAM_BASE`). The S8.2 oracle MUST force a real `AH=48h` allocation so this path is exercised, not a tiny `USE`+`LIST` that fits in static BSS (Rule 3 -- do not pass because the inputs don't overlap yet).

### DEC-05 -- BSS-zeroing belongs in a SAMIR entry stub, not the shared loader.

A flat `.COM` does not carry `.bss` in its file, and neither the loader nor `kstart.asm` zeroes BSS (it "works" today only because the emulator resets RAM to 0 -- a latent Rule-3 bug). A SAMIR `crt0`-equivalent entry stub (linked first at org 0x30100) zeroes `__bss_start..__bss_end`, ensures ESP (the loader supplies `PROGRAM_STACK_TOP`), binds `pal_milton`, calls `samir_repl`, and `INT 21h AH=4Ch` on return. `.bss` is marked `NOLOAD` so `objcopy -O binary` does not emit zero-fill (small image, honest org); the stub materializes it. BSS-zeroing in the *shared* loader is deferred to the MZ-loader future (it has the section table the flat loader lacks).

### DEC-06 -- `pal_milton.c` is the sole `int 0x21` site; S8.2 needs no console primitives.

`pal_milton.c` binds the 15-slot vtable: open/close/read/write/seek/remove/rename -> INT 21h 3Dh/3Ch/3Eh/3Fh/40h/41h/42h/56h; `conout` -> AH=40h (handle 1); `conin_line` -> AH=3Fh/0Ah on CON; `today` -> AH=2Ah; `alloc`/`reset` -> the (DEC-04-fixed) AH=48h arena. It is the **only** TU that issues `int 0x21` (Law 3). `gotoxy`/`set_attr` (console primitives, not INT 21h) bind as **no-ops** and `conin_char` as an AH=07h passthrough for S8.2 -- they are exercised only at S8.4 (`@SAY/GET/READ` forms), which is gated on S8.3/M4.

### DEC-07 -- `spec/hardware.json` declares the FPU OPTIONAL.

The locked §5 hardware contract (Rule 8; none exists today) declares `"fpu": "optional"` (a present-but-optional 387, per ADR-0001's 386+387/486 platform) with `"init_by_kernel": false` (binding DEC-01: InitechDOS does not initialize the FPU, exactly as DOS 3.3 did not). It MUST NOT say `"fpu": "required"` (anachronistic -- most period PCs lacked an 8087). It records the conventional program window (citing `spec/memory_map.h`, not duplicating it), the pinned-libgcc provenance (DEC-02), and a `test-spec`-style schema gate, mutation-proven (perturb a constant -> RED).

### DEC-08 -- The S8.2 oracle: in-emulator boot -> USE -> LIST, screendump + serial, mutation-proven, QEMU+Bochs.

An additive **emu** gate boots the image, EXECs `SAMIR.COM`, drives a scripted `USE <table>` / `LIST` / `QUIT`, and asserts on (a) the **screendump** (Law-4 "look at the screendump": dot prompt + echoed commands + correctly-formatted record listing) and (b) the **serial transcript** (the deterministic diffable signal -- the LIST rows match the known `.dbf`). Mutation-proven on the **on-target I/O path** (e.g. a `pal_milton` short-read mutant garbles the rows -> RED), not the weak "module unregistered" mutant. Run on **QEMU + Bochs** (Rule 5); a cross-emulator disagreement is a stop-condition, not a QEMU-pin. The gate is additive (does not disturb the 176 host / 27 emu gates); the count delta is verified wired (WL-0028 lesson).

---

## 3. Consequences

- **Positive:** SAMIR runs inside InitechOS with no kernel FPU dependency; fully deterministic numerics; tri-emulator-safe; period-authentic (software FP, DOS uninvolved); a real InitechDOS arena bug is fixed; the flat-`.COM`/MZ split is recorded honestly.
- **Negative / debt:** soft-float is slower than hardware x87 (irrelevant at SAMIR's interactive scale); kernel x87 + the MZ/overlay authentic packaging are deferred (tracked: `zj6w`, `dtw`); a pinned-libgcc obligation enters the build (Rule 11).
- **Risk register:** the **arena/program overlap (DEC-04)** is the biggest risk -- it passes today for the wrong reason and would corrupt SAMIR's heap nondeterministically; the S8.2 oracle must force an allocation. The **BSS-zero + (now N/A) control-word** items (DEC-05) are cheap but invisible until hit -- they must be in S8.2 from the first commit with an explicit on-target assertion (read a static sentinel).
- **Stop-conditions watch (Rule):** do NOT widen any numeric oracle tolerance to accommodate an x87 divergence -- soft-float removes that category entirely; if a future x87 end-state reintroduces it, the remedy is precision-control (FLDCW), never a looser diff.

---

*-- End of ADR-0009 (RATIFIED 2026-06-19) --*

<!-- Tedium certified compliant with NFR-7. The FPU is optional; the bug is the feature; the overlay you do not need is retained in full. -->
