<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals: Borland Turbo Pascal 6.0 (with 5.0/5.5 and brochures)

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural reference for typesetters and technical writers)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.3`, originals scour C)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-TP60 |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Sibling | `docs/pubs/originals/wordperfect-51.md` |
| Prior art | `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` s.4 (this annex deepens it) |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/turbopascal/` (outside the repo; manifest `SOURCES.md`) |

### Rules of use

1. **Structure, not prose.** No sentence of the originals is reproduced; quoted words are
   limited to labels needed to name a convention ("Declaration", "See also").
2. **No trade dress.** Do not copy the yellow-band cover, the version box, the letterspaced
   wordmark or the cover photography (`docs/packaging/MANUALS-AND-BOXES-RESEARCH.md`). Mirror
   the documentation *system* (four-book split, grid, entry anatomy, syntax diagrams).
3. **Provenance tags.** [READ] seen at the cited page. [MEAS] measured from a 100-150 dpi
   render (pixel profiles; the 6.0 scans are 400 ppi bitonal and their scale checks out against
   a 12-pt body leading, so absolute values are good to about +/-3 %). [INF] inferred. Faces
   are identified by eye, hence always [INF].
4. **Page citations** are printed folios; PDF offsets below.

| Book | File | PDF pp. | PDF = folio + | Part no. |
|---|---|---|---|---|
| TP 6.0 User's Guide (UG) | `Turbo_Pascal_Version_6.0_Users_Guide_1990.pdf` | 271 | 11 | 11MN-PAS02-60, BOR 1850 (back cover PDF 271 [READ]) |
| TP 6.0 Programmer's Guide (PG) | `Turbo_Pascal_Version_6.0_Programmers_Guide.pdf` | 382 | 11 | 11MN-PAS05-60, BOR 1851 [prior survey] |
| TP 6.0 Library Reference (LR) | `Turbo_Pascal_Version_6.0_Library_Ref_1990.pdf` | 184 | 7 | 11MN-PAS03-60, BOR 1852 [prior survey] |
| TP 6.0 Turbo Vision Guide (TV) | `Turbo_Pascal_Version_6.0_Turbo_Vision_1990.pdf` | 427 | 14 | 11MN-PAS04-60, BOR 1853 [prior survey] |
| TP 5.0 User's Guide | `Turbo_Pascal_Version_5.0_Users_Guide_1989.pdf` | 373 | 17 | 11MN-PAS01-50, BOR 0867 [READ, back cover] |
| TP 5.0 Reference Guide | `Turbo_Pascal_Version_5.0_Reference_Guide_1989.pdf` | 488 | 19 | 11MN-PAS02-50, BOR 0866 [READ] |
| TP 5.5 OOP Guide | `Turbo_Pascal_Version_5.5_Object-Oriented_Programming_Guide_1989.pdf` | 130 | -- | 11MN-PAS03-55, BOR 1309 [READ] |
| Turbo Assembler 2.0 Quick Reference (QRG) | `Turbo_Assembler_2.0_Quick_Reference_1990.pdf` | 159 | -- | -- |
| Turbo Languages brochure 1988 | `Turbo_Languages_Brochure_1988.pdf` | 12 | -- | MK CT-BOR011, BOR 1047A [READ] |
| TP 5.5 brochure 1989 | `Turbo_Pascal_5.5_Brochure_1989.pdf` | 2 | -- | MK CT-BOR012, BOR 1047B [READ] |
| 6.0 disk labels | `media/Borland_Turbo_Pascal_6.0_1990_*.jpg` | -- | -- | 11LB-PAS04, BOR 1855 [prior survey] |

---

## 1. What each book is for, and how the set divides the material

The UG introduction states that the four manuals serve four different purposes (UG p. 2); the
PG and LR introductions each restate the split and give a "read the other book if..." routing
list (PG p. 1, LR p. 1). Structure:

| Book | Reader | Job | Organisation |
|---|---|---|---|
| **User's Guide** | Everyone; beginners first | Install, learn the IDE, learn the language by example, then OOP, debugging, projects; plus the IDE, editor and command-line *reference* | Introduction + 9 chapters + index (UG pp. i-vii, 1-259) |
| **Programmer's Guide** | Experienced programmers | The language definition (grammar with syntax diagrams), standard units, implementation internals, assembler interfacing, directives, error messages | 5 Parts, 23 chapters, 2 appendices, index (PG pp. 1-370) |
| **Library Reference** | Everyone, at the keyboard | Every RTL procedure/function, A-Z, one entry each with an example | Introduction + 1 chapter + index (LR pp. 1-176) |
| **Turbo Vision Guide** | Application framework users | Tutorial (Part 1), concepts (Part 2), object and global reference A-Z (Part 3) | 3 Parts, 14 chapters, index (TV pp. 1-411) |

Two design principles are visible: (1) **tutorial and reference never share a chapter** --
learning matter is in UG Ch. 1-6 and TV Part 1-2; lookup matter is in UG Ch. 7-9, PG, LR, TV
Part 3; (2) **the RTL is documented twice at two depths** -- PG Part 2 groups routines by unit
and purpose in tables; LR gives one entry per routine. Error messages live only in PG App. A.

**Mirror for Turbo Initech:** User's Guide / Programmer's Guide (language + internals) /
Library Reference (A-Z) / Quick Reference, keeping the tutorial-vs-reference wall and the
two-depth RTL treatment.

---

## 2. Physical and page grid (6.0 set; all four books share it)

| Aspect | Finding | Cite |
|---|---|---|
| Trim | 6.75 x 9.0 in at scan scale (2700 x 3600 px at 400 ppi, tight crop); prior survey measured the 5.0 UG at 6.96 x 9.05 in. Read as a **7 x 9 in** book trimmed slightly by the scan [MEAS/INF] | UG p. 13 (PDF 24) |
| Binding | Perfect-bound paperback, card cover [MEM, prior survey]; QRG is **coil (spiral) bound**, visible on every page | QRG PDF 1-6 [READ] |
| Colour | Interior one colour (black) throughout | all [READ] |
| Text block | Full width 0.57 -> 6.37 in from scan left (5.8 in). Two zones: **margin column** 0.4 -> about 2.0 in; **body column** 2.1 -> 6.15 in (measure 4.05 in, about 24 pc) | UG p. 2, 13 [MEAS] |
| Body | about 10/12 (pitch 24.6 px at 150 dpi = 11.8 pt), ragged right | UG p. 13 [MEAS] |
| Top/foot | First text line 0.83 in from scan top; running-foot baseline 8.65 in | UG p. 13 [MEAS] |
| Running foot | Every text page. Recto: chapter label left ("Chapter 1, Learning the new IDE" / "Introduction" / "Index" / "Appendix A, Error messages") in italic of the heading sans; folio at right margin. Verso: folio at left; book title at right ("Turbo Pascal User's Guide"). Spans full text-block width | UG p. 1-13; PG p. 6, 321 |
| Running head | None in UG/PG. **LR and TV add a guide-word head**: entry name in bold sans at the outer top corner, naming the **first entry on the page** (including one continued from the previous page) | LR p. 4-8; TV p. 206-209 |
| Thumb index | LR and TV reference chapters only (s.6.3) | LR p. 5+ |
| Folios | Arabic continuous from the Introduction (p. 1); roman for front matter (i-viii). Part-title pages counted, folio printed on the recto | UG, PG p. 3 |
| Recto starts | Every chapter, part title, appendix and index starts recto (UG chapters 7, 29, 55, 73, 121, 157, 173, 219, 229, index 241; PG chapters all odd); the preceding verso is left blank with running foot only | UG/PG TOC; PG p. 4 |

### 2.1 Typography (by eye)

| Element | Face [INF] | Size [MEAS] | Placement | Cite |
|---|---|---|---|---|
| Body | Palatino (Roman/Italic/Bold) | 10/12 | body column | UG p. 2 |
| Display and heads | ITC Avant Garde Gothic Book (and Book Oblique for titles) | see hierarchy | -- | UG p. 2, 7 |
| Labels, table heads, reserved words | Helvetica Bold | body size or 1 pt less | -- | UG p. 5, 9; PG p. 7 |
| Margin notes | Avant Garde Book Oblique | about 8/10 | margin column, flush right against the body, ragged left, top-aligned with the paragraph they gloss | UG p. 1, 3, 8 |
| Code / typed text | a narrow monospace (Courier-class) | about 8.5-9 pt, i.e. smaller than body; reserved words in code set **bold** | indented 0.15-0.25 in into body | PG p. 86; LR p. 4 |
| Keycaps | a condensed sans oblique (Helvetica Narrow Oblique-like) | body size | inline: *Enter*, *Alt-X*, *F1* | UG p. 3, 5, 9 |
| Colophon claim | all faces produced by Sprint on a PostScript laser printer -- every face above is in the PostScript 35 set | UG p. 5 [READ] |

### 2.2 Heading hierarchy (UG/PG/LR/TV identical)

| Level | Form | Cite |
|---|---|---|
| Display opener | A word letterspaced across the full text width in Avant Garde Book capitals about 11 pt ("C H A P T E R", "P A R T", "A P P E N D I X", "I N T R O D U C T I O N", "C O N T E N T S", "T A B L E S", "F I G U R E S", "I N D E X"); 1-pt rule under it at about 2.1 in from top; for numbered openers a numeral (about 24 pt) flush right below the rule; then the title in Avant Garde Book Oblique about 24 pt, flush right; text begins about 3.5 in down | UG p. 7, i, 241; PG p. 3, 5, 321 |
| A-head | Avant Garde Book about 17 pt, sentence case, set from the **left edge of the margin column** (hangs over the margin), followed by a heavy rule (about 2.2 pt) across the full text width; text resumes in the body column | UG p. 2, 4, 5 [MEAS] |
| B-head | Avant Garde Book about 14 pt, **flush right inside the margin column**, may break to two lines; a thin rule (0.5 pt) across the body column at the head's cap line; text starts beside the head's last line | UG p. 4, 8, 11 |
| C-head | Avant Garde Book about 11 pt, flush right in the margin column, no rule; paragraph starts on the same line (side head) | UG p. 9, 13 |
| Run-in label | Helvetica Bold, then body text ("Letters--the English alphabet...") | PG p. 5 |
| Case | **Sentence case** for every head in 6.0 ("Learning the new IDE"); 5.0 used Title Case | UG p. 7 vs 5.0 UG p. 11 |

### 2.3 Lists, tables, figures, notes, icons

| Device | Form | Cite |
|---|---|---|
| Bullets | Solid square, flush with body; second level round bullet, indented | UG p. 1, 11 |
| Numbered steps | "1." serif numerals with full stop, hanging; continuation paragraphs indented to text | UG p. 4, 8 |
| Tables | Caption in the margin column, flush right, two lines: "Table 1.1" / title (Avant Garde Book about 9 pt). Table in body column (may extend right): thin top rule, header row in Helvetica Bold, thin rule, body rows serif with keys in keycap face, thin bottom rule; no vertical rules. Numbered per chapter; listed in front-matter TABLES | UG p. 9-11, vi; PG p. 7 |
| Figures | Same caption treatment ("Figure 1.1 / A typical window"); screen art redrawn as line art in monospace with box-drawing frames, unboxed on the page, callout boxes with leader arrows and the key term reversed (white on black) | UG p. 12, vii |
| Notes | No NOTE/CAUTION labels. Two devices: the margin note (s.2.1) and a small **pointing-arrow icon** in the label column for an important remark | LR p. 4; UG p. 1 |
| Shortcut notes | margin note with bold run-in "Shortcut:" | UG p. 13 |
| Icons | keyboard-hand icon = keyboard procedure; mouse icon = mouse procedure; placed in the margin column beside the paragraph | UG p. 5, 8, 13 |
| Menu paths | Menu and command names in roman with the hot letter in **bold**, separated by a vertical bar: File | Save, Options | Environment | Startup; abbreviated form gives only hot letters ("D | W | Add Watch") | UG p. 4, 8 |
| Typography key | A dedicated section "Typefaces used in these books" (two-column list: sample in the face, meaning) in the Introduction | UG p. 5 |

---

## 3. Front matter and back matter (each book)

| Item | Form | Cite |
|---|---|---|
| Cover | See s.8 | PDF 1 |
| Title page (i) | "Turbo Pascal(R)" Avant Garde Book Oblique about 24 pt; "Version 6.0" roman about 18 pt; 1-pt rule; book title about 24 pt; company address in small capitals at foot; all flush left on the body column's left edge | UG PDF 2 |
| Imprint (ii) | Copyright paragraph (1983, 1990) set in Avant Garde in the lower third; "PRINTED IN THE USA."; printer's key "10 9 8 7 6 5 4 3 2 1"; revision code "R2" alone at the far left margin level with the key | UG/PG/LR PDF 3 [READ] |
| Contents | Two columns; chapter lines in Helvetica Bold with folio; A- and B-level heads in Palatino with dot leaders, B indented, C indented again; Part lines underlined | UG pp. i-v; PG pp. i-vi |
| Tables, Figures lists | Separate letterspaced-head pages, two columns, numbered "1.1:" entries | UG pp. vi-vii; PG pp. vii-viii |
| Introduction | Unnumbered chapter: product scope and new features as bullets; the four manuals; installation; README; typefaces; how to contact the company | UG pp. 1-6 |
| Index | Two columns; entries Palatino; **locators in italic sans** (Avant Garde Oblique) -- a distinctive Borland trait; sub-entries indented 1 em, up to three levels; *See* / *See also* in italic; symbol and switch entries ("$A compiler directive", "/B option") sorted at their letter, pure symbols first; letter heads Avant Garde about 14 pt | UG pp. 241-259; PG p. 353+ |
| Back cover | Mirror of the front (version box moves to top-left), no photograph; company address, offices list and "Part# ... BOR nnnn" in condensed bold sans at the foot | UG PDF 271 |

---

## 4. Chapter inventories with extents

### 4.1 User's Guide (259 pp.)

| Unit | Pages | Notes |
|---|---|---|
| Introduction | 1-6 | manuals, install, README, typefaces, contact |
| 1 Learning the new IDE | 7-28 | components, menus, windows, status line, dialogs, first three programs |
| 2 Programming in Turbo Pascal | 29-54 | elements, data types, operators, I/O, control, procedures |
| 3 Turbo Pascal units | 55-72 | unit structure, standard units, writing units |
| 4 Object-oriented programming | 73-120 | tutorial |
| 5 Debugging Turbo Pascal programs | 121-156 | taxonomy of bugs, integrated debugger |
| 6 Project management | 157-172 | make/build, MAKE, conditional compilation |
| 7 The IDE reference | 173-218 | command-line options, then **one A-head per menu, one B-head per command**, in menu order (System, File, Edit, Search, Run, Compile, Debug, Options, Window, Help) with dialog-box figures |
| 8 The editor from A to Z | 219-228 | command tables |
| 9 The command-line compiler | 229-240 | options table, TPC.CFG |
| Index | 241-259 | |

### 4.2 Programmer's Guide (370 pp.)

| Part | Chapters (first page) |
|---|---|
| Introduction | 1 |
| Part 1 The Turbo Pascal standard (title p. 3) | 1 Tokens and constants 5; 2 Blocks, locality, and scope 15; 3 Types 21; 4 Variables 47; 5 Typed constants 57; 6 Expressions 65; 7 Statements 81; 8 Procedures and functions 95; 9 Programs and units 113 |
| Part 2 The standard libraries | 10 System 125; 11 Dos 141; 12 Graph 149; 13 Overlay 169; 14 Using the 8087 189; 15 Crt 199 |
| Part 3 Inside Turbo Pascal | 16 Memory issues 209; 17 Objects 225; 18 Control issues 241; 19 Input and output issues 251; 20 Automatic optimizations 255; 21 Compiler directives 261 |
| Part 4 Using Turbo Pascal with assembly language | 22 The inline assembler 281; 23 Linking assembler code 307 |
| Part 5 Appendixes | A Error messages 321 (compiler errors, then run-time: DOS, I/O, critical, fatal); B Reference materials 347 (ASCII, extended keys, scan codes) |
| Index | 353 |

### 4.3 Library Reference (176 pp.)

Introduction 1; Chapter 1 The run-time library 3-167 (sample entry p. 3, then about 220
entries: 134 procedures, 86 functions, A-Z, Abs p. 4 to Writeln p. 166); Index 169.

### 4.4 Turbo Vision Guide (411 pp.)

| Part | Chapters (first page) |
|---|---|
| Introduction | 1 |
| Part 1 Learning Turbo Vision | 1 Inheriting the wheel 7; 2 Writing Turbo Vision applications 23 |
| Part 2 Programming Turbo Vision | 3 The object hierarchy 65; 4 Views 81; 5 Event-driven programming 109; 6 Writing safe programs 131; 7 Collections 137; 8 Streams 151; 9 Resources 169; 10 Hints and tips 175 |
| Part 3 Turbo Vision Reference | 11 How to use the reference 185; 12 Unit cross reference 189; 13 Object reference 205; 14 Global reference 327 |
| Index | 387 |

---

## 5. Syntax diagram style (PG Part 1)

| Aspect | Finding | Cite |
|---|---|---|
| Kind | Railroad diagrams, read left to right along arrows; reading rules explained once in prose at first use | PG p. 5-6 |
| Lines | Thin (about 0.5 pt) orthogonal lines, **square corners**, small solid arrowheads at every entry into a box and at the exit; loops drawn as rectangular returns below or above the main line | PG p. 6, 8-10, 86 |
| Non-terminals | Lowercase names in Helvetica regular about 8-9 pt inside **rectangles** ("digit sequence", "statement") | PG p. 8-10 |
| Terminals | Symbols and single characters in **circles**; reserved words in **ovals/pills** in Helvetica Bold ("if", "then", "else") | PG p. 6, 86 |
| Ranges | Two circled terminals joined by an ellipsis ("A ... Z") | PG p. 6 |
| Diagram label | The production name in Helvetica regular at the left, level with the main line (or above it, with the line dropping from it, when long) | PG p. 6, 10, 86 |
| Placement | In the body column, indented about 0.1 in, white space above and below, **no caption, no number**; several diagrams may stack on a page | PG p. 8-10 |
| Accompanying text | Reserved words in running text in Helvetica Bold lowercase; identifiers in italic; code examples follow in the monospace | PG p. 7, 86 |

## 6. Reference-entry anatomy

### 6.1 Library Reference procedure/function entry

The book defines the pattern itself with a "Sample procedure" entry whose labels describe
their own contents (LR p. 3).

| # | Block | Form | Cite |
|---|---|---|---|
| 1 | Title | A-head style: "Name procedure" / "Name function" in Avant Garde Book about 17 pt from the margin-column edge, **unit name flush right on the same line** (Crt, Dos, Graph...; omitted for System), heavy rule below across full width. Variant entries qualify in parentheses ("Eof function (text files)") | LR p. 4-8 |
| 2 | Label column | Labels in **Helvetica Bold, flush right** in the margin column, top-aligned with their text: Function -> Declaration -> Result type (functions only) -> Remarks -> Restrictions (if any) -> See also -> Example | LR p. 3-8 |
| 3 | Function | One sentence | LR p. 4 |
| 4 | Declaration | Heading line in the monospace, reserved words bold (`var`) | LR p. 5 |
| 5 | Result type | Type name | LR p. 4 |
| 6 | Remarks | Prose paragraphs; parameters in italic; directives bold ({$I-}); error behaviour stated | LR p. 5 |
| 7 | Restrictions | One-line constraints ("Must be in graphics mode") | LR p. 6 |
| 8 | Important note | arrow icon in the label column, one sentence | LR p. 4 |
| 9 | See also | Comma-separated italic routine names | LR p. 4-7 |
| 10 | Example | Complete compilable program or fragment in the monospace, reserved words bold, right-aligned brace comments as annotations | LR p. 4-8 |

Entries **run on** (several per page), separated only by the next A-head. Running head = first
entry on the page. TP 5.0 had the same label set (Reference Guide Ch. 16, p. 224-225) but with
labels flush **left**, Palatino Bold heads between two rules, justified text: the 6.0 redesign
changed the dress, not the anatomy.

### 6.2 Other reference patterns in the set

| Pattern | Form | Cite |
|---|---|---|
| Compiler directive | A-head = directive name; labels Syntax / Default / Type (Global/Local) / Menu equivalent / Remarks; syntax in monospace | PG p. 262 |
| Error message | Run-in Helvetica Bold: number then message text ending in a full stop; explanation in body; numbered in ascending order; recovery advice as square-bullet list | PG p. 321-322 |
| IDE command | B-head per command under an A-head per menu; dialog-box figure; option descriptions | UG Ch. 7 pp. 173-218 |
| Turbo Vision object | A-head object name + unit flush right; **ancestry diagram** (monospace names in thin boxes joined by lines, the subject in a heavy box, immediate descendants below); prose; then B-head sections Fields / Methods / Palette; each field or method name in Helvetica Bold flush right in the margin column with its declaration in monospace on the same line, "Read only" / "Read/write" in Helvetica Bold flush right at the far edge, override advice ("Override: Sometimes") in italic in the margin under the method name; "See also:" line in roman with italic targets; palette shown as a numbered cell diagram | TV p. 205-209 |
| Quick-reference entry (QRG) | Rule above; entry name in Helvetica Bold flush left, mode tag (MASM / Ideal) in small bold capitals flush right on the same line; one or two sentences of body; dozens per page; Part title pages with a letterspaced "P A R T" line and a short tab-rule outdented toward the binding | QRG PDF 28-29 |

### 6.3 Thumb index (LR, TV reference)

| Aspect | Finding | Cite |
|---|---|---|
| Form | Solid black rectangle bleeding off the fore-edge of **rectos only**, with the letter(s) reversed in white Helvetica Bold | LR p. 5, 7; TV p. 207, 209 |
| Size | About 0.36 in visible width (plus bleed) x 0.48 in tall | LR p. 5 [MEAS] |
| Stepping | Tabs step down the page by about 0.475 in (one tab height, so adjacent tabs abut). Observed positions from the top of the scan: A 0.36 in (step 0), D 1.33 (step 2), G 2.27 (4), M 4.12 (8), R 6.02 (12), W-Z 7.91 (16). About 17 positions, letters grouped where entries are few ("W-Z") | LR p. 5, 23, 47, 87, 121, 163 [MEAS] |
| TV variant | Two-letter tabs (TA, TB, ...) because every object name starts with T | TV p. 207, 209 |
| Guide word | Bold-sans running head at the outer top corner on every reference page (verso and recto) | LR p. 4-8 |

---

## 7. Tutorial (lesson) anatomy -- User's Guide Ch. 1-2, TV Part 1

There are no "lessons" as such. Tutorial chapters are continuous prose chapters in the normal
grid: the chapter opener promises what the reader will have done by the end; A-heads name
milestones ("Creating your first program", "Stepping up: your second program", "Programming
pizzazz: your third program"); short complete programs in the monospace are typed, run and
then dissected; numbered steps for IDE actions with keyboard and mouse icons; margin notes for
asides and shortcuts; hot-key tables inline. No exercises, quizzes or summary boxes; chapters
end on a forward pointer (UG pp. 7-28, TOC pp. i-ii).

---

## 8. Cover, spine, disks, brochures

### 8.1 Manual covers (6.0)

| Element | Form | Cite |
|---|---|---|
| Field | Saturated yellow-orange band across the top 37 % of the cover; cream/white below | UG/PG PDF 1 |
| Product name | Black condensed geometric sans capitals (Futura Condensed-like), two lines, very large (cap height about 1.1 in), flush left near the left edge, (R) superscript | UG PDF 1 |
| Version box | Top right, about 1.0 in wide, bleeding off the top: upper square in a dark brown-red with the version "6.0" reversed white; lower black panel with the book name in small white capitals ("USER'S GUIDE", "PROGRAMMER'S GUIDE") | UG, PG PDF 1 |
| Image | Photographic panel (code text projected over architecture) at the lower right, about 3 x 4 in, bleeding off the foot; the same image on all four 6.0 books | UG, PG PDF 1 |
| Company | Letterspaced extra-bold wide capitals at lower left, about 1.2 in above the foot | UG PDF 1 |
| Back | Mirror image of the band, version box at top left, no photograph; condensed bold address/part-number lines at the foot | UG PDF 271 |
| Per-version box colour | 5.0: brown box with book name only (no number); 5.5: red upper square "5.5" + black "OOP GUIDE"; 6.0: dark brown "6.0" + black; TASM 2.0 QRG: navy "2.0" + black | 5.0 UG, 5.5 OOP, QRG PDF 1 |
| Earlier artwork | 5.0/5.5 used a pixelated painted portrait in the same panel position | 5.0 UG, 5.5 OOP PDF 1 |
| Spine | Not visible in any scan | -- |

### 8.2 Disk labels (6.0, 3.5 in)

Yellow label: dark-green vertical bar at the left edge, dark-green square at top right,
product name in dark green condensed capitals (two lines), letterspaced company name; a white
lower panel overprinted **per copy** by impact printer in a monospace: disk title in capitals
("INSTALL/TURBO VISION/COMPILER"), "VERSION 6.0", and a serial number of the form two letters +
twelve characters. Two disks in the 6.0 package (INSTALL/TURBO VISION/COMPILER; HELP/BGI/
UTILITIES) (`media/Borland_Turbo_Pascal_6.0_1990_Install.jpg`; prior survey).

### 8.3 Brochures (box-copy voice and feature-list layout)

| Aspect | Turbo Languages 1988 (12 pp.) | TP 5.5 1989 (2 pp.) |
|---|---|---|
| Size | 6.9 x 8.85 in (page box) | 5.47 x 8.45 in |
| Structure | Cover (same band system as manuals, slogan line under the band, pixel-art illustration); p. 1 drop-cap manifesto + three caps-headed benefit blocks; then **one spread per product** (Professional Series, Assembler/Debugger, C, Pascal, Prolog, Basic): huge product-name headline, body copy, screenshot(s) with a two-line caption, a feature bullet list, a comparison/benchmark table, footnoted benchmark machine and minimum system requirements; back cover band + corporate and overseas addresses + part number | p. 1: headline + coloured corner block ("NEW") + lead paragraph + screenshot + two caps-headed sections; p. 2: headline + screenshot + "FEATURES" bullet list + benchmark footnote + minimum requirements + trademark line + part number |
| Version flag | Coloured square at the page foot outer corner carrying the version ("5.0", "2.0", "1.0", "1.1") bleeding off; "NEW" tab bleeding off the top outer corner of rectos | coloured block top left ("NEW") |
| Type | Headlines in the condensed geometric sans capitals; body in a light condensed sans (about 10/14, ragged); subheads in condensed bold capitals; table heads bold | same |
| Lists | Hollow-square bullets; nested lists in brochures use solid square then hollow square | p. 3-10; 5.5 p. 2 |
| Tables | "FEATURE COMPARISON" / "... BENCHMARK" tables: row labels left, own product column bold ("Yes", "Zero K"), competitor column light; thin rules; footnote under the table naming the test machine | pp. 3-8 |
| Voice | Second person, exclamatory, speed numbers first (lines per minute), comparison against the market leader, "100% compatible" claims, short punchy subheads in capitals, every claim footnoted with the test machine | pp. 1-10 |
| Colour | Full colour; screenshots in their real CGA/EGA colours | all |

---

## 9. Part numbers, print codes, edition notices

| Item | Pattern [READ] | Reading [INF] |
|---|---|---|
| Manual | `11MN-PAS02-60` + `BOR 1850` | 11 = division/category; MN = manual; PAS = product; 02 = book number in the set; 60 = version 6.0; BOR nnnn = sequential corporate job number |
| Disk set | `11LB-PAS04` + `BOR 1855` | LB = label/disk set |
| Brochure | `MK CT-BOR011` + `BOR 1047A`, `...012` + `BOR 1047B` | MK = marketing; letter suffix = revision |
| Edition | "R2" (6.0 books), "R1" (5.5 OOP, QRG), "R3"/"R4" (5.0 books) at far left of the imprint + printer's key "10 9 8 7 6 5 4 3 2 1" (5.0 UG shows "10 9 8", i.e. a later printing) | reprint number = lowest digit remaining; R = revision of the text |
| Colophon | 5.0/5.5: a separate line or page "This manual was produced with Sprint ..."; 6.0: folded into the Typefaces section of the Introduction | 5.0 UG PDF 3; 5.5 OOP PDF 3; UG p. 5 |

## 10. Design facts to mirror (summary)

1. 7 x 9 in, one colour, perfect-bound; a margin column (about 1.6 in) carrying side heads,
   captions, margin notes, icons and entry labels, all flush right toward the body.
2. Letterspaced display word + rule + right-aligned numeral and oblique title for every
   chapter, part, appendix and front-matter opener; all start recto.
3. A-heads hang into the margin column over a heavy full-width rule; B/C heads sit flush right
   in the margin column; sentence case.
4. Library entry = title + unit flush right + labelled rows (Function, Declaration, Result
   type, Remarks, Restrictions, See also, Example) + complete example; entries run on; guide
   word at top; stepped black thumb tabs on rectos.
5. Railroad syntax diagrams: rectangles for non-terminals, circles for symbols, pills for
   reserved words, square-cornered lines, unnumbered.
6. Index locators in italic sans; Typefaces-used section defines the notation once.
7. Four books, each one job; tutorial and reference never mixed; RTL documented at two depths.

## 11. Not determined from the scans

- True trim and paper; cover stock and finish; spine layout (no spine visible anywhere).
- Box design and dimensions, registration card, No-Nonsense License Statement, RTL source order
  form (not held as scans).
- Exact ink colours (yellow-orange, brown, green); exact faces (identified by eye).
- Whether the LR thumb-tab letter grouping is A, B, C, D... one per step or grouped (only six
  positions were sampled); the full tab list needs a page-by-page pass.
- PG/LR/TV back-cover part numbers (taken from the prior survey; their back covers are not in
  these scans or were not checked).

## 12. Slips the books carried (PUB-REF-0001 VO-30 evidence)

Read as page images on 2026-10-17 for the house register of tolerated inconsistencies (bead
`initech-ipu5.16`, worklog WL-0150, `spec/pubs_inconsistencies.json`).

| Book | Page | What the page prints | Register entry |
|---|---|---|---|
| Turbo Vision Guide (1990) | PDF 201 [186] | "If you don't find the method listed for that object, check it's immediate ancestor object type." -- in the same paragraph "its relationships to its ancestors" | VO30-06 |
| Library Reference (1990) | PDF 8 [1] | The text layer reads "The Users Guide tells you"; the image prints "The User's Guide" -- an OCR artefact, not a slip, and not used | -- |
