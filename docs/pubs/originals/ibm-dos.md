<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals (A-1): IBM PC DOS and Microsoft MS-DOS Manuals, 1985-1989

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural model, not prose)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.1`, originals scour A)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-0001-A |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Companions | `apple-macintosh.md` (PUB-ORIG-0001-B), `ibm-packaging.md` (PUB-ORIG-0001-C) |
| Supersedes | nothing; deepens Sec 5 of `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` (PUB-RES-0001-A) |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/dos33/` (outside the repo; manifest `SOURCES.md`) |
| Copyright posture | Structure, measurements and conventions only. No prose is transcribed; the only quoted strings are fixed labels (a head word, a sub-heading name) needed to name a convention. |

---

## 0. Defined terms and provenance tags

| Term | Meaning in this annex |
|---|---|
| **PDF p. N** | Page N of the scan file, counted from 1. Every observation cites one. |
| **[folio]** | The printed page number on that page, given in brackets after the PDF page, e.g. PDF p. 116 [7-11]. |
| **Recto / verso** | Right-hand (odd) / left-hand (even) page of a spread. |
| **Chapter-page folio** | A folio of the form `chapter-page` (`7-11`, `A-4`), restarting at 1 in every chapter. |
| **Hanging head** | A head set at the left margin while the body is indented to a wider left edge, so the head "hangs" out into the margin. |
| **Thumb tab** | A solid black bar that bleeds off the fore-edge and carries a section name reversed out and rotated 90 degrees. |
| **Entry** | One reference item (a command, a function call, a message) laid out to a fixed sub-heading pattern. |
| **[READ]** | Seen on the cited page in this task. |
| **[MEAS]** | Measured from the scan (method given). Scans are cropped close to trim; tolerance about +/- 0.1 in on trim, +/- 1 pt on leading. |
| **[INF]** | Inferred. Typeface names are always [INF]: a scan carries no font data. |

Files read (all in `dos33/`): `6138519_DOS_3.10_Reference_Feb85.pdf` (546 pp.), `80X0945_DOS_3.30_Technical_Reference_Apr87.pdf` (546 pp.), `84X1712_DOS_4.0_Reference_Jul88.pdf` (333 pp.), `MS-DOS_3.3_Users_Guide_198707.pdf` (471 pp.), `ZR23-6845-01_PC_DOS_Operation_Training_Version_3.3_and_4.0_PC_and_PS2_198903.pdf` (77 pp.). About 150 pages were viewed as images; text layers (OCR) were used to map folios and contents.

**Gap stated up front.** The DOS **3.30** user-level *Reference* is not in the library (SOURCES.md, "Not found"). The 3.10 *Reference* (same series, same binder system) is the model for it, and the 3.30 *Technical Reference* shows the 1987 state of the same house style.

---

## 1. How the set divides the material (what each book is FOR)

| Book | Audience and task | Evidence |
|---|---|---|
| IBM DOS 3.10 **Application Setup Guide**, **DOS User's Guide**, **DOS Reference** (three books per DOS release) | Setup Guide: installing applications. User's Guide: the novice, keys and editing (cited from the Reference for "editing keys"). Reference: every command, CONFIG.SYS, directories, EDLIN, LINK, DEBUG, messages | PDF p. 16 [1-3] lists the three books; PDF p. 117 [7-12] refers the reader to the User's Guide for editing keys |
| IBM DOS 3.10 **Reference** (6138519) | "Read this first" users and experienced programmers; consulted, not read through. Diskettes ship in a plastic pocket at the back of this book | PDF p. 4-5 [iii-iv]; PDF p. 16 [1-3] |
| IBM DOS 3.30 **Technical Reference** (80X0945) | Experienced user, system programmer, application developer; assumes 8088 knowledge. Separately purchased package: the book, a quick reference card, one 5.25 in and one 3.5 in utilities diskette | PDF p. 5 [iv] |
| IBM **Using DOS 4.00** (84X1712; file name says "Reference") | New users first; task-by-example. Library of four: *Getting Started* (install + Shell), *Using DOS 4.00*, *Command Reference* (extra purchase), *Technical Reference and Application Programming* (extra purchase) | PDF p. 5 [iii], PDF p. 7 [v] |
| Microsoft **MS-DOS 3.3 User's Guide and User's Reference** (OEM print) | Two books bound as one volume. Guide: the newcomer, six short chapters. Reference: "resource manual" for lookup, not cover-to-cover reading. A third, *Programmer's Reference*, is cited but not included | PDF p. 10-12 [v-vii], PDF p. 82, 85 [vii, x] |
| IBM **PC DOS Operation Training** (ZR23-6845-01) | Internal self-study course 78099 for IBM customer engineers; sessions + lab projects + quiz | PDF p. 2-3 [ii-iii] |

**Pattern to mirror [INF].** IBM splits by *role of the reader*: a thin learning book, a fat lookup Reference in a binder, and a programmer's Technical Reference sold separately. Microsoft compresses the first two into one bound volume with two paginations. By 1988 IBM moves to a four-book "library" with a page that pictures each book and its binding (PDF p. 5 of 84X1712: coil-bound booklet, two perfect-bound books, one ring binder).

---

## 2. Physical format

| Aspect | DOS 3.10 Reference | DOS 3.30 Tech Ref | Using DOS 4.00 | MS-DOS 3.3 | DOS Training |
|---|---|---|---|---|---|
| Trim | 5.5 x 8.5 in (half letter) [MEAS: 2200 x 3392 px at 400 ppi, PDF p. 7] | 5.5 x 8.5 in [MEAS: 2226 x 3409 px at 400 ppi] | 5.5 x 8.5 in [MEAS: 2200 x 3392 px at 400 ppi] | about 6.5 x 9.0 in [MEAS: 3921 x 5423 px at 600 ppi] | 8.5 x 11 in (US letter) [MEAS: 3434 x 4427 px at 400 ppi] |
| Binding | Three-hole punched, loose-leaf in a ring binder (holes visible on cover, PDF p. 1) [READ]; binder in slipcase (see `ibm-packaging.md`) | Same; holes visible PDF p. 1 [READ] | Soft cover, no punch holes PDF p. 1 [READ]; perfect-bound [INF from the library pictogram, PDF p. 5] | Perfect-bound soft cover [INF: no holes, single block, PDF p. 1] | Three-hole punched soft cover, letter size (PDF p. 1, p. 77) [READ] |
| Cover stock | Solid IBM blue, full bleed [READ PDF p. 1] | Same blue [READ PDF p. 1] | White, grey 8-bar IBM logo set on a diagonal [READ PDF p. 1] | Pale cyan field, navy panel with rounded corners [READ PDF p. 1] | Burgundy with a cream band and white horizontal stripes [READ PDF p. 1] |
| Interior colour | Black only [READ] | Black only [READ] | Black plus a grey screen tint on entry heads [READ PDF p. 46] | Black plus grey screen on rules and bars [READ PDF p. 14] | Black only [READ] |
| Pages | 546 PDF pp.: xii front + 10 chapters + App. A (110 pp.) + 16-page index + 3 Reader's Comment Forms | 546 PDF pp.: xiv + two Sections, 13 chapters, App. A, 18-page index | 333 PDF pp.: continuous folios 1-320 | 471 PDF pp.: Guide (vii + 62) + Reference (x + 386) | 77 PDF pp.: viii + 4 sessions + App. A-F + forms |

---

## 3. Cover and title-page layout (IBM 1985-87 manual system)

Measured on DOS 3.10 Reference PDF p. 1 and 3.30 Technical Reference PDF p. 1 (identical grid) [READ]. Positions are fractions of trim height (top = 0) and width (left = 0).

| Zone | Content | Position | Type |
|---|---|---|---|
| Product line | "Disk Operating System Version 3.10" | top, y ~ 0.03; starts x ~ 0.13 | Bold grotesque sans, ~11 pt, white [INF Helvetica Bold] |
| Hairline rule | Thin white rule under the product line | y ~ 0.05, from x ~ 0.13 to the right edge (bleeds) | 0.5 pt |
| Book title | "Reference" / "Technical Reference" | y ~ 0.06-0.13, x ~ 0.13 | Large light serif, ~40 pt, white [INF ITC Garamond] |
| Family band | White bar carrying "Programming Family" in blue bold sans | y ~ 0.24-0.27; bleeds off the LEFT edge, stops at x ~ 0.86 | Bar ~0.25 in deep; text ~11 pt bold sans |
| Punch holes | Three, in the left margin | Standard 2.75 in centres (see PUB-RES-0001-A) | -- |
| Logo block | 8-bar IBM logo, white; under it "Personal / Computer / Software" stacked in three lines | lower left, logo top y ~ 0.80, x ~ 0.13 | Bold sans ~11 pt, flush left |
| Part number | Seven characters (`6138519`, `80X0945`) | lower right, on the baseline of "Software" | ~8 pt regular sans |

**Title page** (PDF p. 2 of both books) repeats the cover grid in black on white: product line, rule, title, a **black** family bar with white text, logo block lower left; no part number [READ].

**Back cover** (PDF p. 546 of both) is white with blue type: a colophon block top left (copyright line, "All rights reserved", corporate name over three lines, P.O. box and Boca Raton address, "Printed in the United States of America", then the part number on its own line), and a blue 8-bar logo with registered mark lower left [READ]. Box numbers differ by year (1328-S in 1985, 1328-W in 1987) [READ PDF p. 546 of each].

**DOS 4.00 change (1988).** White cover, very large mid-grey 8-bar logo rotated about 25 degrees, the title "Using Disk Operating System Version 4.00" in light sans running along the same angle top right of the logo, and "Programming Family" small on the same angle lower centre (PDF p. 1, 3) [READ]. The IBM Program License Agreement is printed on the inside front cover (headed by corporate name and Armonk address, a bold all-caps opening warning, two text columns, "Continued on inside back cover") with the book part number at the foot (PDF p. 2) [READ]. The back cover carries logo + colophon side by side at the bottom left and a different part number (`15F1371`) from the title page (`84X1712`) (PDF p. 333) [READ]; the second is probably the package or order number [INF].

---

## 4. Front-matter sequence and legal boilerplate

### 4.1 IBM order (3.10 Reference PDF p. 1-13; 3.30 Tech Ref PDF p. 1-15)

| # | Page | Content (structure only) |
|---|---|---|
| 1 | Cover | Sec 3 |
| 2 | Title page (recto) | Sec 3 |
| 3 | Edition notice page (verso of title) | Bold line "First Edition (Month Year)"; a bold run-in lead "The following paragraph does not apply to the United Kingdom or any country where ..." then the AS IS warranty disclaimer in capitals; a paragraph on technical inaccuracies; a paragraph on unannounced products; where to request copies; a bold run-in "applies only to the United States and Puerto Rico" paragraph announcing the Reader's Comment Form with the mailing address; the copyright line with year range (PDF p. 3 of 3.10) [READ]. 3.30 drops the comment-form paragraph from this page (PDF p. 3) [READ]. 4.00 adds an all-rights-reserved reproduction sentence after the copyright line (PDF p. 4) [READ]. |
| 4 | "About This Book" (iii onward) | Fixed sub-heads in this order: Read This First / First Time Users / Experienced Users / Terms Used / Using Applications with DOS / How This Book is Organized (3.10 PDF p. 4-7) [READ]. 3.30: version-applicability note; How This Book is Organized; "What comes in the ... package?" item list (PDF p. 4-5) [READ]. |
| 5 | Contents (vii-xii) | Sec 6 |
| 6 | Blank verso where needed | e.g. 3.30 PDF p. 15 [xiv] |

Roman folios bottom outside, bold (PDF p. 4 [iii]) [READ].

### 4.2 IBM edition notice pattern (to mirror)

`<Ordinal> Edition (<Month> <Year>)` -> country-limited warranty disclaimer -> errors/changes paragraph -> unannounced-products paragraph -> ordering paragraph -> comment-form paragraph (US/PR only) -> `(C) Copyright International Business Machines Corporation <first year>, <this year>`. Revisions are signalled only by the ordinal; no printer's key, no print run code [READ PDF p. 3 of 3.10, 3.30, 4.00 and ZR23 PDF p. 2].

### 4.3 Microsoft order (MS-DOS 3.3, PDF p. 1-13 and p. 76-85)

| # | Page | Content |
|---|---|---|
| 1 | Cover | Navy panel, rounded corners, inset on pale cyan; top strip "User's Guide and User's Reference"; "MICROSOFT / MS-DOS" in wide extended sans caps, "Operating System / Version 3.3" smaller below, "Microsoft Corporation" at the panel foot (PDF p. 1) [READ] |
| 2 | Half-title/title | Black bar across the top; "Microsoft MS-DOS / User's Guide" large bold sans, flush left; "Operating System / Version 3.3" lower right under a short grey bar; OEM printer credit and an OEM P/N lower left (PDF p. 2) [READ] |
| 3 | Copyright page (ii) | Subject-to-change sentence, license/copying paragraph, copyright line, "Simultaneously published in the United States and Canada", trademark lines, then **Document Number** in the form `<9 digits>-<version 330>-R<revision>-<MMYY>` (PDF p. 3; Reference: PDF p. 77) [READ] |
| 4 | Program License Agreement | Three pages, run-in caps heads (TERM, LIMITATIONS OF REMEDIES ...) (PDF p. 4-6) [READ] |
| 5 | Contents (iii-iv) | Sec 6 |
| 6 | Welcome / Introduction (v-x) | Fixed sub-heads: Before You Begin / Notational Conventions / How to Use This Guide (Guide, PDF p. 10-12); What is MS-DOS? / MS-DOS Version 3.3 / Notational Conventions / How to Use This Manual / Other manuals that may help you / What comes next? (Reference, PDF p. 82-85) [READ] |

---

## 5. Page grid and running furniture

### 5.1 IBM half-size reference (3.10 Reference; 3.30 Tech Ref identical in principle)

| Element | Specification | Evidence |
|---|---|---|
| Head margin / position | Level-1 heads and the hanging labels sit at x ~ 0.5-0.6 in from the left trim edge on both rectos and versos (no mirrored gutter is visible in the scans) | [MEAS PDF p. 117, 119, 100 dpi column profile] |
| Body left edge | ~1.35-1.5 in from the left trim (indent ~0.8 in from the head margin) | [MEAS PDF p. 117: body 1.49-4.76 in] |
| Text measure | ~3.3 in (about 20 pica), single column | [MEAS PDF p. 116-117] |
| Running head | **None.** Except: a reference entry that runs on repeats its title at the top of each continuation page, flush to the outside edge (Sec 8.1) | [READ PDF p. 120-121] |
| Folio | Bottom outside corner, bold serif, chapter-page form (`7-11`); verso flush left, recto flush right | [READ PDF p. 114-121] |
| Thumb tab | Recto only (and on chapter openers), black bar bleeding off the fore-edge ~0.35 in wide x ~1.2 in tall, with the section name in condensed sans caps reversed and rotated to read bottom-to-top. Tabs step down the fore-edge from chapter to chapter (Ch. 1 at top, Ch. 6 near the foot), then restart at the top with Ch. 7; App. A is low again | [READ PDF p. 14, 22, 32, 56, 82, 98, 106, 294, 330, 358, 416, montage] |
| Tab labels seen | INTRODUCTION, FILE SPEC, FIXED DISK, CONFIGURING, DIRECTORIES, STANDARD I/O, COMMANDS, LINE EDITOR, LINK, DEBUG, MESSAGES | same pages |
| Section dividers (3.30 only) | A recto with "Section 1" as a level-1 head top left and a full-height black tab bleeding down the whole fore-edge with SECTION 1 reversed; blank verso follows | [READ 3.30 PDF p. 16-17, 418-419] |
| Chapter openers | Always recto. "Chapter N. Title" bold serif, ~20 pt, at the top margin; a **chapter mini-contents** below it (3.10 labels it "Contents"; 3.30 omits the label) with dot leaders to chapter-page folios, level-1 entries bold; rest of page empty | [READ 3.10 PDF p. 14; 3.30 PDF p. 18] |
| Verso after opener | Either a page headed only "Notes:" (3.10) or a blank page (3.30) | [READ 3.10 PDF p. 15; 3.30 PDF p. 19] |
| Chapter start page | Content begins at page N-3 (opener N-1, notes/blank N-2) | [READ: every chapter TOC starts at x-3] |
| End-of-chapter fill | Odd leftovers become "Notes:" pages (3.30 PDF p. 173 [6-50], 368 [7-5]) | [READ] |

### 5.2 IBM Using DOS 4.00 (1988)

Single column, body flush left at the margin (no hanging indent), Helvetica-family sans throughout [INF], folios bottom outside in bold sans, continuous numbering, no thumb tabs, no running heads (PDF p. 13-14, 46-48) [READ]. Chapter opener: a heavy black rule (~4 pt) over the left two-thirds of the measure, then "Chapter N.  Title" bold sans ~18 pt, followed directly by text on the same page (PDF p. 13) [READ]. Level-1 heads inside a chapter carry a thin full-measure rule above (PDF p. 13 "Summary of Changes") [READ].

### 5.3 Microsoft MS-DOS 3.3

| Element | Specification | Evidence |
|---|---|---|
| Grid | Two columns: a narrow **outer** margin column (~1.6 in) for side notes and command-name tags, and a wide text column (~4.1 in). The margin column sits on the left of versos and the right of rectos | [READ PDF p. 14, 12, 109-110, 135-137] |
| Running head | Top of page, bold-italic sans ~9 pt. Verso: folio then book name ("2  MS-DOS User's Guide"); recto: chapter or appendix title then folio ("MS-DOS Commands 23") | [READ PDF p. 14, 108-111, 393-394] |
| Folio | In the running head, outside; continuous per book (Guide 1-62, Reference 1-386) | [READ] |
| Side notes ("sideheads") | Short bold sans phrases in the outer column under a dotted grey rule, aligned with the paragraph they gloss; the book itself names them as a finding aid | [READ PDF p. 108: sample page] |
| Cross-reference icon | An open-book pictogram in the margin column where text sends the reader to another manual | [READ PDF p. 12, 14] |
| Chapter opener | Recto. Grey screened bar (~0.2 in) across the top, "N  Title" bold sans ~24 pt under it, folio top outside, an "In this chapter you will learn" list with square bullets, and a pale halftone illustration filling the lower half (keyboard keys, a PC and paper) | [READ PDF p. 14, 106] |
| Level-1 head | Bold sans ~16 pt under a full-measure 1 pt rule | [READ PDF p. 12, 108] |
| Level-2 head | Bold sans ~12 pt, no rule | [READ PDF p. 14 "Program", "File"] |
| Body | Book serif ~10 pt on ~11-12 pt [MEAS: line pitch 10.6-11.0 pt, PDF p. 137 at 150 dpi]; [INF ITC Garamond-like] | |

### 5.4 IBM Training booklet (letter size)

Hairline rule across the top of every page; hanging bold sans side heads in a left column (~2.0 in) with body serif in a right column (~4.9 in); hairline rule above the foot; running foot = verso "folio  DOS Versions 3.3 and 4.0", recto "Session N. Title  folio"; folios bold, chapter-page form (PDF p. 3, 25-27) [READ]. Session opener: short heavy rule then "Session N.  Title" bold sans, then fixed sub-heads Introduction / Objectives (numbered) / topic heads (PDF p. 25) [READ]. This is the late-1980s IBM "BookMaster" look [INF].

---

## 6. Contents pages

| Book | Style | Evidence |
|---|---|---|
| DOS 3.10 Ref | Title "Contents"; chapter lines bold serif with bold folio; three indent levels; dot leaders; folios right-aligned; "Index ..... Index-1" last | [READ PDF p. 8-13] |
| DOS 3.30 TR | As 3.10, with "Section 1" / "Section 2" lines dividing the list; function calls listed by hex code then name | [READ PDF p. 6-14] |
| Using DOS 4.00 | Sans; spaced-dot leaders; continuous folios | [READ PDF p. 9-12] |
| MS-DOS 3.3 | No leaders: folio follows the entry after a word space; chapter numbers hang left of titles; sub-entries indented | [READ PDF p. 8-9, 78-81] |

---

## 7. Chapter inventories (titles and extents)

### 7.1 IBM DOS 3.10 Reference (6138519), PDF p. 8-13 and folio map

| Unit | Title | Folios | Pages |
|---|---|---|---|
| Ch. 1 | Introduction | 1-1 to 1-8 | 8 |
| Ch. 2 | File Specification | 2-1 to 2-10 | 10 |
| Ch. 3 | Preparing Your Fixed Disk | 3-1 to 3-24 | 24 |
| Ch. 4 | Configuring Your System | 4-1 to 4-26 | 26 |
| Ch. 5 | Using Tree-Structured Directories | 5-1 to 5-16 | 16 |
| Ch. 6 | Standard Input and Standard Output | 6-1 to 6-8 | 8 |
| Ch. 7 | DOS Commands (alphabetical, batch subcommands nested under "Batch File Commands") | 7-1 to 7-188 | 188 |
| Ch. 8 | The Line Editor (EDLIN) | 8-1 to 8-36 | 36 |
| Ch. 9 | The Linker (LINK) Program | 9-1 to 9-28 | 28 |
| Ch. 10 | DEBUG Program | 10-1 to 10-56 | 56 |
| App. A | Messages (preceded by an "Appendixes" opener with its own mini-contents, PDF p. 414) | A-1 to A-110 | 110 |
| Index | -- | Index-1 to Index-16 | 16 |
| Forms | Reader's Comment Form x 3 (form page + reply-mail page each) | unnumbered | 6 |

The commands chapter is 38% of the book; messages another 22% [MEAS from extents].

### 7.2 IBM DOS 3.30 Technical Reference (80X0945), PDF p. 6-14

| Unit | Title | Folios |
|---|---|---|
| Section 1 | (divider) | -- |
| Ch. 1 | DOS Technical Information | 1-1 to 1-10 |
| Ch. 2 | Installable Device Drivers | 2-1 to 2-42 |
| Ch. 3 | Using Extended Screen and Keyboard Control | 3-1 to 3-20 |
| Ch. 4 | File Management Notes | 4-1 to 4-18 |
| Ch. 5 | DOS Disk Allocation | 5-1 to 5-16 |
| Ch. 6 | DOS Interrupts and Function Calls | 6-1 to 6-240 |
| Ch. 7 | DOS Control Blocks and Work Areas | 7-1 to 7-20 |
| Ch. 8 | Executing Commands from within an Application | 8-1 to 8-4 |
| Ch. 9 | Fixed Disk Information | 9-1 to 9-18 |
| Ch. 10 | EXE File Structure and Loading | 10-1 to 10-6 |
| Ch. 11 | DOS Memory Management | 11-1 to 11-6 |
| Section 2 | (divider; utilities from the Technical Reference diskettes) | -- |
| Ch. 12 | The Linker (LINK) and EXE2BIN Programs | 12-1 to 12-30 |
| Ch. 13 | DEBUG Program | 13-1 to 13-58 |
| App. A | Using the Library Manager | A-1 to A-14 |
| Index | -- | X-1 to X-18 |

Every Section 1 chapter has the same second head, "Version Specific Information", straight after "Introduction" (PDF p. 6-12) [READ]: a structural device for one book covering several releases.

### 7.3 IBM Using DOS 4.00 (84X1712), PDF p. 9-12

Ch. 1 Introducing DOS 4.00 (1), Ch. 2 Managing Information (21), Ch. 3 Redirecting Input and Output (59), Ch. 4 Changing the System Configuration (67), Ch. 5 Creating and Changing Batch Files (111), Ch. 6 DOS Command Tables (127), Ch. 7 Using Code Page Switching (153), App. A Creating and Editing Files Using EDLIN (165), App. B Accented Characters (193), App. C System Code Pages (195), App. D Messages (201-307), Index (309-320) [READ].

### 7.4 Microsoft MS-DOS 3.3, PDF p. 8-9 and 78-81

| Book | Units |
|---|---|
| User's Guide | Welcome (v); 1 Learning About MS-DOS (1); 2 Learning About Disks, Files, and Directories (11); 3 Getting Started (19); 4 Using Commands (27); 5 Using Applications with MS-DOS (39); 6 Setting Up MS-DOS (45); Terms (51); Index (57) |
| User's Reference | Introduction (vii); 1 More About Files and Directories (1); 2 About Commands (15); 3 MS-DOS Commands (21); 4 Batch Processing (131); 5 MS-DOS Editing and Function Keys (153); 6 The Line Editor (Edlin) (161); 7 Link: A Linker (195); 8 Debug (223); App. A single-floppy users (257); App. B How to Configure Your System (259); App. C Installable Device Drivers (273); App. D Configuring Your Hard Disk (Fdisk) (285); App. E How to Use Code Pages (297); App. F MS-DOS Message Directory (307); Index (367) |

### 7.5 IBM PC DOS Operation Training (ZR23-6845-01), folio map

Preface (Getting Started, Course Description, Prerequisite, Duties and Tasks); Contents; Session 1 DOS 3.3 (1-1 to 1-6); Session 2 DOS 4.0 (2-1 to 2-10); Session 3 Using the Shell, with Lab Projects 1-7 (3-1 to 3-20); Session 4 Warranty/Support Structure (4-1 to 4-6); App. A Student Course Evaluation; B FIS Terminal Procedures; C Answers to Study Questions; D DOS Review; E Final Quiz; F Data Collection Sheets; tear-out student data sheets (form ZR31-1007); Reader's Comment Form (PDF p. 3-76) [READ].

---

## 8. Reference-entry anatomy (the part to mirror exactly)

### 8.1 IBM command entry (DOS 3.10 Reference, Ch. 7; PDF p. 119-121 [7-14 to 7-16])

| Order | Element | Form |
|---|---|---|
| 1 | Entry title | Command name in caps + parenthetical expansion + the word "Command", bold serif ~18 pt, broken over two lines (name line / "Command"), flush left on the first page |
| 2 | Rule | Full-measure 1 pt rule under the title, first page only |
| 3 | `Purpose:` | Bold label in the hanging (head) column; one-sentence answer in the body column |
| 4 | `Format:` | Syntax line in the body column (notation Sec 9) |
| 5 | `Type:` | The two words Internal and External side by side; three asterisks set under the one that applies |
| 6 | `Remarks:` | Parameter-by-parameter explanation; each parameter in italic leads its own paragraph; indented `Note:` paragraphs (bold label) sit further in |
| 7 | `Example:` | Bold hanging label; prose then example command lines on their own lines in the code face |
| 8 | Continuation pages | Title repeated at the top in the same two-line bold form, flush to the OUTSIDE edge (flush right on rectos, flush left on versos), no rule |

Entries start on a new page (ASSIGN ends mid-page 7-16, ATTRIB starts 7-17) [READ PDF p. 121; TOC PDF p. 10].

**Batch subcommands** are nested under one "Batch File Commands" entry with "... Subcommand" titles (PDF p. 10 TOC) [READ]. **EDLIN and DEBUG** commands use the same entry pattern, titled "Letter (Name) Command", in letter order (PDF p. 12-13 TOC) [READ]. Each command chapter opens with notation, then "Information Common to All ... Commands", then the entries (PDF p. 114-118) [READ].

### 8.2 IBM function-call entry (DOS 3.30 TR, Ch. 6; PDF p. 259-262 [6-136 to 6-139])

| Order | Element | Form |
|---|---|---|
| 1 | Title | Hex code line (`3FH`) over name line, bold serif; flush left on versos, flush right on rectos; full-measure rule below |
| 2 | `Purpose:` | Label on its own line, text indented below (differs from 3.10, where label and text share the line) |
| 3 | On-entry table | Boxed two-column ruled table: header "On Entry" / "Register Contents"; one row per register (AH = function number first) |
| 4 | On-return table | Same box, header "On Return" |
| 5 | `Remarks:` | Indented prose; cross-references by page number ("on page 6-38") |
| 6 | `Network Access Rights:` | Bold run-in lead, last |
| 7 | Continuation pages | Title repeated, no rule (PDF p. 261) |

### 8.3 IBM 1988 command entry (Using DOS 4.00; PDF p. 46-48 [34-36])

Title block: command name in bold caps over a parenthetical gerund phrase ("(Copying Files)"), inside a grey screened panel; flush left on versos, flush right on rectos. Sub-heads in bold sans: **Purpose** -> optional **Warning:** run-in -> a two-column task table with bold column heads **If You Want To:** / **Enter:** (task in prose left, command in the code face right) -> **Remark** or **Remarks** (bulleted) [READ]. Ch. 6 tables use a boxed two-column "Command / Purpose" table with page references in parentheses and a diskette pictogram under external commands (PDF p. 139-141 [127-129]) [READ].

### 8.4 Microsoft command entry (MS-DOS 3.3 Reference, Ch. 3; PDF p. 108 "Sample Command", PDF p. 135-137 [50-52])

| Order | Element | Form |
|---|---|---|
| 0 | Margin tag | Command name repeated in bold sans in the outer column, under a dotted grey rule, with a boxed **I** (internal) or **E** (external) symbol below it; a circled-and-struck NET symbol marks commands that fail on a network drive (key on PDF p. 107 [22]) |
| 1 | Title | Command name, initial capital only ("Copy"), bold sans ~16 pt, full-measure rule above |
| 2 | `Purpose:` | Bold sans sub-head, text below |
| 3 | `Syntax:` | Each form introduced by a short prose lead ("To copy files:") then the indented syntax line |
| 4 | `Comments:` | Prose; switch tables with bold column heads "Switch" / "Purpose" over a rule |
| 5 | `Notes:` | Square-bulleted items |
| 6 | `Examples:` | Prose lead then the command in the wide-spaced code face |
| 7 | Side notes | Outer-column glosses beside topics within the entry ("The Copy switches") |

Synonyms appear in the title in parentheses ("Del (Erase)") (PDF p. 141) [READ]. A three-page summary table (bold command name, one-line purpose) precedes the entries (PDF p. 111 [26]) [READ].

### 8.5 Message entries

| Book | Anatomy | Evidence |
|---|---|---|
| IBM 3.10 App. A | Intro + **Responses** list (hanging letters A / R / I with bold verb) -> "Device Error Messages" (format shown with angle-bracketed italic placeholders) -> alphabetical "Other Messages". Each entry: message text bold at the head margin; then indented **Explanation:** run-in paragraph; **Action:** run-in paragraph; optional bullets | PDF p. 416-419 [A-3 to A-6] |
| MS-DOS App. F | Response prompt and letter list first; then entries grouped under a large bold letter in the margin column; each entry: message in bold sans (variables in bold italic), the issuing command(s) in square brackets on the next line, a square bullet + **italic** cause sentence, then roman action text | PDF p. 392-394 [307-309] |
| Using DOS 4.00 App. D | Response list then "Extended and Parse Errors"; message head + "Action:" (from folio map) | PDF p. 213-300 |
| Training | Not a message book | -- |

---

## 9. Notation conventions

| Convention | IBM 3.10 (PDF p. 114 [7-9]) | IBM 4.00 (PDF p. 7 [v]) | Microsoft 3.3 (PDF p. 12 [vii], 110 [25]) |
|---|---|---|---|
| Optional item | Square brackets; do not type them | (in syntax diagrams, App. A "Reading the Command Diagram") | Square brackets |
| Keyword / literal | CAPS (any case may be typed) | CAPITAL letters | **bold**, lower case (`copy`, `dir`) |
| Variable | lowercase *italic* (*filename*) | lowercase *italic* | *italic* |
| Either/or | Vertical bar, shown in the code face (`ON|OFF`) | -- | Arguments chosen from **on**/**off** |
| Repeat | Ellipsis ... | -- | Ellipsis (...) |
| Punctuation | Type all shown; inside brackets is optional | -- | Separators: space, semicolon, equals, tab |
| Example lines | Light condensed monospaced sans, prompt included (`A>assign a=c`) [INF Letter Gothic-like] | Monospaced sans, no prompt (`COPY A:\FRIDAY C:\`) | Wide letter-spaced monospace [INF], no prompt (`copy memo.doc /a letter.doc`) |
| User-typed text in prose | **bold** (`DIR A:`) (PDF p. 120) | caps | **bold** |
| Key names | Spelled in text with initial caps and a hyphen or en rule: Ctrl-Break, Enter; named functions bold ("**Pause Screen**") (PDF p. 116) | `Ctrl+Break`, `F1`, plus-sign joins (index PDF p. 316) | SMALL CAPS for keys and acronyms (`CONTROL-C`, `ESC`, `F1`) |
| Placeholders in messages | `<type>`, `<device>` italic in angle brackets (PDF p. 417) | `xxx`, `ddd` | bold-italic word; `xxx` |
| Notes | Bold `Note:` run-in, paragraph indented further than body | Bold `Note:` hanging | Bold-italic sans `Note` run-in (PDF p. 111, 393) |
| Warnings | `*Important:` bold run-in (PDF p. 20 [1-7]) | Bold `Warning:` run-in (PDF p. 46) | -- |
| Drive terms | "Terms Used" section fixes diskette / fixed disk / disk, source / target, local / remote (PDF p. 6 [v]) | -- | "drive:", "path", "pathname" defined in an option table (PDF p. 109 [24]) |

**Screen reproductions.** IBM 1985-87 reference books carry almost no screen pictures; prompts and output are set inline in the code face (PDF p. 114-121) [READ]. The 1989 training booklet frames each screen in a rounded rectangle, numbers its fields with reversed square numerals joined by arrows, and explains them in a ruled three-column legend (number / field name / description) on the facing page (ZR23 PDF p. 26-27 [3-2, 3-3]) [READ]. Microsoft shows sessions in the code face, no frame (PDF p. 135-137) [READ].

---

## 10. Type (all [INF])

| Book | Body | Heads | Code | Measured |
|---|---|---|---|---|
| DOS 3.10 / 3.30 | Times-style serif, ~11 pt | Bold Times-style serif (chapter ~20 pt, level-1 ~18 pt, level-2 ~12 pt) | Light condensed monospaced sans | Leading 12 pt; paragraph space = one full line (24 pt baseline jump) [MEAS PDF p. 117, 200 dpi row profile] |
| Using DOS 4.00 | Helvetica-style sans ~10 pt | Bold sans | Monospaced sans | -- |
| MS-DOS 3.3 | Book serif (ITC Garamond-like) ~10 pt | Helvetica Bold-style | Wide letter-spaced monospace | Leading ~11 pt [MEAS PDF p. 137, 150 dpi] |
| Training | Times-style serif | Helvetica Bold-style hanging heads | -- | -- |

---

## 11. Index style

| Book | Style | Evidence |
|---|---|---|
| DOS 3.10 Ref | Two columns; "Index" as a level-1 head; first group "Special Characters"; letter heads single bold capitals with space above; entries roman, locators chapter-page separated by commas; turnover lines indented one em; sub-entries indented under a main entry; last page often mostly blank; folio `Index-N` | PDF p. 524-539 |
| DOS 3.30 TR | As 3.10 but letter heads are capitals inside small ruled boxes; a boxed "Numerics" group at the end; folio `X-N` | PDF p. 539 [X-18] |
| Using DOS 4.00 | Two columns, sans; "Index" under a short heavy rule; "Special Characters" first; bold sans letter heads; "(continued)" in italic on carried-over main entries; ranges with en rule | PDF p. 321-332 [309-320] |
| MS-DOS 3.3 | Two columns, serif; **no letter heads**, only an extra blank line between letters; heavy dependence on "purpose" / "syntax" sub-entries under each command; *See* and *See also* italic; "(continued)" italic | PDF p. 452-471 [367-386] |
| DisplayWrite 4 (IBM, for comparison) | Two columns sans, bold letter heads, "Numerics" group last | `packaging/` PDF p. 62-66 |

---

## 12. Back matter: Reader's Comment Form (IBM)

Two-page unit, repeated three times at the back of the 3.10 Reference (PDF p. 540-545) [READ]:

| Side | Layout |
|---|---|
| Form side | IBM logo top left; series name top right ("The Personal Computer / Programming Family"); bold "Reader's Comment Form"; bold book short title left, part number right; three short paragraphs (how comments are used, IBM's right to use them, not for technical questions); "Comments:" then open space |
| Mailer side | Postal barcode bars top centre; "NO POSTAGE NECESSARY ..." box top right with the horizontal indicia bars below it; boxed "BUSINESS REPLY MAIL" with permit line; "POSTAGE WILL BE PAID BY ADDRESSEE"; IBM address in caps; dotted "Fold here" line across the middle printed upside-down text below it; "Please do not staple" and "Tape" at the foot, upside-down |

The 1989 training booklet has one form, internal-mail version, with a "Possible topics for comment" line and reply-address rules (ZR23 PDF p. 75-76) [READ].

---

## 13. Part numbers, edition codes, legal objects (summary)

| Item | Form | Evidence |
|---|---|---|
| IBM manual part number | 7 characters: all-numeric 1985 (`6138519`), letter in third place from 1986 (`80X0945`, `84X1712`, `74X9319`, `15F1371`) | covers/backs [READ] |
| IBM training form number | `ZR23-6845-01` (prefix-4 digits-2 digits suffix = revision) and course number on the cover | ZR23 PDF p. 1 |
| IBM edition | "First edition (February 1985)" style only | Sec 4.2 |
| Microsoft document number | `410630001-330-R09-0787`: base number, version 330, revision R09, month-year 0787 | MS-DOS PDF p. 3, 77 |
| OEM credit | Printer and licence credit plus an OEM P/N on the title page | MS-DOS PDF p. 2 |
| Licence placement | IBM 1985-87: on the box back (see `ibm-packaging.md`); IBM 1988: inside covers of the book; Microsoft: three pages in the front matter | 84X1712 PDF p. 2; MS-DOS PDF p. 4-6 |

---

## 14. Rules a typesetter can take straight from these originals [INF]

1. Half-letter page, black only, Times-style 11/12, ~20-pica measure with heads hanging ~0.8 in left of the body; folio bottom outside in bold `chapter-page` form; no running heads.
2. Chapters open on a recto with a dot-leadered mini-contents; the verso behind it is a "Notes:" page.
3. Black stepped thumb tabs on rectos, one position per chapter, cycle restarting part-way through the book.
4. Each command entry starts a new page; title in two bold lines and a rule; Purpose / Format / Type / Remarks / Example as hanging labels; title repeated flush outside on continuation pages.
5. Messages: bold message, then indented **Explanation:** and **Action:** run-in paragraphs.
6. Blue punched cover: product line + hairline, big light serif title, white family bar bleeding left, logo block lower left, part number lower right. Back cover: blue colophon top left, logo lower left.
7. Three Reader's Comment Forms as the last leaves.

## 15. Slips the books carried (PUB-REF-0001 VO-30 evidence)

Read as page images (pdftoppm, 100-110 dpi) on 2026-10-17 for the house register of tolerated
inconsistencies (bead `initech-ipu5.16`, worklog WL-0150, `spec/pubs_inconsistencies.json`).
Each item was found in the text layer and then confirmed on the image [READ]; OCR artefacts
that the image did not confirm are left out.

| Book | Page | What the page prints | Register entry |
|---|---|---|---|
| ZR23-6845-01 Operation Training | PDF 12 [1-4] | "These codes ... are found in the DOS 3.3 Users' Guide." (apostrophe after the s, title not italic) | VO30-01 |
| ZR23-6845-01 | PDF 12 [1-4] | "hard file" (head *Installation on Hard File*), "install DOS on the hard disk", "Replace existing DOS files on the fixed disk" -- three words for one object on one page | VO30-11 |
| ZR23-6845-01 | PDF 12 [1-4] | "You have the choice of making a DOS partition of up to 32MB and several extended partitions, however only one will be a DOS partition." (comma splice) | VO30-07 |
| ZR23-6845-01 | PDF 12 [1-4] | Bulleted steps "Load the system ... Prepare the hard file using the Fdisk command ..." then "That is accomplished by selecting the appropriate prompt from the menus." (imperative to passive); "Select", "SELECT" and "Fdisk" in one section | VO30-05 |
| ZR23-6845-01 | PDF 29 [3-5] | "DISKCOPY the Master Lab Diskette(s) onto the work diskette(s)."; "Enter a "Y."" with Enter capitalised mid-sentence | VO30-02 |
| ZR23-6845-01 | PDF 7 [vii] | Contents: "Installation on Hard File" (Session 1) and "Installation on Fixed Disk" (Session 2) | VO30-11 |
| IBM DOS 3.10 Reference 6138519 | PDF 106 [7-1] | Chapter mini-contents: "Creating a Batch File", "Executing a Batch file", "The AUTOEXEC.BAT File", "Creating an AUTOEXEC.BAT file" | VO30-04 |
| IBM DOS 3.10 Reference | PDF 132-133 [7-27, 7-28] | the heads themselves: "Executing a Batch file", "Creating an AUTOEXEC.BAT file" | VO30-04 |
| IBM DOS 3.10 Reference | PDF 525 [Index-2] | "ATTRIB (Attribute) Command 7-17" and "ATTRIB command 7-17"; "BREAK Command", "CHDIR command"; "Batch commands" against the head "Batch File Commands" | VO30-09 |
| MS-DOS 3.3 User's Reference | PDF 179 [94] | *more*, See Also: "see Chapter 2, "Using MS-DOS Commands."" -- Chapter 2 is "About Commands" (contents PDF 100; cited correctly at PDF 202 [117]) | VO30-08 |
