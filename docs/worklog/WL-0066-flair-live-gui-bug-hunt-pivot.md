<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0066 -- The FLAIR live-GUI testing pivot: 138 real bugs found by actually driving the desktop

**Type:** operator-forced QA pivot (mid-session). Feature work HALTED.
**Date:** 2026-06-28.
**Beads:** 138 new bug beads (label `gui-bughunt`), 2 P0 + ~20 P1 + the rest P2. Report:
`docs/FLAIR-GUI-bug-hunt-2026-06-28.md`. Lesson: `bd memories flair-live-gui-testing-gap`.

## Context (the failure)

Earlier this session I orchestrated FLAIR Phase 4.5 (ubd0 split-arena + Resource Manager +
Scrap + TextEdit + List + Standard File), each graded by GREEN host oracles + a single
boot-frame screendump, and shipped 5 commits. The operator then ran `make run-flair-tenants`
and found ~5 bugs in 10 seconds, calling it "dreadful unacceptable dross". **They were right.**
I had never DRIVEN the live desktop -- the host oracles "grade math, not composited pixels or
live behaviour" (initech-pipa's own words), and there were already 4 filed live-GUI bugs
(pipa, 34gp, rgt8, z1f5) I built straight over. A deep-research round on how to debug these
apps (bead `initech-l9cd`) existed and I had not applied it.

## The method (recovered + now standing)

`build/qemu_harness --disk build/flair_live.img --mouse 'm<dx>:<dy>,l1,l0' --keys SPEC
--keys-after FLAIR-LIVE-READY --screendump --screendump-after <MARKER>` (markers
FLAIR-LIVE-READY / FLAIR-DRAG / FLAIR-MENU / FLAIR-CLOSE from os/milton/kmain.c). Convert
the PPM to PNG (`pnmtopng | convert -filter point -resize 200%`) and READ it; sample exact
RGB from `build/*.ppm` with python. Compare composited pixels to the `../system7-decomp`
goldens (Law 4). NOTE: the harness mouse Y axis is INVERTED (rel +y -> cursor up).
I drove flair_live / flair_tenants / the drag / menu / app-switch scenes myself first and
confirmed real bugs by eyeball + pixel-sampling before scaling.

## The hunt (3 rounds, each adversarially verified)

Three fan-out workflows (inspector lenses -> opus adversarial verify killing NOT_REAL/canon/
dup -> opus synthesize/dedup). I independently re-verified the 2 P0s + sampled others against
source (Rule 4); the verifier's precision was high (NOT_REAL rate 27/93, 27/82, 6/54).

- **Round 1 (49 bugs)** -- live GUI + Phase-4.5 modules: the compositor/App-Contract path
  (drag + app-switch ERASE peer windows; menu-bar clobber), NO active/inactive window chrome
  at all (hilited never read), placeholder chrome (apple black square, gray menus, no grow box
  / h-scrollbar, 0% progress bar), + a latent C tail.
- **Round 2 (45 bugs, 1 P0)** -- un-inspected managers + DOS: **P0 initech-ojxn** COPY <file>
  <file> truncates to zero + reports success (OPEN src -> CREAT dst truncates the SAME file ->
  READ 0; independently code-verified); P1 dos_read no Carry-Flag check; heap overflow/LIFO/
  alias bugs; the FindWindow hit-zone family; make_offset_view underflow -> OOB write.
- **Round 3 (44 bugs, 1 P0)** -- SAMIR/dBASE + seed compiler + ANSI + region/FAT: **P0
  initech-mswo** region_op xmerge scratch[256] stack overflow when na+nb>256; SAMIR ROUND
  negative-floor, CTOD calendar validation, .dbf +2-form header, .ndx duplicate-key delete,
  SKIP/LOCATE off-by-ones, ?/?? logical render heresy; + many oracle gaps (Rule 6).

## Acceptance / totals

**138 new bug beads filed** (label `gui-bughunt`) + 4 known = **142 distinct verified bugs**.
2 P0 (data loss + stack overflow), ~20 P1, the rest P2. Every bug cites file:line or a frame +
pixel region, with expected/actual/repro. `bd stats` Total 438 -> 579. NO feature code shipped
during the pivot; the uncommitted `os/flair/stdfile.*` (Lane F, halted) is left untracked.

## Frictions / lessons

- A structural/host oracle is NECESSARY but NOT SUFFICIENT for a GUI. Law 4 demands driving the
  live desktop + grading COMPOSITED PIXELS vs the decomp goldens. The biggest single gap: the
  compositor does not damage-track the non-WindowMgr layers (menu bars, modal, shadows,
  scrollbars) NOR repaint background windows -- so drag/app-switch erase content.
- There is no active/inactive window chrome distinction (the root visual bug); `flair_draw_
  document_window` has no `hilited` param.
- The oracle-gap findings show several guards are mutation-unproven (Rule 6 holes) -- the test
  suites pass while the guarded path is dead/uncovered.

## Pointers / next

- `docs/FLAIR-GUI-bug-hunt-2026-06-28.md` (all 138, round-by-round). `bd list` label `gui-bughunt`.
- **NEXT (operator's call):** a FIX arc. Suggested order: the 2 P0 (initech-ojxn self-COPY data
  loss; initech-mswo region stack overflow) -> the ~20 P1 (compositor damage-tracking +
  active/inactive chrome are the highest-impact GUI fixes; dos_read CF; SAMIR result bugs) ->
  the P2 tail. AND: build the deterministic GUI-interaction capture (`initech-l9cd`) + add
  composited-pixel emu gates so this class of bug cannot ship green again.
