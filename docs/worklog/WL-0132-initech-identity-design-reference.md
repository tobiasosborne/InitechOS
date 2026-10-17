# WL-0132 -- The Initech identity in the design reference

Lane `design` (branch `lane/design`), epic `initech-ipu5`. Operator directive
2026-10-17: "It currently does not feature the initech logo correctly from the
movie, nor the official cyan. Our manuals must mirror the originals both in
terms of content and presentation and also completeness." Mid-lane the
operator added the identity references (`initech-manuals-ref/initech-identity/`)
and the slogan directive ("Is This Good for the COMPANY?", exact capitals).

`bd create` failed (another process holds the embedded-dolt lock): **PM to file
the child bead** under `initech-ipu5` -- suggested title "Initech identity in
the design reference: the logo, Initech Cyan, company banner AN-27".

## Context

PUB-REF-0001 1.0 ruled D-8 "wordmark only; no illustration logo", and PK-01
carried the desktop teal `#8DDCDC` only as a FLAIR box accent. The lane brief
assumed the "official cyan" was that locked desktop canon. Both are revised.

## What changed

| Piece | Change |
|---|---|
| `pubs/common/initech-logo.tex` (new) | The logo as original TikZ drawing commands, authored from observation and measurement, parameterised by height: `\InitechLogo`, `\InitechLogoBlack`, `\InitechMark(Black)`, `\InitechLogotype`, `\InitechLogoForWidth`. Header states the measurements and the honesty rule (never "extracted") |
| `pubs/class/initech-manual.cls` | `InitechCyan` cmyk 62/0/34/8, `InitechCubeLight` 16 % K, `InitechCubeMono` 42 % K; loads tikz and the logo; the header documents every stable identity macro |
| `pubs/class/initech-forms.sty` | `\Wordmark` keeps its name, now the logo (colour on colour pieces, black inside); `\IsThisGoodForTheCompany[<width>]`; cover grid: logo cell at 23/183 mm 30 mm wide, 1 pt cyan title hairline, 6 mm cyan line band at the foot; new `\MakeBackCover` (logo, banner, address, part-number plate, line band); spine: logo 22 mm, line band; banner under the edition notice of every verso and at the foot of every Reader's Comment Form |
| `pubs/manuals/initechdos-{reference,users-guide}/cover.tex` | page 2 = the back cover |
| `pubs/common/colophon.tex` | says the logo is an original TikZ drawing authored from observation |
| `harness/pubs/cover_check.c` (new), `families.sh`, `pubs.mk`, `mutants.sh` | identity leg of `test-pubs-families` (below); `pubs/common` joins TEXINPUTS of the harness scripts |
| `docs/pubs/DESIGN-REFERENCE.md` 1.1 | document control + revision history; tags `[idn]`, `[web]`; D-8 revised (dated, superseding 1.0 and the [res s8] caution); Sec 1.3 three directive rows; TY row (logotype drawn, never typeset); AN-20, AN-25, AN-26 touched; **AN-27 company banner**; new **Sec 7.0 the logo (measured)**; PK-01 Initech Cyan rows, measurement table, arithmetic, desktop-canon recommendation, line-colour decision; PK-02..PK-15 logo cells and line band; **PN-10 part-number plate**; GC identity leg; 11.5 conformance row; C-17..C-20; O-6..O-9 |

## What the logo is (measured)

Three identical blocks in 2:1 dimetric (receding edges rise 1 in 2; verticals
sqrt(3/2) of the unit run -- the 30 deg-elevation orthographic view). Each block
is a cube of edge 2 with a cube of edge 1 removed from its top front corner.
One block at the back, two in front at (-2.5u, -2u) and (+2.5u, -2u): mark
9u x 6.449u. Tops and notch floors cyan, left faces near-black, right faces
light grey. Under it INITECH in a black, extended, oblique (11.7 deg)
Helvetica-class grotesque, cap height 2.536u, slanted width 15.264u; mark
centred, 1.471u above the cap line. Lockup aspect 1.459. Measured on the
clean still (u = 19.5 px) and a 1200-px third-party rendering (u = 62.89 px):
agreement within 2 %. The operator's "Eurostile/Microgramma" reading is
recorded as C-20: the measured C (round bowl, horizontal terminals) is
Helvetica-class; the drawing follows the measured outlines either way.

## The cyan

| Sample | RGB | Hex |
|---|---|---|
| clean logo cube tops (record) | 87/177/159 | #57B19F |
| headquarters sign cubes | 75/151/147 | #4B9793 |
| banner COMPANY | 120/169/157 | #78A99D |
| locked desktop canon | 141/220/220 | #8DDCDC |

The reference frame (`spec/assets/preview.webp`, viewed in place, not copied)
shows no cyan desktop -- the windows cover it; the frame-sampled ground is
grey. So the frame cannot confirm `#8DDCDC`, and the identity ink is not it.
House device arithmetic gives 51/0/10/31, which the proof renderer shows as a
blue-grey; the press build 62/0/34/8 renders 87/179/160. Nearest Pantone 7472 C.
Recommendation (not performed): the desktop canon should not silently
follow; that is a locked-spec decision for its own bead (O-6).

## Acceptance

- `make pubs`: 7 documents built, 0 overfull (covers now 2 pages: front + back).
- `make test-pubs-families`: 7 families green; identity leg green (front, back
  and spine logo cells: dark ~21 %, light ~8 %, Initech Cyan ~7 %; line bands
  100 % cyan); mutants `mutant-nologo` RED `COVER-LOGO`, `mutant-wrongcyan`
  RED `COVER-CYAN` ("logo cube tops are not Initech Cyan #57B19F (cyan 0.0%,
  other cyan-family ink 8.1%)").
- `make test-pubs`: build, commands (129 shipped / 45 documented / 84
  omissions), mentions, allow-list, lint (ASCII, period vocabulary/years),
  fonts, examples all green; **test-pubs-screens RED on a pre-existing drift
  unrelated to this lane**: `chkdsk.txt` / `chkfile.txt` (both DOS books)
  print 118448 bytes of memory where the base commit's kernel now reports
  118320 (the kernel grew in the merges before 4832d01; no os/ file is
  touched here). One earlier run also timed out on `forwild` at load 17
  (prompt did not settle). Needs `make pubs-screens` on main once the
  kernel settles (PM).
- `test-pubs-mutant` body (`harness/pubs/mutants.sh`, run directly because
  its test-pubs prerequisite stops at the screen drift): 11 named mutants
  rejected, green.
- `make test-pubs-repro`: 7 PDFs byte-identical across two clean builds.
- Law 4: rendered `cover.pdf` page 1 and 2 and the spine at 60-600 dpi and
  looked. Front: white ground, product line, EB Garamond "Reference", Systems
  band at 55 mm, the logo at the lower left reading exactly as the reference
  (three notched blocks, green-teal tops, black and grey sides, black oblique
  INITECH), part number lower right, the cyan band along the foot. Back: logo
  top left, the banner "Is This Good for the COMPANY?" across the measure with
  COMPANY in cyan bold oblique over the rule and the tall question mark past
  its end, address lower left, part number lower right, the foot band. Title
  page: the same grid in black with the one-colour logo. Verso: the banner in
  black under the edition lines. Reader's Comment Form: the banner at the
  foot. Spine: logo over the product name, family stripe, version, part
  number, foot band.
- Mutants (Rule 6): a cover without the logo goes RED `COVER-LOGO`; a cover in
  the desktop teal's build goes RED `COVER-CYAN`.

## Frictions

- The period-year lint reads a decimal fraction of four or more digits above
  1993 as a year (`2.4495`): every coordinate is rounded to three decimals.
- xcolor's `[cmyk]` model converts an RGB definition without the 1/(1-K)
  normalisation; the cyan is therefore given in CMYK.
- poppler's CMYK conversion is not the naive formula, so the gate's
  tolerance is about the rendered press build, while the expected value stays
  the independent measured sample.

## Not done

- Logo on the Reader's Comment Form's form side, box panels, diskette labels,
  dividers and cards: no class templates exist; the rules are in PK-06..PK-15 (O-8).
- The desktop canon decision (O-6), the press proof (O-7), clip frames (O-9).

## Pointers

Sources consulted on the web (nothing downloaded into the repository):
https://www.brandsoftheworld.com/logo/initech-coloured ,
https://vectorseek.com/vector_logo/initech-logo-vector/ ,
https://iconape.com/initech-coloured-logo-logo-icon-svg-png.html ,
https://www.pantone.com/color-finder/7472-C , https://www.pantone.com/color-finder/563-c ,
https://www.colorxs.com/color/pantone-7472-c , https://icolorpalette.com/color/6bbbae .
Private references: `initech-manuals-ref/initech-identity/` (README, Initech.png,
SQ.png, Culture-Change-Methods.jpg, clip/COLOURS.md).
