# sys8/window-chrome.md -- Mac OS 8.1 Platinum window chrome (title bar, widgets, frame, grow box)

## Purpose + consumer

Mac OS 8.1's "Platinum" window is drawn by the **Appearance Extension** WDEF, not by the
System-7 `documentProc` WDEF: the same 640x480 indexed-8 screen, but a 22-px title bar
carrying a 12-row horizontal racing stripe over a 3-D bevelled bar, THREE title-bar
widgets (close left; zoom then collapse right, collapse RIGHTMOST), a 4-px raised
picture-frame border around the content, 16-px scroll gutters, a diagonal-grip grow box,
and a hard 1-px black drop shadow. Every number and every RGB below is measured
programmatically from the `s8_*` goldens in `goldens/captures/` at the cited pixel
coordinates -- nothing here is transcribed from a document, because the Platinum grays are
**hard-coded in the Appearance Extension CDEF/WDEF code**, not published and not in a
`wctb` (the shipped `wctb` 0 still carries the System-7 tinge anchors; see
[platinum-palette.md](platinum-palette.md) sec 5).

**Era (Law 3).** This spec is Mac OS 8.1 Platinum ONLY. It is a *different era* from
`../chrome/title-bar.md` (System 7.0/7.1, 19-px bar, lavender `#DADAFF`/`#B3B3DA` tinge,
two widgets). Do not mix the two. Deltas vs System 7 are tagged inline.

**Gamma warning (read before using any RGB here).** The goldens were minted at 256
colours with **"Mac HiRes Std Gamma"** active (`goldens/captures/s8_81_monitors_256.png`
shows both settings). Every RGB in this file is the **as-sampled framebuffer value**.
The *nominal* CLUT entry the WDEF writes is the de-gamma'd value; the bijection is proven
in [platinum-palette.md](platinum-palette.md) sec 1 and given in the "nominal" column of
every colour table below. A reimplementation that fills a standard Mac 8-bit CLUT must use
the **nominal** column; a reimplementation graded by SSIM against these PNGs must use the
**sampled** column.

Cross-links: [scrollbars.md](scrollbars.md), [menus.md](menus.md),
[controls.md](controls.md), [platinum-palette.md](platinum-palette.md),
[../chrome/title-bar.md](../chrome/title-bar.md) (the System 7 predecessor),
[../chrome/close-zoom-box.md](../chrome/close-zoom-box.md) (System 7 widgets).

---

## 1. Structure rect + drop shadow

Measured on the active Finder window "Initech81" in `s8_doc_window_active.png`.
Corner-to-corner scans: `vscan x=120 y=60..325`, `hscan y=150 x=60..490`,
`vscan x=480/481 y=66..322`, `hscan y=315/316 x=60..486`.

| metric | rendered px | golden + coords | notes |
|---|---|---|---|
| struct rect (1-px black outline) | x=65..480, y=70..315 (416 x 246) | s8_doc_window_active x=65/480 cols, y=70/315 rows all `#000000` | the outline is part of the window, NOT the shadow |
| drop shadow ink | `#000000` (1 px) | s8_doc_window_active x=481, y=316 | solid black, not dithered, not gray |
| drop shadow right column | x=481, y=**72**..316 | vscan x=481: `#000000` y=72..316 | starts at `struct.top + 2`, ends at `struct.bottom + 1` |
| drop shadow bottom row | y=316, x=**67**..481 | hscan y=316: `#000000` x=67..481 | starts at `struct.left + 2`, ends at `struct.right + 1` |
| shadow offset rule | (+1, +1) with a 2-px notch at the near corners | both scans | no shadow on the top or left edges |

Cross-check on a second window (Applications, `s8_doc_window_inactive.png`): struct
x=309..627, y=253..420; shadow column x=628, shadow row y=421 (`hscan y=407 x=627..628 =
#000000 #000000`). [verified: s8_doc_window_active + s8_doc_window_inactive]

**Inactive delta:** the shadow is drawn in `#777777`, not black, and follows the same
+2 notch: `vscan x=326 y=31..199 = #777777`, `hscan y=199 x=15..326 = #777777`
(struct x=13..325, y=29..198). [verified: s8_doc_window_inactive]

---

## 2. Active title bar

### 2.1 Band geometry

Vertical cross-section at a stripe-only column (`vscan x=120`, and `x=200` on the
Appearance CP). Rows are given relative to **T = the top black frame row**.

| row | value | role | golden @ coords |
|---|---|---|---|
| T+0 | `#000000` | top window-frame line | s8_doc_window_active y=70 x=120 |
| T+1 | `#FFFFFF` | frame-bar highlight | y=71 |
| T+2..T+3 | `#DADADA` | frame-bar face (2 rows) | y=72..73 |
| T+4..T+15 | stripes (12 rows) | racing stripe field | y=74..85 |
| T+16..T+19 | `#DADADA` | frame-bar face (4 rows) | y=86..89 |
| T+20 | `#B3B3B3` | frame-bar shadow | y=90 |
| T+21 | `#000000` | bottom line (top of the content frame) | y=91 |

**Title-bar height = 22 px frame-to-frame inclusive** (T..T+21).
[verified: four independent windows -- s8_doc_window_active y=70..91;
s8_doc_window_inactive "Applications" y=253..274; s8_window_collapse y=253..274;
s8_controls_dialog "Appearance" y=53..74]

vs System 7: 19 px (`minTitleH`), see `../chrome/title-bar.md`. **Platinum is +3 px.**

### 2.2 The stripe field

The 12 stripe rows alternate, **starting light at T+4 and ending dark at T+15**:

| row parity | value | nominal | golden @ coords |
|---|---|---|---|
| T+4, +6, +8, +10, +12, +14 | `#FFFFFF` | `#FFFFFF` | s8_doc_window_active y=74 x=86..238 (153 px uniform) |
| T+5, +7, +9, +11, +13, +15 | `#969696` | `#777777` | s8_doc_window_active y=75 x=87..239 (153 px uniform) |

Each stripe row is uniform along x (no dither, no sub-pixel structure -- verified over the
153-px run above and the 138-px run x=305..442).

**Measured 1-px row-parity offset.** The light rows and the dark rows do NOT share the
same x-extent: on this window the light rows run **x=86..442** and the dark rows run
**x=87..443** (both 357 px long). Left edge: `(86,74)=#FFFFFF` but `(86,75)=#DADADA`;
right edge: `(443,74)=#DADADA` but `(443,75)=#969696`. The same +1 shift appears at the
title-text gap (light rows gap x=239..304, dark rows gap x=240..305).
[verified: s8_doc_window_active hscan y=74 vs y=75 vs y=76]
Mechanism [inferred]: the field is filled `#FFFFFF` and the dark scanlines are then stroked
as 1-px lines whose endpoints are offset by one; an implementation that fills a 2-row
pattern will be 1 px off at both ends of every dark row. Flagged, not guessed.

### 2.3 Title text

| metric | rendered | golden @ coords |
|---|---|---|
| ink (active) | `#000000` | s8_doc_window_inactive "Applications" x=429..506, y=259..273 |
| text-gap fill | `#DADADA` | s8_doc_window_active light rows x=239..304 (66 px), dark rows x=240..305 |
| gap padding around the glyph run | 6 px left / 5 px right | active: black ink x=245..299 inside the light-row gap x=239..304 (this run also contains cursor pixels -- see the hazard note below; the padding figure is from the run's outer edges, which are glyph, not cursor) |
| glyph band (Charcoal 12) | ascender T+6, cap band T+8..T+14, descender to T+20 | Applications: ink rows y=259 (T+6) .. y=273 (T+20), cap mass y=261..267 |
| title placement | centred in the bar | both windows |

System font is **Charcoal** (not Chicago) -- `s8_controls_dialog.png` shows the Appearance
CP "System Font: Charcoal" popup. [verified: s8_controls_dialog x=313..449 y=192..211]

> **Capture hazard.** In `s8_doc_window_active.png` the arrow cursor sits ON the title
> text (approx x=257..266, y=76..90+). Use the "Applications" title in
> `s8_doc_window_inactive.png` / `s8_window_collapse.png` for clean title-text pixels.

---

## 3. Title-bar widgets (close / zoom / collapse)

### 3.1 Placement

Three widgets. **Close is leftmost; zoom and collapse are on the right, with COLLAPSE
RIGHTMOST.** Each widget is a 12x12 dark box plus a 1-px `#FFFFFF` outer highlight on its
right and bottom edges -> a 13x13 footprint.

Let `L` = struct left frame x, `R` = struct right frame x, `T` = title-bar top frame y.

| metric | rule | s8_doc_window_active (L=65,R=480,T=70) | s8_doc_window_inactive "Applications" (L=309,R=627,T=253) | s8_controls_dialog "Appearance" (L=121,R=516,T=53) |
|---|---|---|---|---|
| widget box top | `T+4` | y=74..85 | y=257..268 | y=57..68 |
| close box (12x12) | `L+4 .. L+15` | x=69..80 | x=313..324 | x=125..136 |
| zoom box (12x12) | `R-32 .. R-21` | x=448..459 | x=595..606 | **absent** |
| collapse box (12x12) | `R-16 .. R-5` | x=464..475 | x=611..622 | x=500..511 |
| outer highlight col / row | box right +1, box bottom +1 | x=81 / y=86 (close) | x=325 / y=269 | x=137 / y=69 |
| gap between right-hand footprints | 3 px `#DADADA` | x=461..463 | x=608..610 | n/a |
| `#DADADA` margin, widget footprint -> stripe field | 4 px / 5 px (row-parity, sec 2.2) | light rows x=82..85 and x=443..447; dark rows x=82..86 and x=444..447 | -- | -- |

The Appearance control panel window carries close + collapse but **no zoom box**, and its
title bar is otherwise identical -- the widget slots are per-window flags, and the collapse
box keeps the `R-16` slot when zoom is absent. [verified: s8_controls_dialog hscan y=63]

vs System 7: two widgets (close `left+9`, zoom `right-20`), 11x11 rendered, lavender bevel
-- see `../chrome/close-zoom-box.md`. Platinum adds the collapse box and moves everything.

### 3.2 Widget anatomy (identical for all three; only the glyph differs)

Close box from `s8_doc_window_active.png`, x=66..86 / y=72..88. `.`=surrounding stripe/face,
`a`=`#A5A5A5`, `x`=`#3F3F3F`, `W`=`#FFFFFF`, `d`=`#DADADA`, `b`=`#B3B3B3`, `c`=`#C0C0C0`,
`n`=`#CDCDCD`, `e`=`#E7E7E7`, `f`=`#F3F3F3`:

```
      66677777777778888888
      78901234567890123456
y= 73 dddddddddddddddddddd   <- title-bar face above the widget
y= 74 ddaaaaaaaaaaaadddddW   <- box row 0: #A5A5A5 top edge (x=69..80, 12 px)
y= 75 ddaxxxxxxxxxxxWddddd   <- #3F3F3F ring; #FFFFFF outer highlight col starts (x=81)
y= 76 ddaxWddddddddxWddddW
y= 77 ddaxdbbccnndaxWddddd   <- interior: the diagonal gradient
y= 78 ddaxdbccnnddaxWddddW
y= 79 ddaxdccnnddeaxWddddd
y= 80 ddaxdcnnddeeaxWddddW
y= 81 ddaxdnnddeefaxWddddd
y= 82 ddaxdnddeeffaxWddddW
y= 83 ddaxdddeeffWaxWddddd
y= 84 ddaxdaaaaaaaaxWddddW   <- interior bottom shadow row #A5A5A5
y= 85 ddaxxxxxxxxxxxWddddd
y= 86 dddWWWWWWWWWWWWddddd   <- #FFFFFF outer highlight row (x=70..81)
y= 87 dddddddddddddddddddd
```
(exact machine transcription of x=67..86. The alternating `W` / `d` in the last column is
x=86, the first pixel of the stripe field -- light on even rows, `#DADADA` on odd rows,
the row-parity offset of sec 2.2.)

| part | value | nominal | rule |
|---|---|---|---|
| top edge + left edge | `#A5A5A5` | `#888888` | box row 0 (12 px) and box col 0 (12 px) |
| dark ring | `#3F3F3F` | `#222222` | box rows/cols 1 and 11 |
| outer highlight | `#FFFFFF` | `#FFFFFF` | col box.right+1 (rows 1..12), row box.bottom+1 (cols 1..12) |
| interior top row + left col | `#DADADA`, corner pixel `#FFFFFF` | `#CCCCCC` / `#FFFFFF` | interior (0,0) is white |
| interior bottom row + right col | `#A5A5A5` | `#888888` | interior dx=8 col, dy=8 row |
| interior 7x7 face | diagonal gradient, see below | -- | keyed on `dx+dy`, box-relative |

**The Platinum diagonal gradient.** Inside the 9x9 interior, the 7x7 face at
`dx,dy = 1..7` is a 45-degree ramp that depends only on `dx+dy` (verified box-relative:
identical values at identical `(dx,dy)` in the close box at x=69 and the zoom box at
x=448, whose absolute coordinates differ by 379):

| `dx+dy` | 2,3 | 4,5 | 6,7 | 8,9 | 10,11 | 12,13 | 14 |
|---|---|---|---|---|---|---|---|
| sampled | `#B3B3B3` | `#C0C0C0` | `#CDCDCD` | `#DADADA` | `#E7E7E7` | `#F3F3F3` | `#FFFFFF` |
| nominal | `#999999` | `#AAAAAA` | `#BBBBBB` | `#CCCCCC` | `#DDDDDD` | `#EEEEEE` | `#FFFFFF` |

i.e. **the nominal ramp is exactly `0x99 + 0x11 * floor((dx+dy-2)/2)`** -- seven
consecutive entries of the standard 16-step gray ramp, two pixels per step, dark at the
upper-left corner and white at the lower-right. [verified: s8_doc_window_active close box
x=69..80 y=74..85 + zoom box x=448..459 y=74..85, pixel-for-pixel]

### 3.3 Glyphs

All glyph ink is `#3F3F3F` (the same value as the dark ring), drawn over the gradient.
Coordinates are interior-relative (`dx,dy` = 0..8, interior origin = box + (2,2)).

| widget | glyph | golden @ coords |
|---|---|---|
| **close** | none -- bare gradient face | s8_doc_window_active x=69..80 y=74..85 (no `#3F3F3F` inside the ring) |
| **zoom** | a 6x6 square in the upper-left, drawn as its RIGHT edge (`dx=5`, `dy=0..5`) and its BOTTOM edge (`dy=5`, `dx=0..5`) only; its top/left edges are the widget's own `#DADADA`/`#FFFFFF` interior highlight | s8_doc_window_active x=455 y=76..81 + y=81 x=450..455; cross-check s8_doc_window_inactive x=602 y=259..264 + y=264 x=597..602 |
| **collapse** | two full-interior-width dark rows at `dy=3` and `dy=5` (a "rolled shade": light row between them) | s8_doc_window_active y=79 and y=81, x=465..475; cross-check s8_doc_window_inactive y=262 and y=264, x=612..622 |

The collapse glyph is **identical whether the window is expanded or collapsed** (compare
`s8_doc_window_inactive.png` x=611..622 y=257..268 with `s8_window_collapse.png` at the
same rect). [verified: both goldens, byte-identical widget rects]

> In `s8_window_collapse.png` the arrow cursor overlaps the collapse widget's right half
> (from x~619). Use `s8_doc_window_inactive.png` for clean collapse-widget pixels.

---

## 4. Window body frame (the 4-px raised picture frame)

Between the outer black struct outline and the inner black line that bounds the content /
scroll gutters there is a **4-px raised bar**. Read in the +x direction (left AND right
border) and in the +y direction (top AND bottom border) the sequence is always the same:

```
  #FFFFFF  #DADADA  #DADADA  #B3B3B3
```

| edge | cross-section (outside -> inside) | golden @ coords |
|---|---|---|
| left | `#000000`(65) `#FFFFFF`(66) `#DADADA`(67,68) `#B3B3B3`(69) `#000000`(70) | s8_doc_window_active hscan y=150 |
| right | `#000000`(475) `#FFFFFF`(476) `#DADADA`(477,478) `#B3B3B3`(479) `#000000`(480) | s8_doc_window_active hscan y=150 |
| top | (the title bar occupies it: `#FFFFFF` T+1, `#DADADA` T+2..3 ... `#B3B3B3` T+20) | vscan x=120 y=71..90 |
| bottom | `#000000`(310) `#FFFFFF`(311) `#DADADA`(312,313) `#B3B3B3`(314) `#000000`(315) | vscan x=120 |

Note this is **not** a set of concentric frames: on the right border the white is on the
*inner* side and the `#B3B3B3` on the *outer* side, i.e. every border strip is lit as its
own raised bar (highlight on its top/left, shadow on its bottom/right).
[verified: s8_doc_window_active hscan y=150 + vscan x=120]

**Content inset.** Immediately inside the inner black line the content area carries a 1-px
`#FFFFFF` highlight on its top and left and a 1-px `#C0C0C0` shadow on its bottom and
right. Invisible on a white Finder content area; plainly visible on the `#E7E7E7` dialog
content of the Appearance CP: `hscan y=200` gives `#000000`(126) `#FFFFFF`(127) ...
`#C0C0C0`(510) `#000000`(511); `vscan x=300` gives ... `#C0C0C0`(338) `#000000`(339).
[verified: s8_controls_dialog]

---

## 5. Grow box (size box)

The bottom-right corner cell where the two 16-px scroll gutters meet. It has **no frame of
its own** -- its top and left are the gutters' black lines, and its bottom and right merge
into the window's frame bar.

| metric | rendered | golden @ coords |
|---|---|---|
| cell | x=461..478, y=296..313 (18 x 18) | s8_doc_window_active |
| bounding black lines | x=460 (col, y=296..309), y=295 (row, x=459..475) | grid x=459..481 y=293..315 |
| fill | `#DADADA` (nominal `#CCCCCC`) | x=462..478 y=297..313 |
| highlight | 1-px `#FFFFFF` on the cell's top row (y=296, x=461..476) and left col (x=461, y=296..311) | same grid |
| grip glyph | three 45-degree lines, `#FFFFFF` leading pixel + `#969696` trailing pixel, `#C0C0C0` where a line terminates lower-left; 4-px pitch | y=299..309, x=464..474 |

```
      444444444444444444
      666666666777777777        W=#FFFFFF  d=#DADADA
      123456789012345678        g=#969696  c=#C0C0C0
y=296 WWWWWWWWWWWWWWWWdd
y=297 Wddddddddddddddddd
y=298 Wddddddddddddddddd
y=299 WdddddddWWdddddddd
y=300 WddddddWdgdddddddd
y=301 WdddddWdgdWWdddddd
y=302 WddddWdgdWdgdddddd
y=303 WdddWdgdWdgdWWdddd
y=304 WddWdgdWdgdWdgdddd
y=305 WddcgdWdgdWdgddddd
y=306 WddddWdgdWdgdddddd
y=307 WddddcgdWdgddddddd
y=308 WddddddWdgdddddddd
y=309 Wddddddcgddddddddd
y=310 Wddddddddddddddddd
```
(exact machine transcription of x=461..478, y=296..310 in `s8_doc_window_active.png`.
Each grip line is a `#FFFFFF` leading pixel with a
`#969696` pixel one column to its right, stepping one column left per row; the pitch
between lines is 4 px and each line terminates lower-left with `#C0C0C0` + `#969696`.)

**Inactive delta:** the grow-box cell is flat `#E7E7E7` with **no grip lines and no
highlight** (`s8_doc_window_inactive` x=306..324, y=179..197 uniform `#E7E7E7`).
[verified: s8_doc_window_inactive grid x=300..330 y=172..201]

---

## 6. Inactive state (the full delta)

Measured on the deactivated "Initech81" window in `s8_doc_window_inactive.png`
(struct x=13..325, y=29..198).

| element | ACTIVE | INACTIVE | golden @ coords |
|---|---|---|---|
| struct outline + all internal frame lines | `#000000` | `#777777` (nominal `#555555`) | inactive x=13/325 cols, y=29/198 rows |
| drop shadow | `#000000` | `#777777` | inactive x=326 y=31..199; y=199 x=15..326 |
| title-bar fill | 12 stripe rows `#FFFFFF`/`#969696` + bevel | **flat `#E7E7E7`, 20 rows, no stripes, no bevel** | inactive vscan x=120: y=30..49 uniform `#E7E7E7` |
| title-bar height | 22 (y=29..50 incl.) | 22 (y=29..50 incl.) -- unchanged | inactive vscan x=120 |
| widgets | close / zoom / collapse drawn | **all three absent** (nothing but flat fill) | inactive hscan y=40 x=14..324: only title ink |
| title text ink | `#000000` | `#878787` (nominal `#666666`) | inactive hscan y=40, glyph run x=142..196 |
| body frame bar | `#FFFFFF`/`#DADADA`x2/`#B3B3B3` | **flat `#E7E7E7`** (4 rows/cols) | inactive vscan x=120 y=194..197 |
| scroll bars | arrows + track + thumb (see scrollbars.md) | **hollow**: `#F3F3F3` trough, `#777777` frame, no arrows, no separators, no thumb | inactive vscan x=312 y=72..177 uniform `#F3F3F3` |
| grow box | `#DADADA` + grip lines | flat `#E7E7E7`, no grip | inactive x=306..324 y=179..197 |

Note the two fills are *different grays*: the inactive title bar and the inactive frame
bar are `#E7E7E7` (nominal `#DDDDDD`), while the active frame bar face is `#DADADA`
(nominal `#CCCCCC`). [verified: s8_doc_window_inactive vs s8_doc_window_active]

---

## 7. Collapsed (windowshade) state

`s8_window_collapse.png`, the "Applications" window collapsed in place.

| metric | rendered | golden @ coords |
|---|---|---|
| collapsed struct rect | x=309..627, y=253..**274** (22 rows) | vscan x=400 y=248..285 |
| body below the title bar | **none** -- y=274 is the bottom black line, y=275 is the drop shadow | vscan x=400: y=274 `#000000`, y=275 `#000000`, y=276 desktop |
| title bar contents | byte-identical to the expanded window: same 22-row band, same stripes, same three widgets, same title text x=430..506 | compare hscan y=265 in s8_window_collapse vs s8_doc_window_inactive -- identical run list |
| widget glyphs | unchanged (collapse glyph does NOT invert or change) | x=595..622, y=257..268 in both |

So a collapsed window is exactly "the title bar, alone, plus its drop shadow": height 22,
shadow row at struct.bottom+1. [verified: s8_window_collapse + s8_doc_window_inactive]

---

## 8. What these captures do NOT answer (gaps)

- **Pressed / tracking widget art.** No golden shows a close/zoom/collapse box under the
  mouse button. System 7 has `s7_close_pressed.png` / `s7_zoom_pressed.png`; Platinum has
  no equivalent. [golden-resolves: a mouse-down capture on each widget]
- **Zoomed-state chrome.** No capture shows a window after clicking zoom, so any
  zoom-specific chrome delta (there is probably none) is unverified. [golden-resolves]
- **Drag outline / grow outline.** No capture during a drag or a resize, so the drag
  feedback (gray outline pattern, thickness) is unknown. [golden-resolves]
- **Title bar with a proxy icon / title SICN**, and **the "no title" and utility-window
  (`floatProc`) variants** -- not present. [golden-resolves]
- **Widget hit rects** (as opposed to drawn rects) cannot be measured from pixels at all;
  they need the CDEF/WDEF disassembly. [documented: n/a -- RE required]
- **Window header** (the `#E7E7E7` "7 items, 416.3 MB available" band, y=93..111 in
  s8_doc_window_active) is Finder content drawn with CDEF 21 "Window Header", not window
  chrome; deliberately out of scope here.

---

## Sources

Local goldens (all 640x480, PNG mode `P`, indexed-8, no lossy artefacts -- 84..140 distinct
colours per capture, every one an exact CLUT entry):

- `goldens/captures/s8_doc_window_active.png` -- active Finder window "Initech81".
  Struct x=65..480 y=70..315; title bar y=70..91; stripes y=74..85; close box x=69..80,
  zoom x=448..459, collapse x=464..475 (all y=74..85); v-gutter x=460..475; h-gutter
  y=295..310; grow box x=461..478 y=296..313; shadow x=481 / y=316.
- `goldens/captures/s8_doc_window_inactive.png` -- THE PAIR. Inactive "Initech81"
  x=13..325 y=29..198 (flat `#E7E7E7` bar, `#777777` frame, hollow bars) beside ACTIVE
  "Applications" x=309..627 y=253..420 (title bar y=253..274; widgets x=313..324 /
  595..606 / 611..622; enabled scroll bars).
- `goldens/captures/s8_window_collapse.png` -- "Applications" collapsed, x=309..627
  y=253..274 + shadow y=275.
- `goldens/captures/s8_controls_dialog.png` -- "Appearance" CP window x=121..516 y=53..344:
  third title-bar instance (22 rows y=53..74), close+collapse but NO zoom, and the
  `#E7E7E7` content area that exposes the content inset bevel.
- `goldens/captures/s8_81_monitors_256.png` -- proves the mint conditions: Colours=256,
  Gamma="Mac HiRes Std Gamma" (the reason for the sampled-vs-nominal split).
- `goldens/resources/CATALOG_s8.txt` + `goldens/resources/s8_wctb_0_System81.bin` --
  the shipped System 8.1 `wctb` 0 still carries the System-7 anchors (`#CCCCFF`,
  `#333366`); none of the Platinum grays above come from it. The chrome is drawn by
  `s8_CDEF_*_ApprExt81.bin` / the Appearance Extension WDEF with hard-coded values, so
  **the captures are the only ground truth for these RGBs.**

Method: all values produced by programmatic run-length scans (`vscan`/`hscan`) and pixel
grids over the PNGs via PIL; no value in this file was read off a screen by eye.

Certainty: all geometry and all RGBs [verified: golden @ the cited coords, each
cross-checked on >= 2 captures where the element appears -- title-bar band on 4 windows,
widget anatomy on 3, frame/shadow on 3, inactive delta on 1 window x 2 captures].
The stripe row-parity +1 offset is [verified: measured] with an [inferred] mechanism.
Residuals are listed in sec 8 and in [INDEX-sys8.md](INDEX-sys8.md).
