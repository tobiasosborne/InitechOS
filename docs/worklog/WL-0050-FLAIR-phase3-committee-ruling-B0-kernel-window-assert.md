# WL-0050 -- FLAIR Phase 3 committee ruling + B0 kernel-window fail-loud assert

beads: epic initech-re30 (Phase 3); re30.1 (B0) CLOSED; re30.2-.9 filed.
commit: 50c1374 (B0). Follows WL-0049 (Phase 0/1, commit fe4d9b1).

## Context

With the era-layered canonical spec landed (WL-0049, 254 host gates green), the
next-most-consequential work is Phase 3 -- getting FLAIR to actually BOOT (today
only surface.o links into the kernel; the booted OS shows seafoam + a console
banner). The architecture fork (grow the impl to honor the locked imaging spec
vs trim it; how to sequence the kernel-link safely) is exactly what the operator
said to take to the committee, so an ADR-by-committee was convened.

## The committee (2 opus seats, read-only deliberation; chair-synthesized)

- **Seat A (Toolbox/QuickDraw imaging architect):** ruled GROW, not trim. The
  locked grafport.h/imaging.h/drawing_ops.h is ratified spec-as-data with biting
  oracles; build the GrafPort verb layer (Std rect/rgn/bits dispatched via
  grafProcs) on the existing blitter_*_clipped as the SINGLE drawing path, ONE
  palette module from clut.h (kills the 5 duplicated index->RGB switches +
  centralizes the scattered bpp depth-conversion), CopyBits + 8 src modes +
  maskRgn + colorize + one offscreen GWorld. Land palette -> verbs -> per-Manager
  flip -> CopyBits/GWorld, each mutation-proven so make test stays green.
- **Seat B (kernel/boot engineer + risk skeptic):** ship the SMALLEST live
  desktop first -- a static shell_render scene + modal FILE COPY to the LFB on
  the EXISTING cfill path (NOT coupled to the verb refactor), gated by a new
  test-flair-desktop (QEMU screendump structural-band + Bochs serial). Bring up
  the live loop in 3 on-metal steps (tick -> keyboard -> mouse/IRQ12), the mouse
  leg Bochs-gated for the dual-PIC EOI. Fidelity = host-model-PER-MODE (AM-6),
  SSIM guide-not-gate. Land 86Box at M4, do NOT waive DEC-04. **Headline risk
  R1: the kernel window is far tighter than memory_map.h claims.**

## Chair ruling (no gridlock -- the seats converged)

GROW the impl (Seat A architecture) executed on Seat B's bisectable sequence:
**B0 kernel-window -> M3.0 link (gated on the assert) -> M3.1 static live desktop
on the cfill path; the verb-layer refactor lands in PARALLEL, off the first-pixels
critical path, re-greening test-chrome/drag/fb-agree before merge** (folding in
Seat A's depth-conversion centralization). Recorded as `bd memory
flair-phase-3-committee-ruling-2026-06-21`; Phase-3 DAG filed re30.1-.9 with deps.

## B0 (re30.1) -- what changed

Verified Seat B's R1 myself (Law 4): `nm build/kernel.elf` -> _kernel_end=0x28890,
PROGRAM_BASE=0x38000, headroom ~61 KiB -- and memory_map.h:33 was STALE (claimed
~0x1fd20, off by ~36 KiB). Then found the BOOTING kernel (kernel_shell.elf, the
TRACER_IMG kernel) is TIGHTER still: _kernel_end=0x30920, only **~29 KiB
headroom** < FLAIR's ~51 KiB -- so M3.0 WILL exceed the window.

- **os/milton/kernel.ld:** top-level `ASSERT(_kernel_end < 0x38000, "...")` AFTER
  the SECTIONS block, so a window bust is a LINK error (Rule 2), not silent
  boot-time corruption of the program-image arena. kernel.ld is a prerequisite
  of every kernel link rule, so it protects kernel.elf, kernel_shell.elf, and all
  test-kernel variants.
- **spec/memory_map.h:** corrected the stale headroom comment to measured reality
  (kernel 0x28890/~61 KiB; booting shell kernel 0x30920/~29 KiB) and recorded
  that the +0x8000 PROGRAM_BASE raise is pre-authorized as the M3.0 mitigation.

## Frictions (Law 4 -- caught by re-running, not trusting)

A bare `ASSERT(...)` INSIDE the SECTIONS block is a GNU ld **syntax error**
(ld:kernel.ld:44). My first "mutation passed" was therefore a FALSE POSITIVE --
the link failed on the syntax error, not the assert firing. Moved the ASSERT to
top-level (the portable form) and RE-proved: at bound 0x30000 the shell kernel
(0x30920) fails the link emitting the real message
`ld: kernel image busts the PROGRAM_BASE window ...`; restored to 0x38000 links
green. The guard genuinely bites (Rule 6).

## Acceptance

- make build/kernel.elf + build/kernel_shell.elf + make image: all GREEN with the
  restored 0x38000 assert.
- ASSERT mutation-proven (fires WITH its message at 0x30000, not a syntax error).
- kernel.ld + memory_map.h ASCII-clean. Commit 50c1374 pushed.

## Next

- **re30.2 M3.0:** the +0x8000 PROGRAM_BASE raise (Rule-8 whole-map shift,
  0x38000 -> 0x40000, 192 KiB window) mirroring o0td EXACTLY -- preserve SAMIR's
  heap arena byte-identical (the Wave-4 raise-alone broke it; o0td's pairing did
  not), then link the Manager set into KERNEL_OBJS. **FULL `make clean && make
  test` incl. emu + Bochs** (kernel/memory-map change -> tri-emulator obligation,
  Rule 5; the WL-0028 hard lesson).
- **re30.3 M3.1:** static live desktop via shell_render + the test-flair-desktop
  screendump gate -- the milestone that puts the chimera desktop on the 386.
- Verb-layer track (re30.4-.7) runs parallel; input arc re30.8/.9 + initech-26d.

## Pointers

- os/milton/kernel.ld (top-level ASSERT), spec/memory_map.h (corrected note)
- Plan: docs/plans/FLAIR-implementation-plan.md Phase 3
- bd: initech-re30 (.1 closed; .2-.9 open), memory flair-phase-3-committee-ruling-2026-06-21
