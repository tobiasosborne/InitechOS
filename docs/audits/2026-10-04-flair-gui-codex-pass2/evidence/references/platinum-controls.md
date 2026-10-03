# sys8/controls.md -- Mac OS 8.1 Platinum controls (checkbox, push button, default ring, popup, group box, help button, alert frame)

## Purpose + consumer

The Appearance Extension ships one `CDEF` per control family
(`goldens/resources/s8_CDEF_*_ApprExt81.bin`: 23 `b'Push Buttons'`, 25 `b'Popup Button'`,
10 `b'Group Box'`, 22 `b'List Box'`, 4 `b'DisclosureTriangles'`, ...), all with their
grays hard-coded. This spec measures the ones the two Appearance-control-panel captures
and the caution alert actually put on screen: **checkbox**, **etched group box**,
**popup button**, **push button + default ring**, **round help button**, and the
**modal alert frame** (which is NOT neutral gray -- see sec 6).

All RGBs are **as-sampled**; nominal (pre-gamma) values alongside -- see
[platinum-palette.md](platinum-palette.md) sec 1.

Cross-links: [window-chrome.md](window-chrome.md), [scrollbars.md](scrollbars.md),
[platinum-palette.md](platinum-palette.md),
[../toolbox/control-manager.md](../toolbox/control-manager.md),
[../toolbox/dialog-manager.md](../toolbox/dialog-manager.md),
[../chrome/dialog-borders.md](../chrome/dialog-borders.md) (System 7 dBox border -- a
different era).

---

## 1. Dialog / control-panel content area

The Appearance CP window (`s8_controls_dialog.png`, struct x=121..516, y=53..344) has a
standard Platinum title bar (see [window-chrome.md](window-chrome.md) sec 2) over an
`#E7E7E7` content area with its own 1-px inset bevel.

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| content face | `#E7E7E7` | `#DDDDDD` | hscan y=200 x=128..201 |
| content inset highlight (top + left) | `#FFFFFF` | `#FFFFFF` | x=127 (hscan y=200); y=75 (vscan x=300) |
| content inset shadow (bottom + right) | `#C0C0C0` | `#AAAAAA` | x=510 (hscan y=200); y=338 (vscan x=300) |
| bounding lines | `#000000` | `#000000` | x=126 / x=511; y=74 / y=339 |

---

## 2. Checkbox (CDEF 2/23 family)

**12 x 12**, a 1-px `#000000` square outline enclosing a 10x10 raised tile.

```
      223333333333       K=#000000  W=#FFFFFF  a=#A5A5A5
      890123456789       e=#E7E7E7  g=#969696  c=#C0C0C0
y=109 KKKKKKKKKKKK
y=110 KWWWWWWWWWeK
y=111 KWeKeeeeKeaK
y=112 KWeKKeeKKgaK
y=113 KWeeKKKKgcaK
y=114 KWeeeKKgceaK
y=115 KWeeKKKKeeaK
y=116 KWeKKgcKKeaK
y=117 KWeKgceeKgaK
y=118 KWeeceeeecaK
y=119 KeaaaaaaaaaK
y=120 KKKKKKKKKKKK
```
(exact transcription of x=228..239 in `s8_controls_dialog.png`, the checked
"Double-click title bar to collapse" box.)

| part | sampled | nominal | rule |
|---|---|---|---|
| outline | `#000000` | `#000000` | 1 px, all four sides of the 12x12 |
| interior highlight | `#FFFFFF` | `#FFFFFF` | interior row 0 (x=229..237) and col 0 (y=111..118) |
| interior shadow | `#A5A5A5` | `#888888` | interior row 9 (x=230..238) and col 9 (y=111..118) |
| face | `#E7E7E7` | `#DDDDDD` | the rest |
| check mark | `#000000` X with a `#969696` + `#C0C0C0` drop shade to its lower-right | `#000000` / `#777777` / `#AAAAAA` | rows y=111..118 |

Cross-checked pixel-for-pixel on the other two checkboxes in the same window
("Play sound when collapsing windows" y=134..145, "System-wide platinum appearance"
y=222..233) -- identical 12x12 art. Label text baseline is aligned with the box; label
ink `#000000`. [verified: s8_controls_dialog, three instances]

**Unchecked state is a GAP**: all three checkboxes in the goldens are checked.
[golden-resolves]

---

## 3. Etched group box (CDEF 10 "Group Box")

A 1-px `#A5A5A5` rectangle with a 1-px `#FFFFFF` companion offset (+1,+1) -- the classic
"etched in" groove -- whose top edge is interrupted by the group title.

| metric | rendered | golden @ coords |
|---|---|---|
| "Collapsing Windows" box | x=220..494, y=98..157 | hscan y=98; vscan x=220 y=98..157 |
| "Appearances" box | x=220..494, y=181..299 | vscan x=220 y=181..299 |
| groove dark line | `#A5A5A5` (nominal `#888888`) | (220,98) and the runs x=220..228 / x=362..494 at y=98 |
| groove light line | `#FFFFFF`, offset +1 in x and +1 in y | (221,99); left col x=221 |
| title gap in the top line | x=229..361 (133 px) for a title whose ink spans x=235..356 | hscan y=98 |
| title ink | `#000000`, glyph rows y=90..98 | hscan y=98 + grid x=216..240 y=84..100 |

So the title sits **on** the top groove line (its glyph band straddles y=90..98) and the
groove is simply not drawn across the gap. [verified: s8_controls_dialog, two group boxes]

---

## 4. Popup button (CDEF 25 "Popup Button")

The "System Font: Charcoal" popup. Outer rect **x=311..471, y=192..211 (161 x 20)**.

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| frame | `#000000` rounded rect: top/bottom rows x=314..468 at y=192/211; left/right cols x=312/x=470 at y=193..210 | `#000000` | hscan y=192, y=211; hscan y=193 |
| corner smoothing | `#3F3F3F` single pixels at (313,192) (469,192) (313,211) (469,211) and along x=311 / x=471 at y=194..209 | `#222222` | hscan y=192, y=194, y=209 |
| body highlight | `#FFFFFF`: top row y=193 (x=313..449), left col x=312 (y=194..209) | `#FFFFFF` | hscan y=193, y=194 |
| body face | `#E7E7E7` | `#DDDDDD` | hscan y=194 x=313..449 |
| body shadow | `#C0C0C0`: bottom row y=210 (x=313..450), right col x=450 (y=194..209) | `#AAAAAA` | hscan y=210, y=194 |
| **arrows well** (nested at the right) | x=451..470: `#FFFFFF` top row y=194 (x=452..468) + left col x=452; `#C0C0C0` at y=209 x=453..468; `#969696` at x=469..470 and y=210 x=452..469 | -- | hscan y=194, y=209, y=210 |
| arrow glyphs | `#000000`, two solid triangles pointing up and down, 7 px base | `#000000` | up: y=197..200 (apex x=460 at y=197, base x=457..463 at y=200); down: y=203..206 mirrored |
| label ink | `#000000` | -- | hscan y=200 x=320..373 |

The popup therefore reads as one 20-px-tall rounded button whose right ~20 px is a
separately bevelled arrows well. [verified: s8_controls_dialog, hscans y=192/193/194/209/
210/211 + grids x=308..326 and x=448..474]

A second popup ("Highlight Colour: Black & White",
`s8_controls_appearance_colour.png` rect **x=330..488, y=247..266**, again 159 x 20 with
the same `#000000` frame / `#FFFFFF` at y=248 and x=331 / `#C0C0C0` at y=265 and x=467 /
arrows well x=468..488) shows the same anatomy plus a colour swatch in the label area.
[verified: s8_controls_appearance_colour hscan y=258 + vscan x=400]

---

## 5. Push button + default ring (CDEF 23 "Push Buttons")

Reference: the caution alert's Cancel (plain) and OK (default) buttons in
`s8_alert_modal.png`.

### 5.1 Button

| metric | rendered | golden @ coords |
|---|---|---|
| Cancel rect | x=363..421, y=158..177 (**59 x 20**) | grids x=358..376 and x=412..428, y=155..181 |
| OK rect | x=435..493, y=158..177 (**59 x 20**) | hscan y=160; vscan x=450 |
| frame | `#000000` rounded rect, radius 2, corner pixels `#3F3F3F` / `#CDCDCD` | (365,158) `#3F3F3F`, (364,159) `#000000`, (363,160) `#3F3F3F` |
| inset row/col | `#E7E7E7` 1 px immediately inside the frame on the TOP and LEFT | y=159, x=364 |
| highlight | `#FFFFFF` 1 px: row y=160 (x=365..418) and col x=365 (y=161..174) | hscan y=160 |
| face | `#E7E7E7` (nominal `#DDDDDD`) | hscan y=168 |
| shadow ramp | `#C0C0C0` then `#969696`, 2 px on the BOTTOM and RIGHT, immediately inside the frame | rows y=175/176; cols x=419/420 |
| label ink | `#000000` | "Cancel" x=372..411, "OK" x=457..471, both y=163..171 (ink scan restricted to the face) |

So the Platinum push button is asymmetric: 1 px of face + 1 px white on the light side,
2 px of `#C0C0C0`/`#969696` on the dark side, all inside a rounded black frame.
[verified: s8_alert_modal, both buttons independently]

### 5.2 Default ring

| metric | rendered | golden @ coords |
|---|---|---|
| ring | 1-px `#000000` rounded rect at x=432..496, y=155..180 (**65 x 26**) | hscan y=160 (x=432, x=496); vscan x=450 (y=155, y=180) |
| relation to the button | `InsetRect(button, -3, -3)` -- 432 = 435-3, 496 = 493+3, 155 = 158-3, 180 = 177+3 | both scans |
| gap fill (2 px between ring and button) | top/left: `#E7E7E7` then `#C0C0C0`; bottom/right: `#C0C0C0` then `#969696` | vscan x=450 y=156,157 and y=178,179; hscan y=160 x=433,434 and x=494,495 |

i.e. the ring is a plain black outline and the 2-px moat between it and the button carries
the button's outer drop shadow. [verified: s8_alert_modal grids x=428..452 y=152..168 and
x=478..500 y=168..184]

**Pressed buttons are a GAP** -- no golden shows a button under the mouse.
[golden-resolves]

---

## 6. Modal alert frame -- the pink-tinted bevel (SURPRISE)

The caution alert's own border is **not** the neutral `#FFFFFF`/`#B3B3B3` bevel used by
document windows and dialogs. Its outer bevel ring is **pink-tinted**.

| metric | rendered | nominal | golden @ coords |
|---|---|---|---|
| alert struct rect | x=133..506, y=87..190 (374 x 104) | -- | hscan y=120, vscan x=300 |
| drop shadow | 1 px `#000000`, col x=507 (y=89..191), row y=191 (x=135..507) -- same (+1,+1) / +2-notch rule as a window | -- | vscan x=507, hscan y=191 |
| outer bevel, top + left | **`#FFB3B3`** | `#FF9999` | (134,120) and (300,88) |
| outer bevel, bottom + right | **`#FF8787`** | `#FF6666` | (505,120) and (300,189) |
| inner bevel, top + left | `#FFFFFF` | `#FFFFFF` | (135,120), (300,89) |
| inner bevel, bottom + right | `#B3B3B3` | `#999999` | (504,120), (300,188) |
| face | `#E7E7E7` | `#DDDDDD` | hscan y=120 x=136..160 |
| caution icon | full-colour yellow triangle, `#FFFF00` body, x=159..184 y=103..128 (26x26 icon cell) | -- | yellow-pixel bounding box |
| message ink | `#000000` | -- | hscan y=120 x=212..488 |

`#FF9999` / `#FF6666` are the pure-red-tinted analogues of `#FFFFFF` / `#999999`; the
alert border is the standard 2-ring Platinum bevel with the OUTER ring drawn in a red
tint. **Cause unresolved** [inferred: an alert/caution-specific tint in the Appearance
dialog CDEF, or a theme colour keyed to the alert kind]. The Appearance CP window in the
same session uses the neutral `#FFFFFF`/`#DADADA`/`#B3B3B3` frame, so this is specific to
the modal alert and not a session-wide setting. **Needs a second, non-caution alert
(note/stop) to decide whether the tint tracks the alert kind.**
[golden-resolves: a note alert and a stop alert]

---

## 7. Round help button

The `?` button at the CP's bottom left: a **21 x 20** tile at **x=220..240, y=311..330**.

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| frame, top + left | `#878787` | `#666666` | row y=311 x=220..239; col x=220 y=311..329 |
| frame, bottom + right | `#545454` | `#333333` | row y=330 x=221..240; col x=240 y=312..330 |
| off-diagonal corners | `#777777` at (240,311) and (220,330) | `#555555` | vscan x=220 / x=240, hscan y=311 / y=330 |
| inner ring | `#DADADA` | `#CCCCCC` | x=221 col, y=312 row |
| highlight | `#FFFFFF` | `#FFFFFF` | y=313 x=222..237; x=222 col |
| shadow | `#B3B3B3` then `#969696` | `#999999` / `#777777` | x=238..239 cols; y=328..329 rows |
| face | `#E7E7E7` / `#DADADA` graded | -- | interior |
| glyph | a yellow (`#FFFF00`, highlight `#FFFFDA`) help balloon with `#000000` and `#666666` detail | -- | x=224..237, y=314..327 |

Note this control's frame is drawn in **grays, not black** -- unique among the controls
measured here. [verified: s8_controls_dialog grid x=218..242 y=308..334]

---

## 8. Gaps

- **Unchecked checkbox**, **radio buttons** (none in these panes), **disabled controls**
  of any kind, and every **pressed/tracking** state. [golden-resolves]
- **Editable text fields, sliders, tabs, disclosure triangles, progress bars, little
  arrows, bevel buttons, placards, window headers, image wells** -- all shipped as
  Appearance CDEFs (see CATALOG_s8) but none are on screen in these captures.
- **List box** (the Colour/Options icon strip at x~136..196, y~85..220 and the accent
  picker in the colour pane) is only partially characterised here; its selection
  highlight is visible in `s8_controls_appearance_colour.png` (the selected "Lavender"
  cell is a black bar with white label at y=175..187) but its frame/scroll behaviour is
  not measured. The one solid datum: the SELECTED cell's label bar is a solid `#000000`
  bar (x=219..310, y=171..190) with `#FFFFFF` label text -- i.e. list selection uses the
  "Black & White" **Highlight Colour** setting shown in the same pane, not the accent
  colour. [verified: s8_controls_appearance_colour vscan x=265 y=171..190 + hscan y=180]
  [golden-resolves: the same list under a non-B&W highlight colour]
- **The keyboard-focus ring** around a focused control is not exercised.

---

## Sources

- `goldens/captures/s8_controls_dialog.png` -- Appearance CP, Options pane. Window
  x=121..516 y=53..344; content bevel x=127/510, y=75/338; checkboxes x=228..239 at
  y=109..120 / 134..145 / 222..233; group boxes x=220..494 at y=98..157 and y=181..299;
  popup x=311..471 y=192..211; help button x=221..240 y=311..330.
- `goldens/captures/s8_controls_appearance_colour.png` -- Appearance CP, Colour pane:
  second popup x=331..489 y=248..267, the "Black & White" highlight swatch
  (`#000000` field, `#FFFFFF` text, y=282..297), the accent list, and a third enabled
  horizontal scroll bar (see [scrollbars.md](scrollbars.md)).
- `goldens/captures/s8_alert_modal.png` -- caution alert x=133..506 y=87..190 + shadow;
  Cancel x=363..421 and OK x=435..493 (both y=158..177); default ring x=432..496
  y=155..180; caution icon x=159..184 y=103..128.
- `goldens/resources/CATALOG_s8.txt` + `s8_CDEF_*_ApprExt81.bin` -- the control defprocs
  by name/ID (drawing code; not disassembled here).

Certainty: all geometry + RGBs [verified: golden @ cited coords; checkbox on 3 instances,
push button on 2, popup on 2, group box on 2]. The alert's pink outer bevel is
[verified: measured on 4 separate edge samples] with an [inferred] and explicitly
unresolved cause.
