<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Initech Publications Design Reference

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Binding Design Reference (the one document every Initech manual, card,
label and box is typeset and written from)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.4`)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-REF-0001 |
| Version | 1.0 |
| Date | 2026-10-16 |
| Status | RATIFIED for D-2, D-3, D-4, D-6, D-8, D-10, D-11, D-12, D-14, D-15 (PM decisions 2026-10-16). D-1, D-5, D-7, D-9, D-13 OPEN and non-blocking (Sec 1) |
| Supersedes | `docs/packaging/DESIGN-SYSTEM.md` (PUB-RES-0001-C) in full; the design rules (not the outlines) of `docs/packaging/INITECH-DOCUMENTATION-SET.md` (PUB-RES-0001-B) |
| Evidence base | The seven structural annexes in `docs/pubs/originals/` (PUB-ORIG-0001-A/B/C, PUB-ORIG-DB3P, -L123, -WP51, -TP60), and `docs/packaging/` PUB-RES-0001, -A, -B, -D. Where an annex corrects the 2026-10-04 survey, the annex governs |
| Consumers | The `initech-manual` LuaLaTeX class author; technical writers; the `test-pubs` gate lane; package artwork |
| Change control | A change to any numbered rule in this document is a deliberate act with a bead and a worklog note (CLAUDE.md Rule 8, applied to design data) |

---

## 0. How to read this document

### 0.1 Defined terms

| Term | Meaning |
|---|---|
| **Book** | Any bound publication: binder manual, perfect-bound or saddle-stitched book. Quick References are *booklets*, not books (Sec 7.9) |
| **Piece** | Any printed item: book, booklet, card, label, envelope, sleeve, box |
| **Family** | The design family of a book: the original it is modelled on fixes its grid usage, furniture, anatomy and voice (Sec 2). Not the same as the **product family** (colour band, Sec 7.1) |
| **Model** | The original a book mirrors in content, presentation and completeness |
| **Text block** | The type area: 142.0 x 186.3 mm (Sec 3) |
| **Side column** | The left column of the text block that carries side heads, labels, margin notes and callout legends |
| **Text column** | The right column of the text block that carries running text |
| **Gated** | Proven by a green oracle in this repository (CLAUDE.md Law 2). Only gated behaviour is documented as fact (D-14) |
| **DRAFT** | A chapter or entry whose cited gates are not all green; built with a visible DRAFT marker, never released (Sec 11.4) |
| **Shipped table** | The data the build actually ships for a user-visible surface (command table, menu definitions, message catalogue, key map) |
| **House ruling** | A rule made here where the originals disagree or are silent; always stated with its reason |

### 0.2 Source tags

Every rule carries one or more tags. Annex tags give the annex section, then the original's
printed folio or PDF page where the annex records one.

| Tag | Source |
|---|---|
| `[ibm sN]` | `docs/pubs/originals/ibm-dos.md` (IBM PC DOS 3.10 Ref, 3.30 Tech Ref, 4.00, MS-DOS 3.3, IBM training booklet ZR23) |
| `[mac sN]` | `docs/pubs/originals/apple-macintosh.md` |
| `[pkg sN]` | `docs/pubs/originals/ibm-packaging.md` |
| `[ate sN]` | `docs/pubs/originals/ashton-tate-dbase3plus.md` |
| `[lot sN]` | `docs/pubs/originals/lotus-123-r22.md` |
| `[wpc sN]` | `docs/pubs/originals/wordperfect-51.md` |
| `[bor sN]` | `docs/pubs/originals/borland-turbo-pascal.md` |
| `[dsy sN]`, `[set sN]`, `[res sN]`, `[sur sN]`, `[src sN]` | `docs/packaging/` DESIGN-SYSTEM, INITECH-DOCUMENTATION-SET, MANUALS-AND-BOXES-RESEARCH, ORIGINALS-PHYSICAL-SURVEY, PRODUCTION-SOURCING |
| `[D-n]` | PM decision, Sec 1 |
| `[CL ...]`, `[PRD sN]`, `[ADR-nnnn ...]` | `CLAUDE.md`, `InitechOS-PRD.md`, `docs/adr/` |
| `[house: reason]` | House ruling made in this document |

### 0.3 Rule identifiers

Rules are numbered by area so the gate lane and reviewers can cite them: **PG** page,
**TY** type, **NT** notation, **AN** anatomy, **PK** packaging, **PN** part numbers, **VO**
voice, **CP** completeness, **GC** gate contract, **DS** documentation set.

---

## 1. Decisions

### 1.1 Ratified (PM 2026-10-16, binding)

| ID | Ruling | Reasoning from the research |
|---|---|---|
| D-2 | **Six product boxes**: InitechDOS, FLAIR, InitechBase, Initech 123, InitechWord, Turbo Initech | Matches the epics and the period: the operating system and the graphical environment were separate purchases [res s9]. The README's combined part `1024-3300-01` stays the InitechDOS package number (Sec 8) |
| D-3 | **Version numbers on manuals and boxes match the shipped About box or banner.** InitechDOS is fixed at **3.30** (`spec/dos_banner.txt`; `os/milton/kmain.c`). Every other product prints the version string its shipped build displays; until the build displays one, the book prints no version and its part number carries `0000` (PN-03) | The box must agree with the screen; the rule is decided in software first [res s9 D-3]. Enforced by GC-08 |
| D-4 | **One trim, 7 x 9 in (177.8 x 228.6 mm), for every book**, binder or bound | The dominant trim of the originals: Ashton-Tate 7.00 x 9.00 [ate s2], Lotus 6.97 x 9.02 [lot s2], Borland about 7 x 9 [bor s2]; Apple 7.2-7.5 x 9 [mac s2]. IBM's 5.5 x 8.5 half-letter [ibm s2] is folded into the house grid (Sec 3) for shelf unity |
| D-6 | **Black-only interiors.** Colour only on covers, spines, tab dividers, boxes, labels and cards | IBM, Borland and Lotus interiors are black only [ibm s2; bor s2; lot s4]; it is also the cost lever [res s1]. Two-colour models (Ashton-Tate blue [ate s6], WordPerfect blue [wpc s2.3]) are converted by NT-06 / TY-11 |
| D-8 | **Wordmark only**: "INITECH" + "Systems Corporation" + a rule. No illustration logo, no film artwork | Bland is correct; IBM and Ashton-Tate both ran a wordmark-plus-rule title system [dsy s2; ate s8.1; ibm s3] |
| D-10 | **Sources live in this repo under `pubs/`** | Chapters can cite gates and generators can read `spec/` [res s9]; the scans stay outside the repo |
| D-11 | **LuaLaTeX house class `initech-manual`** | Installed, PDF/X capable, reproducible with `SOURCE_DATE_EPOCH`, indexes and cross-references native [res s7] |
| D-12 | **Fictional address; telephone numbers 555-0100 to 555-0199; Technical Support extension 2504** | 555-01xx is reserved for fiction; extension 2504 is already in `README.md` [res s8] |
| D-14 | **Manuals document gated behaviour only**, written as phases land; anything not yet gated carries a visible **DRAFT** marker | Law 2 applied to prose [set s0.1; CL Law 2]. Sec 11.4 |
| D-15 | **Honest colophon naming the real tools** | Borland and Apple printed colophons [bor s9; mac s6]; an in-universe one is true only when InitechWord typesets the books [res s7] |

### 1.2 Open (must not block typesetting)

| ID | Open question | Interim rule so typesetting proceeds |
|---|---|---|
| D-1 | Where production happens | The "Printed in ..." line is a build parameter; omitted when unset; never false (AN-20) |
| D-5 | Binder mechanism (US half-size 3-ring, 70 mm, vs DIN) | Binder pages are laid out for 3 holes at 70 mm (2.75 in) centres [sur s5; ibm s2]; the 22 mm inside margin clears either mechanism (PG-03) |
| D-7 | Media (5.25-in 1.2 MB / 360 K / 3.5-in) | Labels are specified for both sizes (PK-10); media legend on the box is a parameter |
| D-9 | Commercial status | Nothing in the pieces depends on it; legal boilerplate is newly written in either case (VO-12) |
| D-13 | Buy physical originals for measurement | All [MEAS]/[INF] physical figures here stand until a ruler says otherwise; only Sec 7 dimensions would move |

### 1.3 Operator directives encoded

| Directive | Where encoded |
|---|---|
| Manuals mirror the originals in **content, presentation and completeness** | Sec 2 (families), Sec 3-6 (presentation and anatomy), Sec 10 (completeness checklists) |
| Manuals are **correct**, with correctness enforced by gates: every documented command, key and menu exists in the build; every printed example is replayed by `make test-pubs` | Sec 11 (GC-01..GC-12) |
| Updated **in lockstep** with the software | GC-11, GC-12 (bead rule) |
| **Print quality** | PG, TY, NT-06 (screen reproduction at integer scale), PK, GC-07 (reproducible PDF/X) |
| The **TPS Report Generator** gets a dedicated operating-procedures manual in the IBM operations-training register, its busywork rituals documented straight-faced | Family F-OPS (Sec 2), AN-15, VO-06, DS row DOS-TPS |

---

## 2. Book families

A family is chosen per book by its model. Line-level elements (trim, colour, covers, part
numbers, house faces) are uniform across families; grid usage, furniture, anatomy and voice
follow the family. [house: one line, one look outside; the model's system inside -- the
operator's "mirror presentation" directive against the IBM line-system principle [dsy s1]]

| Family | Model (annex) | Register | Books (Sec 12) |
|---|---|---|---|
| **F-IBM** | IBM DOS 3.10 Reference, 3.30 Technical Reference [ibm] | IBM reference voice | InitechDOS Reference, User's Guide, Technical Reference |
| **F-OPS** | IBM PC DOS Operation Training ZR23-6845 [ibm s5.4, s7.5]; boxed procedures of DisplayWrite 4 Getting Started [pkg s6] | IBM operations-training voice | TPS Report Generator Operating Procedures |
| **F-MAC** | Mac OS 8 Installation (Platinum grid) [mac s5.4]; Macintosh User's Guide 1991 (Learning/Reference split) [mac s1, s8.2] | Apple task voice | FLAIR User's Guide, Getting Started with FLAIR, FLAIR Accessories Guide |
| **F-ATE** | dBASE III PLUS 1.1 set [ate] | Ashton-Tate tutorial voice (Learning, Programming); encyclopedic voice (Using) | InitechBase Volume One and Two |
| **F-LOT** | Lotus 1-2-3 Release 2.2 [lot] | Lotus task voice | Initech 123 Setting Up/Tutorial/Quick Start/Sample Applications; Initech 123 Reference |
| **F-WPC** | WordPerfect 5.1 Reference and Workbook [wpc] | WordPerfect feature voice | InitechWord Reference, Workbook, install card |
| **F-BOR** | Turbo Pascal 6.0 four-book set [bor] | Borland programmer voice | Turbo Initech User's Guide, Programmer's Guide, Library Reference, Self-Compilation Guide |
| **F-SUP** | Ashton-Tate Customer Support Guide [ate s11.2]; IBM edition and licence conventions [ibm s4; pkg s3] | Corporate support voice (IBM reference voice applied to policy) | Initech Customer Support Guide, cards, inserts |

---

## 3. The page

### 3.1 Trim, margins, grid

| ID | Rule | Value | Source |
|---|---|---|---|
| PG-01 | Trim, every book | 177.8 x 228.6 mm (7 x 9 in) | [D-4] |
| PG-02 | Bleed on every bleeding element (tabs, covers, dividers) | 3.0 mm | [house: printer default; covers and bleed tabs need it] |
| PG-03 | Margins, mirrored (one grid for binder and bound books) | inside 22.0 mm; outside 13.8 mm; top 19.0 mm to the top of the text block; text block 142.0 mm wide x 186.3 mm deep (44 lines of 12 pt) | Text-block width follows Ashton-Tate 5.80 in and Borland 5.8 in [ate s2; bor s2], reduced 5 mm so one inside margin clears 3-hole drilling at 70 mm [sur s5] and perfect-binding grind; supersedes the two grids of [dsy s5] |
| PG-04 | Running-foot baseline | 219.6 mm from the top trim (9.0 mm above the foot) | Borland 8.65 in, WordPerfect 8.71 in, Ashton-Tate 8.59-8.63 in from top [bor s2; wpc s2; ate s2] |
| PG-05 | Columns | side column 34.0 mm; gap 4.0 mm; text column 104.0 mm | Side column = Ashton-Tate 1.33 in [ate s2]; text column between Borland 4.05 in and Ashton-Tate 4.45 in [bor s2; ate s2] |
| PG-06 | Side column position | **left of the text column on rectos and versos alike (not mirrored)**; the margins mirror, the columns do not | Ashton-Tate PDF 29/30 [ate s2]; IBM no mirrored gutter [ibm s5.1]; WordPerfect and Borland left column [wpc s2; bor s2] |
| PG-07 | Baseline grid | 12 pt; body leading 12 pt; heads, rules and figures re-synchronise to the grid at the next text line | Lotus, Borland, WordPerfect body pitch 12 pt [lot s2; bor s2; wpc s2.2] |
| PG-08 | Paragraph spacing | F-IBM: one full line (12 pt) between paragraphs; all others: 6 pt, no first-line indent | IBM paragraph space = one line [ibm s10]; [house: others unmeasured, ragged-right blocks] |
| PG-09 | Alignment | Ragged right everywhere; no hyphenation of command, key, menu or file names | Borland ragged right [bor s2]; [dsy s4] |

### 3.2 Column usage per family

| Family | Side column carries | Text column carries | Full block (142 mm) used for | Source |
|---|---|---|---|---|
| F-IBM | Hanging entry labels (`Purpose:` ...), level-2 heads | Body; syntax lines; examples | Level-1 heads; tables; message appendix | [ibm s5.1, s8.1] |
| F-OPS | Bold sans side heads; screen callout legends | Body; procedures | Session openers; procedure boxes; data sheets | [ibm s5.4] |
| F-MAC | Level-1 heads hanging at block left; step numbers in grey squares | Body (indented 38 mm, cf. 1.4 in in the model) | Screens; WARNING boxes; summary tables | [mac s5.4] |
| F-ATE | A- and B-heads, entry names, entry labels, callout labels | Body | Error-message chapter; appendix tables; index; contents | [ate s2] |
| F-LOT | **Not used**: single column, measure 132 mm from the block's left edge | -- | Tables and figures may extend to 142 mm | [lot s2: 5.20-5.25 in measure] |
| F-WPC | Side heads ("Notes"), alphabet letters, callout legends, step numerals hang in the gap | Body, procedures | Entry-title rules; index (3 columns) | [wpc s2, s4.2] |
| F-BOR | B- and C-heads flush right, captions flush right, margin notes, icons, entry labels flush right | Body; code | A-head rules; tables may extend right | [bor s2, s2.2] |

### 3.3 Running heads, running feet and folios

| ID | Family | Running head | Running foot, verso | Running foot, recto | Folio form | Source |
|---|---|---|---|---|---|---|
| PG-10 | F-IBM | **None.** A command entry that runs over repeats its two-line title flush to the outside edge, no rule | folio flush left (outside) | folio flush right (outside) | chapter-page `7-11`, Heros Bold 9 pt; appendices `A-4`; index `Index-3`; front matter roman | [ibm s5.1, s8.1, s11] |
| PG-11 | F-OPS | 0.5 pt hairline across the block at the top of every page | `folio  <book short title>` over a hairline | `Session N. <Title>  folio` | chapter-page `3-2`; appendices `A-1` | [ibm s5.4] |
| PG-12 | F-MAC | None | `folio  Chapter N: <Title>` | `Chapter N: <Title>  folio` | continuous arabic; roman front matter | [mac s5.2, s5.4] |
| PG-13 | F-ATE | verso: `CHAPTER n` inner; recto: chapter title inner; Heros Bold caps 7.5 pt over a 0.5 pt full-block rule. Reference chapters: **dictionary head** (entry name Heros Bold caps 14 pt + `CHAPTER 5`/`COMMANDS` small) | folio outer, book title inner (Heros Bold caps 7.5 pt) | book title inner, folio outer | **section-prefixed**: `U5-26`, `LI-1` (Introduction), `UA-1` (appendix), `UG-1` (glossary), `X-1` (master index) | [ate s2.2, s4] |
| PG-14 | F-LOT | None | `3-2  <chapter title>` | `<A-head in force>  3-1` | chapter-page; `Glossary-1`; `Index-1`; Setting Up volume: chapter-page per part, parts told apart by the Master Index codes `SU TU QS SA` | [lot s2.1, s9] |
| PG-15 | F-WPC | None | `folio  ENTRY NAME` (outer only) | `ENTRY NAME  folio` | continuous arabic through all sections; roman front matter; dividers and blanks counted, unprinted. The foot names the **entry in force at the foot of the page** | [wpc s2] |
| PG-16 | F-BOR | None, except Library Reference: **guide word** = first entry on the page, Heros Bold, outer top corner | folio left, book title right | chapter label left in Adventor Oblique (`Chapter 1, <Title>`), folio right | continuous arabic from the Introduction; roman front matter | [bor s2] |
| PG-17 | F-SUP | None | folio outer | folio outer | continuous arabic | [ate s11.2] |

### 3.4 Chapter openers

| ID | Family | Recipe (top to bottom) | Source |
|---|---|---|---|
| PG-20 | F-IBM | Recto. `Chapter N. <Title>` Heros Bold 20/24 at the top of the block; **chapter mini-contents** (AN-22) below; rest of page empty; black thumb tab (PG-30). The verso behind it is a `Notes:` page; text starts on the next recto (folio N-3) | [ibm s5.1] |
| PG-21 | F-OPS | Recto. 2 pt rule 40 mm long at block left; `Session N.  <Title>` Heros Bold 16/18; then fixed heads **Introduction** and **Objectives** (numbered list); text follows on the same page | [ibm s5.4] |
| PG-22 | F-MAC | Recto. A 25 mm square in 40% black at block left with the chapter numeral reversed (Heros Bold 36 pt); title Heros Cn Bold 20/24 beside it; text follows on the same page. Part openers: Part label + part title + chapter list | [mac s5.4, s5.2] |
| PG-23 | F-ATE | Recto. Chapter pictogram in a 25 mm rounded square (solid black, drawing reversed) at the top outer corner; `Chapter N` EB Garamond 30/36; chapter title Heros Bold caps 14/16 over a full-block 1 pt rule; one orientation paragraph; then **What This Chapter Covers** (tutorials) | [ate s3, s5, s10]. Pictograms are original Initech drawings (open asset, Sec 13.2) |
| PG-24 | F-LOT | Recto. `Chapter N` and the title on two lines, flush left, Heros Bold 18/20; command chapters set the title in Heros Bold Italic (`The Copy Command`); command chapters continue with the menu tree (AN-08) | [lot s3, s7.1] |
| PG-25 | F-WPC | Sections open on a recto after a divider leaf (PK-07) with a blank verso. Feature entries **run on** (AN-11). Workbook lessons begin at the top of the next page, verso or recto | [wpc s2, s2.1, s5.1] |
| PG-26 | F-BOR | Recto. The display word letterspaced across the full block (`C H A P T E R`, also `P A R T`, `A P P E N D I X`, `I N T R O D U C T I O N`, `C O N T E N T S`, `I N D E X`) in Adventor caps 11 pt; 1 pt rule under it 53 mm from the top trim; numeral Adventor 24 pt flush right below the rule; title Adventor Oblique 24/28 flush right; text begins 89 mm from the top trim | [bor s2.2] |
| PG-27 | all | Every chapter, part, appendix, glossary and index starts recto, except F-WPC entries and Workbook lessons (PG-25). A needed blank verso carries the running foot only (F-BOR) or nothing (all others) | [ibm s5.1; lot s2; bor s2; ate s8.2; wpc s2] |

### 3.5 Notes pages, thumb tabs, contents

| ID | Family | Rule | Source |
|---|---|---|---|
| PG-28 | F-IBM | `Notes:` page behind every chapter opener; leftover pages at chapter end become `Notes:` pages | [ibm s5.1] |
| PG-29 | F-ATE / F-LOT / F-SUP | Notes leaves fill the last signature at the end of each **section** (F-ATE: headed `Notes` or `<Section> Notes`, no folio) or book (F-LOT: `NOTES:` centred Heros Bold caps; F-SUP: ruled writing lines). Never at chapter ends in F-ATE | [ate s8.2, s13; lot s9; ate s11.2] |
| PG-30 | F-IBM | Interior printed bleed tabs: rectos only and chapter openers; solid black, 9.0 mm visible + 3 mm bleed, 30 mm tall; chapter short name in Heros Cn Bold caps 7 pt reversed, reading bottom to top; positions step down the fore-edge one per chapter (6 positions), then restart at the top | [ibm s5.1: 0.35 x 1.2 in, cycle restarts at Ch. 7] |
| PG-31 | F-BOR | Library Reference only: letter tabs on rectos, solid black 9.0 mm visible x 12.0 mm tall, abutting steps of 12.0 mm, letter(s) reversed Heros Bold 8 pt; letters grouped where entries are few (`W-Z`) | [bor s6.3: 0.36 x 0.48 in, step 0.475 in] |
| PG-32 | F-ATE | Chapter pictogram printed solid black at the top outer corner of every non-opener page as a printed thumb mark; reference chapters (Commands, Functions) use the dictionary head instead | [ate s3] |
| PG-33 | F-WPC | No interior thumb tabs; sections found by divider leaves (PK-07); first entry of each letter carries the letter, Heros Cn Bold 24 pt, in the side column level with the entry title | [wpc s2.1, s4.1] |
| PG-34 | F-MAC / F-LOT / F-OPS / F-SUP | No interior tabs | [mac s5.4; lot s2; ibm s5.4] |
| PG-35 | Contents | F-IBM: `Contents`, chapter lines bold with bold folio, three indent levels, dot leaders. F-ATE: `Contents` EB Garamond 24 + section title Heros Bold caps + rule; `CHAPTER 1` label, title, dot leader. F-LOT: chapter lines Heros Bold with leaders, A-heads body bold, B-heads roman indented. F-WPC: section names in the side column, entries with leaders in the text column, hairline between sections. F-BOR: two columns, chapter lines Heros Bold, A/B heads body with leaders, separate Tables and Figures lists | [ibm s6; ate s8.1; lot s5; wpc s3; bor s3] |

---

## 4. Type

### 4.1 Families and substitutes

Every face is loaded **by file name** from a pinned font list (`pubs/fonts.lock`: file name,
TeX Live path, sha256) so two machines produce the same PDF (GC-07). [house: Rule 11]

| Role | Period face (as identified in the originals) | Free substitute | File (TeX Live unless stated) | Metric-compatible | Used by | Source |
|---|---|---|---|---|---|---|
| Body serif A | Palatino (Lotus, Borland; Ashton-Tate Garamond/Palatino class) | **TeX Gyre Pagella** | `texgyrepagella-{regular,italic,bold,bolditalic}.otf` | yes | F-MAC, F-ATE, F-LOT, F-BOR, F-SUP | [lot s3; bor s2.1; ate s5] |
| Body serif B | Times (IBM 3.x, IBM training, WordPerfect) | **TeX Gyre Termes** | `texgyretermes-*.otf` | yes | F-IBM, F-OPS, F-WPC | [ibm s10; wpc s2.2] |
| Heads sans | Helvetica (Lotus, IBM training, IBM 4.00, Borland labels) | **TeX Gyre Heros** | `texgyreheros-*.otf` | yes | all families: heads, labels, tables, captions, feet | [lot s3; ibm s5.4; bor s2.1] |
| Condensed sans | Helvetica Condensed Bold/Black (WordPerfect titles, side heads, tabs) | **TeX Gyre Heros Cn** | `texgyreheroscn-*.otf` | yes | F-WPC heads; all tab labels; F-MAC titles | [wpc s2.2] |
| Programmer display | ITC Avant Garde Gothic Book / Book Oblique (Borland) | **TeX Gyre Adventor** | `texgyreadventor-*.otf` | yes | F-BOR display words, heads, margin notes, foot labels, index locators | [bor s2.1, s3] |
| Code and screen text | Courier (Borland, Apple listings) | **TeX Gyre Cursor** | `texgyrecursor-*.otf` | yes | all families | [bor s2.1; mac s6]. Proof item: if Cursor reads too light at 9 pt, Nimbus Mono PS (`/usr/share/fonts/opentype/urw-base35/NimbusMonoPS-*.otf`) replaces it everywhere |
| Display serif | ITC Garamond Light-like (IBM cover titles, IBM box titles, Ashton-Tate title sheets) | **EB Garamond** (SIL OFL) | `EBGaramond-*.otf` | no (display only) | covers, title pages, box titles, disk titles, F-ATE chapter-number line | [ibm s3; pkg s2; ate s8.1] |
| -- not used -- | New Century Schoolbook (proposed in [dsy s4]) | TeX Gyre Schola | -- | -- | none | No original uses it: the IBM body is Times-style [ibm s10], correcting [dsy s4] |
| -- not used -- | Optima/Frutiger (Ashton-Tate heads) | none free | -- | -- | Heros substitutes | [ate s5]; [house: no metric clone; one sans in the line] |
| -- not used -- | Apple Garamond, the Borland cover sans, WordPerfect and Ashton-Tate wordmarks | -- | -- | -- | none | Trade dress [res s8; bor rules; ate rules; wpc rules] |
| -- never typeset -- | Chicago 12, Geneva 9 (FLAIR screen faces) | none in TeX Live | -- | -- | Appear **only** inside emulator captures. The command-key glyph and any FLAIR-only glyph in running text are generated as bitmaps from the shipped strikes (`spec/assets/chicago12.h`, `geneva9.h`, `apple_glyph.h`) at integer scale (NT-06) | [house: no free metric Chicago; the repo strikes are the independent truth, CL Law 2] |

### 4.2 Sizes and leading

| ID | Element | Face | Size/leading (pt) | Notes | Source |
|---|---|---|---|---|---|
| TY-01 | Body | Pagella (A) / Termes (B) | 10/12 (A); 10.5/12 (B) | Termes is set 0.5 pt larger for x-height parity on the 12 pt grid | [lot s3 10/12; bor s2.1 10/12; ibm s10 11/12; wpc s2.2 10/12] |
| TY-02 | Defined term, first use | body bold | body | Every bolded term has a Glossary entry (CP-03) | [lot s3] |
| TY-03 | Level-1 / A-head | Heros Bold (F-BOR Adventor) | F-IBM 16/18; F-LOT 14/16 + 2 pt rule to the right margin; F-ATE 12/14 in side column; F-WPC Heros Cn Bold 10/12 in side column over a 0.5 pt full-block hairline; F-BOR Adventor 17/20 hanging from the side-column edge over a 2.2 pt full-block rule; F-MAC 12/14 hanging; F-OPS 11/13 hanging | Sentence case in F-BOR and F-MAC; Title Case elsewhere | [ibm s10; lot s3; ate s5; wpc s2.2; bor s2.2; mac s5.4] |
| TY-04 | Level-2 / B-head | Heros Bold (F-BOR Adventor) | F-IBM 12/14; F-LOT 11/13 no rule; F-ATE 10/12; F-BOR Adventor 14/16 flush right in side column + 0.5 pt rule across the text column | | [ibm s10; lot s3; ate s5; bor s2.2] |
| TY-05 | Level-3 / C-head | Heros Bold / Adventor | 10/12, run-in or side head | F-BOR: flush right in side column, paragraph starts level with it | [bor s2.2] |
| TY-06 | Running head / foot | Heros (F-WPC Heros Cn caps; F-BOR Adventor Oblique) | 8/10; folio Heros Bold 9 | | [ibm s5.1; wpc s2; bor s2] |
| TY-07 | Inline code, file names, messages | Cursor | 9 | Body-size optical match | [bor s2.1] |
| TY-08 | Typed input (inline and displayed) | Cursor Bold | 9 | | [ibm s9; wpc s4.4] |
| TY-09 | Displayed code and transcripts | Cursor | 9/12 on the grid | Indented 4 mm into the text column | [bor s2.1; ate s5] |
| TY-10 | Key names | body small capitals (`smcp`) | body | ENTER, CTRL-BREAK, F1 | [lot s3; ibm s9 MS small caps] |
| TY-11 | Tables | Heros | 8/10; heads Heros Bold over a 0.5 pt rule; 0.25 pt rule under the last row; no vertical rules | | [lot s3; bor s2.3] |
| TY-12 | Figure and table captions | Heros | 8/10; `Figure 3-1` bold then caption. Placement: F-LOT flush left under; F-ATE centred under; F-BOR in side column flush right, two lines; F-MAC one line under | Numbering chapter-sequential (`Figure 13-8`; F-BOR `Figure 1.1`) | [lot s3, s10; ate s5; bor s2.3; mac s7] |
| TY-13 | Margin notes (F-BOR) | Adventor Oblique | 8/10, flush right in side column, top-aligned with the glossed paragraph | | [bor s2.1] |
| TY-14 | Note / Caution labels | Heros Bold | 9 | See NT-07 | [ibm s9; lot s3] |
| TY-15 | Index | body | 8.5/10 | Letter heads Heros Bold 12 (F-BOR Adventor 14) | [ate s5; lot s9; bor s3] |
| TY-16 | Glossary term | Heros Bold | 9.5/12 | | [ate s9.4; lot s9; wpc s4.6] |
| TY-17 | Legal text (verso, licence, boxes) | Heros | 6.5/8 | | [dsy s4] |
| TY-18 | Cover product line / title | Heros Bold 11 / EB Garamond 40 | | | [ibm s3] |
| TY-19 | Box title / media legend / blurb | EB Garamond 60 / EB Garamond 36 / Heros 14 | | Panel 210 x 245 mm | [pkg s2] |
| TY-20 | Rules | 0.5 pt black under running heads; 0.25 pt in tables; heavy rules as per family | Never coloured inside (D-6) | [dsy s4; D-6] |
| TY-21 | Emphasis | Italic only for book titles, placeholders and new terms in F-ATE; never for emphasis in procedures | | [dsy s4; ate s5] |

---

## 5. Notation

### 5.1 Command syntax (one Initech rule, four dialects)

| ID | Element | Initech rule | Source |
|---|---|---|---|
| NT-01 | Keyword or literal | UPPERCASE in Cursor (any case may be typed where the product accepts it, and the book says so once) | [ibm s9] |
| NT-02 | Placeholder | lowercase Cursor Italic in syntax lines; body italic in prose (*filename*, *drive:*) | IBM, Microsoft, Lotus, Borland all italic [ibm s9; lot s7.3; bor s6.1] |
| NT-03 | Optional item | square brackets `[ ]`; never typed | [ibm s9] |
| NT-04 | Choose one | vertical bar between items, e.g. `ON\|OFF` (in a table cell the bar is escaped in source) | [ibm s9 IBM 3.10] |
| NT-05 | Repeatable | ellipsis `...` after the item | [ibm s9] |
| NT-06a | Punctuation shown | type it exactly; separators are a space unless shown | [ibm s9] |

Dialects (only where the model demands them):

| Dialect | Product | Rule | Source |
|---|---|---|---|
| xBase | InitechBase | Typed-expression placeholders in angle brackets: `<expC> <expN> <expD> <expL> <condition> <scope> <field list>`; `/` = choose one; `[...]` repeatable; `&&` comment. A **Symbols and Conventions** table opens the Commands chapter | [ate s7, s9.2; U5-1] |
| Menu-path | Initech 123 | Commands named by slash path in menu words as shown on the control panel: `/Range Format Currency`; @functions named with `@` in caps, arguments italic: `@ROUND(`*x*`,`*n*`)`; macro keys and commands in braces `{EDIT}`, `{BEEP}`; syntax given as a sentence (`@ABS(`*x*`)` returns ...) | [lot s7.2, s7.3] |
| Keystroke | InitechWord | `F8` press; `Shift-F8` hold then press (hyphen); `Shift-F8,1` complete, release, then press (comma); **Feature** (Shift-F8) = feature name bold + key roman in parentheses; menu option: mnemonic letter bold + number in parentheses, **O**ther (4); hidden codes in brackets `[BOLD]`..`[bold]`, `[HRt]` | [wpc s4.4] |
| Railroad | Turbo Initech | Grammar given by railroad diagrams, never by BNF: non-terminals lowercase Heros 8.5 pt in rectangles; symbols in circles; reserved words Heros Bold in rounded pills; 0.5 pt square-cornered lines with solid arrowheads; production label at left; no caption, no number; diagram reading rules stated once in prose at first use. In running text reserved words Heros Bold lowercase, identifiers italic; in code reserved words Cursor Bold | [bor s5] |

### 5.2 Keys, menus, typed text, output

| ID | Rule | Source |
|---|---|---|
| NT-10 | **Key names** are body small capitals, spelled as the keycap of the 101-key reference keyboard reads: ENTER, ESC, TAB, BACKSPACE, SHIFT, CTRL, ALT, INS, DEL, HOME, END, PGUP, PGDN, F1-F12, UP ARROW (arrow keys as words). Never arrow or return glyphs | [lot s5; ibm s9]; [house: ASCII sources, no glyph dependence] |
| NT-11 | Hold-and-press: hyphen (CTRL-BREAK). Press-then-press: a space (END HOME); InitechWord dialect: comma | [lot s5; wpc s4.4] |
| NT-12 | A function key with an assigned name: NAME (KEY). Initech 123: name in small caps, HELP (F1); InitechWord: feature bold, **Help** (F3) | [lot s5; wpc s4.4] |
| NT-13 | FLAIR keyboard equivalents: printed exactly as the shipped menu draws them (the command-key glyph generated from the shipped strike, TY table) followed by the character; in prose the physical PC key bound to the command modifier (`spec/event_model.h` `FLAIR_EVT_MOD_CMD_KEY`) is named per NT-10 | [house: the shipped binding is the truth, CL Law 2] |
| NT-14 | **Menu paths in prose.** FLAIR and FLAIR accessories: "Choose **Open** from the **File** menu" (item and menu bold, item text exactly as drawn, ellipsis included). Turbo Initech IDE: File \| Save, hot letter bold. InitechWord: *Select X from the Y menu* in italic after the step, mnemonic bold italic. Initech 123: the slash path (dialect) | [mac s5.1, s9; bor s2.3; wpc s4.2] |
| NT-15 | **What the user types**: Cursor Bold inline (`DIR A:`) and in displayed examples; system output Cursor regular. F-LOT tutorials use the keystroke table (AN-10) | [ibm s9; wpc s4.4; lot s8.1] |
| NT-16 | **Messages** are quoted character for character from the shipped message table (`spec/dos_messages.json` for InitechDOS; SAMIR's shipped table for InitechBase; the compiler's message table for Turbo Initech) and set in Cursor; placeholders appear as the table defines them (`%c` rendered as italic *d*) | [set s0.3; ibm s8.5] |
| NT-17 | **Displayed examples** sit in the text column, indented 4 mm, Cursor 9/12, input bold. They show the prompt the transcript shows. F-ATE dot-prompt input lines begin with the dot; F-BOR examples are complete compilable programs with reserved words bold and brace comments right-aligned. Every displayed example is a replayed transcript (GC-03) | [ibm s9; ate s7; bor s6.1] |

### 5.3 Screen reproductions

| ID | Rule | Source |
|---|---|---|
| NT-20 | **Always real emulator output.** Every screen figure is generated by the harness from a locked injected trace (the `record-flair` machinery for FLAIR; the text-mode screendump for character screens). No hand-drawn, redrawn or retouched screens. The source names the trace and frame, never an image file | [CL Rule 14; res s7]. Overrides Borland's redrawn line-art screens [bor s2.3] |
| NT-21 | **Scale**: integer pixel replication on a 600 dpi grid, no resampling, no smoothing. FLAIR graphics (square pixels): 3 x 3 (one screen pixel = 0.127 mm; 640 x 480 = 81.3 x 61.0 mm, fits the text column); a full-block figure may use 4 x 4 (108.4 x 81.3 mm). VGA text mode 720 x 400: 3 x horizontally, 4 x vertically (91.4 x 67.7 mm; restores the 4:3 shape) | [house: print quality; the pixel is the truth] |
| NT-22 | **Tone**: character screens are inverted (foreground ink black, background paper; reverse-video cells solid black). FLAIR colour captures are converted to grey through a fixed palette-to-grey table generated from `spec/assets/clut.json`, then screened by the RIP; 1-bit captures stay 1-bit | WordPerfect reverse video as black bars [wpc s4.3]; Mac OS 8 greyscale screens [mac s5.4]; [D-6] |
| NT-23 | **Framing per family**: F-ATE and F-WPC: 10% black tint panel behind character screens, square corners, no bezel (the blue tints of the models become grey). F-LOT: 10% black panel with 2 mm rounded corners. F-MAC: square crop, no frame, no shadow. F-IBM reference: no screen pictures; prompts and output as transcripts (NT-17). F-OPS: rounded-rectangle 0.5 pt frame, numbered callouts (reversed square numerals) and a three-column legend (number / field / description) on the same or facing page. F-BOR: square crop, 0.5 pt frame, callout boxes with the key term reversed | [ate s7; wpc s4.3; lot s10; mac s5.4; ibm s9; bor s2.3] |
| NT-24 | **Callouts**: leader lines (0.3 pt) to labels in the side column (F-ATE, F-MAC, F-BOR); lettered triangles with a side-column legend (F-WPC); right-side horizontal leaders (F-LOT); numbered squares (F-OPS) | [ate s7; wpc s4.3; lot s10; ibm s9] |

### 5.4 Notices, cross-references, terms

| ID | Rule | Source |
|---|---|---|
| NT-30 | **Three notice levels, one meaning line-wide.** *Note* = helpful condition. *Caution* = data can be lost. *Warning* = loss cannot be undone or equipment can be harmed. | [ibm s9; lot s3; mac s5.4] |
| NT-31 | Placement per family: F-IBM run-in **Note:** indented one step further than body; **Important:** for caution; **Warning:** run-in. F-LOT run-in **NOTE** / **CAUTION** caps. F-ATE ruled box (0.5 pt all round) with **TIP** / **NOTE** / **WARNING** label on its own line in the text column. F-WPC italic unlabelled note, 1 pt smaller; caution as bold-italic run-in *Important:*. F-BOR margin note (TY-13); caution as an arrow icon in the side column. F-MAC run-in italic *Note:*; caution and warning in a 10% tint box with **WARNING** caps run-in. F-OPS as F-IBM | [ibm s9; lot s3; ate s7; wpc s4.2; bor s2.3; mac s5.4] |
| NT-32 | **Cross-references** always name the book in italic, even inside the same book: "see *Reference*, Chapter 4" or "see "Batch Files" on page 7-11". Page references in chapter-page form where the family uses it | [lot s1; wpc s1, s4.4; ate s1; ibm s8.2] |
| NT-33 | **See Also** per anatomy (AN-04, AN-11, AN-13); never on F-IBM command entries (the model has none) | [ate s9.1; wpc s4.2; bor s6.1; ibm s8.1] |
| NT-34 | **Defined terms**: bold at first use in a chapter and listed in the Glossary; F-IBM also fixes its core terms in *Terms Used* (diskette, fixed disk, source, target) | [lot s3; ibm s9] |
| NT-35 | **Conventions page**: every book states its notation once (F-IBM in *About This Book*; F-LOT *Conventions* list with an Example line per rule; F-BOR *Typefaces used in these books*; F-ATE *Conventions Used in This Manual* side-headed block) | [ibm s4.1; lot s5; bor s2.3; ate s7] |
| NT-36 | **Canon is documented, never explained**: the 116% quarterly pie, the `570-` trailing-minus format, two-digit years, the hourglass busy pointer, `PC LOAD LETTER`, the InitechPaint menu bar `File Edit Image Layer Select View Window Help`. Each appears where a real manual would show the feature, with a straight face. The FLAIR index lists the hourglass pointer in the slot the 1988 Apple index gave the wristwatch | [set s0.4; CL Law 4; mac s10; `spec/assets/menu_canon.h`] |
