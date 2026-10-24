# WL-0178 -- PM session 2026-10-23/24, 21:00-: wave 2 after the audit fix wave -- features and bugs by lane, merged as they came

## Context

The operator wound the 2026-10-23 audit fix wave down at the Codex reset, then at 21:00 asked for
work to resume: orchestrate, a mix of bugs and new features, Codex gpt-6.1-sol xhigh for coding
and review, Opus for coding, an occasional Fable lane told about the quota app, 2-3+ concurrent
lanes, merges landing at velocity, no review spirals, commit and push regularly. Quota band:
up to and a little over pace on both Claude and Codex; Fable within Claude + 2.

## What changed on main (lane merges, in order; PM commits between them)

| Lane | Model | Beads | What |
|---|---|---|---|
| calcquit | Sonnet (from wave 1) | qxft | InitechCalc: repeated Save keeps the model; staging never clobbers TABLES.TMP |
| panic | Codex | 480p, dbp1 | typed loader/startup reasons on the PC LOAD LETTER screen |
| i123cells | Opus | 3qtt, 3s12 | oversized first column clipped to the area (real R2.2: >72 clips to 72); default-cell erase frees; absent cells keep default protection |
| iwordnav | Opus | 1zk9, 10bx | PgUp/PgDn + Go To (real WP 5.1 mint); "Invalid file name" on spaces |
| calcda | Codex | 0xs6 | Calculator answers the shutdown quit query; refused quit explained on screen |
| execguard | Fable | 13ig (P1) | flagship EXEC COMMAND.COM -> resident COMMAND /C; COMMAND3 overlay refused by content; transient imports derived at build time |
| itextundo | Opus | 4hkk | one-level Undo in InitechText (Mac guide 6.0 p.118) |
| finderopen | Fable | fbo4 | Finder File > Open / Ctrl-O launch a selected application through the one launch spine |
| samirabbr | Codex | 13d3 | four-letter abbreviations; REPLACE scope grammar; three real-III+ mints |
| menubars | Codex | c8w7 | preference redraw through the shell compositor (both bars, clock, foreground menu) |
| i123copy | Codex | fyji | /Copy destination rectangles + shared POINT selection (real R2.2 mints) |
| doscmd | Codex | f8f0, 0qw1 | DEL <directory> with the DOS question; "Terminate batch job (Y/N)?" incl. Ctrl-C during an EXEC child (real DOS 3.30 mint) |

Rule 8 notes (locked spec / controlled vocabulary changes, each with its bead):
- spec/dos_messages.json + ADR-0003 Appendix C: MSG-DOS-0063 "Terminate batch job (Y/N)?"
  (MS-DOS 3.3 Reference PAUSE, printed p.149; initech-0qw1). Catalogue route, no dosmsg.allow
  exception.
- spec/iword/listfiles.h + spec/iword/fkeys.json: List Files keys 2/3/8 (Delete, Move/Rename,
  Copy) live=1 (real WP 5.1; initech-7aou) -- pending the lane's merge.
- i123 test-i123-range-menu R6 re-keyed: /Range Value into protected empty cells is refused
  (it only passed because of the B02 bug; initech-3s12).

## Frictions

- Agent isolation:worktree spawned three lanes from e67b8311 (not HEAD) and the harness removed
  the unchanged worktrees when the STEP-0 gate stopped them. Remediation (bd memory updated):
  the PM cuts ../initech-os-wt-<lane> at the pinned sha and spawns without isolation.
- Two merges landed Makefile hunks that were green in the lane and red on main: a
  read-before-definition (TRANSIENT_IMPORTS_LD) and a bare-path fixture volume with no derived
  contract. pm-merge now always runs test-makefile-vars, test-fixture-volumes and
  test-pubs-lint before the lane's gates.
- A grep-derived gate list picked up test-samir-fns-pending, which names a missing target
  (bead filed); gate lists are now filtered against the registered TEST_*_GATES vectors.
- The pubs period-year lint read "about 2000 characters" as a year (reworded).
- Tail-append Makefile conflicts (two lanes) resolved by keeping both sides in the lane
  worktree, then re-queuing.
- Codex lanes stop correctly on Rule-5 questions (doscmd: locked message; iwordlist: locked
  List Files table; calcda: no real-Mac paste fixture) -- each got a PM ruling or a follow-up.
- SAMIR.COM: DISPLAY STATUS/MEMORY at full fidelity costs +4,664 B (2,210 B over the reclaim
  cap); a size round runs on the lane before merge rather than stripping behaviour.

## Acceptance

Every merged lane re-verified by the PM on main (its families + mutants + the three lints +
headroom; Bochs legs where the family has them: panic, execguard x3, finderopen, menubars,
i123copy x2). Eleven Rule-14 clips recorded and repro-proven on main after the merges
(film_panic, film_scene, calculator_power, calculator_refused, i123_wide, iword_goto,
iword_badname, itext_undo, finder_open_app, cpanels_pattern_bars, i123_copy_tile): operator
eyeball owed. Full `make clean && make test` certificate: NOT run on this HEAD (first job next
session, ../initech-os-cert).

## Pointers

- Reviews: one gpt-6.1-sol read-only review per lane, one fix round max (every FIX-THEN-MERGE
  tonight was a real defect: protection on absent cells, REPLACE scope grammar, five unchecked
  transient imports + two unguarded loader entries, a uint8 launch count, a clip image).
- Follow-ups filed: uyi1 (Range Name protection), 0zm4 (B02 emu replay), kz49 ([Wn] width tag),
  COMSPEC on the flagship, iword goto emu leg, Go To completions, test-samir-fns-pending.
- fxrq (Calculator Copy/Paste) blocked on a real-Mac paste mint (bead note).
