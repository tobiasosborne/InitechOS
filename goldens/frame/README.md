# The frame: reference package

The surviving still of the "Saving tables to disk..." screen, read once,
carefully, by hand, and written down. Everything here is independent of the
product's code: InitechCalc's content checks read `transcription.tsv`; nothing
in this directory was produced by running the product. The gap drawing, added
with InitechCalc's content, is a proposal for the operator's eye and not a test
oracle.

## Questions only the operator can settle

Each has a recommendation. Item ids refer to `transcription.tsv`.

1. **Vendoring the still.** The repository is public, and `.gitignore` has
   kept film stills out of it since 7b213e4c ("measured/derived from, never
   vendored"), and the operator ruled on 2026-10-24 (initech-dam) that the
   still is a local-only fixture with nothing to commit. This package pins
   the still by SHA-256 below and does not commit its bytes. *Recommend: keep it out of the repository; the factory
   frame smoke reads it through a pinned local corpus root, as the Lotus and
   dBASE captures are read.* If the operator wants it vendored, it is one
   commit of the two files named below plus a `.gitignore` exception.
2. **LED.R07.C1, Matte Cutters, first amount: `738.00` or `730.00`.** The
   design read `730.00`; the third digit has the closed shape of an 8 at
   every magnification. *Recommend `738.00`.*
3. **LED.R08.CL, `Fiml Strippers`.** The still spells it `Fiml`.
   *Recommend keeping the spelling as seen*: the still is the specification,
   and a typing slip in a surviving ledger is part of what survived.
4. **LED.R13.CL `Ink form TOYO` and LED.R14.CL `Ganns Best ink`**: also kept
   as seen (`form`, lower-case `ink`). *Recommend as seen.*
5. **The 57 probable ledger cells as a whole.** Each is a reading I would bet
   on; the plausible alternative (mostly 0/8 and 5/6 confusions at this
   resolution) is in its note. *Recommend promoting all of them to data, as
   the brief directs, with this sheet as the record of which were probable.*
6. **The three cut-off rows (LED.R01-R03).** Only `Comput`, `Printi` and
   `Iorge` show before FILE COPY covers them. *Recommend empty label cells*:
   completing a word would be invention, and half a word on a ledger reads as
   a rendering fault.
7. **Rows above the fragment.** The ledger window's top 168 still pixels
   (about eleven 15.2-pixel rows) are under Drive A Files and the chart.
   *Recommend drawing that space as empty ruled rows*, so the window has the
   size and the visible rows have the position the still shows; the row count
   is geometry, the content is "not recovered".
8. **The ledger window's title.** Only `Graph c` shows (LEDWIN.TITLE); the
   rest is under the chart. *Recommend keeping InitechCalc's title until the
   operator chooses*; the alternative is the Macintosh convention of the
   document's name.
9. **Where the note goes.** The design put "Blank entries were not recovered
   from the surviving material." in a strip above the total; the still has the
   total directly under the last data row, with ruled blank rows beneath it.
   *Recommend a strip at the foot of the ledger, below the total.*
10. **The pie's angles.** Measured clockwise from 12 o'clock on the unsquashed
    ellipse, the 40%, 14% and 9% slices span about 103, 36 and 21 degrees:
    proportional to their labels, but at about 2.6 degrees per percent, which
    closes neither a 116-percent pie (3.10) nor a 100-percent one (3.6). The
    35%/18% boundary is under FILE COPY. *Recommend slices of value/sum
    (the pie closes; the labels keep their stated values and sum to 116).*
11. **Number alignment.** Every amount in the still is left-aligned in its
    cell, as labels are. *Recommend left-aligned, as seen.*
12. **The chart's blues.** The still has a mid-blue top face (relative
    0.26 0.51 1.00 of the photographed white) and a darker side (0.20 0.29
    0.53) on a near-black field. The Toolbox's colour tokens are white, black,
    grey and navy. *Recommend navy on black now, and a request to the SDK
    owner for a slice-blue token.*
13. **Contradictions inside the still, for the record.** Two menu bars
    (settled: one, operator 2026-11-13); an outer worksheet whose rows are
    20.8 pixels apart under a ledger whose rows are 15.2 (SHEETWIN, LEDGRID:
    the grids do not align); a name box reading `C17` that matches the outer
    worksheet's row 17, not the ledger; FILE COPY with scroll bars and no
    Cancel button; a progress fill with no track, so its fraction cannot be
    read. *Recommend building none of the second application's chrome*
    (second bar, toolbars, outer worksheet), as the design says.

## The still

| | |
|---|---|
| Source | `attic/spec/assets/preview.webp` in the main checkout (git-ignored); byte-identical to legacy-final `spec/assets/preview.webp` |
| Format | WebP (lossy), 1456 x 819, RGB |
| SHA-256 | `11014f3332b66f2028c41f4c0236007de5dd540c8127a9f89942df84e0c52b6e` |
| Decoded | binary PPM (P6), 3,577,408 bytes |
| SHA-256 of decoded | `7a5a50c7091d6232af48d26ca60e004eb93172854eb0ba42286beade0360a7d7` |

Decoding, reproducibly: `convert preview.webp -depth 8 ppm:preview.ppm`
(ImageMagick 6.9.12-98 with libwebp 1.3.2). PIL 10.2.0 over the same libwebp
gives the same bytes. ffmpeg's own WebP decoder upsamples chroma differently
and does not (`5b7b09b5...`); use libwebp. The factory reads PPM already, so a
pixel check needs no new decoder. `sha256sum -c SHA256SUMS` checks both files
when they are placed beside this README.

## The sheets

All three are tab-separated, one header row, coordinates in the still's
pixels, boxes half-open `[x0,x1) x [y0,y1)`.

- `transcription.tsv`: every piece of text and every number visible, with
  the reading, a confidence and its box. 220 items: 47 certain, 57 probable
  and 36 unreadable ledger cells, 30 visibly empty ledger cells, and the
  menus, titles, chart, dialog and canon facts.
  - *certain*: read the same at every magnification and no digit has a
    plausible alternative;
  - *probable*: I would bet on it; the alternative is in the note;
  - *unreadable*: hidden or not legible; I would not bet;
  - *blank*: visible and empty. Not the same as unreadable.
  The chart's five label items (CHT.40 ... CHT.9) are listed clockwise
  from 12 o'clock, the order of the slices in the still.
  Ledger cells are named by fragment position (`LED.R07.C1` is fragment
  row 7, amount column 1; `CL` is the label column, `C9` the narrow column),
  never by invented spreadsheet addresses. The canon facts (116%, the
  hourglass, `570-`) are recorded as items regardless of legibility.
- `geometry.tsv`: bounds of the windows, menu bar, dialog, chart, pie and
  ledger grid, in still pixels, as proportions of the visible raster
  `(293,72)-(1191,725)`, and mapped linearly to 640 x 480. The raster's left,
  right and top edges are sharp; its bottom edge is not, so vertical
  proportions carry about 1% uncertainty. CRT curvature is ignored.
- `colours.tsv`: median colours of sampled boxes with the role each plays
  (window white, window grey, chart field, slice blue, slice side, progress
  blue), and each relative to the photographed white, because the photograph
  is dim (white reads about #848089).

Glyphs are not extracted: the still is too small and too compressed, and the
product's fonts are hand-authored (CLAUDE.md, "Hazards that are still true").

## The gap drawing (treatment A)

`proposal-ledger.png` (479 x 327) and `proposal-chart.png` (222 x 173) are
the content of InitechCalc's ledger and chart windows at native resolution,
as the product draws the shipped ledger: readable cells filled, unreadable
ones empty and ruled, the one-line note at the foot. They were produced by
running the real CALC.EXE code over sdk/host, recording its Toolbox calls
and replaying them into a 640 x 480 buffer with the product's own Geneva 9
strike and the four Toolbox colours (white, black, grey #C0C0C0, navy
#000080); window chrome is not drawn. They are a proposal for the operator's
eye, not a test oracle, and nothing reads them.

## How this is used

InitechCalc's tier-0 content check reads `transcription.tsv` with an
independent reader and compares the shipped ledger document against it, and
takes the pie's place on screen from `geometry.tsv` (PIE, PIEDEPTH). Its
smoke grades the booted chart against `geometry.tsv` and the chart-field
samples of `colours.tsv` (apps/initechcalc/tests/frame-pixels.sh). The
product's code never reads this directory. `goldens/CORPORA.lock` pins
`transcription.tsv`, so changing a reading is a deliberate, reviewed act.
