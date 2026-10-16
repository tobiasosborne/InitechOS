<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals: Ashton-Tate dBASE III PLUS 1.1

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural reference for typesetters and technical writers)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.2`, originals scour B)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-DB3P |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Sibling | `docs/pubs/originals/lotus-123-r22.md` (same lane); `wordperfect-51.md`, `borland-turbo-pascal.md` (lane C) |
| Prior art | `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` s.1 (this annex deepens it; corrections listed in s.13) |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/dbase3plus/` (outside the repo; manifest `SOURCES.md`) |

### Rules of use

1. **Structure, not prose.** These notes record grids, hierarchies, notations and anatomy.
   No sentence of the originals is reproduced; quoted words are limited to the few needed to
   name a convention (a label such as "TIP" or "See Also").
2. **No trade dress.** Do not copy the Ashton-Tate wordmark, the diagonal-stripe mark, the
   chapter pictograms, or the specific blue as a brand colour. Mirror the *system* (hanging
   side-head grid, two-colour interior, pictogram thumb mark, section-prefixed folios), not
   the look.
3. **Provenance tags.** [READ] seen in the scan at the cited page. [MEAS] measured from the
   embedded page image by pixel row/column profile (method in s.2.1; absolute sizes carry
   about +/-4 %, relative proportions about 1 %). [INF] inferred. Typeface names are
   identification by eye, hence always [INF].
4. **Page citations.** Printed folio first (e.g. U5-26), then the scan's PDF page
   ("PDF 138"). The folio scheme is itself a design fact (s.4).

| Book (section) | File (under `dBASE III Plus 1.1 (1986) Manuals/`) | PDF pages | Folio to PDF |
|---|---|---|---|
| Learning dBASE III PLUS | `Learning dBase III Plus.pdf` | 244 | L1-1 = PDF 13; LI-1 (Introduction) = PDF 11 |
| Using dBASE III PLUS (+ master Index) | `Using dBase III Plus.pdf` | 596 | U1-1 = PDF 19; U5-n = PDF 112+n; U6-n = PDF 396+n; X-1 = PDF 547 |
| Programming with dBASE III PLUS, Networking, Runtime+, Applications Generator (+ duplicate Index) | `Programming with dBase III Plus.pdf` | 612 | P1-1 = PDF 19; N1-1 = PDF 303; R1-1 = PDF 491; A1-1 = PDF 529; X-1 = PDF 573 |
| Quick Reference Guide | `dBase III Quick Reference.pdf` | 36 | PDF only (no folios) |
| Customer Support Guide | `Customer Support Guide.pdf` | 22 | own arabic folios 1-13 |
| Technical Support pamphlet; flyers 1-3; "dBASE Users" insert | `Technical Support.pdf`, `Flyer1-3.pdf`, `dBase Users.pdf` | 8; 2/8/2; 3 | PDF only |
| 3.5-in media (1.1, 1987-02, evaluation copy) | `../dBASE3PLUS_1.1_1987-02_Media_Scans.zip` (`disk1.jpg`, `disks.jpg`) | 2 images | -- |

The floppy-sleeve JPEGs beside the zip are a non-original Dutch user-club sleeve (SOURCES.md)
and are not described here.

---

## 1. What the set is for, and how it divides the material

The set is organised as **titled sections inside two binders**, not as separate books. Each
section has its own title sheet, contents, folio letter and end-of-section "Notes" leaves; a
single master index (folio X) indexes every section and appears in both volumes
(Using UI-3, PDF 3 [READ]: index covers "Volumes One and Two", duplicate in Volume Two).

| Section | Folio letter | Job | Organisation | Evidence |
|---|---|---|---|---|
| Getting Started | (G) [INF] | Install, make boot disk, copy sample disk | Referenced, not scanned | Learning LI-1 PDF 11; Using U4-2 PDF 102 [READ refs] |
| **Learning** | L | Hands-on tutorial driven entirely through the menu shell (The Assistant), one running business case (a travel agency) | 10 chapters, each a lesson sequence; ends by stepping out to the dot prompt and a first program | Intro LI-1..2, PDF 11-12 |
| **Using** | U | "Encyclopedia": specs, task-oriented command use, then the A-Z command and function reference, error messages, appendices, glossary | 7 chapters + App. A-D + Glossary + Index | Intro UI-1..2, PDF 5-6 |
| **Programming** | P | From user to programmer: builds one worked application (a checkbook manager, files on the sample disk) module by module | Intro + 16 chapters | PI-1, PDF 13-18 |
| Networking | N | LAN edition: admin, security, network commands | Intro + 5 ch. + App. A-B + Glossary (NG) | PDF 291-488 |
| Runtime+ | R | Encoding/linking programs for distribution | 4 ch. + Appendix (RA) | PDF 489-524 |
| Applications Generator | A | Menu-driven program generator | Intro + 7 ch. | PDF 525-572 |
| Index | X | Master index, keyed by section letter | 2 columns | X-1, PDF 547 / PDF 573 |
| Quick Reference Guide | -- | Syntax-only crib of functions, commands, Runtime+ tools, network commands | Booklet | QR PDF 1-36 |

Division of labour (paraphrase): Learning = menus only, assumes no database knowledge;
Using = authoritative syntax and behaviour, the reference reader's home; Programming =
narrative method around one application, sends syntax questions back to Using; Quick
Reference = syntax only. Cross-references always name the section in italic
(e.g. "Chapter 6 of *Using ...*") [READ: U3-12 PDF 100; P1-1 PDF 19].

Binder allocation [INF, consistent with prior survey]: Volume One = Getting Started,
Learning, Using, Index (the Using scan ends with X and 16 Notes leaves, PDF 547-596);
Volume Two = Programming, Networking, Runtime+, Applications Generator, Index (PDF 573-612).

---

## 2. Physical format and page grid

| Item | Finding | Tag / cite |
|---|---|---|
| Binding | Ring-binder pages in **two binders** [READ: N1-1, PDF 303 states each of the two binders carries an index], slipcased | [INF for slipcase: prior survey s.1 citing NMAH + emsps; scans show no holes because every page image is cropped to the same box] |
| Interior page | 7.00 x 9.00 in (2100 x 2700 px at 300 ppi; every interior page image identical in size) | [MEAS; image size suggests normalised cropping, so true trim may be slightly larger: INF] |
| Section title sheet | 7.06 x 9.04 in (2117 x 2711 px) -- slightly larger than text pages | [MEAS: PDF 1 of each book] -- heavier sheet or divider [INF] |
| Text block (rule to foot) | 5.80 in wide; head rule at 1.62-1.66 in from top; running-foot baseline at 8.59-8.63 in | [MEAS U2-1 PDF 29, U1-1 PDF 19] |
| Inner (binding) margin | 0.81-0.82 in on both rectos and versos | [MEAS PDF 29 recto, PDF 30 verso] |
| Outer margin | 0.34-0.39 in (as cropped) | [MEAS; INF: true outer margin larger if pages were trimmed in scanning] |
| Columns | Two-column *hanging side-head* grid: side-head column 1.33 in, gutter about 0.0 (heads ragged), text column 4.45 in (~32 pc). The side-head column stays on the **left on both rectos and versos** (not mirrored) | [MEAS PDF 29 (side 0.82-2.15 in), PDF 30 (0.38-1.71 in)] |
| Full-measure exceptions | Chapter 7 error list, appendix tables, glossary terms (term in side column), index (2 cols), contents | [READ PDF 489, 511, 527, 547, 7] |
| Body line pitch | 0.152-0.154 in = 11 pt | [MEAS PDF 29, 138] |
| Index line pitch | 0.138 in = 10 pt | [MEAS X-1 PDF 547] |

### 2.1 Method

`pdfimages -png` extracts each page's native image; a script finds ink rows (grey < 128)
and blue pixels (B - R > 50) and reports band extents in inches at 300 ppi. Line pitch is the
mean distance between successive text bands.

### 2.2 Running heads and feet

| Element | Verso | Recto | Cite |
|---|---|---|---|
| Thumb pictogram (s.3) | top outer (left) corner, black | top outer (right) corner, black | PDF 20, 30, 82 |
| Running head | "CHAPTER n" in small bold blue sans caps, flush right (inner) | chapter title in small bold blue sans caps, flush left (inner) | PDF 20, 103, 83 |
| Head rule | blue hairline full text-block width under the head line | same | PDF 20, 30 |
| Running foot | folio left (outer), book title in bold black sans caps right (inner) | book title left (inner), folio right (outer) | PDF 19, 20 |
| Reference chapters (U5, U6) | dictionary head: **entry name** large bold black sans caps left + "CHAPTER 5" small blue right | "COMMANDS"/"FUNCTIONS" small blue left + **entry name** large bold black right; no pictogram | PDF 138-139, 316-317, 477 |
| Index pages | foot carries "dBASE III PLUS" not the section title | -- | PDF 547-549 |

Folios sit at the **outer** foot corner; the book title sits at the **inner** foot. No folio
appears on section title sheets, copyright pages or Notes leaves [READ PDF 1-2, 581].

---

## 3. Chapter pictograms (the thumb index)

Each chapter has its own pictogram in a rounded-corner square about 1.0 in on a side
[MEAS: band 0.55-1.60 in, PDF 19]. On the chapter-opening recto it is printed **blue** at
top right beside the chapter title; on every following page it is printed **black** (solid
square, reversed drawing) at the top outer corner -- functioning as a printed thumb tab
[READ PDF 19-20, 29-30, 41-42, 489-490].

| Chapter / matter | Motif (described, not to copy) | Cite |
|---|---|---|
| Overview; Quick Start; App. Generator ch. | product wordmark tilted in the square | PDF 19, L PDF 13, P PDF 529 |
| Using Commands | screen with dot prompt and cursor | PDF 29 |
| Entry forms | form boxes with a pen | L PDF 41 |
| Programming chapters | flowchart blocks | P PDF 19, 82, L PDF 237 |
| Testing/debugging | magnifier/sparkle motif | P PDF 248, 263 |
| Error messages | road "yield" sign | PDF 489 |
| Appendix | paper clip | PDF 511 |
| Glossary | page with letters | PDF 527 |
| Index | stepped tab dividers lettered A-E | PDF 547 |
| Networking | computer with attached peripheral | P PDF 303 |
| Runtime+ | padlock | P PDF 491 |

The reference chapters (U5 Commands, U6 Functions) drop the pictogram on interior pages and
use dictionary running heads instead (s.2.2).

---

## 4. Folio and numbering scheme

| Element | Format | Example | Cite |
|---|---|---|---|
| Body pages | section letter + chapter + hyphen + page | U5-26, L10-8, P14-4, N5-6 | PDF 138, 238, 248, 394 |
| Introduction | letter + "I" | UI-1, LI-1 | PDF 5, 11 |
| Appendix | letter + appendix letter | UA-1, NA-2, NB-2, RA-2 | PDF 511, 428, 436, 522 |
| Glossary | letter + "G" | UG-1, NG-2 | PDF 527, 484 |
| Master index | X-n | X-1 | PDF 547 |
| Contents | entries cite *chapter-page* without the section letter (1-1, 5-131) | -- | PDF 7 |
| Figures/tables | "Figure 5-3", "Table 2-1": chapter-number + sequence | -- | PDF 158, 30 |
| Index locators | section letter + page, ranges with spaced en rule; tags "(figure)", "(table)", "(tip)", "(note)" | L1-13 (figure), U5-28 - U5-31 | PDF 548-549 |

The master index opens with a boxed "Key to Index Page Numbers" listing the letters
(L, N, NA-ND, P, R, U) [READ X-2, PDF 548; A also used].

---

## 5. Typography (by eye) [INF]

| Role | Appearance | Size / leading | Cite |
|---|---|---|---|
| Body | Old-style book serif, ITC Garamond / Palatino class, roman with italic for book titles and new terms | ~10 / 11 pt [MEAS pitch] | PDF 19, 138 |
| Chapter number line ("Chapter 1") | Same serif, regular, upper/lower case | ~30 pt (cap height 0.30 in) [MEAS] | PDF 19 |
| Chapter title line | Bold humanist sans caps (Frutiger/Optima class), blue | ~14 pt (cap height 0.13 in) [MEAS] | PDF 19 |
| A-head (side head) | Bold humanist sans, blue, upper/lower, 2-3 short lines ragged in side column | ~12 pt | PDF 19, 29, 41 |
| B-head (side head) | Same sans, black, smaller | ~10 pt | PDF 30, 82, 263 |
| Entry labels (Syntax, Usage...) | Bold sans black in side column | ~12 pt | PDF 138 |
| Entry name in side column | Bold sans **blue** caps | ~12 pt | PDF 138, 316 |
| Dictionary running head (entry name) | Bold sans black caps | ~14 pt | PDF 138 |
| Running head / foot | Bold sans caps, ~7-8 pt; head blue, foot black | -- | PDF 20 |
| Key names in text (Ctrl-End, PgUp, F1) | **Grotesque** bold sans (Helvetica class), distinct from the humanist heads | body size | PDF 138 (crop) |
| Keys as glyphs | Enter shown as a hooked-arrow glyph; cursor keys as arrow glyphs | -- | PDF 29, 138 |
| Code / dot-prompt examples | Monospace on a tint panel (s.6); input lines prefixed by a dot; in Using, input set italic mono, output roman mono | ~8-9 pt | PDF 139, 477 |
| Program listings | Bold monospace (dot-matrix/"letter gothic" look), caps keywords | ~8 pt | P PDF 82, 248; L PDF 237 |
| Figure / table captions | Small grotesque sans, centred under the item: "Figure 5-1 The APPEND command" pattern | ~8 pt | PDF 139, 317 |
| Index | Serif ~8.5/10, two columns; letter-group heads bold blue sans single capital | -- | PDF 547-549 |
| Error-message headings | Bold serif message text; error number bold in square brackets flush right | body | PDF 489 |

---

## 6. Two-colour usage (black + one blue)

Scan values: solid blue about RGB(10-23, 109-137, 220-231); tint panels about
RGB(205-226, 249-255, 255), i.e. a light screen of the same blue [MEAS PDF 19, 138, 477;
scanner-uncalibrated]. Section title sheets use a different, dark navy for the stripe mark,
about RGB(1, 56, 93) [MEAS PDF 1].

| Where blue appears | Where it never appears | Cite |
|---|---|---|
| Chapter title line and the full-width rule under it | Body text | PDF 19 |
| Opening pictogram (solid blue square); interior pictograms are black | Running foot | PDF 19-20 |
| Running head text and head rule | Error messages | PDF 20, 489 |
| A-level side heads; entry names in side column | Glossary terms (black) | PDF 29, 138, 527 |
| Tip/NOTE boxes: no -- black ruled box | -- | PDF 29, 102-103 |
| **Tint panels** behind screen reproductions and code examples | Tables (plain ruled) | PDF 139, 316-317, 477 |
| Step numerals in tutorial procedures; bullets | Step text | L PDF 21, 41 |
| **What the user types** in tutorial steps (e.g. a filename followed by the Enter glyph), set in blue sans | -- | L PDF 42 |
| Index letter-group heads; Contents chapter-title lines | Index entries | PDF 547, 7 |
| "Notes" heading on blank Notes leaves | -- | PDF 581 |

---

## 7. Notation conventions

| Convention | Form | Cite |
|---|---|---|
| Syntax brackets | `< >` = supply an item (do not type brackets); `[ ]` = optional; `/` = choose one; `?` = pick-list query; `<expC> <expN> <expD> <condition>` typed expressions; `[...]` repeatable; `?`, `*` wildcards; `&&` comment | Symbols table U5-1, PDF 113 |
| Syntax set in | Body serif, keywords in caps, placeholders in angle brackets, optional clauses nested in square brackets | PDF 138, 316, 477 |
| Limits table | Item / Limitations two-column ruled table (filenames, extensions, field names, expression, index key) at head of the command chapter | U5-2, PDF 114 |
| TIP | Ruled box (thin black rule all round), label "TIP" bold caps on its own line, indented to text column | PDF 29, 102, 114 |
| NOTE | Same ruled box with "NOTE"; may hold a numbered list | PDF 103, L PDF 21 |
| Warnings | Declared as boxed like TIP/NOTE (N1-1, PDF 303); in Using only one "Warning" sub-head occurs and no distinct caution styling was seen | Using text tally |
| Screen reproductions | Actual 80x25 screen dumps in a screen font, printed on a blue tint panel, framed by the tint only (no bezel); callout labels with leader lines in the side column | PDF 139, 317 |
| Menu-bar / Assistant screens | Same; inverse-video highlight rendered as blue solid | L PDF 13, 22 |
| Examples at the dot prompt | Each input line begins with a dot; output lines follow unindented | PDF 477 |
| Sample data | One running sample database per section (travel agency in Learning/Using; checkbook in Programming) on the "Sample Programs and Utilities" disk | L PDF 11, P PDF 13-16 |
| Cross-reference | "See Also" line naming commands; section titles in italic | PDF 139 |
| Stated convention list | Networking N1-1 states the house rules explicitly as a bulleted "Conventions Used in This Manual" side-headed block: typed entries in a different type *and colour*; the Return key as a glyph; menu items and on-screen messages in bold; program listings in a different type in shaded areas; tips, notes and warnings in boxes; screen displays to show correct results | N1-1, PDF 303 |

---

## 8. Front matter and chapter inventory

### 8.1 Front matter (each section)

| Leaf | Content | Cite |
|---|---|---|
| Title sheet (recto) | Wordmark small caps at 0.50 in from top, hairline at 0.93 in (5.72 in long); section title in two lines of light serif caps and small caps (~40 pt caps, 0.43 in) at 1.52 and 2.09 in; hairline at 2.66 in; stripe mark 4.7 x 4.5 in in the lower right | [MEAS PDF 1] |
| Title verso | Product name centred bold; copyright years; unpublished-work notice; license summary; limited-warranty paragraph with the disclaimer sentences in **bold**; trademark-owners paragraph; print code "6/86" flush right at the foot | PDF 2 (all three books) |
| Introduction | Side-headed: what the section is, who should read it, chapter-by-chapter list (chapter labels as blue side heads in Using) | PDF 5-6, 11-12, 13-18 |
| Contents | "Contents" serif ~24 pt + section title blue sans caps + blue rule; chapter label black bold sans caps ("CHAPTER 1"), chapter title blue bold caps with dot leader to folio; two indent levels of serif entries with dot leaders; folios flush right | PDF 7-18, P 291 |

Order differs: Using puts Introduction before Contents (PDF 5 then 7); Learning and
Programming put Contents first (PDF 5 / 3, Introduction PDF 11 / 13) [READ].

### 8.2 Chapter inventory and extents (PDF pages; chapters open on rectos)

**Learning (L)**: Intro 2 pp; 1 Quick Start 28; 2 Entry Forms 36; 3 Updating Records 20;
4 Searching and Indexing 26; 5 Queries 16; 6 Reports and Labels 30; 7 Relating Data 16;
8 Import/Export 8; 9 dBASE III PLUS Commands 38; 10 Creating and Using Program Files 8;
Notes 6 (PDF 239-244).

**Using (U)**: Intro 2; Contents 12; 1 Overview 10; 2 Using Commands 60; 3 About Program
Files 12; 4 Configuration 12; **5 Commands 284 (PDF 113-396)**; **6 Functions 92
(397-488)**; 7 Error Messages 22 (489-510); App. A Cursor Control Keys 4; B Beginning/End of
File and Filter Conditions 4; C Structure of .DBF and .DBT files 4; D ASCII Code Tables 4;
Glossary 20 (527-546); Index 34 (547-580); Notes 16 (581-596).

**Programming (P)**: Contents 9; Intro 6; 1 Programming in a Nutshell 24; 2 Program
Structure and Flow 18; 3 Memory Variables 16; 4 Setting Up the Main Program 14; 5 Functions
for Fields and Memvars 26; 6 Communicating with the User 18; 7 Templates and Ranges 10;
8 Screen Forms and Format Files 18; 9 Evaluating User Input 14; 10 Working with the Database
20; 11 Working with Data 16; 12 Printing 18; 13 Housekeeping 14; 14 Putting It Together 8;
15 Testing and Debugging 14; 16 More Advanced Features 18; Programming Notes 6 (285-290).

**Networking (N)**: Contents 8; Intro 4; 1 Overview and Requirements 14; 2 Operational
Considerations 10; 3 Security 36; 4 Network Programming Concepts 26; 5 Networking Commands
and Functions 38; App. A Error Messages 8; App. B Installing on an IBM PC Network 48;
Glossary 6. **Runtime+ (R)**: Contents 2; 1-4 (12, 6, 6, 6); Appendix Error Messages 4.
**Applications Generator (A)**: Contents 2; Intro 2; 1-7 (4, 8, 4, 8, 6, 6, 8).
Duplicate Index 34 (573-606); Notes 6 (607-612).

"Notes" leaves (headed "Notes" or "<Section> Notes", otherwise blank) fill out the last
signature at the end of each **section**, not of each chapter [READ PDF 239-244, 285-290,
581-596].

---

## 9. Reference-entry anatomy (Using ch. 5 Commands, ch. 6 Functions; Networking ch. 5)

Every command and every function **starts a new page**; pages are often half empty
[READ PDF 158-161: CHANGE ends U5-46, CLEAR fills U5-47 alone, CLEAR ALL U5-48, CLEAR
FIELDS U5-49].

### 9.1 Skeleton, in fixed order

| # | Element | Position / form | Cite |
|---|---|---|---|
| 0 | Dictionary running head | entry name repeated large in the head (s.2.2) | PDF 138 |
| 1 | Entry name | blue bold sans caps in side column | PDF 138 |
| 2 | Purpose | one or two sentences in the text column beside the name | PDF 138, 159 |
| 3 | **Syntax** | one or more syntax lines; alternative forms separated by "or"; may add a "where ..." gloss | PDF 316, 394 |
| 4 | **Defaults** | default state/values | PDF 316, 477 |
| 5 | **Usage** | behaviour paragraphs; key names in bold grotesque | PDF 138 |
| 6 | **Options** | each option explained | PDF 138 |
| 7 | **Tips** | practical notes | PDF 138 |
| 8 | **Example(s)** | prose lead-in then tint panel of dot-prompt input + output; may add a screen figure and caption | PDF 139, 477 |
| 9 | **See Also** | comma list of related commands/functions (caps) | PDF 139, 477 |

Order frequency over the whole chapter (text-layer tally of sub-head sequences, 151 command
entries / 70 function entries): the commonest command patterns are Syntax > Defaults > Usage
> See Also (19), Syntax > Usage > See Also (13), Syntax > Defaults > Usage > Examples > See
Also (12); fullest form Syntax > Defaults > Usage > Options > Tips > Examples > See Also.
Functions: Syntax > Usage > Examples > See Also (38 of 70). The order is never permuted,
only thinned [MEAS: OCR tally; OCR misses some heads, counts approximate].

Function entries are named with empty parentheses (e.g. SUBSTR(), DTOC()), alphabetic
[READ PDF 422, 477]. Networking command entries reuse the skeleton and add a command
*class* table at chapter head (Command Class / Description) [READ N5-2, PDF 390].

### 9.2 Chapter-head furniture for the reference chapters

Ch. 5 opens with "Symbols and Conventions" (two-column ruled Symbol / Use table) and
"Limitations" (Item / Limitations table) and a TIP box about using an untouched copy of the
sample disk [READ U5-1..2, PDF 113-114]. Ch. 6 opens with "Using Functions" and the same
TIP [READ U6-1, PDF 397]. Command coverage is alphabetic, beginning with symbol commands
(?, ??, @...) before A [READ Contents PDF 11-15].

### 9.3 Error-message chapter (U7, NA, RA)

Single full-measure column. Intro paragraphs (messages alphabetical; number in brackets is
the ERROR() value). Each entry: message text bold serif flush left (hanging), error number
bold in square brackets flush right on the same line where one exists, explanation in roman
indented below (cause first, remedy second) [READ PDF 489-490].

### 9.4 Glossary

Term bold sans black in the side column; definition serif in text column; symbols sort
first [READ UG-1, PDF 527].

---

## 10. Tutorial lesson anatomy (Learning; Programming chapters follow the same frame)

| Element | Form | Cite |
|---|---|---|
| Opener | Chapter opener (s.3) + one-paragraph orientation in the text column | L PDF 13, 41 |
| "What This Chapter Covers" | blue side head + bulleted topic list (blue bullets) | L PDF 13, 41; P PDF 19 |
| "Preparing for This Chapter" | blue side head: which disk/file must be in place, which earlier chapter is prerequisite | L PDF 13, 41 |
| Concept sections | blue side head + 1-3 paragraphs + figure (photo-like drawing or screen) | L PDF 14 |
| Procedure | numbered steps with **blue numerals**; menu choices in **bold**; typed input in blue sans; keys as glyphs; system response on an unnumbered indented line after the step ("... appears") | L PDF 21, 42 |
| Screens | full screen dump on tint, numbered figure caption | L PDF 13, 22 |
| Boxes | NOTE/TIP ruled boxes interleaved with steps | L PDF 21-22 |
| Closing | "Where Do You Go From Here?" side head, then "Summary" (numbered recap of the commands/keys taught) | L10-8, PDF 238 |
| Learning ch. 9 | bridges menu to dot prompt (commands chapter, 38 pp) | Contents PDF 8 |

---

## 11. Small items

### 11.1 Quick Reference Guide

| Item | Finding | Cite |
|---|---|---|
| Format | 3.83 x 8.33 in [MEAS at 300 ppi, INF]; 36 pages = 9 sheets folded, saddle-stitched [INF]; cream stock (scan paper about RGB 255,254,233) | QR PDF 1 |
| Cover | Same title-sheet system as the binders scaled down: wordmark over rule, title in caps+small caps over three lines, stripe mark lower right in navy | PDF 1 |
| Back cover | Small stripe mark + wordmark, address two lines, copyright/trademark block, part number bottom right (reads "10060-0302", partly clipped) | PDF 36 |
| Inside | Blue + black. Section head: blue sans caps centred between two blue rules | PDF 3, 5, 18 |
| Functions | Two columns: blue function name / black description; no syntax | PDF 3-4 |
| Commands | Full syntax line(s) bold blue; one-line description in black serif indented beneath; alphabetical | PDF 5-17 |
| Tools | dBCODE, dBLINKER, dBRUN pages: "Command Line Formats" and "Options" as blue sub-heads, options as bold flag + indented explanation | PDF 18-20 |
| Network | Networking commands and features on the last text pages | PDF 20-21 |
| Notes | 14 headed Notes pages fill the booklet | PDF 22-35 |

### 11.2 Customer Support Guide (separate booklet)

4.83 x 8.33 in [MEAS/INF]. Cover in the title-sheet system with the stripe mark in **red**
instead of navy (support literature colour) [READ PDF 1]. Contents: ruled list, item left,
folio right, hairline between entries [READ PDF 3]. Sections: registration, updates, disk
replacement, support plans (three tiers with prices), corporate services, on-line services,
publications, 90-day warranty, license agreement, phone numbers, Notes (ruled writing
lines), then perforated business-reply cards (disk replacement, change of address) on the
inside back cover [READ PDF 3, 18-22].

### 11.3 Pamphlets and flyers

| Item | Structure | Cite |
|---|---|---|
| Technical Support | 8 panels on grey stock; red stripe mark; heads in red italic serif caps; tiered support packages each with price line | TechnicalSupport PDF 1-8 |
| dBASE Users insert | Cover with script-like red slogan over the mark; inside a promotional article with screen samples; red coupon flash | dBaseUsers PDF 1-3 |
| Flyer 1 | Cross-sell for a companion product (feature bullets in red) | Flyer1 |
| Flyer 2 | Publishing-group book catalogue: per-title page with title, author line, blurb, price/ISBN line, "at a glance" contents | Flyer2 PDF 1-8 |
| Flyer 3 | Order form: red banner, multi-column price table, ordering terms | Flyer3 PDF 1-2 |

### 11.4 Disk labels (3.5 in, version 1.1, 1987)

| Field | Form | Cite |
|---|---|---|
| Label stock | Pale grey-green, rounded corners, almost full label area | disk1.jpg |
| Product name | Light serif caps (same face as title sheets) with a TM mark, hairline rule under | disk1.jpg |
| Disk name | Bold extended grotesque caps ("SYSTEM #1/SYSTEM #2", "TUTORIAL") | disks.jpg |
| Fields | "Format:" and "Version:" labels bold grotesque, values regular grotesque; format names the PS/2 platform | disk1.jpg |
| Serial / status | Serial number and "EVALUATION COPY" overprinted in a typewriter/impact face | disk1.jpg |
| Part numbers | 5+4 digit, right-aligned above the foot rule (09060-0553 system; 09060-0513 tutorial); a second number printed vertically up the left edge (09060-0753) | disks.jpg |
| Foot | Small stripe mark + wordmark; two-line copyright/licence line in serif | disk1.jpg |

---

## 12. Part numbers and print codes

| Item | Code | Cite |
|---|---|---|
| Manual sections (title versos) | print code "6/86" only; no part number visible on any scanned page | PDF 2 (Learning, Using, Programming) |
| Quick Reference Guide | 10060-0302 (partly clipped) | QR PDF 36 |
| Disk labels | 09060-0553, 09060-0513 (label part numbers); 09060-0753 (vertical) | media scans |
| Pattern | five digits, hyphen, four digits; 10xxx for printed pieces, 09xxx for media [INF from three samples] | -- |

---

## 13. Corrections to the prior survey

| Prior statement | Finding here | Cite |
|---|---|---|
| "Notes" pages at chapter ends | At **section** ends only (and in the QR booklet) | PDF 239-244, 285-290, 581-596 |
| Key names in blue bold sans | In Using the key names are **black** bold grotesque; blue is used for *typed input* in Learning | PDF 138 crop; L PDF 42 |
| Running head over a blue rule, title left | Head text alternates: "CHAPTER n" on versos, chapter title on rectos, both on the inner side; rule full width | PDF 20, 103 |
| Running foot title left, folio right | Mirrored: folio always outer | PDF 19-20 |

---

## 14. Design facts to mirror (summary)

1. Hanging side-head grid, **not mirrored**: 1.33 in head column at left, 4.45 in text column,
   on a 5.8 in block; body ~10/11 serif; heads bold humanist sans.
2. Two colours: black + one mid blue, spent on chapter titles and rules, running heads,
   A-heads, entry names, step numerals, typed input, index letters, and a light tint of the
   same blue behind every screen dump and code example. Body text never blue.
3. Per-chapter pictogram in a rounded square: blue on the opener, black at the outer top
   corner of every other page as a printed thumb tab.
4. Section-prefixed folios (U5-26, LI-1, UA-1, UG-1, X-1) and a single master index keyed by
   section letter, duplicated in both binders.
5. Reference entries: one per page, dictionary running heads, fixed sub-head order Syntax >
   Defaults > Usage > Options > Tips > Examples > See Also (thinned, never permuted).
6. Error messages: alphabetical, bold message, bracketed number flush right, cause then cure.
7. Tutorial chapters: "What This Chapter Covers" / "Preparing for This Chapter" open; numbered
   blue-numeral steps with response lines; "Where Do You Go From Here?" + "Summary" close.
8. Ruled TIP/NOTE boxes (black hairline, label caps) inside the text column.
9. Covers/title sheets: wordmark between hairlines, light caps-and-small-caps title, large
   abstract corporate mark lower right; mark colour codes the document class (navy product,
   red support).

## 15. Not determined from the scans

- True trim and binder dimensions, ring count/diameter, slipcase and binder-spine design (no
  binder or slipcase scan held; all pages cropped identically).
- Whether tab dividers had protruding tabs or were plain title sheets (section sheets are
  only slightly larger than text pages).
- Exact blue and navy (Pantone/process); exact faces and sizes (by eye).
- Part numbers of the manuals themselves; the Getting Started section; keyboard template
  (none in this set's scans).
- The 5.25-in retail disk labels (only the 1987 3.5-in evaluation set is scanned).
