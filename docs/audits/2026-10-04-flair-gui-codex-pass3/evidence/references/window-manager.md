# Window Manager -- WindowRecord, FindWindow part-codes, WDEF dispatch, damage model

The Window Manager owns the window data structure (`WindowRecord`), the z-order
window list, hit-testing (`FindWindow`), and the update/damage machinery that
drives `updateEvt`. It delegates all chrome drawing + per-window hit geometry to a
window definition function (WDEF). This spec carries the verbatim Inside Macintosh
record layout, the `FindWindow` part-code catalog, the WDEF variant + message
dispatch constants, and the System-7 damage (update region) model. The FLAIR
consumer is `../initech-os` `spec/window_record.h` (LOCKED, build-time
`_Static_assert`s on every constant here) and `os/flair/window.c`; it verifies
against the chrome screendumps, `test-window`, and the drag-gate (`test-drag`).

This is one subsystem. The native-pixel chrome geometry the WDEF draws lives in
`../chrome/title-bar.md`, `../chrome/close-zoom-box.md`, `../chrome/grow-box.md`,
`../chrome/window-frame.md`, and `../chrome/wdef-variant-geometry.md`; the window
color table that feeds the WDEF is `../resources/wctb-mctb-format.md`. The
GrafPort embedded at offset 0 is specified in `../quickdraw/grafport.md`; the
regions the WDEF computes are `../quickdraw/regions-api.md`. Sibling Manager specs:
`control-manager.md`, `menu-manager.md`, `dialog-manager.md`, `event-manager.md`,
and the consolidated `manager-part-codes.md`.

Target precision (Law 3): System 7.0/7.1 COLOR chrome. The color additions
(`wctb`, the color WDEF reading shade indices) are 7.x; System 6 used a flat B&W
WDEF with the same record layout and part-codes. Tagged inline where it matters.

--------------------------------------------------------------------------------
## 1. WindowRecord (the window data structure)

VERBATIM Inside Macintosh layout. `port` is at offset 0 -- this is the
load-bearing invariant: a `WindowPtr` IS a `GrafPtr`, so `SetPort(theWindow)`,
`DrawControls(theWindow)`, etc. all work on the same storage.
[verified: refs/im-toolbox/im-toolbox-records-verbatim.md WindowRecord +
refs/IM_Tb_TypesOfWindows.txt; IM-I p. I-268; MTE Ch 4 p. 4-78..4-81]

Pascal declaration (verbatim, source order):

```
TYPE WindowRecord =
  RECORD
     port:          GrafPort;     {window's grafport -- FIRST, offset 0}
     windowKind:    INTEGER;      {window class}
     visible:       BOOLEAN;      {TRUE if visible}
     hilited:       BOOLEAN;      {TRUE if highlighted (active)}
     goAwayFlag:    BOOLEAN;      {TRUE if has a close (go-away) box}
     spareFlag:     BOOLEAN;      {reserved (zoom available)}
     strucRgn:      RgnHandle;    {structure region (whole window)}
     contRgn:       RgnHandle;    {content region}
     updateRgn:     RgnHandle;    {update region (area to be redrawn)}
     windowDefProc: Handle;       {window definition function (WDEF)}
     dataHandle:    Handle;       {data used by windowDefProc}
     titleHandle:   StringHandle; {window's title}
     titleWidth:    INTEGER;      {title width in pixels}
     controlList:   ControlHandle;{window's control list}
     nextWindow:    WindowPeek;   {next window in z-order list}
     windowPic:     PicHandle;    {picture for drawing window content}
     refCon:        LONGINT;      {window's reference value}
  END;
WindowPeek = ^WindowRecord;   WindowPtr = ^GrafPort;   (both alias offset 0)
```

Field table (68K layout; `GrafPort` is 108 bytes, `RgnHandle`/`Handle`/`Ptr`/
`StringHandle`/`PicHandle`/`ControlHandle` are 4-byte 68K pointers; `INTEGER`=2,
`BOOLEAN`=1, `LONGINT`=4). Offsets are the classic 68K offsets; the verbatim layout
is the authority, the offsets are [inferred] arithmetic from the published field
sizes and are not load-bearing for FLAIR (which re-types the fields, Sec 6).

| field         | offset | size | type         | notes |
|---------------|--------|------|--------------|-------|
| port          | 0      | 108  | GrafPort     | LOAD-BEARING: WindowPtr casts to GrafPtr |
| windowKind    | 108    | 2    | INTEGER      | class; see Sec 1.1 |
| visible       | 110    | 1    | BOOLEAN      | TRUE if drawn |
| hilited       | 111    | 1    | BOOLEAN      | TRUE == active (highlighted title bar) |
| goAwayFlag    | 112    | 1    | BOOLEAN      | TRUE == has close box |
| spareFlag     | 113    | 1    | BOOLEAN      | reserved (zoom-available) |
| strucRgn      | 114    | 4    | RgnHandle    | structure region (frame + content) |
| contRgn       | 118    | 4    | RgnHandle    | content region (interior only) |
| updateRgn     | 122    | 4    | RgnHandle    | accumulated damage; see Sec 4 |
| windowDefProc | 126    | 4    | Handle       | the WDEF (resID in upper 12 bits of defID) |
| dataHandle    | 130    | 4    | Handle       | WDEF private data (zoom state etc.) |
| titleHandle   | 134    | 4    | StringHandle | window title (Pascal string) |
| titleWidth    | 138    | 2    | INTEGER      | title text width in px (measured) |
| controlList   | 140    | 4    | ControlHandle| head of this window's control list |
| nextWindow    | 144    | 4    | WindowPeek   | next window front-to-back |
| windowPic     | 148    | 4    | PicHandle    | optional content QuickDraw picture |
| refCon        | 152    | 4    | LONGINT      | app reference value |

[offsets: inferred from published field sizes; the verbatim ORDER is the authority]

### 1.1 windowKind (the window class)

| name         | value     | meaning |
|--------------|-----------|---------|
| (system/DA)  | negative  | system window or desk accessory (driver refNum) |
| dialogKind   | 2         | dialog/alert window (owned by Dialog Manager) |
| userKind     | 8         | application window (apps use 8..32767) |
| documentKind | 8         | conventional app document window (== userKind) |

[verified: A+B; im-toolbox-records-verbatim.md + IM-I p. I-270]. `windowKind`
distinguishes who owns the window so the system routes events correctly:
`inSysWindow` (Sec 2) is returned for windows whose `windowKind` is negative.

### 1.2 Two regions + the structure/content distinction

The WDEF computes two regions for every window [verified: refs/StandardWDEF_a.txt
`wCalcRgns` handler + IM-I]:

- **strucRgn** -- the entire window (title bar + frame + drop shadow + content).
  Hit-testing the structure region tells `FindWindow` the point is somewhere in
  this window at all.
- **contRgn** -- the interior content rectangle only (inside the frame, below the
  title bar, excluding scroll bars/grow box if the app reserves them).

`strucRgn DIFF contRgn` == the chrome (frame). The WDEF's `wHit` handler then
sub-classifies a point inside the structure into the part-codes of Sec 2.

--------------------------------------------------------------------------------
## 2. FindWindow part-codes

`FindWindow(point, &whichWindow)` walks the window list front-to-back, tests the
global-coordinate `point` against each visible window's `strucRgn`, and returns a
part code (`INTEGER`) plus the window pointer (NULL for `inDesk`/`inMenuBar`).
[verified: A+B; im-toolbox-records-verbatim.md + MTE Table 4-2 p. 4-63; IM-I p. I-280]

| name        | value | window ptr | meaning |
|-------------|-------|------------|---------|
| inDesk      | 0     | NULL       | desktop / no window (also written inDesktop) |
| inMenuBar   | 1     | NULL       | in the menu bar (-> MenuSelect) |
| inSysWindow | 2     | sys window | in a system/DA window (-> SystemClick) |
| inContent   | 3     | window     | in the content region (app handles) |
| inDrag      | 4     | window     | in the title-bar drag region (-> DragWindow) |
| inGrow      | 5     | window     | in the size/grow box (-> GrowWindow/SizeWindow) |
| inGoAway    | 6     | window     | in the close box (-> TrackGoAway/CloseWindow) |
| inZoomIn    | 7     | window     | in zoom box, window in STANDARD (large) state (-> ZoomWindow in, shrink to user) |
| inZoomOut   | 8     | window     | in zoom box, window in USER (small) state (-> ZoomWindow out, grow to standard) |

The two zoom codes are state-dependent: the WDEF returns `inZoomIn` when the window
is currently at its standard (large) size -- clicking zooms IN, shrinking it back to
the user state -- and `inZoomOut` when it is at its user (small/resized) size --
clicking zooms OUT, growing it to the standard state. (Mac terminology: "standard
state" == the large default-zoom rect; "user state" == the user-resized/moved rect.)
The local WDEF source proves the mapping directly: `IsItSmall` returns "window is
large if all corners are within 7 of their default positions" and the hit handler
branches `wInZoomIn` on the large case ("say window big") and `wInZoomOut` on the
small case ("say window small").
[verified: refs/StandardWDEF_a.txt hit handler lines 1195-1200 (`wInZoomIn` ==
"say window big" / `wInZoomOut` == "say window small") + `IsItSmall` lines 1840-1842
+ IM-I FindWindow (inZoomIn=7 standard state, inZoomOut=8 user state)]

Note the WDEF's internal `wHit` return constants (`wInDesk`, `wInContent`,
`wInDrag`, `wInGrow`, `wInGoAway`, `wInZoomIn`, `wInZoomOut`) are numerically the
same 0,3,4,5,6,7,8 values `FindWindow` exposes; `FindWindow` supplies `inMenuBar=1`
/ `inSysWindow=2` itself before ever calling the WDEF.
[verified: refs/StandardWDEF_a.txt + im-toolbox-records-verbatim.md]

--------------------------------------------------------------------------------
## 3. WDEF -- window definition function (variants + messages)

### 3.1 Window definition IDs (variant codes)

A window definition ID packs the WDEF resource ID in the upper 12 bits and a
variation code 0..15 in the low 4 bits: `defID = WDEF_resID*16 + variant`. The
standard WDEF is resource ID 0, so for FLAIR the defID equals the variant directly.
[verified: A+B; refs/IM_Tb_TypesOfWindows.txt "Constant / Window definition ID"
table + im-toolbox-records-verbatim.md; MTE Table 4-1 p. 4-8]

| name            | defID | description |
|-----------------|-------|-------------|
| documentProc    | 0     | movable, sizable window, NO zoom box |
| dBoxProc        | 1     | alert box / fixed modal dialog box |
| plainDBox       | 2     | plain box (no chrome) |
| altDBoxProc     | 3     | plain box with drop shadow |
| noGrowDocProc   | 4     | movable window, NO size box, NO zoom box (modeless dialog) |
| movableDBoxProc | 5     | movable modal dialog box (title bar, no grow/zoom) |
| zoomDocProc     | 8     | standard document window (close + zoom + size) |
| zoomNoGrow      | 12    | zoomable, NON-resizable window (close + zoom, no size) |
| rDocProc        | 16    | rounded-corner window with close box (desk accessory) |

[verified: A+B]. Note `rDocProc=16` is defID 16, i.e. variant 0 of WDEF resource 1
(rounded WDEF), not a variant of WDEF 0. The chrome-relevant subset (the variant
AND 3 dispatch the title-bar WDEF uses) is detailed in
`../chrome/wdef-variant-geometry.md`. Two source notes:

- `movableDBoxProc=5` is present in MTE (refs/IM_Tb_TypesOfWindows.txt) but is
  OMITTED from the brief/`im-toolbox-records-verbatim.md` variant list (which jumps
  4 -> 8). This is a source gap, not a contradiction: MTE is the fuller list and is
  the authority here. [documented: refs/IM_Tb_TypesOfWindows.txt vs
  im-toolbox-records-verbatim.md] FLAIR encodes documentProc..zoomNoGrow; 5/16 are
  out of M3 scope. [inferred from window_record.h scope]
- The dBoxProc-with-title proc ID 5 the WDEF source calls `dboxWithTitle EQU 5`
  (refs/StandardWDEF_a.txt line 69) is the SAME slot 5 -- the movable/titled dialog
  variant; its 7px border + title share-area adjust (`proc5TopAdjust`,
  `proc5HitZAdjust`) are chrome, owned by `../chrome/dialog-borders.md`.
  [verified: refs/StandardWDEF_a.txt lines 69-72]

### 3.2 WDEF message dispatch

The Window Manager calls the WDEF with a message selector + a parameter. The
standard WDEF's dispatch jump table fixes the message numbers:
[verified: refs/StandardWDEF_a.txt lines 411-419 (`GoDocProc` jump table:
"draw is message #0" .. "draw grow icon is #6") + range check lines 234-236 +
IM-I WDEF chapter (wNew=3, not wInitialize)]

| message    | value | param        | action |
|------------|-------|--------------|--------|
| wDraw      | 0     | drawing part | draw the window frame/chrome (param selects part) |
| wHit       | 1     | mouse Point  | hit-test -> return a part-code (Sec 2) |
| wCalcRgns  | 2     | --           | recompute strucRgn + contRgn |
| wNew       | 3     | --           | WDEF init (zoom state, title icon handle) |
| wDispose   | 4     | --           | WDEF teardown |
| wGrow      | 5     | grow Rect    | draw the XOR grow outline during a resize drag |
| wDrawGIcon | 6     | --           | draw the size box + scroll-bar delimiter lines |

[verified: refs/StandardWDEF_a.txt]. The `wDraw` param sub-selects which chrome
part to (re)draw; for highlighting the close/zoom boxes the param carries the
part-code being tracked (the WDEF source compares it to `wInGoAway` for an XOR
toggle, refs/StandardWDEF_a.txt lines 534/550). `wDrawGIcon` is what
`DrawGrowIcon` invokes; its 16x16 size box + nested-box interior is chrome
(`../chrome/grow-box.md`).

### 3.3 Color WDEF (System 7) vs B&W WDEF (System 6)

The System 7 color WDEF reads a window color table (`wctb`, default ID -4096) for
its shade indices; `minColorDepth=8` (refs/StandardWDEF_a.txt line 116) means the
color path activates at 8bpp -- satisfied by FLAIR's indexed-8 640x480. The
title-bar shade index names the WDEF uses (`wHiliteLight=5 wHiliteDark=6
wTitleBarLight=7 wTitleBarDark=8 wDialogLight=9 wDialogDark=10 wTingeLight=11
wTingeDark=12`, refs/StandardWDEF_a.txt lines 75-82) are part-codes into the
`wctb`; the gadget pixmap IDs are `kPixmapID=-14336`, `kHighlightPix=-14334`
(lines 112-113). The actual RGB values behind those indices are [golden-resolves]
-- owned by `../resources/wctb-mctb-format.md` + `../chrome/pinstripe.md`, NOT this
spec. System 6 used a flat B&W WDEF (no pinstripe, no `wctb`) with the SAME
record layout, the SAME FindWindow part-codes, and the SAME variant IDs. [Law 3
delta: documented: refs/StandardWDEF_a.txt + im-toolbox-records-verbatim.md]

--------------------------------------------------------------------------------
## 4. The damage / update model (System 7 update regions)

Each window owns an `updateRgn` -- accumulated screen area that needs redrawing.
The model (verbatim IM behavior, the FLAIR D-5 mapping):

1. When a window is moved, resized, closed, or revealed, the Window Manager
   computes the newly-exposed area of each affected window and ACCUMULATES it into
   that window's `updateRgn` (union of old damage + newly exposed). Exposure is
   computed with region difference: the area a front window vacated, intersected
   with a back window's structure, minus what is still covered.
   [verified: A+B; im-toolbox-records-verbatim.md (updateRgn field) + IM-I Window
   Manager; FLAIR D-5]
2. The event pump sees a non-empty `updateRgn` and posts an `updateEvt` (event
   what=6) whose `message` is the WindowPtr. [verified: refs cross to
   event-manager.md / EventRecord message semantics]
3. The app responds with `BeginUpdate(w)` -- which sets the window's `visRgn` to
   `oldVisRgn INTERSECT updateRgn` and clears `updateRgn` -- draws its content
   (automatically clipped to the damaged area), then `EndUpdate(w)` restores the
   full `visRgn`. Net effect: the app draws clipped to `visRgn INTERSECT
   updateRgn`; nothing outside the damage is repainted. [documented: IM-I
   BeginUpdate/EndUpdate; the visRgn-intersect invariant is `../quickdraw/grafport.md`]

FLAIR damage model (ADR-0004 D-5): `MoveWindow` accumulates newly-exposed area into
each window's `updateRgn` via `DiffRgn` (the ATKINSON region engine); the pump
issues `updateEvt`; the app draws clipped to `visRgn INTERSECT updateRgn`. No
over-repaint. [verified: ADR-0004 D-5 + recon-flair-impl]. The region operations
themselves are `../quickdraw/regions-api.md` (`DiffRgn`/`SectRgn`/`UnionRgn`).

`InvalRect`/`InvalRgn` let the app add to `updateRgn` manually; `ValidRect`/
`ValidRgn` subtract from it. [documented: IM-I Window Manager]

--------------------------------------------------------------------------------
## 5. Key Window Manager entry points (semantics, not records)

| routine        | part-code link | semantics |
|----------------|----------------|-----------|
| NewWindow/GetNewWindow | --     | allocate + insert a WindowRecord; calls WDEF wNew + wCalcRgns |
| FindWindow     | Sec 2          | classify a global point -> part-code + window |
| SelectWindow   | --             | bring to front, activate (hilited:=TRUE), z-order shuffle |
| DragWindow     | inDrag         | track + move; accumulates exposure into others' updateRgn |
| GrowWindow     | inGrow         | track the wGrow XOR outline; returns new size |
| SizeWindow     | inGrow         | apply new size; wCalcRgns; inval newly-exposed content |
| ZoomWindow     | inZoomIn/Out   | toggle user<->standard state; redraw chrome + inval |
| TrackGoAway    | inGoAway       | track the close box highlight; TRUE if released inside |
| DrawGrowIcon   | --             | invoke WDEF wDrawGIcon (size box + scroll delimiters) |
| BeginUpdate/EndUpdate | --      | clip drawing to the damage region (Sec 4) |

[verified: A+B; IM-I Window Manager + MTE Ch 4]. These are the routines that read
the part-codes of Sec 2 + drive the WDEF messages of Sec 3.2.

--------------------------------------------------------------------------------
## 6. FLAIR mapping + deliberate deviations

FLAIR consumer: `../initech-os` `spec/window_record.h` (LOCKED) + `os/flair/
window.c` (~513 LOC). WindowRecord field NAMES are verbatim; `port` is embedded at
offset 0 with a `_Static_assert`; the FindWindow part-codes, `windowKind`, and
WDEF variant IDs are all present verbatim with per-value `_Static_assert`s
(`inDesk=0`..`inZoomOut=8`; `dialogKind=2`; `userKind=8`; `documentProc=0`..
`zoomNoGrow=12`). [verified: A+B; research/raw/s7-toolbox.md Sec 2 + window_record.h read]

DELIBERATE DEVIATIONS (freestanding / no-heap-handles; all justified in-header,
all preserve the IM field NAMES + load-bearing numeric VALUES):

| IM field/type        | FLAIR form                | why |
|----------------------|---------------------------|-----|
| port: GrafPort (ptr semantics) | embedded GrafPort   | keeps offset-0 contract, avoids 2nd alloc |
| strucRgn/contRgn/updateRgn: RgnHandle | region_t* (arena ptr) | no heap handle table; arena-backed regions |
| windowDefProc: Handle | int16 windowDefProcVariant | built-in WDEF dispatch keyed by variant |
| titleHandle: StringHandle | char[64] in-place    | no StringHandle; name preserved |
| dataHandle/controlList/windowPic | OMITTED in current release | no heap handles; controls tracked by Control Mgr [inferred from header absence] |
| nextWindow: WindowPeek | WindowRecord* (typed) | same z-order list, typed |
| refCon: LONGINT       | int32                     | direct |

[verified: research/raw/s7-toolbox.md Sec 2 + s7-toolbox.md Sec 8 deviation
pattern]. Verification: `test-window` (15 checks, bit-exact vs an independent
owner-grid; ZORDER/OVERPAINT mutants bite); the M3 capstone is the drag-gate
`test-drag` (AM-8). [documented: recon-flair-impl]

--------------------------------------------------------------------------------
## 7. Certainty ledger

- [verified: A+B] WindowRecord layout (verbatim, port@0); FindWindow part-codes
  inDesk=0..inZoomOut=8 (inZoomIn=7 == standard/large state, inZoomOut=8 == user/
  small state -- per StandardWDEF_a.txt IsItSmall + IM-I FindWindow); windowKind;
  WDEF variant IDs documentProc=0..rDocProc=16; WDEF message numbers wDraw=0,
  wHit=1, wCalcRgns=2, wNew=3, wDispose=4, wGrow=5, wDrawGIcon=6 (message 3 is
  wNew, NOT wInitialize); the D-5 DiffRgn damage model.
- [documented: single src] movableDBoxProc=5 (MTE/refs/IM_Tb_TypesOfWindows.txt
  only -- the brief's variant list omits it); color WDEF shade indices 5-12 +
  gadget pixmap IDs (refs/StandardWDEF_a.txt only).
- [golden-resolves] ALL pixel values the WDEF emits -- pinstripe shade RGBs behind
  indices 5-8, close/zoom/grow box rendered interiors, content-body white. These
  are owned by `../chrome/*` and `../resources/wctb-mctb-format.md`; this record/
  part-code spec contains NO pixel claims to resolve.
- [inferred] 68K field offsets in Sec 1 (arithmetic from published field sizes;
  not load-bearing); FLAIR omission of dataHandle/controlList/windowPic.

--------------------------------------------------------------------------------
## Sources

LOCAL (authority):
- refs/im-toolbox/im-toolbox-records-verbatim.md -- WindowRecord verbatim Pascal
  record; FindWindow part-codes; windowKind; WDEF variant IDs. (first-hand cached)
- refs/IM_Tb_TypesOfWindows.txt -- MTE Ch 4 "Types of Windows": the nine window
  types + the Constant/Window-definition-ID table (incl. movableDBoxProc=5,
  rDocProc=16). (first-hand cached)
- refs/StandardWDEF_a.txt -- Apple System 7 standard WDEF assembly: the WDEF
  message jump table (wDraw=0..wDrawGIcon=6), the wHit part-code returns, the
  wCalcRgns region computation, shade-index EQUs (5-12), gadget pixmap IDs
  (-14336/-14334), minColorDepth=8, dboxWithTitle/dBoxBorderSize. (first-hand cached)
- refs/im-toolbox/wctb-window-color-table-partcodes.txt -- wctb part-code indices
  the color WDEF reads (cross-link; RGBs are golden-resolves).
- research/raw/s7-toolbox.md Sec 2 (Window Manager) -- the research brief + FLAIR
  mapping + deviations.
- research/system7-gui-ground-truth.md Sec 4.2 -- master-brief restatement + D-5.
- ../initech-os spec/window_record.h (LOCKED) -- FLAIR record + _Static_asserts
  (cited via the brief; not re-read here).

WEB (cross-check only; LOCAL files above are the authority):
- Inside Macintosh Vol I, Window Manager (p. I-268 WindowRecord, I-280 FindWindow).
- Inside Macintosh: Macintosh Toolbox Essentials, Ch 4 Window Manager
  (p. 4-8 Table 4-1 variant IDs, p. 4-63 Table 4-2 part-codes, p. 4-78 record).
- dev.os9.ca techpubs Toolbox mirror (Toolbox-191 Types of Windows).

ASCII-clean.
