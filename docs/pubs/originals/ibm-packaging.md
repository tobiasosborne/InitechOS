<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Structural Notes on the Originals (A-3): IBM Personal Computer Software Packaging, 1984-1995

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Research Annex (structural model, not prose)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.1`, originals scour A)

## Document Control

| Field | Value |
|---|---|
| Document ID | PUB-ORIG-0001-C |
| Version | 1.0 (research findings, not a decision) |
| Date | 2026-10-16 |
| Companions | `ibm-dos.md` (PUB-ORIG-0001-A), `apple-macintosh.md` (PUB-ORIG-0001-B) |
| Supersedes | nothing; deepens the "IBM package line" row of `docs/packaging/ORIGINALS-PHYSICAL-SURVEY.md` Sec 5 |
| Reference library | `/home/tobias/Projects/initech-manuals-ref/packaging/` (outside the repo; manifest `SOURCES.md`) |
| Copyright posture | Layout, proportions and the kinds of copy each panel carries. No artwork reproduced; licence text summarised by heading only. |

---

## 0. Defined terms and provenance tags

Tags as in PUB-ORIG-0001-A Sec 0 ([READ], [MEAS], [INF]). For images, **"img"** cites the scan file; positions are fractions of the panel (x from the left edge, y from the top edge), [MEAS] at 25% scale by colour segmentation or read off the displayed image (about +/- 0.01).

| Term | Meaning |
|---|---|
| **Slipcase / package** | The open-ended rigid card case that holds the ring binder(s) and the diskette holder. IBM called it a "storage box" (Smithsonian record, quoted in PUB-RES-0001-A Sec 5). |
| **Front / back panel** | The two large faces. Only fronts and backs were photographed; the spine and edges of the 1980s packages are not in the library. |
| **Family band** | The single saturated-colour bar that names the product family; the only strong colour on the package apart from the illustration. |
| **Ground** | The panel's background colour. |

Files (all read in full): `Displaywrite_4_Box_F.jpg`, `Displaywrite_4_Box_B.jpg`, `PC_3270_V3.00_Box_F.jpg`, `PC_3270_V3.00_Box_B.jpg`, `PC_Network_Program_1.0_Box_F.jpg`, `PC_Network_Program_1.0_Box_B.jpg`, `SCRIPT_PC_Box_F.jpg`, `Personal_Communications_3270_4.0_F.jpg`, `Personal_Communications_3270_4.0_S.jpg`, and the booklet `74X9319_DisplayWrite_4_Getting_Started_Aug86.pdf` (68 pp., about 25 viewed).

---

## 1. Inventory and dating

| Package | Front | Back | Copyright on box | Box part no. | Family / band colour |
|---|---|---|---|---|---|
| SCRIPT/PC | img F | -- | not visible | not visible | "Productivity Series", purple |
| PC Network Program 1.0 | img F | img B | 1984 | 6024195 | "Communications Family", green |
| DisplayWrite 4 | img F | img B | 1986 | 74X9904 | "Office Systems Family", blue |
| PC 3270 Emulation Program 3.00 | img F | img B | 1986 | 59X9969 | "Communications Family", green |
| Personal Communications/3270 4.0 | img F | img S (side) | c. 1995 (SOURCES.md) | not visible | none: different system |

Panel aspect (width/height) of the four 1980s fronts is 0.87-0.88 [MEAS: 2416-2426 x 2753-2780 px]. That matches the only dimensioned IBM box of the line, the DOS 2.10 storage box, 9.5 x 8 x 2.25 in (aspect 0.84) [READ via PUB-RES-0001-A Sec 5, Smithsonian nmah_1695396]. Front panel about 8 x 9.5 in [INF]. The PComm 4.0 front is taller and narrower, aspect 0.68 [MEAS: 1870 x 2759 px], with a ~0.33-width side panel [MEAS: 912 px] [INF: a retail shelf box, not a slipcase].

---

## 2. Front panel grid (1984-88 line)

Common to SCRIPT/PC, PC Network, DisplayWrite 4 and PC 3270 [READ all four img F]. The family band is measured; the rest read off the images.

| Zone | Position | Content | Type and colour |
|---|---|---|---|
| Ground | full bleed | Warm grey (PC Network, PC 3270) or khaki/olive-grey (DisplayWrite 4, SCRIPT/PC) [READ]; a matt card | -- |
| Kicker line (optional) | x 0.11-0.35, y 0.03-0.04 | Series or category above the rule: "IBM DisplayWrite Series", "Text Formatter" | Bold sans ~14 pt, black [INF Helvetica Bold] |
| Hairline rule | y 0.04-0.05, from x 0.11 (aligned with the title) to the right edge (bleeds) | -- | ~1 pt, white |
| Product title | x 0.11, y 0.06-0.16; one to three lines | "DisplayWrite 4"; "IBM PC Network Program"; "IBM PC 3270 / Emulation Program / Version 3.00" | Large serif, ~60-72 pt, black, tight leading [INF ITC Garamond, the same face as the binder covers] |
| Media legend | x 0.63-0.85, y 0.06-0.13, aligned to the title's first line | `5.25" diskette`; `3.5" & 5.25" diskettes` (absent on PC Network and SCRIPT/PC) | Same serif ~48 pt, white (DW4) or white + black (3270) |
| Document-type line (optional) | x 0.11, y 0.20-0.22, just above the band | Name of the binder the package holds: "Reference"; "System Planner's and User's Reference" | Regular sans ~14 pt, black |
| **Family band** | y 0.245-0.27 (DW4 0.244-0.267, PC Net 0.245-0.271, 3270 0.249-0.273, SCRIPT 0.269-0.294); x from the LEFT EDGE (bleeds) to 0.84-0.85; depth 2.3-2.6% of panel height (~0.25 in) | Family name, reversed | Bold sans ~16 pt, white, starts at x 0.11 |
| Illustration | Square, directly under the band: x 0.18-0.84, y 0.27-0.80; its right edge aligns with the band's end | Photograph or airbrush art of the product in use (screens, keyboard, network diagram, SCRIPT markup tags) | Full colour, no border |
| Logo block | x 0.11-0.24, y 0.79-0.93 | White 8-bar IBM logo; below it "Personal / Computer / Software" in three lines (SCRIPT/PC, the earliest: "IBM Software" bold over "for IBM Personal Computers" light) | Bold sans ~16 pt, white |
| Product blurb | x 0.46-0.83, y 0.80-0.96, ragged right | Three to seven lines: imperative verb phrases saying what the product does (SCRIPT/PC sets its first phrase in bold) | Regular sans ~16 pt, black |

**Proportions to keep [INF].** Left text edge at about 0.11 of width; band and illustration share a right edge at 0.84; band about a quarter of the way down; illustration fills the middle half; logo and blurb share a baseline zone in the bottom fifth.

---

## 3. Back panel grid (1984-88 line)

Read on DisplayWrite 4, PC 3270 and PC Network img B [READ]. Two halves.

| Zone | Position | Content |
|---|---|---|
| Left half: specification columns | x 0.04-0.48 | Bold sans heads with a 1 pt rule under each, text ~14 pt regular sans below, thin rules between items. Fixed head order: **Software required:** -> **Software included:** (diskette count and size) -> (family band) -> **System requirements:** -> **Options supported:** (DW4) or product-specific heads ("Emulated functions:", "For Network Station and Stand-Alone User Station:", "For Communications Gateway ...:" on 3270); configuration sub-lists by role (Redirector / Receiver / Messenger / Server on PC Network) |
| Back family band | y 0.26-0.28, from the left edge to x ~0.48-0.49 (half width), no text | A shortened echo of the front band, dividing "software" from "system" blocks [MEAS DW4, read 3270 / PC Net] |
| Warranty line | In the left half, under a rule, after the options | Statement of the limited warranty period and its end date (DW4) [READ] |
| Footnotes | Left half, small | Superscript-numbered notes and a "Note:" on product replacement (3270) [READ] |
| White IBM logo | Centre of the panel: x 0.28-0.38 at y 0.79-0.82 (DW4, 3270) or y 0.25-0.28 aligned with the band (PC Network) | -- |
| Colophon | Lower left, x 0.04, y 0.79-0.95 | Copyright year + "All rights reserved."; corporate name in two lines + P.O. box + city (Boca Raton, or Irving, Texas for the DW4 office line); "Printed in the United States of America"; **box part number** alone on the last line |
| Right half: licence | x 0.51-0.98, full height | IBM Program License Agreement printed on the box: heading block (corporate name + "IBM Program License Agreement" left, city right, rule under) -> italic opening condition (opening the package means acceptance) -> **License** (lettered a-d) -> **Term** -> **Limited Warranty** (and Disclaimer) in bold italic -> **Limitation(s) of Remedies** (numbered) -> **Service** -> **General**; ~8-9 pt sans; a licence form number with month-year at the foot (DW4: `Z125-3301-1 10 83`) |

The licence is on the package so that it binds before opening; IBM moved it to the inside covers of the books by 1988 (see PUB-ORIG-0001-A Sec 3) [READ 84X1712 PDF p. 2].

---

## 4. Contrast: Personal Communications/3270 4.0 (c. 1995)

| Panel | Content | Evidence |
|---|---|---|
| Front | White ground; blue 8-bar logo top right; italic serif tagline in four short lines upper left; a staged photograph (mask, ribbon, monitor and keyboard on a violet diamond) filling the middle; product name very large in a Garamond-like serif, two lines, lower left; a spec strip at the foot in bold condensed sans, warm grey: version / media / platforms | img F [READ] |
| Side | Blue logo top; a short licence-acceptance sentence; bold sans heads **Hardware Requirements**, **Supported Hardware**, **Software Requirements**, **Related Software** with regular sans items; four compatibility marks (Windows-compatible, NetWare, DOS, IBM LAN) between the blocks | img S [READ] |

What changed [INF]: no family band, no fixed grid, white ground, marketing tagline, third-party compatibility logos, requirements moved to a side panel. For a 1990-93 Initech line the 1984-88 grid (Sec 2-3) is the model; this box marks the end of it.

---

## 5. What the package holds (from the books inside)

| Package | Contents stated | Evidence |
|---|---|---|
| DOS 3.10 | Three books (Application Setup Guide, DOS User's Guide, DOS Reference) and two diskettes in a plastic pocket at the back of the Reference | `dos33` 6138519 PDF p. 16 [1-3] |
| DOS 3.30 Technical Reference | The book, a Quick Reference card, one 5.25 in and one 3.5 in utilities diskette | `dos33` 80X0945 PDF p. 5 [iv] |
| DisplayWrite 4 | Five 5.25 in diskettes (box back); *Getting Started* booklet, *Reference Guide*, *Technical Reference*, keyboard template | img B; 74X9319 PDF p. 5 [iv], 22 [15] |
| PC 3270 3.00 | One 3.5 in + two 5.25 in diskettes; *System Planner's and User's Reference* | img B, img F |
| PC Network 1.0 | Three double-sided diskettes | img B |

---

## 6. The booklet in the box: DisplayWrite 4 *Getting Started* (74X9319, Aug 1986)

| Aspect | Finding | Evidence |
|---|---|---|
| Purpose | Install, load, and work one guided exercise (create, save, print, revise, spell-check, paginate a sample letter) before using the Reference Guide | PDF p. 4 [iii] |
| Format | 5.5 x 8.5 in [MEAS: 2200 x 3392 px at 400 ppi]; three-hole punched blue card cover (holes visible front and back), i.e. the same object as the DOS binder books, in booklet thickness | PDF p. 1, 68 |
| Cover | Same grid as the DOS covers: "DisplayWrite 4" bold sans line + hairline, "Getting Started" large light serif, white "Office Systems Family" bar bleeding left, logo block lower left, part number lower right | PDF p. 1 |
| Title page | Black-on-white repeat with a black family bar | PDF p. 2 |
| Edition page | "First Edition August 1986", change notice, licensed-program equivalence sentence, trademark list, copyright | PDF p. 3 |
| Front matter | About This Booklet (with a boxed sample of how steps are shown) / Related Publications (each book with a one-paragraph role) / Contents (no leaders) | PDF p. 4-6 |
| Body | Sans throughout (Helvetica-style), continuous folios bottom outside in bold; chapter opener = heavy short rule + bold sans title, text follows on the same page | PDF p. 22 [15] |
| Steps | Each procedure in a ruled box whose top rule is interrupted by a bold title ("To Select Create"); numbered steps inside; notes as bold "Note:" | PDF p. 24-25 [17-18] |
| Screens | Menu panels reproduced in a dot-matrix-style face on a grey screen, with callout labels and leader lines below | PDF p. 23 [16], 25 [18] |
| Chapters | About DisplayWrite 4 (1); Installing (9); Loading (13); Creating a Sample Document (15); Ending and Saving (31); Printing (33); Revising (35); Checking Spelling (47); Paginating (49); Where To Go for More Information (53); Index (55) | PDF p. 6 [v] |
| Index | Two columns, bold sans letter heads, "Numerics" group last | PDF p. 62-66 [55-59] |
| Back cover | Blue; colophon top left in a light blue sans (copyright, Irving, Texas address, printed-in line, part number); logo lower left | PDF p. 68 |

---

## 7. Rules for an Initech package line [INF]

1. One ground colour for the whole line (warm grey or khaki), matt; one saturated family band per product family, colour-coded, bleeding left and stopping at 0.84 of the width, a quarter of the way down.
2. Title in the same serif as the binder covers, flush left at 0.11 under a white hairline that bleeds right; media legend top right.
3. Square illustration hung from the band; logo block bottom left; a short imperative blurb bottom right.
4. Back: spec columns left with fixed bold heads (Software required / Software included / System requirements / Options supported), a half-width echo of the band, the licence printed in full on the right half with its own form number, colophon with the box part number bottom left.
5. Inside: punched card-cover books in the same blue cover grid, one per reader role, plus a quick reference card and the diskettes.

---

## 8. Not determinable from these scans

- Spine and end-panel layout of any 1980s IBM package (only fronts and backs were photographed).
- Physical dimensions and board thickness of these four packages (aspect only; size inferred from the DOS 2.10 record).
- Pantone or process values of the grounds and bands (scan colour is uncalibrated and the cards have aged).
- Whether the slipcase was a closed box with a lid or an open-ended case (the photographs show flat panels only).
