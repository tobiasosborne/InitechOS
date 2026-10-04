# Control Manager -- ControlRecord, part-codes, CDEF proc IDs (specs/toolbox)

The Control Manager owns the on-screen *controls* a window contains: push
buttons, check boxes, radio buttons, and -- the load-bearing case for the Office
Space frame -- scroll bars and the determinate progress bar. This spec is the
ground truth behind FLAIR's `os/flair/control.{c,h}` (795 LOC), the CDEF
dispatch that draws each control type and hit-tests it, and the `test-control`
oracle (143 checks). It carries the verbatim Inside Macintosh `ControlRecord`
field layout, the FindControl/TestControl part-codes, the standard CDEF
definition IDs, and the value/min/max/hilite/track semantics -- every numeric
constant here is a PUBLISHED Inside Macintosh constant (Law 4 `[verified: A+B]`),
none is `[golden-resolves]`. The rendered pixels of each control (button corner
arcs, scroll-bar arrow glyphs, thumb fill, the inactive-grey dimming) ARE
pixel-exact and stay `[golden-resolves]`; this spec records WHICH state drives
them, not their RGB. Sibling specs: the scroll-bar chrome geometry lives in
[../chrome/scrollbar.md](../chrome/scrollbar.md); the grow/size box (a Window
Manager gadget, NOT a Control Manager control) in
[../chrome/grow-box.md](../chrome/grow-box.md); the consolidated part-code
catalog in [./manager-part-codes.md](./manager-part-codes.md); the window that
owns a control's coordinate space in
[./window-manager.md](./window-manager.md); dialogs that embed controls via DITL
in [./dialog-manager.md](./dialog-manager.md).

Target era (Law 3): System 7.0/7.1 color chrome. The standard button-family CDEF
(resID 0) and scroll-bar CDEF (resID 1) are the System 7 procs; the
System 7.5+/Appearance-Manager `kControl*` redraw model and `kControlNoPart`
naming are a LATER era and are tagged as deltas, never imported silently.

## ControlRecord layout

Verbatim Inside Macintosh record. `[verified: A+B]` -- cached first-hand in
`refs/im-toolbox/im-toolbox-records-verbatim.md` (dev.os9.ca Toolbox-317,
fetched) cross-checked against IM Vol I Ch 10 p. I-318. It is a `PACKED RECORD`;
offsets below are the classic 68K Mac in-memory layout (Pascal field order, no
inter-field padding -- `PACKED`). `[inferred]` offsets are arithmetic from the
documented field order + classic type sizes (Handle/Ptr/ProcPtr = 4, Rect = 8,
INTEGER = 2, Byte = 1, LONGINT = 4, Str255 = 256); the FIELD ORDER and TYPES are
`[verified: A+B]`, the byte offsets are `[inferred]` arithmetic.

| field          | offset | size | type           | notes |
|----------------|--------|------|----------------|-------|
| nextControl    | 0      | 4    | ControlHandle  | next control in the owning window's control list (singly-linked) |
| contrlOwner    | 4      | 4    | WindowPtr      | the control's window (its coordinate space) |
| contrlRect     | 8      | 8    | Rect           | bounding rectangle, in `contrlOwner` LOCAL coords |
| contrlVis      | 16     | 1    | Byte           | 255 if visible, 0 if hidden |
| contrlHilite   | 17     | 1    | Byte           | highlight state: 0 = no highlight; a part-code = that part pressed; 255 = inactive/dimmed |
| contrlValue    | 18     | 2    | INTEGER        | current setting (in `[contrlMin, contrlMax]`) |
| contrlMin      | 20     | 2    | INTEGER        | minimum setting |
| contrlMax      | 22     | 2    | INTEGER        | maximum setting |
| contrlDefProc  | 24     | 4    | Handle         | handle to the CDEF (control definition function) |
| contrlData     | 28     | 4    | Handle         | CDEF private data (e.g. scroll-bar thumb region) |
| contrlAction   | 32     | 4    | ProcPtr        | default action procedure (see TrackControl) |
| contrlRfCon    | 36     | 4    | LONGINT        | application reference value |
| contrlTitle    | 40     | 256  | Str255         | control's title (length byte + up to 255 chars) |

`ControlHandle = ^ControlPtr; ControlPtr = ^ControlRecord.` Total in-memory size
= 296 bytes. `[inferred]` arithmetic.

Value/min/max semantics by control type `[verified: A+B; refs/im-toolbox cached
+ MacTech ControlsFORTRAN]`:
- Scroll bar: `contrlValue` ranges over `[contrlMin, contrlMax]`; the thumb
  position is `contrlValue` mapped proportionally across the track. An empty or
  whole-document scroll bar sets `min == max` (disabled appearance).
- Check box / radio button: `contrlValue` is 0 (off) or 1 (on). `contrlMin = 0`,
  `contrlMax = 1`.
- Push button: value/min/max are unused (0).

## FindControl / TestControl part-codes

Returned by `FindControl` and `TestControl`; also the value stored in
`contrlHilite` while a part is pressed. `[verified: A+B]` -- two independent
sources confirmed this session (MacTech ControlsFORTRAN + dev.os9.ca
ControlMgrRef), cached in `refs/im-toolbox/im-toolbox-records-verbatim.md`;
source IM Vol I Ch 10 p. I-327.

| name        | value | meaning |
|-------------|-------|---------|
| inButton    | 10    | in a push button (push-button body only) |
| inCheckBox  | 11    | in a check box OR a radio button -- one code covers both (the check box / radio body returns 11, NOT inButton=10) |
| inUpButton  | 20    | scroll-bar up (or left) arrow |
| inDownButton| 21    | scroll-bar down (or right) arrow |
| inPageUp    | 22    | scroll-bar paging region ABOVE/left of the thumb |
| inPageDown  | 23    | scroll-bar paging region BELOW/right of the thumb |
| inThumb     | 129   | scroll-bar thumb (the draggable indicator / "scroll box") |

Notes:
- `0` = the point is NOT in any active control (FindControl returns 0 and a NULL
  control). `[verified: A+B]`
- A point in an INACTIVE control (`contrlHilite == 255`) returns 0 from
  FindControl -- inactive controls are not hit. `[documented: IM-I p. I-327]`
- `inThumb = 129` is deliberately high so the thumb-drag case is distinguishable
  from the auto-tracking arrow/page parts (10..23), which is why TrackControl
  treats it specially (live drag vs repeating action). `[verified: A+B]`

ERA DELTA (Law 3): the System 7.5+/Appearance Manager adds `kControlNoPart = 0`
and renames these as `kControlButtonPart` etc.; the NUMERIC values 10/11/20/21/
22/23 are preserved, and `inThumb`'s indicator part becomes
`kControlIndicatorPart = 129`. The System 7.0/7.1 spelling above is the target.
`[documented: IM-I + later-header naming]`

## Standard CDEF definition IDs

`defID = CDEF_resID * 16 + variation_code`. `[verified: A+B]` -- two sources,
cached; source IM Vol I Ch 10 p. I-322.

| name          | value | CDEF resID | variant | meaning |
|---------------|-------|------------|---------|---------|
| pushButProc   | 0     | 0          | 0       | standard push button (button-family CDEF, resID 0) |
| checkBoxProc  | 1     | 0          | 1       | standard check box (same CDEF, variant 1) |
| radioButProc  | 2     | 0          | 2       | standard radio button (same CDEF, variant 2) |
| scrollBarProc | 16    | 1          | 0       | standard scroll bar (scroll-bar CDEF, resID 1) |
| useWFont      | 8     | --         | +8      | variation BIT OR'd in: draw the title in the window's font instead of the system font |

So the button family (push/check/radio) is ONE CDEF resource (ID 0) selected by
the low variation bits 0/1/2; the scroll bar is a SEPARATE CDEF (ID 1), giving
`scrollBarProc = 1*16 + 0 = 16`. `[verified: A+B]`

ERA DELTA: `popupMenuProc = 1008` (the System 7 pop-up-menu CDEF, defID
1008 = 63*16) and its `popupPrivateData` record exist in System 7 but are NOT in
the Office Space frame and NOT implemented in FLAIR M4. Recorded for the constant
catalog only. `[documented: refs/im-toolbox Toolbox-319 fetch]`

There is NO standard "progress bar" CDEF in System 7 -- the determinate progress
indicator the FLAIR FILE COPY box shows is a FLAIR-internal control type, not an
Apple CDEF. `[verified: research/raw/s7-toolbox.md Sec 4 + ADR-0004 D-3]`

## Semantics

### Drawing and hilite
- `DrawControls(theWindow)` draws every control in a window's control list;
  `Draw1Control(theControl)` draws one. `[documented: IM-I Ch 10]`
- `HiliteControl(theControl, hiliteState)` sets `contrlHilite` and redraws.
  `hiliteState` is a part-code (0..129) to show that part PRESSED, `0` to show
  the control normal, or **255 to make the control INACTIVE/dimmed** (drawn
  greyed; not hit-testable). `[verified: A+B; WebSearch "value 255 signifies the
  control is to be made inactive" + IM-I]` The exact dimmed pixels (which CLUT
  indices the grey text/outline use) are `[golden-resolves]`.
- Control title color: the standard CDEFs draw the title using `wTextColor`
  (index 2) from the window color table; the inactive/dimmed shades draw from
  `wHiliteColorLight`(5)/`wHiliteColorDark`(6). The RGBs are `[golden-resolves]`
  -- see [../resources/wctb-mctb-format.md](../resources/wctb-mctb-format.md)
  (wctb ID -4096). `[documented: refs/im-toolbox/wctb-window-color-table-partcodes.txt]`

### Hit-testing and tracking
- `FindControl(point, theWindow, &whichControl) -> partCode` finds the
  front-most active control under `point` and returns its part-code (0 = none).
  `[documented: IM-I p. I-326]`
- `TestControl(theControl, point) -> partCode` tests one control. `[documented:
  IM-I p. I-327]`
- `TrackControl(theControl, startPoint, actionProc) -> partCode` runs the
  mouse-tracking loop while the button is held; returns the part-code the mouse
  was released over (0 if released outside the original part). `[verified: A+B;
  IM-I + WebSearch]`
  - For BUTTON-LIKE parts (push button, check box, radio body) TrackControl just
    highlights the part while the mouse is inside and returns the part if
    released inside. `[documented: IM-I]`
  - For scroll-bar ARROWS and PAGING regions, TrackControl repeatedly calls the
    `actionProc` (a `ProcPtr` taking `(theControl, partCode)`) as long as the
    button is held -- this is how a held scroll arrow scrolls continuously.
    `[verified: A+B; WebSearch "TrackControl ... repeatedly performing some
    action as long as the user holds down the mouse button"]`
  - For the THUMB (`inThumb = 129`), TrackControl does LIVE DRAG: it moves the
    indicator with the mouse and, on release, sets `contrlValue` proportionally
    to the final thumb position, clamped to `[contrlMin, contrlMax]`. The
    `actionProc` is conventionally `NIL` for the thumb (the app reads the new
    value after TrackControl returns). `[verified: A+B; IM-I + MacTech]`
  - Passing `actionProc = Pointer(-1)` tells TrackControl to use the control's
    own `contrlAction` field. `[documented: IM-I p. I-328]`

### Value accessors and geometry
- `GetControlValue/SetControlValue`, `GetControlMinimum/SetControlMinimum`,
  `GetControlMaximum/SetControlMaximum` read/write the int16 value/min/max,
  redrawing the indicator on change (the scroll thumb moves, the check/radio
  glyph toggles). `[documented: IM-I Ch 10]`
- `NewControl(window, boundsRect, title, visible, value, min, max, procID,
  refCon) -> ControlHandle` allocates and links a control; `GetNewControl(resID,
  window)` builds from a `'CNTL'` resource. `DisposeControl` unlinks/frees;
  `KillControls(window)` frees all. `[documented: IM-I Ch 10]`
- `MoveControl(c, h, v)` / `SizeControl(c, w, h)` reposition/resize (update
  `contrlRect`); `DragControl` drags within a window. `[documented: IM-I Ch 10]`

### Scroll-bar value<->thumb mapping (the load-bearing invariant)
The thumb Y (for a vertical bar) is `contrlValue` mapped linearly onto the track
between the two arrows: `thumbPos = arrowTop + (value-min)/(max-min) *
trackSpan`, and the inverse (thumb drag -> value) is the round-trip-invertible
mapping, with the result CLAMPED to `[min, max]`. `[documented: IM-I "scroll bar
value/thumb proportional over [min,max]"; FLAIR test-control THUMB_OFF/NO_CLAMP
mutants enforce this]` The track geometry (16px width, arrow boxes, thumb size)
is chrome -- see [../chrome/scrollbar.md](../chrome/scrollbar.md); the rendered
arrow glyph and thumb fill pixels are `[golden-resolves]`.

## FLAIR mapping and deliberate deviations

FLAIR's `os/flair/control.{c,h}` keeps the LOAD-BEARING verbatim fields
(`contrlRect`, `contrlValue`, `contrlMin`, `contrlMax`, `contrlHilite`) with
`_Static_assert`-checked part-code and proc-ID values, and deviates structurally
for the freestanding / no-heap-handle model (ADR-0004 DEC-03, Law 3). `[verified:
research/raw/s7-toolbox.md Sec 4 -- control.h read]`

| IM field / mechanism | FLAIR counterpart | why |
|----------------------|-------------------|-----|
| `contrlDefProc` (Handle to CDEF) | built-in CDEF dispatch keyed by `flair_ctrl_type` enum {pushButton, checkBox, radioButton, scrollBar, progressBar} | no resource fork / no loadable defprocs |
| `contrlData` (Handle) | FLAIR-internal per-type state | no heap handle table |
| `contrlAction` (stored ProcPtr) | OMITTED as a field; `TrackControl` takes an action FUNCTION parameter | freestanding; matches the common `actionProc != -1` call form |
| `contrlVis/contrlOwner/contrlTitle/nextControl` | FLAIR-internal handling | structural |
| (no Apple progress CDEF) | `progressBar` enum member; filled fraction = `contrlValue/contrlMax` | the comedic "Saving tables to disk..." FILE COPY centerpiece (ADR-0004 D-3) |

The part-codes (10/11/20/21/22/23/129), proc IDs (0/1/2/16), and value/min/max/
hilite semantics are kept EXACTLY and asserted at build time. The progress bar is
a documented FLAIR addition, not an Apple control. `[verified: research/raw/
s7-toolbox.md Sec 4]`

## Golden-resolves (pixel/byte items deferred to a mint)

NONE of the records/part-codes/proc-IDs are golden-resolves -- they are published
constants. The PIXEL-exact items this spec points at (owned by chrome / resource
specs, not by record layout):
- Push-button rounded-rect corner arc radius + outline pixels -- chrome render
  `[golden-resolves]`.
- Check-box square + check glyph, radio-button circle + dot bitmaps --
  `[golden-resolves]` (CDEF render).
- Scroll-bar up/down arrow glyph bitmaps, thumb fill, track pattern -- see
  [../chrome/scrollbar.md](../chrome/scrollbar.md); `[golden-resolves]`.
- Inactive (`hilite == 255`) dimmed-text / dimmed-outline CLUT indices, and the
  control-title color RGB (wTextColor / wHiliteColor* from wctb -4096) --
  `[golden-resolves]`, see [../resources/wctb-mctb-format.md](../resources/wctb-mctb-format.md).

## Sources

Local (authoritative, Law 1):
- `refs/im-toolbox/im-toolbox-records-verbatim.md` -- verbatim ControlRecord,
  part-codes, CDEF proc IDs (first-hand fetched-and-cached: dev.os9.ca
  Toolbox-317; MacTech ControlsFORTRAN; dev.os9.ca ControlMgrRef).
- `refs/im-toolbox/wctb-window-color-table-partcodes.txt` -- wTextColor /
  wHiliteColor part indices for control title + dimmed shades.
- `research/raw/s7-toolbox.md` Sec 4 (Control Manager) + Sec 8/9 -- the research
  brief: FLAIR mapping, deviations, certainty ledger.
- `research/system7-gui-ground-truth.md` Sec 1.5, 4.4 -- scroll-bar 16px width
  context, ControlRecord summary, test-control mutant list.
- `specs/OUTLINE.md` -- the row for this spec (scope, FLAIR consumer, golden).
- `CLAUDE.md` -- the Laws + table conventions.

Cross-checked (web, this session; authority remains the local cache above):
- Inside Macintosh Vol I Ch 10 "The Control Manager" via dev.os9.ca mirror
  (Toolbox-317 "The Control Record"; ControlMgrRef table of contents).
- MacTech Vol.03.04 "Controls From FORTRAN" (part-code / proc-ID confirmation).
- WebSearch: HiliteControl "value 255 signifies the control is to be made
  inactive"; TrackControl action-procedure repeating semantics
  (macgui.com All About Scrolling Part 2; dev.os9.ca Toolbox-317).
