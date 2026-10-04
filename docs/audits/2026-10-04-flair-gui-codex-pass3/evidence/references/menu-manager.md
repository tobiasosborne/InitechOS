# Menu Manager -- MenuInfo, item attributes, enableFlags, MenuSelect result, MDEF

The Menu Manager owns the menu data structure (`MenuInfo`), the menu bar (the
20px strip across the top of the screen), per-item attributes (text, mark char,
command-key equivalent, style, icon, divider), the per-menu/per-item enable
bitfield (`enableFlags`), and the pull-down tracking that returns the chosen
menu+item. It delegates the drawing + hit-testing of a pulled-down panel to a
menu definition function (MDEF). This spec carries the verbatim Inside Macintosh
`MenuInfo` record, the item-attribute model, the `enableFlags` bit assignment,
the standard mark / style / MDEF-message constant catalogs, and the
`MenuSelect`/`MenuKey` result-word packing. The FLAIR consumer is `../initech-os`
`os/flair/menu.{c,h}` (a struct-refactored `MenuInfo`) + `spec/assets/menu_canon.h`
(the frozen InitechPaint bar string); it verifies against the menu-bar + pull-down
screendumps (G9) and the `canon` byte oracle and `test-menu`.

This is one subsystem. The native-pixel menu-bar height + fill + Apple glyph +
item rendering are chrome/desktop, owned by `../chrome/menu-bar-geometry.md` and
`../desktop/menu-bar.md` / `../desktop/apple-menu.md` -- NOT this spec. The menu
color table (`mctb`) that a color MDEF reads is `../resources/wctb-mctb-format.md`;
the `MENU` template resource format is `../resources/wind-menu-dlog-ditl.md`; the
compiled MDEF resource layout is `../resources/wdef-mdef-cdef-format.md`. The
Chicago-12 system font the bar + items draw in is `../fonts/chicago.md`. Sibling
Manager specs: `window-manager.md`, `control-manager.md`, `dialog-manager.md`,
`event-manager.md`, and the consolidated `manager-part-codes.md`.

Target precision (Law 3): System 7.0/7.1 COLOR chrome. The Menu Manager RECORD,
the `MenuSelect` result word, the part-codes, the mark/style/message constants,
and `enableFlags` are UNCHANGED from System 6 through 7.x -- they are published
constants. The color additions (a color MDEF reading an `mctb` for shade indices,
the Apple-menu glyph in color) are 7.x rendering details; the System 6 menu was
flat B&W with the SAME record + constants. Era deltas tagged inline. The pop-up
menu CDEF (`popupMenuProc=1008`) is a System-7 addition and is flagged but OUT OF
the frame's scope.

--------------------------------------------------------------------------------
## 1. MenuInfo (the menu data structure)

VERBATIM Inside Macintosh layout. A `MenuHandle` is a handle to a `MenuInfo`; the
title characters (and, historically, the packed per-item data) live in the
variable-length `menuData` field at the end, so the record is allocated to fit.
[verified: A+B; refs/im-toolbox/im-toolbox-records-verbatim.md MenuInfo +
research/raw/s7-toolbox.md Sec 3; IM-I Ch 13 p. I-344; RetroTechCollection wiki]

Pascal declaration (verbatim, source order):

```
TYPE MenuInfo =
  RECORD
     menuID:      INTEGER;   {menu ID (>=1)}
     menuWidth:   INTEGER;   {menu width in pixels when drawn}
     menuHeight:  INTEGER;   {menu height in pixels when drawn}
     menuProc:    Handle;    {handle to the menu definition procedure (MDEF)}
     enableFlags: LONGINT;   {bit 0 = title; bits 1..31 = items 1..31}
     menuData:    Str255;    {menu title text + (historically) packed item data}
  END;
MenuHandle = ^MenuPtr;   MenuPtr = ^MenuInfo;
```

Field table (68K layout; `Handle` is a 4-byte 68K pointer; `INTEGER`=2,
`LONGINT`=4; `Str255` is a length byte + up to 255 chars, but for the TITLE the
Menu Manager allocates only "title length + 1" bytes, then appends the packed
item data after it). Offsets are the classic 68K offsets up to `menuData`; the
verbatim ORDER is the authority, the offsets are [inferred] arithmetic and are not
load-bearing for FLAIR (which re-types the record, Sec 6).

| field       | offset | size | type    | notes |
|-------------|--------|------|---------|-------|
| menuID      | 0      | 2    | INTEGER | menu's ID (>=1); the HIGH word of the result, Sec 4 |
| menuWidth   | 2      | 2    | INTEGER | drawn width in px (measured from the widest item) |
| menuHeight  | 4      | 2    | INTEGER | drawn height in px (item count x item height) |
| menuProc    | 6      | 4    | Handle  | the MDEF (resID 0 == standard pull-down), Sec 5 |
| enableFlags | 10     | 4    | LONGINT | per-title/per-item enable bitfield, Sec 3 |
| menuData    | 14     | var  | Str255  | title chars + packed item records (variable length) |

[offsets: inferred from published field sizes; the verbatim ORDER is the authority]

`menuData` is the variable-length tail: IM allocates "only the storage necessary
for the title characters plus 1" for the title, then appends the per-item packed
data (text, optional icon ID, optional keyboard equivalent, optional mark char,
optional style byte) after the title. Live per-item attributes are read/written
via the accessor calls of Sec 2, not by poking `menuData` directly.
[verified: A+B; im-toolbox-records-verbatim.md "menu data" note + IM-I p. I-345]

--------------------------------------------------------------------------------
## 2. Menu items + per-item attributes

Each item in a menu carries, in its packed `menuData` entry:

| attribute | accessor (get/set)        | meaning |
|-----------|---------------------------|---------|
| text      | GetMenuItemText/SetMenuItemText | the item's display string |
| mark      | GetItemMark / SetItemMark | leading mark char (check / diamond / blank), Sec 2.1 |
| cmdChar   | GetItemCmd / SetItemCmd   | command-key equivalent char (0 == none); `MenuKey` matches it, Sec 4 |
| style     | GetItemStyle / SetItemStyle | QuickDraw text style byte (bold/italic/...), Sec 2.2 |
| icon      | GetItemIcon / SetItemIcon | optional icon resource (drawn at the item's left) |
| enable    | EnableItem / DisableItem  | per-item enable bit in `enableFlags`, Sec 3 |

[verified: A+B; im-toolbox-records-verbatim.md item-attributes note +
research/raw/s7-toolbox.md Sec 3; IM-I p. I-345]

DIVIDER: a menu item whose text begins with the two characters `(-` is a
DISABLED gray separator line (a "divider"). It is never selectable and is drawn as
a horizontal gray rule spanning the menu width. `AppendMenu`/the `MENU` resource
uses `(-` as the divider marker; the live item is permanently disabled.
[verified: A+B; im-toolbox-records-verbatim.md + research/raw/s7-toolbox.md Sec 3]
The leading `(` (without `-`) disables an item; other metacharacters in
`AppendMenu` strings (`/` cmd-key, `!` mark, `<` style, `^` icon, `;`/return item
separator) set the corresponding attribute. [documented: IM-I AppendMenu syntax]

### 2.1 Item mark characters

The mark is a single character drawn in the left margin of the item (Mac Roman).
`SetItemMark(menu, item, markChar)`; `markChar = noMark (0)` clears it.

| name        | value | hex   | meaning |
|-------------|-------|-------|---------|
| noMark      | 0     | 0x00  | no mark (blank left margin) |
| commandMark | 17    | 0x11  | the command (cloverleaf/propeller) glyph (Chicago control-range 0x11) |
| checkMark   | 18    | 0x12  | the check mark (the common "this item is on" mark) |
| diamondMark | 19    | 0x13  | the diamond mark |
| appleMark   | 20    | 0x14  | the Apple-logo glyph (Chicago control-range 0x14; the Apple-menu title) |

[verified: A+B; im-toolbox-records-verbatim.md (check=0x12, diamond, blank) +
WebSearch cross-check (commandMark=17/checkMark=18/diamondMark=19/appleMark=20)].
The mark char is any Mac Roman char, not just these five; these are the named
constants. The Apple glyph (0x14) is the chrome detail owned by
`../desktop/apple-menu.md`; its rendered bitmap is [golden-resolves].

### 2.2 Item style byte (QuickDraw Style)

The per-item style is a QuickDraw `Style` -- a set stored as a byte; each bit is
one face attribute. `SetItemStyle(menu, item, styleByte)`.

| name      | bit | value | hex   |
|-----------|-----|-------|-------|
| (plain)   | --  | 0     | 0x00  |
| bold      | 0   | 1     | 0x01  |
| italic    | 1   | 2     | 0x02  |
| underline | 2   | 4     | 0x04  |
| outline   | 3   | 8     | 0x08  |
| shadow    | 4   | 16    | 0x10  |
| condense  | 5   | 32    | 0x20  |
| extend    | 6   | 64    | 0x40  |

[verified: A+B; QuickDraw Style is shared with `txFace` in
refs/im-toolbox/im-toolbox-records-verbatim.md TERec (`txFace: Style`) + WebSearch
cross-check (bold=bit0..extend=bit6)]. Same byte the Control/Window managers use
for title styles; defined once by QuickDraw (`../quickdraw/` text model).

--------------------------------------------------------------------------------
## 3. enableFlags (the per-menu/per-item enable bitfield)

`enableFlags` is a 32-bit field. Bit 0 enables the whole menu (the TITLE in the
bar); bit i (1..31) enables item i. Items numbered above 31 are ALWAYS enabled
(there are only 31 maskable bits after the title bit). A disabled menu draws its
bar title dimmed and cannot be pulled down; a disabled item draws dimmed and is
never returned by `MenuSelect`/`MenuKey`.
[verified: A+B; im-toolbox-records-verbatim.md MenuInfo `enableFlags` comment +
research/raw/s7-toolbox.md Sec 3; IM-I p. I-344]

| bit  | mask        | controls |
|------|-------------|----------|
| 0    | 0x00000001  | the menu TITLE (whole menu enabled/disabled) |
| 1    | 0x00000002  | item 1 |
| 2    | 0x00000004  | item 2 |
| ...  | (1 << i)    | item i |
| 31   | 0x80000000  | item 31 |
| 32+  | (none)      | items > 31: ALWAYS enabled (no bit) |

Entry points: `EnableItem(menu, item)` sets the bit (`item==0` enables the whole
menu -- bit 0); `DisableItem(menu, item)` clears it (`item==0` disables the menu).
A divider (Sec 2) is permanently disabled regardless of its bit.
[verified: A+B; IM-I p. I-344 + im-toolbox-records-verbatim.md]

ERA NOTE: a disabled item dims by drawing the text in a gray (50% stipple) pattern
in System 6 B&W and via the menu/system dimmed color in System 7 color; the BIT
SEMANTICS are identical. The exact dim rendering is [golden-resolves] (G9), owned
by `../desktop/menu-bar.md`. [Law 3 delta: documented]

--------------------------------------------------------------------------------
## 4. MenuSelect / MenuKey result word

`MenuSelect(startPt)` runs the pull-down tracking loop (the user pressed in the
menu bar at `startPt`); `MenuKey(ch)` resolves a command-key press. Both return a
`LONGINT` whose HIGH word is the chosen `menuID` and LOW word is the 1-BASED item
number; 0 means nothing was chosen.
[verified: A+B; im-toolbox-records-verbatim.md MenuSelect result +
research/raw/s7-toolbox.md Sec 3; IM-I Ch 13 + os/flair/menu.h]

```
result = (menuID << 16) | item        item is 1-based; 0 == no selection
menuID  = (result >> 16) & 0xFFFF     (HiWord)
item    = result & 0xFFFF             (LoWord; 1-based, 0 == nothing chosen)
```

| field    | bits      | meaning |
|----------|-----------|---------|
| menuID   | high word (31..16) | the chosen menu's `menuID` (Sec 1); 0 == nothing |
| item     | low word (15..0)   | 1-based item number; 0 == nothing chosen |

After handling the result the app calls `HiliteMenu(0)` to un-highlight the bar
title. `MenuKey` only matches ENABLED items whose `cmdChar` (Sec 2) equals the
typed char; a disabled item / disabled menu yields 0. A click that lands on a
disabled item or releases outside any menu also yields 0.
[verified: A+B; IM-I Ch 13 + research/raw/s7-toolbox.md Sec 3]

`MenuSelect` is reached from the event loop when `FindWindow` returns
`inMenuBar=1` (see `window-manager.md` Sec 2 and `manager-part-codes.md`); the
Menu Manager itself defines no FindWindow-style part-codes -- the menu bar is a
single hit region and item resolution happens inside the MDEF tracking loop.
[verified: cross-link window-manager.md + im-toolbox-records-verbatim.md]

--------------------------------------------------------------------------------
## 5. MDEF -- menu definition function (messages)

The MDEF draws the pulled-down panel, sizes it, and hit-tests which item the mouse
is over. The standard pull-down MDEF is resource ID 0. The Menu Manager calls it
with a message selector:
[verified: A+B; im-toolbox-records-verbatim.md (MDEF resID 0) + WebSearch
cross-check of the message values + research/raw/s7-toolbox.md Sec 3; IM-I Ch 13]

| message    | value | action |
|------------|-------|--------|
| mDrawMsg   | 0     | draw the menu (the pulled-down panel + all items) |
| mChooseMsg | 1     | given a mouse point, highlight + report the item under it |
| mSizeMsg   | 2     | calculate the menu's `menuWidth`/`menuHeight` |
| mPopUpMsg  | 3     | calculate the rectangle for a pop-up menu box (IM Vol V, pre-System-7) |

[verified: A+B; WebSearch (mDrawMsg=0..mPopUpMsg=3) + im-toolbox-records-verbatim.md
MDEF note]. The MDEF signature is `MenuDefProc(message, theMenu, menuRect, hitPt,
whichItem)`. `mPopUpMsg=3` predates System 7 (`PopUpMenuSelect`, IM Vol V) and is reused by the
System-7 pop-up CDEF path (Sec 7); the classic pull-down uses messages 0..2.
[documented: IM Vol V PopUpMenuSelect; Law 3 delta] The compiled MDEF resource layout is owned by
`../resources/wdef-mdef-cdef-format.md`; the rendered item geometry (item height,
text margins, mark/cmd columns) is [golden-resolves] (G9), owned by
`../desktop/menu-bar.md`. The standard MDEF (like the standard WDEF/CDEF) is NOT
present in the leaked System 7 source -- it is a ROM resource. [documented:
research/system7-gui-ground-truth.md]

--------------------------------------------------------------------------------
## 6. The menu bar (geometry pointer + entry points)

The menu bar is a separate 20px strip across the top of the main screen, NOT drawn
by any window WDEF and NOT part of the menu pull-down panel. Its height is
`GetMBarHeight()` == 20 px for the Roman script system.
[documented: research/system7-gui-ground-truth.md Sec 1.1 (`menubar_height=20`,
[verified: S1+IM-128]); IM Toolbox-128]. The native-pixel bar height + fill +
item spacing live in `../chrome/menu-bar-geometry.md` + `../desktop/menu-bar.md`;
this spec restates 20px only for the menu-bar context. The fill pattern, the Apple
glyph bitmap, and the bar-title rendering are [golden-resolves] (G9).

Menu-bar + menu management entry points (semantics, not records):

| routine                | semantics |
|------------------------|-----------|
| InitMenus              | initialize the Menu Manager + allocate the menu bar |
| NewMenu / GetMenu      | create a menu (NewMenu builds empty; GetMenu loads a `MENU` resource) |
| AppendMenu / InsertMenuItem | add items (with the `(-`, `/`, `!`, `<`, `^` metachars, Sec 2) |
| InsertMenu             | add a menu to the menu list (at `beforeID` or at the end / hierarchical) |
| DrawMenuBar            | redraw the whole 20px bar (titles from the menu list) |
| ClearMenuBar          | empty the menu list |
| MenuSelect / MenuKey   | the tracking + command-key paths (Sec 4) |
| HiliteMenu             | highlight (or `HiliteMenu(0)` un-highlight) a bar title |
| EnableItem/DisableItem | flip an `enableFlags` bit (Sec 3) |
| CalcMenuSize          | invoke the MDEF `mSizeMsg` to set `menuWidth`/`menuHeight` |
| GetMenuHandle/GetMHandle | look up a `MenuHandle` by `menuID` |

[verified: A+B; IM-I Ch 13 Menu Manager + research/raw/s7-toolbox.md Sec 3]. The
menu list is the Menu Manager's internal ordered set of installed `MenuHandle`s
(driving `DrawMenuBar`); FLAIR keeps an equivalent menu array.

--------------------------------------------------------------------------------
## 7. FLAIR mapping + deliberate deviations

FLAIR consumer: `../initech-os` `os/flair/menu.{c,h}` (~463 LOC) +
`spec/assets/menu_canon.h`. `MenuSelect`/`MenuKey` return `(menuID << 16) | item`
(1-based item) VERBATIM; `MenuResult`/`MenuResultID`/`MenuResultItem` helpers do
the packing/unpacking. [verified: A+B; os/flair/menu.h read (via
research/raw/s7-toolbox.md Sec 3) + im-toolbox-records-verbatim.md]

DELIBERATE DEVIATION (freestanding / no-malloc; preserves IM field NAMES + the
load-bearing result-word + constant VALUES): FLAIR's `MenuInfo` is REFACTORED away
from the packed `Str255 menuData` + `LONGINT enableFlags` blob into an exploded
struct. It carries:

| IM field/concept              | FLAIR form | why |
|-------------------------------|------------|-----|
| menuID: INTEGER               | menuID (int16, verbatim name) | same value; HIGH word of result |
| menuWidth: INTEGER            | menuWidth (cached, verbatim name) | computed from widest item |
| menuHeight: INTEGER           | DROPPED (computed from item count) | no stored height needed |
| menuProc: Handle (MDEF)       | DROPPED (built-in MDEF dispatch) | no heap handle table; MDEF is built in |
| menuData: Str255 (title+items)| title: const char* + items[] array | no Pascal-packed blob; typed struct |
| enableFlags: LONGINT          | per-item `enabled` byte + `is_divider` | same semantics, exploded out of the bitfield |
| (per item attrs)              | MenuItem{text,mark,cmdChar,style,enabled,is_divider} | the SAME attrs IM packs (Sec 2), exploded |

[verified: research/raw/s7-toolbox.md Sec 3 (os/flair/menu.h read) +
research/system7-gui-ground-truth.md Sec 4.3]. The exploded form is SEMANTICALLY
EQUIVALENT to the IM packed form: a disabled-or-divider item is never selectable
(`MenuInfo_item_selectable` enforces the Sec 3 + Sec 2 divider rule). The per-item
`mark`/`cmdChar`/`style` carry the Sec 2 / Sec 2.1 / Sec 2.2 constants directly.

CANON (FROZEN, do-not-correct -- ADR-0004 AM-4): the InitechPaint menu-bar string
`File Edit Image Layer Select View Window Help` is locked in
`spec/assets/menu_canon.h`, gated by the `canon` byte oracle. A Mac-located menu
bar carrying a Photoshop item set is the DELIBERATE chimera anachronism
(research/system7-gui-ground-truth.md Sec 4.3 / Part 8 G9) -- do NOT "correct" it
to a real System 7 Finder bar. [verified: ADR-0004 AM-4 + research brief]

Verification: `test-menu` (41 checks; `FIXED_WIDTH` / `SELECT_DISABLED` mutants
bite -- proportional item layout, the 20px bar, dividers, disabled items); the G9
menu-bar + pull-down screendumps confirm rendering; the `canon` oracle byte-checks
the bar string. [documented: research/raw/s7-toolbox.md Sec 3 (recon-flair-impl)]

POP-UP (out of scope, flagged): System 7 added a pop-up menu CDEF
`popupMenuProc=1008` (a Control, not a bare menu) with a `popupPrivateData`
record `{mHandle, mID, mPrivate}`; the MDEF `mPopUpMsg=3` (Sec 5) serves it. NOT
needed for the Office Space frame and NOT implemented in FLAIR M4.
[documented: research/raw/s7-toolbox.md Sec 3 (Toolbox-319 fetch)]

--------------------------------------------------------------------------------
## 8. Certainty ledger

- [verified: A+B] `MenuInfo` 6-field verbatim layout + `enableFlags` semantics
  (bit 0 title, bits 1..31 items, >31 always on); the `MenuSelect`/`MenuKey`
  result word `(menuID<<16)|item` (1-based); the divider `(-` rule; the item
  attribute model (text/mark/cmdChar/style/icon); MDEF resID 0.
- [verified: A+B, local + web cross-check] mark constants (noMark=0,
  commandMark=0x11, checkMark=0x12, diamondMark=0x13, appleMark=0x14); Style bits
  (bold=1..extend=0x40); MDEF messages (mDrawMsg=0..mPopUpMsg=3).
- [documented: single src] `GetMBarHeight()==20` (IM Toolbox-128; restated from
  the chrome brief Sec 1.1 -- it is a CHROME metric, owned by
  `../chrome/menu-bar-geometry.md`, not a record claim here);
  `popupMenuProc=1008` + `popupPrivateData` (Toolbox-319; out of FLAIR scope).
- [golden-resolves] ALL rendered menu pixels -- the 20px bar fill, the Apple glyph
  bitmap (Chicago control-range 0x14), item height + mark/cmd-key column geometry, the
  disabled-item dim rendering, and pull-down panel drawing. These are G9, owned by
  `../desktop/menu-bar.md` + `../chrome/menu-bar-geometry.md` + `../fonts/chicago.md`.
  This record/constant spec contains NO pixel claims to resolve.
- [inferred] 68K field offsets in Sec 1 (arithmetic from published field sizes;
  not load-bearing -- the verbatim ORDER is the authority); the FLAIR
  drop of `menuHeight`/`menuProc` (read from the brief's account of menu.h).

CONTRADICTION CHECK: none found between local sources. The cached records-verbatim
gives the mark as "check 0x12 / diamond / blank" without the full named-constant
set; the web cross-check supplies the complete commandMark=0x11..appleMark=0x14
catalog and agrees on checkMark=0x12 and diamondMark=0x13. No conflict -- the local
file is a subset the web fills out, with the overlap matching exactly.

--------------------------------------------------------------------------------
## Sources

LOCAL (authority):
- refs/im-toolbox/im-toolbox-records-verbatim.md -- `MenuInfo` verbatim Pascal
  record; `enableFlags` bit semantics; the item-attribute note (text/mark/cmdChar/
  style/icon, `(-` divider); the `MenuSelect`/`MenuKey` result-word packing; MDEF
  resID 0. Also the QuickDraw `Style` byte via TERec `txFace`. (first-hand cached)
- refs/im-toolbox/wctb-window-color-table-partcodes.txt -- the color-table model a
  color MDEF would read (cross-link; the `mctb` RGBs are golden-resolves, owned by
  `../resources/wctb-mctb-format.md`).
- research/raw/s7-toolbox.md Sec 3 (Menu Manager) -- the research brief + the FLAIR
  `os/flair/menu.h` mapping + struct-refactor deviation + `test-menu` + canon.
- research/system7-gui-ground-truth.md Sec 1.1 (menubar_height=20), Sec 4.3 (Menu
  Manager restatement + canon), Part 8 G9 (menu-bar/pull-down golden backlog),
  Part 9 (era discipline).

WEB (cross-check only; LOCAL files above are the authority):
- Inside Macintosh Vol I, Menu Manager (p. I-344 MenuInfo, I-345 menu data/items).
- Inside Macintosh: Macintosh Toolbox Essentials, Menu Manager chapter
  (mark constants, MDEF messages, AppendMenu metacharacters).
- dev.os9.ca / preterhuman.net techpubs Menu Manager mirror (Toolbox-89 ff.) --
  MDEF messages mDrawMsg=0/mChooseMsg=1/mSizeMsg=2/mPopUpMsg=3.
- InformIT / Carbon Menu Manager constants -- mark constants noMark=0/
  commandMark=17/checkMark=18/diamondMark=19/appleMark=20 (the hex 0x11..0x14).
- QuickDraw.p / IM QuickDraw text -- Style bits bold(0)..extend(6).

ASCII-clean.
