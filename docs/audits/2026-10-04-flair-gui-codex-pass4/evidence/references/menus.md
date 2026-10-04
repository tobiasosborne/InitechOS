# sys8/menus.md -- Mac OS 8.1 Platinum menu bar + pull-down panel

## Purpose + consumer

Two objects: the **menu bar** (a 20-px 3-D strip with rounded top corners, `#E7E7E7` face,
drawn by the Menu Manager, not by a WDEF) and the **pull-down panel** (a bevelled
`#E7E7E7` box with a 1-px `#3F3F3F` drop shadow, 16-px items, 6-px separator items, an
`#A5A5A5`+`#FFFFFF` etched separator groove, `#A5A5A5` disabled ink, and a right-hand
command-key column). Every number is measured from `s8_desktop_menubar.png` (idle bar) and
`s8_menu_dropped.png` (File pulled down); the bar geometry is additionally confirmed
identical in three more captures.

All RGBs are **as-sampled**; nominal (pre-gamma) values are given alongside -- see
[platinum-palette.md](platinum-palette.md) sec 1.

Cross-links: [platinum-palette.md](platinum-palette.md),
[window-chrome.md](window-chrome.md), [controls.md](controls.md),
[../chrome/menu-bar-geometry.md](../chrome/menu-bar-geometry.md) (System 7: flat WHITE
bar, no bevel, square corners -- a different era),
[../desktop/menu-bar.md](../desktop/menu-bar.md), [../toolbox/menu-manager.md](../toolbox/menu-manager.md).

---

## 1. Menu bar

### 1.1 Vertical structure

| row | sampled | nominal | role |
|---|---|---|---|
| y=0 | `#FFFFFF` | `#FFFFFF` | top highlight |
| y=1..17 | `#E7E7E7` | `#DDDDDD` | face (17 rows) |
| y=18 | `#B3B3B3` | `#999999` | bottom shadow |
| y=19 | `#000000` | `#000000` | baseline hairline, full 640-px width |
| y=20 | (desktop) | -- | first desktop row |

**Total bar height = 20 px** -- the same `MBarHeight` as System 7, but the fill is a 3-D
bar (`white / #E7E7E7 x17 / #B3B3B3 / black`) instead of System 7's flat white + black
hairline.

[verified: byte-identical `vscan x=300 y=0..21` in FIVE captures --
s8_desktop_menubar.png, s8_menu_dropped.png, s8_alert_modal.png, s8_81_desktop_8bpp.png,
s8_controls_dialog.png]

### 1.2 Rounded corners

The bar's top corners are rounded with a ~5-px radius, and the pixels **outside** the
round are `#000000` (the bar reads as a black-cornered strip against the desktop).

```
      00000000001111 1        K=#000000  s=#777777  c=#C0C0C0
      01234567890123 4        e=#E7E7E7  W=#FFFFFF  b=#B3B3B3
  y=0 KKKKKsceWWWWWWW
  y=1 KKKscWWWeeeeeee
  y=2 KKseWeeeeeeeeee
  y=3 KseWeeeeeeeeeee
  y=4 KcWeeeeeeeeeeee
  y=5 sWeeeeeeeeeeeee
  y=6 cWeeeeeeeeeeeee
  y=7 eWeeeeeeeeeeeee
  y=8 Weeeeeeeeeeeeee
```
(exact transcription of `grid x=0..14 y=0..8` in `s8_desktop_menubar.png`. The right
corner is the mirror: `grid x=626..639 y=0..8` gives `WWWWWWecsKKKKK` at y=0.)

The corner is smoothed with a 3-value ramp (`#777777`, `#C0C0C0`, `#CDCDCD`, `#A5A5A5`
appear as single corner pixels) -- deliberate corner shading in an indexed-8 buffer, not
JPEG-style artefacting (the capture has no lossy artefacts; see
[INDEX-sys8.md](INDEX-sys8.md)). [verified: s8_desktop_menubar grid, both corners]

### 1.3 Titles

| metric | rendered | golden @ coords |
|---|---|---|
| title ink (idle) | `#000000` | s8_desktop_menubar y=2..16 |
| glyph band | y=2..16 within the 17-row face | dark-pixel scan y=2..16 |
| Apple logo | full-colour rainbow apple, x=17..26 | hscan y=10: `#DA8700 #FFDAB3 #FF8700 x5 #DA8700 #B38754` |
| title ink runs (this bar) | File x=43..62, Edit x=78..100, View x=114..144, Special x=159..202, Help x=218..243 | black-ink column grouping over y=2..16 |
| right-hand cluster | application/help icons at x=542..592 and x=612..622 (colour, incl. `#545487`/`#8787DA`/`#DADAFF`) | same scan |

### 1.4 Pulled-down (highlighted) title

When a menu is open its title block is filled with the theme accent, not inverted to black:

| row | sampled | nominal | = `clut` 208 "Lavender" | golden @ coords |
|---|---|---|---|---|
| y=0 | `#8787DA` | `#6666CC` | entry 3 | s8_menu_dropped vscan x=50 |
| y=1..17 | `#5454B3` | `#333399` | entry 4 | vscan x=50 |
| y=18 | `#0000A5` | `#000088` | entry 5 | vscan x=50; hscan y=18 x=33..53 |
| y=19 | `#000000` | `#000000` | -- | the bar baseline, unchanged |
| title text | `#FFFFFF` | `#FFFFFF` | -- | hscan y=5, white glyph runs inside the block |

Block extent for "File": **x=33..72 (40 px)**, i.e. 10 px of padding either side of the
idle ink run x=43..62. [verified: s8_menu_dropped hscan y=3 -- `#5454B3` x=33..72]

That the three fill rows are consecutive entries 3/4/5 of the shipped `clut` 208
"Lavender" is an independent confirmation of the accent identification.
[verified: golden pixels + `goldens/resources/s8_clut_208_ApprCP81.bin`]

---

## 2. Pull-down panel

Reference: the File menu in `s8_menu_dropped.png`, 15 items in 4 groups.

### 2.1 Frame + shadow

| part | sampled | nominal | golden @ coords |
|---|---|---|---|
| panel rect (black outline) | x=33..230, y=19..278 (198 x 260) | -- | hscan y=100 / y=275; vscan x=36 / x=230 |
| top edge | `#000000` at y=19 -- **shared with the menu-bar baseline row** | `#000000` | vscan x=36 y=19 |
| inner highlight | `#FFFFFF`, 1 px on the top row (y=20) and left col (x=34) | `#FFFFFF` | vscan x=36 y=20; hscan y=100 x=34 |
| face | `#E7E7E7` | `#DDDDDD` | hscan y=100 x=35..228 |
| inner shadow | `#B3B3B3`, 1 px on the bottom row (y=277) and right col (x=229) | `#999999` | vscan x=36 y=277; hscan y=100 x=229 |
| bottom / right frame | `#000000` at y=278 and x=230 | `#000000` | both scans |
| **drop shadow** | **1 px `#3F3F3F`** (nominal `#222222`), right col x=231 and bottom row y=279 | -- | hscan y=100 x=231; vscan x=230 y=279 |

Note the menu's drop shadow is a **dark gray (`#3F3F3F`)**, not the solid black used for
window shadows ([window-chrome.md](window-chrome.md) sec 1). [verified: s8_menu_dropped]

### 2.2 Item metrics

Item text ink-band top rows, measured by a per-row ink profile over x=35..228:

```
  23  39  55  71  87 | 109 125 141 157 173 189 | 211 227 | 249 265
```

| metric | value | derivation / golden |
|---|---|---|
| **item height** | **16 px** | consecutive text tops differ by exactly 16 within every group |
| first item rect top | y=21 (= panel top frame + 2) | text top 23 = rect top + 2 |
| text top inside an item rect | +2 | 23-21, 39-37, ... |
| glyph cap band | 9 rows (e.g. y=23..31) | ink profile |
| **separator item height** | **6 px** | text top jumps by 22 (16+6) across each separator: 87->109, 189->211, 227->249 |
| separator groove | 2 px: `#A5A5A5` then `#FFFFFF`, at separator-rect rows +1 and +2 | y=102/103, 204/205, 242/243, each spanning the full interior x=35..228 |
| item text left edge | **x=53** (= panel left frame + 20) | ink column grouping, every enabled and disabled item |
| command-key column | starts x=199, right-most ink x=216 (14 px clear of the right frame) | "New Folder" cmd run x=199..216 |
| submenu arrow | x=208..213 (a solid right-pointing triangle) | the "Label" item, ink group (208,213) |

Panel height check: 15 items x 16 + 3 separators x 6 = 258; panel top frame y=19 + 2 +
258 = 279, i.e. the last item's nominal rect (y=263..278) has its final two rows covered
by the panel's own bottom bevel (`#B3B3B3` y=277) and frame (`#000000` y=278).
[verified: s8_menu_dropped -- the last item's text top y=265 is exactly rect+2]

### 2.3 Item ink

| item state | sampled | nominal | golden @ coords |
|---|---|---|---|
| enabled | `#000000` | `#000000` | "New Folder" y=23..31, "Get Info" y=109..117 |
| disabled | `#A5A5A5` | `#888888` | "Print" y=55..63, "Move To Wastebasket" y=71..79, "Close Window" y=87..95, "Duplicate" y=157..167, "Show Original" y=227..237 |
| command glyphs | same ink as the item (black enabled / `#A5A5A5` disabled) | -- | the cmd column dims with its item ("Print" cmd run x=199..215 is `#A5A5A5`) |
| separator groove dark | `#A5A5A5` | `#888888` | y=102, 204, 242 (194 px wide, uniform) |
| separator groove light | `#FFFFFF` | `#FFFFFF` | y=103, 205, 243 |

---

## 3. Gaps

- **Selected (tracking) menu item.** No golden shows an item under the cursor, so the
  item highlight -- accent fill like the title block, or classic invert -- is UNKNOWN.
  This is the single most load-bearing missing menu pixel.
  [golden-resolves: a capture with the mouse over an item]
- **Hierarchical (submenu) panel**: the "Label" item has a submenu arrow but the submenu
  is not open. [golden-resolves]
- **Checkmarked / icon / styled items**: none present in this File menu.
- **Apple menu, and a menu whose panel is wider than its title**: not captured.
- **A menu that reaches the screen bottom** (scrolling arrows) is not captured.
- **Per-title x-origins beyond this five-title bar** and the exact Charcoal advance
  widths are out of scope here (see `../fonts/`).

---

## Sources

- `goldens/captures/s8_desktop_menubar.png` -- idle bar. `vscan x=300 y=0..21` (structure),
  `grid x=0..14 y=0..8` and `x=626..639 y=0..8` (rounded corners), `hscan y=10 x=0..639`
  (Apple logo x=17..26, title runs), black-ink column grouping over y=2..16.
- `goldens/captures/s8_menu_dropped.png` -- File menu open. Panel x=33..230 y=19..278 +
  `#3F3F3F` shadow x=231 / y=279; highlighted "File" title block x=33..72 y=0..18;
  per-row ink profile x=35..228 y=20..278 (item tops 23/39/55/71/87 | 109/125/141/157/
  173/189 | 211/227 | 249/265; grooves 102-103, 204-205, 242-243); ink column groups
  (text left x=53, cmd column x=199..216, submenu arrow x=208..213).
- `goldens/captures/s8_alert_modal.png`, `s8_81_desktop_8bpp.png`,
  `s8_controls_dialog.png` -- three further identical `vscan x=300 y=0..21` menu-bar
  cross-sections.
- `goldens/resources/s8_clut_208_ApprCP81.bin` -- `clut` 208 "Lavender": the highlighted
  title block's three fills are entries 3/4/5 of this table.

Certainty: bar structure [verified: 5 captures, identical]; panel frame, item pitch,
separator height, inks [verified: s8_menu_dropped @ cited coords]; accent identification
[verified: golden pixels + extracted `clut` 208]. The selected-item appearance is a
[golden-resolves] gap.
