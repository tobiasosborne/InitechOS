# WL-0135 -- InitechWord manuals: Reference, Workbook, installation card, template

## Context

Bead initech-ipu5.10 (writing + gating lane, branch lane/wordman, base
4832d01). The InitechWord publications of PUB-REF-0001 Sec 12: WRD-REF
(InitechWord Reference, 1062-0000-10, F-WPC, alphabetical feature entries
AN-11, PK-07 divider leaves), WRD-WKB (InitechWord Workbook, 1062-0000-11,
AN-18 lessons in three graded sections), WRD-INS (installation card
1062-0000-50, PK-13, and the function-key template strip 1062-0000-20,
PK-14). Model: docs/pubs/originals/wordperfect-51.md and the scans of the
WordPerfect 5.1 Reference, Workbook, template strip and install cards
(structure, furniture and register modelled; no prose, table or artwork
copied). Product: WORD.EXE, os/iword (P1 editor, P2 slices 1-3: codes,
Reveal Codes, block ops, tabs/indents/hard pages, margins, justification,
line spacing, the Home prefix keys, the Format screens), spec/iword (LOCKED
codes and file-format data), the host face oracle test_iword_face.c and the
locked emulator traces spec/flair_iword_traces.mk. InitechWord displays no
version, so no book prints one and every part number carries 0000 (D-3).

## What changed

Books (pubs/manuals/):

- initechword-reference/ -- binder: title page, verso (AN-20, with the
  operator's slogan under the edition notice), Initech Software page,
  contents (PG-35 F-WPC: section names in the side column), four sections
  behind Office Systems divider leaves with the triangle field (PK-07):
  Getting Started (The Basics: Registration, Installation, The Template,
  Starting InitechWord, Keys to Know, Function Keys, Pull-Down Menus, Menu
  Options, Codes, Exit; Getting Help; A Brief Lesson), Reference (46 AN-11
  entries, inverted names, letter-by-letter order, the first entry of each
  letter with its letter, numbered procedures with the pull-down
  alternative, alphabetised Notes, See Also), Appendix (A Characters, B
  Codes, C Messages and Prompts, D Document File Format, E Initial Settings,
  F Keyboard, G Menus, H Limits), Glossary/Index (glossary AN-23, 3-column
  index), colophon, three Reader's Comment Forms; cover and spine.
- initechword-workbook/ -- perfect-bound: verso, contents, Introduction,
  Fundamentals I (lessons 1-8), Fundamentals II (9-16), Special Features
  (17-36), black divider leaves, Appendix A Feature List (Alphabetical) and
  B (By Lesson) built from the lessons, 2-column index, colophon; cover and
  spine.
- initechword-install/ -- main.tex the installation card (5.0 x 4.25 in,
  four panels, black band, Heros Cn Bold numerals, "(See other side)",
  code bottom right); strip.tex the 17.5 x 2.25 in function-key template
  (five-line cells: Ctrl red, Alt blue, Shift green, plain black, key name
  italic; groups of four with legend cells; quick-list panel).

Toolchain (each documented in its file header):

- pubs/class/initech-wpc.sty -- the F-WPC family: running foot at the outer
  corner naming the entry in force (PG-15), contents, divider triangle
  field, Feature / Proc / WPSteps / PullDown / Notes / Topic / SeeAlso,
  Lesson / Task / LessonFeatures / Aside, the glossary, lettered
  appendices, 3- or 2-column index; order checks (entries letter by
  letter, Notes topics, See Also lists; every See Also and \InRef target an
  entry of the book); the registration (\Documents -> \RegisterCommand),
  key mentions (\K -> .wpkeys), the replay record (\BeginReplay, \Type,
  \Press, \Choose, \PressKC, \TypeSpaces, \CapsLock, \CloseBox,
  \PullChoose -> .wpreplay), \WordScreen / \CompareScreen (the replayed
  76 x 24 screen with faces, reverse cells and the block band, in a 10%
  panel; cropping can never hide a row that holds something), \FlairShot
  (an emulator figure at 3 x 3 on a 600 dpi grid, interpolation off),
  \WPMessage / \WPCode / \WPAlias for the appendices.
- pubs/class/initech-manual.cls -- \DividerColor (PK-07: the Office Systems
  colour in the Reference, black in the Workbook).
- pubs/build.sh builds strip.tex too; pubs/common/colophon.tex takes a
  book's \ColophonScreens paragraph (the InitechWord books say truthfully
  that their panels are host replays of WORD.EXE and their figures emulator
  captures).
- harness/pubs/iword_probe.c -- the shipped surface MEASURED from WORD.EXE
  (face_a.c over the real core with a recording Toolbox mock) and the replay
  engine; harness/pubs/iword_check.sh (test-pubs-iword), iword_features.sh,
  iword_mutants.sh (test-pubs-iword-mutant), iword.allow (empty);
  harness/pubs/pubs_gate.c accepts the five InitechWord kinds and gains
  `keys`.
- harness/pubs/ppm_grey_png.c + pubs/screens/flair_capture.sh -- the pixel
  route (NT-21 b, NT-22) for FLAIR figures; the PNGs (binary, so outside
  pubs/) in fixtures/pubs/initechword-reference/; test-pubs-flair-screens
  re-captures them in test-pubs-screens.
- pubs/pubs.mk -- test-pubs-iword in test-pubs, test-pubs-iword-mutant in
  test-pubs-mutant, test-pubs-flair-screens in test-pubs-screens,
  pubs-iword-screens and pubs-flair-screens to regenerate; the DOS
  COMMAND-SURFACE leg skips the InitechWord .cmds (graded by their own gate).

## Why

Law 1 / Law 2 applied to a word processor whose key map is code, not a
table: the inventory had to be measured, not scraped. The probe asks the
shipped program what each key, menu digit, pull-down item and scenario
does, so GC-01 is both directions against behaviour, not against text. The
Workbook's lessons ARE their replay scripts: the keystrokes are recorded by
the same macros that print them, so a printed step and the replayed step
cannot drift; a wrong count in a lesson showed up at once as a wrong screen
(several were caught and fixed this way while writing).

## Frictions and findings (for the PM; no beads filed by this lane)

1. WORD.EXE ignores Shift/Alt/Ctrl on many keys: Shift-, Alt- and Ctrl-F7
   all start Exit, Alt- and Ctrl-F10 start Save, and every modifier is
   ignored on Backspace, Delete, Enter (but Ctrl), Tab, the cursor keys and
   Home. WordPerfect 5.1 gives Shift-F7, Ctrl-F7, Alt-F7, Ctrl-F10, Alt-F10
   other features. Documented as shipped (Appendix F, "Keys That Act as
   Other Keys"); a bead may want face_a.c to refuse them instead.
2. With a left margin under one inch the start of each line is off the left
   edge of the window (screen column 0 is fixed at 1"). Documented (Screen).
3. [Center] and [Flsh Rgt] codes of retrieved WordPerfect documents are kept
   but the text is drawn at the left margin. Documented (Codes).
4. The FLAIR desktop's key path dropped presses from a fast run of the same
   key (a Block trace with Right x 11 lost three, once in four runs). The
   figures now use single keys; the emulator gates with long same-key runs
   may be flaky for the same reason.
5. `--rtc-deterministic` ends the FLAIR pump life before an injected trace
   is read (the PIT is tied to retired instructions); the FLAIR capture does
   not pin the clock and crops the menu-bar clock out instead.
6. NT-22 names spec/assets/clut.json for the grey table, but the shipped
   desktop paints the locked Win-3.1 accents of color_canon.json (#000080,
   #C0C0C0, the Platinum greys), none of which is in the CLUT: the table is
   the union of both locked files (house ruling in ppm_grey_png.c). The
   design reference wants that sentence amended.
7. A four-digit telephone extension is a "year after 1993" to the period
   lint: the phone list uses three-digit extensions.
8. \FinishBook[1] left the Workbook at 81 pages (not a multiple of 4); the
   Workbook is perfect-bound and AN-25 gives comment forms to binders, so it
   carries none (\FinishBook). The odd-count padding path wants a look.
9. The class's RCF is drawn by \FinishBook, so the slogan at the foot of each
   form waits for the design lane's \IsThisGoodForTheCompany class change;
   the books provide the macro (the coordinator's interim definition) and
   call it on the versos and the card. There is no back-cover piece yet.

## Not shipped, omitted (D-14; P3-P5 of the plan)

Search and Replace (F2, Shift-F2, Alt-F2), Cancel/Undelete and undo, List
Files (F5), Print (Shift-F7), Help (F3), Setup (Shift-F1), Switch
(Shift-F3), Center (Shift-F6) and Flush Right (Alt-F6), Footnotes (Ctrl-F7),
Columns and Tables (Alt-F7), Style (Alt-F8), Merge (F9, Shift-F9), Macros
(Alt-F10, Ctrl-F10), Spell (Ctrl-F2) and Thesaurus (Alt-F1), Screen
(Ctrl-F3), Text In/Out (Ctrl-F5), Typeover (Ins), Base Font and Print Color,
Tabular Column, Rectangle and Append, the Page/Document/Other Format groups
and the other Format: Line settings, headers and footers. None is described;
where a shipped screen shows one (the Format screens, the Font and Move
menus) the book documents what the shipped program does with it (nothing,
or a return to the document). The template leaves their cells blank.

## DRAFT markers remaining (3)

- Reference and Workbook versos: the fictional address and permit number
  are to be checked against real businesses before print (PUB-REF O-5, D-12;
  the same marker as the InitechDOS books).
- Reference, Getting Started: the Warranty Registration Card is not yet
  produced (PK-13).

## Acceptance

Books: initechword-reference/main.pdf 120 pages (46 entries, 8 appendices,
34 replayed screens, 7 FLAIR figures, 0 overfull, 2 DRAFT), cover and spine
1 page each; initechword-workbook/main.pdf 112 pages (36 lessons, 82
replayed screens, 0 overfull, 1 DRAFT), cover and spine; initechword-install
main.pdf 4 panels and strip.pdf 1 page (0 DRAFT).

- make pubs: 13 documents built (+ the two InitechWord covers/spines).
- make test-pubs-build, test-pubs-commands, test-pubs-lint,
  test-pubs-examples: green.
- make test-pubs-iword: green -- inventory 220 shipped items (88 keys, 63
  menu choices, 3 pull-down items, 49 codes, 17 messages); 220 shipped, 220
  documented, 0 DRAFT, 0 omissions; template 12 of 12; 95 running-text keys
  shipped; 150 lesson features each a Reference entry; 64 launches of
  WORD.EXE replayed, 116 printed screens == the replay byte for byte.
- make test-pubs-iword-mutant: green, 10 named mutants rejected.
- make test-pubs-flair-screens: green, 7 figures == the booted desktop.
- make test-pubs-repro (13 PDFs byte-identical), test-pubs-repro-mutant,
  test-pubs-families, test-pubs-honesty, test-makefile-vars: green.
- make test-pubs / test-pubs-mutant / test-pubs-screens: RED on the
  InitechDOS Reference's chkdsk.txt (the running CHKDSK now reports 118320
  bytes of memory where the committed capture says 118448) -- an InitechDOS
  book's screen drifting against the merged kernel, not touched by this
  lane (no os/ change on lane/wordman); it wants a `make pubs-screens` on
  the DOS books by their owner. Every InitechWord leg of those aggregates
  was run on its own and is green; harness/pubs/mutants.sh (the DOS
  mutants) run directly: green, 11 named mutants rejected.

## Pointers

- pubs/manuals/initechword-{reference,workbook,install}/
- pubs/class/initech-wpc.sty, harness/pubs/iword_*.{c,sh}, ppm_grey_png.c,
  pubs/screens/flair_capture.sh, fixtures/pubs/initechword-reference/
- harness/pubs/README.md "The InitechWord manuals"
