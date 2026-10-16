# WL-0134 -- The Initech 123 manuals

## Context

One-shot writing and gating lane for bead `initech-ipu5.9` (epic
`initech-ipu5`): the Initech 123 books of the Documentation Set, PUB-REF-0001
Sec 12 rows 123-REF, 123-SET and 123-QR, family F-LOT, modelled on the Lotus
1-2-3 Release 2.2 set (`docs/pubs/originals/lotus-123-r22.md`, the scans in
`../initech-manuals-ref/lotus123/`). Worktree `lane-l123man`, branch
`lane/l123man`. CLAUDE.md, PUB-REF-0001, the Lotus annex, the pilot InitechDOS
books, the class, `pubs.mk`, `harness/pubs/*`, WL-0119 and WL-0125, the
Initech 123 plan, `spec/i123/*` and `os/i123/**` were read first. `bd` could
not open the shared database (another writer held the embedded lock), so the
bead texts were read from `.beads/issues.jsonl`. Two Sonnet helpers drafted
the @Functions chapter and the five refusing-command chapters (Add-In, Data,
Graph, Print, System) under a written brief; their output was gated here.
Operator directive 2026-10-17: the slogan `\IsThisGoodForTheCompany` at the
title verso, the foot of the Reader Comment Form, the back covers and the
first page of the Setting Up procedures chapter (`\providecommand` until
the design lane's class macro lands).

## What changed

**The books** (`pubs/manuals/`):

| Book | Part | Pages | Contents |
|---|---|---|---|
| `initech123-reference` | 1051-0000-10 | see Acceptance | About This Book; Ch. 1 Using Initech 123; Ch. 2-12, one per main-menu command in menu order (AN-08 opener with a menu tree generated from the shipped resource, Command/Task table, Common Uses, Related Reading, Terms, AN-09 entries with option tables); Ch. 13 @Functions (AN-10, 90 entries, each with a replayed example); App. A Worksheet Capacity; App. B Troubleshooting (Problem/Solution, C-16); Glossary; three-column index; NOTES leaves; colophon |
| `initech123-setting-up` | 1051-0000-11 | see Acceptance | One volume, four parts with half-titles: Setting Up (SU, 3 ch.), Tutorial (TU, 17 lessons in 5 ch., AN-17 keystroke tables), Quick Start (QS, 3 ch. with Task Summary / Other Tasks), Sample Applications (SA, 4: share of quarterly results, loan payments, depreciation, commission); Master Index with SU/TU/QS/SA codes; rating-grid Reader Comment Form; NOTES leaves |
| `initech123-quick-reference` | 1051-0000-20 | 40 | PK-11 booklet (3.5 x 9 in): control panel, keys, prefixes, operators, the generated menu map (refusing commands daggered), all 90 @functions, command summary, messages, task reference |
| `initech123-templates` | 1051-0000-21 / -22 | 4 | PK-14 101-key strip and 84-key L template, printed from the shipped key map |

Each has `cover.tex` (front and back cover), the two perfect-bound books a
`spine.tex`; the saddle-stitched booklet has none (PK-03, under 6 mm).

**The gates** (C, make, thin shell):

- `harness/pubs/i123_face.c` -- the SHIPPED `os/i123/app/face_a.c` compiled
  on the host over the core and the shipped menu resource, with a recording
  Toolbox mock. `inventory` obtains the shipped surface by driving the
  application: 381 menu paths (the resource walked), 90 @functions (each
  typed into a cell; stored = shipped), 14 keys (each pressed in five
  contexts; a key that changes nothing is absent). `refusals` lists the 224
  leaves that answer `Command not implemented`. `tree`, `outline`, `keymap`
  generate the AN-08 trees, the Quick Reference menu map and the template
  key map. `tokens`/`run` turn a `.k123` key script into QEMU tokens or a
  host run; `replay` replays a printed `.i23` example.
- `harness/pubs/i123_gate.c` -- GC-01 both directions by NAME|KIND (menu,
  function, key) over the books' `main.cmds` (the one `\RegisterCommand`
  mechanism), plus the state rule: a menu path is DRAFT exactly when the
  application refuses it (`DOC-GATED-REFUSED`, `STALE-DRAFT`); mentions;
  CP-03 terms vs Glossary.
- `harness/pubs/flair_shot.c` + `pubs/screens/flair_capture.sh` -- the
  NT-21/NT-22 pixel route for FLAIR: the 123.EXE launch trace of
  `spec/flair_i123_traces.mk` + the figure's keys (50 ms apart), pointer
  parked on the menu bar before the last key, QEMU screendump, crop, grey
  through the table built from `spec/assets/clut.json` +
  `color_canon.json` (any other colour fails loud), 4 x integer replication,
  PNG at 600 dpi. A capture is refused unless the tenant received exactly one
  keyDown per key. Figures are committed as base64 (`screens/*.b64`):
  pubs/ is ASCII-only (GC-05), and the build decodes them into
  `build/pubs/gen/i123shots/`.
- `harness/pubs/i123_replay.sh` -- GC-03 replay of every `\LotExample`, and
  GC-11: the quarterly shares total 116%.
- `harness/pubs/i123_mutants.sh` -- ten named mutants (below).
- `pubs/class/initech-lotus.sty` -- the F-LOT furniture the class did not
  have, loaded by these books only (class untouched).
- `pubs/pubs.mk` -- the wiring: `test-pubs` gains `test-pubs-i123` and
  `test-pubs-i123-examples`; `test-pubs-screens` gains
  `test-pubs-flair-screens`; `test-pubs-mutant` gains
  `test-pubs-i123-mutant`; the DOS COMMAND-SURFACE loop exempts the
  initech123 books (they have their own inventory);
  `harness/pubs/check_screens.sh` accepts the FLAIR route.

## Why

D-14 and GC-01 together decide the shape of these books. The menus of the
shipped application are complete (381 items, all navigable), but 224 of its
leaves refuse. Every one of them is documented, because it is on the screen,
and every one is DRAFT, because it does nothing; the gate fails the book if
either drifts from the build. The P4/P5 features (/Data, /Print, /Graph,
macros) therefore appear as DRAFT entries inside complete menu chapters; no
Macros chapter is written (nothing of it ships, and no menu item names it).

## Frictions

- The publications lint rejects any four-digit number above 1993, so the
  numbers of the examples were chosen below 1994 or written with commas
  (8,192 rows).
- The shipped formula display drops the leading `+` of a formula that begins
  with an absolute reference (`+$A$1*2` shows `$A$1*2`); the examples avoid
  it. Product finding for the PM (Release 2.2 shows `+$A$1*2`).
- Typed entries are capped at 74 characters (`SCR_COLS - 2`), not the
  240 of Release 2.2; the books state the shipped limit.
- Under host load QEMU key injection dropped keys; the capture now holds
  50 ms between keys and refuses any capture whose key count is short.
- `bd` held by another process all session: no bead was updated.

## Acceptance

(see the final section, filled at the end of the lane)

## Pointers

`pubs/manuals/initech123-*`, `pubs/class/initech-lotus.sty`,
`harness/pubs/{i123_face.c,i123_gate.c,flair_shot.c,i123_replay.sh,i123_mutants.sh}`,
`pubs/screens/flair_capture.sh`, `pubs/pubs.mk` (Initech 123 section).
