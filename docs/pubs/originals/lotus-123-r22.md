<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals: Lotus 1-2-3 Release 2.2

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural reference for typesetters and technical writers)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.2`, originals scour B)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-L123 |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Sibling | `docs/pubs/originals/ashton-tate-dbase3plus.md` (same lane); `wordperfect-51.md`, `borland-turbo-pascal.md` (lane C) |
| Prior art | `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` s.2 (this annex deepens it) |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/lotus123/` (outside the repo; manifest `SOURCES.md`) |

### Rules of use

1. **Structure, not prose.** No sentence of the originals is reproduced; quoted words are
   limited to the few needed to name a convention (a label such as "Procedure" or "Tip").
2. **No trade dress.** Do not copy the 1-2-3 cover illustration or logotype, the magenta,
   or the title lock-up. Mirror the *system* (single-column grid, menu-tree openers,
   procedure anatomy, section-coded master index), not the look.
3. **Provenance tags.** [READ] seen in the scan. [MEAS] measured from a 300-ppi render
   (pixel row profile; these scans are true 300 ppi, but page registration on the scanner
   varies by up to about 0.3 in, so *margins* are unreliable while measures, line pitch and
   head sizes are good to about 2 %). [INF] inferred. Typeface names are by eye unless the
   book's own trademark page names them.
4. **Page citations.** Printed folio first, then the PDF page.

| Book | File | PDF pages | Folio to PDF |
|---|---|---|---|
| Reference | `Lotus 1-2-3 2.2 Manuals (1989)/Lotus 1-2-3 Release 2.2 - Reference Manual.pdf` | 700 | iii = PDF 5; xi = 13; 1-1 = 17; 3-1 = 69; 4-1 = 75; 13-1 = 289; 14-25 = 377; F-1 = 629; Glossary-1 = 649; Index-1 = 671 |
| Setting Up 1-2-3, Tutorial, Quick Start and Sample Applications (one volume, four parts) | `... - Setting Up.pdf` | 436 | SU 1-1 = PDF 13; Tutorial title PDF 99, TU 1-1 = 109; Quick Start title PDF 269, QS 1-1 = 277; Sample Applications title PDF 351, SA 1-1 = 359; Master Index-1 = 399 |
| Upgrader's Handbook | `... - Upgrader_s Handbook.pdf` | 76 | iii = PDF 5; 3-1 = 27 |
| R3 Reference (comparison only) | `Lotus_123_R3_Reference_Manual_1989_text.pdf` | 672 | text layer; used for one comparison row (s.12) |

The interior scans are 1-bit (bilevel) at 300 ppi; covers are 24-bit colour [MEAS:
`pdfimages -list`]. A bilevel scan cannot show a second ink, so "black only" is [INF]
strengthened by the absence of any colour reference in the conventions pages.

---

## 1. What the set is for, and how it divides the material

The Reference's own documentation table (xii, PDF 14) and the Reader Comment Form (PDF 411,
Setting Up volume) list seven pieces: Setting Up 1-2-3, Tutorial, Quick Start, Sample
Applications, Reference, Upgrader's Handbook, Quick Reference [READ].

| Piece | Index code | Job | Organisation | Cite |
|---|---|---|---|---|
| **Setting Up 1-2-3** | SU | Check the package, initialise, copy disks, run Install, start/stop, Allways setup, hardware notes, problems | 8 chapters | SU v-vi, PDF 7-8 |
| **Tutorial** | TU | Graded step-by-step skills for newcomers, one sample business (a sporting-goods shop) running through | 5 chapters containing about 20 numbered **lessons** (lesson numbering runs across chapters) | TU vii, PDF 105; lesson heads PDF 109, 193, 215 |
| **Quick Start** | QS | Fast overview for people who learn by doing but already know spreadsheets | 5 short chapters, each ending in a Task Summary | QS PDF 273-347 |
| **Sample Applications** | SA | Ready-made worksheet templates with explanation of each cell block | 7 chapters, one application each | SA PDF 351-398 |
| **Reference** | (none; own index) | Every command, @function, macro command and utility | 18 chapters + App. A-F + Glossary + Index | Ref iii-x, PDF 5-12 |
| **Upgrader's Handbook** | -- | Install over an earlier release; what is new; compatibility | 4 chapters | UH iii, PDF 5 |
| **Quick Reference** | -- | Keys, operators, file types, macro commands, @functions, and a "Task Reference" mapping tasks to commands | Not scanned | named Ref F-1, PDF 629; Ref xii |

The Reference's chapter split (Ref xi, PDF 13 [READ]): 1-14 = 1-2-3 itself (basics, then
one chapter per top-level menu command in **main-menu order**, then @functions and
macros); 15-18 = the four bundled programs (PrintGraph, Translate, Macro Library Manager,
Allways). Every chapter is self-contained and the reader is told to start with ch. 1 then
read in any order.

Cross-references always name the book in italic ("Chapter 6 of *Reference*",
"Lessons 14 and 15 in the *Tutorial*") [READ PDF 76, 297].

---

## 2. Physical format and page grid

| Item | Finding | Tag / cite |
|---|---|---|
| Binding | Perfect-bound paperbacks, one per piece; the Setting Up volume binds four parts with half-titles | [INF: cover edge shows spine colour; no ring holes; prior survey] |
| Trim | 6.97 x 9.02 in = **7 x 9 in** (2091 x 2705 px at 300 ppi; Upgrader's 6.89 x 8.86 in, 2067 x 2659 px) | [MEAS PDF 4, 76] |
| Columns | **Single column**, full measure; no side-head column | [READ throughout] |
| Measure | 5.20-5.25 in (~31 pc) from text left edge to head-rule end | [MEAS PDF 69, 79, 80: text x 0.88-6.08, rules to 6.14-6.18] |
| Bullet indent | first-level bullet text indented 0.19-0.21 in; procedure turnovers the same | [MEAS PDF 79] |
| Body line pitch | 0.163-0.166 in = **12 pt** | [MEAS PDF 79, 80] |
| Index line pitch | 0.14 in = 10 pt; 3 columns (Master Index) / 3 columns (Reference index) | [MEAS PDF 672; READ PDF 399, 672] |
| Running heads | **None** | [READ] |
| Running foot | Folio at outer foot corner; next to it a text label (s.2.1) | PDF 69-72 |
| Margins | Head and inner/outer margins could not be measured reliably (scan registration shifts the page by up to 0.3 in) | -- |
| Chapter start | Chapters open on rectos (odd folio x-1); blank versos inserted as needed | PDF 17, 69, 297 (blank PDF 320 before a recto opener) |

### 2.1 Running-foot rule

| Page | Foot content (serif, ~9 pt) | Example (described) | Cite |
|---|---|---|---|
| Recto | the **A-head in force** at the foot of the page, then the folio flush right | "<section name>  3-1" | PDF 69, 71, 79 |
| Verso | the folio flush left, then the **chapter title** | "3-2  <chapter title>" | PDF 70, 72, 80 |
| Front matter | roman folio with section name | "<section>  xi" | PDF 13 |
| Index / Glossary | "Glossary-1", "2-Index" (word + number) | PDF 649, 672 |

A thin rule seen across the bottom of many page images is the scanner bed edge, not a printed
rule [INF: position varies page to page].

---

## 3. Typography

The Reference title verso names Helvetica and Palatino among its trademarks, and CG Times,
CG Courier and CG Triumvirate [READ PDF 4]; the visible faces match **Palatino** (body) and
**Helvetica** (headings, tables, captions) [READ + INF]. The Setting Up volume's title verso
names Helvetica and Times instead [READ PDF 4 of Setting Up]; its interior faces look
identical to the Reference [INF].

| Role | Face | Size / leading | Cite |
|---|---|---|---|
| Body | Palatino roman; italic for book titles, emphasis, argument placeholders | 10 / 12 [MEAS pitch; INF size] | PDF 13, 79 |
| Defined terms in running text | Palatino **bold** at first use (and defined in the Glossary) | -- | PDF 18, 109 |
| Chapter opener | two lines flush left: "Chapter n" and the title, Helvetica bold, **title sometimes bold italic** for command chapters ("The Copy Command") | ~18 / 20 [MEAS: bands 0.25 in, pitch 0.27 in] | PDF 17, 69, 75 |
| A-head | Helvetica bold U/lc, ~14 pt, with a ~2 pt rule beneath running the full measure (the rule starts under the head and continues to the right margin) | [MEAS band 0.17-0.21 in; rule 0.027 in] | PDF 69, 79 |
| Command entry head | Same as A-head, set as the command path ("/Data Fill") | -- | PDF 79 |
| B-head | Helvetica bold ~11-12 pt, no rule | -- | PDF 79 ("Procedure", "Terms You Need to Know") |
| Sub-command head | Helvetica bold ~11 pt, no rule ("/Data Matrix Invert") | -- | PDF 80 |
| Run-in labels | **NOTE**, **CAUTION** in Helvetica bold caps followed by body text on the same line | -- | PDF 71-72, 81 |
| Small label heads | "Tip", "Example", "Examples", "Procedure" in Palatino bold, own line | -- | PDF 79, 303 |
| @function / macro entry head | Helvetica bold ~12 pt, no rule (@ABS, {BEEP}) | -- | PDF 303, 377 |
| Tables | Helvetica ~8 pt, bold column heads over a rule, hairline under the last row; no verticals | -- | PDF 75, 304 |
| Figure captions | Helvetica ~8 pt, flush left under the figure: "Figure 3-1" then a wide space then the caption | -- | PDF 70 |
| Callout labels | Helvetica ~6-7 pt at the right of a figure with a horizontal leader | -- | PDF 18, 303 |
| Key names | Palatino **small capitals** (ENTER, HOME, HELP (F1), ALT-F4) | -- | PDF 13, 72 |
| User-typed text | "a different typeface": Helvetica (e.g. directory names typed at the prompt) | -- | PDF 13 convention; TU PDF 110 |

---

## 4. Colour and covers

| Element | Finding | Cite |
|---|---|---|
| Interior | One colour (black). Screens printed as halftone grey panels; no tint panels or coloured heads | [READ/INF, see header note] |
| Reference and Setting Up covers | White board; title lock-up top left: "Lotus" in grey condensed serif, "1-2-3 Release 2.2" in black bold condensed serif with the ".2" smaller, over a **dotted rule**; subtitle below in italic serif (book name, or the four part names); a large airbrushed 3D illustration of the numerals 1-2-3 with bars and a pie, in purple, violet, teal and grey | PDF 1 (both) |
| Spine colour | Magenta band about 0.10 in visible at the spine edge of the Reference front and back covers, scan about RGB(154, 16, 87) | [MEAS PDF 1, 700] |
| Upgrader's Handbook cover | Solid deep magenta/aubergine, about RGB(106, 5, 79); a small grey square tilted logo plate; spine title reading top-to-bottom | [MEAS/READ UH PDF 1, 76] |
| Back covers | Reference and Setting Up: blank white; "Part No. nnnnnn" in small Helvetica bottom right. Upgrader's: solid magenta with the part number in black | PDF 700, 436, 76 |
| Half-title / title page | Lock-up repeated in black, subtitle italic, and the cover illustration redrawn as **outline line art** | Ref PDF 3; SU PDF 5, 99, 269, 351 |

---

## 5. Front matter and conventions

| Leaf | Content | Cite |
|---|---|---|
| Inside cover | blank | SU PDF 2 |
| Part locator (Setting Up only) | One leaf listing the volume's parts down the outer edge at stepped heights (Setting Up 1-2-3, Tutorial, Quick Start, Sample Applications, Customer Assurance Plan, Master Index) -- the key to stepped edge marks [INF] | SU PDF 3 |
| Title page | outline-art title page (s.4) | Ref PDF 3 |
| Title verso | copy-prohibition sentence; copyright years with the corporate address set as a hanging block; all-rights line + "first edition" + country of printing; trademark paragraph | Ref PDF 4 |
| Contents | "Contents" Helvetica bold ~18 pt; chapter lines Helvetica bold "Chapter n" + title + dot leader + bold folio; A-heads in Palatino bold with leaders; B-heads indented Palatino roman; generous space between chapters; continues over 8 pages (iii-x) in the Reference | Ref PDF 5-12 |
| "About <Book>" | Fixed sub-sections: Who Should Read This Book; **Conventions**; Organization of <Book>; Other 1-2-3 Documentation (two-column Manual / Description table); After You Complete <Book> | Ref PDF 13-14; SU PDF 9-11; TU PDF 105-108; QS PDF 273-276 |

Conventions list (identical in every book, bulleted, each with an "Example:" line)
[READ Ref xi, PDF 13; SU vii, PDF 9]:

| Rule | Form |
|---|---|
| Function and special keys | small capitals; function keys as key name + key in parentheses (HELP (F1)) |
| Hold-then-press | key names joined by a hyphen (CTRL-arrow) |
| Press-then-press | key names separated by a space (END HOME) |
| Typed information | a different typeface (sans) |
| Instructions to follow as read | bold (Press) |
| Other bold words | defined in place and in the Glossary |

---

## 6. Reference: chapter inventory (start folios from Contents, PDF 5-12)

| Ch. | Title (described) | Starts | Last listed | PDF start |
|---|---|---|---|---|
| 1 | Using 1-2-3 (screen, indicators, keys, menus, settings sheets, data entry, ranges, formulas, files, linking, undo, help) | 1-1 | 1-47 | 17 |
| 2 | Add-In commands | 2-1 | 2-4 | 65 |
| 3 | Copy | 3-1 | 3-4 | 69 |
| 4 | Data | 4-1 | 4-47 | 75 |
| 5 | File | 5-1 | 5-23 | -- |
| 6 | Graph | 6-1 | 6-31 | -- |
| 7 | Move | 7-1 | 7-2 | -- |
| 8 | Print | 8-1 | 8-22 | -- |
| 9 | Quit | 9-1 | -- | -- |
| 10 | Range | 10-1 | 10-23 | -- |
| 11 | System | 11-1 | -- | -- |
| 12 | Worksheet | 12-1 | 12-41 | -- |
| 13 | @Functions | 13-1 | 13-14 (descriptions run on) | 289 |
| 14 | Macros (incl. advanced macro command descriptions 14-22 ff., /X commands, sample macros) | 14-1 | 14-63 | 353 |
| 15 | PrintGraph | 15-1 | 15-25 | -- |
| 16 | Translate | 16-1 | 16-11 | -- |
| 17 | Macro Library Manager add-in | 17-1 | 17-17 | -- |
| 18 | Allways spreadsheet publishing (a book within the book: its own command chapters for Display, Format, Graph, Layout, Print, Special, Worksheet) | 18-1 | 18-88 | -- |
| A | Lotus International Character Set (LICS) tables | A-1 | A-5 | -- |
| B | Printing with setup strings (incl. table of setup strings) | B-1 | B-9 | 587 |
| C | Managing memory | C-1 | C-6 | 599 |
| D | Using 1-2-3 on a network | D-1 | D-6 | 609 |
| E | HP LaserJet printers / PCL | E-1 | E-11 | 615 |
| F | Troubleshooting (by area: database, file, formulas, graph, macros, networks, print, range names, undo, worksheet) | F-1 | F-18 | 629 |
| -- | Glossary | Glossary-1 | -- | 649 |
| -- | Index | Index-1 | -- | 671 |

There is **no error-message appendix** in Release 2.2: errors are covered by on-line Help
and the problem/solution appendix F [READ Contents PDF 12; F-1 PDF 629].

---

## 7. Reference: command chapter and command-entry anatomy

### 7.1 Command-chapter opener (fixed order)

| # | Element | Form | Cite |
|---|---|---|---|
| 1 | Chapter title | two-line Helvetica bold | PDF 65, 75 |
| 2 | **Menu tree** | boxed diagram of the 1-2-3 main menu with the chapter's command highlighted by a connecting line down to its submenu box, and further boxes for each sub-level; menu words in ~5-6 pt Helvetica | PDF 65, 75 |
| 3 | Purpose paragraph | one or two sentences | PDF 75 |
| 4 | Command / Task table | two columns, one row per sub-command, in menu order | PDF 75 |
| 5 | "Common Uses of the <X> Commands" | A-head + bulleted list of tasks with the command path in parentheses | PDF 69, 76 |
| 6 | "Related Reading" | A-head + bullets pointing to chapters and Tutorial lessons; a closing line about pressing HELP (F1) | PDF 69, 76 |
| 7 | "Terms You Need to Know" | A-head + bullets, each term **bold** at its start | PDF 69 |
| 8 | "Before You Use <command>" | A-head, cautions and prerequisites (where needed) | PDF 70 |
| 9 | Command entries | in menu order, **run on** (not one per page) | PDF 72, 79-82 |

### 7.2 Command entry skeleton

| # | Element | Form | Cite |
|---|---|---|---|
| 1 | Head | command path "/Data Fill" as A-head with rule | PDF 79 |
| 2 | Purpose | one sentence | PDF 79 |
| 3 | Figure | screen with settings/prompt, numbered caption | PDF 79 |
| 4 | Terms You Need to Know | B-head + bullets with bold terms (entry-local glossary) | PDF 79 |
| 5 | How it works | paragraph(s) | PDF 79 |
| 6 | **Procedure** | B-head; numbered steps, numerals bold; each step an imperative ending in "press ENTER"; an indented unnumbered paragraph under a step explains defaults/choices; nested bullets for options | PDF 72, 79-80 |
| 7 | Tip | Palatino bold label + bullets | PDF 79-80 |
| 8 | NOTE / CAUTION | run-in labels; CAUTION paragraphs typically about overwriting data and the undo key | PDF 72, 81 |
| 9 | Sub-commands | each as a smaller head ("/Data Matrix Invert") with its own purpose, figure, Procedure, CAUTION | PDF 80-82 |
| 10 | Worked example | occasionally an A-head "<Task> with <command>: An Example" with its own numbered steps | PDF 82 |

Option lists inside steps use a two-column hanging table (option word / effect) between two
hairlines [READ UH PDF 28; Ref PDF 75].

### 7.3 @function entry (ch. 13) and advanced macro command entry (ch. 14)

| Element | @function | Macro command | Cite |
|---|---|---|---|
| Chapter front | "How to Use This Chapter"; "Syntax of @Functions" with an annotated syntax diagram (labels "@ sign", "Function name", "Open parenthesis", "Arguments", "Argument separators" with leaders); "Types of @Functions" by category, each with a summary table and guidelines | "Advanced Macro Command Descriptions" with conventions for arguments | PDF 290-302, 373 |
| Order | alphabetical, run on, several per page | alphabetical, run on | PDF 303-304, 377 |
| Head | name only ("@ABS"), Helvetica bold ~12 pt | name in braces ("{BEEP}") | PDF 303, 377 |
| Lead | syntax-as-sentence: the call with italic argument names, then what it returns | same, with italic optional arguments in brackets | PDF 303, 377 |
| Argument rule | separate short paragraph stating what each argument may be | paragraph | PDF 303 |
| Usage / notes | paragraphs, NOTE run-ins, value tables (x / y / result) | paragraphs | PDF 304 |
| Examples | "Example(s)" label then lines of formula = result (with units in parentheses) or indented macro code | "Examples" + indented code lines | PDF 304, 377 |
| Figure | worksheet screen with callouts where useful | rare | PDF 303 |

---

## 8. Tutorial and Quick Start anatomy (Setting Up volume)

### 8.1 Tutorial lesson

| Element | Form | Cite |
|---|---|---|
| Chapter opener | "Chapter n / <title>" + orientation + figure of the finished worksheet the chapter builds | TU 1-1, PDF 109 |
| Lesson head | A-head with rule: "Lesson n: <title>"; lesson numbers continue across chapters | PDF 109, 193, 215 |
| Lesson goals | one line + bullets of what you will do | PDF 109-110 |
| Task sub-heads | B-heads (e.g. per platform: hard-disk vs two-diskette) | PDF 110 |
| **Keystroke table** | two-column hanging list: action verb in Palatino bold at left (**Type**, **Press**, **Select**, **Move**, **Insert**, **Highlight**), what to do at right -- typed text in Helvetica, keys in small caps -- followed by an *italic* gloss of the purpose ("to move to ..."); verb column about 0.7 in | PDF 110, 113 |
| Commentary | body paragraphs between keystroke blocks explain what changed on screen | PDF 113 |
| Screens | grey halftone screen panels with right-side callouts | PDF 109 |
| Running foot | recto shows the current **Lesson** head | PDF 109, 113 |

### 8.2 Quick Start chapter

Opener ("Chapter n / title" + intro + NOTE) > "What This Chapter Shows You" (bullets) >
task sections with keystroke tables > chapter-end **"Task Summary"** (two-column
"To / You used" table, Helvetica, rule under heads) > **"Other Tasks to Try"** (paragraphs +
"To / Use" table) [READ QS PDF 277-278, 297, 319].

### 8.3 Sample Applications chapter

One application per chapter: scenario narrative > bullet list of inputs > per-block A-heads
("Entering the Data", "Completing the <x> Table", "Interpreting the Results", "Changing
the Template", "Printing Your Work") > three-column "Item / Address / Meaning" tables
(cell ranges in the middle column) [READ SA PDF 359, 365, 397].

---

## 9. Glossary, troubleshooting, indexes, back matter

| Item | Anatomy | Cite |
|---|---|---|
| Troubleshooting (App. F) | Intro + "Related Reading"; then A-heads per area; each entry a **Problem:** paragraph wholly in bold (run-in bold label) then **Solution:** run-in label + bullets; code lines in Helvetica | Ref PDF 629-630 |
| Glossary | Title "Glossary" Helvetica bold, intro line; term in Helvetica bold lower case on its own line, definition Palatino below; bold words inside definitions are themselves entries | Ref PDF 649 |
| Reference index | 3 columns, Palatino ~8/10; letter-group heads single Helvetica bold capital ~14 pt; sub-entries indented ~0.1 in, turnovers ~0.2 in; ranges "1-25 to 1-26"; "*see*" / "*See also*" italic; key names in small caps | Ref PDF 672 |
| Master Index (Setting Up volume) | Title "Master Index" Helvetica bold; boxed key of codes (SU, TU, QS, SA with italic book names); 3 columns; each entry's locators on separate lines, **one line per book**, prefixed by the bold book code | SU PDF 399-400 |
| Reader Comment Form | Helvetica bold title; mail-to block; check-line questions; a rating grid (rows = the seven pieces, columns = Excellent, Very good, Good, Fair, Poor, Didn't use); free-text questions; a "cut here" dashed line along the binding edge | SU PDF 411 |
| NOTES pages | "NOTES:" Helvetica bold caps centred at head, page otherwise blank; 22 leaves fill the Setting Up volume | SU PDF 413-434; UH PDF 67-74 |

---

## 10. Figures and screen reproductions

| Convention | Form | Cite |
|---|---|---|
| Screens | Real 1-2-3 screen captures (control panel, worksheet frame, status line) reproduced as a light grey halftone panel with rounded corners, no bezel | PDF 18, 79 |
| Callouts | horizontal leaders to labels at the right of the panel; bracket-style leaders below for row/column features | PDF 18, 278 |
| Diagram figures | worksheet-grid drawings with dotted cell lines and boxed cells to show FROM/TO ranges | PDF 70-71 |
| Keyboard figures | function keys drawn as rounded keycaps in two rows (plain and ALT) with the key's 1-2-3 name above/below | PDF 26 ("Figure 1-6") |
| Menu trees | boxed menu hierarchy at command-chapter openers | PDF 65, 75; UH PDF 28 |
| Numbering | chapter-sequential, "Figure 13-8" | PDF 303 |

---

## 11. Part numbers, edition notices

| Item | Code / notice | Cite |
|---|---|---|
| Reference | Part No. 300367 | PDF 700 |
| Setting Up 1-2-3, Tutorial, Quick Start and Sample Applications | Part No. 300368 | PDF 436 |
| Upgrader's Handbook | Part No. 300370 | UH PDF 76 |
| Quick Reference | not scanned (prior survey cites 33064 from a seller list, a different printing) | -- |
| Edition statement | copyright years 1986 and 1989; "first edition printed 1989"; printed in Ireland (both main books) | Ref PDF 4; SU PDF 4 |
| Pattern | six-digit part number, back cover only, no print-date code on the scanned leaves; 300369 presumably the Quick Reference [INF from the gap] | -- |

---

## 12. Comparison note: Release 3 Reference (text layer)

Release 3's Reference restructures the set: ch. 1 basics, ch. 2 **all commands in one
chapter**, then @functions and macros; appendices are **numbered** (Appendix 1-8, folios
"A2-1") rather than lettered; printed in the United States, first edition 1989
[READ: text layer, front matter and contents]. Release 2.2's one-chapter-per-menu-command
layout is the model to follow for an Initech spreadsheet reference.

---

## 13. Design facts to mirror (summary)

1. 7 x 9 in perfect-bound paperbacks; single full-measure column ~31 pc; Palatino ~10/12
   body; Helvetica bold heads; black ink only inside, colour spent on covers.
2. A-heads carry a ~2 pt rule to the right margin; B-heads and entry sub-heads plain.
3. No running heads; running foot = folio outer + current A-head (recto) / chapter title
   (verso).
4. One Reference chapter per top-level menu command in menu order, each opening with a
   boxed menu-tree diagram, a Command/Task table, then Common Uses / Related Reading /
   Terms You Need to Know.
5. Command entry: path head > purpose > screen > Terms > Procedure (numbered, "press ENTER")
   > Tip > NOTE/CAUTION; sub-commands nested; entries run on.
6. Tutorial keystroke tables: bold verb column (Type/Press/Select/Move) + sans typed text +
   small-caps keys + italic purpose gloss.
7. Chapter-end Task Summary ("To / You used") and Other Tasks to Try in the quick guide.
8. Volume of several parts with outline-art half-titles and a Master Index whose locators
   are prefixed by bold two-letter book codes (SU/TU/QS/SA).
9. Problem/Solution troubleshooting appendix instead of an error-message list.
10. Back matter: Reader Comment Form with a rating grid naming every piece of the set; NOTES
   leaves to fill the final signature.

## 14. Not determined from the scans

- Keyboard templates and the Quick Reference card/booklet (not in the archive); the box and
  its printed Hardware Chart; Customer Assurance Plan pages (listed on the part locator,
  SU PDF 3, but not found in the scan); warranty card.
- Whether the interior carried a second colour (scans are bilevel; nothing suggests one).
- Exact margins (scan registration varies); paper stock; spine design and spine width.
- Exact cover colours (scanner-uncalibrated) and the cover title face (a condensed serif,
  by eye).
- How the stepped labels on SU PDF 3 map to physical edge marks (if any) on the trimmed
  block.
