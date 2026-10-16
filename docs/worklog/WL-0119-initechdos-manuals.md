# WL-0119 -- InitechDOS 3.30 manuals: the complete User's Guide and the Reference

## Context

Bead initech-ipu5.7 (writing lane, branch lane/dosman, base 6f27de1). The
InitechDOS manuals pilot: grow the 32-page User's Guide specimen of
initech-ipu5.5 into the complete book, and write the InitechDOS Reference
binder with an entry for every shipped command, both written against the
shipped build so the correctness gates can replay them. Binding design
reference: docs/pubs/DESIGN-REFERENCE.md (PUB-REF-0001): family F-IBM for
both books (Sec 2), AN-01 command entries, AN-03 message entries, AN-20
verso, AN-21 About This Book, AN-22 mini-contents, AN-26 colophon, PG-10 /
PG-20 / PG-28 / PG-30 furniture, the DS part numbers (Reference
1024-3300-10, User's Guide 1024-3300-11; the specimen used -12, which the DS
table gives to the TPS manual). Model: docs/pubs/originals/ibm-dos.md (IBM
DOS 3.10 Reference anatomy, page-cited); the DOS 3.3 PDFs in
initech-manuals-ref/dos33/ for register only.

## What changed

Books (pubs/manuals/):

- initechdos-reference/ (new): title, verso (AN-20 order), About This Book
  (AN-21 sub-heads), Contents; Ch 1 How to Use This Book; Ch 2 InitechDOS
  Commands (notation, Information Common to All Commands, 24 entries);
  Ch 3 Configuring Your System (CONFIG.SYS, the standard settings, 7
  "Configuration Command" entries); Ch 4 Batch Files (8 "Subcommand"
  entries); App A Messages (Responses; Device Error Messages and Other
  Messages generated from spec/dos_messages.json with authored Explanation
  and Action per id; Other Messages Displayed by Commands quoted from
  command.c, dosutil.c, chkdsk.c, xcopy_tree.c); Index; colophon; three
  Reader's Comment Forms. 100 pages. 35 example transcripts.
- initechdos-users-guide/ (rewritten and grown): About This Book; Ch 1
  Introduction; Ch 2 Getting Started; Ch 3 Working with Files and
  Directories; Ch 4 Diskettes and the Fixed Disk; Ch 5 Batch Files; Ch 6
  Configuring Your System (CONFIG.SYS and the environment); Ch 7 Redirecting
  Input and Output (filters); Ch 8 The InitechDOS Editing Keys; Ch 9 The
  Utility Programs; App A Messages (generated list); App B Glossary; Index;
  colophon; RCF. 88 pages, 25 screen figures. The specimen's summary
  CommandEntry boxes moved to the Reference as full entries.

Toolchain (pubs/, small, each documented in its file header):

- initech-manual.cls: option `ibm` (F-IBM furniture: Termes body 10.5 pt;
  "Chapter N. Title" openers with the AN-22 mini-contents and a "Notes:"
  verso; folio-only foot; stepped black thumb tabs from \ChapterTab; index
  folios Index-n; free ragged margin); command-entry continuation titles
  (PG-10) drawn at shipout on every page an entry runs over.
- initech-syntax.sty: the documented-command list. Every entry writes
  NAME|KIND|STATE|ENTRY lines to build/pubs/<manual>/main.cmds
  (\RegisterCommand; \AlsoNamed for DEL/ERASE etc.; DRAFT entries say so)
  for the parallel GC-01 gate. DosEntry, the AN-01 IBM entry (two-line
  title + 1 pt rule, hanging Purpose/Format/Type/Remarks/Example in fixed
  order, Type thinned only for batch/config, out-of-order or missing labels
  are errors). \MessageDetailsOn / \MessageDetail / \CheckMessageDetails
  (CP-06: a catalogue id with no detail, or a detail with no id, fails the
  build; mutation-proven by renaming one id). \OtherMessage, \Param,
  IBMNote/Important, \see. \SyntaxLine now breaks at its spaces.
- initech-screen.sty: \Transcript / \Shot -- an F-IBM example set from a
  committed capture (same 25x80 ASCII checks as \Screen), typed text bold
  from the screen's SCREENS keystrokes.
- initech-forms.sty: verso in AN-20 order (\Trademarks, \Revision,
  \PrintCode; "Printed in" only when set -- D-1 is open, so it no longer
  prints "Germany"); \FinishBook pads with Notes: pages so the book ends on
  a multiple of four after the colophon and the RCFs.
- pubs/common/colophon.tex (AN-26, the GC-06-exempt page; the verso no
  longer names QEMU/TeX).
- tools/pubs_msgtable.c: optional selector all|device|other (AN-03 split).
- pubs/screens: the practice diskette is now pubs/screens/disk/MANIFEST:
  AUTOEXEC.BAT (@ECHO OFF / SET PATH=A:\APPS), the three text files, APPS
  holding the five shipped utilities (the same binaries flair-data-rules
  plants in APPS; pubs.mk stages them), SAMPLES holding seven batch files.
  Label date pinned (CHKDSK prints it; mformat used the host clock).
  keys.sh types the full DOS syntax (= + , < | $ % @ ( ) " and Ctrl-Z/C);
  capture.sh takes `marker#N` (the N-th occurrence).
- harness/emu: qcode tokens equal/comma/apostrophe; --keys and --mouse caps
  4096 bytes; --screendump-after-count N.
- pubs/build.sh: pubs/common on TEXINPUTS; -interaction=nonstopmode
  -halt-on-error passed to latexmk too (the -usepretex mutant build used to
  hang on a LaTeX error waiting for the terminal).

## Why

Law 1/2: every behavioural sentence was checked against the source and
then against a capture of the running system. Where the build lacks DOS 3.3
behaviour the book states the DOS 3.3 behaviour inside a DRAFT marker that
names the bead (D-14), and shipped deviations that are not defects are
documented as built (DATE/TIME without prompts, SHIFT keeping %0, nine
parameters at most, FOR without wildcard expansion -- the latter flagged).

## Frictions and defects found (no bead filed by this lane -- beads are out
of scope; each wants one)

1. CD \ is a no-op from a subdirectory (int21.c do_chdir treats "\" as
   "stay"); DOS 3.3 goes to the root. The specimen told readers to use it.
2. IF and FOR hand their command to the command dispatcher, so IF ... GOTO,
   SHIFT, CALL, PAUSE are "Bad command or file name"; IF "%1"=="" GOTO X
   loops forever (seen in a probe; 1684 lines of the message).
3. PROMPT $D shows 00-00-0000 (the REPL passes year/month/day 0) and $$
   cuts the prompt (it is printed with AH=09h, '$'-terminated).
4. XCOPY run from a subdirectory resolves a relative source against the
   root (XCOPY *.BAT in A:\SAMPLES copied A:\AUTOEXEC.BAT).
5. COPY CON echoes Ctrl-Z as a raw glyph, which harness/console_text cannot
   decode (so no COPY CON screen); DOS echoes ^Z.
6. Ctrl-C at the prompt displays the debug text INT23-BREAK on the console.
7. MORE, FIND, SORT are built and gated but not on the shipped disk.
8. README.md's requirements (80386, 2 MB) disagree with spec/hardware.json
   and FLAIR_HEAP_MIN (the boot panics below 4 MiB of extended memory); the
   User's Guide follows the spec.
9. CP-07 orders the Reference Introduction, Files, Fixed Disk, Configuring,
   Directories, Standard I/O, Batch, Commands; this lane followed the
   brief's order (How to Use, Commands, Configuring, Batch, Messages) and
   put Files/Directories/Standard I/O in the User's Guide. No Keyboard
   appendix or Glossary in the Reference yet.
10. QMP socket paths: the harness fails silently (exit 1, empty serial) when
   --out is a long absolute path (sun_path 108 bytes) -- capture from a
   relative build/ path.
11. Screen markers that already occurred earlier in a session (banner text
   "Version 3.30", typed echo) trigger the screendump early; hence #N.
12. Class conformance (PUB-REF Sec 11.5: 12 pt grid, 142 mm block, small
   caps keys, EB Garamond covers) is still the toolchain follow-up; the ibm
   option changes the body face only.

## Remaining DRAFT markers (35: Reference 19, User's Guide 16)

Both books:
- Verso address / permit number: PUB-REF O-5, D-12 (publications check).
- Quick Reference not yet written: DS row DOS-QR.
- Package pieces other than the books: PK-12/PK-13 plan, not produced.
- Start-up date/time prompts: initech-qskz (Ref Ch 1; UG Ch 2).
- Date stamps of new files/directories (0-00-80 12:00a): initech-dwbl
  (Ref COPY, MD; UG Ch 2 x2, Ch 4).
- CD \ to the root: bead needed (Ref CD; UG Ch 3).
- IF ... GOTO: bead needed (Ref IF; UG Ch 5).
- PATH command: initech-sy5f (Ref PATH entry is DRAFT; UG Ch 6).
- XCOPY relative source in a subdirectory: bead needed (Ref XCOPY; UG Ch 9).
Reference only:
- BREAK ON checking beyond keyboard input: DEC-16 C-6 scope.
- DIR parameters (path, name, /P, /W): initech-e47v.
- PROMPT $D and $$: bead needed.
- FOR wildcards and FOR at the prompt: bead needed.
- MSG-DOS-0006, 0008, 0013, 0014, 0015: catalogue entries no code path of
  this version displays (5 markers).
User's Guide only:
- Practice-diskette copying (DISKCOPY) and the package: bead needed.
- FORMAT, DISKCOPY, FDISK not shipped: bead needed (Ch 4).
- Filters MORE/FIND/SORT not on the shipped disk: bead needed (Ch 7).
- Ctrl-C shows INT23-BREAK instead of ^C: bead needed (Ch 8).
- Template editing keys, Ctrl-S, Ctrl-P not built: bead needed (Ch 8).

Cleared from the specimen's 11: the single-drive note, the system-file
names note, the requirements note (now from spec/hardware.json), the DIR
"for the Reference" note (now in the Reference), the message-appendix stub
(the Reference has every explanation), the Related Publications note for
the Reference (it exists).

## Acceptance

- make test-pubs-build: green -- reference/main.pdf 100 pages, 0 overfull,
  19 DRAFT; users-guide/main.pdf 88 pages, 0 overfull, 16 DRAFT; covers and
  spines 1 page each.
- make test-pubs-repro: green (6 PDFs byte-identical); test-pubs-repro-
  mutant: green; test-pubs-honesty: green.
- make test-pubs-screens: green (60 screens == live OS output), run twice.
- make test-shell (harness regression after the qemu.c changes): PASS.
- Command coverage: build/pubs/initechdos-reference/main.cmds lists 44
  names in 39 entries: all 24 internal names of command.c cmd_classify (18
  entries), the 5 externals on the shipped disk, the 7 CONFIG.SYS
  directives of config_sys.c, the 8 batch subcommands of batch.c, plus
  PATH as a DRAFT entry.
- Viewed: every chapter opener, a spread per chapter and both indexes of
  both books (pdftoppm); fixed the long-syntax overfulls, the empty-macro
  \ifx tests (tab on the title page, "Printed in ."), table widths.

## Pointers

- pubs/manuals/initechdos-reference/, pubs/manuals/initechdos-users-guide/
- pubs/class/initech-{manual.cls,syntax.sty,screen.sty,forms.sty}
- pubs/screens/{capture.sh,keys.sh,disk/MANIFEST}, pubs/common/colophon.tex
- tools/pubs_msgtable.c, harness/emu/qemu.c, harness/emu/qemu_main.c
