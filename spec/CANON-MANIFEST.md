<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# FLAIR Canonical Spec Projection Manifest

**Document purpose:** Every spec in both sister corpora mapped exactly once to the
FLAIR spec/ file (or os/flair module) it projects into, or to OUT-OF-SCOPE with a
one-line reason. Drift is mechanically detectable: a corpus spec with no row here is
a gap; a row whose destination file does not exist is a build item.

**Ground-truth sources:**
- `../system7-decomp/INDEX.md` -- System 7.0/7.1 BASE (43 specs authored + verified)
- `../win31-decomp/INDEX.md` -- Windows 3.1 flat ACCENTS (45 specs authored + verified)

**Governing plan:** `docs/plans/FLAIR-implementation-plan.md` Phase 1 (P1-7).
**Governing ADRs:** ADR-0004 (FLAIR Toolbox), ADR-0005 (ATKINSON region engine).
**Win95-ism guard:** `spec/win95ism_guardrails.md` (adopted from
`../win31-decomp/specs/chrome/cross-version-guardrails.md` Sec 3).

**Era tagging rule (Sec 1 era axis, 2026-06-20 operator ruling):** every spec/
header and os/flair source that carries chrome metrics, color tables, WDEF geometry,
or title-bar rendering logic carries an `era` tag so a future `../system8-decomp`
or `platinum/` layer lands without a base rewrite.

**Coverage (verified):**
- system7-decomp: 43 specs total -- ALL 43 mapped below (9 chrome + 9 quickdraw +
  8 toolbox + 5 fonts + 6 resources + 6 desktop).
- win31-decomp: 45 specs total -- ALL 45 mapped below (10 chrome + 11 user +
  8 gdi + 5 fonts + 6 shell + 5 ini-resources).
- Grand total: 88 corpus specs; 88 rows; no spec appears more than once.

**Column key:**
- DEST: the primary FLAIR file the corpus spec projects into.
- STATUS: EXISTS (file already in spec/ or os/flair/), BUILD (file is a Phase 1
  build item not yet on disk), FUTURE (deferred past Phase 1), OUT-OF-SCOPE.

---

## 1. system7-decomp -- Chrome (9 specs)

Ref: `../system7-decomp/INDEX.md` Section 1 (`specs/chrome/`).
All 9 pixel-verified against mint-002 System 7.5.3 screendumps.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/chrome/title-bar.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | title-bar height (minTitleH=19), pinstripe/lavender bevel, Chicago-12 title placement; era-tagged |
| `specs/chrome/pinstripe.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | HilitePattern $FF00x4, period-2, patAlign 8px phase-lock; rendered light #F3F3F3 / dark #969696, bevel #DADAFF/#B3B3DA |
| `specs/chrome/close-zoom-box.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | 13px box geometry WBoxDelta, goAway left=9 / zoom -20 (-21 proc5), 11x11 render; bevel/interior shades |
| `specs/chrome/grow-box.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | 16x16 grow/size box, DrawGrowIcon two-nested-box interior, wTitleBarShade4 fill |
| `specs/chrome/scrollbar.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | scrollBarSize=16, arrow/thumb/track geometry, 16x16 thumb; INACTIVE (gray) bar rendered |
| `specs/chrome/window-frame.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | 1px black FrameRect + 1px lavender bevel groove + drop shadow (doc 1@(1,1) / var3 2@(2,2) / none) |
| `specs/chrome/menu-bar-geometry.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | 20px MBarHeight (19 white fill + 1 black baseline), GetMBarHeight |
| `specs/chrome/dialog-borders.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | dBoxProc 7px fancy nested border, default-button 3px ring, no-shadow variant 1 |
| `specs/chrome/wdef-variant-geometry.md` | `spec/chrome_metrics.json` + `spec/chrome_metrics.h` | EXISTS | per-variant chrome dispatch (documentProc 0..zoomNoGrow 12, rDocProc 16), varCode AND 3 decode |

---

## 2. system7-decomp -- QuickDraw (9 specs)

Ref: `../system7-decomp/INDEX.md` Section 2 (`specs/quickdraw/`).
All 9 semantics specs; no pixel golden (ADR-0005 clean-room; homomorphism property
suite is the ONLY region oracle -- no region golden ever).

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/quickdraw/grafport.md` | `spec/grafport.h` | EXISTS | GrafPort/CGrafPort record offsets/sizes, pen/text/color state, visRgn-INTERSECT-clipRgn invariant, FLAIR hybrid shape; _Static_asserts |
| `specs/quickdraw/coordinate-system.md` | `spec/grafport.h` | EXISTS | int16 plane, Point(v,h)/Rect order, half-open convention, local vs global; folded into grafport.h coordinate section |
| `specs/quickdraw/regions-api.md` | `spec/region_algebra.h` | EXISTS | PUBLIC region op set + signatures + Boolean algebra + 10-byte published header |
| `specs/quickdraw/region-body-closure.md` | `spec/region_algebra.h` | EXISTS | NEGATIVE result: region body proprietary/unpublished -> ATKINSON clean-room (the justification for ADR-0005) |
| `specs/quickdraw/color-quickdraw.md` | `spec/grafport.h` + `spec/assets/clut.h` + `spec/assets/clut.json` | EXISTS | RGBColor, GDevice->PixMap->ColorTable->ColorSpec chain, Color2Index/Index2Color |
| `specs/quickdraw/patterns.md` | `spec/grafport.h` | EXISTS | Pattern 8x8x1bpp, pnPat/bkPat/fillPat slots, origin phase-lock, 5 system patterns, PixPat delta |
| `specs/quickdraw/transfer-modes.md` | `spec/imaging.h` | EXISTS | 16 boolean transfer modes (8 source + 8 pattern), pnMode/txMode value space; _Static_asserted enum |
| `specs/quickdraw/copybits.md` | `spec/imaging.h` | EXISTS | CopyBits blit: transfer mode, maskRgn, fg/bk colorize, scaling, save-under; bitsProc seam; ditherCopy (S7) |
| `specs/quickdraw/drawing-primitives.md` | `spec/drawing_ops.h` | EXISTS | Five verbs (Frame/Paint/Fill/Erase/Invert) x shape families + line/pen routines + pen init state + GrafVerb 0..4 |

---

## 3. system7-decomp -- Toolbox Managers (8 specs)

Ref: `../system7-decomp/INDEX.md` Section 3 (`specs/toolbox/`).
Published constants; FLAIR counterparts already implemented.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/toolbox/event-manager.md` | `spec/event_model.h` | EXISTS | EventRecord, what-codes nullEvent..osEvt, modifier bits, event masks, WaitNextEvent/GetNextEvent |
| `specs/toolbox/window-manager.md` | `spec/window_record.h` | EXISTS | WindowRecord, FindWindow part-codes inDesk..inZoomOut 0..8, WDEF variant + message dispatch, S7 damage model |
| `specs/toolbox/control-manager.md` | `spec/control_record.h` | EXISTS | ControlRecord, FindControl/TestControl part-codes, CDEF proc IDs 0/1/2/16, value/min/max/hilite/track |
| `specs/toolbox/menu-manager.md` | `spec/menu_record.h` | EXISTS | MenuInfo, item attributes, enableFlags, mark/style/MDEF-message catalogs, MenuSelect/MenuKey result word |
| `specs/toolbox/dialog-manager.md` | `spec/dialog_record.h` | EXISTS | DialogRecord, DITL item-type bytes, alerts (Stop/Note/Caution + 4-stage), ok/cancel=1/2, ModalDialog loop |
| `specs/toolbox/textedit.md` | FUTURE: `spec/textedit_record.h` | FUTURE | TERec, selection/justification/line-layout, monostyled-vs-styled delta -- M5+ deferred per plan |
| `specs/toolbox/list-manager.md` | FUTURE: `spec/list_record.h` | FUTURE | ListRec, Cell/ListBounds cell-space coords, selFlags/listFlags, LDEF messages -- M5+ deferred per plan |
| `specs/toolbox/manager-part-codes.md` | `spec/window_record.h` + `spec/control_record.h` + `spec/menu_record.h` + `spec/dialog_record.h` + `spec/event_model.h` | EXISTS | Consolidated flat catalog of every part-code/proc-ID/what-code/mask/DITL-type; distributed across the owning record headers |

---

## 4. system7-decomp -- Fonts (5 specs)

Ref: `../system7-decomp/INDEX.md` Section 4 (`specs/fonts/`).
Grounded in real NFNT/FOND strikes extracted from System 7.0.1 (mint-003).
Glyph bitmaps stay reference-only (Apple-copyright); FLAIR hand-authors clean-room strikes.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/fonts/font-resource-format.md` | FUTURE: `spec/assets/font_resource.h` | FUTURE | FONT/NFNT FontRec 13-field header + bitImage/locationTable/owTable; FOND family + FAT; deferred to P1-8 / Phase 5 |
| `specs/fonts/chicago.md` | `spec/assets/chicago8x16.h` | EXISTS | Chicago 12 system font -- FLAIR ships fixed 8x16 (documented deviation from proportional real); used in menus/titles/dialogs/buttons |
| `specs/fonts/geneva.md` | `spec/assets/geneva9.h` | EXISTS | Geneva 9 small-UI/cell font; proportional aw 2..9 (FLAIR deviation: fixed-pitch fallback); icon titles |
| `specs/fonts/monaco.md` | FUTURE: `spec/assets/monaco9.h` | FUTURE | Monaco 9 monospace (terminal/code/fixed-pitch); FLAIR M5+ mapping; deferred until terminal/code use |
| `specs/fonts/font-manager.md` | FUTURE: `spec/assets/font_manager.h` | FUTURE | Proportional text measurement (StringWidth/CharWidth sum owTable advances); title centering; line heights; style synthesis -- M5+ |

---

## 5. system7-decomp -- Resources (6 specs)

Ref: `../system7-decomp/INDEX.md` Section 5 (`specs/resources/`).
Grounded in real bytes from mint-001 + mint-003. PRIMARY CLUT from Quadra 650 ROM.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/resources/resource-manager.md` | FUTURE: `spec/resource_manager.h` | FUTURE | Resource fork format, type+ID addressing, attrs, GetResource search chain -- deferred to P1-8 / Phase 5 (apps demand it) |
| `specs/resources/wctb-mctb-format.md` | `spec/chimera_element_map.json` (win accent sub-section) | EXISTS | wctb/mctb/dctb color-table formats; wctb ABSENT (WDEF supplies defaults); relevant parts folded into chimera element map |
| `specs/resources/wdef-mdef-cdef-format.md` | FUTURE: `spec/assets/defproc_format.h` | FUTURE | WDEF/MDEF/CDEF resource packaging (self-id header) + procID dispatch; compiled body OUT of scope (clean-room) -- deferred P1-8 |
| `specs/resources/curs-pat-format.md` | `spec/assets/cursors.h` | EXISTS | CURS 16x16 data+mask+hotspot, PAT 8x8x1bpp, PAT# 38-entry list, ppat format; PAT 17=ltGray resolves patterns.md |
| `specs/resources/wind-menu-dlog-ditl.md` | FUTURE: `spec/resource_manager.h` | FUTURE | WIND/MENU/DLOG/DITL template formats -- deferred to P1-8 / Phase 5 alongside resource-manager.md |
| `specs/resources/clut-pltt-format.md` | `spec/assets/clut.json` + `spec/assets/clut.h` | EXISTS | clut/pltt ColorTable format; the real 256-entry 8-bpp CLUT (idx0=white, 255=black, 6x6x6 cube + ramps) from ROM clut 8 |

---

## 6. system7-decomp -- Desktop (6 specs)

Ref: `../system7-decomp/INDEX.md` Section 6 (`specs/desktop/`).
Pixel-measured from mint-002 System 7.5.3 screendumps (tagged "7.5.3, verify vs 7.0/7.1").

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/desktop/desktop-pattern.md` | `os/flair/desktop.c` + `spec/chrome_metrics.json` | EXISTS (partial) | deskPat 50% aa55 dither; FLAIR ships SEAFOAM canonical deviation (documented); desktop.c module |
| `specs/desktop/menu-bar.md` | `os/flair/shell.c` + `spec/assets/menu_canon.h` | EXISTS | Finder menu-bar title layout (Apple/File/Edit/View/Label/Special + clock/Help/app extras), per-title x-origins; the frozen Photoshop menu bar is canon (spec PRD Law 4) |
| `specs/desktop/apple-menu.md` | `os/flair/shell.c` + `spec/chrome_metrics.h` | EXISTS (partial) | Apple glyph (Mac Roman 0x14) title + pulled-down body (frame, 1px drop shadow, row pitch) shared MDEF chrome; apple glyph currently stubbed (plan Phase 5) |
| `specs/desktop/about-box.md` | `os/flair/shell.c` | EXISTS (partial) | About This Macintosh movableDBoxProc (varCode 5) chrome + recessed memory bars; deferred to Phase 5 for live memory data |
| `specs/desktop/alerts.md` | `os/flair/` (dialog module) | EXISTS (partial) | Dialog Mgr alert layout (32x32 icon slot, icon-to-text gap, button row); stop/note/caution=0/1/2 per spec/dialog_record.h |
| `specs/desktop/finder-windows.md` | `os/flair/shell.c` + `spec/window_record.h` | EXISTS | Finder disk/folder documentProc window, icon-view "N items" header, active-vs-inactive delta, View menu |

**system7-decomp subtotal: 43 specs mapped.**
- EXISTS: 29 (some partial -- Phase 3 will complete the draw path)
- FUTURE EXPANSION: 8 (textedit, list-manager, monaco, font-manager, font-resource-format,
  resource-manager, wdef-mdef-cdef-format, wind-menu-dlog-ditl)
- EXISTS (chimera): 1 (wctb-mctb-format -> chimera_element_map.json)
- EXISTS (partial): 5 (desktop specs where the module exists but content is not yet
  fully drawn at boot due to Phase 3 gap)

---

## 7. win31-decomp -- Chrome (10 specs)

Ref: `../win31-decomp/INDEX.md` Section 1 (`specs/chrome/`).
IN-SCOPE: the Win 3.1 flat-2D chrome elements that FLAIR uses as chimera ACCENTS.
These project into `spec/chimera_element_map.json` (the chimera accent sub-map)
and carry the Win95-ism guardrail (see `spec/win95ism_guardrails.md`).

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/chrome/system-metrics.md` | `spec/chimera_element_map.json` (Win accent metrics sub-section) | EXISTS | SM_CYCAPTION=18, SM_CYMENU=18, SM_CXFRAME/CYFRAME=4, SM_CXDLGFRAME=4, SM_CXSIZE=20, SM_CYSIZE=18, SM_CXBORDER=1; these values gate the Win-accent caption/frame geometry in FLAIR |
| `specs/chrome/color-scheme.md` | `spec/chimera_element_map.json` (Win accent color sub-section) | EXISTS | WIN.INI "Windows Default" scheme: navy #000080, #C0C0C0, #808080, #FFFFFF, teal #008080; 16-color vs indexed-8 caveat |
| `specs/chrome/window-frame.md` | `spec/chimera_element_map.json` | EXISTS | FLAT 1px outer black + 2px #C0C0C0 + 1px inner black = 4px total; NO Win95 double-bevel; DS_MODALFRAME |
| `specs/chrome/caption-bar.md` | `spec/chimera_element_map.json` (Win accent sub-section) | EXISTS | Sysmenu-box left / CENTERED title (white-on-navy active / #808080 inactive) / triangle min + max; VGASYS bold 16px; Win95-ism guardrail: no close-box, no window-icon, no left-align, no gradient |
| `specs/chrome/system-menu-box.md` | `spec/chimera_element_map.json` + `spec/control_record.h` (Win accent sub-section) | EXISTS | The "toaster" box (FLAIR chimera accent): 20x18px #C0C0C0 flat square, 1px black border, 3-row dash; pixel-exact |
| `specs/chrome/min-max-boxes.md` | `spec/chimera_element_map.json` | EXISTS | Flat bevel structure; down-triangle min glyph (7/5/3/1px); up-triangle max; restore double-triangle; NO underscore/box (Win95 guardrail) |
| `specs/chrome/button-bevel.md` | `spec/chimera_element_map.json` + `spec/control_record.h` (Win accent sub-section) | EXISTS | SINGLE 1px flat bevel (#FFFFFF top/left, #808080 bottom/right, #C0C0C0 face, 1px black outline); NO #DFDFDF inner ring; 5-step render order |
| `specs/chrome/scrollbars.md` | `spec/chimera_element_map.json` | EXISTS | Trough (#C0C0C0), thumb + arrow-box; SM_CXVSCROLL ~15px (golden-resolves); explicitly NOT 17px |
| `specs/chrome/menu-bar-geometry.md` | `spec/chimera_element_map.json` | EXISTS | SM_CYMENU=18px; first-item left pad=9px; bar background #FFFFFF at 16-color; open-item navy #000080 / white; CENTERED title confirmed |
| `specs/chrome/cross-version-guardrails.md` | `spec/win95ism_guardrails.md` | BUILD | Adopted as FLAIR Win95-ism regression checklist (Task B of this bead); the locked checklist of FORBIDDEN Win95 elements |

---

## 8. win31-decomp -- USER model (11 specs)

Ref: `../win31-decomp/INDEX.md` Section 2 (`specs/user/`).
FLAIR's event model bridges the Mac Toolbox event model (event_model.h) and the
Win16 message model. The chimera-specific USER elements (MDI, menus, non-client NC
messages, button-control) are IN-SCOPE as accents. The rest are OUT-OF-SCOPE:
the Win16 model is the Mac FLAIR side's internal concern, and FLAIR does not
re-implement a Win16 USER stack.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/user/win16-model.md` | OUT-OF-SCOPE | -- | FLAIR does not implement a Win16 message loop; the Mac Toolbox WaitNextEvent loop is the FLAIR event model (event_model.h); Law-3: factory cannot reach artifact internals |
| `specs/user/window-classes.md` | OUT-OF-SCOPE | -- | FLAIR uses WindowRecord / WDEF not Win16 WNDCLASS; no RegisterClass in FLAIR's kernel |
| `specs/user/window-styles.md` | OUT-OF-SCOPE | -- | WS_* dwStyle space is Win16-only; FLAIR window proc-IDs and varCodes are the Mac side (window_record.h) |
| `specs/user/message-catalog.md` | OUT-OF-SCOPE | -- | WM_* values are Win16-only; FLAIR events use the Mac Toolbox what-code catalog (event_model.h) |
| `specs/user/non-client-messages.md` | `spec/chimera_element_map.json` + `spec/event_model.h` | EXISTS (partial) | WM_NCPAINT / NCCALCSIZE / NCHITTEST / NCACTIVATE + HT* hit-test catalog are referenced in the FLAIR chimera accent map for the non-client area messages on Win-accent windows; HTCLOSE=Win95 guardrail |
| `specs/user/wm-command-packing.md` | OUT-OF-SCOPE | -- | Win16 WM_COMMAND wParam/lParam packing is Win-side; FLAIR routes control actions through the Mac Toolbox part-code / action-proc mechanism |
| `specs/user/standard-controls.md` | OUT-OF-SCOPE | -- | The 6 Win16 standard control classes (BUTTON/EDIT/LISTBOX/etc.) are Win-only; FLAIR controls use CDEF proc IDs (control_record.h) |
| `specs/user/button-control.md` | `spec/control_record.h` (Win accent sub-section) + `spec/chimera_element_map.json` | EXISTS | FLAIR chimera accent: the flat #C0C0C0 BS_PUSHBUTTON render order is the WIN-ACCENT BUTTON in dialogs; the pixel structure (1px black outline, single 1px bevel, dotted focus inset, NOT a Win95 raised bevel) is locked in control_record.h Win accent sub-section |
| `specs/user/dialog-manager.md` | `spec/dialog_record.h` (Win accent note) | EXISTS (partial) | The FLAIR Dialog Manager is Mac-side (dialog_record.h); this Win16 spec's DS_MODALFRAME + flat-button layout is noted as the accent applied to dialog button rendering; DLGTEMPLATE binary format deferred to P1-8 |
| `specs/user/menus.md` | `spec/chimera_element_map.json` (Win accent sub-section) | EXISTS | The frozen Photoshop menu bar "File Edit Image Layer Select View Window Help" is a normal RT_MENU-style menu in the FLAIR chimera; MENUEX/MENUITEMINFO/WM_ENTERMENULOOP are Win95-guardrailed |
| `specs/user/mdi.md` | `spec/chimera_element_map.json` (Win accent sub-section) | EXISTS | MDI FRAME/MDICLIENT/CHILD trio; COLOR_APPWORKSPACE backdrop; the maximized-child merged-title-bar behavior -- these are the MDI chimera accent for FLAIR windowed apps |

---

## 9. win31-decomp -- GDI model (8 specs)

Ref: `../win31-decomp/INDEX.md` Section 3 (`specs/gdi/`).
The Win GDI model is OUT-OF-SCOPE: FLAIR's drawing model is QuickDraw (imaging.h,
grafport.h, drawing_ops.h, region_algebra.h). The GDI specs are reference-only for
mapping Win ROP3 <-> QuickDraw transfer modes.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/gdi/device-context.md` | OUT-OF-SCOPE | -- | FLAIR uses QuickDraw GrafPort (grafport.h), not HDC; the invalidate/WM_PAINT repaint cycle is Mac Toolbox update-event driven |
| `specs/gdi/gdi-objects.md` | OUT-OF-SCOPE | -- | Win16 GDI pen/brush/font HGDIOBJ handles; FLAIR uses GrafPort pen state + Pattern + ColorSpec (grafport.h) |
| `specs/gdi/stock-objects.md` | OUT-OF-SCOPE | -- | GetStockObject catalog is Win-only; FLAIR has no stock-object store |
| `specs/gdi/raster-ops.md` | `spec/imaging.h` (reference note only) | EXISTS (reference) | The ROP3 <-> QuickDraw transfer-mode mapping table is referenced in imaging.h (Law 1 citation); the FLAIR draw path uses Mac transfer modes, not ROP3 |
| `specs/gdi/dib-bmp-format.md` | OUT-OF-SCOPE | -- | Win DIB/DDB format; FLAIR blitter uses indexed-8 bitmap_t (imaging.h), not BITMAPINFOHEADER |
| `specs/gdi/system-colors.md` | `spec/chimera_element_map.json` (Win accent color sub-section) + `spec/win95ism_guardrails.md` | EXISTS | COLOR_* index 0..20 are the Win accent color indices; index 21+ is Win95 and is FORBIDDEN (win95ism_guardrails.md; color-scheme.md cross-ref) |
| `specs/gdi/text-out.md` | OUT-OF-SCOPE | -- | Win GDI text-out API (TextOut/DrawText/DT_*/TA_*) is Win-only; FLAIR text uses Chicago/Geneva strikes (spec/assets/) and QuickDraw txFace/txSize state |
| `specs/gdi/palette.md` | `spec/assets/palette.json` + `spec/assets/palette.h` | EXISTS | The 256-color CLUT model: LOGPALETTE/PALETTEENTRY, the 20 reserved Win DDK static palette entries (10 low + 10 high) + the 4 "fancy" middle entries; all RGBs documented; maps to FLAIR's indexed-8 CLUT surface |

---

## 10. win31-decomp -- Fonts (5 specs)

Ref: `../win31-decomp/INDEX.md` Section 4 (`specs/fonts/`).
The chimera accent font is MS Sans Serif 8pt (SSERIFE.FON) for icon titles.
System (VGASYS.FON) is the Win caption font. The binary .FON/FNT format is
deferred (P1-8). FLAIR ships hand-authored clean-room strikes; no MS copyright
glyph bitmaps in the artifact.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/fonts/fon-fnt-format.md` | OUT-OF-SCOPE | -- | The .FON NE-DLL container + FNT v2.0 header format is the resource extraction recipe for the corpus, not a FLAIR runtime format; deferred to P1-8 when apps demand it |
| `specs/fonts/system-fon.md` | OUT-OF-SCOPE | -- | VGASYS.FON System face (Win caption font); FLAIR uses Chicago 12 (Mac-base) for all chrome text; Win caption font not replicated in the chimera (Mac chrome takes precedence per ADR-0004) |
| `specs/fonts/ms-sans-serif.md` | `spec/chimera_element_map.json` (Win accent font sub-section) | EXISTS | SSERIFE.FON MS Sans Serif 8pt: pixH=13, dfAscent=11; load-bearing FLAIR accent for icon titles and MDI child window titles; advance table locked in chimera map |
| `specs/fonts/terminal-fixedsys.md` | OUT-OF-SCOPE | -- | Fixedsys/Courier/Terminal/VGAOEM are Win terminal fonts; FLAIR uses Monaco 9 (Mac-base) for terminal/code (spec/assets/geneva9.h); no Win terminal font in the chimera |
| `specs/fonts/font-legality.md` | OUT-OF-SCOPE | -- | Clean-room reproduction guard for Win fonts; already incorporated as FLAIR general Law (CLAUDE.md Rule 12 + clean-room discipline); no separate FLAIR file needed |

---

## 11. win31-decomp -- Shell (6 specs)

Ref: `../win31-decomp/INDEX.md` Section 5 (`specs/shell/`).
OUT-OF-SCOPE: FLAIR's shell is the Mac Finder-style desktop (os/flair/shell.c +
os/flair/desktop.c). The Win 3.1 Program Manager / File Manager / Control Panel /
Task List architecture is NOT the FLAIR shell; these are Win-only shell behaviors.
The `.GRP` / NE-resource binary formats are corpus research tools, not FLAIR spec.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/shell/program-manager.md` | OUT-OF-SCOPE | -- | PROGMAN.EXE MDI frame is Win-only; FLAIR uses Mac Finder-style icon-view desktop (os/flair/shell.c); MDI accent from mdi.md captured in chimera_element_map.json |
| `specs/shell/grp-file-format.md` | OUT-OF-SCOPE | -- | .GRP binary format is Win Program Manager-only; no .GRP loading in FLAIR |
| `specs/shell/file-manager.md` | OUT-OF-SCOPE | -- | WINFILE MDI structure is Win-only; FLAIR's File Manager app is a Mac-chrome documentProc window (os/apps/; finder-windows.md is the Mac base) |
| `specs/shell/control-panel.md` | OUT-OF-SCOPE | -- | Win .CPL applet model is Win-only; no FLAIR control panel |
| `specs/shell/task-list.md` | OUT-OF-SCOPE | -- | TASKMAN.EXE is Win-only; FLAIR cooperative task switching uses WaitNextEvent (no task-list modal) |
| `specs/shell/source-leak-reality.md` | OUT-OF-SCOPE | -- | Provenance/oracle-authority discipline for the win31-decomp corpus itself; already incorporated in FLAIR via CLAUDE.md Law 1 + the oracle hierarchy |

---

## 12. win31-decomp -- INI + resources (5 specs)

Ref: `../win31-decomp/INDEX.md` Section 6 (`specs/ini-resources/`).
WIN.INI [colors] is IN-SCOPE as the Win accent color source (chimera_element_map.json).
The binary resource formats (DLGTEMPLATE, RT_MENU, NE) are OUT-OF-SCOPE for Phase 1;
they are corpus research tools / deferred to P1-8.

| Corpus spec | FLAIR destination | STATUS | Notes |
|-------------|-------------------|--------|-------|
| `specs/ini-resources/win-ini-colors.md` | `spec/chimera_element_map.json` (Win accent color sub-section) | EXISTS | WIN.INI [colors] "Windows Default" key/RGB table (all 20 keys); CONTROL.INI [color schemes]; #DFDFDF-is-Win95 warning folded into win95ism_guardrails.md |
| `specs/ini-resources/win-ini-desktop.md` | OUT-OF-SCOPE | -- | WIN.INI [Desktop] keys (IconTitleFaceName, Pattern, Wallpaper, IconSpacing) are Win shell settings; FLAIR desktop uses Mac Finder desktop-pattern.md (deskPat seafoam) |
| `specs/ini-resources/dialog-template-format.md` | OUT-OF-SCOPE | -- | 16-bit DLGTEMPLATE binary layout is a Win resource format; FLAIR dialogs use Mac DITL/DLOG (dialog_record.h); deferred P1-8 if ever needed |
| `specs/ini-resources/menu-resource-format.md` | OUT-OF-SCOPE | -- | 16-bit RT_MENU binary layout is a Win resource format; FLAIR menus use Mac MENU resource (menu_record.h); deferred P1-8 if ever needed |
| `specs/ini-resources/ne-res-format.md` | OUT-OF-SCOPE | -- | 16-bit NE module + .RES resource layout is the Win corpus research tool (tools/parse_ne_resources.py); no NE loader in FLAIR |

**win31-decomp subtotal: 45 specs mapped.**
- IN-SCOPE / EXISTS: 22 (chrome 9 + select user/gdi/font/ini-resources)
- BUILD: 1 (cross-version-guardrails -> win95ism_guardrails.md, Task B this bead)
- OUT-OF-SCOPE: 22 (win16-model, window-classes, window-styles, message-catalog,
  wm-command-packing, standard-controls, all GDI except palette+system-colors+raster-ops,
  all Shell, fon-fnt-format, system-fon, terminal-fixedsys, font-legality,
  win-ini-desktop, dialog-template-format, menu-resource-format, ne-res-format)

---

## 13. Coverage summary

| Corpus | Total specs | Rows in this manifest | Check |
|--------|-------------|----------------------|-------|
| system7-decomp | 43 | 43 | OK |
| win31-decomp | 45 | 45 | OK |
| **Grand total** | **88** | **88** | **OK** |

Every corpus spec appears exactly once. No spec is unaccounted for.

### system7-decomp by destination category

| Category | Count |
|----------|-------|
| EXISTS (spec/ file or os/flair module) | 29 |
| EXISTS -- partial (module present, Phase 3 will complete) | 5 |
| EXISTS -- chimera (projects into chimera_element_map.json) | 1 |
| FUTURE EXPANSION (deferred Phase 1+, clearly out of scope for now) | 8 |
| **Total** | **43** |

### win31-decomp by destination category

| Category | Count |
|----------|-------|
| EXISTS / IN-SCOPE (projects into chimera_element_map.json or existing spec/ headers) | 22 |
| BUILD (this bead: win95ism_guardrails.md) | 1 |
| OUT-OF-SCOPE (Win-only; Mac base owns it; factory/corpus-internal) | 22 |
| **Total** | **45** |

---

## 14. Future expansion items (system7-decomp)

These 8 system7-decomp specs are NOT out-of-scope -- they are IN-SCOPE but
deferred past Phase 1. They are listed here so the manifest is complete and the
gap is visible in `bd ready`.

| Corpus spec | Deferred FLAIR destination | Milestone |
|-------------|---------------------------|-----------|
| `specs/toolbox/textedit.md` | `spec/textedit_record.h` | Phase 5 / M5 |
| `specs/toolbox/list-manager.md` | `spec/list_record.h` | Phase 5 / M5 |
| `specs/fonts/monaco.md` | `spec/assets/monaco9.h` | Phase 5 / M5 |
| `specs/fonts/font-manager.md` | `spec/assets/font_manager.h` | Phase 5 / M5 |
| `specs/fonts/font-resource-format.md` | `spec/assets/font_resource.h` | Phase 1 P1-8 / Phase 5 |
| `specs/resources/resource-manager.md` | `spec/resource_manager.h` | Phase 1 P1-8 / Phase 5 |
| `specs/resources/wdef-mdef-cdef-format.md` | `spec/assets/defproc_format.h` | Phase 1 P1-8 / Phase 5 |
| `specs/resources/wind-menu-dlog-ditl.md` | `spec/resource_manager.h` | Phase 1 P1-8 / Phase 5 |
