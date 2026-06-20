<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# FLAIR Win95-ism Regression Checklist

**Document purpose:** LOCKED regression checklist of FORBIDDEN Win 95 elements
in FLAIR's chimera chrome. Adopted from
`../win31-decomp/specs/chrome/cross-version-guardrails.md` Section 3 (the
adversarially-verified win31-decomp boundary catalog). Every item below is a
Win 95 (or later) addition that must NEVER appear in a FLAIR spec file, header
constant, or rendered chimera. The chimera is Win 3.1 FLAT 2-D ACCENTS on a
System 7.0/7.1 BASE -- never Win 95.

**Ground truth source:**
`../win31-decomp/specs/chrome/cross-version-guardrails.md` Sec 2 + Sec 3.
`../win31-decomp/specs/chrome/color-scheme.md` (the canonical color values).
`../win31-decomp/specs/chrome/button-bevel.md`, `caption-bar.md`,
`min-max-boxes.md`, `window-frame.md`, `system-menu-box.md`.
`../win31-decomp/oracles/wine/include/winuser.h` line 909 (`/* win95 colors */`
boundary comment; COLOR_3DLIGHT index 22 = #DFDFDF first-hand confirmation).

**Governing plan:** `docs/plans/FLAIR-implementation-plan.md` Phase 0 (P0-a) +
Phase 1 (P1-7); `spec/CANON-MANIFEST.md` row for
`../win31-decomp/specs/chrome/cross-version-guardrails.md`.

**Era axis:** See `docs/plans/FLAIR-implementation-plan.md` Sec 1 "Era axis" --
System 7.0/7.1 BASE now; System 8 / Platinum era-delta LATER (additive only,
never by mutating these locked 3.1-era constants). The Win 3.1 accent layer is
likewise era-tagged; Win 95 and later are OUTSIDE the era window.

**This is a LOCKED spec-data file (CLAUDE.md Rule 8).** Changing it is a
deliberate act requiring an issue + worklog note. Items are strengthened (new
forbidden items added), never relaxed.

**Proposed grep gate (do NOT wire until the orchestrator authorizes -- see
Sec 4):** a simple shell gate the Makefile can run over spec/ to catch
Win95-ism leakage (colors and API names) before they bake in.

---

## 1. Colors -- Forbidden Win 95 values

| Item | Forbidden value | Correct Win 3.1 value | Corpus citation |
|------|-----------------|----------------------|-----------------|
| Button / control highlight color | `#DFDFDF` / `dfdfdf` / `COLOR_3DLIGHT` (index 22) | `#FFFFFF` (`COLOR_BTNHIGHLIGHT`, index 20) | `../win31-decomp/specs/chrome/button-bevel.md`; `color-scheme.md`; `winuser.h:911` under `/* win95 colors */` |
| Outer dark bevel ring | `COLOR_3DDKSHADOW` (index 21) | N/A -- no outer dark ring in Win 3.1; only `#808080` (`COLOR_BTNSHADOW`, index 16) bottom/right single bevel | `../win31-decomp/specs/chrome/button-bevel.md`; `winuser.h:910` |
| Gradient active caption | `COLOR_GRADIENTACTIVECAPTION` (index 27) | Flat solid navy `#000080` (`COLOR_ACTIVECAPTION`, index 2) | `../win31-decomp/specs/chrome/caption-bar.md`; `winuser.h:916` |
| Gradient inactive caption | `COLOR_GRADIENTINACTIVECAPTION` (index 28) | Flat solid gray `#808080` (`COLOR_INACTIVECAPTION`, index 3) | `../win31-decomp/specs/chrome/caption-bar.md`; `winuser.h:917` |
| Tooltip text color | `COLOR_INFOTEXT` (index 23) | N/A -- Win 3.1 has no tooltip support | `winuser.h:912` |
| Tooltip background color | `COLOR_INFOBK` (index 24) | N/A -- Win 3.1 has no tooltip support | `winuser.h:913` |
| Hot-light / hover color | `COLOR_HOTLIGHT` (index 26) | N/A -- Win 3.1 has no hot-tracking | `winuser.h:915` |
| Menu highlight color | `COLOR_MENUHILIGHT` (index 29) | `COLOR_HIGHLIGHT` (index 13) = navy `#000080` | `../win31-decomp/specs/chrome/menu-bar-geometry.md`; `winuser.h:918` |
| Menu bar background | `COLOR_MENUBAR` (index 30) | `COLOR_MENU` (index 4) = `#FFFFFF` at 16-color | `../win31-decomp/specs/chrome/menu-bar-geometry.md`; `winuser.h:919` |
| Win 95 3D alias names (slots exist in 3.1; 3D* NAMES do not) | `COLOR_3DFACE` / `COLOR_3DSHADOW` / `COLOR_3DHIGHLIGHT` | Use numeric indices 15 / 16 / 20 respectively | `../win31-decomp/specs/gdi/system-colors.md` Sec 5; `winuser.h:920-922` |

**Rule:** Any COLOR_* index > 20 is Win 95 or later and is FORBIDDEN in any
FLAIR spec or FLAIR chimera chrome code. The Win 3.1 COLOR_* index space is
0..20 (21 indices total). [verified: first-hand in winuser.h:909 `/* win95 colors */`]

---

## 2. Caption bar -- Forbidden Win 95 chrome elements

| Item | Forbidden (Win 95) | Correct Win 3.1 value | Corpus citation |
|------|-------------------|----------------------|-----------------|
| Close (X) button at right of caption | ANY close button widget on the caption | NO close button; close is reached ONLY through the system-menu toaster box (double-click or Alt+Space -> Close) | `../win31-decomp/specs/chrome/caption-bar.md`; `system-menu-box.md` |
| Window icon at left of caption | Miniature window icon replacing the toaster box | Featureless gray square toaster box (20x18px #C0C0C0, 1px black border, 3-row dash); NOT an icon | `../win31-decomp/specs/chrome/system-menu-box.md` |
| Left-aligned caption title | Title starting just right of the icon / toaster box | CENTERED title within [sysmenu-right .. minbutton-left] title area [verified: "Program Manager" centers at x=313, area center x=314 in win31 golden] | `../win31-decomp/specs/chrome/caption-bar.md` |
| Gradient caption fill | Two-color gradient (COLOR_GRADIENTACTIVECAPTION / GRADIENTINACTIVECAPTION) | Flat solid navy `#000080` active; flat solid gray `#808080` inactive | `../win31-decomp/specs/chrome/caption-bar.md`; `color-scheme.md` |
| HTCLOSE hit-test code | `HTCLOSE` (= 20) -- the Win 95 close-button hit region | HTSYSMENU (= 3) for the toaster box; no HTCLOSE in Win 3.1 | `../win31-decomp/specs/user/non-client-messages.md` |

---

## 3. Button and control bevel -- Forbidden Win 95 chrome elements

| Item | Forbidden (Win 95) | Correct Win 3.1 value | Corpus citation |
|------|-------------------|----------------------|-----------------|
| Second inner bevel ring (CTL3D double-bevel) | SECOND inner ring using `COLOR_3DLIGHT` (#DFDFDF) top/left + `COLOR_3DDKSHADOW` outer dark | SINGLE 1px bevel ONLY: `#FFFFFF` (BTNHIGHLIGHT) top/left + `#808080` (BTNSHADOW) bottom/right. NO second ring. | `../win31-decomp/specs/chrome/button-bevel.md` |
| #DFDFDF as button highlight | `#DFDFDF` anywhere in a button/control bevel | `#FFFFFF` (pure white) as the BTNHIGHLIGHT top/left bevel edge | `../win31-decomp/specs/chrome/button-bevel.md`; `re/win31-color-measurements.md` Part 4 WARNING 1 |
| DrawEdge API call | `DrawEdge` / `EDGE_RAISED` / `EDGE_SUNKEN` | Direct pixel drawing via Rectangle + MoveTo/LineTo in Win 3.1 USER control draw path | `../win31-decomp/specs/chrome/button-bevel.md` |
| WS_EX_CLIENTEDGE / WS_EX_STATICEDGE | Extended styles that signal Win 95 3-D sunken/static edges on controls | Win 3.1 flat controls; no 3-D extended-style edges | `../win31-decomp/specs/user/window-styles.md` |

---

## 4. Frame and border -- Forbidden Win 95 chrome elements

| Item | Forbidden (Win 95) | Correct Win 3.1 value | Corpus citation |
|------|-------------------|----------------------|-----------------|
| 3-D edge double-bevel frame | `DrawEdge` / `EDGE_RAISED` / `EDGE_SUNKEN` double-bevel ring around window or dialog | FLAT frame: 1px outer black + 2px `#C0C0C0` gray + 1px inner black = 4px total (SM_CXFRAME=4 verified) | `../win31-decomp/specs/chrome/window-frame.md` |
| CTL3D applied to dialog / controls | The CTL3D Win 95/NT look applied to dialogs/controls | Win 3.1 dialogs and controls are FLAT; 1px black outline + flat face + 1px bevel | `../win31-decomp/specs/chrome/button-bevel.md` |

---

## 5. Min / max buttons -- Forbidden Win 95 glyph shapes

| Item | Forbidden (Win 95) | Correct Win 3.1 value | Corpus citation |
|------|-------------------|----------------------|-----------------|
| Minimize glyph | Horizontal underscore bar | DOWN-POINTING TRIANGLE (rows 7..10 of face: 7/5/3/1 px wide) | `../win31-decomp/specs/chrome/min-max-boxes.md` |
| Maximize glyph | Square / box outline | UP-POINTING TRIANGLE | `../win31-decomp/specs/chrome/min-max-boxes.md` |
| Restore glyph | Split small-box (up+down split) | DOUBLE-TRIANGLE | `../win31-decomp/specs/chrome/min-max-boxes.md` |
| Close button on caption | Any close (X) button in caption | NONE -- close is toaster-box-only (see Sec 2 above) | `../win31-decomp/specs/chrome/caption-bar.md` |

---

## 6. Menu bar and menus -- Forbidden Win 95 elements

| Item | Forbidden (Win 95 or later) | Correct Win 3.1 value | Corpus citation |
|------|----------------------------|----------------------|-----------------|
| Hot-tracking on menu bar items | Hover highlight on menu bar items without click | Win 3.1 highlights ONLY when the popup is open (click-driven); no hover | `../win31-decomp/specs/chrome/menu-bar-geometry.md` |
| Gradient or themed menu fills | Any gradient / themed menu background | Flat white menu bar; flat `#C0C0C0` popup background | `../win31-decomp/specs/chrome/menu-bar-geometry.md` |
| COLOR_MENUBAR (index 30) | Using index 30 for the menu bar | Use `COLOR_MENU` (index 4) | `../win31-decomp/specs/gdi/system-colors.md` |
| MENUEX template (version 1) | RT_MENU header version=1; MENUITEMINFO / InsertMenuItem | Old-style RT_MENU version=0; AppendMenu / ModifyMenu | `../win31-decomp/specs/user/menus.md` Sec 5 / 7 |
| WM_ENTERMENULOOP / WM_EXITMENULOOP | Message values 0x0211 / 0x0212 (Win 2000+) | `WM_ENTERIDLE` 0x0121 + `MSGF_MENU` 2 | `../win31-decomp/specs/user/menus.md` Sec 3.3 |
| TrackPopupMenuEx | Win 95+ extended popup alignment flags | `TrackPopupMenu` with 5 flags only | `../win31-decomp/specs/user/menus.md` Sec 6 |

---

## 7. API and resource format -- Forbidden Win 95 constructs

| Item | Forbidden (Win 95 or later) | Correct Win 3.1 form | Corpus citation |
|------|----------------------------|---------------------|-----------------|
| DLGTEMPLATEEX / DLGITEMTEMPLATEEX (version 1) | Extended 32-bit dialog template (Win 95) | Old-style packed BYTE/WORD DLGTEMPLATE / DLGITEMTEMPLATE | `../win31-decomp/specs/ini-resources/dialog-template-format.md` |
| DEFAULT_GUI_FONT (index 17) | GetStockObject(DEFAULT_GUI_FONT) = Win 95 | `SYSTEM_FONT` (index 13) is the chrome font in Win 3.1 | `../win31-decomp/specs/gdi/stock-objects.md`; `STOCK_LAST 16` boundary |
| DC_BRUSH (18) / DC_PEN (19) stock objects | Win 2000+ stock object indices | Win 3.1 stock range is 0..16 only | `../win31-decomp/specs/gdi/stock-objects.md` |

---

## 8. Proposed grep gate (NOT WIRED -- orchestrator authorization needed)

The following shell recipe, when added to the Makefile (or run as a standalone
check), catches the most dangerous Win95-ism leakage in spec/ source files.
It does NOT replace reading this checklist; it is a fast mechanical first-pass.

The orchestrator should add this as a `check-win95isms` Makefile target,
guarded by the same `loud-skip` pattern used for `need_flair_goldens`.

```
# Grep gate: assert no Win95 color values or API names in spec/ files.
# Run: make check-win95isms
# Fails loudly if any forbidden token appears as a live constant.
#
# Tokens checked:
#   dfdfdf / DFDFDF   -- COLOR_3DLIGHT (#DFDFDF), the canonical Win95 tell
#   COLOR_3DLIGHT     -- the symbolic name for index 22
#   COLOR_3DDKSHADOW  -- index 21
#   COLOR_3DFACE / COLOR_3DSHADOW / COLOR_3DHIGHLIGHT  -- Win95 alias names
#   COLOR_GRADIENTACTIVECAPTION / COLOR_GRADIENTINACTIVECAPTION
#   COLOR_HOTLIGHT / COLOR_MENUHILIGHT / COLOR_MENUBAR
#   DrawEdge / EDGE_RAISED / EDGE_SUNKEN / CTL3D
#   DLGTEMPLATEEX / DLGITEMTEMPLATEEX
#   WM_ENTERMENULOOP / WM_EXITMENULOOP
#   DEFAULT_GUI_FONT  -- GetStockObject index 17 (Win 95)
#   HTCLOSE           -- hit-test code for the Win95 close button

check-win95isms:
	@echo "[check-win95isms] scanning spec/ for Win95-ism leakage..."
	@if grep -rniP \
	    'dfdfdf|COLOR_3DLIGHT|COLOR_3DDKSHADOW|COLOR_3DFACE|COLOR_3DSHADOW|COLOR_3DHIGHLIGHT|COLOR_GRADIENTACTIVECAPTION|COLOR_GRADIENTINACTIVECAPTION|COLOR_HOTLIGHT|COLOR_MENUHILIGHT|COLOR_MENUBAR|DrawEdge|EDGE_RAISED|EDGE_SUNKEN|CTL3D|DLGTEMPLATEEX|DLGITEMTEMPLATEEX|WM_ENTERMENULOOP|WM_EXITMENULOOP|DEFAULT_GUI_FONT|HTCLOSE' \
	    spec/ --include='*.json' --include='*.h' --include='*.c' \
	    2>/dev/null | grep -v 'win95ism_guardrails'; then \
	    echo "[FAIL] Win95-ism token found in spec/ -- see win95ism_guardrails.md"; \
	    exit 1; \
	fi
	@echo "[check-win95isms] PASS -- no Win95-ism tokens found in spec/"
```

**Exclusion:** the grep excludes `win95ism_guardrails.md` itself (which
necessarily contains the forbidden strings as documentation). It also excludes
the corpus directories `../system7-decomp/` and `../win31-decomp/` (those are
reference-only, not artifact spec files).

**False-positive caveat:** a spec file that mentions `#DFDFDF` as a
FORBIDDEN value in a comment (e.g. a cross-reference to this file) will
trigger the grep. Either route such comments through a `FORBIDDEN:` prefix
that is excluded from the grep, or keep such citations inside this file only.

**Mutation-proof discipline (CLAUDE.md Rule 6):** after wiring this gate,
perturb the implementation by inserting `#DFDFDF` as a dummy constant in
one spec/ header, confirm the gate goes red, then restore and confirm it goes
green. Only then is the gate considered mutation-proven.

---

## 9. Checklist (the canonical reviewer form)

A reviewer checking any FLAIR spec/ file or os/flair/ chrome code runs the
following. Every item maps to a Sec 1-7 entry above with its citation.

### 9.1 Colors

- [ ] No `#DFDFDF` / `dfdfdf` anywhere -- that is `COLOR_3DLIGHT` (index 22, Win 95).
- [ ] Button / control highlight is `#FFFFFF` (white), NOT `#DFDFDF`.
- [ ] No `COLOR_*` index > 20 referenced (Win 3.1 range is 0..20 only).
- [ ] No `COLOR_3DFACE` / `COLOR_3DSHADOW` / `COLOR_3DHIGHLIGHT` name used
      (Win 95 aliases; use numeric indices 15/16/20 for Win 3.1).
- [ ] No gradient caption colors (`COLOR_GRADIENTACTIVECAPTION` / `GRADIENTINACTIVECAPTION`).
- [ ] No `COLOR_HOTLIGHT` (Win 2000), `COLOR_MENUHILIGHT` (Win XP),
      `COLOR_MENUBAR` (Win XP).

### 9.2 Caption bar

- [ ] Caption title is CENTERED (not left-aligned).
- [ ] No close (X) button in the caption.
- [ ] No window icon at the left of the caption (only featureless gray toaster box).
- [ ] Caption fill is a FLAT solid navy (`#000080`), not a gradient.
- [ ] Inactive caption is flat gray (`#808080`), not gradient.
- [ ] No `HTCLOSE` hit-test code used.

### 9.3 Button / control bevel

- [ ] Exactly ONE bevel ring: BTNHIGHLIGHT (`#FFFFFF`) top/left + BTNSHADOW
      (`#808080`) bottom/right. NO second inner ring (CTL3D double-bevel).
- [ ] No `DrawEdge` / `EDGE_RAISED` / `EDGE_SUNKEN` API call.
- [ ] No `#DFDFDF` as the button highlight (it is `#FFFFFF` in Win 3.1).
- [ ] No `WS_EX_CLIENTEDGE` / `WS_EX_STATICEDGE` extended styles on controls.

### 9.4 Frame / border

- [ ] Frame is FLAT: 1px outer black + 2px gray + 1px inner black.
      No double-bevel raised/sunken edge structure.
- [ ] Dialog frame (`DS_MODALFRAME`) is the same flat 1+2+1 structure.

### 9.5 Min / max buttons

- [ ] Minimize = DOWN-POINTING TRIANGLE glyph, not an underscore.
- [ ] Maximize = UP-POINTING TRIANGLE glyph, not a square/box outline.
- [ ] Restore = DOUBLE-TRIANGLE, not a split small box.
- [ ] No close (X) button (see 9.2).

### 9.6 Menu bar

- [ ] No hot-tracking on menu bar items (Win 98+).
- [ ] No gradient or themed menu background.
- [ ] No `COLOR_MENUBAR` (index 30) or `COLOR_MENUHILIGHT` (index 29).
- [ ] Open / highlighted item uses `COLOR_HIGHLIGHT` (navy `#000080`).

### 9.7 API / resource format

- [ ] No `MENUEX` template (version 1); old-style `RT_MENU` (version 0) only.
- [ ] No `MENUITEMINFO` / `InsertMenuItem`; use `AppendMenu` / `ModifyMenu`.
- [ ] No `WM_ENTERMENULOOP` / `WM_EXITMENULOOP`; use `WM_ENTERIDLE` 0x0121 +
      `MSGF_MENU 2`.
- [ ] No `DLGTEMPLATEEX` / `DLGITEMTEMPLATEEX` (version 1); old-style
      packed BYTE/WORD `DLGTEMPLATE` / `DLGITEMTEMPLATE` only.
- [ ] `GetStockObject` uses indices 0..16 only; no `DEFAULT_GUI_FONT` (17),
      `DC_BRUSH` (18), or `DC_PEN` (19).
