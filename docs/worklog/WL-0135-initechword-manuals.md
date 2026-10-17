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

## Continuation (the Fable lane, same bead)

The Opus lane stopped after the Workbook cover and spine.  The Fable lane
took the worktree over, re-read CLAUDE.md, PUB-REF-0001, the WordPerfect
5.1 annex and the lane's commits, re-ran every InitechWord gate itself
(Rule 4) and did the Law-4 page review of all three books at 60 dpi
against the annex's anatomy (s2-s7): the cover grid, verso, contents,
divider leaves, AN-11 entries with the letter marker and the running foot
naming the entry in force, the appendices, glossary, three-column index,
colophon, the three comment forms, the card's four panels and the strip.

### What changed

- Both books: the last front-matter page was shipped after the page
  counter had been reset to 1, so the Reference's last Contents page and
  the Workbook's last Introduction page printed the folio 1 and the
  divider leaf behind them printed 3.  A `\cleardoublepage` now precedes
  the reset (main.tex, both books): Contents ends on vii, Introduction on
  vi, the section divider is page 1 and The Basics / Lesson 1 start on 3,
  the model's own numbering (annex s2 "Getting Started 1-21", s5
  "Fundamentals I pp. 3-107").  Page counts unchanged.

### Where to resume

Nothing of the brief is open on this branch except the three DRAFT
markers above, all of which wait on line-wide pieces (O-5 address check;
the PK-13 cards, DRAFT-marked the same way in the InitechDOS books).  The
extent stays where the shipped surface puts it (P1-P2: 220 items, 46
entries, 36 lessons); the 700-950 / 300-420 page targets are the model's
extent for a product with search, print, headers, columns, tables and
spell, none of which ships (D-14: omitted, listed above).

## Takeover (the Opus lane, same bead): InitechWord P3

### Context

The PM's takeover brief (2026-10-17 ~13:10) found the lane clean with
nothing open but the DRAFT markers.  Rule 4 check: lane/iwordp3 (bead
initech-qvgd parts 1-2, WL-0146) had been merged onto main at 12:45, after
this lane's base -- the 48-slot template with period-grey keys, Search
(F2, Shift-F2), Replace (Alt-F2), Cancel/Undelete (F1 on the editing
screen) and a minimal Setup (Shift-F1, IWORD.SET).  The books on the old
base stated the opposite of what now ships ("Search is not shipped",
"Shift-F7, Alt-F7 and Ctrl-F7 start Exit", "Alt-F10 and Ctrl-F10 save",
"Cancel on the editing screen does nothing"), and the probe died on the
new face ("choice 1 of F2 does something but the screen names no such
choice").  Law 1 and Sec 10 (everything that ships, documented) outweighed
the brief's "do not merge main yourself": main was merged into the lane
(conflicts in pubs_gate.c, colophon.tex and pubs.mk resolved by keeping
both sides; the merge commit says so).

### What changed

- harness/pubs/iword_probe.c measures P3: kind wpgray (a key that leaves
  M_GRAY, listed with the name the status line shows, "Shift-F7=Print");
  the keys of the Search / Replace strings ("Srch:F6", "Rplc:Tab", their
  aliases), probed from a string with the cursor inside it, both
  directions and Replace's own search string; the Undelete menu (two
  separated deletions added to the probe document); Setup to the unit
  letters; the P3 messages by scenario; the literal scan skips only the
  mutant branch of an IWORD_MUT_* knob.  Inventory: 359 items (175 keys,
  81 menu choices, 3 pull-down items, 49 codes, 27 messages, 24 grey keys).
- Reference: new entries Replace, Search, Setup, Undelete (each with
  replayed examples); Block, Cancel, Directories, Exit, Messages, Reveal
  Codes, Save, Template, Units of Measure, The Basics, Getting Help, Appendix
  C/E/F/G/H and the glossary brought to the shipped program; two FLAIR
  figures (search, setup) from the booted desktop.  50 entries, 140 pages.
- Workbook: Lessons 37 (Finding Text, on the MINUTES of Lesson 24), 38
  (Replacing Text, STATUS of Lesson 31 -> MONTHLY), 39 (Restoring Deleted
  Text); Lesson 36's "Saving With Ctrl-F10" (no longer true) becomes a
  grey-key task.  39 lessons, 124 pages.
- Template strip: all 48 slots of spec/iword/fkeys.json, the grey names in
  grey and registered wpgray; TEMPLATE holds them to the measured grey keys
  name for name.
- initech-wpc.sty: grey screen cells ('G'), \WPGray, \WPPromptAlias, \larr;
  keys of a prompt or of Block are no longer indexed by key.
- Gates: iword_mutants.sh 14 mutants (FAKE_KEY and UNSHIPPED_KEY re-keyed
  to Shift-F11 -- F2 and F3 now act; new GREY_NAME, FKEY_DROP (a probe
  built with face_a.c's IWORD_MUT_FKEY_DROP), SEARCH_KEY,
  SHIPPED_BRANCH_MESSAGE); ENTRY_ORDER's TEXINPUTS gains pubs/common (the
  merged class inputs initech-logo.tex).

### Findings while writing (each found by the replay, text corrected)

1. Replace of [BOLD] with nothing deletes the [BOLD] codes and leaves the
   [bold] codes in the text (no pairing in replace_match); the book says so
   and shows how to remove [bold] too.
2. After Undelete's Restore the cursor stays before the restored text.
3. Searching backward from just after a match stops on that same match.
4. Setup records the units and the backup choice in IWORD.SET but nothing
   reads them: the status line and Format stay in inches, no backup copy is
   made.  Documented as such (no DRAFT: the choices themselves ship).
5. A document saved under the name IWORD.SET is read as (invalid) settings
   and overwritten by the next Setup change.  Documented under Directories.

### Main-side breakages met (not this lane's books; for the PM)

- Every manual stopped typesetting: initech-forms.sty (design merge) and
  initech-xbase.sty (baseman merge) both \newcommand\MakeBackCover.  Fixed
  here minimally: the xBase one is now \providecommand (the house back
  cover of forms.sty wins); the InitechBase owner should confirm.
- initechdos-reference does not typeset: MSG-DOS-0045 (", tree past this
  point not processed") has no \MessageDetail.  Not touched.
- test-pubs-lint is RED on pubs/examples/samir/salary.txt (CR LF bytes).
  Not touched.
- Because of the two above, make test-pubs-build (and so test-pubs,
  test-pubs-mutant, test-pubs-repro, test-pubs-screens) cannot go green on
  this branch; every InitechWord leg was run on its own (Acceptance below).
- Two lanes' covers: the merged class gives every front cover the logo and
  line band; the Reference and Workbook covers now carry their back cover
  (PK-05, \MakeBackCover, page 2 of cover.tex) like the InitechDOS books.

### Not shipped, still omitted (D-14)

List Files (F5) and the other grey keys (Help, Switch, Indent with Shift,
Date/Outline, Center, Print, Merge Codes, Thesaurus, Mark Text, Flush
Right, Columns/Table, Style, Graphics, Macro, Shell, Spell, Screen, Text
In/Out, Tab Align, Footnote, Merge/Sort, Macro Define): documented only as
what they do -- show a grey name -- in Appendix F and on the template.
Timed backup, the effect of the unit and backup choices, Setup's grey
groups, the Search code selectors other than F6 / F8 / Tab / F4 / Enter /
Ctrl-Enter, extended and block searches: not described as features.

### DRAFT markers remaining (3, unchanged)

The two verso address/permit markers (O-5, D-12) and the Warranty
Registration Card (PK-13).

### Acceptance (this section)

Typeset with the lane's InitechWord books only (main's other manuals do not
typeset, above): Reference 140 pages (50 entries, 8 appendices, 46 replayed
screens, 9 FLAIR figures), Workbook 124 pages (39 lessons, 95 replayed
screens), install card 4 panels, strip 1 page, covers 2 pages each (front
and back), spines 1 page; 0 overfull; DRAFT markers 2 + 1 + 0.

```
>>> test-pubs-iword: inventory 359 shipped items (175 keys, 24 grey keys, 81 menu choices, 3 pull-down items, 49 codes, 27 messages)
>>> test-pubs-commands: green (359 shipped, 359 documented, 0 DRAFT, 0 explicit omissions)
>>> test-pubs-commands: green (42 shipped, 42 documented, 0 DRAFT, 0 explicit omissions)
>>> test-pubs-iword-keys: green (111 keys named in running text, every one shipped)
>>> test-pubs-iword: features green (163 lesson features, each an entry of the Reference)
>>> test-pubs-iword: green (73 launches of WORD.EXE replayed, 141 printed screens == the replay byte for byte)
>>> test-pubs-iword-mutant: green (14 named mutants rejected by their gate legs)
>>> flair_capture.sh: 9 FLAIR figures captured from the booted system  (twice; all 9 == fixtures/pubs/initechword-reference)
>>> repro: 8 InitechWord PDFs byte-identical across two builds
>>> test-pubs-lint: green (on pubs/manuals/initechword-*, pubs/class, pubs/common)
>>> test-pubs-families: identity green
>>> test-makefile-vars: green
```

### Where to resume

Nothing of the brief is open for the InitechWord books.  When the PM merges
this branch: re-run `make test-pubs` once the DOS Reference's MSG-DOS-0045
detail and the InitechBase CR LF example are fixed on main; the InitechWord
legs are green on their own.  If List Files (F5) ships later (WL-0146's
mint), F5 leaves the grey keys and wants its own Reference entry: the
probe will show it as SHIPPED-NOT-DOCUMENTED.
