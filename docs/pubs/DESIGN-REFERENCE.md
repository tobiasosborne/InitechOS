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
| **Book** | Any bound publication: binder manual, perfect-bound or saddle-stitched book. Quick References are *booklets*, not books (Sec 7.8) |
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

Rule identifiers by area, for citation by reviewers and the gate lane: **PG** page, **TY**
type, **NT** notation, **AN** anatomy, **PK** packaging, **PN** part numbers, **VO** voice,
**CP** completeness, **GC** gate contract, **DS** documentation set.

---

## 1. Decisions

### 1.1 Ratified (PM 2026-10-16, binding)

| ID | Ruling | Reasoning from the research |
|---|---|---|
| D-2 | **Six product boxes**: InitechDOS, FLAIR, InitechBase, Initech 123, InitechWord, Turbo Initech | Matches the epics and the period: the operating system and the graphical environment were separate purchases [res s9]. The README's combined part `1024-3300-01` stays the InitechDOS package number (Sec 8) |
| D-3 | **Version numbers on manuals and boxes match the shipped About box or banner.** InitechDOS is fixed at **3.30** (`spec/dos_banner.txt`; `os/milton/kmain.c`). Every other product prints the version string its shipped build displays; until the build displays one, the book prints no version and its part number carries `0000` (PN-03) | The box must agree with the screen; the rule is decided in software first [res s9 D-3]. Enforced by GC-08 |
| D-4 | **One trim, 7 x 9 in (177.8 x 228.6 mm), for every book**, binder or bound | The dominant trim of the originals: Ashton-Tate 7.00 x 9.00 [ate s2], Lotus 6.97 x 9.02 [lot s2], Borland about 7 x 9 [bor s2]; Apple 7.2-7.5 x 9 [mac s2]. IBM's 5.5 x 8.5 half-letter [ibm s2] is folded into the house grid (Sec 3) for shelf unity |
| D-6 | **Black-only interiors.** Colour only on covers, spines, tab dividers, boxes, labels and cards | IBM, Borland and Lotus interiors are black only [ibm s2; bor s2; lot s4]; it is also the cost lever [res s1]. Two-colour models (Ashton-Tate blue [ate s6], WordPerfect blue [wpc s2.3]) are converted by NT-22, NT-23 and TY-20 |
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
| **Print quality** | PG, TY, NT-21 (screen reproduction at integer scale), PK, GC-07 (reproducible PDF/X) |
| The **TPS Report Generator** gets a dedicated operating-procedures manual in the IBM operations-training register, its busywork rituals documented straight-faced | Family F-OPS (Sec 2), AN-15, VO-06, DS row DOS-TPS |

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
| -- never typeset -- | Chicago 12, Geneva 9 (FLAIR screen faces) | none in TeX Live | -- | -- | Appear **only** inside emulator captures. The command-key glyph and any FLAIR-only glyph in running text are generated as bitmaps from the shipped strikes (`spec/assets/chicago12.h`, `geneva9.h`, `apple_glyph.h`) at integer scale (NT-21) | [house: no free metric Chicago; the repo strikes are the independent truth, CL Law 2] |

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
| TY-14 | Note / Caution labels | Heros Bold | 9 | See NT-30, NT-31 | [ibm s9; lot s3] |
| TY-15 | Index | body | 8.5/10 | Letter heads Heros Bold 12 (F-BOR Adventor 14) | [ate s5; lot s9; bor s3] |
| TY-16 | Glossary term | Heros Bold | 9.5/12 | | [ate s9.4; lot s9; wpc s4.6] |
| TY-17 | Legal text (verso, licence, boxes) | Heros | 6.5/8 | | [dsy s4] |
| TY-18 | Cover product line / title | Heros Bold 11 / EB Garamond 40 | | | [ibm s3] |
| TY-19 | Box title / media legend / blurb | EB Garamond 60 / EB Garamond 36 / Heros 14 | | Panel 210 x 245 mm | [pkg s2] |
| TY-20 | Rules | -- | 0.5 pt black under running heads; 0.25 pt in tables; heavy rules as per family | Never coloured inside (D-6) | [dsy s4; D-6] |
| TY-21 | Emphasis | body italic | body | Italic only for book titles, placeholders and new terms in F-ATE; never for emphasis in procedures | [dsy s4; ate s5] |

## 5. Notation

### 5.1 Command syntax (one Initech rule, four dialects)

| ID | Element | Initech rule | Source |
|---|---|---|---|
| NT-01 | Keyword or literal | UPPERCASE in Cursor (any case may be typed where the product accepts it, and the book says so once) | [ibm s9] |
| NT-02 | Placeholder | lowercase Cursor Italic in syntax lines; body italic in prose (*filename*, *drive:*) | IBM, Microsoft, Lotus, Borland all italic [ibm s9; lot s7.3; bor s6.1] |
| NT-03 | Optional item | square brackets `[ ]`; never typed | [ibm s9] |
| NT-04 | Choose one | vertical bar between items, e.g. `ON\|OFF` (in a table cell the bar is escaped in source) | [ibm s9 IBM 3.10] |
| NT-05 | Repeatable | ellipsis `...` after the item | [ibm s9] |
| NT-06 | Punctuation shown | type it exactly; separators are a space unless shown | [ibm s9] |

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
| NT-15 | **What the user types**: Cursor Bold inline (`DIR A:`) and in displayed examples; system output Cursor regular. F-LOT tutorials use the keystroke table (AN-17) | [ibm s9; wpc s4.4; lot s8.1] |
| NT-16 | **Messages** are quoted character for character from the shipped message table (`spec/dos_messages.json` for InitechDOS; SAMIR's shipped table for InitechBase; the compiler's message table for Turbo Initech) and set in Cursor; placeholders appear as the table defines them (`%c` rendered as italic *d*) | [set s0.3; ibm s8.5] |
| NT-17 | **Displayed examples** sit in the text column, indented 4 mm, Cursor 9/12, input bold. They show the prompt the transcript shows. F-ATE dot-prompt input lines begin with the dot; F-BOR examples are complete compilable programs with reserved words bold and brace comments right-aligned. Every displayed example is a replayed transcript (GC-03) | [ibm s9; ate s7; bor s6.1] |

### 5.3 Screen reproductions

| ID | Rule | Source |
|---|---|---|
| NT-20 | **Always real emulator output.** Every screen figure is generated by the harness from a locked injected trace (the `record-flair` machinery for FLAIR; the text-mode screendump for character screens). No hand-drawn, redrawn or retouched screens. The source names the trace and frame, never an image file | [CL Rule 14; res s7]. Overrides Borland's redrawn line-art screens [bor s2.3] |
| NT-21 | **Two capture routes, both real.** (a) *Text route* (default for character screens): `harness/console_text.c` decodes the QEMU screendump of the 80 x 25 console (8 x 16 cells drawn with the VGA ROM font) back into characters exactly, failing loudly on any glyph outside printable ASCII; the text is typeset in Cursor 7.5/8.5 inside the NT-23 panel. (b) *Pixel route*: mandatory for FLAIR and for any character region holding CP437 graphics or attributes (Cursor has no box-drawing glyphs): integer replication on a 600 dpi grid, no resampling or smoothing, pixels square on paper. FLAIR 640 x 480 at 3 x 3 = 81.3 x 61.0 mm (text column); full-block figures 4 x 4 = 108.4 x 81.3 mm; console 640 x 400 at 3 x 3 = 81.3 x 50.8 mm | `harness/console_text.c` (bead `initech-ipu5.5`); [house: print quality; the pixel is the truth] |
| NT-22 | **Tone**: character screens print dark on light (foreground ink black, background paper; reverse-video cells solid black). FLAIR colour captures are converted to grey through a fixed palette-to-grey table generated from `spec/assets/clut.json`, then screened by the RIP; 1-bit captures stay 1-bit | WordPerfect reverse video as black bars [wpc s4.3]; Mac OS 8 greyscale screens [mac s5.4]; [D-6] |
| NT-23 | **Framing per family**: F-ATE and F-WPC: 10% black tint panel behind character screens, square corners, no bezel (the blue tints of the models become grey). F-LOT: 10% black panel with 2 mm rounded corners. F-MAC: square crop, no frame, no shadow. F-IBM reference: no screen pictures; prompts and output as transcripts (NT-17). F-OPS: rounded-rectangle 0.5 pt frame, numbered callouts (reversed square numerals) and a three-column legend (number / field / description) on the same or facing page. F-BOR: square crop, 0.5 pt frame, callout boxes with the key term reversed | [ate s7; wpc s4.3; lot s10; mac s5.4; ibm s9; bor s2.3] |
| NT-24 | **Callouts**: leader lines (0.3 pt) to labels in the side column (F-ATE, F-MAC, F-BOR); lettered triangles with a side-column legend (F-WPC); right-side horizontal leaders (F-LOT); numbered squares (F-OPS) | [ate s7; wpc s4.3; lot s10; ibm s9] |

### 5.4 Notices, cross-references, terms

| ID | Rule | Source |
|---|---|---|
| NT-30 | **Three notice levels, one meaning line-wide.** *Note* = helpful condition. *Caution* = data can be lost. *Warning* = loss cannot be undone or equipment can be harmed. | [ibm s9; lot s3; mac s5.4] |
| NT-31 | Placement per family: F-IBM run-in **Note:** indented one step further than body; **Important:** for caution; **Warning:** run-in. F-LOT run-in **NOTE** / **CAUTION** caps. F-ATE ruled box (0.5 pt all round) with **TIP** / **NOTE** / **WARNING** label on its own line in the text column. F-WPC italic unlabelled note, 1 pt smaller; caution as bold-italic run-in *Important:*. F-BOR margin note (TY-13); caution as an arrow icon in the side column. F-MAC run-in italic *Note:*; caution and warning in a 10% tint box with **WARNING** caps run-in. F-OPS as F-IBM | [ibm s9; lot s3; ate s7; wpc s4.2; bor s2.3; mac s5.4] |
| NT-32 | **Cross-references** always name the book in italic, even inside the same book: "see *Reference*, Chapter 4" or "see "Batch Files" on page 7-11". Page references in chapter-page form where the family uses it | [lot s1; wpc s1, s4.4; ate s1; ibm s8.2] |
| NT-33 | **See Also** per anatomy (AN-04, AN-11, AN-12); never on F-IBM command entries (the model has none) | [ate s9.1; wpc s4.2; bor s6.1; ibm s8.1] |
| NT-34 | **Defined terms**: bold at first use in a chapter and listed in the Glossary; F-IBM also fixes its core terms in *Terms Used* (diskette, fixed disk, source, target) | [lot s3; ibm s9] |
| NT-35 | **Conventions page**: every book states its notation once (F-IBM in *About This Book*; F-LOT *Conventions* list with an Example line per rule; F-BOR *Typefaces used in these books*; F-ATE *Conventions Used in This Manual* side-headed block) | [ibm s4.1; lot s5; bor s2.3; ate s7] |
| NT-36 | **Canon is documented, never explained**: the 116% quarterly pie, the `570-` trailing-minus format, two-digit years, the hourglass busy pointer, `PC LOAD LETTER`, the InitechPaint menu bar `File Edit Image Layer Select View Window Help`. Each appears where a real manual would show the feature, with a straight face. The FLAIR index lists the hourglass pointer in the slot the 1988 Apple index gave the wristwatch | [set s0.4; CL Law 4; mac s10; `spec/assets/menu_canon.h`] |

## 6. Document anatomies (fill-in templates)

Templates show the fixed order of elements. `<...>` is filled in; `[...]` is present only
when it applies. **Order is never permuted; a template is only thinned** (the Ashton-Tate
finding [ate s9.1], adopted line-wide). Entry text is authored in `pubs/`; the entry list
itself is generated from the shipped table (GC-01).

### 6.1 Reference entries

**AN-01 -- Command entry, F-IBM** (InitechDOS Reference: commands, batch subcommands,
CONFIG.SYS commands) [ibm s8.1; 3.10 PDF 119-121]

```
[new page]
<NAME> (<Expansion>)                        Heros Bold 18/20, two lines, flush left
Command                                     ("Subcommand" under Batch File Commands;
                                             "Configuration Command" in Ch. Configuring)
--------------------------------------------- 1 pt full-block rule, first page only
Purpose:   <one sentence, third person present: "Displays ...">
Format:    <syntax line(s), NT-01..NT-06>
Type:      Internal    External             asterisks *** set under the one that applies
             ***
Remarks:   <parameter-by-parameter: each placeholder in italic leads its own paragraph>
             Note: <indented a further step>
Example:   <prose lead>
           <transcript, NT-17>
[continuation pages: the two-line title repeated flush OUTSIDE, no rule]
```

Batch subcommands are nested under one entry "Batch File Commands" with "... Subcommand"
titles [ibm s8.1]. A command chapter opens with the notation and "Information Common to All
Commands" before the first entry [ibm s8.1].

**AN-02 -- Function-call entry, F-IBM** (InitechDOS Technical Reference, INT 21h) [ibm s8.2]:
title = hex line (`3FH`) over the name line, bold, flush outside, full-block rule (first page
only) -> **Purpose:** label on its own line, text indented below -> boxed two-column ruled
table *On Entry* / *Register Contents* (AH row first) -> same box *On Return* -> **Remarks:**
indented prose with page cross-references ("on page 6-38") -> continuation pages repeat the
title without the rule. Register names are those of the flat calling convention actually
shipped (`spec/int21h_calling_convention.json`, `spec/int21h_register.json`;
ADR-0003-AMENDMENT-DEC-04a). The model's "Network Access Rights" row is dropped.

**AN-03 -- Message entry, F-IBM** (Appendix A Messages) [ibm s8.5]: appendix opener with a
**Responses** list (hanging letters with bold verbs) -> *Device Error Messages* (format line
with italic placeholders) -> *Other Messages*, alphabetical by message text. Entry = message
text exactly as the shipped catalogue, bold, at the head margin -> indented **Explanation:**
run-in paragraph -> **Action:** run-in paragraph. Generated from `spec/dos_messages.json`, one
per `MSG-DOS-nnnn`; only Explanation and Action are authored, keyed by message id. The
system-halt screen (`PC LOAD LETTER`) is an ordinary entry here [set s1].

**AN-04 -- Command entry, F-ATE** (InitechBase *Using*, Chapter 5) [ate s9.1; U5-26]

```
[new page; dictionary running head: <NAME> large | CHAPTER 5 small]
side column          | text column
<NAME>               | <purpose, one or two sentences>
Syntax               | <syntax lines, xBase dialect; alternatives joined by "or";
                     |  optional "where ..." gloss>
Defaults             | <default state or values>
Usage                | <behaviour paragraphs; key names per NT-10>
Options              | <each option>
Tips                 | <practical notes>
Examples             | <prose lead-in> then tint panel: dot-prompt input + output
See Also             | <NAME, NAME, NAME> (caps, commas)
```

Commonest thinned forms in the model: Syntax > Defaults > Usage > See Also; Syntax > Usage >
See Also. Symbol commands (`?`, `??`, `@`) sort before A [ate s9.1, s9.2]. The chapter opens
with *Symbols and Conventions* (Symbol / Use table), *Limitations* (Item / Limitations table,
generated from `spec/samir/dbf_format.h`) and a TIP box [ate s9.2].

**AN-05 -- Function entry, F-ATE** (*Using*, Chapter 6): as AN-04, name with empty
parentheses (`SUBSTR()`), skeleton Syntax > Usage > Examples > See Also, one per page, opener
*Using Functions* [ate s9.1, s9.2].

**AN-06 -- Error-message entry, F-ATE** (*Using*, Chapter 7) [ate s9.3]: single full-measure
column; intro (alphabetical; the bracketed number is the ERROR() value); each entry = message
text bold serif flush left, `[n]` bold flush right on the same line, then cause and remedy in
roman, indented.

**AN-07 -- Glossary entry, F-ATE**: term Heros Bold in the side column, definition in the
text column, symbols first [ate s9.4].

**AN-08 -- Command chapter, F-LOT** (Initech 123 Reference; one chapter per top-level menu
command, in **main-menu order**) [lot s1, s7.1]

```
Chapter n / The <Name> Commands                     (PG-24)
Menu tree figure  -- generated from spec/i123/menu_tree.golden, boxed levels,
                     the chapter's command joined by a line down to its submenu
<Purpose paragraph>
Command | Task table, one row per sub-command, menu order
Common Uses of the <Name> Commands     A-head + bullets "<task> (/<path>)"
Related Reading                        A-head + bullets; closing line: press HELP (F1)
Terms You Need to Know                 A-head + bullets, term bold at start
[Before You Use <command>]             A-head
<command entries, run on>
```

**AN-09 -- Command entry, F-LOT** [lot s7.2; Ref 4-5]

```
/<Menu> <Command>                  A-head with 2 pt rule
<Purpose, one sentence>
Figure n-m <screen with settings sheet or prompt>
Terms You Need to Know             B-head; bullets, term bold
<How it works: paragraph(s)>
Procedure                          B-head
  1. <imperative step ending "press ENTER">     numerals bold
     <indented unnumbered paragraph: defaults, choices>
Tip                                body bold label + bullets
NOTE <run-in> / CAUTION <run-in>
[/<Menu> <Command> <Sub>]          sub-command head, Heros Bold 11, repeats the skeleton
[<Task> with <command>: An Example]   A-head + numbered steps
```

**AN-10 -- @function entry, F-LOT** (Chapter @Functions; run on, alphabetical, several per
page) [lot s7.3]: head `@NAME` Heros Bold 12; lead = syntax as a sentence with italic
arguments and what it returns; argument paragraph; usage and NOTE run-ins, value tables
(x / y / result); **Example(s)** label then `formula = result` lines. The chapter front holds
*How to Use This Chapter*, an annotated syntax diagram, and *Types of @Functions* with one
summary table per category. Macro command entries (`{BEEP}`) follow the same pattern
[lot s7.3].

**AN-11 -- Feature entry, F-WPC** (InitechWord Reference, Section 2) [wpc s4.1, s4.2; Ref
p. 31-39]

```
[3 pt black rule, full block]                 (blue in the model; black, D-6)
<Feature, Qualifier>                          Heros Cn Bold 14, flush left; first entry of
                                              a letter: the letter in the side column
<Lead: 1-4 paragraphs, what it does and when it is useful> [illustration]
To <verb> <feature>,                          introductory clause ending in a comma
  1  <step>                                   numerals Heros Cn Bold, hanging in the gap
     [menu icon] Select <Item> from the <Menu> menu.   (italic, mnemonic bold italic)
       or                                     (italic "or" between alternatives)
  2  <step>
<interstitial note: italic, 1 pt smaller, unlabelled; strong form: Important: run-in>
------------------------------------------------ 0.5 pt hairline, full block
Notes    | <Sub-topic>        Heros Cn Bold run-in head on its own line
         | <paragraphs>       sub-topics in ALPHABETICAL order
See Also: <Entry>; <Entry>; <Entry>           bold label; entries alphabetical, semicolons
```

Entry names are inverted noun phrases, general term first, so families file together
("Backup, Timed"); a bare parent precedes its children; strict letter-by-letter order;
entries run on [wpc s4.1]. There is no "Keystrokes" side head: keystrokes live in the
procedure [wpc s4.2].

**AN-12 -- Library entry, F-BOR** (Turbo Initech Library Reference) [bor s6.1; LR p. 3-8]

```
<Name> procedure | <Name> function                 <Unit>   Adventor 17, unit flush right
================================================== heavy full-block rule
       Function  <one sentence>                    labels Heros Bold, FLUSH RIGHT in the
    Declaration  <heading in Cursor, reserved          side column, top-aligned
                  words bold>
    Result type  <type>                            (functions only)
        Remarks  <prose; parameters italic; error behaviour stated>
   Restrictions  <one-line constraints>            (if any)
            ->   <important remark>                (arrow icon, if any)
       See also  <Name>, <Name>                    (italic, commas)
        Example  <complete compilable program, Cursor, reserved words bold>
```

The book opens with a self-describing "Sample procedure" entry whose label texts describe
their own contents [bor s6.1]. Entries run on; guide word at top (PG-16); letter tabs
(PG-31). Variant entries qualify in parentheses ("Eof function (text files)").

**AN-13 -- Other F-BOR entries** [bor s6.2]: *Compiler directive*: A-head = directive name;
labels Syntax / Default / Type (Global or Local) / Menu equivalent / Remarks. *Error message*:
run-in Heros Bold number then message text ending in a full stop; explanation in body;
ascending number order; recovery advice as square bullets; compiler errors, then run-time
errors. *IDE command*: one A-head per menu, one B-head per command, menu order, dialog-box
figure, option descriptions.

**AN-14 -- FLAIR entries, F-MAC** [mac s5.1, s5.2, s9]

| Entry | Fixed order |
|---|---|
| Menu command (Reference part) | Level-1 head per menu, in menu-bar order -> figure of the pulled-down menu (capture) -> one bulleted paragraph per command: **Command name.** run-in, what it does, conditions, keyboard equivalent |
| Task (Learning part) | Task head -> numbered steps, step text bold -> plain-type result comment under each step |
| Summary table | Topic head -> two-column table *To do this* (bold) / *Take these steps* (numbered) |
| Troubleshooting | Bold symptom line -> alternating italic *Cause n:* / *Solution n:* paragraphs |
| Glossary | Term Heros Bold then definition on the same line, turnovers indented; *See* / *See also* with bold target; small inline capture beside the entry where useful |

The menu list and every keyboard equivalent are generated from the shipped menu definitions
(`os/flair/shell_menus.h`, `os/flair/finder_menu.c`, `spec/assets/menu_canon.h`); disabled
items are documented as present and dimmed, with the condition that enables them only when
that condition is gated (D-14).

### 6.2 Tutorials, procedures, lessons

**AN-15 -- Procedure, F-OPS** (TPS Report Generator Operating Procedures). The session frame
is the IBM training booklet [ibm s5.4, s7.5]; the boxed procedure is the IBM step box
[pkg s6; DisplayWrite 4 Getting Started p. 17-18]; the per-procedure control block is a
house extension in the same register [house: an operations manual is audited by procedure
number].

```
Session N.  <Title>                                  (PG-21)
Introduction       <what the operator will do in this session and why>
Objectives         Upon completion you will be able to:   (house wording; numbered list)
                   1. <observable action>
<Topic heads>      hanging side heads, body in text column

+-- Procedure TPS-P-<nn>: To <verb> <object> -------------------+  title interrupts the
| Purpose:          <one sentence>                              |  top rule of a 0.5 pt box
| Frequency:        <as the program enforces it, e.g. each report cycle>
| Performed by:     <role named in the program's own prompts>   |
| Prerequisites:    <state required, e.g. FLAIR has been exited; |
|                    the operator is at the A:\> prompt>        |
| Materials:        <diskettes, forms, files>                   |
|  1. <imperative step>                                         |
|     <system response line, Cursor, as the transcript shows>   |
|  2. ...                                                       |
| Verification:     <what the screen or file shows when done>   |
| Records retained: <files written, copies, cover sheets>       |
| Note: <bold run-in>                                           |
+---------------------------------------------------------------+
Lab Project N      numbered exercise against the sample files
Study Questions    numbered; answers in Appendix C
```

Back matter in the model's order: App. A Student Course Evaluation; B Terminal (screen)
Procedures; C Answers to Study Questions; D Review; E Final Quiz; F Data Collection Sheets;
tear-out data sheets with their own form number; Reader's Comment Form, internal-mail version
(AN-25) [ibm s7.5]. Every step, response line and ritual the gated program requires is a
procedure, documented exactly and without comment on why (NT-36, VO-06) -- beginning with
leaving FLAIR, since the generator runs only from the InitechDOS prompt and only through the
FCB calls (`docs/research/dbase-ground-truth.md` s6.3.3). A ritual the program does not
enforce is never invented for the manual.

**AN-16 -- Tutorial lesson, F-ATE** (*Learning*, *Programming*) [ate s10; L PDF 13-42,
L10-8]: opener (PG-23) + orientation -> side head **What This Chapter Covers** (bullets) ->
**Preparing for This Chapter** (disk or file in place; prerequisite chapter) -> concept
sections (side head, 1-3 paragraphs, figure) -> procedures: numbered steps, numerals bold,
menu choices bold, typed input Cursor Bold, the system response on an indented unnumbered
line ("... appears") -> TIP/NOTE boxes between steps -> **Where Do You Go From Here?** ->
**Summary** (numbered recap of commands and keys taught). One running sample business per
section; sample files on the "Sample Programs and Utilities" diskette [ate s7].

**AN-17 -- Tutorial lesson, F-LOT** (*Tutorial*; lesson numbers continue across chapters)
[lot s8.1]: A-head with rule `Lesson n: <Title>` -> goal line + bullets of what you will do ->
B-head per task (or per configuration) -> **keystroke table**: action verb in body bold at
left (**Type**, **Press**, **Select**, **Move**, **Highlight**), what to do at right (typed
text Cursor Bold, keys in small caps), then an *italic purpose gloss* ("to move to ...") ->
commentary paragraph on what changed -> screen panel with right-side callouts. The verb
column is 18 mm wide. *Quick Start* chapter: opener > *What This Chapter Shows You* > task sections with keystroke tables > **Task Summary** (*To* / *You used*) >
**Other Tasks to Try** (*To* / *Use*) [lot s8.2]. *Sample Applications* chapter: scenario >
inputs > per-block A-heads (*Entering the Data*, *Interpreting the Results*, *Changing the
Template*, *Printing Your Work*) > *Item / Address / Meaning* tables [lot s8.3].
*Troubleshooting* appendix: per area, **Problem:** in bold, **Solution:** run-in + bullets;
Initech 123 has no error-message appendix, as its model has none [lot s6, s9].

**AN-18 -- Workbook lesson, F-WPC** [wpc s5.1]: lesson starts at the top of the next page
under a 3 pt black rule, `Lesson N: <Title>` Heros Cn Bold 14 -> scenario (one or two
paragraphs: the document and why; which later lessons finish it) -> task sections, each a
hairline + gerund side head ("Typing the ...") -> numbered steps restarting at 1 per task,
passages to type in bold, indented, 1 pt larger, pull-down alternative in italic after each
step -> checkpoints "Compare your screen with ..." + tint panel with lettered callouts ->
italic asides -> one closing sentence pointing to the next lesson. No objectives, summary or
review questions. Appendix A *Feature List (Alphabetical)* (feature -> lesson numbers),
Appendix B *Feature List (By Lesson)*; two-column index [wpc s5].

**AN-19 -- Tutorial chapter, F-BOR**: continuous prose chapters, no lessons; A-heads name
milestones; short complete programs are typed, run and dissected; numbered IDE steps with
keyboard and mouse icons in the side column; margin notes for asides; no exercises, quizzes
or summary boxes; the chapter ends on a forward pointer. Tutorial and reference never share
a chapter [bor s1, s7].

### 6.3 Front matter

**AN-20 -- Title page and verso** (all books). Title page = the cover grid (PK-02) in black on
white, family band in black with reversed text, no part number [ibm s3]. Verso, fixed order
(IBM edition notice [ibm s4.2] + Ashton-Tate licence summary [ate s8.1] + Borland imprint
[bor s3, s9]):

```
<Ordinal> Edition (<Month> <Year>)                         bold
This edition applies to Version <v> of <Product> and to all subsequent releases until
otherwise indicated in new editions.                       (house wording, newly written)
<Warranty disclaimer: bold run-in lead naming where it does not apply; disclaimer in caps>
<Changes paragraph: information changes periodically; changes in new editions>
<Ordering paragraph: request copies from your Initech representative>
<Reader's Comment Form paragraph with the Initech address>       (binder books)
<Licence summary: copying only as the Software License Agreement permits>
(C) Copyright Initech Systems Corporation <first year>, <edition year>.  All rights reserved.
<Trademark paragraph: Initech marks first, then third-party marks>
[Printed in <country>.]                                      (only if true; D-1)
R<rev>                                       10 9 8 7 6 5 4 3 2 1
<Part number>   <print code>                                 (PN-05)
```

Edition month and year are in-universe constants in the book's metadata file (1991-1993),
never the build date [CL Rule 11]. All boilerplate is newly written; no original wording is
reused [res s8; dsy s10].

**AN-21 -- Preface / introduction, per family**

| Family | Title | Fixed sub-heads, in order | Source |
|---|---|---|---|
| F-IBM | About This Book | Read This First / First-Time Users / Experienced Users / Terms Used / How This Book Is Organized / Related Publications / What Comes in the InitechDOS Package | [ibm s4.1; pkg s6] |
| F-OPS | Preface | Getting Started / Course Description / Prerequisite / Duties and Tasks | [ibm s7.5] |
| F-MAC | How to Use This Book | (Part I Learning read in order; Part II Reference used as an encyclopedia; Summary; conventions) | [mac s1, s4] |
| F-ATE | Introduction (per section) | what the section is / who should read it / chapter-by-chapter list (chapter labels as side heads) / Conventions Used in This Manual | [ate s8.1, s7] |
| F-LOT | About <Book> | Who Should Read This Book / Conventions / Organization of <Book> / Other Initech 123 Documentation (Manual / Description table) / After You Complete <Book> | [lot s5] |
| F-WPC | Getting Started (Section 1) | The Basics: Registration, Installation, The Template, Starting InitechWord, Keys to Know, Function Keys, Pull-Down Menus, Menu Options, Codes, Exit / Getting Help / A Brief Lesson | [wpc s1, s3] |
| F-BOR | Introduction (unnumbered chapter) | what is new (bullets) / the manuals and their four jobs, with "read the other book if..." routing / installing / the README file / Typefaces used in these books / how to contact Initech | [bor s3] |

**AN-22 -- Chapter mini-contents** (F-IBM): directly under the chapter title; label
`Contents` (3.10 form); entries = level-1 heads (bold) and level-2 heads, dot leaders,
chapter-page folios flush right [ibm s5.1].

**AN-23 -- Glossary**: F-IBM and F-ATE: term in the side column, definition in the text
column. F-LOT: term Heros Bold lower case on its own line, definition below; bold words
inside a definition are themselves entries. F-WPC: term on its own line, acronyms expanded
with bold initials. F-MAC: AN-14. [ate s9.4; lot s9; wpc s4.6; mac s9]

**AN-24 -- Index**

| Family | Columns | Letter heads | Locators | Special | Source |
|---|---|---|---|---|---|
| F-IBM | 2 | single bold capital with space above | chapter-page, commas | "Special Characters" group first | [ibm s11] |
| F-OPS | 2 | single bold capital | chapter-page | -- | [ibm s11] |
| F-MAC | 3 | Heros Bold capitals, combined when thin (`W, X, Y`) | continuous; ranges with en rule | menu commands qualified "(File menu)"; *See* / *See also* italic | [mac s10] |
| F-ATE | 2 | Heros Bold single capital | **section letter + page** (`U5-28 - U5-31`), tags "(figure)", "(table)", "(tip)", "(note)" | master index in both binders, opens with a boxed *Key to Index Page Numbers* | [ate s4] |
| F-LOT | 3 | Heros Bold capital 14 pt | chapter-page; ranges "1-25 to 1-26" | Setting Up volume: *Master Index* with a boxed key of codes, each locator line prefixed by the bold book code (`SU`, `TU`, `QS`, `SA`), one line per book | [lot s9] |
| F-WPC | 3 (Reference), 2 (Workbook) | Heros Cn Bold | word space before first locator, commas, hyphen ranges | codes and macro commands indexed in bracket/brace form | [wpc s4.6, s5] |
| F-BOR | 2 | Adventor 14 | **locators in Adventor Oblique** | directives and switches sorted at their letter; symbols first; up to three levels | [bor s3] |

**AN-25 -- Reader's Comment Form**

| Variant | Used in | Layout | Source |
|---|---|---|---|
| IBM two-page unit, x3 | every binder book (F-IBM, F-ATE, F-WPC Reference) as the last leaves | Form side: wordmark top left, product family top right, **Reader's Comment Form**, book short title left + part number right, three short paragraphs (how comments are used; Initech may use them; not for technical questions), *Comments:* and open space. Mailer side: BUSINESS REPLY MAIL box with a fictional permit number, POSTAGE WILL BE PAID BY ADDRESSEE, the fictional Initech address (D-12), indicia bars, "Fold here" dotted line with inverted text, "Please do not staple", "Tape" | [ibm s12] |
| Lotus rating grid, x1 | F-LOT volumes, bound in | mail-to block; check-line questions; grid rows = every piece of the set, columns Excellent / Very good / Good / Fair / Poor / Didn't use; free-text; dashed cut line along the binding edge | [lot s9] |
| Internal mail, x1 | F-OPS | "Possible topics for comment" line; internal routing address rules | [ibm s12] |

Postal bars on any form encode nothing routable [house: D-12 fiction rule].

**AN-26 -- Colophon** (D-15): last interior page of every book (Apple position [mac s6]); in
F-BOR also folded into *Typefaces used in these books* [bor s9]. Fixed text kept once in
`pubs/common/colophon.tex`, naming truthfully: LuaLaTeX and the `initech-manual` class; the
faces by their real names (TeX Gyre Pagella/Termes/Heros/Heros Cn/Adventor/Cursor, EB
Garamond); that screens were captured from InitechOS running in an emulator under the
project harness. It is the only page exempt from the period vocabulary check (GC-06).

## 7. Covers, spines, dividers, boxes, labels, cards

### 7.1 Colour system (PK-01)

Values are approximate screen matches; proof against a physical Pantone guide before any
spot job; use CMYK for digital print and proof once [dsy s3]. Colour never enters an
interior (D-6). Family assignment from [dsy s3]; the band *grid* below is IBM's [pkg s2].

| Name | Use | CMYK (approx.) | Pantone (approx.) |
|---|---|---|---|
| Initech Black | type, rules, interiors; rich black 40/30/30/100 for large solids | 0/0/0/100 | Black C |
| Initech Putty | box and binder ground | 0/3/12/10 | 7527 C |
| Interior tint | screen panels, F-MAC warning boxes, numeral squares (black screen, not a colour) | 0/0/0/10 and 0/0/0/40 | -- |
| Systems family (InitechDOS incl. TPS manual, FLAIR) | family band, spine band, tabs, label band | 100/0/45/35 | 3292 C |
| Information Management family (InitechBase) | as above | 100/67/0/23 | 288 C |
| Decision Support family (Initech 123) | as above | 90/12/95/40 | 349 C |
| Office Systems family (InitechWord) | as above | 0/100/63/29 | 201 C |
| Programming family (Turbo Initech) | as above | 90/100/0/15 | 2627 C |
| Desktop Teal | FLAIR box illustration only (the desktop `#8DDCDC`) | 36/0/0/14 | 324 C |
| Rule grey | Reader's Comment Form and card hairlines | 0/0/0/50 | Cool Gray 8 C |
| Support red | Customer Support Guide band and support literature (document-class colour) | 0/100/100/10 | 485 C |

Support red follows Ashton-Tate's practice of colour-coding support literature apart from
product manuals [ate s11.2]. [house: one extra colour, outside the family set]

### 7.2 Book covers (PK-02)

IBM's manual-cover grid [ibm s3], set on **white** stock (IBM's title-page form, black on
white) with the band in the family colour [house: reconciles [dsy s6] white covers with the
IBM grid; IBM's corporate blue cover has no Initech equivalent]. Measured from the trim of
the 177.8 x 228.6 mm cover; fractions from [ibm s3] with a 12 mm minimum safety.

| Zone | Content | Position (mm from top / left) | Type |
|---|---|---|---|
| Product line | `<Product>  Version <v>` (version omitted until shipped, D-3) | baseline 12.0; x 23.0 | Heros Bold 11, black |
| Hairline | -- | y 14.0; x 23.0 to right trim + bleed | 0.5 pt black |
| Title | `Reference`, `User's Guide`, `Technical Reference` ... | cap top 17.0, baseline 28.0 (second line +44 pt); x 23.0 | EB Garamond 40, black |
| Family band | product family name reversed | top 55.0, depth 6.5; from left bleed to x 153.0 | band in family colour; text Heros Bold 11 white from x 23.0 |
| Wordmark | `INITECH` letterspaced +100 over a 0.5 pt rule over `Systems Corporation` | top 183.0; x 23.0; 30.0 mm wide | Heros Bold (extended by tracking) / Heros 7 |
| Part number | `<part number>` | baseline 201.0; flush right at x 154.8 | Heros 8 |
| Illustration | none on manuals | -- | [dsy s6] |

### 7.3 Spines (PK-03)

Text reads **top to bottom** (state it to German printers) [dsy s6; mac s3]. From the top:
wordmark (horizontal if the spine is at least 25 mm, else rotated), product name, book
title, the family band at **exactly the cover height** (55.0-61.5 mm from the top) so a shelf
of Initech books shows one stripe per family, version, part number at the foot. No spine
text below 6 mm (about 120 pages on 80 g/m2 uncoated); the exact width comes from the
printer's calculator [dsy s6; res s7].

### 7.4 Binders and back covers (PK-04, PK-05)

| ID | Item | Rule | Source |
|---|---|---|---|
| PK-04 | Binder cover | Initech Putty vinyl or printed board; black + family band; same grid as PK-02; spine label printed or in a spine pocket (D-5) | [dsy s6] |
| PK-05 | Back cover (books) | wordmark top left; address block (D-12) bottom left; part number, print code and "Printed in" line (if true) bottom right; nothing else | Apple constant 1988-1997 [mac s3]; IBM colophon block [ibm s3] |

### 7.5 Dividers (PK-06, PK-07)

| ID | Used in | Rule | Source |
|---|---|---|---|
| PK-06 | F-IBM and F-ATE binders | Cut-tab dividers, 200-250 g/m2 board, 3 holes at 70 mm; page 177.8 x 228.6 + tab projection 12.7 mm; 5-position bank, tab 38.0 mm tall, positions stepped top to bottom in book order; tab in the family colour, label reversed Heros Cn Bold caps 9 pt; divider face: section title (EB Garamond 24) + short contents list. F-ATE tabs carry the section letters L, U, P, X matching the folio prefixes | [dsy s8; ate s4] |
| PK-07 | F-WPC Reference and Workbook | Printed divider **leaf** on text stock per section (Getting Started, Reference, Appendix, Glossary/Index): a solid bar at the fore-edge 9.0 mm visible + bleed, running from 15 mm below the top to 23 mm above the foot, all sections at the **same** position (no stepping); section name reversed Heros Cn Bold, reading top to bottom; a 6 x 9 field of small triangles at 10% black with the middle row solid and pointing to the tab. Reference: bar in the Office Systems colour (a divider is a tab, D-6); Workbook: black. Blank verso | [wpc s2.1, s5] |

### 7.6 Boxes and slipcases (PK-08, PK-09)

| ID | Item | Rule | Source |
|---|---|---|---|
| PK-08 | Sizes | Standard: outer about 245 x 210 x 70 mm. Large: 245 x 210 x 140 mm. Front panel 210 wide x 245 tall (aspect 0.857, IBM 0.84-0.88). Final inner sizes follow the sourced binder (D-5) | [dsy s7; pkg s1] |
| PK-09 | Assignment | Standard: InitechDOS, FLAIR, Initech 123. Large: InitechBase (two binders), InitechWord (binder up to 950 pp + Workbook), Turbo Initech (four books) | [set s1-s6; house: InitechWord stack exceeds 70 mm] |

**Front panel grid** (IBM 1984-88 line, measured [pkg s2]; mm on the 210 x 245 panel):

| Zone | Position | Content | Type |
|---|---|---|---|
| Ground | full bleed | Initech Putty, matt | -- |
| Kicker (optional) | x 23-74, baseline 12 | series or category, e.g. `Initech Systems Software` | Heros Bold 14, black |
| Hairline | y 13, x 23 to right edge (bleeds) | -- | 1 pt white |
| Product title | x 23; y 15-39; one to three lines | `InitechDOS` / `Version 3.30` | EB Garamond 60, black, tight leading |
| Media legend | x 132-178; y 15-32 | `5.25" diskettes` (D-7 parameter) | EB Garamond 36, white |
| Document line | x 23; y 49-54 | name of the binder or book inside (`Reference`) | Heros 14, black |
| Family band | y 60.0-66.1; left edge (bleed) to x 176 | family name reversed from x 23 | Heros Bold 16, white on family colour |
| Illustration | x 38-176; y 66-196, hung from the band, right edge aligned to the band | one real emulator capture in full colour (NT-20), no border | -- |
| Wordmark | x 23-50; y 194-228 | wordmark block (PK-02), white | -- |
| Blurb | x 97-174; y 196-235, ragged right | three to seven imperative verb phrases ("Manages files on diskette and fixed disk") | Heros 14, black |

**Back panel** [pkg s3]: left half (x 8-101): fixed bold heads, each over a 1 pt rule, in
this order: **Software required:** -> **Software included:** (diskette count and size) ->
half-width family band echo, no text (y 64-69, left edge to x 101) -> **System requirements:**
(processor, memory, display, diskette drive, as `README.md` states them) -> **Options
supported:**; then the limited-warranty line; footnotes; colophon lower left (y 194-233):
copyright, "All rights reserved.", corporate name, address (D-12), "Printed in ..." (if
true), **box part number alone on the last line**. Wordmark centred at x 59-80, y 194-201.
Right half (x 107-206, full height): the Initech Program License Agreement, heads in order
**License** (lettered a-d) / **Term** / **Limited Warranty** / **Limitation of Remedies** /
**Service** / **General**, Heros 6.5/8, licence form number with month/year at the foot. The
same licence text is printed on the License Agreement envelope (PK-12) [pkg s3; dsy s9].

**Side panels** carry the spine logic (product name, band at the front-panel band height);
the Initech 123 box carries the fill-in **Hardware Chart** on one side, repeated in
*Setting Up* [lot s1; sur s2]. A printed paper sleeve may carry the product-specific panels
round an identical slipcase [dsy s7]. Shrink-wrapped [dsy s7].

### 7.7 Diskette labels (PK-10)

| Field | Rule | Source |
|---|---|---|
| Stock | 5.25 in about 105 x 38 mm; 3.5 in front about 70 x 54 mm with top wrap; set from the purchased stock | [dsy s9 INF] |
| Band | family colour across the top (and over the wrap on 3.5 in) with `INITECH  INITECH  INITECH` letterspaced reversed Heros Bold 6 caps | [wpc s8; dsy s9] |
| Product line | product name EB Garamond 10 with TM, hairline under | [ate s11.4] |
| Disk title | EB Garamond 14 (`Startup`, `Utilities`, `Sample Programs and Utilities`) | [wpc s8; ate s11.4] |
| Fields | `Version:` and `Format:` labels Heros Bold 7, values Heros 7; `Disk n of m` | [ate s11.4] |
| Instruction | one sentence Heros 7, typed text bold | [wpc s8] |
| Serial | per copy, Cursor 9, impact-printed look: `S<PPPP>-<VV>-<nnnnnn>` (PN-07) | [bor s8.2; dsy s9] |
| Part number + print code | right-aligned above the foot; copyright 5 pt bottom right | [ate s11.4; wpc s8] |

### 7.8 Booklets, cards, templates, envelopes (PK-11..PK-15)

| ID | Item | Size | Rule | Source |
|---|---|---|---|---|
| PK-11 | Quick Reference booklet | 88.9 x 228.6 mm (3.5 x 9 in: the house trim halved), saddle-stitched | Cover in the PK-02 grid scaled; inside: section heads Heros Bold caps between two rules; commands with full syntax bold over a one-line description; F-ATE functions name / description in two columns; Notes pages fill the booklet; part number back cover bottom right | [ate s11.1; lot s1; house: D-4 governs books, booklets take the halved trim for shelf height] |
| PK-15 | Initech Customer Support Guide | 7 x 9 in book, saddle-stitched (D-4) | Contents ruled list item/folio; sections: Software Registration / Software Updates / Disk Replacement / Software Support / Support Plans / Corporate Services / Publications / Limited 90-Day Warranty / Software License Agreement / Important Telephone Numbers (555-01xx, extension 2504) / Notes (ruled lines) / perforated reply cards (Disk Replacement, Change of Address) | [ate s11.2; D-12] |
| PK-12 | License Agreement envelope | to fit the diskettes | Putty; front `INITECH SYSTEMS CORPORATION LICENSE AGREEMENT` + "Read before opening"; full licence on the back, Heros 6.5/8; seal sticker across the flap. Turbo Initech: single-sheet **Initech License Statement** instead | [dsy s9; sur s4; bor s9] |
| PK-13 | Cards | as stated per card | Warranty Registration Card 6 x 4.25 in perforated postcard, business-reply back; In This Package card 7 x 9 in (checklist with part numbers); media exchange coupon 6 x 4.25 in; Proof of License card (InitechDOS); InitechWord install card 5.0 x 4.25 in folded once: black band with title reversed in Heros Cn Bold caps, numbered steps 1-n with Heros Cn Bold numerals, "(See other side)", code bottom right; Turbo Initech Run-Time Library Source order form | [dsy s11; wpc s7; set s6, s7] |
| PK-14 | Keyboard templates | die-cut 0.4 mm card, sized to the keyboard | InitechWord: long strip for the 101-key top row, cells of five centred lines (CTRL command red, ALT blue, SHIFT green, plain black, key name italic), groups of four keys with legend cells, quick-list panel at the left, Heros / Heros Oblique, vertical rules only. Initech 123 and Turbo Initech: 101-key strip and 84-key L-shaped template (F-keys at the left), key name above each command. Command text generated from the shipped key map | [wpc s6; lot s10; set s4] |

Sleeves: white paper sleeve, handling pictograms and temperature range on the back in
English, German, French and Dutch [sur s1 MCN sleeve; dsy s9].

## 8. Part numbers and print codes

| ID | Rule | Source |
|---|---|---|
| PN-01 | **Part number** `PPPP-VVVV-II` on every piece: product code, version without the point, item code. Extends `README.md` `1024-3300-01` | [dsy s10] |
| PN-02 | Product codes: 1000 corporate/common items; 1024 InitechDOS (the README's code); 1031 FLAIR; 1040 InitechBase; 1051 Initech 123; 1062 InitechWord; 1077 Turbo Initech | [dsy s10; house: 1000 for common items] |
| PN-03 | `VVVV` is the shipped version (D-3): `3300` for InitechDOS. A product whose build displays no version uses `0000` and may not be released (GC-08) | [D-3] |
| PN-04 | Item codes: 01 complete package (box part number); 10-19 books (10 = principal reference, then in DS order); 20-29 quick references and templates; 30-39 5.25 in diskettes (30 + disk number); 40-49 3.5 in diskettes; 50-59 cards; 60 License Agreement envelope; 70 Customer Support Guide; 80-89 inserts and flyers | [dsy s10] |
| PN-05 | **Print code** printed beside the part number: `<edition letter><printing digit>-<M>/<YY>`, e.g. `A1-3/92`. The verso adds the revision `R<n>` at the left and the printer's key `10 9 8 7 6 5 4 3 2 1`, lowest digit = printing digit | WordPerfect month codes [wpc s9]; Ashton-Tate "6/86" [ate s12]; Borland R-code + printer's key [bor s9]; IBM edition ordinal [ibm s4.2] |
| PN-06 | **Edition** (letter, verso ordinal "Second Edition (Month Year)") changes when the text changes for a new version or new gated features. **Printing** (digit, printer's key) changes for corrections only. Both are recorded in the book metadata, not derived from the build clock | [ibm s4.2; bor s9; CL Rule 11] |
| PN-07 | Diskette serial `S<PPPP>-<VV>-<nnnnnn>`, e.g. `S1024-33-000127`, sequence per copy | [dsy s9; bor s8.2] |
| PN-08 | Licence form number `LF-<PPPP>-<YY>` at the foot of every licence text | [pkg s3: IBM licence form number] |
| PN-09 | Bar codes, if any: UPC-A in a restricted-circulation prefix (2 or 4) only | [dsy s10] |

Examples: `1024-3300-10 A1-3/92` InitechDOS Reference, first edition, first printing;
`1024-3300-31` InitechDOS 5.25 in Disk 1; `1024-3300-01` InitechDOS package (box);
`1000-0000-70` Initech Customer Support Guide; `1040-0000-10` InitechBase Volume One while the
SAMIR build displays no version.

## 9. Voice and register

### 9.1 Registers

| ID | Register | Families | Person, tense, mood | Signature moves | Source |
|---|---|---|---|---|---|
| VO-01 | IBM reference | F-IBM | Third person, present; imperatives only in steps | `Purpose:` begins with a third-person verb ("Copies ..."); terse remarks parameter by parameter; terms fixed once in *Terms Used*; no rhetorical questions; no "you will love" | [ibm s4.1, s8.1] |
| VO-02 | Apple task | F-MAC | Second person, present, friendly | Steps bold, result comment plain; "choose ... from the ... menu"; troubleshooting as symptom -> cause -> solution | [mac s5.1, s9] |
| VO-03 | Ashton-Tate tutorial and encyclopedic | F-ATE | Learning/Programming: second person, one running business case, the reader "you"; Using: impersonal, authoritative | Openers promise coverage, close with *Where Do You Go From Here?* and a numbered *Summary*; Using sends syntax questions nowhere else | [ate s1, s10] |
| VO-04 | Lotus task | F-LOT | Imperative, task first ("To ..., ..."), present | Keystroke tables with italic purpose glosses; *Common Uses*, *Terms You Need to Know*; Problem/Solution | [lot s7, s8] |
| VO-05 | WordPerfect feature | F-WPC | Lead in third person about the feature; procedure imperative after "To <verb>,"; Workbook second person | Notes alphabetised; "See Also:"; lessons without objectives or review; cross-references always name *Reference* | [wpc s4, s5] |
| VO-06 | IBM operations-training | F-OPS | Course register: "the operator" and "you"; present; objectives as observable actions; steps imperative | Every ritual the program requires is a numbered procedure with Verification and Records retained; frequency and prerequisites stated as fact; **no explanation, apology or irony** -- tedium is documented as diligence; study questions test recall of procedure | [ibm s5.4, s7.5; set s0.4] |
| VO-07 | Borland programmer | F-BOR | Second person, peer to peer, confident; sentence-case heads | Complete compilable examples; the language defined normatively ("Turbo Initech implements ..."); restrictions stated as the definition, not as omissions; no exclamation in manuals (brochure voice is box copy only) | [bor s1, s7, s8.3; set s6] |
| VO-08 | Corporate support | F-SUP, licence, boxes | Third person, formal | Policies as numbered or lettered clauses; box blurbs as imperative verb phrases | [ate s11.2; pkg s2] |

### 9.2 Style sheet

| ID | Rule | Source |
|---|---|---|
| VO-10 | Product names exactly: InitechOS, InitechDOS, FLAIR, InitechBase, Initech 123, InitechWord, Turbo Initech, InitechCalc, InitechPaint, File Manager, FILE COPY, TPS Report Generator. Company: "Initech Systems Corporation" in full on versos, boxes and cards; "Initech" alone only inside product names and "your Initech representative" | `README.md`; [dsy s2] |
| VO-11 | Trademark mark at first use per book: InitechOS(TM), InitechDOS(TM), FLAIR(TM), InitechBase(TM), InitechCalc(TM), InitechPaint(TM); Turbo Initech(R). Sources write `\tm{}` / `\reg{}`; the class renders the symbol | `README.md` trademark paragraph; [CL Rule 12] |
| VO-12 | Every sentence is new, boilerplate included; structure and conventions are modelled, prose never copied; no original trade dress | [res s8; set s0.2] |
| VO-13 | Terms: diskette (never floppy); fixed disk in F-IBM and F-OPS, hard disk in F-MAC (each model's word); file name (two words) in prose, *filename* as a placeholder; on-line (hyphenated); menu bar; dialog box; pointer; click, double-click, drag; directory (F-IBM) and folder (F-MAC) for the same object, each in its own book | [ibm s9 Terms Used; mac s7; wpc s1 On-Line] |
| VO-14 | Capitalisation: DOS and xBase commands UPPERCASE (COPY, LIST); Pascal reserved words lowercase bold; Initech 123 menu words as on the control panel; menu and dialog text exactly as drawn; heads Title Case except F-BOR and F-MAC (sentence case) | [ibm s9; ate s7; bor s2.2; mac s5.2] |
| VO-15 | Numbers: numerals for keys, sizes and counts; sizes in period form 640K, 1.2MB, 360K; dates in examples are whatever the replayed transcript shows (two-digit years included) | [house: period form; GC-03 makes the transcript the authority] |
| VO-16 | Tense present; never future for shipped behaviour; never mention unbuilt features except inside a DRAFT block | [D-14] |
| VO-17 | No 2026-isms anywhere except the colophon: no URLs, e-mail addresses, "Internet", "web", "click here", "app" (write "application"), "download", years after 1993, modern product names | [CL Law 3; GC-06] |
| VO-18 | Canon is never explained, apologised for or winked at (NT-36). The words "joke", "easter egg", "canon", "intentional", "film" and "movie" never appear in pubs sources outside source comments | [set s0.4; CL Law 4] |

## 10. Completeness rules

A book is **complete** when every row for its type passes. Rows marked **M** are
mechanised by `test-pubs` (GC-10); the rest are checked at review against this table.

| ID | Applies to | Requirement | Check |
|---|---|---|---|
| CP-01 | every book | Cover, title page, verso (AN-20 fields all present), contents, preface (AN-21 sub-heads in order), body, index, colophon (AN-26); binders add Reader's Comment Forms (x3) | M: element presence in the source tree |
| CP-02 | every book | Every chapter carries `% GATES:` naming at least one gate, or is DRAFT | M |
| CP-03 | every book with a glossary | Every first-use bold term has a Glossary entry; every Glossary term is used | M |
| CP-04 | every book | Every cross-reference resolves; every index locator resolves; no "??" | M (LaTeX log) |
| CP-05 | F-IBM Reference | One AN-01 entry per internal command in the shipped command table, per external utility on the shipped image, per batch subcommand, per CONFIG.SYS command; no entry for anything not shipped | M (GC-01) |
| CP-06 | F-IBM Reference | Appendix A has one AN-03 entry per `MSG-DOS-nnnn` in `spec/dos_messages.json`, each with Explanation and Action | M |
| CP-07 | F-IBM Reference | Chapters in model order: Introduction; Files and File Names; Preparing a Fixed Disk; Configuring Your System; Directories; Standard Input and Output; Batch Files; Commands; Messages appendix; Keyboard appendix; Glossary; Index; RCFs | review [ibm s7.1; set s1] |
| CP-08 | F-IBM Technical Reference | One AN-02 entry per INT 21h function the kernel dispatches; disk layout and executable-format chapters cover `.COM` and InitechMZ | M for entries |
| CP-09 | F-OPS | Preface sub-heads; at least one Session with Introduction + Objectives; one AN-15 procedure per operator-visible step sequence the program enforces; Lab Projects; Study Questions with an Answers appendix; Data Collection Sheets; Course Evaluation; RCF (internal) | M for structure; review for coverage |
| CP-10 | F-MAC User's Guide | Part I Learning + Part II Reference; Reference lists every menu and every item of every shipped menu, with keyboard equivalents; troubleshooting; Summary tables; Glossary; Index | M for menus (GC-01) |
| CP-11 | F-ATE Using | Commands chapter: one AN-04 entry per command in SAMIR's shipped command table, symbols first; Functions chapter: one AN-05 per shipped function; Error Messages: one AN-06 per shipped message; appendices for cursor keys, BOF/EOF, `.DBF`/`.DBT` structure (from `spec/samir/dbf_format.h`), ASCII; coercion appendix generated from `spec/samir/xbase_coercion.json` | M |
| CP-12 | F-ATE Learning / Programming | AN-16 frame in every chapter; one running case per section; sample files exist on the shipped sample diskette image | M for files |
| CP-13 | F-ATE set | Master index (X) in both volumes with the boxed key; Notes leaves at section ends | M |
| CP-14 | F-LOT Reference | One chapter per top-level menu item, in menu order, each with the AN-08 opener and menu tree; one AN-09 entry per menu path in `spec/i123/menu_tree.golden` that the build ships; @Functions and Macros chapters with one entry per shipped @function / macro command; Troubleshooting appendix; Glossary; Index | M |
| CP-15 | F-LOT Setting Up volume | Four parts with half-titles; Tutorial lessons numbered across chapters; Quick Start Task Summaries; Sample Applications including the quarterly results worksheet and its pie chart; Master Index; Rating-grid RCF; NOTES leaves | review; M for Master Index codes |
| CP-16 | F-WPC Reference | Getting Started sub-sections (AN-21); one AN-11 entry per shipped feature, inverted names, letter-by-letter order; appendices for codes (from `spec/iword/reveal_codes.h`), file format (`spec/iword/wp5_format.h`), messages, keyboards; Glossary; 3-column Index | M |
| CP-17 | F-WPC Workbook | Lessons in three graded sections; Feature List appendices A (alphabetical) and B (by lesson) consistent with the lesson sources | M for appendix consistency |
| CP-18 | F-BOR set | Introduction with Typefaces section; tutorial chapters separate from reference chapters; Programmer's Guide grammar as railroad diagrams covering every production of ADR-0007 DEC-02; Library Reference one AN-12 entry per shipped RTL routine, A-Z, opening sample entry; error appendix one AN-13 entry per compiler message number; directives one entry each | M for entries and messages |
| CP-19 | F-BOR Self-Compilation Guide | The `K2 == K3` acceptance procedure written as an MIS procedure; every command in it replayed (GC-03) | M |
| CP-20 | every box | Front and back panel zones of PK-08 all filled; In This Package card lists every piece with its part number; every listed piece exists | M for the parts list |

## 11. Correctness and lockstep

### 11.1 Source layout (D-10, D-11)

The toolchain lane (bead `initech-ipu5.5`) has landed the tree; this reference adopts its
layout and names:

```
pubs/
  class/initech-manual.cls      the house class (+ initech-syntax, -screen, -forms .sty)
  manuals/<book-id>/            main.tex, cover.tex, spine.tex, chapter sources,
                                 screens/ (committed captures, re-derived by the gate)
  screens/                      capture scripts and the sample-disk files they use
  build.sh, latexmkrc, pubs.mk  thin shell + make (CL Law 3); TeX external, like nasm
harness/console_text.c          screen decoder (NT-21), C
tools/pubs_msgtable.c           message appendix generator from spec/dos_messages.json, C
```

Still to add under the same tree: `pubs/fonts.lock` (Sec 4.1), `pubs/common/` (imprint,
licence, colophon, address, RCF text shared by all books) and `pubs/denylist.txt` (GC-06).
Chapter sources use the class's semantic macros only (`\Gates{}`, `\draft`, `\key`,
`\type`, `\cmd`, `\ph`, `\opt`, `\rep`, `\choice`, `\file`, environments `CommandEntry`,
`MessageEntry`, `Note`, `Caution`); no raw formatting in chapters [house: the checker parses
semantics, not appearance]. Every chapter opens with a source comment naming its Law-1
sources and a `\Gates{...}` line naming the gates that prove it.

### 11.2 The gate contract (`make test-pubs`)

Landed gates (ipu5.5): `test-pubs-build` (builds, zero overfull boxes, no undefined
references or missing glyphs, ASCII-only, DRAFT count; in `make test` with the shouting
`SKIP_PUBS=1` opt-out), `test-pubs-honesty`, `test-pubs-repro` (+ `-mutant`) = GC-07, and the
emulator gate `test-pubs-screens` = GC-04 for the text route. `test-pubs` is the umbrella
the remaining checks join. Each check is mutation-proven (CL Rule 6).

| ID | Check | Truth source | Proving mutant |
|---|---|---|---|
| GC-01 | **Documented = shipped, both directions.** Every command, switch, CONFIG.SYS directive, menu, menu item, keyboard equivalent, function key binding, @function, xBase command/function, RTL routine and directive named in an entry or `\menu{}`/`\key{}` macro exists in the shipped table; every shipped user-visible item has its entry (CP rows) | InitechDOS: command table in `os/milton/command.c`, `batch.c`, `config_sys.c`, external utilities listed from the built image. FLAIR: `os/flair/shell_menus.h`, `finder_menu.c`, `spec/assets/menu_canon.h`. InitechBase: SAMIR shipped tables (`os/samir/`). Initech 123: `spec/i123/menu_tree.golden` + `os/i123/`. InitechWord: `spec/iword/`, `os/iword/`. Turbo Initech: compiler and RTL tables (`os/tps/`) | rename one documented command; delete one shipped menu item |
| GC-02 | **Messages verbatim.** Every `\msg{}` equals its catalogue entry byte for byte; every catalogue entry is documented | `spec/dos_messages.json`; SAMIR, compiler and application message tables | change one character of a printed message |
| GC-03 | **Examples replayed.** Every `example[trace=...]` is extracted, its typed lines injected into the emulator as a locked trace, and the captured transcript (text-mode buffer or serial) must equal the printed lines exactly, prompt and output included | the built image under the harness | alter one output digit in a printed example |
| GC-04 | **Screens regenerated.** Every `screen[trace=,frame=]` is produced by the harness (record-flair for FLAIR, text screendump otherwise), converted by the fixed NT-21/NT-22 pipeline; the figure's hash is recorded and re-derived every run | harness + locked traces | swap one trace frame index |
| GC-05 | **ASCII-only sources.** No byte outside printable ASCII (0x20-0x7E, plus LF) anywhere in `pubs/`; symbols come from macros | CL Rule 12 | insert a smart quote |
| GC-06 | **No 2026-isms.** Deny-list scan (URLs, e-mail, "Internet", "web", "click here", "app", years 1994 and later, VO-18 words) over all sources except `pubs/common/colophon.tex` | `pubs/denylist.txt` | add "www." to a chapter |
| GC-07 | **Reproducible PDF.** Two `make pubs` runs with `SOURCE_DATE_EPOCH` set from the book's edition date produce byte-identical PDF/X files; no host paths, no build timestamps, deterministic /ID, fonts from `fonts.lock` | CL Rule 11 | leave `\today` in a verso |
| GC-08 | **Versions match the build** (D-3): the version on cover, verso, spine, label and box equals the shipped banner or About string; `0000` blocks a release | `spec/dos_banner.txt`; each product's shipped About string | print 3.31 |
| GC-09 | **Gate citations real.** Every `% GATES:` name is a real Makefile target; a chapter is GATED only if all its gates are green in the same run, else it builds DRAFT | Makefile; the run | cite a non-existent gate |
| GC-10 | **Completeness.** All mechanised CP rows for the book's type | Sec 10 | delete one entry |
| GC-11 | **Canon asserted.** The quarterly pie example sums to 116; the accounting format shows `570-`; the busy pointer is documented as the hourglass; the InitechPaint menu bar string equals `spec/assets/menu_canon.h` | canon specs; CL Law 4 | print 100% |
| GC-12 | **Release has no DRAFT.** `make pubs RELEASE=1` fails if any chapter or entry is DRAFT, any part number carries `0000`, or any check above is red | -- | mark one chapter DRAFT |

### 11.3 Lockstep (the bead rule)

Any change that alters a user-visible surface -- a command name, syntax or switch; a
message; a key binding; a menu, menu item or keyboard equivalent; dialog or screen text in a
documented figure; a documented file format; a version string -- carries a **pubs delta in
the same commit**, or the commit message states `pubs: none (<reason>)`. GC-01..GC-04
catch the drift mechanically either way: a shipped change without the delta turns
`test-pubs` red. A bead that adds a user-visible feature is not closed until its pubs delta
is GATED or explicitly filed as a DRAFT entry in the relevant book.

### 11.4 DRAFT marker (D-14)

A DRAFT chapter or entry prints `DRAFT -- NOT FOR RELEASE` in the running foot of every page
it occupies (Heros Bold 8) and a 40% black diagonal `DRAFT` on its opener; the contents
line carries `(DRAFT)`. DRAFT text may describe the planned behaviour only inside that
marked span, and it is excluded from GC-03 replay but not from GC-05/GC-06.

### 11.5 Conformance delta for the landed class

`initech-manual.cls` (ipu5.5) was built to the superseded [dsy] values. Under this reference
it changes as follows (a follow-up for the toolchain lane):

| Item | Class today ([dsy]) | This reference |
|---|---|---|
| Body face / leading | Schola 10/13, 13 pt baseline | Pagella 10/12 or Termes 10.5/12 by family; 12 pt grid (TY-01, PG-07) |
| Grid | 36 + 6 + 98 = 140 mm; outside 16 mm; binder inside 25 mm | 34 + 4 + 104 = 142 mm; outside 13.8 mm; one 22 mm inside margin (PG-03, PG-05) |
| Key names | Heros Bold caps at 80% | body small capitals (TY-10, NT-10) |
| Cover titles | Termes 30-40 | EB Garamond 40 (TY-18); IBM cover grid (PK-02) |
| Furniture | one foot style; options chapterfolio / continuous | per-family heads, feet and folios (PG-10..PG-17), incl. F-ATE section prefixes and F-IBM `Index-n` |
| Chapter opener | number over a 2 pt rule, 24 pt title | per-family recipes (PG-20..PG-27) |

## 12. The Documentation Set (DS)

Updated from [set s1-s7]: TPS Report Generator manual added; families, anatomies and part
numbers assigned. Extents are planning targets scaled from the model [set s0]. State =
software state of the subject per [set] and `docs/HANDOFF.md`; books are written as phases
land (D-14).

| ID | Book | Family / model | Binding | Anatomies | Target pp. | Part no. | State |
|---|---|---|---|---|---|---|---|
| DOS-REF | InitechDOS Reference | F-IBM / IBM DOS 3.10 Reference | binder, PK-06 dividers | AN-01, 03, 20-25 | 360-440 | 1024-3300-10 | BUILT (commands, batch, CONFIG.SYS, redirection) |
| DOS-UG | InitechDOS User's Guide | F-IBM / IBM DOS User's Guide (MS-DOS 3.3 Guide as structural stand-in [ibm s7.4]) | perfect-bound | AN-20-24; task chapters | 64-96 | 1024-3300-11 | BUILT |
| DOS-TPS | TPS Report Generator Operating Procedures | F-OPS / IBM DOS Operation Training ZR23 | perfect-bound, 3-hole drilled | AN-15, 20, 21, 24, 25 (internal) | 48-80 | 1024-3300-12 | PLANNED (bead `initech-8479.1`, needs FCB `initech-509.9`) -- DRAFT until gated |
| DOS-QR | InitechDOS Quick Reference | booklet | PK-11 | command summary | 16 | 1024-3300-20 | BUILT |
| DOS-TR | InitechDOS Technical Reference | F-IBM / IBM DOS 3.30 Technical Reference | binder | AN-02, 20-25 | 300-400 | (O-1) | PARTIAL |
| FLR-UG | FLAIR User's Guide | F-MAC / Macintosh User's Guide 1991 split, Mac OS 8 grid | perfect-bound | AN-14, 20-24 | 200-260 | 1031-0000-10 | PARTIAL (window-system audit `initech-tdnl`) |
| FLR-GS | Getting Started with FLAIR | F-MAC / Mac OS 8 Installation | saddle-stitched | AN-14 task, troubleshooting | 32-48 | 1031-0000-11 | PARTIAL |
| FLR-ACC | FLAIR Accessories Guide | F-MAC / What's New in System 7 feature topics | perfect-bound | AN-14 | 64-120 | 1031-0000-12 | PLANNED per accessory |
| FLR-QR | FLAIR Quick Reference | card | PK-11 | menu map, shortcuts | 4 panels | 1031-0000-20 | PARTIAL |
| BAS-V1 | InitechBase Volume One: Getting Started, Learning (L), Using (U), Index (X) | F-ATE / dBASE III PLUS Volume One | binder, PK-06 dividers L U X | AN-04-07, 16, 20-25 | 450-600 | 1040-0000-10 | BUILT core (dot prompt, .dbf/.dbt, .ndx, REPLACE/APPEND); BROWSE/EDIT, REPORT/LABEL FORM, @SAY..GET PLANNED |
| BAS-V2 | InitechBase Volume Two: Programming (P), duplicate Index (X) | F-ATE / dBASE III PLUS Volume Two | binder | AN-16, 24, 25 | 250-400 | 1040-0000-11 | BUILT core; sample applications (Initech Accounting System `initech-586.1`; salary rounding procedure `initech-586.2`) |
| BAS-QR | InitechBase Quick Reference Guide | F-ATE QR booklet | PK-11 | syntax only | 32-40 | 1040-0000-20 | follows BAS-V1 |
| 123-SET | Setting Up Initech 123, Tutorial, Quick Start and Sample Applications | F-LOT / Lotus 2.2 Setting Up volume | perfect-bound | AN-17, 20-25 (rating grid) | 360-440 | 1051-0000-11 | PLANNED (P1-P5) |
| 123-REF | Initech 123 Reference | F-LOT / Lotus 2.2 Reference | perfect-bound | AN-08-10, 17 (troubleshooting), 20-24 | 600-720 | 1051-0000-10 | PLANNED |
| 123-QR | Initech 123 Quick Reference + templates | booklet; PK-14 | saddle; die-cut | keys, @functions, menu trees | 40-48 | 1051-0000-20, -21, -22 | PLANNED |
| WRD-REF | InitechWord Reference | F-WPC / WordPerfect 5.1 Reference | binder in slipcase with paper sleeve, PK-07 divider leaves | AN-11, 20-25 | 700-950 | 1062-0000-10 | PLANNED (P1-P4) |
| WRD-WKB | InitechWord Workbook | F-WPC / WordPerfect 5.1 Workbook | perfect-bound | AN-18 | 300-420 | 1062-0000-11 | PLANNED |
| WRD-INS | InitechWord install card + template strip | F-WPC | PK-13, PK-14 | numbered steps | 2 sides | 1062-0000-50, -20 | PLANNED |
| TIN-UG | Turbo Initech User's Guide | F-BOR / TP 6.0 User's Guide | perfect-bound | AN-13 (IDE), 19, 20-24 | 180-260 | 1077-0000-10 | command-line compiler BUILT; IDE PLANNED (M8) |
| TIN-PG | Turbo Initech Programmer's Guide | F-BOR / TP 6.0 Programmer's Guide | perfect-bound | railroad (Sec 5.1), AN-13, 20-24 | 200-320 | 1077-0000-11 | BUILT for the ADR-0007 DEC-02 language |
| TIN-LR | Turbo Initech Library Reference | F-BOR / TP 6.0 Library Reference | perfect-bound | AN-12, 20-24 | 80-160 | 1077-0000-12 | BUILT as far as the RTL boundary (ADR-0007 DEC-05) |
| TIN-SCG | Turbo Initech Self-Compilation Guide | F-BOR (fourth book in place of Turbo Vision Guide) | perfect-bound | AN-12, AN-15-style acceptance procedure, 20-24 | 60-120 | 1077-0000-13 | PLANNED (`make selfhost`) |
| TIN-QR | Turbo Initech Quick Reference + templates | booklet; PK-14 | saddle | editor keys, directives, error numbers | 32-48 | 1077-0000-20 | follows TIN-PG |
| SUP-CSG | Initech Customer Support Guide | F-SUP / Ashton-Tate Customer Support Guide | saddle-stitched | PK-15 | 16-24 | 1000-0000-70 | -- |

Common to every box (PK-12, PK-13): License Agreement envelope (or Turbo Initech licence
statement), Warranty Registration Card, In This Package card, Customer Support Guide, media
exchange coupon, companion-products flyer; Reader's Comment Forms in every binder [set s7].

## 13. Conflicts resolved and open items

### 13.1 Conflicts between sources, and their resolution

| # | Conflict | Resolution | Rule |
|---|---|---|---|
| C-1 | IBM half-letter 5.5 x 8.5 [ibm s2] vs the 7 x 9 application manuals | D-4: 7 x 9 for all; IBM's hanging-head system is mapped onto the house side column (34 mm vs IBM's ~0.8 in indent) | PG-03, PG-05 |
| C-2 | Side column mirrored or fixed? Microsoft mirrors to the outer edge [ibm s5.3]; Ashton-Tate, IBM, WordPerfect, Borland keep it left | Fixed left (four of five models; the Microsoft book is not a model) | PG-06 |
| C-3 | Cover family band at 62% height, full bleed [dsy s6] vs IBM band at 24-27%, bleeding left and stopping at 0.84-0.86 of the width [ibm s3; pkg s2] | IBM measured grid wins (annex over proposal); spine band follows the cover band | PK-02, PK-03, PK-08 |
| C-4 | Book cover ground: IBM solid corporate blue [ibm s3] vs white [dsy s6] | White with the family band (IBM's own title-page form); putty for binders and boxes | PK-02, PK-04 |
| C-5 | Box back columns "Software Required / Package Contents / System Requirements" [dsy s7] vs IBM's fixed order Software required / Software included / System requirements / Options supported, licence on the right half [pkg s3] | IBM order and licence placement; contents go on the In This Package card | PK-08 |
| C-6 | Body face: New Century Schoolbook [dsy s4] vs the annexes (Palatino: Lotus, Borland; Times: IBM, WordPerfect) | Two body faces by family; Schola dropped. Survey's "Century-style" IBM identification is corrected by [ibm s10] | TY table |
| C-7 | Ashton-Tate "Notes pages at chapter ends" and "key names in blue" [sur s1] vs [ate s13]: section ends only; key names black, typed input blue | Annex governs; under D-6 typed input is Cursor Bold | PG-29, NT-15 |
| C-8 | Two-colour models (Ashton-Tate, WordPerfect, Apple) vs D-6 | Spot elements become black rules and 10%/40% black tints; WordPerfect divider bars, being tabs, keep a colour | NT-22, NT-23, PK-07 |
| C-9 | Thumb tabs: IBM stepped section tabs, Borland stepped letter tabs, WordPerfect unstepped bars, Ashton-Tate pictogram marks | Each family keeps its model's device (presentation directive); binder cut-tab dividers are a line-wide addition | PG-30-PG-33, PK-06 |
| C-10 | Screens: Borland redrew screens as line art [bor s2.3]; IBM printed none [ibm s9] | Operator directive and CL Rule 14: real emulator captures always; IBM reference keeps transcripts instead of pictures | NT-20 |
| C-11 | Apple model era: 1991 landscape two-column [mac s5.2] vs 1997 Platinum 7.5 x 9 grid [mac s5.4] | Grid from 1997 (fits D-4; FLAIR base era is Mac OS 8 Platinum, ADR-0004 DEC-10); Learning/Reference split from 1991 | F-MAC |
| C-12 | Notice devices differ in every model | One three-level meaning line-wide (NT-30), placement per family (NT-31) | NT-30, NT-31 |
| C-13 | Quick Reference sizes: Ashton-Tate 3.83 x 8.33, Lotus about 4 x 9, [dsy s11] 7 x 9 folded | 3.5 x 9 in booklet (halved house trim); D-4 binds books, not booklets | PK-11 |
| C-14 | Print codes: IBM edition ordinal only; Ashton-Tate month/year; WordPerfect type-market code + month/year; Borland revision + printer's key; Apple none | One Initech code combining edition letter, printing digit and month/year, with IBM ordinal and Borland key on the verso | PN-05, PN-06 |
| C-15 | Colophon honesty (D-15) vs no 2026-isms (CL Law 3) | The colophon is the single exempt page, held in one file | AN-26, GC-06 |
| C-16 | Lotus 2.2 has no error appendix [lot s6]; other models do | Each product follows its model: Initech 123 uses Problem/Solution | AN-17, CP-14 |

### 13.2 Open items (do not block typesetting)

| # | Item | Owner |
|---|---|---|
| O-1 | InitechDOS Technical Reference was a separately sold product [ibm s1; set s1], but D-2 fixes six boxes: decide whether it ships unboxed, inside the Turbo Initech box, or waits | PM |
| O-2 | The F-ATE chapter pictogram set and the F-OPS / F-BOR side-column icons (keyboard, mouse, arrow) are original drawings still to be made; until then the opener square is left solid black without a drawing | artwork |
| O-3 | Cursor at 9 pt may proof too light; Nimbus Mono PS is the named fallback (TY table) | first specimen print |
| O-4 | All [MEAS]/[INF] physical figures (box sizes, label stock, tab projection) pending D-13 | PM |
| O-5 | The fictional address string and Business Reply permit number are fixed once in `pubs/common/` and checked not to collide with a real business (D-12) | publications |
