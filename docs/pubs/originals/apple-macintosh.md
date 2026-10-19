<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals (A-2): Apple Macintosh System Software Manuals, 1988-1997

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural model, not prose)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.1`, originals scour A)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-0001-B |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Companions | `ibm-dos.md` (PUB-ORIG-0001-A), `ibm-packaging.md` (PUB-ORIG-0001-C) |
| Supersedes | nothing; deepens Sec 6 of `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` (PUB-RES-0001-A) |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/macos/` (outside the repo; manifest `SOURCES.md`) |
| Copyright posture | Structure, measurements and conventions only. No prose transcribed; no artwork reproduced. Quoted strings are fixed labels only. |

---

## 0. Defined terms and provenance tags

Same tags as PUB-ORIG-0001-A Sec 0: **PDF p. N** (scan page), **[folio]** (printed number), **[READ]**, **[MEAS]**, **[INF]**. Additional terms:

| Term | Meaning |
|---|---|
| **Spot colour** | The single accent ink used inside a book (red in 1988, green in 1991). |
| **Letterspaced head** | Small capitals set with very wide tracking (e.g. a word spread to the width of a column). |
| **Label bar** | A solid spot-colour rectangle carrying a reversed word ("Chapter 1", "Index") above a display title. |
| **Wrap cover** | A one-piece card cover that wraps round a coil-bound block, gives a printed spine, and ends in a flap tucked inside the back cover. |

Files read: `Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf` (296 pp., 1988), `Apple_Macintosh_Users_Guide_Desktop_Z030-1751-A.pdf` (325 pp., 1991), `Apple_Whats_New_In_System_7_Z030-3935-A.pdf` (40 pp., 1991), `Apple_Mac_OS_8_Installation_Manual.pdf` (52 pp., 1997 scan) and `Apple_Mac_OS_8_Install_Manual_alt.pdf` (52 pp., Apple's own 1997 Distiller PDF). About 95 pages viewed.

**The FLAIR target.** ADR DEC-10 fixes FLAIR on Mac OS 8 Platinum with System 7 as a heritage era. The 1991 *Macintosh User's Guide* and *What's New in System 7* are therefore the primary book models; the 1988 guide is the ancestor that fixes Apple's chapter-and-task logic; the 1997 manual fixes the Platinum-era screenshot and step conventions.

---

## 1. What each book is for, and how the set divides material

| Book | Audience / task | Evidence |
|---|---|---|
| **Macintosh System Software User's Guide v6.0** (1988, 030-3281-A) | Everyone who received system software with a Mac; Ch. 1 is a self-paced tutorial, the rest is task reference. Sister books: the owner's guide (setup, glossary), *Macintosh Utilities User's Guide*, *HyperCard User's Guide*; a Tell Apple card is bound in at the back | PDF p. 16-18 [xii-xiv] |
| **Macintosh User's Guide for desktop Macintosh computers** (1991, Z030-1751-A) | One book in two Parts: Part I *Learning Macintosh* (read in sequence, Ch. 1-7), Part II *Macintosh Reference* (use like an encyclopaedia, Ch. 8-16), then a green-paper *Summary*, three appendices, Glossary, Index. Model-specific setup lives in a separate setup book | PDF p. 6 [iii], 18 [xv], 102 [83] |
| **What's New in System 7** (1991, Z030-3935-A) | Upgraders who know System 6; points newcomers elsewhere; each topic ends with a pointer into the *Macintosh Reference* | PDF p. 7 [1], 37 [31] |
| **Mac OS 8 Installation Manual** (1997, Z97609-001A) | Installing, first setup, Internet access, learning resources, troubleshooting; six chapters, no index | alt PDF p. 51-52 [iii-iv] |

**Pattern [INF].** Apple puts *learning* and *reference* inside one book wherever it can (1988: tutorial chapter + reference chapters; 1991: Part I + Part II, with colour-coded thumb tabs), and keeps hardware setup and upgrade news in separate thin books.

---

## 2. Physical format

| Aspect | System 6 Guide (1988) | Mac User's Guide (1991) | What's New in System 7 (1991) | Mac OS 8 Installation (1997) |
|---|---|---|---|---|
| Trim | about 7.4 x 9.0 in [INF: image tagged 300 ppi gives 6.0 x 7.25 in, implausible; rescaled assuming the coil pitch is the standard 0.5 in (17 holes, 120.6 px pitch, PDF p. 1), which also makes body leading 14 pt] | about 8.8 x 7.5 in, **landscape** (wider than tall) [MEAS: 1752 x 1500 px at 200 ppi, PDF p. 105; body leading 14.0 pt at that scale] | 7.2 x 8.9 in [MEAS: 2167 x 2672 px at 300 ppi] | **7.5 x 9.0 in** [READ: page box 540 x 648 pt in Apple's own PDF, alt PDF] |
| Binding | Concealed coil (wire-o style, 17 square holes) inside a one-piece wrap cover with printed spine and a tuck flap; the inside of the flap carries an instruction to tuck it inside the back cover when the book is open [READ PDF p. 1, 296] | Perfect-bound, printed spine reading top-to-bottom [READ PDF p. 2] | Saddle-stitched, self-contained 40 pp. with a gatefold inside front cover [INF: no spine in scan, PDF p. 2-3 spread] | Saddle-stitched [INF: 52 pp., no spine] |
| Colour | Black + red (rules, chapter numerals, index letters) [READ] | Black + green, plus green-tinted pages for the Summary [READ PDF p. 252] | Full colour cover and gatefold; interior black + green, colour screenshots [READ] | Black + grey tints (scan); cover full colour [READ] |
| Pages | xvi + 274 | xvi + ~300 | iv + 31 + covers | iv + 48 (scan edition 40) |

---

## 3. Covers and spines

| Book | Front | Spine | Back |
|---|---|---|---|
| System 6 Guide | White. Six-colour Apple logo top left; title in Apple Garamond (light, condensed) three lines flush left beside it: product name with registered mark, "User's Guide", "Version 6.0" smaller; a large black-and-white System 6 desktop screenshot (overlapping windows, Trash, disk icon) lower right, square-cornered with no frame [READ PDF p. 1] | On the wrap: "Apple" in red then title and version in black, reading top-to-bottom [READ PDF p. 296] | Six-colour logo top left; Apple Computer, Inc. name, street address, phone and telex bottom left; part number + "Printed in U.S.A." bottom right [READ PDF p. 296] |
| Mac User's Guide | Cream. Small black Apple logo top left; large stippled black illustration of mouse and keyboard bleeding off the top right; title in green Apple Garamond light, very large (two lines) lower left; subtitle in black below [READ PDF p. 1] | Title + subtitle in black Garamond, top-to-bottom [READ PDF p. 2] | Black Apple logo top left; address block bottom left; "Z030-1751-A / Printed in Ireland" bottom right [READ PDF p. 325] |
| What's New | White. Six-colour logo top left; title one line in Garamond below it; a colourful isometric illustration lower right [READ PDF p. 1] | -- | Six-colour logo top left; address block bottom left (bold company name); part number bottom right [READ PDF p. 40] |
| Mac OS 8 | Off-white. Six-colour logo top left touching a huge "Mac OS" in black serif with "8" in red; a red square containing a giant white numeral 8 with photo insets in its counters; "Installation Manual" italic below [READ PDF p. 1] | -- | Six-colour logo top left; address block (now with URL) bottom left; part number in bold Z prefix + "Printed in Ireland" bottom right [READ PDF p. 52] |

Constant across 1988-1997 [READ]: logo top left of front and back; address block bottom left of back; part number bottom right of back.

---

## 4. Front matter

| Book | Sequence | Evidence |
|---|---|---|
| System 6 | Cover / inside front: limited warranty on media and replacement (small caps head, caps paragraphs) / loose Addendum slip with its own part number (031-0361-A) / title page repeating the cover / copyright page (Apple logo + company in small caps; copyright and permitted-copy paragraph; two-column trademark list; "Simultaneously published in the United States and Canada") / Contents (iii-x) / Preface (xi-xv) | PDF p. 1-19 |
| Mac User's Guide | Cover / spine / inside front: warranty (left column) + FCC Class B and Canadian DOC notices in English and French (right column) / grey reprise of the cover as title page / copyright page (two columns: notice, the "keyboard Apple logo" trademark caution, copyright, trademarks) / Contents (iii) with thumb tabs / Preface "How to Use This Book" (xv) | PDF p. 1-18 |
| What's New | Cover / full-bleed illustration / gatefold spread "New Features at a Glance" (inside front cover) / copyright page / Contents (iii-iv) / opener (1) | PDF p. 1-7 |
| Mac OS 8 | Cover / blank / title (grey reprise) / copyright page (now with URL; "Every effort has been made" accuracy disclaimer) / Contents (iii-iv) | PDF p. 1-6 |

The warranty is a fixed Apple block: media and manuals replaced within a stated period (90 days in 1988, 12 months in 1991) with the implied-warranty limit in capitals and an "AS IS" disclaimer [READ System 6 PDF p. 2; 1991 PDF p. 3; What's New PDF p. 39].

---

## 5. Page grids

### 5.1 System 6 Guide (1988), PDF p. 15-24, 133-136

| Element | Specification |
|---|---|
| Columns | Wide left **margin column** (~1.8 in [INF]) for figure captions, callout labels and notice labels; text column ~4.2 in to its right |
| Running head | Top left, a letterspaced small-caps word ("T U T O R I A L" in Ch. 1; "M A C I N T O S H" on front matter); a red 2 pt rule runs from above the text column to the right margin [READ PDF p. 7, 22-24] |
| Running foot | Verso: folio then "Chapter N: Title", flush left; recto: current level-1 head then folio, flush right [READ PDF p. 22-24, 134-136] |
| Chapter opener | Recto. Top left "C H A P T E R" letterspaced with the numeral; red rule; title in bold Apple Garamond ~24 pt over the text column; large white space; then an opening with a big light outline initial (drop cap) and small-caps lead-in phrasing a reader question ("Who needs this chapter?"); folio bottom right [READ PDF p. 21, 133] |
| Level-1 head | Bold Garamond ~16 pt under a red rule spanning the text column [READ PDF p. 17, 134] |
| Level-2 head | Bold ~12 pt under a short red rule (~1 in) [READ PDF p. 18, 22] |
| Task block | Related-task heads sit in the margin column; numbered **bold** steps; plain-type comment after each step; the preface explains this anatomy with a labelled thumbnail spread "Figure P-1" (major topic / related task / step-by-step instructions / comment) [READ PDF p. 17] |
| Reference chapter (Finder Menu Commands) | One level-1 head per menu, in menu-bar order; a picture of the pulled-down menu in the margin column as a numbered figure; each command a red-bulleted paragraph with the command name in bold as a run-in lead [READ PDF p. 134-136] |
| Fore-edge | Coil holes along the binding edge of every page [READ] |

### 5.2 Macintosh User's Guide (1991), PDF p. 18-24, 102-105, 250-254

| Element | Specification |
|---|---|
| Format | Landscape page, **two equal columns** separated by a vertical hairline in the gutter between them [READ PDF p. 20-24] |
| Label bar | Green rectangle across the left column, ~0.45 in deep, carrying "Chapter 1", "Part I", "Preface", "Summary", "Glossary", "Index" reversed in Garamond ~18 pt [READ PDF p. 20, 22, 252, 280, 298] |
| Opener | Label bar; display title in Garamond light ~36 pt black, left column; "In this chapter" in green italic with a square-bulleted list (with time estimate in Part I); right column starts with "Before you begin" or the first section [READ PDF p. 22, 104] |
| Part opener | A divider spread: a green horizontal band crossing a giant black Garamond roman numeral (PDF p. 19); then a page with the Part label bar, part title, a chapter list with green "Chapter N" labels, and an italic usage note in the right column (PDF p. 20, 102) [READ] |
| Heads | Level 1 green Garamond light ~20 pt, sentence case; level 2 green ~15 pt [READ PDF p. 23, 105] |
| Thumb tabs | Green bars bleeding off the right edge of rectos, word reversed in letterspaced caps reading top-to-bottom; four positions stepped down the edge: LEARNING, REFERENCE, GLOSSARY, INDEX (all four shown on the Contents page) [READ PDF p. 6, 20, 104, 298] |
| Running foot | Verso: folio + "Chapter N: Title", bottom left; recto: "Chapter N: Title" + folio, bottom right; Garamond ~9 pt [READ PDF p. 23-24, 104-105] |
| Sidebar | Concept panels on a pale green tint spanning both columns [READ PDF p. 24] |
| Summary section | Green-tinted paper; two-column "To do this / Take these steps" tables, task in bold left, numbered steps right [READ PDF p. 252-254] |
| Troubleshooting | Each problem a green level-2 head stating the symptom, a one-line diagnosis, then square-bulleted remedies [READ PDF p. 250-251] |

### 5.3 What's New in System 7 (1991), PDF p. 7-9, 36-37

Single text column (~4.7 in) on the right; **hanging side heads** in green Apple Garamond light ~20 pt set flush right against the text column in a left margin column (~1.6 in); callout text for screenshots also in the margin column, flush right, with leader lines into the picture; folios bottom outside; no running heads [READ]. Opener: a green bar across the top right two-thirds, title in Garamond light, a large initial cap, short intro (PDF p. 7) [READ]. Gatefold "at a glance": one large colour screenshot of the System 7 desktop with ~20 callouts around it, each callout a sentence with the key term in bold and "page N" below it, linked by hairline leaders (PDF p. 3) [READ].

### 5.4 Mac OS 8 Installation (1997), PDF p. 5-9, 17-20, 33-35

| Element | Specification |
|---|---|
| Grid | Level-1 heads hang at the left margin; body indented ~1.4 in [READ PDF p. 7-8] |
| Chapter opener | Recto-or-verso; a grey gradient square (~1 in) at the left with the chapter numeral large and reversed; a row of six dots beneath; title in bold condensed sans ~20 pt beside it [READ PDF p. 7, 9, 17, 33] |
| Folio | Bold numeral at the outside foot beside a vertical stack of five small dots; verso adds "Chapter N", recto the chapter title, small sans [READ PDF p. 5-8] |
| Steps | Each step number in a small grey square in the margin, step text bold condensed sans; follow-on prose roman below [READ PDF p. 8, 17-20] |
| Notice | Grey-tinted box spanning the body measure, bold caps "WARNING" run-in [READ PDF p. 9] |
| Note / Tip | Italic label "Note:" / "Tip:" run-in [READ PDF p. 7-8] |
| Troubleshooting | Bold condensed sans symptom line; then alternating italic "Cause N:" / "Solution N:" run-in paragraphs [READ PDF p. 33-35] |
| Screenshots | Platinum windows and dialogs reproduced at roughly screen size in greyscale, square-cropped, no frame or drop shadow, set within the body measure [READ PDF p. 18-19] |

---

## 6. Type

| Book | Faces | Source |
|---|---|---|
| System 6 | "Apple's corporate font, a condensed version of Garamond" for text and display; ITC Zapf Dingbats bullets; Apple Courier for listings; proofs on LaserWriter Plus, final pages on Varityper VT600, art in Illustrator on Linotronic 300; written in Microsoft Word | Colophon "The Apple Publishing System", PDF p. 295 [READ] |
| Mac User's Guide | Apple Garamond text; Zapf Dingbats and custom symbols for ornaments; Apple Courier for "computer voice"; QuarkXPress pages to film on Linotronic | Colophon PDF p. 324 [READ] |
| What's New | **Display** Apple Garamond; **text** Adobe Garamond; Zapf Dingbats ornaments; Photoshop for screenshots | Colophon PDF p. 39 [READ] |
| Mac OS 8 | Font list of Apple's PDF: AppleGaramond-Book, AppleGaramond-LightItalic, Times-Roman, Times-Italic, Helvetica, Helvetica-Bold, AppleIconThree (symbol font). The printed copy shows a Times-style text face and bold condensed grotesque heads [READ: `pdffonts`; PDF p. 7] | the Garamond faces are used on the cover and titling [INF] |
| Measured | Body leading 14 pt (1991, at tagged scale; 1988 after rescale) [MEAS]; body ~11 pt [INF] | |

Every 1988-1991 Apple book ends with a colophon page titled "The Apple Publishing System" on the last interior page [READ System 6 PDF p. 295; 1991 PDF p. 324; What's New PDF p. 39]. It is a fixed, mirrorable element.

---

## 7. Notices, ornaments and cross-reference marks

| Device | Form | Evidence |
|---|---|---|
| Important | Open triangle + bold "Important" in the margin column (1988) or run-in in the text (1991); the paragraph closes with a second small open triangle | System 6 PDF p. 22; 1991 PDF p. 23 |
| Tip / aside | Four-diamond dingbat + italic lead phrase ("Different screen?") then roman; 1991 closes the paragraph with the same dingbat | System 6 PDF p. 24, 136; 1991 PDF p. 23 |
| Bullets | Small solid squares (Zapf Dingbats), red in 1988 lists, black in 1991 | System 6 PDF p. 17; 1991 PDF p. 20 |
| Cross-reference to another book | Arrowhead dingbat + bold "For more information in the *Macintosh Reference*, see" + quoted section title + chapter | What's New PDF p. 37 |
| Figure | "Figure N-N" bold + caption on the next line, both in the margin column under the picture (1988); "Figure N" bold + caption on one line under the picture (1991) | System 6 PDF p. 22-24; What's New PDF p. 8 |
| Screen callouts | Label text in the margin column with a thin leader line to the feature (red leaders in 1988, black hairlines in 1991) | System 6 PDF p. 24; What's New PDF p. 9 |
| Menu pictures | The pulled-down menu shown as a figure beside the text that describes it | System 6 PDF p. 134-135 |
| Keys | Command-key symbol and named keys in text ("Option", "Shift", "Return") | 1991 PDF p. 250, 254 |

---

## 8. Chapter inventories

### 8.1 System 6 Guide (PDF p. 7-14; folio offset PDF = printed + 20)

| Unit | Title | Starts | Pages |
|---|---|---|---|
| Preface | -- | xi | 5 |
| 1 | Tutorial: Learning Macintosh Basics | 1 | 50 |
| 2 | Basic Macintosh Techniques | 51 | 26 |
| 3 | Working With the Finder Desktop | 77 | 36 |
| 4 | Finder Menu Commands | 113 | 10 |
| 5 | Desk Accessories | 123 | 32 |
| 6 | Keyboards | 155 | 16 |
| 7 | 3.5-Inch Disks | 171 | 22 |
| 8 | Hard Disks | 193 | 10 |
| 9 | Startup Disks | 203 | 16 |
| 10 | MultiFinder | 219 | 28 |
| App. A | A History of Macintosh System Software | 247 | 11 |
| App. B | Troubleshooting | 258 | 11 |
| Index | -- | 269 | 6 |

Every reference chapter opens with "What's in this chapter?" as its first level-1 head (PDF p. 134) [READ].

### 8.2 Macintosh User's Guide 1991 (PDF p. 6; offset PDF = printed + 19)

| Unit | Title | Starts |
|---|---|---|
| Part I | Learning Macintosh | 1 |
| 1-7 | Turning the Macintosh On and Off (3); Understanding the Desktop (7); Setting Up Your Programs (25); Creating a Document and Saving Your Work (39); Changing a Document and Printing Your Work (47); Organizing Your Work (61); Learning More About Your Macintosh (77) | |
| Part II | Macintosh Reference | 83 |
| 8-16 | Setting Up Your Macintosh System (85); Hard Disks and Floppy Disks (97); Organizing Your Files (109); Adapting Your Macintosh to Your Own Use (121); Printing (153); Using Your Macintosh on a Network (169); Expanding Your Macintosh System (209); Care and Maintenance (219); Tips and Troubleshooting (225) | |
| Summary | Working on the Macintosh Desktop (green pages) | 233 |
| App. A-C | Keyboards and Character Sets (244); Exchanging Files and Disks With MS-DOS and Apple II Computers (252); Where to Get More Information About Your Macintosh (257) | |
| Glossary | -- | 261 |
| Index | -- | 279 |

### 8.3 What's New in System 7 (PDF p. 5-6)

New Features at a Glance (inside front cover); one chapter "What's New in System 7" (1-31) of ~20 feature topics, each one to three pages: Help menu, Application menu, opening and saving, System Folder, Startup Items, Apple Menu Items, desk accessories, fonts and sounds, control panels, Views, Finder windows, Trash, Labels, Find, Aliases, File sharing, Virtual memory, Stationery pads, Custom icons, publish and subscribe [READ].

### 8.4 Mac OS 8 Installation (alt PDF p. 51-52)

1 Welcome to Mac OS 8 (1); 2 Installing Mac OS 8 (3); 3 After Installing Mac OS 8 (11); 4 Accessing the Internet (17); 5 Learning About Mac OS 8 (31); 6 Troubleshooting (35-48). The scanned print edition has a shorter Ch. 4 (5 Learning at 23, 6 Troubleshooting at 27) [READ scan PDF p. 5-6]: two printings with different extents exist.

---

## 9. Reference-entry anatomy (Apple has no command pages; these are the equivalents)

| Entry type | Fixed order | Evidence |
|---|---|---|
| Menu command (1988) | Menu head (level 1) -> margin figure of the menu -> one bulleted paragraph per command: **Command name.** run-in, what it does, then conditions, then a dingbat aside where MultiFinder changes behaviour | System 6 PDF p. 134-136 |
| Task (1988) | Task head in the margin column -> numbered bold steps -> plain explanation under each step | PDF p. 17 (Figure P-1) |
| Summary task (1991) | Topic head (green) -> table "To do this" (bold) / "Take these steps" (numbered) | 1991 PDF p. 254 |
| Glossary entry (1991) | Term in green Garamond, then definition in black on the same line, turnovers indented; "See" / "See also" with the target in bold; small inline screenshot or icon beside many entries, text wrapping round it | 1991 PDF p. 280-281 |
| Feature (What's New) | Green hanging side head -> one or two paragraphs -> figure with margin callouts -> arrowhead cross-reference to the *Macintosh Reference* | What's New PDF p. 8-9, 37 |
| Troubleshooting (1997) | Bold symptom -> *Cause n:* / *Solution n:* pairs | Mac OS 8 PDF p. 33-35 |

---

## 10. Index style

| Book | Style | Evidence |
|---|---|---|
| System 6 | "Index" display head under a red rule; **three columns**; red bold letter heads (combined where thin, e.g. "W, X, Y"); sub-entries indented one level; menu commands qualified "(File menu)"; *See* / *See also* italic; ranges with en rule | PDF p. 289-294 [269-274] |
| Mac User's Guide | Green label bar "Index"; **four columns**; green Garamond letter heads; "(continued)" italic on carried entries; INDEX thumb tab | PDF p. 298-299 [279-280] |

A fact worth keeping for canon: the 1988 index has an entry for the "wristwatch pointer" (PDF p. 294 [274]). FLAIR canon makes the hourglass correct and the wristwatch the bug (CLAUDE.md); an Initech index therefore lists the hourglass in the same slot.

---

## 11. Part numbers and print codes

| Book | Code | Position | Evidence |
|---|---|---|---|
| System 6 Guide | 030-3281-A | back cover bottom right, with "Printed in U.S.A." | PDF p. 296 |
| System 6 Addendum slip | 031-0361-A | bottom right of slip | PDF p. 3 |
| Mac User's Guide | Z030-1751-A (Z = international, A = first revision [MEM]) | back cover, with "Printed in Ireland" | PDF p. 325 |
| What's New | Z030-3935-A | back cover bottom right | PDF p. 40 |
| Mac OS 8 | Z97609-001A (print edition); Apple PDF metadata gives 034-0259A and U97609-001A | back cover | scan PDF p. 52; alt `pdfinfo` |

No edition line, printer's key or month code appears inside any Apple book [READ copyright pages]. The year is carried only by the copyright line.

---

## 12. Rules a typesetter can take straight from these originals [INF]

1. One spot colour per book, used only for rules, chapter labels, heads, tabs and index letters; text stays black.
2. Apple Garamond (condensed Garamond) for everything; Zapf Dingbats for bullets and notice ornaments; Courier for program text; a colophon page at the end naming the tools.
3. 1991 model (System 7 era): landscape 8.8 x 7.5 in, two columns with a hairline between, green label bars, light display titles, stepped four-position thumb tabs, green-tinted summary pages, four-column index.
4. 1997 model (Platinum era): 7.5 x 9 in, hanging sans heads, grey numbered step squares, grey WARNING boxes, square Platinum screenshots without frames, dotted-column folios.
5. Learning and reference in one book, split into Parts; troubleshooting entries as symptom -> cause -> remedy.
6. Back cover: logo top left, address bottom left, part number and country of printing bottom right.

## 13. Slips the books carried (PUB-REF-0001 VO-30 evidence)

Read as page images on 2026-10-17 for the house register of tolerated inconsistencies (bead
`initech-ipu5.16`, worklog WL-0150, `spec/pubs_inconsistencies.json`).

| Book | Page | What the page prints | Register entry |
|---|---|---|---|
| Macintosh System Software User's Guide 6.0 (1988, 030-3281-A) | PDF 1 (cover) and PDF 296 (wrap spine) | The cover sets "Version 6.0"; the spine, reading top to bottom, "Apple Macintosh System Software User's Guide version 6.0" with a lower-case v | VO30-10 |
| Macintosh User's Guide (1991) | PDF 1 and PDF 2 | Cover and spine agree ("Macintosh User's Guide for desktop Macintosh computers"): checked, no slip | -- |
