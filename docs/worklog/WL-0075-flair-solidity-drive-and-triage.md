# WL-0075 — FLAIR solidity: the first-person drive battery, the three-painter diagnosis, the oracle-first RED foundation

**Date:** 2026-07-21 · **Session:** solidity triage (operator-directed pivot from B8)
**Baseline:** `bfac0d3` — re-verified first-person: `make test` ALL GREEN **320 host + 83 emu** before any work.
**Epic:** `initech-av7s` (label `flair-solidity`).

## Context

The operator redirected the session from the Turbo Initech B8 step to FLAIR:
"the windowing system must be solid and dependable... a dozen little visual
quirks and the sum total is a feeling of real mess. Use whatever feedback
mechanisms you can to truly test the dynamical nature of it." This shard
records a diagnosis-and-foundation session: no artifact code changed; the
oracle foundation and the full campaign structure landed.

## What changed

1. **The drive battery** (`harness/emu/drive_flair.py`, dev aid): 15
   deterministic QMP-injected boots of `flair_tenants.img`, marker-gated
   screendumps, every frame inspected first-person (the operator's Law-4
   eyeball, delegated to the agent's own eyes). Found the live desktop
   **degrades monotonically while all 83 emu gates stay green.**
2. **The three-painter diagnosis** (full detail:
   `docs/FLAIR-solidity-drive-2026-07-21.md`): boot painter, compositor, and
   activation dispatch render the same windows under NO shared invariant —
   white content holes (validate-before-delivery, `desktop.c:269-273`;
   route only on switch, `kmain.c:2406-2413`), half-active title bars
   (0→1 seeds nothing, `window.c:226-235`), chrome divergence (tenant
   content erases the scrollbar column), dead band-2 menus (y<20 test only,
   `bar_sys` hard-coded), menu-cancel never restores (deliberate demo wart,
   `kmain.c:1276-1285`), ghost drag, no drag clamp, zoom→drag misroute,
   `win -1` serial identity. Three parallel read-only forensics agents
   produced file:line roots for every symptom; all claims re-verified against
   the live frames.
3. **Oracle-first RED foundation** (committed, NOT yet wired into `make
   test`): `tools/ppm_flair_solid_check.c` (legs A/B/C) + LOCKED traces
   `spec/flair_solid_traces.mk`. Each leg proven RED against the captured
   frames for the right reason (A/B: #FFFFFF where canon #C0C0C0; C: zero
   pinstripe rows + an 18-row stale black run at x=359, with the HELLO-flat
   sub-check passing — the grader discriminates).
4. **Beads:** epic `initech-av7s` with 17 children — 8 new (incl. P1 roots
   `gofc` update-contract, `rqz5` activation-invalidate), 6 annotated with
   forensic roots (`0zxp` `t1rv` `7tjp`→P2 `8fhu` `9d0e` `vfd8`), 4 adopted
   (`javs` `l0mh` `ci4o` `8kx3`).
5. **Design committee** on DQ1-DQ10 (validation ownership, pump ordering,
   activation seeding, close semantics, raise-on-click, drag clamp, menu
   restore, band-2 dispatch, gate shape, non-goals) was launched and
   STOPPED before seats completed (operator wrap-up). The DQ list is fully
   recorded in the drive report §6; resume or re-launch next session.

## Why

Law 2's blind spot made this arc necessary: every defect lives inside a
green gate vector because the existing FLAIR oracles grade single
transitions (one switch, one drag) and never COMPOSED interactions, and no
oracle asserts tenant content SURVIVES a WM operation. The new legs grade
exactly the invariants the users feel ("my window's content is still there
after I drag it").

## Frictions

- **QMP unix socket `sun_path` 108-char cap**: a deep `--out` path makes
  QEMU exit=1 with an EMPTY log — looks like a boot failure, is a socket
  bind failure. Use short out dirs (recorded in the drive report + bd memory).
- The harness injects `--keys` before `--mouse` (fixed order), so
  keys-after-close (8fhu) could not be probed live; code forensics confirmed
  it instead.
- The appswitch grader's header still records the pre-a9iq "renderer has no
  active/inactive chrome distinction" assumption as a NOTE — stale, but
  conveniently means the `rqz5` fix will NOT flip that gate (verified).

## Acceptance

- Baseline certificate re-verified ALL GREEN 320+83 (this session, before
  and untouched — no artifact code changed).
- solid_check legs A/B/C RED-proven against live captures (Rule 1's
  fail-first, on the real defect, before any fix exists).
- No gate weakened, no locked spec touched (the new traces file is NEW
  locked data under the epic, Rule 8 deliberate-act note = this shard).

## Pointers

- `docs/FLAIR-solidity-drive-2026-07-21.md` — evidence catalog, forensic
  citations, canon rulings, DQ agenda, wave plan (the campaign home).
- `tools/ppm_flair_solid_check.c`, `spec/flair_solid_traces.mk`,
  `harness/emu/drive_flair.py`.
- `bd show initech-av7s` — the epic + 17 children.
- Next agent: run/resume the committee (drive report §6) → ratify DQ1-DQ10
  → Wave A (the repaint contract, `gofc`+`rqz5`, first-person on main) →
  Wave B lanes → wire `test-flair-solid` + mutants → full aggregate + Bochs.
