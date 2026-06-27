<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# FLAIR GUI Bug Hunt -- 2026-06-28 (Round 1)

Method: drove the live desktop via qemu_harness --mouse/--keys + screendump, inspected frames vs the ../system7-decomp goldens + spec, plus code-logic lenses over the Phase 4.5 modules. 14 lenses -> 93 raw -> 63 verified REAL -> 49 deduped (adversarial verify killed 27 NOT_REAL / 1 canon / 2 known-dup). Label `gui-bughunt`.

**Counts:** 49 total (7 P1, 42 P2). Known pre-filed (not re-filed): initech-pipa, initech-34gp, initech-rgt8, initech-z1f5.

## Executive summary

The GUI is worst in two layers. (1) The compositor / App-Contract path is broken: dragging one window erases a peer window entirely (3->2) and an app-switch erases the background NOTES window while leaving an orphaned frameless gray fragment on the teal desktop — non-WindowMgr layers (menu bars, modal, drop shadows, scrollbars) are not damage-tracked, and the app-switch also clobbers the static System-7 menu bar so both chimera bars collapse to identical Photoshop content with no Apple slot. (2) The Window Manager has no active/inactive chrome distinction at all — every window unconditionally draws the active racing-stripe title bar (the tracked hilited flag is never read), destroying the System-7 depth cue. Chrome/menu fidelity is riddled with authored placeholders and omissions: the Apple menu is a solid black square, dropped menu panels are gray instead of white, the grow box and horizontal scrollbar are entirely missing, and the FILE COPY modal is a borderless dBoxProc with a permanently-empty 0% progress bar instead of the reference's titled moveable modal with ~65% fill. Beneath the visible GUI sits a deep tail of latent C defects (integer overflows, missing NULL/contract guards, mutation-unproven oracle paths) in the List, Scrap, Resource, Process and PS/2-input managers.

## Bugs

### 1. [P1] No inactive-window chrome: flair_draw_document_window has no hilited param and always draws the active racing-stripe title bar
- **bead:** initech-a9iq  **subsystem:** FLAIR/chrome
- **evidence:** chrome.h:62 `void flair_draw_document_window(GrafPort*, rgn_rect_t, const char*)` has no hilited/active arg. chrome.c:197-318 unconditionally draws bevel rows + 15 pinstripe rows + close/zoom gadgets with no inactive branch. desktop.c:114-119 paint_window_chrome reads w->titleHandle but never w->hilited. window.c:174-182 reaffirm_active correctly sets w->hilited=0 for background windows, but no renderer consumes it; ref_tenant.c:347-349 comment admits 'the renderer draws no active/inactive chrome distinction'. flair_appswitch_pre.png: both foreground and background windows (HELLO and NOTES) show identical pinstripe title bars with full close/zoom boxes. Ground truth s7_701hd_afterw1.png shows the background window with a plain solid title bar.
- **expected:** Background (hilited=0) windows render a plain solid-gray title bar with no pinstripes/bevel rows and de-emphasized gadgets; only the frontmost (hilited=1) window shows the racing stripe. flair_draw_document_window must take a hilited flag and branch on it (StandardWDEF wHilited path).
- **actual:** Every window, including background windows, renders the full active pinstripe title bar. A user cannot tell the active window from background ones by chrome.
- **repro:** Boot the two-tenant App Contract scene (HELLO + NOTES). Inspect flair_appswitch_pre.png: both title bars show alternating light/dark stripe rows. Compare to s7_701hd_afterw1.png.

### 2. [P2] cbox() inner-bottom dark ring fills sz-4 (7) cols instead of sz-3 (8) — corner pixel (col 9,row 9) left as bevel/teal on every close/zoom box
- **bead:** initech-hber  **subsystem:** FLAIR/chrome (chrome.c cbox)
- **evidence:** chrome.c:181 `cfill(port, bx0+2, by0+last-1, sz-4, dark)` — with sz=11,last=10 this fills cols 2..8 (7 cols). The function's own ASCII diagram at chrome.c:129 specifies row 9 = `D ^ D D D D D D D D ^` (8 dark cols, 2..9). The inner-right dark ring (chrome.c:177-179) covers col 9 only for rows 2..8 (stops at by1-2), so the corner pixel (col 9,row 9) is set by neither and stays bevel from the initial crect. Pixel scan of chrome_window.png close box (display y=86-87 = row 9, x=116-117 = col 9) reads raw_idx=0 (teal #8DDCDC) not raw_idx=1 (dark); identical defect at zoom box x=698-699. Golden system7-decomp/specs/chrome/close-zoom-box.md confirms col 9 is dark on the inner-bottom row.
- **expected:** Inner-bottom dark ring spans 8 cols (sz-3) meeting the inner-right column at corner (col 9,row 9), closing the ring. Fix: chrome.c:181 `sz-4` -> `sz-3`.
- **actual:** Ring spans 7 cols; corner pixel (col 9,row 9) is bevel-colored, leaving a 1px teal notch at the bottom-right of the inner ring on every close and zoom box.
- **repro:** chrome_window.png: close box row 9 (display y=86-87) col 9 (display x=116-117) = teal; same at zoom box (x=698-699).

### 3. [P2] 8bpp palette has no BEVEL_SHADOW slot — close/zoom box dark ring and title bevel-lo row quantize to pure black, collapsing the 3-D double bevel
- **bead:** initech-l0ra  **subsystem:** FLAIR/chrome (flair_look.c color policy)
- **evidence:** flair_look.c:117-119 8bpp else-branch maps FLAIR_PART_BEVEL_SHADOW to CIDX_TITLE_INK (idx 4 = #000000). color_canon.json bevel_shadow = #4E9BA3 (teal-dark) but the 9-entry CLUT (idx 0-8) has no teal-dark slot, so the resolver falls back to black; 32bpp path (flair_look.c:122) correctly returns INITECH_CANON_BEVEL_SHADOW_RGB=0x4E9BA3. Effect 1 (close/zoom): the box dark elements (outer frame, inner-right ring col 9 rows 2-8, inner-bottom ring) all read 0x04 black, identical to FLAIR_PART_FRAME (idx 0). Effect 2 (title bevel): chrome_window.png column x=400, native rows 30/47/48 (outer frame, bevel-lo, shared frame) all rgb(0,0,0) — the bevel-lo row is invisible, making a 2px double black line at the title/content boundary.
- **expected:** Add a dedicated CIDX_BEVEL_SHADOW (e.g. idx 9) so the 8bpp path returns teal-dark #4E9BA3, giving three distinct bevel tones (dark-ring teal-dark, bevel-hi teal, face gray) and a visible bevel-lo groove.
- **actual:** BEVEL_SHADOW renders black in 8bpp, merging with the outer frame; the close/zoom box reduces to two tones and the title-bar groove disappears.
- **repro:** Pixel-probe close box inner-right column (col 9 rows 2-8) at 8bpp: 0x04 (black). chrome_window.png x=400 native rows 47-48 both black.

### 4. [P2] Title text left-edge clamp is struct.left+23 but the independent golden specifies struct.left+32
- **bead:** initech-lwh4  **subsystem:** FLAIR/chrome (title centering)
- **evidence:** chrome.c:393 `int box_clear = 9 + FLAIR_CHROME_WBOX_RENDER + 3;` = 9+11+3 = 23. The independent golden spec/chrome_fidelity_golden.h states 'centering indent clamps right of the go-away box (title-bar.md x=left+32)'. 32-23 = 9 native px too little clearance for any title whose centered position lands between left+23 and left+32 (narrow windows / short titles).
- **expected:** box_clear = 32 native px per title-bar.md.
- **actual:** box_clear = 23; a short-titled narrow window pins the title 9px closer to the close box than the WDEF reference.
- **repro:** Render struct.left=0, w=60, title='OK': title starts at x=23 instead of x=32. Current wide test windows center past 32 so the clamp does not fire.

### 5. [P2] Zoom box glyph is a 3x3 centred hollow frame instead of the ~7x7 top-left-anchored nested square
- **bead:** initech-jxsf  **subsystem:** FLAIR/chrome (zoom box glyph)
- **evidence:** chrome.c:185-190 `cframe(port, bx0+4, by0+4, bx0+7, by0+7, dark)` — a 3x3 hollow frame centred in the 11x11 box. WDEF TingeZoom (refs/StandardWDEF_a.txt:1573-1578) does FrameRect on rectReg with botRight-(4,4), yielding a 7x7 frame anchored at the box top-left; after the outer bevel overwrites row0/col0 the visible glyph is a ~6x6 top-left-quadrant partial square. s7_701_titlebar_8bpp_zoom.png shows the zoom glyph spanning much more interior than a centred 3x3.
- **expected:** ~7x7 FrameRect nested square anchored at the box top-left interior (BEVEL_SHADOW), visible as a ~6x6 partial square after the bevel overwrite.
- **actual:** A 3x3 centred frame — roughly half size and wrong position; close and zoom boxes look nearly identical except for a small centred dot.
- **repro:** Zoom into the top-right gadget in chrome_window.png; compare glyph size/position to s7_701_titlebar_8bpp_zoom.png.

### 6. [P2] documentProc window missing grow box (DrawGrowIcon) at bottom-right
- **bead:** initech-h6y1  **subsystem:** FLAIR/chrome
- **evidence:** spec/chrome_metrics.h defines FLAIR_CHROME_GROW=16 plus five grow-box color constants and a _Static_assert tying GROW to SCROLLBAR_W; spec/chrome_fidelity_golden.h documents the grow-box geometry. grep of os/flair/chrome.c finds zero references to FLAIR_CHROME_GROW or any grow-box draw. window.c:501-508 implements inGrow hit-testing but the draw side is absent. chrome_window.png and s7_701_8bit_docwindow.png comparison: the golden shows the 16x16 nested-square grow icon at bottom-right; FLAIR shows plain content.
- **expected:** A 16x16 DrawGrowIcon nested-squares gadget (GROUND/FIGURE/HIGHLIGHT/FILL per spec) at the bottom-right where the two scrollbars meet.
- **actual:** No grow box drawn; bottom-right corner is plain content/frame.
- **repro:** Inspect chrome_window.png bottom-right corner — no grow gadget.

### 7. [P2] documentProc windows have no horizontal scrollbar — only the vertical bar is drawn
- **bead:** initech-q2m9  **subsystem:** FLAIR/chrome
- **evidence:** chrome.c flair_draw_document_window draws one 16px vertical scrollbar on the right (lines 451-621); no horizontal scrollbar section exists anywhere in the function. The bottom content edge meets the outer frame directly. s7_701_8bit_docwindow.png shows both a right vertical and a bottom horizontal scrollbar meeting at the grow box.
- **expected:** documentProc windows carry both a right vertical and a bottom horizontal scrollbar meeting at the 16x16 grow box.
- **actual:** Only the vertical scrollbar is rendered; the bottom edge has no horizontal bar.
- **repro:** Inspect chrome_window.png / flair_desktop.png document windows: no bottom scrollbar.

### 8. [P2] Right vertical scrollbar extends to the full content bottom instead of stopping at the grow-box row
- **bead:** initech-kvxl  **subsystem:** FLAIR/chrome
- **evidence:** chrome.c:506-507 `int sb_top = content_top; int sb_bot = content_bot;` — sb_bot = bottom-fr with no subtraction for FLAIR_CHROME_GROW (16). chrome_window.png: the down-arrow box bottom touches the bottom window frame with zero gap. In s7_701_8bit_docwindow.png the right scrollbar terminates 16px above the frame (top of grow box).
- **expected:** sb_bot = content_bot - FLAIR_CHROME_SCROLLBAR_W, leaving the bottom 16px for the grow box.
- **actual:** Down-arrow box bottom touches the window frame, occupying the grow-box space.
- **repro:** chrome_window.png bottom-right: down-arrow box reaches the bottom frame with no break.

### 9. [P2] chrome.c scrollbar down-arrow glyph placed 1 row too high — asymmetric with the up-arrow
- **bead:** initech-l9xt  **subsystem:** FLAIR/chrome
- **evidence:** chrome.c:594-596 up-arrow: `gy=sb_top+1; y2=gy+1+k` (1 blank interior row before apex). chrome.c:607-611 down-arrow: `dy=sb_bot-sb_w+1; y2=dy+k` (0 blank rows before base). The comment at chrome.c:591 claims the down-arrow is the vertical mirror, but the missing +1 breaks the symmetry.
- **expected:** Down-arrow loop uses `y2 = dy + 1 + k` to match the up-arrow's 1-row blank gap.
- **actual:** Down-arrow base sits immediately below the inner separator (0 blank rows above, 4 below); asymmetric with the up-arrow.
- **repro:** Pixel-inspect the down-arrow box interior in chrome_window.png: base bar abuts the inner separator with no gap.

### 10. [P2] ControlRecord scrollbar track and idle thumb fill use BTNFACE (#C0C0C0) instead of PIN_LIGHT (#F3F3F3)
- **bead:** initech-ml5c  **subsystem:** FLAIR/control (control.c)
- **evidence:** control.c:63 `CTRL_CONTROL = FLAIR_PART_BTNFACE`; control.c:535 track fill and control.c:544 thumb face both use CTRL_CONTROL. FLAIR_PART_BTNFACE = #C0C0C0 (idx 6). spec/chrome_fidelity_golden.h:382-384 FG_SB_FACE_IDX = 7 (FLAIR_PART_PIN_LIGHT, #F3F3F3); scrollbar.md: 'arrow-box face + page track #F3F3F3 (idx 7), SOLID'.
- **expected:** Track and idle-thumb face use FLAIR_PART_PIN_LIGHT (idx 7, #F3F3F3).
- **actual:** Track and thumb face render medium gray #C0C0C0 (idx 6), darker than the System-7 golden.
- **repro:** DrawControl a scrollBar ControlRecord; track/thumb face appear #C0C0C0 instead of #F3F3F3.

### 11. [P2] ControlRecord scrollbar inner arrow-box separator drawn in FRAME/black instead of PIN_DARK/gray
- **bead:** initech-359n  **subsystem:** FLAIR/control (control.c)
- **evidence:** control.c:553 and :568 `cframe_ctrl(..., CTRL_BLACK)` draw all four edges of each arrow box (including the inner separator facing the track) in FRAME/black (idx 0). spec/chrome_fidelity_golden.h:374-379: outer edges idx 0, but FG_SB_SEPARATOR_IDX=8 (PIN_DARK/gray) for the inner separator; the golden comment at :358-364 explicitly flags the all-black-cframe pattern as WRONG.
- **expected:** Only outer edges FRAME/black; inner separators between arrow box and track = PIN_DARK gray (idx 8, #969696).
- **actual:** All four edges black; the inner separator is black instead of gray — the exact topology the golden documents as wrong.
- **repro:** DrawControl a scrollBar; the row between up-arrow box and track is #000000 instead of #969696.

### 12. [P2] ControlRecord scrollbar arrow glyph is a 3-row solid filled triangle instead of the 10-row hollow triangle-on-stem
- **bead:** initech-eppk  **subsystem:** FLAIR/control (control.c)
- **evidence:** control.c:558-561 (up) fill solid spans 1,3,5; control.c:572-575 (down) spans 5,3,1 — a 3-row solid wedge. spec/chrome_fidelity_golden.h:341-350 + scrollbar.md require a 10-row hollow-outlined triangle-on-stem in PIN_DARK (FG_SB_GLYPH_MIN_PX=8). The correct glyph is implemented in chrome.c:568-590 (apex/barbs/shaft/base) — a fundamentally different shape from control.c's solid triangle.
- **expected:** 10-row hollow triangle-on-stem in PIN_DARK matching chrome.c up_glyph[] table.
- **actual:** A 3-row solid filled triangle — wrong shape, wrong pixel count, solid instead of outline.
- **repro:** DrawControl a scrollBar; arrow boxes show a tiny solid triangle, not the hollow stem+triangle.

### 13. [P1] Apple menu slot rendered as a solid black filled square, not an apple glyph
- **bead:** initech-yx4v  **subsystem:** FLAIR/menu (menu.c)
- **evidence:** menu.c DrawMenuBar:365-373 fills the apple slot via blitter_fill_rect_clipped(bm, apple, fg, clip) — a solid rectangle in fg (CIDX_BLACK); the comment at menu.c:339 admits it is 'a contrasting square so the canon apple slot is visibly present'. No apple SICN/ICON bitmap exists in spec/assets/ (only chicago8x16.h, geneva9.h, cursors.h). flair_desktop.png top-left (x~2-18,y~2-17) shows a solid black square. Real goldens s7_701_8bit_docwindow.png / cal_filemenu.png show the apple-with-bite glyph. (Apple slot present is canon; a solid square is not.)
- **expected:** A ~16x15 monochrome apple-with-bite glyph centred in the Apple slot, per the System-7 reference.
- **actual:** A solid black filled rectangle the size of the slot; visible in flair_desktop.png and flair_appswitch_pre.png.
- **repro:** Boot any scene with has_apple=1 (bar_sys); inspect top-left of the first menu bar.

### 14. [P2] HiliteMenu never called: the active menu title is not inverted while its panel is open (dead code)
- **bead:** initech-3pl4  **subsystem:** FLAIR/menu (menu.c, kmain.c)
- **evidence:** flair_live_do_menu (kmain.c:1257-1345) has no HiliteMenu call and no bar-title inversion blit. HiliteMenu is defined (menu.c:225-239) and declared (menu.h:315) but has zero callers — dead code. flair_menu.png: with the File panel dropped, the 'File' title (x~20-80,y=0-20) appears normal (black-on-white), not inverted.
- **expected:** While a menu is open, its bar title slot is inverted (filled rect with title redrawn in bg), matching System-7 MenuSelect.
- **actual:** The title stays uninverted for the whole menu-open lifetime; HiliteMenu is never invoked.
- **repro:** Click 'File' in bar 1 on the live 386; the title stays black-on-white while the panel is open.

### 15. [P2] Dropped menu panel body renders as BTNFACE gray (#C0C0C0) instead of white (#FFFFFF)
- **bead:** initech-zw9c  **subsystem:** FLAIR/menu (kmain.c)
- **evidence:** kmain.c:1213 `#define FLAIR_MENU_PANEL_BG_IDX 6u /* BTNFACE gray (CIDX_CONTROL) */` = #C0C0C0; all three flair_draw_menu_panel calls (kmain.c:1280,1315,1334) pass it. build/flair_menu.ppm y=21..36 x=21..88 (panel interior) = solid #C0C0C0, zero #FFFFFF. The test oracle test_menu.c:142 uses bg=1 (CIDX_WHITE) — a direct code-vs-oracle divergence. System-7 menu panels EraseRect to white (#FFFFFF); CIDX_CONTROL gray is a Win-3.1 control face.
- **expected:** FLAIR_MENU_PANEL_BG_IDX = 1u (CIDX_WHITE) / idx 3 (CIDX_MENUBAR), white body, matching the test oracle and System-7.
- **actual:** Panel body is gray #C0C0C0, giving a Win-3.1 control-panel look.
- **repro:** Drop a menu on the live 386; non-hilited rows are gray. `set(panel pixels in build/flair_menu.ppm[21:37,21:89])` -> {(192,192,192)}.

### 16. [P2] Dropped menu panel has no drop shadow — 1px frame only
- **bead:** initech-ek9g  **subsystem:** FLAIR/menu (menu.c flair_draw_menu_panel)
- **evidence:** flair_draw_menu_panel (menu.c:406-420) draws exactly four 1px frame lines (FLAIR_MENU_PANEL_FRAME=1) then items; there is no pass drawing a right-column or bottom-row shadow outside the panel rect. The window WDEF shadow convention (1px L at offset (1,1), FG_SHADOW_OFFSET=1) is documented in chrome_fidelity_golden.h:165-203; System-7 MDEF menus use the same. flair_menu.png shows no shadow on the panel edges.
- **expected:** 1px black drop shadow on the right and bottom edges, offset (1,1) outside the frame (System-7 MDEF shadow).
- **actual:** No shadow; only a 1px border on all four sides.
- **repro:** Drop a menu or run test_menu; inspect the right/bottom panel edges for a shadow row/column.

### 17. [P1] App-switch clobbers the static System-7 top menu bar — both chimera bars collapse to identical Photoshop content with no Apple slot
- **bead:** initech-4w15  **subsystem:** FLAIR/process + shell + menu
- **evidence:** flair_appswitch_pre.png: bar 1 (System-7) = Apple-slot square + 'File Edit View Special'; bar 2 = 'File Edit Image Layer Select View Window Help'. flair_appswitch_post.png (after click-to-activate): BOTH bars show 'File Edit Image Layer Select View Window Help' with no Apple slot. kmain.c:1961 `ten_hello->menubar = &ctx.scene->bar_photoshop`; when HELLO is frontmost DrawMenuBar (kmain.c:2291) draws bar_photoshop into the bar-1 position, making bar 1 == bar 2. The FLAIR shell bar (bar_sys, shell.c:267) must stay static; instead the live activation path repaints it with the active app's menu.
- **expected:** The top System-7 shell bar is shell-owned and static across app switches (always Apple slot + File/Edit/View/Special); only the second (Photoshop chimera) bar reflects the active app. The two stacked bars stay visually distinct.
- **actual:** After the switch both bars are identical Photoshop strings and the Apple slot disappears.
- **repro:** Boot HELLO + NOTES; click HELLO to activate; both stacked menu bars become identical and the Apple slot is gone.

### 18. [P1] App-switch erases the background NOTES window entirely from the compositor (chrome + content gone)
- **bead:** initech-jmc5  **subsystem:** FLAIR/desktop + process (compositor / app-switch)
- **evidence:** flair_appswitch_pre.png shows HELLO (white) and NOTES (gray, bounds (260,120)-(560,340) per spec/flair_tenants_demo.h). flair_appswitch_post.png: after click-to-activate on HELLO, the entire NOTES window is absent — its non-HELLO-covered footprint shows bare teal desktop, no title bar, frame, or scrollbars. SelectWindow (window.c:357-371) raises HELLO without generating exposure damage for NOTES's newly-relevant area and no full desktop_paint_all follows the z-order change, so NOTES chrome is never repainted as a background window.
- **expected:** After SelectWindow raises HELLO, NOTES stays in the z-order as a background window and is repainted (inactive chrome) in its non-occluded region.
- **actual:** NOTES vanishes completely; only HELLO plus a stray fragment remain (see orphaned-fragment bug).
- **repro:** Boot the two-tenant scene (NOTES front, HELLO back); click HELLO's content; NOTES is gone from the post composite.

### 19. [P2] App-switch leaves an orphaned frameless gray fragment of NOTES on the bare teal desktop (stale damage)
- **bead:** initech-8148  **subsystem:** FLAIR/compositor (damage tracking)
- **evidence:** flair_appswitch_post.png: a ~50-80 x 37-70px solid gray (~#C0C0C0) rectangle sits on bare teal desktop near the NOTES bottom-left corner (approx native x[254,304] y[297,334], near NOTES.left=260/bottom=340), outside any window chrome. It is absent from flair_appswitch_pre.png. The gray matches NOTES fill (ref_tenant.c:119-120 NOTES fill = CIDX_CONTROL/#C0C0C0). The compositor's damage distribution during the z-order change missed this sub-region, so the seafoam (#8DDCDC) refill never covered it (a content-bleed facet of the non-WindowMgr-layer damage gap).
- **expected:** Every pixel of NOTES's formerly-visible area is repainted with the seafoam desktop background; no stale content remains outside any window.
- **actual:** A gray rectangle persists on the teal desktop at NOTES's old corner.
- **repro:** Boot HELLO + NOTES; click HELLO to switch; observe the gray fragment on the desktop below the windows.
- **related:** initech-pipa

### 20. [P1] Drag of one window erases a peer document window from the composite (3 windows before -> 2 after)
- **bead:** initech-qi8v  **subsystem:** FLAIR/compositor (drag)
- **evidence:** 01_desktop_before_2x.png shows 3 cascaded windows; 02_desktop_after_2x.png (after dragging another window) shows only 2 — the front (upper-left) window is gone, its area correctly filled with seafoam (so desktop_update covered it) but its z-order entry/chrome never re-added. flair_drag.png corroborates (only untitled-1 + 'Saving tables to disk' remain mid-drag). This is a WindowMgr document window vanishing — distinct from the modal/menu-bar erasure of initech-pipa.
- **expected:** All windows present before the drag remain after; only the dragged window moves, others repaint exposed areas and stay in the z-order.
- **actual:** The non-dragged front window is removed from the composite and never re-added.
- **repro:** Open 3 cascaded document windows; drag a non-front window; the original front window disappears.
- **related:** initech-pipa

### 21. [P2] Window drop-shadow L-shape (x=right, y=bottom) is outside strucRgn and untracked by the damage model — stale shadow pixels remain after a drag
- **bead:** initech-o0cs  **subsystem:** FLAIR/compositor (damage)
- **evidence:** chrome.c:669-674 draws the shadow at x=right, y in [top+1,bottom+1) and y=bottom, x in [left+1,right+1) — at the exclusive edges of the half-open strucRgn [left,right)x[top,bottom). MoveWindow (window.c:393) calls visible_into computing strucRgn DIFF fronts_union (window.c:124-138); shadow pixels at x=strucRgn.right and y=strucRgn.bottom are not members of strucRgn, so they are excluded from `exposed` and never distributed to desktop_update (window.c:212-247). The old shadow position retains stale 1px (2px at 2x) black paint.
- **expected:** When a window moves, all pixels it painted — including the 1px L-shaped shadow at x=right and y=bottom — are included in the exposure set and repainted.
- **actual:** Shadow L-shape pixels are excluded; after a drag the old position keeps a stale black 1px fringe.
- **repro:** Drag a window over solid teal; the old x=old_right / y=old_bottom L retains black pixels.
- **related:** initech-pipa

### 22. [P2] Tenant content rect extends into the 16px scrollbar column; updateEvt repaint erases the chrome-drawn scrollbar
- **bead:** initech-javs  **subsystem:** FLAIR/apps (ref_tenant.c)
- **evidence:** ref_tenant.c:291 `content.right = bounds.right - FLAIR_CHROME_FRAME` (1px inset only) — FLAIR_CHROME_SCROLLBAR_W (16) is never subtracted, so content extends to bounds.right-1 over the scrollbar band [bounds.right-17,bounds.right-1). At updateEvt repaint (ref_tenant.c:342-344) paint_content fills the content rect (incl. scrollbar column) clipped to contRgn INTERSECT updateRgn, overwriting the scrollbar. flair_appswitch_pre.png: neither HELLO nor NOTES shows a right scrollbar; NOTES gray fill runs flush to the frame.
- **expected:** Tenant content right = bounds.right - FLAIR_CHROME_FRAME - FLAIR_CHROME_SCROLLBAR_W; the right 16px belong to the scrollbar.
- **actual:** Tenant content covers the scrollbar column; repaints erase the scrollbar (no scrollbar visible on either window).
- **repro:** Run FLAIR_LIVE_TENANTS; trigger any window update; the right scrollbar is gone.
- **related:** initech-pipa

### 23. [P2] Both co-resident tenant windows show blank title bars — window name text not rendered
- **bead:** initech-vfd8  **subsystem:** FLAIR/chrome + process
- **evidence:** flair_appswitch_pre.png: HELLO and NOTES title bars contain only the pinstripe pattern and close/zoom boxes — no title text; windows are distinguishable only by content fill color. chrome.c:385-409 correctly renders centered Chicago text when title is non-empty, but window.c:296 leaves `w->titleHandle[0]='\0'` at NewWindow and shell.c:257-263 copies from win_titles[i]; the FlairAppProcs.open() contract (process.h:153) does not document setting the title, so the tenants pass empty strings.
- **expected:** The HELLO window shows 'HELLO' and the NOTES window shows 'NOTES' centered in their title bars.
- **actual:** Both title bars are blank in pre- and post-switch frames; the only distinguishing visual is content fill color.
- **repro:** Boot the two-tenant scene; both title bars between close and zoom box show only stripes, no glyphs.

### 24. [P2] shell_menu_bg() 32bpp path returns REVOKED preview.webp menubar color 0x67696C instead of canon #FFFFFF
- **bead:** initech-c2az  **subsystem:** FLAIR/shell (shell.c)
- **evidence:** shell.c:91 `#define SHELL_MENUBAR_BG_RGB 0x67696Cu` = rgb(103,105,108), the 'menubar' sample from spec/assets/palette.h's preview_webp 3x3 frame. color_canon.json marks preview_webp_samples as 'PROVENANCE ONLY (REVOKED as render/oracle source, HER-03)'. The 8bpp path (shell.c:106-108, CIDX_MENUBAR idx 3 = white) is correct; the 32bpp path (shell.c:108) returns the revoked dim gray. Canon CIDX_MENUBAR = #FFFFFF.
- **expected:** shell_menu_bg() for >8bpp returns flair_canon_rgb(CIDX_MENUBAR) = 0xFFFFFF.
- **actual:** Any >8bpp render (SSIM grade, 32bpp screendump) paints the menubar dim gray #67696C from a revoked CRT mock-up; 8bpp PPM captures hide it.
- **repro:** grep SHELL_MENUBAR_BG_RGB in shell.c (0x67696Cu); cross-ref color_canon.json preview_webp_samples.menubar status REVOKED (HER-03).

### 25. [P1] FILE COPY progress bar value 0 never set before first DrawDialog — renders empty vs reference ~65% blue fill
- **bead:** initech-a90f  **subsystem:** FLAIR/dialog
- **evidence:** dialog.c:789 `control_init(ctrlStorage, progressBar, bar_rect, 0 /* value=0 */, 0, 100, 1, "")`. No SetControlValue runs between FileCopyDialog (shell.c:287-288) and shell_render (kmain.c:970); the control is still 0 at draw. draw_progress_bar (control.c:587-608) -> ctrl_progress_fill_px returns 0 for v<=0 (control.c:334), so the CTRL_ACCENT fill branch (control.c:605-607) is never entered. flair_desktop.png bar rect (~x308-966,y472-506 at 2x) shows only a 1px frame + white interior, zero fill. preview.png reference shows the bar ~65-70% filled solid blue.
- **expected:** shell_build_scene (or caller) calls SetControlValue to a canon non-zero value (~65/100) before the initial render, matching the Law-4 reference frame.
- **actual:** The progress bar always renders fully empty (white interior, 1px frame, no blue).
- **repro:** Boot the shell with show_modal=1 (default non-tenant path); the FILE COPY progress bar is an empty white rectangle.

### 26. [P2] FileCopyDialog item rects stored in GLOBAL screen coords, not port-local — breaks ModalDialog hit-testing and makes the dialog non-relocatable
- **bead:** initech-ct15  **subsystem:** FLAIR/dialog
- **evidence:** dialog.c:763-766,781-784 build item rects as FILECOPY_LEFT+offset (text_rect.left=154, bar_rect.top=236) — absolute screen coords. ModalDialog (dialog.c:599-602) converts the click to dialog-LOCAL (pt.h -= portRect.left) then calls FindDialogItem (dialog.c:454-455 tests pt.h >= item->rect.left). A click at global x=200 -> local 60 vs item left 154 -> never hits; FindDialogItem always returns 0 for any in-dialog click. IM-I Ch6 specifies item rects in port-local coords. Currently masked (FILE COPY items are enabled=0), but any future enabled pushButton is permanently unclickable; also the dialog cannot be repositioned without items drawing/hit-testing at stale screen coordinates.
- **expected:** Item rects use dialog-local offsets (text_rect.left = FILECOPY_ITEM_LEFT = 14, bar_rect.top = FILECOPY_BAR_TOP = 36), matching the dialog-local point ModalDialog passes to FindDialogItem.
- **actual:** Item rects encode global position; FindDialogItem returns 0 for every realistic in-dialog click, and the layout is non-relocatable.
- **repro:** Add an enabled pushButton with the FILECOPY_LEFT+offset pattern; ModalDialog with a mouseDown at its global position returns itemHit=0 regardless of accuracy.

### 27. [P2] dBoxProc dialog border drawn as uniform solid black across all 7px — missing the System-7 outer-frame/inner-band structure
- **bead:** initech-9ek6  **subsystem:** FLAIR/dialog
- **evidence:** dialog.c:367-370 fills all four border bands with DLG_BLACK (FLAIR_PART_FRAME, pure black) uniformly across the full 7px width. Real System-7 dBoxProc in s7_dialog_alert.png shows tonal variation within the band (1px black outer frame, lighter dark interior fill, inner boundary before white content). flair_desktop.png shows a featureless solid black slab.
- **expected:** 7px band (FLAIR_CHROME_DIALOG_BORDER=7, dBoxBorderSize EQU 7) with outer-frame/inner-shadow tonal variation per s7_dialog_alert.png.
- **actual:** All 7px are the same CIDX_BLACK — a flat solid black rectangle frame with no structure.
- **repro:** Render the shell with show_modal=1; pixel-inspect the dialog border bands — all CIDX_BLACK.

### 28. [P1] FILE COPY modal is a titleless dBoxProc (thick 7px border) instead of the reference's moveable titled 'FILE COPY' modal
- **bead:** initech-zvo6  **subsystem:** FLAIR/dialog
- **evidence:** NewDialog (dialog.c:248-249) creates the dialog as dBoxProc, which (dialog.c:28-29) renders a solid 7px filled border and no title bar. flair_desktop.png shows the modal as a white rect inside a thick solid black band, no title bar, no close gadget. The Office Space reference preview.png shows FILE COPY as a moveable modal with a 'FILE COPY' title bar, thin 1px document-style frame, and standard gadgets. Law 4 fidelity miss.
- **expected:** A moveable titled modal (movableDBoxProc / documentProc variant) with a 'FILE COPY' title bar and thin 1px chrome, matching preview.png.
- **actual:** A dBoxProc box: thick 7px solid black border on all sides, no title bar, no close box — unlike the reference.
- **repro:** Run any scene with show_modal=1 (or inspect flair_desktop.png): the centered modal is a thick black titleless box.

### 29. [P2] te_recompute_lines word-wrap: a space at the wrap boundary becomes the leading char of the next line instead of being consumed as a separator
- **bead:** initech-d3vn  **subsystem:** TextEdit (textedit.c)
- **evidence:** textedit.c:137-150. The wrap check (line 137 `(i-s) >= wrapWidth`) fires before the space check (line 149 `if (ch==TE_CH_SP) last_space=i`). When text[i] is a space, (i-s)==wrapWidth, and last_space<s, the hard-break arm (line 142) sets brk=i; te_emit_line records i as the new line start; `continue` (line 146) re-examines text[i] and records the space as last_space for the NEW line. The function comment (lines 94-96) says 'the space stays on the line' but it ends up at the START of the next line. test_textedit.c step 9b ('the cat sat', wrapWidth=4) only exercises the soft-break path.
- **expected:** A boundary space is consumed as a separator: brk=i+1, new line starts at i+1 (trailing space on the current line).
- **actual:** For 'abc d' wrapWidth=3: lineStarts={0,3}, line1=' d' (leading space). Renderers using lineStarts render a spurious leading space on each such wrapped line.
- **repro:** FlairTE_init(te,3,0); FlairTESetText(te,'abc d',5); te.lineStarts[1]==3 (space first char) vs expected 4 ('d'). Not caught by existing oracle.

### 30. [P2] FlairLAddRow: int16_t signed overflow in new_rows lets a large count bypass the FLAIR_LIST_MAX_ROWS guard
- **bead:** initech-8mip  **subsystem:** List Manager (list.c)
- **evidence:** list.c:126 `new_rows = (int16_t)(cur_rows + count);` — sum truncated to int16_t. cur_rows=1, count=32767 -> 32768 wraps to -32768; list.c:128 `if (new_rows > FLAIR_LIST_MAX_ROWS)` tests -32768>32 = false, guard bypassed; list.c:136 sets dataBounds.bottom=-32768 (copied to visible.bottom at :148) and returns FLAIR_LIST_OK.
- **expected:** Return FLAIR_LIST_ERR_OVERFLOW whenever cur_rows+count would exceed FLAIR_LIST_MAX_ROWS, including near INT16_MAX.
- **actual:** Overflow wraps new_rows negative, the guard passes, dataBounds.bottom is a large negative value, and the function returns OK.
- **repro:** FlairLAddRow(&l,1,-1); then FlairLAddRow(&l,32767,-1) returns FLAIR_LIST_OK with negative dataBounds.bottom.

### 31. [P2] Oracle gap: FlairLAddRow overflow path never exercised — the MAX_ROWS guard is mutation-unproven (Rule 6)
- **bead:** initech-nbnx  **subsystem:** List Manager oracle (test_list.c)
- **evidence:** test_list.c:107 is the only FlairLAddRow call (count=3); steps 2-8 never call it again, so the guard at list.c:128 is never reached. The named mutants (CELL_INDEX_SWAP, HIT_OFFBYONE, NO_DESELECT) include none that removes the overflow check. Deleting list.c:128 leaves all tests green.
- **expected:** An oracle step calling FlairLAddRow with count>FLAIR_LIST_MAX_ROWS asserting FLAIR_LIST_ERR_OVERFLOW, plus a LIST_MUT_NO_OVERFLOW_CHECK mutant that goes RED.
- **actual:** No overflow step exists; the guard has never proven it can catch a regression.
- **repro:** Remove the overflow check at list.c:128 and run make test-list — all green.

### 32. [P2] FlairLAddRow auto-column inner guard is dead code — inner if is trivially true whenever the outer if holds
- **bead:** initech-42dq  **subsystem:** List Manager (list.c)
- **evidence:** list.c:141-144: outer `if (dataBounds.right == dataBounds.left)` guarantees right-left==0; inner `if (dataBounds.right - dataBounds.left < FLAIR_LIST_MAX_COLS)` evaluates 0<8 = always true, so the inner guard can never suppress auto-column creation. If left ever equalled right==FLAIR_LIST_MAX_COLS the outer fires and right is set to MAX_COLS+1, risking later OOB on selected[ri][FLAIR_LIST_MAX_COLS].
- **expected:** Inner condition should check actual headroom (e.g. dataBounds.right - dataBounds.left + 1 <= FLAIR_LIST_MAX_COLS), or be removed if always safe.
- **actual:** Inner guard is dead code providing no protection.
- **repro:** Code inspection list.c:141-144: inner reduces to 0<8 under the outer condition; no input makes it false.

### 33. [P2] FlairLClick: NULL hit_out returns 0 (miss), indistinguishable from a legitimate miss and silently suppressing selection
- **bead:** initech-6vvw  **subsystem:** List Manager (list.c)
- **evidence:** list.c:222 `if (!l || !hit_out) return 0;` returns 0 for both NULL l and NULL hit_out. list.h:222-223 documents 0 = 'point outside rView/dataBounds (no selection change)'. A caller doing `if (FlairLClick(l,pt,NULL))` gets 0 even on a true hit and no selection update; there is no error path to distinguish the programming error from a miss (Rule 2 fail-loud).
- **expected:** Panic (or a dedicated FLAIR_LIST_ERR_NULL sentinel) on NULL hit_out, or update selection and return 1 regardless of hit_out.
- **actual:** NULL hit_out silently short-circuits returning 0 with no selection change, identical to a real miss.
- **repro:** FlairLClick(&l, in_bounds_point, NULL) returns 0 and changes nothing, masking the bug.

### 34. [P2] FlairLGetSelect returns FLAIR_LIST_ERR_NULL (-5) for a NULL list but the header contract omits this value
- **bead:** initech-14oe  **subsystem:** List Manager (list.h/list.c)
- **evidence:** list.h:230-234 documents only {1,0,FLAIR_LIST_ERR_BOUNDS} — no FLAIR_LIST_ERR_NULL. list.c:299 `if (!l) return FLAIR_LIST_ERR_NULL;` returns -5. A caller using `if (FlairLGetSelect(l,c) > 0)` silently treats -5 as 'not selected'. (FlairLSetSelect documents the NULL case at list.h:241; this one does not.)
- **expected:** Header documents 'Returns FLAIR_LIST_ERR_NULL if l is NULL', matching FlairLSetSelect.
- **actual:** Undocumented -5 return creates a silent error-masking hazard for callers checking only for 1.
- **repro:** FlairLGetSelect(NULL,c) returns -5, absent from list.h:230-234.

### 35. [P2] FlairPutScrap: NULL data with len>0 reads from address 0 (the IVT) instead of failing loud
- **bead:** initech-m1ry  **subsystem:** Scrap Manager (scrap.c)
- **evidence:** scrap.c:141 panics on s==NULL but there is no NULL guard for data. scrap.c:152 and :164 call scrap_memcpy(dst, data, len) unconditionally; scrap_memcpy (scrap.c:53-60) dereferences src in a byte loop. With data=NULL,len>0 it reads address 0; in the freestanding flat-32 kernel address 0 is valid (real-mode IVT), so no fault — the scrap is silently filled with IVT bytes.
- **expected:** SCRAP_PANIC when data==NULL && len>0 (Rule 2), matching the existing s==NULL guard.
- **actual:** Silently copies IVT bytes from address 0 into the flavor payload with no panic.
- **repro:** FlairPutScrap(&s, FLAIR_SCRAP_TYPE_TEXT, NULL, 5) on an init'd scrap; no panic, GetScrap returns 5 bytes from address 0.

### 36. [P2] FlairPutScrap NO_FLAVOR full-table branch is mutation-unproven — removal enables OOB write to flavors[4]
- **bead:** initech-iuwq  **subsystem:** Scrap Manager (scrap.c)
- **evidence:** scrap.c:157-158 `if (s->n_flavors >= FLAIR_SCRAP_MAX_FLAVORS) return FLAIR_SCRAP_ERR_NO_FLAVOR;`. Declared mutants (IGNORE_FLAVOR, NO_INCREMENT, NO_CLEAR; scrap.c:13-19) include no SCRAP_MUT_FULL_FLAVOR. The oracle (test_scrap.c steps 1-7) writes only TEXT + PICT (2 of 4 slots), never fills to 4 + a 5th add. Mutating >= to > or deleting the check leaves the oracle green; without the guard a 5th add writes slot=4 to flavors[4], OOB of flavors[FLAIR_SCRAP_MAX_FLAVORS].
- **expected:** A SCRAP_MUT_FULL_FLAVOR mutant and a test step filling all 4 slots then asserting FLAIR_SCRAP_ERR_NO_FLAVOR on the 5th.
- **actual:** Live code with no mutation proof; a mutation passes all current steps without going RED.
- **repro:** Delete scrap.c:157-158 and run make test-scrap — not RED.

### 37. [P2] FlairResMap_load: additive bounds check at line 150 can overflow uint32_t, bypassing the map-preamble size guard
- **bead:** initech-6mfi  **subsystem:** Resource Manager (resource.c)
- **evidence:** resource.c:150 `if (m->map_off + 28u > len) goto bad;` — additive, unlike the safe subtraction at lines 145-146. With map_off near UINT32_MAX-28 (e.g. 0xFFFFFFE0), map_off+28u wraps small and the check falsely passes; the code then reads copy+map_off+24 wrapping into unmapped memory. (Exploiting needs len near UINT32_MAX too; the 4MB flat-kernel arena bounds it in practice, but the arithmetic is formally wrong and inconsistent.)
- **expected:** Safe subtraction `if (28u > len - m->map_off) goto bad;` (valid since map_off<=len from line 144).
- **actual:** Addition allows a uint32 overflow that bypasses the guard for map_off near UINT32_MAX.
- **repro:** src_fork with mapOffset=0xFFFFFFE0, len=0xFFFFFFFF; load succeeds and subsequent reads access wrapped addresses.

### 38. [P2] FlairResMap_load: n_types wraps to 0 when numTypes-1=0xFFFF — load reports success but all resources are silently hidden
- **bead:** initech-s3bh  **subsystem:** Resource Manager (resource.c)
- **evidence:** resource.c:160 `m->n_types = (uint16_t)(rd_u16(copy + m->type_list_abs) + 1u);` — rd_u16 max 0xFFFF, +1 = 0x10000, truncated to uint16_t = 0. find_type then iterates 0..-1 (zero iterations) and returns NULL; every FlairGetResource returns NULL and FlairCountResources returns 0, while FlairResMap_load returns 0 (success). No fail-loud (Rule 2).
- **expected:** Detect the wrap and goto bad: compute uint32 raw, reject if raw>0xFFFF or raw==0 before assignment.
- **actual:** n_types=0, load returns success, all lookups silently return NULL/0.
- **repro:** Fork with typeList header word=0xFFFF; FlairResMap_load returns 0 while FlairCountResources returns 0 for every type.

### 39. [P2] FlairGetResource: data extent validated against fork length (m->len) not the data section, allowing map bytes to be returned as resource data
- **bead:** initech-u37e  **subsystem:** Resource Manager (resource.c)
- **evidence:** resource.c:195 `if (dlen > m->len - (abs + 4u)) return NULL;` bounds against the whole fork, not data_off+data_len. resource.h Sec 1.2 defines the data area as [data_off, data_off+data_len). A crafted fork (data_off=16,data_len=40,map_off=56,len=100; resource abs=50,dlen=20) runs past data_off+data_len=56 into the map but satisfies 20<=100-54=46, so the check passes and a pointer into the map section is returned.
- **expected:** Validate wholly within the data section: `if (do24 >= m->data_len || dlen > m->data_len - do24 - 4u) return NULL;`.
- **actual:** Validation uses m->len, accepting resources overlapping the map and returning map metadata as payload.
- **repro:** Fork where abs+4+dlen exceeds data_off+data_len but < len; FlairGetResource returns a non-NULL pointer into the map.

### 40. [P2] FlairGetResource/FlairGetResInfo: abs = data_off + do24 has no overflow guard; a wrapped abs passes the > m->len check
- **bead:** initech-nc0z  **subsystem:** Resource Manager (resource.c)
- **evidence:** resource.c:192 `uint32_t abs = m->data_off + do24;` then :193 `if (abs > m->len || m->len - abs < 4u) return NULL;` (same at FlairGetResInfo :239-240). do24 (rd_u24, max 0xFFFFFF) + data_off (uint32) can exceed 2^32; e.g. data_off=0xFF000002, do24=0xFFFFFF -> abs=0xFF000001 wraps small, abs>m->len is false, and rd_u32(m->fork+1) reads a length prefix from near the header.
- **expected:** Check do24 against data section bounds before adding: `if (m->data_len < 4u || do24 > m->data_len - 4u) return NULL;`.
- **actual:** Unchecked uint32 addition; a wrapped abs passes the guard and reads from an aliased location near fork byte 0.
- **repro:** Fork with data_off near UINT32_MAX and resDataOffset24 summing over 2^32; FlairGetResource returns non-NULL from near byte 0 rather than NULL.

### 41. [P2] flair_route_updates: WindowMgr_validate clears damage unconditionally even when the updateEvt is not delivered
- **bead:** initech-0zxp  **subsystem:** process.c (App Contract dispatcher)
- **evidence:** process.c:646-658: the updateEvt is delivered only inside `if (owner->procs && owner->procs->event != ...)`, but WindowMgr_validate(w) at line 658 is OUTSIDE that guard and runs unconditionally. A tenant with NULL procs/event has its updateRgn cleared without the event ever firing, so content is never refreshed and no future repaint is triggered — permanently stale.
- **expected:** WindowMgr_validate(w) only when the updateEvt was actually delivered (inside the guard).
- **actual:** Damage is cleared regardless of delivery; a NULL-event-proc tenant is left permanently stale.
- **repro:** Register an app with procs->event==NULL; expose it to damage; flair_route_updates clears updateRgn without delivering the event.

### 42. [P2] FlairSF_doOpen: FlairSF_navigate return value discarded via (void) — reply claims navigation occurred on provider/path/list failure
- **bead:** initech-q9cr  **subsystem:** stdfile.c (Standard File)
- **evidence:** stdfile.c:433 `(void)FlairSF_navigate(sf, sel);`. On FLAIR_SF_ERR_PROVIDER/PATH/LIST the error is discarded and the function returns reply->is_dir=1, good=0, fName=folder_name (navigated into folder X) while FlairSF_navigate only commits path on success (stdfile.c:372-374) — so list/path are unchanged. The pump sees 'navigated into DATA' but state still shows the parent.
- **expected:** Check FlairSF_navigate's return; on failure propagate the error or revert reply to no-selection state.
- **actual:** Provider/path/list failure is silently swallowed; reply is inconsistent with displayed list.
- **repro:** Provider that fails on subdirectories; FlairSF_select + FlairSF_doOpen on a folder -> reply->is_dir==1/fName set while FlairSF_count still returns the parent's count.

### 43. [P2] FlairProcess_terminate/kill have no runtime guard against _register'd apps; teardown frees are silently rejected (contract violation undetected)
- **bead:** initech-k0n8  **subsystem:** process.c (App Contract lifecycle)
- **evidence:** process.h:226-227 documents that _register'd apps have app->block==NULL and must NOT be torn down with FlairProcess_terminate, but there is no runtime check in terminate (process.c:368) or kill (process.c:397). Both call teardown_common -> flair_free(.., app->block) (process.c:335, NULL), flair_free(.., app->records_block) (process.c:345, NULL), flair_free(.., app) (process.c:355, caller-owned non-heap). flair_free short-circuits NULL (heap.c:152) and rejects out-of-window payloads (heap.c:155) silently. The magic check (process.c:373/403) cannot distinguish _register from _launch (both FLAIR_APP_MAGIC).
- **expected:** terminate/kill check app->block==NULL (or an is_registered flag) and panic (Rule 2) or skip the heap frees.
- **actual:** Terminate/kill on a _register'd app is accepted silently; all three flair_free calls are rejected with no error and no memory freed.
- **repro:** Register an app with caller-owned storage; FlairProcess_terminate it; no panic/error, master heap n_free does not increment.

### 44. [P2] teardown_common does not zero app->magic before free — double-teardown is undetected and double-pushes blocks onto free-lists (potential cycle/hang)
- **bead:** initech-vibs  **subsystem:** process.c (App Contract lifecycle)
- **evidence:** process.c:354-355 frees block, records_block, then app via flair_free(.., FLAIR_CLASS_HANDLE, app). The free-list next pointer is written at header+8 (heap.h:126-130, heap.c:56-58), which does NOT overlap app->magic at payload offset 0, so app->magic==FLAIR_APP_MAGIC survives the free. A second terminate/kill on the same pointer passes the magic check (process.c:373/403) and double-frees all three blocks. heap.h:195 states the allocator does not detect double-free; the resulting free-list cycle makes flair_alloc's reuse scan (heap.c:96-111) loop forever.
- **expected:** Zero app->magic before the final flair_free(app) so a second teardown panics at the magic check (Rule 2).
- **actual:** app->magic survives; a second teardown is undetected, double-frees, and can cycle the free-list, hanging the next allocation.
- **repro:** Launch an app, terminate it, terminate the same pointer again; all three frees succeed again; a later flair_alloc(HANDLE,...) hangs if the list now cycles.

### 45. [P2] PS/2 byte0 sign bits XS (bit4) and YS (bit5) never applied — dx/dy get the wrong sign/magnitude for |delta| > 127
- **bead:** initech-qupy  **subsystem:** mouse/input (mouse.c)
- **evidence:** mouse.c:220-222 reads b0=g_pkt[0] but only extracts b0 & PKT_BTN_MASK (bits 0-2); bits 4 (XS) and 5 (YS), the 9-bit two's-complement sign bits, are never consulted. dx/dy are cast straight: `int8_t dx=(int8_t)g_pkt[1]; int8_t dy=(int8_t)g_pkt[2];`. No XS(0x10)/YS(0x20) define exists. event.c:45-47 assumes the payload is already sign-extended, but the producer never does it. The overflow check (PKT_OVERFLOW=0xC0) only discards |delta|>255, so [128,255] passes with the wrong sign. (Distinct from initech-rgt8 vertical-axis inversion.)
- **expected:** `int16_t dx = (int16_t)g_pkt[1] - ((b0 & 0x10u) ? 256 : 0);` (and YS for dy).
- **actual:** byte1=200 -> dx=-56 (cursor jumps left on fast rightward move); XS=1,byte1=100 -> dx=+100 (jumps right on leftward move).
- **repro:** Move the PS/2 mouse fast right so byte1>=128 (XS=0); the cursor moves left / wrong magnitude.

### 46. [P2] After drag/menu dispatch the cursor sprite is re-shown at the mouseDown position, not the current post-dispatch position
- **bead:** initech-9oci  **subsystem:** mouse/input (kmain.c)
- **evidence:** kmain.c:2399-2402 `cur_show_h=ev.where.h; cur_show_v=ev.where.v; cursor_show(...)` uses the outer WNE event (the mouseDown). After flair_live_do_drag's sub-loop, g_cursor_h/v (event.c) has advanced to mouseUp, but ev.where still holds the original click. Same pattern in the FLAIR_LIVE_TENANTS arm (kmain.c:2311-2313). flair_live_do_drag (kmain.c:1128-1183) tracks where1 to mouseUp for DragWindow but never updates the outer ev.
- **expected:** After a drag/menu the cursor sprite appears at the final (mouseUp) position.
- **actual:** The cursor sprite blinks at the drag-start position for one WNE iteration (~up to 30ms) then teleports to the actual position.
- **repro:** FLAIR_LIVE_INTERACTIVE: drag a window; at mouse-up the cursor briefly appears at the drag-start point.
- **related:** initech-34gp

### 47. [P2] Cursor sprite absent during the entire menu-tracking gesture — cursor invisible while navigating drop-down items
- **bead:** initech-vp9f  **subsystem:** mouse/input (kmain.c)
- **evidence:** flair_live_do_menu (kmain.c:1251-1355): the tracking loop (1296-1326) calls WaitNextEvent + flair_draw_menu_panel + flair_desktop_present per highlight change but contains no cursor_show anywhere. The cursor is hidden by cursor_hide (kmain.c:2375-2377) before dispatch and only re-shown at the pump bottom (2400-2402) after the menu function returns, so the panel is presented to the LFB repeatedly with no cursor. flair_menu.png shows the dropped File menu (About highlighted) with no cursor visible.
- **expected:** The arrow cursor stays composited on top of the dropped menu, re-shown after each panel redraw, with the highlight following it.
- **actual:** The cursor is invisible for the whole menu gesture (button-down on title until selection presented).
- **repro:** FLAIR_LIVE_INTERACTIVE: click 'File' and move over About/Quit — no cursor sprite during tracking.

### 48. [P2] Arrow cursor has no white dilation/mask — pure black silhouette is invisible against dark backgrounds (modal border, frames, title text)
- **bead:** initech-hq7l  **subsystem:** mouse/input (cursors.h, kmain.c)
- **evidence:** spec/assets/cursors.h comment: 'mask == data (no dilation needed because the cursor is a solid silhouette)'. kmain.c:1481-1484: `if (mrow & bit) *p = (drow & bit) ? CURSOR_CIDX_INK : CURSOR_CIDX_PAPER;` — since mask==data, every opaque bit also has data set, so CURSOR_CIDX_PAPER (white) is never written; all opaque pixels are CURSOR_CIDX_INK (CIDX_BLACK). The thick modal border (flair_desktop.png x~280-290,y~395-555) is also CIDX_BLACK, so the cursor is pixel-identical to the background there.
- **expected:** A 1px white dilation: pixels where (mask set, data clear) painted white (CURSOR_CIDX_PAPER), giving the black arrow a white outline visible on any dark background.
- **actual:** Pure black silhouette with no outline; the cursor disappears over the modal border, window frames, and dark title text. The PAPER branch is dead code for the locked arrow asset.
- **repro:** FLAIR_LIVE_INTERACTIVE: move the cursor over the FILE COPY modal border or a window frame — it vanishes.

### 49. [P2] Drag/menu guards and WNE deadline use additive tick (g_tick + N) — instant fire when the tick counter is near UINT32_MAX
- **bead:** initech-gwts  **subsystem:** mouse/input (kmain.c, event.c)
- **evidence:** kmain.c:1129 `uint32_t guard = flair_tick_count()+150u;` (drag) tested at :1144; kmain.c:1295 same for menu, tested at :1323; event.c:546 `deadline = g_tick + sleepTicks;` tested at :563. When g_tick is near UINT32_MAX (e.g. 0xFFFFFF80) the sum wraps small and `>=` is immediately true, committing the drag/menu instantly and returning WNE timeout on the first call. The main pump correctly uses unsigned subtraction `(flair_tick_count()-start) < FLAIR_LIVE_TICK_BUDGET` (kmain.c:2345).
- **expected:** Use the unsigned-subtraction idiom `(flair_tick_count() - start) < 150u` everywhere so the comparison wraps correctly.
- **actual:** Near tick wrap (~497 days uptime at 100Hz) the guard fires on the first poll: the drag commits with zero displacement and WNE busy-spins.
- **repro:** Set g_tick=0xFFFFFF80 before arming the pump; a drag does not move the window (guard fires before any mouse events).

