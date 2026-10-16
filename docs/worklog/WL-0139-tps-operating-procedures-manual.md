<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0139 -- TPS Report Generator Reference and Operating Procedures

**Bead:** initech-ipu5.14 (depends on initech-5igf, written against its
decided strings; initech-9yw6 research; initech-3p1a, initech-8479.1)
**Lane:** tpsman (Opus), branch lane/tpsman
**Book:** DS row DOS-TPS, part 1024-3300-12, family F-OPS, 80 pages

## Context

The operator directive of 2026-10-16 asked for a TPS Report Generator manual
"replete with mind-numbing busywork irritations", documented straight in the
voice of an operations procedures binder, with every keystroke sequence
replayed against the emulator. The 2026-10-17 Law-4 ruling (bead
initech-5igf) moved the seven space-bar presses out of the program: the
manual is the only place the count is written. The PM decided the R2
alignment-test-line loop and the R5 form-id prompt the same morning; lane
tpsr2 implements them in parallel, so this book is written against the
DECIDED wording, with DRAFT markers wherever that wording appears.

Model: IBM PC DOS Operation Training ZR23-6845-01 (session frame,
objectives, lab projects, study questions, answers, review, final quiz,
data collection sheets, internal-mail Reader's Comment Form; scan pages
viewed with pdftoppm) and the DisplayWrite 4 Getting Started step box
(title interrupting the top rule, p. 13 of the scan). Structure from the
research brief Sec 5. Structure and furniture modelled; no prose copied.

## What changed

**The book** (`pubs/manuals/tps-report-generator/`): title page and verso
(AN-20, with the house slogan), Preface (AN-21 F-OPS sub-heads: Getting
Started, Course Description, Prerequisite, Duties and Tasks), Contents,
seven sessions (Introduction / Objectives openers, PG-21), the eight boxed
procedures TPS-P-01..08 (AN-15: Purpose, Frequency, Performed by,
Prerequisites, Materials, numbered steps with response lines, Verification,
Records retained, Note), checklists TPS-C-1 Pre-Run, TPS-C-2 Post-Run,
TPS-C-3 Escalation, six lab projects, study questions per session,
Appendices A Operator Course Evaluation, B Terminal Procedures, C Answers,
D Review (DOS commands), E Final Quiz with supervisor answer sheet, F Data
Collection Sheets (Forms TPS-1..TPS-5, Rev. 10/89), G Messages (every
program message, the program's other lines, the InitechDOS device errors);
two-column index, colophon, internal-mail Reader's Comment Form; cover
(front and back) and spine.

**The generated inventory** (`harness/pubs/tps_inventory.c`, C): parses
the locked `spec/tps_ritual.json` and emits (a) the inventory
`NAME|tps|line...` (TPS-001..TPS-008, R2.prompt, R2.refuse, R2.accept,
R3.prompt, R4.prompt, R5.prompt, END), (b) `tps_messages.tex`, the table
the book prints every message from (GC-02 by construction), with the R2
count, and (c) the `check` and `reports` modes of the gate.

**The registration and the anatomy** (`pubs/class/initech-ops.sty`,
`initech-ops.lua`): `\TpsMsg`, `\TpsLine`, `\TpsMessageEntry` print from the
table and register `NAME|tps|GATED|NAME` in the one `.cmds` file;
`\TpsDraftMsg` / `\TpsDraftMessageEntry` print the decided wording and
register DRAFT; `\AlignCount{PLACE}` prints "seven (7)" from the spec and
registers its place; the `Procedure` and `Checklist` boxes; `\OpsShot`
(a span of a committed capture selected by TEXT, `fromtext`/`totext`, with
F-OPS reversed-square callouts and the three-column `CalloutLegend`,
NT-23/24); `\DraftShot` (a hand-written `.draft` transcript, DRAFT-marked
and listed); `\ReportLines` (lines of a program file); data-sheet forms; the
internal Reader's Comment Form. Screen and message text is set in Cursor
without TeX ligatures.

**The capture route** (`pubs/screens/capture.sh`, disk field `flair`): the
flagship image boots the FLAIR desktop, the locked trace of
test-flair-exit-to-dos leg 3 (FLAIR_EXIT_DOS_SPEC up to its W SHELL-READY,
derived in pubs.mk) chooses Special > Exit to InitechDOS and clicks Exit,
and the screen's `.keys` are typed at A:\> as k-tokens in the same stream;
`\Wtext\W` in a `.keys` waits for the guest (a second run typed ahead
overran the type-ahead buffer). Every flair `.keys` begins `\n\nCLS\n`.

**The gate** (`pubs/pubs.mk`): `test-pubs-tps` joins `test-pubs`: the
book's `.cmds` against the generated inventory both directions, TPSREP
external documented and shipped, the count in its four places (TPS-P-03,
FORM-TPS-4, STUDY, QUIZ), inline `\cmd` against the DOS inventory,
`\ReportLines` files limited to gate-proved files, DRAFT transcripts listed
on every run. `test-pubs-commands` skips the TPS book for that reason only.
`harness/pubs/tps_mutants.sh` (run from `test-pubs-mutant`): seven mutants.

**The class** (`initech-manual.cls`, operations branch only): "Session N."
and "Appendix A." openers, contents labels and recto feet (the label fixed
when the chapter opens); `\ShortTitle` for the verso foot.

## Why

Law 1: every statement about the program is read from
`os/milton/tpsrep_program.asm` and `spec/tps_ritual.json` (BACKSPACE
ignored; the hyphen accepted in both number fields; six/eight characters;
an empty report number never agrees; TPS-006 names SIGNATURE for a non-
dBASE file; the corrective-action line only when a report lacks its cover
sheet; one previous report kept). Law 2: every screen is a capture of the
running flagship, re-proved by test-pubs-screens; every printed program
file is one a named gate compares with the program. VO-06: no explanation,
apology or irony; escalation is "see your supervisor"; tedium documented as
diligence. No printer behaviour is documented: the program does no printer
I/O and the research brief's Sec 4 is unapproved.

## Frictions

- The startup time prompt's seconds depend on how long the desktop takes
  to go (host timing, even with `--rtc-deterministic`): a flair screen
  clears the screen first (CLS); capture.sh enforces it.
- `TYPE TPSPARM.DAT` shows the file's Ctrl-Z as a CP437 glyph, which the
  text route rightly refuses; the record is printed from the shipped file.
- The period-year lint caught 4-digit numbers above 1993 in captures (file
  sizes 7256 and 2432, report number 4417): the screens use `DIR /W` and
  report numbers 1014 / 1041 / 1015.
- The class's `Ligatures=TeX` turns `--` and `---` in Cursor into dashes;
  the TPS book uses a ligature-free Cursor. The DOS books' screen sty has
  the same exposure (not changed here; noted for the toolchain lane).
- **Pre-existing on main, not this lane:** test-pubs-screens is RED on the
  InitechDOS books' `chkdsk.txt` (CHKDSK memory 118448 -> 118320 bytes);
  the umbrella `make test-pubs` stops there. The PM must re-capture those
  books (`make pubs-screens`). Every other leg, and test-pubs-screens for
  the TPS book alone (`PUBS_MANUALS=tps-report-generator`), is green.

## Acceptance

```
>>> test-pubs-build: tps-report-generator/main.pdf 80 pages, 0 overfull, 11 DRAFT markers
>>> test-pubs-tps: green (15 shipped TPS messages, all documented; 39 registrations, 2 DRAFT; alignment count written in 4 places; TPSREP documented)
>>> test-pubs-screens: green (10 screens in the manuals == live OS output)
>>> test-pubs-mutant: green (7 TPS-manual mutants rejected by their gate legs)
>>> test-pubs-repro: green (10 PDFs byte-identical across two clean builds)
>>> test-pubs-families: green (7 model families typeset with their required body faces)
```

DRAFT markers (11): the verso address check (PUB-REF O-5/D-12, as the
pilot); R2.prompt (session text, appendix entry), the reprint after a
refusal, the r2align transcript; R5.prompt (session text, appendix entry),
r5cover and r5refuse transcripts (all lane tpsr2 / initech-5igf); MSG-DOS-
0006 and 0008 (no code path displays them, bead initech-vlhp, as the
Reference).

## Where to resume (after lane tpsr2 merges)

1. Remove the DRAFT markers that name lane tpsr2 and change
   `\TpsDraftMsg{R2.prompt}`/`{R5.prompt}` and the two
   `\TpsDraftMessageEntry` to `\TpsMsg` / `\TpsMessageEntry` (the text then
   comes from the new spec). If tpsr2 gives the test line its own spec key,
   test-pubs-tps goes RED SHIPPED-NOT-DOCUMENTED for it: add its entry.
2. Turn r2align / r5cover / r5refuse into SCREENS rows (their `.keys` are
   written) and `\DraftShot` into `\OpsShot`; delete the `.draft` files.
3. `make pubs-screens` and review the diff: every screen that passes R2
   (tpsstart, tpsritual, tpsdone, tpsbak) changes in its unprinted part.
4. Re-run test-pubs-tps, test-pubs-screens, test-pubs-mutant.

## Pointers

`pubs/manuals/tps-report-generator/`, `pubs/class/initech-ops.{sty,lua}`,
`harness/pubs/tps_inventory.c`, `harness/pubs/tps_mutants.sh`,
`pubs/screens/capture.sh` (flair route), `pubs/pubs.mk` (test-pubs-tps),
`docs/research/TPS-REPORT-GENERATOR-PERIOD-GROUNDING.md`,
`spec/tps_ritual.json`, `os/milton/tpsrep_program.asm`.
