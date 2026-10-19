<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals: WordPerfect 5.1 for DOS

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural reference for typesetters and technical writers)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.3`, originals scour C)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-WP51 |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Sibling | `docs/pubs/originals/borland-turbo-pascal.md` |
| Prior art | `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` s.3 (this annex deepens it; where they differ, this one was read more closely) |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/wordperfect51/` (outside the repo; manifest `SOURCES.md`) |

### Rules of use

1. **Structure, not prose.** These notes record grids, hierarchies, notations and anatomy.
   No sentence of the originals is reproduced; quoted words are limited to the few needed to
   name a convention (a label such as "Notes" or "See Also").
2. **No trade dress.** Do not copy the WordPerfect wordmark, cover photography, the triangle
   field artwork or the blue as a brand colour. Mirror the *system* (two-colour interior,
   bleed tabs, notation), not the look.
3. **Provenance tags.** [READ] seen in the scan at the cited page. [MEAS] measured from a
   150-dpi render of the scan (method: pixel row/column profile; scan scale is not
   guaranteed, so absolute sizes carry about +/-4 %; relative proportions are good to about
   1 %). [INF] inferred. Typeface names are always identification by eye, hence [INF].
4. **Page citations.** "Ref p. 31" is the printed folio; the scan's PDF page is given once per
   book as an offset. Front matter is cited by roman folio or PDF page.

| Book | File | PDF pages | Printed-folio to PDF offset |
|---|---|---|---|
| Reference (6/92 printing) | `WPDOS51Reference.pdf` | 1048 | Getting Started, Reference, Appendix, Glossary/Index: PDF = folio + 17. Front matter: iii = PDF 6, v = 8, ix = 12, xiii = 16 |
| Workbook (11/89) | `WordPerfect 5.1 Manual/WordPerfect 5.1 - WordPerfect Workbook for IBM PC.pdf` | 512 | PDF = folio + 9 (Introduction v = PDF 6) |
| Function-key strip | `WordPerfect 5.1 Manual/WP51 Funct Strip.jpg` | 3492 x 411 px | -- |
| Install cards (12/90) | `media/WP51_1990-12_installcard_*.jpg` (4 sides) | 600 dpi | -- |
| Disk labels | `media/WP51_1990-12_media_*.jpg`, `media/WP51_1992-03_disks*.jpg` | 600 dpi | -- |

---

## 1. What the set is for, and how it divides the material

| Component | Job | Organisation | Evidence |
|---|---|---|---|
| **Reference** | The single authority on every feature; also the installer's and beginner's first stop | Four tabbed sections: *Getting Started* (install, keys, menus, a short guided lesson), *Reference* (202 features, A to Z), *Appendix* (A to T, 20 lookup tables and technical topics), *Glossary/Index* | TOC PDF 4-5; self-description Ref p. 12 |
| **Workbook** | Teach by doing: produce realistic business documents end to end | 36 lessons in three graded sections (*Fundamentals I*, *Fundamentals II*, *Special Features*), each built on supplied LEARN files | Workbook TOC pp. iii-iv; Introduction pp. v-viii |
| **Keyboard template** | Put the whole function-key map under the user's fingers | One strip, four colour-coded modifier rows per key | Strip scan; explained Ref p. 5-6 |
| **Install cards** | Update/printer installation without opening the binder | Two cards, one task each, numbered steps | Install card scans |
| **On-line tutorial, Help (F3)** | Interactive first contact; context help | Referenced, not printed | Ref p. 12-14 |

Division of labour, stated by the books themselves (paraphrase): the Reference describes how
each feature works and sends the reader to *Notes* and *See Also*; the Workbook shows
features *combined* for an application; the Workbook explicitly assumes the reader has
installed the program and read *Getting Started* first (Workbook p. v). Cross-references go
both ways: Reference steps say "see *X* in *Reference*" (always naming the book even inside
it), Workbook text sends the reader to named Reference entries (Workbook p. 4).

**Mirror for InitechWord:** Reference (A-Z features) + Workbook (lessons on sample files) +
template + install cards. The Reference's built-in primer (Getting Started) means there is no
separate "Getting Started" booklet in this set.

---

## 2. Reference -- physical and page grid

| Aspect | Finding | Cite |
|---|---|---|
| Edition | "Version 5.1", (c) 1989, print code `MNUSIWP51X1C--6/92` with a second line of spaced digits under it (a printer's/job code); corporate address, telephone, telex, fax centred at foot | PDF 3 and again PDF 1048 [READ] |
| Binding | Ring binder in slipcase (from prior survey, CCH record); scan shows a dark binding edge at the left of the cover | PDF 1 [READ/INF] |
| Trim | About 8.0-8.2 x 9.1 in. Scan page boxes vary 548-587 pt wide (per-page crop); the 150-dpi render of PDF 52 is 8.16 x 9.14 in with the page edge inside it. Body line pitch measures 11.5 pt at scan scale; if the true leading is 12 pt the trim rises to about 8.5 x 9.5 in. A ruler on a physical copy settles it | [MEAS] |
| Columns | Two-column *hanging side-head* grid: side-head column from the text-block left edge (1.08-1.10 in from the scan's left edge) to about 2.5 in; body column from 2.57 in to 7.21 in (measure 4.64 in, about 28 pc) | Ref p. 37 (PDF 52), p. 31 (PDF 48) [MEAS] |
| Full text-block width | 6.14 in (rules span side-head column + body) | PDF 48 blue rule 1.10-7.24 in [MEAS] |
| Top of text | 0.91 in below scan top (continuation pages) | PDF 52 [MEAS] |
| Running head | None | throughout [READ] |
| Running foot | At the outer corner only. Verso: folio then section/entry title; recto: title then folio. Title in small capitals of a light condensed sans, folio in bold condensed sans; both blue in the Reference (black in the Workbook). Foot baseline 8.71 in from scan top | Ref p. iii (PDF 6), pp. 31-39 [READ][MEAS] |
| What the foot names | Getting Started: the sub-section ("THE BASICS", "GETTING HELP", "A BRIEF LESSON"). Reference: the **entry in force at the bottom of the page** (a page where a new entry starts mid-page carries the new entry's name: p. 43 "BACKUP, TIMED", p. 560 "REVEAL CODES"). Appendix: the full appendix title ("APPENDIX K: MACROS AND MERGE, PROGRAMMING COMMANDS") | Ref pp. 3-21, 34, 43, 560, 791 [READ] |
| Folios | Arabic, continuous across Getting Started, Reference, Appendix, Glossary/Index (1-1029); roman for front matter (iii-xiii). Dividers and blanks are counted but unprinted | TOC PDF 4-5 [READ] |
| Recto starts | Every front-matter section, every section opener after a divider, and every appendix (A 715, B 721 ... T 963, all odd) starts recto. **Feature entries do not**: they run on | TOC; Ref p. 34, 36, 43 [READ] |
| Blank versos | Blank (unprinted) pages after each front-matter section and behind each divider | PDF 7, 11, 17, 19, 47, 729, 731, 1003 [READ] |

### 2.1 Section dividers (the WP "thumb index")

| Aspect | Finding | Cite |
|---|---|---|
| Form | A printed divider *leaf* (same stock as text, not a cut-tab card) per section: Getting Started, Reference, Appendix, Glossary/Index | PDF 18, 46, 728, 1002 [READ] |
| Tab | A solid blue vertical bar at the fore-edge, about 0.35 in wide and running from about 0.6 in below the top to about 0.9 in above the foot (roughly 80 % of page height); section name reversed out in white bold condensed sans, centred on the bar and rotated 90 deg clockwise (reads top to bottom) | PDF 18, 46, 728, 1002 [READ/MEAS] |
| Stepping | **None.** All four bars sit at the same position; the reader finds a section by the coloured edge, not by a stepped tab. There is no alphabet thumb index for the 202 entries | same [READ] |
| Field | A 6 x 9 grid of small upward triangles printed very pale (screen tint), with the middle row of six printed solid blue and turned to point right (toward the tab) | same [READ] |
| Verso | Blank | PDF 19, 47, 729, 1003 [READ] |

### 2.2 Typography (by eye)

| Element | Face [INF] | Size/leading [MEAS] | Colour | Cite |
|---|---|---|---|---|
| Body | Times Roman (PostScript Times) | about 10/11.5-12 pt (line pitch 24 px at 150 dpi) | black | Ref p. 37 |
| Section/entry title | Helvetica Condensed Black (or Bold) | cap height 0.13 in, so about 14 pt | black | Ref p. 31 |
| Rule over entry title | solid rule 0.04 in (about 3 pt), full text-block width, title set flush left about 0.1 in below it | | blue | Ref p. 31, 34, 36 |
| Side head (left column) | same condensed bold sans, about 10 pt, flush left, may break to two lines | | black | Ref p. 3, 32 |
| Hairline over each side-head section | 0.5 pt rule spanning side-head column + body | | black | Ref p. 3, 32 |
| Sub-head inside body (run-in above paragraph) | same condensed bold sans, body size, on its own line, no space below | | black | Ref p. 32 |
| Notes / cautions | Times Italic, about 1 pt smaller than body | | black | Ref p. 3, 32 |
| Keystroke names in text | Times Bold for the feature name, roman for the key in parentheses | | black | Ref p. 31 |
| Code examples in appendices | Times Roman, smaller, with visible-space middle dots and tildes (not monospace) | | black | Ref p. 798-799 |
| Screen reproductions | a small monospace (screen font) inside a pale tinted panel | | black on light blue tint | Ref p. 8, 20, 561 |
| Index | Times Roman, sub-entries indented 1 em, turnovers further | about 9/10 | black | Ref p. 991 |
| Index letter heads | condensed bold sans | about 11 pt | blue | Ref p. 991 |
| Cover title | wordmark in a blue old-style serif; "Reference" in an ITC Garamond-like roman, subtitle in its italic | | blue/black | PDF 1-2 |

### 2.3 Colour

Two colours, black + one blue (a medium process-like blue). Blue is used only for: entry and
section rules; the alphabet letter markers; divider bars and triangle fields; margin-callout
triangles; running feet; screen-panel tints; index letter heads (Ref p. 31, 41, 991; PDF 18).
All running text, steps and headings are black. **The Workbook is one colour (black)** with the
same layout: heavy black rules and black divider bars (Workbook p. 3; PDF 10).

---

## 3. Reference -- front matter and chapter inventory

| Section | Folios | Pages | Contents (structure only) |
|---|---|---|---|
| Cover / title page | PDF 1-2 | 2 | Cover with photograph; title page repeats cover type without the photo |
| Imprint (title verso) | PDF 3 | 1 | Trademark paragraph at top; version, copyright, "Printed in U.S.A.", print code, corporate address block centred at foot |
| Table of Contents | PDF 4-5 | 2 | Section names in the side-head column (bold condensed), entries with dot leaders in the body column, hairline between sections |
| WordPerfect Software | iii | 1 | Bulleted list of other platforms |
| International WordPerfect Offices | v-vii | 3 | Two-column address directory, country as condensed-bold head |
| International Distributor Offices | ix-xi | 3 | same |
| Developer's Toolkit | xiii | 1 | One-paragraph offer |
| **Getting Started** (divider PDF 18) | 1-21 | 21 | The Basics 3-11 (Registration, Installation, The Template, Start, Clean Screen, Keys to Know, Function Keys, Pull-Down Menus, Menu Options, Codes, Exit); Getting Help 12-17 (Help, Reference Manual, Workbook, On-Line Tutorial, Customer Support, Ordering Templates); A Brief Lesson 18-21 |
| **Reference** (divider PDF 46) | 23-711 | 689 | Reference Contents 25-29 (single column, leaders); 202 feature entries 31-~711 |
| **Appendix** (divider PDF 728) | 713-985 | 273 | Appendix Contents 713; A ASCII chart 715; B Graphic images 721; C Codes 725; D Equation palette 729; E Error messages 741; F Font programs 763; G Initial settings 765; H Keyboards 769; I-L Macros and Merge (comparison 777, expressions 783, programming commands 791-886 = 96 pp, variables 887); M Pull-down menus 893; N Startup options 897; O Troubleshooting 905; P Characters 913; Q Files 921; R Networking 931-960; S .PIF file 961; T Value tables 963 |
| **Glossary/Index** (divider PDF 1002) | 987-1029 | 43 | Glossary 987-990; Index 991-1029 (39 pp) |
| Back imprint | PDF 1048 | 1 | Repeats the imprint block |

---

## 4. Reference -- the feature-entry anatomy

Read in full: Advance (pp. 31-34), Append (34-36), Attributes (36-39), Backup, Original
(41-43), Backup, Timed (43-), Reveal Codes (560-562), Rewrite (563); appendix command entries
{Block Append}/{Block Copy}/{Block Move} (798-799).

### 4.1 Naming and order

| Rule | Example | Cite |
|---|---|---|
| Entry names are **inverted noun phrases**, general term first, qualifier after a comma, so related features file together | "Backup, Original", "Backup, Timed"; "Print Job, Cancel/Display/Rush"; "Macros, Define/Execute/Macro Editor ..." | Reference Contents pp. 25-29 |
| A bare parent entry often precedes its qualified children | "Macros" 369 then "Macros, Conversion Program" 370 ... | p. 26-27 |
| Strict letter-by-letter alphabetical order; 202 entries; average 3.4 pp, from half a page (Rewrite) to 14 pp (Graphics, Define a Box 235-245) | | pp. 25-29 [READ/count] |
| First entry of each letter carries a large blue capital (the letter) in the side-head column, level with the entry title | "A" beside Advance, "B" beside Backup, Original | p. 31, 41 |

### 4.2 Entry skeleton (in order)

| # | Block | Form | Cite |
|---|---|---|---|
| 1 | **Title** | 3-pt blue rule across full text width; title below it, flush left at the side-head column edge, condensed black sans about 14 pt; about 0.6 in of white below before body | p. 31, 34, 36 |
| 2 | **Lead** | One to four body paragraphs in the body column: what the feature does, when it is useful. Often an illustration (printed sample, screen panel or diagram) | p. 31, 36, 41 |
| 3 | **Procedure** | Introductory clause ending in a comma ("To use advance,"), then numbered steps. Numerals are bold condensed sans, hanging in the gutter left of the body column; step text in roman | p. 31, 37, 561 |
| 3a | Pull-down alternative | After any step that has a menu route: a small menu-bar icon, then one italic sentence of the form *Select X from the Y menu*, with the mnemonic letter in bold italic. Not numbered | p. 31, 37, 42; convention explained p. 10 |
| 3b | Alternatives inside a step | an italic "or" on its own line between two indented alternatives | p. 4, 11 |
| 3c | Interstitial notes | italic paragraph, slightly smaller, flush with the body, no label; strong form uses a bold-italic run-in "Important:" | p. 3, 42, 43 |
| 4 | **Notes** | Hairline across the full text width; the side head "Notes" in the left column; then a series of sub-topics, each a condensed-bold run-in head on its own line followed by body paragraphs. **Sub-topics are alphabetised** (Advance: Advance and Graphics Images, Advance and Page Breaks, Advance Options, Cursor Position, Filling in Forms, Returning..., Text Boxes) | p. 32-34, 35, 37-39, 42-43 |
| 5 | **See Also** | Last line of the entry: bold "See Also:" then entry names in roman separated by semicolons, alphabetical | p. 34, 36, 39, 43, 562, 563 |

Counts across the feature section: 169 "Notes" side heads and 185 "See Also" lines for 202
entries; the only side head used inside feature entries is "Notes" (text-layer census,
PDF 48-730). There is **no "Keystrokes" heading** in the 5.1 Reference: the keystrokes live
in the numbered procedure. (A "Keystrokes"/"Options" side head appears only in a few
appendix/networking passages.)

### 4.3 Illustrations inside entries

| Type | Treatment | Cite |
|---|---|---|
| Screen reproduction | Unframed pale-blue tinted rectangle, about 4 in wide, containing the 80 x 25 screen in a small monospace; reverse video shown as solid black bars | p. 8, 20, 561 |
| Callouts | Small blue outline triangles each carrying a letter (A, B, C...) placed on the screen at the point described; the same lettered triangles repeat as a **legend in the side-head column** with a label in small capitals of the light condensed sans | p. 8, 33, 41, 561 |
| Printed-output sample | Miniature page or document in a thin box or as overlapping sheets | p. 36, 41-42 |
| Diagram | Thin-line drawing with blue measurement callouts | p. 33-34 |

### 4.4 Keystroke and text notation (defined in Getting Started)

| Convention | Meaning | Cite |
|---|---|---|
| `F8` | press the key | p. 9 |
| `Shift-F8` (hyphen) | hold the first key, press the second | p. 9 |
| `Shift-F8,1` (comma) | complete the first keystroke, release, then press the next | p. 9 |
| **Format** (Shift-F8) | feature name in bold, key in roman in parentheses | p. 31 |
| select **O**ther (4) | menu option: mnemonic letter bold, option number in parentheses | p. 31 |
| type **a:install** | text the user types: bold | p. 3 |
| Enter | "enter X" means type X and press Enter | p. 6-7 |
| *Kerning* in *Reference* | cross-reference: italic entry name + "in *Reference*" even inside the Reference | p. 31 |
| [BOLD] ... [bold], [HRt], [AdvRgt:3"] | hidden codes in square brackets, on/off pairs by case | p. 32, 37 |
| {BELL}, {ASSIGN} | macro commands in braces, small caps-like roman | p. 791 |

Colour of keys on the template is part of the notation: Ref p. 6 gives the colour key (see
s.6).

### 4.5 Appendix-entry anatomy (macro programming commands, App. K)

Command name in braces, bold, with one or two pictograms (a loop arrow = available in
Macros, a branch mark = available in Merge) on the same line; one-paragraph description;
then indented sub-heads **Macros** and **Merge** (Times Bold, not sans) each with prose and an
example; examples in small Times with visible-space dots. Category lists open the appendix:
command name column + icon columns (p. 791-794, 798-799).

### 4.6 Glossary and index

| Item | Form | Cite |
|---|---|---|
| Glossary | ~45 terms, one per block: condensed bold term on its own line, definition in body roman below; acronyms expanded with bold initials | pp. 987-990 |
| Index | Three columns across the full text width (side-head column abolished); head "Index" in the section style; letter-group heads in blue condensed bold; entries Times roman; word space (no comma) between entry and first locator; locators separated by commas; page ranges with hyphen ("685-687"); "see" in italic for cross-references; codes and macro commands indexed in their bracket/brace form; sub-entries one level deep, occasionally a second | pp. 991-1029 |

---

## 5. Workbook -- anatomy

| Aspect | Finding | Cite |
|---|---|---|
| Edition | "Version 5.1", (c) 1989, print code `WKUSIWP51--11/89`, **ISBN 1-55692-476-3**, corporate address block | PDF 3, PDF 512 [READ] |
| Trim | Scan page box 566 x 666 pt (7.86 x 9.25 in) on all 511 pages; body pitch 25 px at 150 dpi (12 pt), consistent with the Reference grid | [MEAS] |
| Binding | Paper cover; not determinable from scan (perfect-bound per prior survey [MEM]) | -- |
| Grid | Identical to the Reference (hanging side heads, body column at 2.03 in from scan edge, condensed bold heads, Times body) but **one colour**: black rules, black divider bars, black triangles | pp. v-viii, 3 [READ] |
| Front matter | Imprint; TOC (iii-iv) with side-head section names; Introduction (v-viii) covering: before you start, LEARN files, tutorial, your computer, your printer (a special demonstration printer definition is used so screens match), default settings, how lessons are grouped and which must be done in sequence | pp. iii-viii |
| Sections | Fundamentals I: lessons 1-12 (pp. 3-107); Fundamentals II: 13-25 (109-291); Special Features: 26-36 (293-~473). Each section behind a black divider leaf (as Reference, PDF 10) | TOC p. iii-iv |
| Appendix | A Feature List (Alphabetical) 475-477: two-column table feature -> lesson numbers; B Feature List (By Lesson) 478-481: lesson title (bold sans) with its features listed beneath; Two Disk Drives 482 | pp. 475-482 |
| Index | Two columns (not three), same entry style | pp. 487-501 |
| Covers | Front: black serif title over two lines, sans subtitle, small "VERSION 5.1" top right, full-bleed still-life photograph of office documents lower half; inside title page repeats type in blue. Back: solid blue with the imprint reversed in white at the foot | PDF 1, 2, 512 |

### 5.1 Lesson anatomy

| # | Block | Form | Cite |
|---|---|---|---|
| 1 | Opener | Heavy rule (about 3 pt, black) across full width; "Lesson N: Title" in condensed black sans; lessons begin at the top of the **next page** (verso or recto) | p. 3, 6, 88 |
| 2 | Scenario | One or two body paragraphs: what document you will make and why; multi-part lessons say which later lessons finish the document | p. 3, 6 |
| 3 | Task sections | Hairline + side head naming the task as a gerund phrase ("Typing the ...", "Printing the ...", "Saving the ...") | p. 3-4, 6-10 |
| 4 | Steps | Numbered (restart at 1 in each task section); typed text in **bold Times, indented to the step text, at a larger size** for passages to be typed; mouse alternative after each step (menu icon + italic) | p. 3, 6-8 |
| 5 | Checkpoints | "Compare your screen with..." then an unframed tinted screen panel; lettered triangle callouts with margin legend | p. 3, 7, 9, 89 |
| 6 | Asides | Italic smaller paragraphs (recovery hints, printer differences) | p. 3, 7 |
| 7 | Close | A plain sentence returning to a clear screen and pointing to the next lesson; **no summary box, no review questions, no objectives list** | p. 5, 11 |
| -- | Exception | "Special Techniques" lessons (12, 25) are anthologies of independent mini-exercises, one per side head, to be dipped into | p. 88-90 |
| -- | Running document | Lessons 2, 4, 6 build one letter (first, second, final draft); the doc names lesson groups to be done together (2-6, 8-11, 13-14, 16-17, 19-22) | p. viii |

---

## 6. Function-key template (keyboard strip)

| Aspect | Finding | Cite |
|---|---|---|
| Form | One long strip with rounded ends, for the enhanced (101-key) keyboard's single top row F1-F12; cream stock | strip scan [READ] |
| Proportion | 3492 x 411 px = 8.5 : 1. Physical size not recorded in the scan; at an assumed 200 dpi it would be 17.5 x 2.05 in [INF] | [MEAS/INF] |
| Edition on the scan | UK printing: code `TMUKIWP51XID--3/90`, "(c) WordPerfect Corp. 1990", a three-group number `74210-74210-74210`; UK spelling "Centre" on Shift-F6 | strip, left panel [READ] |
| Zones, left to right | (1) Info panel, about 15 % of length: product name, "for IBM Personal Computers", then a 9-line two-column quick list of non-function-key commands (command in black, key in red or green), codes at foot. (2) F1-F4 cells. (3) A narrow legend cell. (4) F5-F8. (5) Legend cell. (6) F9-F12 | strip [READ] |
| Cell structure | Each key cell has five centred lines, top to bottom: **Ctrl** command (red), **Alt** command (blue), **Shift** command (green), unmodified command (black), key name in italic (F1...) | strip [READ] |
| Legend cells | Between the groups of four: the words *Ctrl*, *Alt*, *Shift* in italic, each in its row colour, aligned to the rows | strip [READ] |
| Colour code (authoritative) | Black = press key alone; Green = Shift; Blue = Alt; Red = Ctrl. Peel-off coloured keycap stickers were also supplied (Shift green, Alt blue, Ctrl red) | Ref p. 6 |
| Sparse cells | F11 and F12 carry only the unmodified command; empty rows are left blank, not ruled | strip |
| Glyphs | Small diamond markers flank some commands (paired-feature marks, e.g. the two Indent variants) | strip |
| Type | Helvetica (regular) for commands, Helvetica Oblique for key names and legends [INF]; product name in the serif wordmark | strip |
| Rules | Thin vertical rules between every cell; no horizontal rules | strip |
| Ordering | Ref p. 17 tells users whose keyboard does not fit to send the old template back with a sketch of their keyboard (an L-shaped/alternate template existed for other layouts [INF]) | Ref p. 17 |

---

## 7. Install cards (12/90 update package)

| Aspect | Finding | Cite |
|---|---|---|
| Format | Two separate cards, each printed both sides: PROGRAM INSTALLATION INSTRUCTIONS (code `DIUSIWP51X2A--12/90`) and PRINTER INSTALLATION INSTRUCTIONS (`DIUSIWP51X2B--12/90`) | installcard scans 142905/143241 |
| Size | About 5.0 x 4.25 in (600-dpi scan: card 3000 x 2550 px); folded once vertically (centre crease) to fit a disk envelope [MEAS/INF] | scans |
| Stock | Cream/ivory card, black ink only | scans |
| Front | Black band across the top about 0.5 in deep, title reversed in white condensed black sans capitals; body Times about 12 pt: a lead paragraph naming the disk to find (disk name in bold), "To install ... ," then numbered steps 1-6 with bold condensed sans numerals; "(See other side)" centred italic at foot; code bottom right in small sans with a spaced-digit second line | 142905, 143241 |
| Back | No band; continues the numbered steps (7-10 or 7-12) and ends with a one-line result statement; generous empty lower half | 143055, 143418 |
| Notation | Same as the Reference (bold menu mnemonic + number in parentheses; typed text bold; "or" in italic between alternatives) | all |

## 8. Disk labels (3.5 in)

| Aspect | Finding | Cite |
|---|---|---|
| Layout | Label wraps over the top edge. Front: blue band with the product name repeated three times in letterspaced reversed capitals; white (later speckled) field; disk title in a large serif ("Install/Learn/Utilities 1", "Program 1", "Speller/Thesaurus 1", "PTR Program/Graphics 1"); one or two lines of instruction in Helvetica with typed text in bold; "Version 5.1" bottom left (Helvetica, about 11 pt); copyright bottom right (about 6 pt) | `WP51_1992-03_disks1-4.jpg`, `WP51_1990-12_media_*` |
| Back flap | Blue band continues; copyright and a label code per disk: `L3USIWP51X3B`, `...X3C`, `...X3D`, `...X3E` (letter suffix = disk) | `WP51_1992-03_disks1-4_back.jpg` |
| Variants | One 12/90 label omits the disk number and the Version line (PTR Program/Graphics) -- labels were revised per printing | `WP51_1990-12_media_135206` |

---

## 9. Part numbers and print codes (pattern)

| Code | Item | Read as [INF] |
|---|---|---|
| `MNUSIWP51X1C--6/92` | Reference manual | MN = manual, US = market, I = IBM platform, WP51 = product, X1C = component/revision, 6/92 = print month |
| `WKUSIWP51--11/89` + ISBN | Workbook | WK = workbook |
| `DIUSIWP51X2A--12/90`, `...X2B` | Install cards | DI = disk insert (card) |
| `TMUKIWP51XID--3/90` | Template (UK) | TM = template; UK market |
| `L3USIWP51X3B`...`X3E` | 3.5-in disk labels | L3 = 3.5-in label |

A spaced second line of digits ("74 -- 2 0--7 0 S", "74 - 10- 4 0") sits under the print code on
manuals and cards (PDF 3; card scans): a production/job reference, unexplained in the scans.
**Mirror:** one family code grammar `TYPE + MARKET + PLATFORM + PRODUCT + REV -- M/YY` across
every printed piece, Initech-specific and newly invented.

---

## 10. Design facts to mirror (summary)

1. Hanging side-head grid: about one third of the text block is a left column for side heads,
   callout legends and alphabet letters; body in the right two thirds.
2. Entry = blue 3-pt rule + condensed-black title; lead; numbered procedure with pull-down
   alternative after each step; hairline + "Notes" with **alphabetised** sub-topics; closing
   "See Also:" line.
3. Inverted, comma-qualified entry names keep families together in a single A-Z run.
4. Running foot only, outer corner, names the entry in force at the foot of the page.
5. Two colours in the Reference (black + blue, blue never on text), one colour in the Workbook.
6. Undifferentiated full-height blue bleed bar on four divider leaves; no stepped tabs.
7. Keystroke grammar: hyphen = hold, comma = then; bold feature name + key in parentheses;
   bold mnemonic + option number.
8. Template: five-line cells (Ctrl red, Alt blue, Shift green, plain black, key italic), groups
   of four keys, legend cells between groups, quick-list panel at left.

## 11. Not determined from the scans

- True trim of either book (scan scale uncertain; see s.2 and s.5), paper weight, ring
  diameter, slipcase and paper sleeve design, spine and binder-spine insert.
- Exact blue (Pantone/process mix); exact faces and sizes (identified by eye).
- Template physical size and whether an L-shaped template for 84-key keyboards was in the
  same box.
- Box, registration card, license envelope, keycap sticker sheet (no scans held).
- What the spaced digit line under each print code encodes.

## 12. Slips the books carried (PUB-REF-0001 VO-30 evidence)

Read as a page image on 2026-10-17 for the house register of tolerated inconsistencies (bead
`initech-ipu5.16`, worklog WL-0150, `spec/pubs_inconsistencies.json`).

| Book | Page | What the page prints | Register entry |
|---|---|---|---|
| WordPerfect 5.1 Reference | PDF 70 [53] | The feature is "Block Protect" in its entry, its side head and the running foot (BLOCK PROTECT), and "a Block Protect code" in the body; the next paragraph reads "without block protect (Alt-F7,1,3,1,2), you can only block protect one column" -- one name, two capitalisations, one page | VO30-02 |
