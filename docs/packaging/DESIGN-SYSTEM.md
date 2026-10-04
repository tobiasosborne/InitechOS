<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Initech Systems Corporation -- Publications and Packaging Design System (Proposal)

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (supporting file to `MANUALS-AND-BOXES-RESEARCH.md`)
**Programme:** STAPLER -- Physical Edition (epic `initech-gtge`, task `initech-gtge.1`)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-RES-0001-C |
| Version | 1.0 (proposal; values marked "approx." must be proofed physically) |
| Date | 2026-10-04 |
| Parent | `docs/packaging/MANUALS-AND-BOXES-RESEARCH.md` |
| Evidence | `ORIGINALS-PHYSICAL-SURVEY.md` (what each element is modelled on) |

---

## 1. Principles

1. **One line, one look.** Every Initech product sits in the same box and binder system and is
   told apart by its title and a single family colour. The model is IBM's Personal Computer
   Software line (common slipcase, coloured family band, fixed positions for logo, title,
   media legend and part number) -- the only original surveyed that ran a true line system.
2. **Corporate blandness, done rigorously** (HANDOFF Sec 2 "Design stance"). No illustration
   style that dates the line to a single product; no jokes; generous white space; small type
   for legal text; every item numbered.
3. **Period means of production.** A 1990-93 corporate publications department set type on
   a PostScript imagesetter or laser printer with the standard PostScript 35 faces (Borland
   says so in its colophon). The system uses only those faces (or metric clones), simple
   rules, flat spot-colour bands and halftoned screen captures.
4. **Colour is spent outside, not inside.** Interiors are black only (IBM, Borland, Lotus);
   colour appears on covers, spines, tab dividers, boxes, labels and cards. This is period
   practice and also what keeps short-run digital printing cheap (see the main report).
5. **Same grid everywhere.** One trim (7 x 9 in), one text grid, one spine system, two box
   sizes. Shelf alignment is a design requirement, not a nicety.

## 2. Identity

| Element | Specification | Grounding |
|---|---|---|
| Company name | "Initech Systems Corporation" in full on title versos, boxes and cards; "Initech" alone in product names | `README.md`, `spec/dos_banner.txt` |
| Product names | InitechDOS, FLAIR, InitechBase, Initech 123, InitechWord, Turbo Initech, plus InitechCalc, InitechPaint, File Manager, FILE COPY | `README.md`, epics |
| Trademark marks | "InitechDOS(TM)", "Turbo Initech(R)" on first use per document and on boxes (README already assigns TM and R) | `README.md` trademark paragraph |
| Logo | Open decision D-8. Constraint: original artwork only -- not the film's logo, not IBM's striped logotype, not Ashton-Tate's stripes. Recommended direction: a wordmark "INITECH" in Helvetica Bold Extended-style capitals, letterspaced +100, with "Systems Corporation" in Helvetica Regular below a 0.5 pt rule. Bland is correct | IBM, Ashton-Tate wordmark-plus-rule pattern |
| Address and telephone | Open decision D-12. Use a fictional street address that does not belong to a real business, and telephone numbers in the 555-0100 to 555-0199 range reserved for fiction. Keep "Technical Support: extension 2504" from the README | README |
| Copyright years | 1991-1993 range (banner 1991; README 1992, 1993) | repo |

## 3. Colour

All Pantone references are approximate visual matches from screen values; confirm against a
physical Pantone Formula Guide (coated and uncoated) before any spot-colour job. For digital
(CMYK) printing use the CMYK values and proof once.

| Name | Use | CMYK (approx.) | Pantone (approx.) | Grounding |
|---|---|---|---|---|
| Initech Black | text, rules, interiors | 0/0/0/100 (text); rich black 40/30/30/100 for large solids | Black C | all originals |
| Initech Putty | box and slipcase ground, binder vinyl | 0/3/12/10 | 7527 C | period PC case beige; IBM's warm grey slipcases |
| Desktop Teal | accent on FLAIR material only; screen-capture duotone | 36/0/0/14 | 324 C | the FLAIR desktop `#8DDCDC` (PRD Sec 0, WL-0053) |
| Systems family (InitechDOS, FLAIR) | family band, spine band, tabs | 100/0/45/35 | 3292 C | IBM family bands |
| Information Management family (InitechBase) | as above | 100/67/0/23 | 288 C | Ashton-Tate blue |
| Decision Support family (Initech 123) | as above | 90/12/95/40 | 349 C | -- |
| Office Systems family (InitechWord) | as above | 0/100/63/29 | 201 C | -- |
| Programming family (Turbo Initech) | as above | 90/100/0/15 | 2627 C | IBM "Productivity Series" purple |
| Rule grey | hairlines, table rules, Reader's Comment Form | 0/0/0/50 | Cool Gray 8 C | -- |

Interior colour: none. If the operator chooses two-colour interiors (decision D-6), the second
colour of a manual is its family colour, used only for running-head rules, side heads and
key names (the Ashton-Tate and WordPerfect pattern).

## 4. Typefaces

The PostScript 35 set, as a 1991 publications department would have had it. Free, metric-
compatible clones are listed for production; the commercial originals are licensable if the
operator prefers (Linotype/Monotype desktop licences; prices not checked).

| Role | Period face | Free equivalent (licence) | Setting |
|---|---|---|---|
| Body text | New Century Schoolbook | TeX Gyre Schola (GUST Font License) or URW C059 (base35) | 10/13 pt, ragged right, no hyphenation of command names |
| Chapter title | Helvetica Bold | TeX Gyre Heros Bold | 24/28 pt |
| Section heads (H1/H2) | Helvetica Bold | TeX Gyre Heros Bold | 13/16 pt and 10.5/13 pt |
| Side heads, tab labels, table heads | Helvetica Condensed Bold | TeX Gyre Heros Cn Bold or Nimbus Sans Narrow Bold | 9/11 pt |
| Running foot, folio, captions | Helvetica | TeX Gyre Heros | 8/10 pt |
| Typed input and screen text | Courier | TeX Gyre Cursor or Nimbus Mono PS | 9/12 pt |
| Key names | Helvetica Bold small capitals | TeX Gyre Heros Bold, faked small caps at 80% | inline, e.g. ENTER, F1 (the Lotus convention "HELP (F1)") |
| Cover and box titles | Times Roman | TeX Gyre Termes | 30-40 pt on covers; IBM boxes used a Times-like serif |
| Legal text | Helvetica | TeX Gyre Heros | 6.5/8 pt |

Rules: 0.5 pt black under running heads and over the running foot; 0.25 pt grey in tables.
No italics for emphasis in procedures; italics only for document titles and placeholders.

## 5. Page grid (manuals)

| Item | Value |
|---|---|
| Trim | 7 x 9 in (177.8 x 228.6 mm). Matches Lotus, Borland, Ashton-Tate (inferred) and Apple; see the survey |
| Margins, perfect-bound | inside 22 mm, outside 16 mm, top 19 mm, bottom 22 mm |
| Margins, binder pages | inside 25 mm (holes), others as above |
| Columns | side-head column 36 mm, gap 6 mm, text column 98 mm (perfect-bound) or 95 mm (binder) |
| Baseline grid | 13 pt |
| Running head | none on text pages (IBM, Lotus); the chapter title appears in the running foot |
| Running foot | outside: chapter-page folio "4-26"; inside: chapter title (Lotus "4-26 The Data Commands") |
| Folios | chapter-page for reference manuals; continuous arabic for tutorials and the Turbo Initech books (Borland) |
| Chapter opener | recto; chapter number "Chapter 4" in Helvetica Bold 10 pt over a 2 pt black rule; title 24 pt; a short "What This Chapter Covers" list (Ashton-Tate pattern) |
| Notes, cautions | run-in "NOTE" / "CAUTION" in Helvetica Bold caps (Lotus) |
| Figures | screen captures from the emulator at 1:1 pixels, printed at 150-200 ppi equivalent as greyscale halftones, with a 0.5 pt frame; caption below in 8 pt |
| Tabs | binder manuals: separate tab dividers (see 8); perfect-bound reference books: printed bleed tabs 9 mm deep x 30 mm tall on section openers only (WordPerfect), black with reversed Helvetica Condensed Bold caps |
| Back matter | Glossary, Index (two columns, 8.5/11 pt), Reader's Comment Form and Business Reply panel in every binder manual (IBM) |

## 6. Covers, title inserts and spines

**Cover layout (books and binder covers; all measurements from trim, 7 x 9 in page):**

- Top left, 19 mm from top: product name in Times Roman 36 pt ("InitechDOS"), then version in
  Times Roman 18 pt ("Version 3.30").
- Below at 70 mm: document title in Helvetica 14 pt ("Reference").
- **Family band** at 62% of the height (centre 142 mm from top), 10 mm deep, full bleed, in
  the family colour, carrying the family name reversed in Helvetica Condensed Bold 8 pt
  ("Systems Family"). Same height on every cover in the line.
- Lower left: Initech wordmark, 30 mm wide, 16 mm from the bottom and the left edge.
- Lower right: part number in Helvetica 7 pt ("1024-3300-11").
- Ground: white for books and title inserts; Initech Putty for binders and boxes.
- No illustration on manuals. (Boxes may carry one restrained product photograph or a
  screen capture in the area between the title and the band; IBM did this.)

**Spines:** text reads top to bottom (US convention; tell German printers explicitly, the
German convention is bottom to top). From the top: wordmark (horizontal if the spine is at
least 25 mm, otherwise rotated), product name, document title, the **family band at exactly
the same height as on the cover** so that a shelf of Initech books shows one continuous
stripe per family, version, and the part number at the foot. Minimum spine for text: 6 mm
(about 120 sheets at 80 g/m2; below that, spine left blank as Borland's early books were).

**Binder covers:** putty vinyl or printed coated board over board, one colour (black) plus the
family colour band, screen-printed or digitally printed; same layout as the book covers; spine
label printed directly or in a spine pocket (period binders used both; decision D-5).

## 7. Boxes and slipcases

| Item | Proposal | Grounding |
|---|---|---|
| Standard box | outer about 245 x 210 x 70 mm (9 5/8 x 8 1/4 x 2 3/4 in): one binder (38 mm rings) or two or three perfect-bound books | IBM DOS 2.10 box 9 1/2 x 8 x 2 1/4 in (Smithsonian) |
| Large box | outer about 245 x 210 x 140 mm (9 5/8 x 8 1/4 x 5 1/2 in): two binders (InitechBase) or four books (Turbo Initech) | dBASE III PLUS box 9 5/8 x 9 1/16 x 5 3/8 in (Smithsonian) |
| Construction | rigid slipcase open on one long side (IBM, Ashton-Tate, WordPerfect) holding binder(s)/books and a diskette envelope; or a two-piece lid-and-base box for perfect-bound products. Final inner dimensions follow the sourced binder; these are the planning envelope | survey Sec 7 |
| Paper sleeve | a printed paper sleeve (wrap) around the slipcase carries the product-specific front and back panels; the slipcase itself can be identical across the line. Period-correct (WordPerfect and IBM sold "in slipcase with paper sleeve") and the cheapest way to make product-specific boxes in tiny runs | emsps.com listings |
| Front panel | as the cover layout, plus top-right media legend "5.25-inch diskettes" in Helvetica 9 pt (IBM) and one line of product description lower right | IBM packages |
| Back panel | three columns: "Software Required", "Package Contents", "System Requirements" (486 or 386 processor, memory, display, diskette drive -- as in the README); a short license notice; copyright line; "Printed in ..."; part number; optional bar code (see 10) | IBM packages; README requirements |
| Side panel | product name and family band (spine logic); Initech 123 box carries the fill-in **Hardware Chart** (Lotus) | Lotus Setting Up p. 1-6 |
| Shrink-wrap | yes, as all originals | MEM |

## 8. Tab dividers (binder manuals)

Cut-tab dividers on 200-250 g/m2 board, half-size three-hole drilled (70 mm spacing), tabs in
the family colour with section names reversed in Helvetica Condensed Bold 9 pt, tab positions
stepped down the edge in order; the divider face carries the section title and a short
contents list. InitechBase uses section letters L, U, P on tabs to match its folio prefixes.

## 9. Diskette labels, sleeves and envelopes

| Item | Specification | Grounding |
|---|---|---|
| 5.25-in label | top band in the family colour with "INITECH INITECH INITECH" repeated in reversed Helvetica Bold 6 pt caps (WordPerfect); disk title in Times Roman 14 pt ("Distribution Diskette 1"); product and version in Helvetica 8 pt; "Disk 1 of 4"; copyright line 5 pt; a white panel for the per-copy serial number. Label size to be set from purchased blank stock (about 105 x 38 mm **[INF]**) | WordPerfect and Borland labels |
| 3.5-in label | same content; front panel about 70 x 54 mm with or without top wrap -- measure the stock **[INF]** | WordPerfect 3.5-in labels |
| Serial number | impact- or laser-printed per copy in Courier 9 pt, format "IS1024-33-000127" (product, version, sequence) | Borland per-copy serials |
| Write protection | 5.25-in masters write-protected with tabs (period practice; notchless jackets are not obtainable) | MEM |
| Sleeve | white paper sleeve; back printed with the six handling pictograms and temperature range in four languages (English, German, French, Dutch) | the MCN sleeve scan |
| License Agreement envelope | putty envelope holding the diskettes; front: "INITECH SYSTEMS CORPORATION LICENSE AGREEMENT -- Read before opening"; full license text on the back in 6.5 pt; a seal sticker across the flap | IBM, Ashton-Tate, WordPerfect |

## 10. Part numbers, print codes and legal conventions

**Part numbers** extend the README's `1024-3300-01` pattern: `PPPP-VVVV-II`.

- `PPPP` product: 1024 InitechDOS / InitechOS (existing), and proposals 1031 FLAIR,
  1040 InitechBase, 1051 Initech 123, 1062 InitechWord, 1077 Turbo Initech.
- `VVVV` version without the point: 3300 = 3.30.
- `II` item: 01 complete package; 10-19 manuals; 20-29 quick references and templates;
  30-39 5.25-in diskettes; 40-49 3.5-in diskettes; 50-59 cards; 60 license envelope;
  70 Customer Support Guide; 80-89 inserts and flyers.

**Edition notice and print code** on every title verso (IBM edition notice plus a printer's
key as Borland and Lotus used):

    First Edition (March 1992)
    This edition applies to Version 3.30 of InitechDOS and to all subsequent releases
    until otherwise indicated in new editions.
    (c) Copyright Initech Systems Corporation 1991, 1992. All rights reserved.
    Printed in Germany.            1024-3300-11   10 9 8 7 6 5 4 3 2 1

"Printed in Germany" is literally true if printed there and period-plausible (Lotus printed
its US-English manuals in Ireland). All boilerplate -- license, warranty, disclaimers,
trademark paragraph -- must be newly written for Initech; do not reuse the originals' wording.

**Bar codes:** if used, a UPC-A in a restricted-circulation number system (prefix 2 or 4,
in-store use) so it can never collide with a real product's GTIN.

## 11. Cards and templates

| Item | Size | Content |
|---|---|---|
| Warranty Registration Card | 6 x 4.25 in postcard, perforated stub | name, company, serial, where purchased, "number of PCs in your organization"; business-reply back |
| In This Package card | 7 x 9 in | checklist with part numbers |
| Quick Reference card | 7 x 9 in, folded to 3.5 x 9, or the tall booklet | commands, keys |
| Keyboard templates | die-cut 0.4 mm card: a 101-key strip (WordPerfect strip model) and an 84-key L-shaped template (F-keys at the left) for Initech 123, InitechWord, Turbo Initech | WordPerfect, Lotus |
| Media exchange coupon | 6 x 4.25 in | "To receive 3.5-inch diskettes, complete and return this coupon" |
| Reader's Comment Form | 7 x 9 in, bound into binders | IBM model, with Business Reply Mail panel |
