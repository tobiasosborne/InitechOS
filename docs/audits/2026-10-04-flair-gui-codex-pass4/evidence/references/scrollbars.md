# sys8/scrollbars.md -- Mac OS 8.1 Platinum scroll bars (CDEF 24 "Scroll Bars")

## Purpose + consumer

The Platinum scroll bar is a 16-px band (14-px interior between two 1-px black lines)
drawn by the Appearance Extension `CDEF` 24 (`goldens/resources/s8_CDEF_24_ApprExt81.bin`,
13425 bytes, resource name `b'Scroll Bars'`). Unlike the System 7 CDEF 1 it has **three
visually distinct states** in the goldens -- ENABLED (thumb + black arrows + a recessed
`#C0C0C0` page well), DISABLED (flat `#F3F3F3` trough, `#A5A5A5` arrows, `#777777`
separators, no thumb), and HOLLOW (inactive window: `#F3F3F3` trough with `#777777`
frame lines and *nothing* inside) -- and it draws the thumb in the **theme accent colour**
(Lavender, `clut` 208), not in gray.

All RGBs are **as-sampled**; see [platinum-palette.md](platinum-palette.md) sec 1 for the
gamma caveat and the nominal (pre-gamma) values, given here in the "nominal" columns.

Cross-links: [window-chrome.md](window-chrome.md) (the gutters + grow box that bound
these bars), [platinum-palette.md](platinum-palette.md),
[../chrome/scrollbar.md](../chrome/scrollbar.md) (the System 7 CDEF 1 predecessor -- a
*different* control: flat `#F3F3F3` boxes, `#969696` separators, non-proportional thumb).

---

## 1. Band geometry (all states)

| metric | rendered px | golden @ coords |
|---|---|---|
| total band width incl. both black lines | **16** | s8_doc_window_active vertical bar x=460..475; horizontal bar y=295..310 |
| interior width | **14** | x=461..474 / y=296..309 |
| bounding lines | 1 px `#000000` each side (active window) / `#777777` (inactive window) | x=460 and x=475 (`vscan x=462`); inactive x=305/320 |
| arrow box | **14 x 14**, one at each end (NOT both at one end) | top box y=113..126, bottom box y=281..294 (s8_doc_window_active) |
| page area | whatever remains between the arrow boxes and the thumb | -- |
| thumb length | **15 px** along the scroll axis, full 14-px interior across | s8_doc_window_inactive h-thumb x=330..344; v-thumb y=311..325 |

Arrow-box placement is **one arrow at each end** in every 8.1 golden here (no
"double-arrow" / smart-scrolling cluster). [verified: s8_doc_window_active v+h bars;
s8_doc_window_inactive v+h bars of the Applications window; s8_controls_appearance_colour
h bar]

---

## 2. ENABLED state

Reference: the Applications window in `s8_doc_window_inactive.png` (that window is the
ACTIVE one in the pair). Horizontal bar y=400..415 (interior y=401..414), x=314..607.
Cross-checked against the same window's vertical bar (x=607..622, interior x=608..621)
and against the Appearance colour pane's horizontal bar in
`s8_controls_appearance_colour.png` (interior y=191..204).

### 2.1 Layout of the horizontal bar

```
 x=314  |K|  window inner frame line
 x=315..328   left arrow box   (14)
 x=329  |K|
 x=330..344   THUMB            (15)   <- accent-coloured
 x=345  |K|
 x=346..591   page area        (246)  <- recessed #C0C0C0 well
 x=592  |K|
 x=593..606   right arrow box  (14)
 x=607  |K|
```
[verified: s8_doc_window_inactive hscan y=407 x=305..635]

### 2.2 Arrow box (enabled)

A **raised** 14x14 tile: `#FFFFFF` highlight on its top row and left column, `#CDCDCD`
shadow on its bottom row and right column, `#E7E7E7` face, solid `#000000` triangle.

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| top row + left col highlight | `#FFFFFF` | `#FFFFFF` | hscan y=401 x=315..327; vscan x=322 y=401 |
| face | `#E7E7E7` | `#DDDDDD` | hscan y=402 x=316..327 |
| bottom row + right col shadow | `#CDCDCD` | `#BBBBBB` | hscan y=414 x=316..328; y=402 x=328 |
| arrow glyph | `#000000` | `#000000` | left arrow x=320..323, y=404..411 |

Triangle: **8 px along the base, 4 px deep**, base widths 2/4/6/8 stepping by 2. For the
left arrow the apex column is x=320 (rows 407-408) and the base column is x=323 (rows
404..411); the up arrow is the same triangle rotated. [verified: s8_doc_window_inactive
grid x=314..348 y=399..416]

### 2.3 Page area (the recessed well)

Cross-axis section of the horizontal bar (top -> bottom, 14 rows):

| rows | sampled | nominal | role |
|---|---|---|---|
| +0 | `#969696` | `#777777` | well shadow, outer |
| +1 | `#A5A5A5` | `#888888` | well shadow, inner |
| +2..+11 | `#C0C0C0` | `#AAAAAA` | well fill (10 rows) |
| +12 | `#CDCDCD` | `#BBBBBB` | well highlight, inner |
| +13 | `#DADADA` | `#CCCCCC` | well highlight, outer |

[verified: s8_doc_window_inactive vscan x=500 y=401..414; identical cross-section on the
Appearance colour pane, s8_controls_appearance_colour vscan x=300 y=191..204]

The vertical bar is the same well rotated: left column `#969696`, then `#A5A5A5`, 10
columns `#C0C0C0`, then `#CDCDCD`, `#DADADA` (s8_doc_window_inactive hscan y=350
x=608..621).

**Along-axis ends.** The 2-px dark inset also appears at the *near* end of each page
rect (x=346,347 = `#969696`,`#A5A5A5` immediately right of the thumb; y=327,328 on the
vertical bar immediately below its thumb) but there is **no** light inset at the far end
(x=590,591 are `#C0C0C0`; y=383,384 are `#C0C0C0`). Recorded as measured; mechanism
[inferred] -- the well is stroked dark on top+left of its rect and light on bottom+right,
with the far along-axis edge left unstroked.
[verified: s8_doc_window_inactive grid x=585..596 y=400..415 and x=344..356 y=400..415;
v-bar grid x=606..623 y=378..388]

### 2.4 Thumb (accent-coloured)

15 px along the axis, spanning the full 14-px interior across, bounded by a `#000000`
line at each end.

| part | sampled | nominal | = `clut` 208 "Lavender" entry | golden @ coords |
|---|---|---|---|---|
| leading highlight (top row / left col) | `#DADAFF` | `#CCCCFF` | entry 1 | h-thumb y=401 x=331..343; v-thumb x=608 col |
| face | `#B3B3FF` | `#9999FF` | entry 2 | h-thumb y=402..413 x=331..343 |
| trailing shadow (bottom row / right col) | `#8787DA` | `#6666CC` | entry 3 | h-thumb y=414 x=331..344 |
| grip lines (4) | `#5454B3` | `#333399` | entry 4 | h-thumb x=334,336,338,340 at y=405..411 (7 px) |
| grip companion highlight | `#DADAFF` | `#CCCCFF` | entry 1 | h-thumb x=333,335,337,339 at y=405..410 (6 px) |

The grip is **4 dark lines** perpendicular to the scroll axis at x = thumb.left + 4, +6,
+8, +10, each 7 px long (thumb rows 4..10), each with a `#DADAFF` companion line one pixel
to its left that is 6 px long and capped by a single `#F3F3F3` pixel at thumb row 3
(y=404, x=333/335/337/339).

```
      33333333333333333
      33333344444444444          H=#DADAFF  P=#B3B3FF  R=#5454B3
      45678901234567890          S=#8787DA  f=#F3F3F3  K=#000000
y=400 KKKKKKKKKKKKKKKKK
y=401 KfHHHHHHHHHHHHHPK
y=402 KHPPPPPPPPPPPPPSK
y=403 KHPPPPPPPPPPPPPSK
y=404 KHPPfPfPfPfPPPPSK
y=405 KHPPHRHRHRHRPPPSK
y=406 KHPPHRHRHRHRPPPSK
y=407 KHPPHRHRHRHRPPPSK
y=408 KHPPHRHRHRHRPPPSK
y=409 KHPPHRHRHRHRPPPSK
y=410 KHPPHRHRHRHRPPPSK
y=411 KHPPPRPRPRPRPPPSK
y=412 KHPPPPPPPPPPPPPSK
y=413 KHPPPPPPPPPPPPPSK
y=414 KPSSSSSSSSSSSSSSK
y=415 KKKKKKKKKKKKKKKKK
```
(exact transcription of x=329..345, y=400..415 in `s8_doc_window_inactive.png`.)

Vertical-thumb cross-check: `vscan x=616 y=310..326` gives `#000000`(310)
`#DADAFF`(311) `#B3B3FF`(312,313) `#DADAFF`/`#5454B3` alternating (314..321)
`#B3B3FF`(322..324) `#8787DA`(325) `#000000`(326) -- same anatomy, grip lines horizontal.
[verified: s8_doc_window_inactive]

That the thumb ramp is *exactly* entries 1-4 of the shipped `clut` 208 "Lavender"
(`goldens/resources/s8_clut_208_ApprCP81.bin` = `#EEEEEE #CCCCFF #9999FF #6666CC #333399
#000088 #000055 #000000`) is an independent, non-pixel confirmation of both the accent
identification and the gamma bijection. [verified: golden pixels + extracted resource]

---

## 3. DISABLED state (active window, nothing to scroll)

Both bars of the "Initech81" window in `s8_doc_window_active.png` are disabled: the folder
contents fit the view. This is NOT the same as the inactive-window hollow bar.

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| trough / arrow-box face (entire interior) | `#F3F3F3`, flat, no bevel, no well | `#EEEEEE` | v-bar x=461..474, y=113..294 (uniform except the marks below) |
| arrow-box separator lines | `#777777`, 1 px, full interior width | `#555555` | v-bar y=127 and y=280 (x=461..474); h-bar x=85 and x=445 |
| arrow glyphs | `#A5A5A5` solid triangle, same 8x4 shape | `#888888` | up arrow y=118..121 (widths 2/4/6/8, apex row y=118 at x=467,468; base row y=121 at x=464..471) |
| thumb | **absent** | -- | no accent pixels anywhere in the band |

So the disabled bar keeps the arrow-box *partitioning* (via the `#777777` separators) but
drops every bevel: there is no `#FFFFFF`/`#CDCDCD` arrow-box edging and no `#C0C0C0` well.
[verified: s8_doc_window_active grid x=458..478 y=108..132 and y=276..295; h-bar hscan
y=303 x=64..482]

---

## 4. HOLLOW state (inactive window)

The deactivated "Initech81" window in `s8_doc_window_inactive.png`.

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| bounding lines | `#777777` (not black) | `#555555` | v-bar x=305 and x=320 |
| interior | `#F3F3F3`, completely empty -- no arrows, no separators, no thumb | `#EEEEEE` | vscan x=312 y=72..177 = 106 rows uniform `#F3F3F3` |

[verified: s8_doc_window_inactive vscan x=312; grid x=300..330 y=172..201]

---

## 5. State summary

| feature | ENABLED | DISABLED | HOLLOW |
|---|---|---|---|
| bounding lines | `#000000` | `#000000` | `#777777` |
| trough fill | `#C0C0C0` well (5-value cross-section) | `#F3F3F3` flat | `#F3F3F3` flat |
| arrow box | raised `#FFFFFF`/`#E7E7E7`/`#CDCDCD` tile | no bevel, `#F3F3F3` | absent |
| arrow glyph | `#000000` | `#A5A5A5` | absent |
| box separators | 1-px `#000000` (part of the layout) | 1-px `#777777` | absent |
| thumb | accent `clut` 208 ramp, 15 px, 4 grip lines | absent | absent |

---

## 6. Gaps

- **Pressed states**: no golden shows an arrow, a page region, or a thumb under the
  mouse. The pressed arrow (inverted? darkened face?) is unknown.
  [golden-resolves: mouse-down captures]
- **Proportional thumb**: both enabled thumbs measured are 15 px, and both bars happen to
  have a large scrollable range, so it is not provable from these pixels whether 8.1's
  thumb is proportional or fixed at 15. The System 7 thumb was fixed 16x16.
  [golden-resolves: a window with a short scrollable range]
- **Non-Lavender accents**: only the Lavender theme was captured; the other 18 `clut`
  200..218 accent tables are extracted but not rendered.
  [golden-resolves: re-capture with a second accent selected]
- **"Smart scrolling" (both arrows at one end)** is not exercised.
- **Live-scroll / proportional-thumb tracking** feedback is not capturable from stills.

---

## Sources

- `goldens/captures/s8_doc_window_inactive.png` -- ENABLED bars: Applications window
  h-bar y=400..415 (arrows x=315..328 / 593..606, thumb x=330..344, well x=346..591) and
  v-bar x=607..622 (arrows y=296..309 / 386..399, thumb y=311..325, well y=327..384).
  Also the HOLLOW bars of the inactive Initech81 window (v-bar x=305..320).
- `goldens/captures/s8_doc_window_active.png` -- DISABLED bars: v-bar x=460..475
  y=112..295 (separators y=127/280, `#A5A5A5` arrows y=118..121 / 286..289); h-bar
  y=295..310 x=70..460 (separators x=85/445, arrows x=76..79 / 451..454).
- `goldens/captures/s8_controls_appearance_colour.png` -- a third ENABLED h-bar
  (y=190..205, thumb x=349..363, well cross-section identical) confirming the well and
  thumb anatomies outside a Finder window.
- `goldens/resources/s8_clut_208_ApprCP81.bin` -- `clut` 208 "Lavender", the accent ramp
  the thumb is painted from (independent of any pixel).
- `goldens/resources/CATALOG_s8.txt` -- `CDEF` 24 `b'Scroll Bars'` (Appearance Extension)
  is the drawing code; not disassembled here.

Certainty: geometry + all RGBs [verified: golden @ cited coords, each state cross-checked
on >= 2 bars, and the thumb ramp additionally [verified: extracted `clut` 208 resource]].
The missing far-end well highlight is [verified: measured] with an [inferred] mechanism.
