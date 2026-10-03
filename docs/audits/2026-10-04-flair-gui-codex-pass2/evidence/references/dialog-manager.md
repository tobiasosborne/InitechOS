# Dialog Manager -- DialogRecord, DITL item types, alerts, the ModalDialog loop

The Dialog Manager builds, draws, and runs alert and dialog boxes: it layers a
typed item list (a DITL: buttons, static/editable text, icons, pictures, user
items) on top of a Window Manager window, draws those items, and runs the modal
event loop (`ModalDialog`) that filters events, tracks controls, services the
editable-text field, and returns the item number the user activated. This spec
carries the verbatim Inside Macintosh `DialogRecord` layout, the DITL item-type
byte catalog, the alert mechanism (`StopAlert`/`NoteAlert`/`CautionAlert` + the
three standard icons + the 4-stage escalation), the `ok=1`/`cancel=2` convention,
and the routine semantics. The FLAIR consumer is `../initech-os` `os/flair/dialog.c`
(~835 LOC) + the `dBoxProc` window variant in `spec/window_record.h`; it verifies
against dialog/alert screendumps including the canon FILE COPY "Saving tables to
disk..." modal (golden G8).

This is one subsystem. The 7px `dBoxProc` fancy-border native geometry the dialog
window draws is chrome, owned by `../chrome/dialog-borders.md`; the alert icons +
alert box layout (note/stop/caution) live in `../desktop/alerts.md`; the compiled
`DLOG`/`DITL`/`ALRT` template binary layout is `../resources/wind-menu-dlog-ditl.md`.
The window the dialog embeds is `window-manager.md` (the `WindowRecord` at offset 0,
the `dBoxProc=1`/`movableDBoxProc=5` variants); dialog controls (buttons, check
boxes, radio buttons) are `control-manager.md`; the editable-text field uses
`textedit.md` (`TERec`); the modal pump consumes `event-manager.md` events; the
consolidated constant catalog is `manager-part-codes.md`.

Target precision (Law 3): System 7.0/7.1. The DialogRecord layout, DITL item
types, alert mechanism, and `ok`/`cancel` constants are STABLE from System 1
through 7.x; the System-7 deltas are color chrome (the `dBoxProc` border reads the
`wctb` dialog shade indices 9/10) and the `aDefItem` default-button OUTLINE
(System 7 draws a thick black ring around the default button) -- see Sec 4 + the
Law-3 ledger. System 7.5+ Appearance-Manager dialog refinements are OUT OF SCOPE.

--------------------------------------------------------------------------------
## 1. DialogRecord (the dialog data structure)

VERBATIM Inside Macintosh layout. `window` (a full `WindowRecord`) is at offset 0
-- the load-bearing invariant: a `DialogPtr` IS a `WindowPtr` IS a `GrafPtr`, so
`SetPort(theDialog)`, `FindWindow`, `DrawControls(theDialog)`, and the whole Window
Manager all operate on the same storage.
[verified: refs/im-toolbox/im-toolbox-records-verbatim.md DialogRecord +
WebFetch dev.os9.ca Toolbox-442 (field listing + sizes); IM-I Ch 13 p. I-410]

Pascal declaration (verbatim, source order):

```
TYPE DialogRecord =
  RECORD
     window:    WindowRecord; {the dialog's window -- FIRST, offset 0}
     items:     Handle;       {handle to the item list (DITL resource)}
     textH:     TEHandle;     {the current editText item's TextEdit handle}
     editField: INTEGER;      {editText item number minus 1 (-1 == none)}
     editOpen:  INTEGER;      {used internally}
     aDefItem:  INTEGER;      {default button item number (usually OK == 1)}
  END;
DialogPeek = ^DialogRecord;   DialogPtr == WindowPtr (== ^GrafPort; window @ offset 0);
```

Field table (68K layout; the embedded `WindowRecord` (dWindow) = 156 bytes per the
Toolbox-442 assembly-language summary -- the 108-byte GrafPort plus the Window
Manager fields, window-manager.md Sec 1; the per-field sum runs slightly over but
the published dWindow size of 156 is the authority for these offsets;
`Handle`/`TEHandle` are 4-byte 68K pointers; `INTEGER`=2). The offsets below match
the Toolbox-442 assembly-language summary verbatim (dWindow@0, items@156, teHandle@160,
editField@164, editOpen@166, aDefItem@168); they are not load-bearing for FLAIR
(which re-types the fields, Sec 6).

| field     | offset | size | type         | notes |
|-----------|--------|------|--------------|-------|
| window    | 0      | 156  | WindowRecord | LOAD-BEARING: DialogPtr casts to WindowPtr/GrafPtr |
| items     | 156    | 4    | Handle       | the DITL item list (parsed item array) |
| textH     | 160    | 4    | TEHandle     | TextEdit handle for the active editText item (NIL if none) |
| editField | 164    | 2    | INTEGER      | active editText item number minus 1; -1 == no edit field |
| editOpen  | 166    | 2    | INTEGER      | internal (edit-field-open state) |
| aDefItem  | 168    | 2    | INTEGER      | default button item (Return/Enter activates it); usually 1 |

[verified: A+B -- im-toolbox-records-verbatim.md field order + Toolbox-442
assembly-language summary, which publishes the exact offsets AND sizes verbatim
(dWindow@0 size 156, items@156, teHandle@160, editField@164, editOpen@166,
aDefItem@168). Both order AND offsets are documented, not inferred.]

`windowKind` (inside the embedded `WindowRecord`) is set to `dialogKind=2` for a
dialog window, which is how the system knows the window is Dialog-Manager-owned and
routes events to `IsDialogEvent`/`DialogSelect`. [verified: A+B; window-manager.md
Sec 1.1 windowKind catalog + im-toolbox-records-verbatim.md]

--------------------------------------------------------------------------------
## 2. DITL -- the dialog item list (item-type byte catalog)

A dialog/alert is a window plus a DITL resource: a count + an array of items, each
item carrying a display rectangle, an item-type byte, and type-specific data (a
control title string, static/edit text, an icon/PICT resource ID, or nothing for a
user item). The Dialog Manager walks this list to DRAW the items and to HIT-TEST
clicks back to a 1-based item number. The compiled DITL binary layout (count,
per-item placeholder handle + display rect + type byte + variable data) is owned by
`../resources/wind-menu-dlog-ditl.md`; THIS spec specifies the item-TYPE byte the
Dialog Manager interprets. [verified: A+B; im-toolbox-records-verbatim.md DITL item
types + WebSearch/WebFetch Toolbox-442; IM-I p. I-403]

The item-type byte = a base type OR'd with the `itemDisable` flag. `ctrlItem=4` is
itself a base that is added to a 0..3 control sub-type:

| name        | value | meaning |
|-------------|-------|---------|
| userItem    | 0     | application-drawn item (app supplies a draw proc via SetDialogItem) |
| helpItem    | 1     | Help Manager balloon-help item (System 7; no visible widget) |
| ctrlItem    | 4     | base for a control; ADD one of the four sub-types below |
| btnCtrl     | 0     | + ctrlItem => 4: standard push button |
| chkCtrl     | 1     | + ctrlItem => 5: standard check box |
| radCtrl     | 2     | + ctrlItem => 6: standard radio button |
| resCtrl     | 3     | + ctrlItem => 7: control defined by a 'CNTL' resource |
| statText    | 8     | static (non-editable) text |
| editText    | 16    | editable text field (drives DialogRecord.textH/editField) |
| iconItem    | 32    | icon ('ICON' resource) |
| picItem     | 64    | QuickDraw picture ('PICT' resource) |
| itemDisable | 128   | OR'd flag: item is disabled -- drawn, but NOT returned by ModalDialog |

[verified: A+B; im-toolbox-records-verbatim.md + Toolbox-442 (btnCtrl=0 chkCtrl=1
radCtrl=2 resCtrl=3; statText=8 editText=16 iconItem=32 picItem=64 itemDisable=128;
userItem=0 helpItem=1)]. Worked sub-types: a push button DITL type = `ctrlItem +
btnCtrl` = 4; a check box = 5; a radio button = 6; a 'CNTL'-resource control = 7.
`helpItem=1` and `btnCtrl=0` collide numerically but never ambiguously: `helpItem`
is a standalone base type, `btnCtrl` is only ever added to `ctrlItem=4`.

`itemDisable` semantics: a disabled item is still DRAWN and still occupies its rect,
but `ModalDialog`/`DialogSelect` will NOT report a hit on it (it never returns that
item number). Static text and icons are typically disabled (they are not clickable);
buttons/checkboxes are enabled. [verified: A+B; im-toolbox-records-verbatim.md
"item not returned by ModalDialog" + Toolbox-442]

The 1-based item numbering is load-bearing: `aDefItem` (Sec 1), the `ok`/`cancel`
constants (Sec 4), and the `itemHit` return of `ModalDialog` (Sec 3) are ALL
1-based DITL item indices. Item 0 means "no item". [verified: A+B]

--------------------------------------------------------------------------------
## 3. ModalDialog -- the modal event loop

`ModalDialog(filterProc, VAR itemHit)` is the modal pump. It repeatedly fetches an
event, optionally passes it through the caller's `filterProc` (a hook that can
consume/rewrite events -- e.g. map Return -> the default button), then dispatches:
clicks on an enabled control item track the control and return that item number;
clicks on an enabled non-control item return its number; the editText field
receives keystrokes; and it returns (via `itemHit`, 1-based) the first item the user
activated. [verified: A+B; im-toolbox-records-verbatim.md + WebFetch Toolbox-442
signature `ModalDialog(filterProc: ModalFilterProcPtr; VAR itemHit: Integer)`;
IM-I Ch 13]

Pascal/IM signatures (verbatim from Toolbox-442):

```
ModalDialog (filterProc: ModalFilterProcPtr; VAR itemHit: Integer);
IsDialogEvent (theEvent: EventRecord): Boolean;
DialogSelect (theEvent: EventRecord; VAR theDialog: DialogPtr;
              VAR itemHit: Integer): Boolean;
```

[verified: WebFetch dev.os9.ca Toolbox-442]. Two ways to run a dialog:

- **Modal:** `ModalDialog` owns the loop until an enabled item is hit. Used for
  alerts and modal dialogs (the FILE COPY box). The app cannot do anything else
  while it spins.
- **Modeless:** the app's own event loop calls `IsDialogEvent(theEvent)` to test
  whether an event belongs to a dialog window (`windowKind == dialogKind`), then
  `DialogSelect(theEvent, &theDialog, &itemHit)` to dispatch it -- returns TRUE +
  the item number when an enabled item was hit. [documented: Toolbox-442 +
  im-toolbox-records-verbatim.md (Modeless vs Modal)]

Standard default-button handling: the modal loop treats Return and Enter as a click
on `aDefItem` (the default button), and -- by the `ok=1`/`cancel=2` convention --
Escape or Cmd-. (period) as a click on item 2 (Cancel). The Return/Enter -> default
mapping is part of `ModalDialog`'s built-in behavior; the Escape->Cancel mapping is
a convention many filterProcs and later toolboxes formalize. [documented: IM-I
Dialog Manager; the System 7 default-button OUTLINE is Sec 4]

--------------------------------------------------------------------------------
## 4. Alerts (Stop / Note / Caution) + the standard items

An ALERT is a transient modal box driven straight from a resource -- you do not
build a window yourself, you call one routine with an alert resource ID:

```
Alert       (alertID: Integer; filterProc: ModalFilterProcPtr): Integer;
StopAlert   (alertID: Integer; filterProc: ModalFilterProcPtr): Integer;
NoteAlert   (alertID: Integer; filterProc: ModalFilterProcPtr): Integer;
CautionAlert(alertID: Integer; filterProc: ModalFilterProcPtr): Integer;
GetAlertStage: Integer;
```

[verified: WebFetch dev.os9.ca Toolbox-442]. Each returns the 1-based item number
the user clicked. `Alert` draws no icon; `StopAlert`/`NoteAlert`/`CautionAlert`
draw the corresponding standard icon (below) in the alert's top-left.
[verified: A+B; im-toolbox-records-verbatim.md + Toolbox-442]

### 4.1 Standard alert icons

| name        | ICON resID | role | drawn by |
|-------------|------------|------|----------|
| stopIcon    | 0          | error / cannot proceed (stop sign) | StopAlert |
| noteIcon    | 1          | informational note (speech / note) | NoteAlert |
| cautionIcon | 2          | warning, proceed-with-care (triangle/!) | CautionAlert |

[verified: A+B; im-toolbox-records-verbatim.md (stopIcon=0 noteIcon=1
cautionIcon=2) + IM-I p. I-409]. The values 0/1/2 are indices/resource selectors
into the System-file alert ICON resources. The RENDERED icon bitmaps (the actual
pixels of the stop sign / note / caution triangle) are [golden-resolves] -- owned by
`../desktop/alerts.md`; this spec carries only the selector constants.

### 4.2 The four alert stages (escalation)

An ALRT resource carries a StageList: four stages (stage 1..4), where stage N
applies to the Nth consecutive invocation of the same alert. Each stage encodes
which DITL item is the default (bold) button, whether the alert box is drawn
visibly, and which of the 4 sound selectors (sound number 0..3) to play (0 ==
silent; 1..3 select sound1/sound2/sound3, whose actual sounds are resource-defined).
The Dialog Manager auto-escalates: repeated alerts climb stages 1->2->3->4 (and
stick at 4) so an error that keeps recurring can beep louder / box less. The current
stage is queried with `GetAlertStage`. [documented: WebFetch Toolbox-442
`GetAlertStage: Integer` + WebSearch IM:Tb Dialog Manager (StageList / 4 stages /
sound number 0..3); IM-I Ch 13]. The exact ALRT/StageList BIT layout (boldItm /
visible / soundNum packed per stage) is the resource format, owned by
`../resources/wind-menu-dlog-ditl.md`. [documented: cross-link]

### 4.3 Standard items + the default button

| name | value | meaning |
|------|-------|---------|
| ok     | 1 | the OK / default button is item 1 (== aDefItem by convention) |
| cancel | 2 | the Cancel button is item 2 by convention |

[verified: A+B; im-toolbox-records-verbatim.md (ok=1 cancel=2) + Toolbox-442
(ok=1 first button, cancel=2 second button)]. These are CONVENTIONS the standard
ALRT templates follow (the OK button is DITL item 1, Cancel is item 2), not values
baked into the record -- `aDefItem` is what actually selects the default button, and
it is set to 1 by the standard templates. [documented: IM-I]

System 7 DEFAULT-BUTTON OUTLINE (Law 3 delta): in System 7 the Dialog Manager draws
a thick black rounded outline around the default button's rect (around `aDefItem`),
so the user sees which button Return activates. IM describes it only as a "thick
black outline" -- it publishes NO pixel measurement. The native pixel geometry of
that ring (inset, thickness, corner radius) is chrome --
[golden-resolves] against a real System 7 alert screendump, owned by
`../desktop/alerts.md` / `../chrome/dialog-borders.md`. System 6 did not draw the
heavy outline the same way (flatter chrome). [Law 3 delta: documented: IM:Tb Dialog
Manager default-outline behavior; rendered ring = golden-resolves]

--------------------------------------------------------------------------------
## 5. Item accessors + ParamText

```
GetDialogItem    (theDialog: DialogPtr; itemNo: Integer; VAR itemType: Integer;
                  VAR item: Handle; VAR box: Rect);
SetDialogItem    (theDialog: DialogPtr; itemNo: Integer; itemType: Integer;
                  item: Handle; box: Rect);
GetDialogItemText(item: Handle; VAR text: Str255);
SetDialogItemText(item: Handle; text: Str255);
ParamText        (param0, param1, param2, param3: Str255);
```

[verified: WebFetch dev.os9.ca Toolbox-442]. Semantics:

- `GetDialogItem`/`SetDialogItem` read/replace one item's `{type, handle, rect}` by
  1-based number. (`GetDItem`/`SetDItem` are the older IM-I names for the same
  routines.) [verified: Toolbox-442 + im-toolbox-records-verbatim.md]
- `GetDialogItemText`/`SetDialogItemText` read/replace the text of a statText or
  editText item (operate on the item's handle, not the dialog).
- `ParamText(p0,p1,p2,p3)` substitutes up to four strings into static text wherever
  the metacharacters `^0 ^1 ^2 ^3` appear -- the mechanism a templated message like
  "Saving ^0 to disk..." uses to fill in a runtime value. [documented: IM-I Dialog
  Manager; the FILE COPY message in Sec 6 is a ParamText-style fill]

--------------------------------------------------------------------------------
## 6. FLAIR mapping + deliberate deviations

FLAIR consumer: `../initech-os` `os/flair/dialog.{c,h}` (~835 LOC) + the
`dBoxProc`/`movableDBoxProc` window variant in `spec/window_record.h`. The
`DialogRecord` field NAMES are preserved; `window` is embedded at offset 0 with a
`_Static_assert` (DialogPtr -> WindowPtr -> GrafPtr chain asserted); the item-type
constants, `ok=1`/`cancel=2`, and the alert icon selectors carry per-value
`_Static_assert`s. [verified: A+B; research/raw/s7-toolbox.md Sec 5 + the FLAIR
dialog.h read recorded there]

DELIBERATE DEVIATIONS (freestanding / no resource fork / no heap handles; all
preserve the IM field NAMES + the load-bearing numeric VALUES):

| IM form | FLAIR form | why |
|---------|------------|-----|
| window: WindowRecord (offset 0) | embedded WindowRecord | keeps offset-0 DialogPtr->WindowPtr contract |
| items: Handle (packed DITL resource) | `DialogItem[]` struct array {type, rect, text, ctrl, enabled} | no resource fork yet; typed in-place |
| textH: TEHandle + editField (full TextEdit) | single-line inline `editBuf` (DIALOG_ITEM_TEXT_MAX=128) + active-item index | minimal TextEdit stand-in; full TERec is M5+ (textedit.md) |
| ModalDialog (synchronous GetNextEvent loop) | COOPERATIVE: drains `flair_raw_ring` via WaitNextEvent in task context (ADR-0004 D-6); takes a `dialog_filter_fn` (== filterProc); returns itemHit | freestanding cooperative scheduler; same ModalDialog(filter,&itemHit) semantics |
| GetDialogItem/SetDialogItem/...Text | FindDialogItem + accessors (same MTE Ch 6 names) | direct |
| the `enabled` byte | == IM itemDisable semantics (disabled => not returned) | direct |

[verified: research/raw/s7-toolbox.md Sec 5 (dialog.h read) + the s7-toolbox.md
deviation pattern]. FLAIR item types present: `userItem=0`, `iconItem=32`,
`statText=8`, `editText=16`, and the `ctrlItem` button/check/radio (delegating to
the Control Manager, control-manager.md). [verified: s7-toolbox.md Sec 5]

FILE COPY (the canon modal): `FileCopyDialog` == the byte-exact "Saving tables to
disk..." modal -- a `dBoxProc` window with static message text + a determinate
progress bar (a scroll-bar-range Control Manager control, control-manager.md /
chrome/scrollbar.md). It is the Office-Space-frame analog the corpus targets and the
golden G8 verifies the dialog border + message against. [verified: recon-flair-impl;
research brief Sec 5]. Verification: `test-dialog` (73 checks; BORDER / HIT_STATIC /
FILECOPY_MSG mutants bite). [documented: recon]

--------------------------------------------------------------------------------
## 7. Certainty ledger

- [verified: A+B] DialogRecord layout (verbatim, window@0; items/textH/editField/
  editOpen/aDefItem); DITL item-type byte catalog (userItem=0, helpItem=1,
  ctrlItem=4 + btnCtrl/chkCtrl/radCtrl/resCtrl=0/1/2/3, statText=8, editText=16,
  iconItem=32, picItem=64, itemDisable=128); alert icons stopIcon=0/noteIcon=1/
  cautionIcon=2; ok=1/cancel=2; routine signatures (ModalDialog/IsDialogEvent/
  DialogSelect/Alert*/Get-SetDialogItem/Text/ParamText/GetAlertStage).
- [documented: single src] the 4-stage alert StageList escalation + sound number
  0..3 (Toolbox-442 summary + WebSearch IM:Tb; the per-stage bit layout is deferred
  to the resource-format spec); ParamText ^0..^3 substitution.
- [golden-resolves] every RENDERED pixel: the stop/note/caution icon bitmaps
  (alerts.md), the System 7 default-button thick-black OUTLINE geometry (no IM pixel
  measurement; alerts.md / dialog-borders.md), and the RENDERED bevel pixels of the
  dBoxProc fancy border (chrome/dialog-borders.md, golden G8). NOTE: the dBoxProc
  border WIDTH of 7px is itself [verified: gui-ground-truth Sec 1.4 `dBoxBorderSize
  EQU 7`] -- only the rendered bevel COLORS golden-resolve. This record/constant spec
  contains NO pixel claims to resolve here.
- [documented: Toolbox-442 asm summary] the 68K field offsets in Sec 1 (the
  assembly-language summary publishes dWindow@0/156, items@156, teHandle@160,
  editField@164, editOpen@166, aDefItem@168 verbatim; not load-bearing for FLAIR).
- [inferred] the Escape/Cmd-. -> Cancel convention (filterProc-level, not in the
  core record); the "thick black" default-outline being a System-6 vs System-7
  delta in DRAWING (the constants are stable; only the rendered chrome differs).

CONTRADICTION CHECK: none found. The two local-cache values (im-toolbox-records-
verbatim.md) agree field-for-field and constant-for-constant with the independently
fetched Toolbox-442 summary (DialogRecord fields + sizes; all item-type values;
ok=1/cancel=2; icon selectors). The only NAME-collision (helpItem=1 vs btnCtrl=0
both "1-ish") is disambiguated by context (helpItem standalone vs btnCtrl always
added to ctrlItem=4), not a conflict.

--------------------------------------------------------------------------------
## Sources

LOCAL (authority):
- refs/im-toolbox/im-toolbox-records-verbatim.md -- DialogRecord verbatim Pascal
  record (window@0, items, textH, editField, editOpen, aDefItem); DITL item-type
  byte catalog; alert icons stopIcon=0/noteIcon=1/cautionIcon=2; ok=1/cancel=2;
  the Alert/StopAlert/NoteAlert/CautionAlert + ModalDialog(filterProc,&itemHit)
  semantics. (first-hand fetched-and-cached; SOURCE line: dev.os9.ca Toolbox-370 +
  IM-I Ch 13 p. I-410 / I-403 / I-409)
- research/raw/s7-toolbox.md Sec 5 (Dialog Manager) -- the research brief: record,
  DITL types, alert-vs-dialog, the FLAIR os/flair/dialog.{c,h} mapping + deviations
  + the FILE COPY canon + test-dialog.
- research/system7-gui-ground-truth.md Sec 4.5 + Sec 1.4 (dBoxProc 7px border) +
  Part 8 G8 -- master-brief restatement + the golden backlog item.
- refs/im-toolbox/im-toolbox-records-verbatim.md TERec section -- the editText/
  TextEdit handle the DialogRecord.textH points at (cross-link to textedit.md).
- ../initech-os os/flair/dialog.{c,h} (LOCKED scope) -- FLAIR DialogItem array +
  cooperative ModalDialog + inline editBuf (cited via the brief; not re-read here).

WEB (cross-check only; the LOCAL files above are the authority):
- dev.os9.ca techpubs Toolbox-442 "Summary of the Dialog Manager" (IM:Tb) --
  FETCHED this session: DialogRecord fields + sizes (window=156); item-type
  constants (btnCtrl=0..picItem=64, itemDisable=128, userItem=0, helpItem=1);
  ok=1/cancel=2; routine signatures (NewDialog/GetNewDialog/ModalDialog/
  IsDialogEvent/DialogSelect/Alert/StopAlert/NoteAlert/CautionAlert/GetAlertStage/
  GetDialogItem/SetDialogItem/GetDialogItemText/SetDialogItemText/ParamText).
- dev.os9.ca techpubs Toolbox-370 "Dialog Manager" (IM:Tb) chapter overview.
- WebSearch (IM:Tb Dialog Manager) -- the DITL item-list / ALRT StageList / 4-stage
  + sound-number escalation cross-check.
- Inside Macintosh Vol I, Dialog Manager (p. I-403 item types, I-409 alert icons,
  I-410 DialogRecord) + IM: Macintosh Toolbox Essentials Ch 6 (Dialog Manager).

ASCII-clean.
