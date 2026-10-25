# WL-0188 -- InitechBase ship plan, day 1 (2026-10-25)

## Context

Picked up after a network outage interrupted wave 1. The operator approved
docs/plans/INITECHBASE-ship-plan.md, ruled "kernel overlays first", ratified
ADR-0014 with 5A, and later ruled that SAMIR loads its overlays the DBASE.EXE
way (not EXEC AL=03h). The session was stopped by the operator at about 15:50
for being over the quota pace.

## What changed (main)

- Factory speed-ups:
  - F4 9baae0e1: conflict-free registries.
  - F1 0538d5a1: source-level manual lockstep, no LaTeX per merge.
  - F2 c2ba6e16: test-merge-samir at -j64 in ~70-90 s, against 792 s serial.
  - W4.0 screen-mint 612c507d.
  - W2 real III+ goldens f374fe02.
- SAMIR overlays Phase B c0ddb049: SAMIR.EXE + SAMIR.OVL.
- Wave 1 complete: SAY 4aaa0f72, NDX 3c194c50, LOG/SQRT + EXP fix 145e1fbc.
  18 functions are graded against real III+ goldens.
- Kernel overlays Lane 1 ff5dd607: FLAIR.OVL, flagship kernel -528 B.
- SAMIR resident shrink cce36cdf (-960 B). Resident is now 86,832 B, with
  6,944 B headroom.
- Fixes:
  - 09779549: repinned the os/version golden hashes. Red since batch 1; a PM
    omission.
  - 4884df08: ADR citation fix.

## Why

Per the ship plan DAG:
- speed up merges first;
- free SAMIR room via overlays;
- land wave 1 against real dBASE III+ captures (Law 2).

## Frictions

- Cold `make -j64` runs are flaky. Nested $(MAKE) calls rebuild shared images
  and kernels while other legs boot them, which shows up as triple faults or
  empty .elf files. Every merge needed a warm rerun. Tracked as
  initech-1x6x2.1, P1.
- Codex Sol hit "model at capacity" several times. Its safety filter refused a
  kernel-internals review, so kernel reviews go to Sonnet (bd memory).
- Lanes repeatedly inferred III+ display and error rules from too few samples.
  Mint a range of cases before writing a rule.
- A lane committed CODEX-OUT.md to main; it was removed and gitignored.

## Stopped mid-flight (all pushed, all UNVERIFIED)

- origin wip/recorder-sync: record-flair determinism (initech-x8gsi.1).
- origin wip/f7b-build-race: cold build race (initech-1x6x2.1).
- origin lane/kov2-r: kernel overlays Lane 2, rebased onto main. It is held
  for the recorder fix (operator ruling) and needs a review.
- origin lane/phaseA-dec19: DEC-19 AL=03h. It merges after Lane 2, and its
  deviation notes await the operator.

## Acceptance

Each merge ran test-merge-samir green on merged main, plus targeted
kernel/desktop/pubs gates as listed in each merge commit. The full
`make test`, Bochs and 86Box sweep and a full pubs typeset were not run this
session.

## Pointers

- Beads: initech-1x6x2 (ship epic) and its children .1 to .7; initech-x8gsi
  and .1; initech-dewv; initech-1x6x2.7 (numeric display: scientific
  fallback).
- Open operator decisions:
  - W2 omission candidates: SET DEBUG/ECHO/STEP, CALL/LOAD/IMPORT/EXPORT.
  - The DEC-19 deviation notes.
