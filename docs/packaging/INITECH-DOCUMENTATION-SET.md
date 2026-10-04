<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# The Initech Documentation Set -- Proposed Manuals and Package Contents per Product

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (supporting file to `MANUALS-AND-BOXES-RESEARCH.md`)
**Programme:** STAPLER -- Physical Edition (epic `initech-gtge`, task `initech-gtge.1`)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-RES-0001-B |
| Version | 1.0 (proposal for operator decision; nothing here is ratified) |
| Date | 2026-10-04 |
| Parent | `docs/packaging/MANUALS-AND-BOXES-RESEARCH.md` |
| Grounding | `README.md`, `docs/HANDOFF.md` Sec 4-5, `InitechOS-PRD.md`, ADR-0003/0004/0007/0008/0012/0013, `docs/plans/INITECH-123-plan.md`, `docs/plans/INITECH-WORD-plan.md`, `bd show initech-68iw`, `bd show initech-fdxa`, `os/milton/{command,batch,config_sys}.c`, `spec/` |

---

## 0. Ground rules for every manual

1. **Describe the real software.** Each chapter documents behaviour that exists and is gated.
   Proposal: every chapter's source carries a comment naming the gate(s) that prove what it
   says (for example `test-samir-write` for the REPLACE/APPEND chapter). A chapter whose
   feature is not yet built is held back, not written from the plan. This is the repo's Law 2
   applied to prose. The outlines below mark each chapter **[BUILT]**, **[PARTIAL]** or
   **[PLANNED]** against the state in `docs/HANDOFF.md` (2026-10-04).
2. **Model the structure, never copy the prose.** The originals are copyrighted. Chapter order,
   document roles, page furniture and conventions are modelled; every sentence is new.
3. **Generate reference tables from locked spec-data.** Message appendices, command and
   function lists, file-format tables and keyboard maps should be emitted from `spec/` (for
   example `spec/dos_messages.json`, `spec/samir/dbase_msg_codes.tsv`,
   `spec/samir/xbase_coercion.json`, `spec/samir/dbf_format.h`) so that the manual cannot
   drift from the software.
4. **Canon is documented, never explained.** The 116% pie chart, the `570-` trailing-minus
   format, the two-digit-year accounting sample, the hourglass, `PC LOAD LETTER`: each appears
   where a real manual would document a feature or show an example, with a straight face and
   no comment.
5. **Unified physical spec** (see `DESIGN-SYSTEM.md`): every manual is 7 x 9 in
   (178 x 229 mm); binders take a half-size three-ring mechanism (2.75 in / 70 mm spacing);
   perfect-bound books are black-only inside.
6. **Version numbers** below are proposals (open decision D-3 in the main report). They follow
   the repo's existing usage where one exists (InitechDOS 3.30 in `spec/dos_banner.txt`;
   InitechOS 3.30, part number `1024-3300-01` in `README.md`).

Page counts are estimates for the finished product, scaled from the reference original and
from how much of the software exists; they are planning numbers, not commitments.

---

## 1. InitechDOS Version 3.30 (model: IBM PC DOS 3.30)

**Package:** binder in the Initech standard slipcase, Systems family band. Software state:
boots on QEMU and Bochs, FAT12/16, INT 21h, PSP and loader (flat `.COM` and InitechMZ
`.EXE`), CONFIG.SYS, COMMAND.COM with batch files, redirection and pipes **[BUILT]**.

| Item | Binding | Trim | Est. pages | Role |
|---|---|---|---|---|
| InitechDOS Version 3.30 Reference | 3-ring binder, printed tab dividers | 7 x 9 in | 360-440 | The whole user-level reference (IBM "Reference") |
| InitechDOS Version 3.30 User's Guide | perfect-bound or saddle-stitched | 7 x 9 in | 64-96 | First-time setup and daily use (IBM "User's Guide") |
| InitechDOS Quick Reference | folded card or 3.8 x 8.3 in booklet | -- | 8-16 | Command summary |
| InitechDOS Technical Reference | separate product (Programming family), 3-ring binder | 7 x 9 in | 300-400 | INT 21h, PSP, FCB, InitechMZ, disk layout (IBM sold this separately) |

**InitechDOS Version 3.30 Reference -- chapter outline**

- Front matter: Edition Notice (IBM-style "First Edition (month year)"), warranty disclaimer,
  Reader's Comment Form paragraph, About This Book (Read This First / First-Time Users /
  Experienced Users / Terms Used / How This Book Is Organized).
- 1 Introduction to InitechDOS -- what an operating system does; diskettes and fixed disks;
  making backup copies of the distribution diskettes **[BUILT for the concepts; any
  DISKCOPY-like utility is PLANNED -- check before writing]**.
- 2 Starting InitechDOS -- the start-up sequence, the banner, the `A:\>` prompt, date and time,
  restarting **[BUILT]**.
- 3 Files and File Names -- 8.3 names, extensions, wildcards, reserved device names `CON`,
  `AUX`, `PRN`, `CLOCK$`, `NUL` (README "Features") **[BUILT]**.
- 4 Directories and Paths -- tree structure, `MD`/`CD`/`RD`, `PATH` **[BUILT]**.
- 5 Preparing a Fixed Disk -- the partition table contract (ADR-0003 DEC-07a) **[PARTIAL:
  document only what the shipped utilities do]**.
- 6 Standard Input and Output -- redirection, pipes, the filters `MORE`, `FIND`, `SORT`
  **[BUILT: `os/milton/{more,find,sort}_program.asm`, `bsy7_redir`/`bsy8_pipe` images]**.
- 7 Batch Files -- `AUTOEXEC.BAT`; `CALL`, `ECHO`, `FOR`, `GOTO`, `IF` (`NOT`, `EXIST`,
  `ERRORLEVEL`), `PAUSE`, `REM`, `SHIFT`; replaceable parameters **[BUILT: `batch.c`]**.
- 8 Commands (alphabetical, one entry per command: purpose, format, type internal/external,
  remarks, examples) -- internal: `BREAK`, `CD`/`CHDIR`, `CLS`, `COPY`, `DATE`, `DEL`/`ERASE`,
  `DIR`, `ECHO`, `EXIT`, `IF`, `MD`/`MKDIR`, `PATH`, `PROMPT`, `RD`/`RMDIR`, `REN`/`RENAME`,
  `SET`, `TIME`, `TYPE`, `VER` **[BUILT: `command.c`]**; external utilities as shipped.
- 9 Configuring Your System -- `CONFIG.SYS`: `BREAK`, `BUFFERS`, `DEVICE`, `FILES`,
  `LASTDRIVE`, `SHELL` **[BUILT: `config_sys.c`]**; the baseline files in
  `spec/dos_config_sys_baseline.txt` and `spec/dos_autoexec_bat_baseline.txt`.
- 10 Installable Device Drivers -- the resident devices; installing a character driver.
- Appendix A Messages -- generated from `spec/dos_messages.json`, alphabetical, each with
  cause and action. Includes the system-halt screen documented plainly: "PC LOAD LETTER --
  An unrecoverable condition has stopped the system. Record the information shown and
  contact your Initech representative." (wording to be written; the screen itself is bead
  `initech-s25`).
- Appendix B Keyboard Functions and Editing Keys.
- Appendix C Starting FLAIR (`WIN`) -- one page pointing to the FLAIR documentation.
- Glossary; Index; two Reader's Comment Forms with Business Reply Mail panels (IBM model).

**Box contents (InitechDOS):** binder in slipcase; diskettes in a sealed License Agreement
envelope (5.25-in set, see main report decision D-7); Proof of License card; Warranty
Registration Card; Quick Reference; User's Guide; Initech Customer Support Guide (common to
all products); "In This Package" card; 3.5-in media exchange coupon.

---

## 2. FLAIR Graphical Operating Environment (model: Apple System 7 kit; Windows 3.0 for the "runs on DOS" angle)

**Package:** perfect-bound books in the Initech standard box, Systems family band. Software
state: desktop, Finder windows with working scroll bars, menus, drag and drop, Trash, Select
All, Arrange, co-resident applications (HELLO, NOTES), application launch from disk;
several menu commands still drawn disabled (Get Info, Duplicate, Empty Trash, Restart,
Shut Down) **[PARTIAL: the window system is under active audit, `initech-tdnl`]**.

| Item | Binding | Trim | Est. pages | Role |
|---|---|---|---|---|
| FLAIR User's Guide | perfect-bound | 7 x 9 in | 200-260 | Learning + Using + Reference in one book (Apple "Macintosh Reference" model) |
| Getting Started with FLAIR | saddle-stitched | 7 x 9 in | 32-48 | Install and first session (Apple "How to Install") |
| FLAIR Accessories Guide | perfect-bound | 7 x 9 in | 64-120 | InitechCalc, InitechPaint, InitechText, File Manager, FILE COPY (as they exist) |
| FLAIR Quick Reference | card | -- | 2-4 panels | Mouse and keyboard shortcuts, menu map |

**FLAIR User's Guide -- outline.** Part 1 Learning: the desktop; the pointer and the mouse
(pointing, clicking, double-clicking, dragging); menus and the menu bar; windows (title bar,
close box, zoom box, collapse box, scroll bars, size box); icons; disks and the Trash
**[BUILT]**. Part 2 Using: opening and saving documents (Standard File dialogs), working with
several applications at once (application switching), copying files, organizing folders,
the Scrapbook/Clipboard (Scrap Manager) **[PARTIAL]**, printing **[PLANNED: Print Manager
`initech-o5vm`]**. Part 3 Reference: every menu command in order with its keyboard
equivalent (generated from the menu definitions in `os/flair/shell_menus.h` and
`spec/assets/menu_canon.h`), messages and alerts, the hourglass busy cursor (shown in a
figure without comment), starting FLAIR from InitechDOS (`WIN`), memory and co-residency
limits. Glossary, Index.

**FLAIR Accessories Guide.** InitechCalc chapter including the worked example that builds the
quarterly distribution chart -- the pie whose slices read 40, 35, 18, 14 and 9 percent,
printed as a figure with no remark **[PLANNED: InitechCalc R4.6]**; InitechPaint with its
`File Edit Image Layer Select View Window Help` menu bar documented as the standard menu
bar of the application **[PLANNED]**; File Manager and FILE COPY ("Saving tables to disk"
progress) **[PLANNED]**. Write only the chapters whose application ships.

**Developer documentation (later, separate box):** "Inside FLAIR" -- the Toolbox managers,
the ATKINSON region engine, the App Contract and INT 81h Toolbox Gate (ADR-0004, ADR-0005,
ADR-0013). Out of scope for the first physical edition; the ADRs are already most of the
content.

**Box contents (FLAIR):** books; diskettes in a License Agreement envelope; registration card;
Quick Reference card; Customer Support Guide; In This Package card; media coupon.

---

## 3. InitechBase (model: dBASE III PLUS 1.1, the reference target of ADR-0008 DEC-03)

**Package:** two binders (Volume One, Volume Two) in the Initech large slipcase box,
Information Management family band (the Ashton-Tate two-binder model). Software state:
SAMIR runs inside InitechOS; `.dbf`/`.dbt` I/O, dot-prompt interpreter, `.ndx` indexes,
REPLACE/APPEND persist (gates `test-samir-boot`, `test-samir-write`) **[BUILT]**; BROWSE /
EDIT full-screen, REPORT FORM / LABEL FORM, @SAY..GET forms, `.mdx` **[PLANNED]**.

| Item | Binding | Trim | Est. pages | Role |
|---|---|---|---|---|
| Volume One: Learning InitechBase + Using InitechBase | 3-ring binder, tab dividers | 7 x 9 in | 450-600 | Tutorial and encyclopedic reference |
| Volume Two: Programming with InitechBase + duplicate Index | 3-ring binder | 7 x 9 in | 250-400 | Program design and the sample applications |
| InitechBase Quick Reference Guide | tall booklet, 3.8 x 8.3 in | -- | 32-40 | Commands and functions with syntax (Ashton-Tate model) |
| Keyboard template | die-cut card | -- | -- | Function-key assignments, if the product binds F-keys |

**Volume One -- Learning InitechBase (section prefix L).** 1 Quick Start: what a database is,
fields and records, creating a structure, adding records, listing, quitting; 2 Entering and
Changing Data; 3 Finding Records (LOCATE, SEEK); 4 Sorting and Indexing (.ndx); 5 Using More
Than One File; 6 Reports **[PLANNED until REPORT FORM]**; Summary pages and "Notes" pages at
each chapter end.

**Volume One -- Using InitechBase (section prefix U).** 1 Overview and technical
specifications (limits from `spec/samir/dbf_format.h`); 2 Using Commands (syntax notation,
expressions, operators, work areas); 3 About Program Files (`.PRG`, `.DBF`, `.DBT`, `.NDX`,
`.MEM`); 4 Configuring InitechBase; 5 Commands A-Z; 6 Functions A-Z; 7 Error Messages
(generated from `spec/samir/dbase_msg_codes.tsv`); Appendix: data type and coercion rules
(generated from `spec/samir/xbase_coercion.json`), file structures, ASCII table; Glossary;
Index.

**Volume Two -- Programming with InitechBase (section prefix P).** Program design, branching,
looping, menus, procedures, memory variables and macro substitution (`&var`, `.mem`
files); the sample applications: **the Initech Accounting System** (bead `initech-586.1`,
built) documented as delivered, with its two-digit year fields described simply as the
date format the system uses; **the Initech salary rounding procedure** from bead
`initech-586.2` documented as a sample routine for currency arithmetic, without comment;
the TPS Report Generator if and when bead `8479.1` lands. Duplicate Index (Ashton-Tate
model).

**Box contents (InitechBase):** two binders in a vinyl-look slipcase box; diskettes in License
Agreement envelope; Quick Reference Guide; registration card; Customer Support Guide;
"In This Package" card; keyboard template envelope (if templates ship); TechNotes-style
newsletter sample (one sheet) and a companion-products flyer.

---

## 4. Initech 123 (model: Lotus 1-2-3 Release 2.2; epic `initech-68iw`)

**Package:** perfect-bound books in the Initech standard box, Decision Support family band.
Software state: plan and Law-1 golden harness done (`initech-qh83`); the grid, formula engine,
slash menus, /Data, /Print and /Graph are **[PLANNED]** (P1-P5). Write the manuals as the
phases land; the Reference outline below is the plan's feature inventory (INITECH-123-plan
Sec 4).

| Item | Binding | Trim | Est. pages | Role |
|---|---|---|---|---|
| Setting Up Initech 123, Tutorial, Quick Start and Sample Applications | perfect-bound | 7 x 9 in | 360-440 | One volume, as Lotus did |
| Initech 123 Reference | perfect-bound | 7 x 9 in | 600-720 | Commands, @functions, macros |
| Initech 123 Quick Reference | saddle-stitched booklet | about 4 x 9 in | 40-48 | Keys, @functions, menu trees |
| Keyboard templates | die-cut cards, two layouts | -- | -- | 10 F-keys at left (84-key) and 12 across the top (101-key) |

**Reference -- outline.** 1 Basics (worksheet 256 x 8192, cell pointer, modes, labels and
values, ranges, the control panel); 2 Worksheet Commands (`/Worksheet` Global, Insert,
Delete, Column, Erase, Titles, Window, Status, Page, Learn); 3 Range Commands (`/Range`
Format -- including the accounting display format that shows negative values with a
trailing minus, `570-`, documented as one format among the others -- Label, Erase, Name,
Justify, Protect, Input, Value, Trans, Search); 4 Copy and Move; 5 File Commands (.WK1
files, Combine, Xtract, Import, List, Admin); 6 Print Commands (to PRN spool and to file);
7 Graph Commands; 8 Data Commands (Fill, Table, Sort, Query, Distribution, Matrix,
Regression, Parse); 9 System, Add-In and Quit (`/System` documented with its actual
behaviour, which the plan says is an explicit "not supported" message); 10 @Functions A-Z
(math, statistical, financial, logical, string, date/time, lookup, database); 11 Macros
(key names, {commands}, `\a`-`\z`); Appendices: Troubleshooting, Recalculation, Memory,
File Compatibility; Glossary; Index.

**Sample Applications (in the Setting Up volume).** Include a quarterly results worksheet and
its pie chart with the canonical slice values, presented as an ordinary worked example.

**Box contents (Initech 123):** books; diskettes; Warranty Registration Card in the disk box;
keyboard templates; Quick Reference; Customer Support Guide; the Lotus-style **Hardware
Chart printed on a box side panel**, repeated in Setting Up (a strong period touch).

---

## 5. InitechWord (model: WordPerfect 5.1 for DOS; epic `initech-fdxa`)

**Package:** Reference in a binder with slipcase and printed paper sleeve; Workbook
perfect-bound; Office Systems family band. Software state: P0 golden harness done
(`initech-rv9y`); document core, codes, Reveal Codes, search, print **[PLANNED]** (P1-P4);
Face B gated on the Font Manager; dictionary ruling: reduced word list first, companion
volume later (operator ruling 2026-10-03).

| Item | Binding | Trim | Est. pages | Role |
|---|---|---|---|---|
| InitechWord Reference | 3-ring binder, printed bleed-tab dividers | 7 x 9 in | 700-950 | Alphabetical feature encyclopedia |
| InitechWord Workbook | perfect-bound | 7 x 9 in | 300-420 | Lessons with practice documents |
| Function-key template | long strip and full template | -- | -- | Four commands per key in four colours |
| Program installation card | card, 7 x 9 in | -- | 2 | Numbered installation steps |
| Speller and Thesaurus | (companion volume diskettes) | -- | -- | Per the dictionary ruling |

**Reference -- outline.** Front: offices and support, conventions. Section 1 Getting Started
(Basics, Registration, Installation, The Template, Starting InitechWord, Keys to Know,
Function Keys, Menus, Codes, Exit, Help). Section 2 Features A-Z, one entry per feature with
keystrokes, menu path, notes and "See Also" (Block, Bold, Cancel, Center, Codes, Columns,
Cursor Movement, Date, Delete, Endnotes, Exit, Flush Right, Footnotes, Headers and Footers,
Hyphenation, Indent, Justification, List Files, Margins, Move, Page Break, Print, Replace,
Retrieve, Reveal Codes, Save, Search, Setup, Spell, Switch, Tab Set, Tables, Thesaurus,
Underline, Undelete, ...; each entry written only when its phase ships). Section 3
Appendix (the `.WP5` file format note, printer files, error messages, ASCII/character
sets). Index.

**Box contents (InitechWord):** Reference in slipcase with paper sleeve; Workbook;
diskettes in License Agreement envelope; templates; install card; registration card;
Customer Support Guide.

---

## 6. Turbo Initech (model: Borland Turbo Pascal 6.0)

**Package:** four perfect-bound books in the Initech standard box, Programming family band.
Software state: the resident compiler is real and compiles its corpus on the OS
(`test-compiler-os` 17/17); language is the ADR-0007 DEC-02 subset (32-bit `integer`,
complete boolean evaluation, no pointers, sets, reals or units yet); the IDE and the
`K2 == K3` self-host certificate are **[PLANNED]** (M8).

| Item | Binding | Trim | Est. pages | Role |
|---|---|---|---|---|
| Turbo Initech User's Guide | perfect-bound | 7 x 9 in | 180-260 | Install, the IDE, tutorial, command-line compiler |
| Turbo Initech Programmer's Guide | perfect-bound | 7 x 9 in | 200-320 | Language definition, run-time, directives, error messages |
| Turbo Initech Library Reference | perfect-bound | 7 x 9 in | 80-160 | Run-time library A-Z |
| Turbo Initech Self-Compilation Guide | perfect-bound | 7 x 9 in | 60-120 | Building Turbo Initech from its own source (fourth book; replaces Borland's Turbo Vision Guide) |
| Quick Reference | booklet | about 4 x 9 in | 32-48 | Editor keys, directives, error numbers |

**User's Guide -- outline.** Introduction (the four manuals, installing, the README file,
typefaces used in these books, how to contact Initech); 1 The Integrated Environment
**[PLANNED: M8 IDE]**; 2 Programming in Turbo Initech (tutorial) **[BUILT for the
command-line compiler]**; 3 The Command-Line Compiler **[BUILT]**; 4 Debugging; 5 Managing
Larger Programs (whatever exists when the book is cut); Appendix: the editor from A to Z.

**Programmer's Guide -- outline.** Part 1 The Language, following ADR-0007 DEC-02 exactly
(tokens, constants, types, variables, expressions with complete boolean evaluation,
statements, procedures and functions including `forward`, programs); the restrictions are
stated as the definition of the language ("Turbo Initech implements Initech Standard
Pascal"), not as omissions. Part 2 The Run-Time Library boundary (ADR-0007 DEC-05) and
memory. Part 3 Compiler Directives. Appendix Error Messages (generated from the compiler's
message table).

**Self-Compilation Guide -- outline.** Recompiling Turbo Initech from the supplied source;
verifying that the compiler reproduces itself bit for bit (the `K2 == K3` procedure, written
as an acceptance procedure an MIS department would run); reproducible builds. Write after
`make selfhost` is real.

**Box contents (Turbo Initech):** four books; diskettes; "Initech No-Nonsense License
Statement"-style single-sheet license (Borland model, with original wording); registration
card; **Run-Time Library Source order form** (Borland shipped one; an in-universe order form
is a perfect deadpan item); Quick Reference; Customer Support Guide.

---

## 7. Items common to every box

| Item | Model | Notes |
|---|---|---|
| License Agreement envelope (holds the diskettes; opening it accepts the license) | IBM, Ashton-Tate, WordPerfect | One Initech-wide license text; "This product is licensed, not sold" (README) |
| Warranty Registration Card | Lotus, Borland, Apple | Perforated postcard, business-reply back; product-specific part number |
| Proof of License card | IBM | Optional for the OS products |
| "In This Package" card | Ashton-Tate | Checklist of the box contents with part numbers |
| Initech Customer Support Guide | Ashton-Tate | One common booklet: registration, updates, disk replacement, support plans, 90-day warranty, license, phone numbers, Disk Replacement Card, Change of Address Card. Technical Support: extension 2504 (README) |
| Reader's Comment Form | IBM | Bound at the back of every binder manual |
| Media exchange coupon | period practice | Offers the other diskette size by mail |
| Companion products flyer | Ashton-Tate, Lotus | Lists the other Initech products with their part numbers -- ties the line together |
