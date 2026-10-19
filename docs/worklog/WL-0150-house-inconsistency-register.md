# WL-0150 -- The house register of tolerated inconsistencies (VO-30)

Lane `pubsslips` (branch `lane/pubsslips`), bead `initech-ipu5.16`, epic
`initech-ipu5`. Operator directive 2026-10-17, verbatim: "the apostrophe in
'User's manual' made my day, that is solid gold" and "i want more inconsistent
grammar and capitalisation. not too much, like just enough to pass qa".
Started by a Fable lane, taken over by an Opus lane (PM, 13:10); the earlier
agent left one commit (90dcdecc, the Reference's Appendix A for the merged
catalogue rows), kept as it was.

## Context

PUB-REF-0001 ruled names and capitalisation "exactly" (VO-10, VO-14). Real
1985-1992 corporate manuals did not manage that, and the operator wants the
Initech books to carry the same kind of slips -- a few, deliberate, and never
in anything a gate proves (commands, syntax, messages, replayed examples,
screens). The danger is mush: slips nobody can count, slips in the wrong
place, slips that hide a real error. So the slips are LOCKED data with a
budget, a place contract and period evidence, and a gate counts them.

## What changed

| Piece | Change |
|---|---|
| `spec/pubs_inconsistencies.json` (new, LOCKED) | PUB-INC-0001 v1: 11 entries (canonical form, tolerated variants, ERE match pattern, optional scan pattern and check, allowed places, min/max per book, period evidence with file, PDF page, printed page and quote), the place vocabulary, head case per family, the budget, the slogan |
| `pubs/class/initech-syntax.sty` | `\Slip{ID}{text}` prints text and writes `ID|PLACE|PAGE|text` to `<job>.slips` at shipout; `\doctitle` is a cross-reference place |
| `pubs/class/initech-manual.cls`, `initech-forms.sty` | the places, set by the class: running heads/feet (`\fancyhfinit`) and the F-IBM continuation title, `\chapter`/`\section`/`\subsection` heads, the contents and the F-IBM mini-contents, the index, the title page, cover and spine jobs; `\RunningTitle{}` (a running foot title of its own) and `\ContinuationTitle{}{}` (a continuation title of its own) -- period paste-up set them apart from the openers, which is how such slips happened |
| `harness/pubs/pubs_gate.c` | `inconsistency` leg (a small JSON reader + the checks below) |
| `harness/pubs/slip_places.sh` (new) | typesets a proof book with one slip per place and compares the `.slips` with a hand-written list (the class contract, never read from the class) |
| `harness/pubs/slip_mutants.sh` (new) | 13 mutants on copies of the books and the class |
| `pubs/pubs.mk` | `test-pubs-inconsistency` (in `test-pubs`), `PUBS_SLIP_BOOKS`, the mutants in `test-pubs-mutant`; the duplicated `test-pubs-mutant`/`test-pubs-examples` recipes merged into one each (make ran only the last: `tps_mutants.sh` had silently stopped running) |
| `docs/pubs/DESIGN-REFERENCE.md` 1.2 | Sec 9.3 **VO-30** with the register table; VO-10/VO-14 exceptions; Sec 1.3 directive row; **GC-13**; C-21 |
| `docs/pubs/originals/{ibm-dos,ashton-tate-dbase3plus,borland-turbo-pascal,apple-macintosh,wordperfect-51}.md` | a "Slips the books carried" section each: the page-level evidence, read on the page images |
| the three books | the slips below |

Merge repairs on the way (main was RED on `make pubs`): one house
`\MakeBackCover` (initech-xbase dropped its duplicate, initech-borland
`\renewcommand` for the booklet geometry); `\Response` of initech-ops replaces
initech-xbase's; the Reference explains MSG-DOS-0062 (CP-06).

## The gate (`make test-pubs-inconsistency`, GC-13)

For every book in `PUBS_SLIP_BOOKS`: the `\Slip`s of its sources number
between ceil(4 x pages / 100) (at least 3) and floor(10 x pages / 100) of the
printed pages of `main.pdf`; each entry within its min/max per book and its
families; text matching its pattern for the book's head case; every printing
in an allowed place; every source slip printed and nothing printed without its
source; the index slip differs only in case from a head of the book (read from
`main.toc`); the stale cross-reference names no head of any book in the run; no
scanned variant (`Users Guide`, `Initech OS`/`Initech DOS`) outside a `\Slip`;
the slogan "Is This Good for the COMPANY?" exact in the books and the class
and in no slip; every entry used by some book. Command registration is by
NAME|KIND in `.cmds` and no slip touches a command, so the command, mention and
lint legs treat the variants as nothing new (the lint lowercases anyway).

Budget, decided: 4..10 per 100 pages, the house placing 6..8 per 100. Cited:
the models ran far above the ceiling (ZR23 p. 1-4 carries four register
entries on one page), so the ceiling is the QA bar the operator asked for, and
the floor keeps the register in every book as books grow.

## The slips placed

| Book (pages; floor..ceiling) | Entry | Where (source) | Printed | Text |
|---|---|---|---|---|
| InitechDOS Reference (112; 5..11) | VO30-01 | frontmatter.tex, Read This First | cross-reference p. iii | *InitechDOS Users Guide* |
| | VO30-04 | ch4-batch.tex, chapter title | heading p. 4-1, contents p. ix | Chapter 4. Batch files |
| | VO30-04 | ch2-entries-a.tex, COPY | running head (continuation title) p. 2-14 | COPY (Copy File) / command |
| | VO30-05 | ch3-config.tex, Creating CONFIG.SYS | body p. 3-3 | The system is then restarted to use the new file. |
| | VO30-06 | ch2-entries-b.tex, REN | body p. 2-34 | The file stays in it's directory. |
| | VO30-10 | spine.tex | spine | version 3.30 (cover: Version 3.30) |
| | VO30-11 | ch1-how-to-use.tex, Drives | body p. 1-5 | ... whether it is a diskette or a hard disk |
| InitechDOS User's Guide (92; 4..9) | VO30-01 | spine.tex | spine | InitechDOS Users Guide (cover: User's Guide) |
| | VO30-04 | ch5-batch.tex, section head | heading p. 5-3, contents p. ix, mini-contents p. 5-1 | Creating a Batch file |
| | VO30-06 | ch3-files.tex, Renaming a File | body p. 3-3 | The file keeps it's size, date and directory |
| | VO30-02 | ch3-files.tex, Organizing Your Files | body p. 3-7 | Make copies of important files on another Diskette |
| | VO30-07 | ch4-disks.tex, Kinds of Disk | body p. 4-3 | A fixed disk is built into the computer, it holds far more ... |
| | VO30-08 | ch5-batch.tex, Batch Subcommands | body p. 5-7 | *InitechDOS Reference*, Chapter 4, "Batch Commands," (the chapter is Batch Files) |
| | VO30-09 | ch5-batch.tex, index | index p. Index-2 | Replaceable parameters (head: Replaceable Parameters) |
| TPS Report Generator Operating Procedures (80; 4..8) | VO30-08 | s1-reference.tex | body p. 1-1 | (Session 2, "Leaving the Desktop") -- the session is Leaving FLAIR |
| | VO30-03 | s1-reference.tex, Files Used | body p. 1-2 | the shipped Initech OS data volume |
| | VO30-05 | s2-leaving-flair.tex, Procedure TPS-P-01 step 6 | body p. 2-3 | Type CD APPS.  Enter is then pressed. |
| | VO30-06 | s2-leaving-flair.tex, The Current Directory | body p. 2-4 | The program reads it's parameter record and its table |
| | VO30-04 | s3-parameter.tex, running foot | running head pp. 3-3, 3-5 | Session 3. Preparing the run (opener: Preparing the Run) |

19 slips; all 11 entries used. The slogan is not a slip and is untouched.

## Frictions

- Main was RED on `make pubs` twice over from parallel merges (two `\newcommand` clashes,
  and a catalogue row without its Appendix A entry); repaired here, minimal.
- `pubs.mk` carried two recipes for `test-pubs-mutant` and `test-pubs-examples`; make kept
  the last and dropped the TPS manual's mutants without a word. Merged.
- The Borland Library Reference text layer reads "The Users Guide"; the image prints
  "User's Guide". OCR is not evidence: every register item was read on the page image.
- No Lotus slip was found in the time spent (the 2.2 scans have no text layer; the R3
  Reference's head/body capitalisation differences are style, not slips).

## Acceptance

See the bead close note for the gate tails: `make pubs`, `make test-pubs` (every leg),
`make test-pubs-mutant`, `make test-pubs-repro`, `make test-pubs-families`.

## Pointers

`spec/pubs_inconsistencies.json`; `docs/pubs/DESIGN-REFERENCE.md` Sec 9.3 (VO-30), GC-13;
`harness/pubs/pubs_gate.c` (`inconsistency`); `harness/pubs/slip_places.sh`,
`harness/pubs/slip_mutants.sh`; `pubs/class/initech-syntax.sty` (`\Slip`). Other books join
the register by placing their slips and adding themselves to `PUBS_SLIP_BOOKS` (InitechBase
and Turbo Initech books are on main and not yet in it: a follow-up for their lanes).
