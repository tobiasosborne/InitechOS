<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Manuals and Boxes -- Research Report for the Physical Edition

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Report (input to an operator decision; not a ratified decision)
**Programme:** STAPLER -- Physical Edition (epic `initech-gtge`, task `initech-gtge.1`)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-RES-0001 |
| Title | Manuals and Boxes -- Research Report for the Physical Edition |
| Version | 1.0 |
| Date | 2026-10-04 |
| Status | RESEARCH COMPLETE -- awaiting operator decisions (Sec 9) |
| Related Issues | beads `initech-gtge` (epic), `initech-gtge.1` (this task); `initech-68iw` (Initech 123), `initech-fdxa` (InitechWord), `initech-586` (InitechBase), `initech-s25` (PC LOAD LETTER screen) |
| Annexes | `ORIGINALS-PHYSICAL-SURVEY.md` (A), `INITECH-DOCUMENTATION-SET.md` (B), `DESIGN-SYSTEM.md` (C), `PRODUCTION-SOURCING.md` (D) -- all in `docs/packaging/` |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/` with manifest `SOURCES.md` (outside the public repo; copyrighted scans) |
| Assumption | Owner in Germany/EU; unconfirmed (decision D-1) |

---

## 1. Summary

The physical edition is feasible at 1, 10 and 50 copies with suppliers in Germany, with two
exceptions that need quotes or a bookbinder: **US-style three-ring binders** and **custom rigid
slipcases/boxes** at quantities below about 25. Books, cards, labels and diskettes are cheap
and available.

**The three most important findings**

1. **The complete original documentation sets are now held locally** -- including the full
   Ashton-Tate dBASE III PLUS printed set (Learning, Using, Programming, Quick Reference,
   Customer Support Guide and inserts), which `../dbase3-decomp/SOURCES.md` had recorded as
   unobtainable (archive.org lending-only). Lotus 1-2-3 R2.2 (three manuals), WordPerfect 5.1
   (Reference and Workbook), Turbo Pascal 6.0 (all four books), IBM DOS 3.x binders, MS-DOS 3.3,
   and Apple System 6/7 guides are also held, plus disk-label and package scans. 65 files,
   1.10 GB. (The dBASE set is also useful for SAMIR's Law-1 grounding.)
2. **The originals share two physical formats.** Application manuals are about **7 x 9 in**
   (Lotus, Borland, Ashton-Tate, Apple), perfect-bound for tutorials and language books;
   reference manuals that get consulted and updated live in **three-ring binders in slipcases**
   (IBM, Ashton-Tate, WordPerfect). I measured the IBM DOS binder pages at 5.5 x 8.5 in with
   three holes at **2.75 in (70 mm)** spacing. That US half-size mechanism is not an EU stock
   item, which makes binders the production bottleneck.
3. **Black-only interiors are both authentic and the main cost lever.** IBM, Borland and Lotus
   printed interiors in black only; colour went on covers, tabs and boxes. In digital short-run
   printing any interior colour is charged at the colour rate, so black-only books cost about
   10-20 EUR each even at one copy, and myBuchdruck (Rosbach, DE) prints any trim between A6
   and 30 x 30 cm, so 7 x 9 in is possible. Real 5.25-in media is still sold (about 1.50-2.50
   USD per disk new), but the OS images are 1.44 MB, so a 1.2 MB multi-diskette installer is
   software work.

**Recommendation in one paragraph.** Adopt one Initech line system modelled on IBM's Personal
Computer Software packages (putty slipcase or box, a colour band per product family, fixed
logo/title/part-number positions), one trim (7 x 9 in) for every manual, the PostScript 35
typefaces via free metric clones, black-only interiors. Build a LuaLaTeX house class fed from
plain-text sources that cite the gates proving each chapter. Pilot with **InitechDOS 3.30**
(the most complete software, and its package exercises the hardest chain: binder, dividers,
slipcase, 5.25-in diskettes). One complete copy first, then 10, then 50.

## 2. The originals, acquired

Full manifest with URL, edition, page count, size and sha256 per file:
`/home/tobias/Projects/initech-manuals-ref/SOURCES.md`. Summary:

| Product | Held (local directory) | Pages (PDF) | Source |
|---|---|---|---|
| dBASE III PLUS 1.1 (1986) | Learning, Using, Programming, Quick Reference, Customer Support Guide, Technical Support, 3 flyers, insert; media scans; a (non-original) disk sleeve -- `dbase3plus/` | 244 / 596 / 612 / 36 / 22 / 8 | WinWorld (366.9 MB .7z), archive.org |
| Lotus 1-2-3 R2.2 (1989) | Reference; Setting Up + Tutorial + Quick Start + Sample Applications; Upgrader's Handbook; R3 Reference for comparison -- `lotus123/` | 700 / 436 / 76 / 672 | WinWorld, archive.org |
| WordPerfect 5.1 (1989) | Reference (6/92 printing); Workbook; function-key strip; 3.5-in disk labels and install card at 600 dpi -- `wordperfect51/` | 1048 / 512 | mendelson.org, WinWorld, archive.org |
| Turbo Pascal 6.0 (1990) | User's Guide, Programmer's Guide, Library Reference, Turbo Vision Guide; 5.0/5.5 manuals; brochures; 6.0 disk labels -- `turbopascal/` | 271 / 382 / 184 / 427 | bitsavers, archive.org |
| IBM PC DOS 3.x / MS-DOS 3.3 | IBM DOS 3.10 Reference, 3.30 Technical Reference, 4.00 Reference, PC DOS training booklet; Microsoft MS-DOS 3.3 User's Guide -- `dos33/` | 546 / 546 / 333 / 77 / 471 | bitsavers |
| Apple System 6/7, Mac OS 8 | What's New in System 7; Macintosh User's Guide (desktop, Z030-1751-A); System Software User's Guide 6.0; two Mac OS 8 install manuals -- `macos/` | 40 / 325 / 296 / 52 / 52 | archive.org |
| Packaging line reference | IBM package photos (DisplayWrite 4, PC Network Program, PC 3270, SCRIPT/PC, Personal Communications/3270); DisplayWrite 4 Getting Started -- `packaging/` | 68 | bitsavers |

Not obtained: the IBM DOS **3.30 Reference** itself (3.10 and 4.00 Reference and the 3.30
Technical Reference stand in -- same series, same binder); dimensioned photos of the Lotus 2.2,
Turbo Pascal 6.0 and WordPerfect 5.1 boxes; a System 7 retail box scan. Large items recorded but
not downloaded are listed in `SOURCES.md`.

## 3. How the originals were physically made

Details, with a provenance tag on every fact, are in Annex A (`ORIGINALS-PHYSICAL-SURVEY.md`).
The essentials:

| Original | Documentation split | Binding | Trim | Interior | Box / package |
|---|---|---|---|---|---|
| dBASE III PLUS | Learning + Using + Programming in **two volumes**; Quick Reference (tall booklet); Customer Support Guide; flyers | 2 binders in a **vinyl-covered slipcase** [READ: emsps; Smithsonian] | ~7 x 9 in [INF] | black + blue; hanging side heads; "UI-3" folios [READ] | 9 5/8 x 9 1/16 x 5 3/8 in, 7 x 5.25-in disks [READ: Smithsonian] |
| Lotus 1-2-3 R2.2 | Setting Up/Tutorial/Quick Start/Sample Applications (one volume); Reference; Upgrader's Handbook; Quick Reference; templates; Customer Assurance Plan; Hardware Chart on the box [READ] | perfect-bound [INF/MEM] | ~7 x 9 in [MEAS] | black only; "4-26" folios [READ] | dims not found; "Printed in Ireland" [READ] |
| WordPerfect 5.1 | Reference (alphabetical); Workbook; templates; install card | Reference **ring binder in slipcase with paper sleeve** [READ: CCH; emsps] | ~7.9 x 8.9 in [MEAS, verify] | black + blue; bleed-tab dividers [READ] | 9.5 x 8.3 x 4 in [READ: listing, weak] |
| Turbo Pascal 6.0 | User's Guide, Programmer's Guide, Library Reference, Turbo Vision Guide; license statement; RTL source order form; registration card [READ] | perfect-bound [MEM] | ~7 x 9 in [MEAS] | black only; typeset in-house with Borland Sprint on a PostScript printer [READ: colophon] | dims not found |
| IBM PC DOS 3.30 | Reference (binder); User's Guide; Technical Reference sold separately [READ: emsps] | **3-ring binder in slipcase**; holes 2.75 in apart [MEAS] | 5.5 x 8.5 in [MEAS] | black only; bleed thumb tabs; Reader's Comment Forms [READ] | DOS 2.10 box 9 1/2 x 8 x 2 1/4 in [READ: Smithsonian] |
| Apple System 7 | What's New, How to Install, Macintosh Reference, Networking Reference, HyperCard Basics; 12 disks [READ: listing summary] | perfect-bound [MEM] | ~7.25 x 8.9 in [MEAS] | Apple Garamond; coloured thumb tabs [READ] | not found |

Cross-cutting: every box carried the same paper kit (license envelope or statement,
registration card, quick reference, templates, support guide, flyers); every item carried a
part or print code; IBM alone ran a true cross-product line system (common slipcase, family
colour bands, fixed positions) -- Annex A Sec 7.

## 4. The Initech documentation set

Full outlines, chapter by chapter, with the software state of each chapter marked
[BUILT]/[PARTIAL]/[PLANNED], are in Annex B (`INITECH-DOCUMENTATION-SET.md`). Summary:

| Product (model) | Manuals | Binding | Est. pages | Box |
|---|---|---|---|---|
| **InitechDOS 3.30** (IBM DOS 3.30) | Reference; User's Guide; Quick Reference; (Technical Reference as a separate Programming-family product) | Reference in 3-ring binder + dividers; User's Guide perfect-bound | 360-440 / 64-96 | standard slipcase box |
| **FLAIR** (Apple System 7 kit) | User's Guide; Getting Started; Accessories Guide; Quick Reference card | perfect-bound / saddle | 200-260 / 32-48 / 64-120 | standard box |
| **InitechBase** (dBASE III PLUS) | Volume One: Learning + Using; Volume Two: Programming + duplicate Index; Quick Reference Guide | two 3-ring binders; tall booklet | 450-600 / 250-400 / 32-40 | large slipcase box |
| **Initech 123** (Lotus 1-2-3 R2.2) | Setting Up + Tutorial + Quick Start + Sample Applications; Reference; Quick Reference; two keyboard templates | perfect-bound | 360-440 / 600-720 / 40-48 | standard box with Hardware Chart side panel |
| **InitechWord** (WordPerfect 5.1) | Reference; Workbook; templates; install card | Reference in binder with slipcase and paper sleeve; Workbook perfect-bound | 700-950 / 300-420 | standard slipcase box + sleeve |
| **Turbo Initech** (Turbo Pascal 6.0) | User's Guide; Programmer's Guide; Library Reference; Self-Compilation Guide; Quick Reference | perfect-bound | 180-260 / 200-320 / 80-160 / 60-120 | large box |

Common to every box: diskettes sealed in a License Agreement envelope, Warranty Registration
Card, "In This Package" card, the Initech Customer Support Guide (one booklet for the line),
media exchange coupon, companion-products flyer; Reader's Comment Forms in every binder.

Two rules govern the writing (Annex B Sec 0): **document only what the gates prove** (each
chapter cites its gates; unbuilt chapters wait), and **generate reference tables from locked
spec-data** (`spec/dos_messages.json`, `spec/samir/*`, menu definitions) so the manual cannot
drift. Canon (116%, `570-`, two-digit years, hourglass, `PC LOAD LETTER`) appears as ordinary
documented behaviour, never explained.

## 5. Unified design system

Specified in Annex C (`DESIGN-SYSTEM.md`) to the level a designer can build templates from.
The key decisions, and which original each comes from:

| Element | Proposal | Taken from |
|---|---|---|
| Line system | common putty slipcase/box, colour band per product family at a fixed height on cover, spine and box, fixed logo/title/media-legend/part-number positions, back panel with Software Required / Package Contents / System Requirements columns | IBM Personal Computer Software packages |
| Families and colours | Systems (InitechDOS, FLAIR) teal, Information Management (InitechBase) navy, Decision Support (Initech 123) green, Office Systems (InitechWord) burgundy, Programming (Turbo Initech) purple; Initech Putty ground; Desktop Teal (`#8DDCDC`) as a FLAIR accent | IBM family bands; the repo's desktop canon |
| Typefaces | New Century Schoolbook body, Helvetica / Helvetica Condensed heads and side heads, Courier for typed text, Times Roman for cover titles -- all PostScript 35; free clones TeX Gyre Schola/Heros/Heros Cn/Cursor/Termes or URW base35 | IBM (Century-style text), Borland colophon (PostScript faces), IBM boxes (Times-like titles) |
| Page | 7 x 9 in, hanging side-head column, chapter-page folios with chapter title in the running foot, run-in NOTE/CAUTION | Lotus, Borland, Ashton-Tate; IBM; Lotus |
| Spines | text top-to-bottom; family band aligned across the line so the shelf shows one stripe per family | IBM line logic |
| Binders | putty, one colour plus family band; half-size 3-ring mechanism, 70 mm spacing; cut-tab dividers in the family colour | IBM, Ashton-Tate, WordPerfect |
| Disk labels | family-colour top band with "INITECH" repeated in reversed small caps; disk title in a serif; per-copy serial number in Courier | WordPerfect labels; Borland per-copy serials |
| Cards and envelopes | License Agreement envelope sealing the diskettes; registration postcard; In This Package card; Reader's Comment Form; keyboard templates (101-key strip and 84-key L) | IBM, Ashton-Tate, Lotus, WordPerfect |
| Numbers and legal | part numbers `PPPP-VVVV-II` extending the README's `1024-3300-01`; IBM-style edition notice plus printer's key; "Printed in Germany" if printed there; all boilerplate newly written; restricted-circulation UPC if any bar code | README; IBM; Borland/Lotus; Lotus "Printed in Ireland" |
| House style source | the header conventions of `docs/HANDOFF.md` and the ADRs (controlled-document notice, document control tables, issuing bodies, revision codes) carry over to title versos, edition notices and the support guide | repo |

## 6. Production

Supplier tables with URL, minimum, price as read, lead time, file requirements and the date
checked (all 2026-10-04) are in Annex D (`PRODUCTION-SOURCING.md`). Summary:

| Item | 1 copy | 10 copies | 50 copies | Real price found? |
|---|---|---|---|---|
| Perfect-bound books, 7 x 9 in | myBuchdruck (any trim A6-30 x 30 cm, from 1) or a copy shop | myBuchdruck / podbuchdruck | same, or an offset quote | partly: podbuchdruck 100 pp 5.66 EUR at 5; epubli A5 80 pp 7.77 EUR at 1; dbusiness binding 7.50-9.50 EUR. Our configuration: ESTIMATE 9-22 EUR per book |
| Binder pages (printed, cut, 3 holes at 70 mm) | copy shop with a paper drill | copy shop | copy shop | no |
| Ring binders, US half-size 3-ring | Buchbinderei, or Schlender with customer-supplied mechanism | same | Vervante (min 25) or mybinding (min 50) in the US | no -- all quote-only |
| Slipcases | Buchbinderei | Buchbinderei / easyOrdner | madika custom quote | no (print24 makes A4-only slipcases from 1, no price) |
| Boxes | digital print + plotter cut, or Buchbinderei | madika (custom, from 1) | madika | partly: madika lid boxes 3.24 EUR at 100 (standard sizes); custom large box needs a quote |
| Tab dividers | copy shop + hand-cut, or easyOrdner | easyOrdner / viaprinto | same | headline only: easyOrdner "from 2.59 EUR per piece" |
| 5.25-in diskettes | FloppyDisk.com (US) or poly.play (DE, NOS) | same | same | yes: 1.2 MB 10-pack 14.95-19.95 USD; 360 K 10-pack 19.95-24.95 USD |
| Disk writing | Greaseweazle V4.1 (27.90 EUR, per search summary) + a 5.25-in drive | same | or FloppyDisk.com duplication 1.99 USD/disk at 100 (+50 USD setup) | yes |
| Disk labels | label stock + laser printer | same | FloppyDisk.com 0.19-0.29 USD each (+50 USD setup under 500) | yes |

**Cost envelope (ESTIMATE, Annex D Sec 9):** a perfect-bound product package about 90-160 EUR
for one copy, 50-90 EUR each at 10, 30-55 EUR each at 50; a binder product about 120-250 EUR
for one, 70-140 EUR at 10, 40-80 EUR at 50. Binders and rigid slipcases at 1-10 copies are the
dominant unknowns; three binder quotes should precede any commitment beyond the pilot.

Where I could not get a real price: every binder, slipcase and custom box maker (quote or
configurator only), loose-leaf drilling at 70 mm, tab dividers in our size, BoD, Lulu, Mixam,
print24, Bookpress.eu, and the physical originals on the collector market.

## 7. How to author the manuals

| Option | Strengths | Weaknesses | Verdict |
|---|---|---|---|
| **LuaLaTeX** with a house class (memoir base), sources in Pandoc Markdown or LaTeX | installed here (TeX Live 2023, LuaHBTeX 1.17); PDF/X-1a/X-4 (`pdfx`), spot colours, xindy/makeindex indexes, cross-references, thumb tabs, TeX Gyre fonts installed; reproducible with `SOURCE_DATE_EPOCH`; the owner already writes LaTeX | the default LaTeX look must be fully overridden; class work up front | **Recommended** |
| ConTeXt (installed, MkIV) | best-in-class grids, spot colour and PDF/X natively | smaller community; steeper for collaborators | strong alternative |
| Typst 0.14.2 (installed) | fast, simple, clean source | `typst compile --help` lists PDF/A and PDF/UA but no PDF/X; spot colours not supported **[MEM]**; indexing via packages | fine for black-only POD interiors; weaker for offset/spot work |
| Scribus / Affinity Publisher / InDesign | best for covers, boxes, labels, die-lines | GUI, not reproducible, poor for 700-page references from repo text | use only for package art (Scribus or Inkscape, free) |
| groff -mm / mom (installed) | genuinely period (1990 Unix DTP) | weak PDF/X, indexes by hand | curiosity only |
| InitechWord itself, printing PostScript | the Borland precedent (TP 6.0 was set in Borland's own Sprint) -- the most in-universe colophon possible | InitechWord does not exist yet and has no PostScript output | long-term option for a later printing; never claim it in a colophon before it is true |

**Recommended pipeline.** Plain-text chapter sources (Pandoc Markdown with a few custom spans:
`[F1]{.key}`, `::: screen`, side-head divs) in version control; Pandoc to LaTeX with a single
`initech-manual` class implementing Annex C; LuaLaTeX to PDF/X-4 (or PDF/X-1a when a printer
asks); covers and spines generated by the same class from a spine-width parameter taken from the
printer's calculator; box nets, labels, cards and templates in Scribus or Inkscape with a named
die-line spot colour; reference tables generated from `spec/` by small tools; screen captures
produced deterministically by the emulator harness (QMP screendump driven by locked traces --
the `record-flair` machinery), converted to greyscale halftones at print size. Every chapter
source carries a comment naming the gates that prove it.

If the publication sources live in this repo, Law 3 (C + make + thin shell) applies to the
generators, and TeX should be treated as an external tool in the way `nasm` is, under a
`make pubs` target outside `make test` (decision D-10).

## 8. Legal and IP notes (not legal advice)

- **"Initech" comes from a 20th Century Fox film.** A private, non-commercial edition is a
  different matter from selling boxes. Recommendation: do not sell the physical edition
  without legal advice; label nothing with the film's logo or artwork (the PRD already treats
  the frame as reference only, PRD Sec 12).
- **Trade dress of the originals.** Model structure, not look: no Borland yellow-and-black
  cover scheme, no Ashton-Tate stripes, no WordPerfect wordmark style, no Apple logo or Apple
  Garamond on Initech items. The IBM-derived line system is generic enough (bands, grids,
  positions) and is re-drawn, not copied.
- **Text.** Write every sentence new, including boilerplate; reference the originals only for
  structure and conventions. Keep the scans out of the public repo (they are in the sibling
  directory, as instructed).
- **Fonts.** TeX Gyre (GUST Font License) and URW base35 are free to use in print; the
  commercial PostScript originals need desktop licences if preferred.
- **Phone numbers, addresses, bar codes.** Use 555-0100..0199 numbers, a fictional address,
  and restricted-circulation UPC prefixes so nothing collides with a real party.

## 9. Open decisions for the owner

| ID | Decision | Options | Recommendation |
|---|---|---|---|
| D-1 | Where production happens | Germany/EU (assumed) / elsewhere | Confirm Germany/EU; Annex D is built on it |
| D-2 | Box line-up | (a) six product boxes (InitechDOS, FLAIR, InitechBase, Initech 123, InitechWord, Turbo Initech); (b) five, with DOS + FLAIR as one "InitechOS 3.30" box (the README's part 1024-3300-01); (c) (a) plus an OEM bundle sleeve | (a): it matches the epic and the period (DOS and the GUI were separate purchases) |
| D-3 | Version numbers on manuals and boxes | mirror the reference originals; or per-product Initech numbering | Decide in software first: the box must match the banner/About box of the shipped build. InitechDOS 3.30 is already fixed |
| D-4 | Trim | one 7 x 9 in trim for all; or per original (IBM 5.5 x 8.5 for InitechDOS) | One 7 x 9 in trim (shelf unity; dominant original size) |
| D-5 | Binder mechanism | US half-size 3-ring, 70 mm (authentic; import mechanisms; bookbinder or US maker); DIN 4-ring (EU stock, wrong for a US company) | US 3-ring. Pilot via a Buchbinderei; ask Schlender to fit imported mechanisms; US maker only for a 50-copy run |
| D-6 | Interior colour | black only; two-colour (Ashton-Tate/WordPerfect style) | Black only (IBM/Borland/Lotus; a fraction of the cost); colour on covers, tabs, boxes |
| D-7 | Media | 5.25-in 1.2 MB HD; 5.25-in 360 K; 3.5-in 1.44 MB; both sizes | 5.25-in 1.2 MB in the box plus a 3.5-in exchange coupon (or both sets, as Lotus and IBM shipped). Needs 1.2 MB images and a multi-diskette SETUP -- file a software bead |
| D-8 | Logo | commission; self-design; wordmark only | Original wordmark only ("INITECH" + "Systems Corporation" + rule); no film artwork |
| D-9 | Commercial status | private/gift edition; sale | Private, non-commercial unless legal advice says otherwise |
| D-10 | Where manual sources live | this public repo (e.g. `pubs/`, `make pubs`); a private sibling repo | This repo: chapters can cite gates and generators can read `spec/`, and the found-footage transparency covers it; the scans stay outside |
| D-11 | Typesetting stack | LuaLaTeX; ConTeXt; Typst | LuaLaTeX house class (Sec 7) |
| D-12 | In-universe address and phone | -- | Fictional address; 555-01xx numbers; keep "extension 2504" |
| D-13 | Buy physical originals for measurement | yes / no | Yes: one each of dBASE III PLUS (binders + slipcase), Lotus 1-2-3 R2.2 box, WordPerfect 5.1 box, Turbo Pascal 6.0 box, IBM DOS 3.30 binder. Many trims, ring and hole sizes, board thicknesses and box dimensions here are inferred from scans; a ruler settles them. Collector prices not checked |
| D-14 | Manuals for unbuilt features (Initech 123, InitechWord, the IDE) | write ahead from plans; write as phases land | Write as phases land; manuals document gated behaviour only |
| D-15 | Colophon | none; Borland-style "produced with ..." line | An honest colophon naming the real tools, until an InitechWord printing makes the in-universe one true |

## 10. Suggested order of work

1. **Decisions D-1 to D-9** (one sitting), then file beads under `initech-gtge` for the steps
   below (I have read-only access to beads).
2. **Measure the originals** (D-13) and correct every [INF]/[MEM] physical figure in Annexes A
   and C; fix the binder, hole and box dimensions.
3. **Templates and a specimen:** the `initech-manual` class, cover/spine template, label and
   card templates; print a 32-page specimen (InitechDOS User's Guide draft) as one book at
   myBuchdruck to check trim, type size, spine and paper.
4. **Pilot package: InitechDOS 3.30** -- Reference (binder, dividers, Reader's Comment Form),
   User's Guide, Quick Reference, the common items (Customer Support Guide, License Agreement
   envelope, registration card, In This Package card), slipcase box, 5.25-in diskettes with
   labels and serials. One complete copy. Reason: the software is the most complete and stable,
   the ADRs already hold the technical content, and the package exercises every hard supply
   chain at once.
5. **InitechBase** (software exists; two binders, large box).
6. **Turbo Initech Programmer's Guide** (ADR-0007 fixes the language), then the User's Guide
   and Self-Compilation Guide when the IDE and `make selfhost` are real.
7. **FLAIR User's Guide** after the window-system audit arc (`initech-tdnl`) settles.
8. **Initech 123 and InitechWord** chapter by chapter as their phases land.
9. Then a run of 10, then 50, with three binder quotes and one box quote in hand.

## 11. What is verified and what is inferred

- **Verified in a source during this task:** package contents of dBASE III PLUS (Smithsonian),
  Lotus 1-2-3 R2.2 (its own Setting Up manual), Turbo Pascal 6.0 (User's Guide, emsps part
  numbers), IBM DOS 3.30 (emsps listings); the dBASE III PLUS and IBM DOS 2.10 box dimensions
  (Smithsonian); WordPerfect 5.1 Reference being ring-bound in a slipcase (Centre for Computing
  History, emsps); all cover, interior, label and card observations (the scans); the Borland
  colophon; all supplier prices marked REAL in Annex D.
- **Measured from scans:** trims of the IBM (5.5 x 8.5 in), Borland, Lotus and Apple manuals
  (about 7 x 9 in), the IBM 2.75 in hole spacing; WordPerfect's 7.9 x 8.9 in is a scan
  measurement that may include padding.
- **Inferred:** dBASE III PLUS trim (from line pitch), typeface identifications, the dBASE
  split of sections across the two binders, Lotus perfect binding, label stock sizes, all
  ESTIMATE prices.
- **From memory, unverified:** Turbo Pascal and Apple perfect binding, the WordPerfect template
  colour-to-modifier mapping, hole diameter, Typst spot-colour support, FOGRA defaults.
- **Not found:** dimensions of the Lotus, Borland and WordPerfect boxes from a reliable
  source; any EU off-the-shelf US three-ring binder; real prices for binders, slipcases and
  custom boxes.

## 12. Sources (all accessed 2026-10-04)

Museum and archive records: Smithsonian NMAH nmah_1695174 (dBASE III Plus) and nmah_1695396
(PC DOS 2.10), via `https://api.si.edu/openaccess/api/v1.0/`; Computer History Museum catalog
102778314 `https://www.computerhistory.org/collections/catalog/102778314`; Centre for Computing
History CH32116 `https://www.computinghistory.org.uk/det/32116/WordPerfect-5-1-Manual/`.

Collector references: `http://www.emsps.com/oldtools/` pages `ibmdos.htm`, `bordbv.htm`,
`lotus123v.htm`, `borpasv.htm`, `wp.htm`, `wpv.htm`.

Scans: see `/home/tobias/Projects/initech-manuals-ref/SOURCES.md` (bitsavers via
`https://bitsavers.trailing-edge.com/pdf/`, WinWorld `https://winworldpc.com/`, archive.org,
`https://mendelson.org/wpdos/manuals.html`).

Suppliers: as listed in Annex D with URLs (myBuchdruck, podbuchdruck, epubli, dbusiness,
Schlender, easyOrdner, ordner.de, print24, madika/EGGER, Pinguin Druck, Pixartprinting,
Packhelp, onlineprinters, mybinding, Vervante, FloppyDisk.com, poly.play, Retro 8bit Shop,
Tindie/Glitch Works, AMIGAstore.eu). Collector listing summaries (System 7 Personal Upgrade
Kit contents; WordPerfect box dimensions) came from web-search summaries and are marked
low or medium confidence where used.
