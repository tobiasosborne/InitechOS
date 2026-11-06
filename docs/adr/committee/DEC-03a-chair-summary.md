<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- CONTROLLED DOCUMENT -->
# DEC-03a -- Chair's recommendation to the operator

Date: 2026-11-06 | Bead: initech-yndl2 | Status: RATIFIED 2026-11-06 by the operator (as drafted); v1.0.
Authority: the operator RATIFIES; the chair drafts. No implementation approval is claimed.
Record: [amendment](../ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md),
[Seat A](DEC-03a-seat-A.md), [Seat B](DEC-03a-seat-B.md); memos copied byte-for-byte.

Seat A: approve in principle; 1993 protected-memory precedents; separate
[0x600000,0x800000) region, 8 MiB machine, new tag/native ABI, low PSP/high stack.
Seat B: MZ-only high execution; freeze flags; retain overlay/real-III+ gates;
class-2 full-console tenancy; 256 KiB ceiling; W4 files + structure + clips.

Chair resolutions / overrides (there is no opposing seat vote):
- Global 8 MiB floor, 7168 KiB extended; decline A's optional 6 MiB boot profile.
- Distinct 0x5849/v1 model; legacy 0x4943, flat apps, TSRs and CHKDSK stay as-is.
- A's low PSP/native 48h/49h/4Ah ABI and high 64 KiB stack; one high owner only.
- B's proposed 256 KiB module ceiling adopted; no floor or byte bargaining.
- Refine B's reserve shorthand: link reserve <= declared loader reserve AND window.
- Freeze SAMIR_SIZE_CFLAGS; new code resident by default, leaf overlays optional.
- W4 only re-graded; W3 exact screens stay. HELP authored samples labeled honestly.
- SAMIR.OVL retained; no nested-swap programme; desktop bit-identity stays exact.
- Correct seats' baseline: h4bk is merged on main 797d87c5; recorded margin now
  1,220 B (168,764 / 169,984), not 2,624 B. No kernel-cap waiver.

Open at ratification (only confirmations that change code/gates):
1. Global floor/window plus contiguous-PC BIOS-fallback assumption and known-hole refusal?
2. New tag/descriptor, low PSP/native ABI, high stack/guard, second-high/KEEP refusals?
3. 256 KiB ceiling/declared reserve and W4 inventory scope, retained exact screens/HELP?
Defaults are the chair's choices above; implementation remains gated on operator sign-off.

Bead dispositions to apply after ratification; no bd writes made here:
- initech-5etom: close delivered in part; credit DEC-19 and initech-5etom.1 (closed).
- initech-1x6x2.18: close won't-do/superseded; nested-swap/shrink scope retired.
- initech-1x6x2.6: already closed; keep delivered lifetime guards, no reversion.
- Other size children named by seats: none; do not retire unrelated .1-.17 siblings.
- initech-9x8cn: keep W4.1, replace acceptance with DEC-03a.10 L1/L2/L3.
- initech-j7lld: keep W4 epic; children inherit per-app acceptance table.
- initech-y29t4: keep W5; verify inventory and disclose non-byte-exact W4 screens.
- initech-p3nn0: unchanged W3 exact-screen scope; slices 2-4 next.
- initech-h4bk: delivered on main; cross-link integration/TSR regressions.
- initech-e7qc / initech-x0i: windowed front-end parked / period evidence still owed.
Seat B export confirms .6/.18; live bd cannot resolve initech-yndl2 here.
Tracker owner must reconcile current states; these are proposed dispositions.

Follow-up beads to file after ratification (titles, not allocated IDs):
- DEC-03a high-model runtime: loader/allocator/crt0/PAL, ownership, stack and lifetime.
- DEC-03a locked memory/ABI: spec/memory_map.h, hardware.json, format/INT21 contract;
  Rule-8 provenance/worklog, global boot floor, known-hole refusal and DEC-13 diagnostic.
- DEC-03a minimum-RAM factory: harness/emu explicit QEMU -m 8M / Bochs megs:8,
  5 MiB/7167-vs-7168 negatives, QEMU+Bochs and initech-x0i period run.
- DEC-03a size/link gates: Makefile/samir_size.mk/samir.ld ceiling and reserve checks;
  keep slot/minalloc/repro, real captures and re-prove suspend/lifetime/TSRs.
- DEC-03a W4 acceptance: locked spec/samir/screen_regrade.json, per-app L1/L2 mutants,
  L3 clips, retained exact screens and W5 inventory certificate; update 9x8cn/j7lld/y29t4.
- DEC-03a publications lockstep: README, PRD/hardware references and manuals to 8 MB;
  fix 4 MB-vs-5120 KiB drift, re-mint gs-dir.txt, preserve CHKDSK screens, run pubs.
- DEC-03a capacity allocation: re-point initech-1x6x2; release packaging/shrink seats
  to Turbo Initech M7/M8 fixpoint and FLAIR fidelity; kernel-cap work stays separate.
