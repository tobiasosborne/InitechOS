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


---

# Round 2 -- un-inspected managers + DOS shell/loader + live pump

82 raw -> 53 verified REAL -> 45 deduped (1 P0, 2 P1, 42 P2). Label `gui-bughunt`.

## Summary

Round 2 surfaced 45 distinct, evidence-grounded defects across previously un-inspected surface (53 raw reports deduped: 4 verifier-listed twins plus 4 near-identical merges collapsed). The most damaged un-inspected areas are COMMAND.COM (a P0 silent data-loss in COPY-onto-itself that truncates a file to zero and reports success, plus a P1 dos_read missing-Carry-Flag bug that injects garbage into TYPE/COPY/batch), the FLAIR heap allocator (multiple unguarded integer-overflow paths that return non-NULL aliasing/cap-0 blocks behind a false "overflow-safe" comment, plus a free-list-walk that violates the LIFO contract), the Control Manager (no disabled/grayed state anywhere in TestControl or any DrawControl path, teal CTRL_DESKTOP bleeding into dialog backgrounds, and silent NULL guards mislabeled "fail-loud"), and FindWindow hit-test geometry (grow/close/zoom zones all misaligned from rendered gadgets and inMenuBar never returned). Cross-cutting these is a pervasive oracle/mutation-coverage debt — blitter non-8bpp paths, window grow/zoom, heap overflow, text wide-advance, menu disabled rendering, and the MZ entry boundary are all mutation-unproven, so several of the logic bugs above sit in code the existing test suite cannot fail on.

## Bugs

### R2.1. [P2] Oracle gap: test_blitter.c covers no 24bpp path and no 32bpp copy-blit — surface_put_pixel 24bpp else-branch and 32bpp dword-copy are mutation-unproven
- **bead:** initech-asci  **subsystem:** FLAIR/blitter oracle (harness/proptest/test_blitter.c)
- **evidence:** test_blitter.c:38 claims it covers 'both surface_put_pixel branches', but surface.h:82-90 has THREE bpp branches (8bpp:82, 32bpp:84, 24bpp else:86 writing B,G,R at off+0/+1/+2). Only mk_dst8 (:164) and mk_dst32 (:196) exist; no mk_dst24. Fill covers 8bpp(:344)+32bpp(:369) but copy-blit is 8bpp-only (:380 'COPY-BLIT (8bpp)', :401 mk_dst8). The 24bpp else-branch (fill+copy) and the 32bpp copy path (copy_clipped_run->surface_put_pixel dword write, blitter.c:173) are never exercised.
- **expected:** Oracle covers all three bpp for both fill and copy; a mutation in the 24bpp byte-order write or the 32bpp copy dword write goes RED (Rule 6).
- **actual:** A byte-order swap in the 24bpp else-branch, or a no-op/wrong-stride in the 32bpp copy path, survives all blitter oracle cases.
- **repro:** Swap off+0/off+2 in surface_put_pixel's 24bpp branch (or zero the 32bpp copy write); make test-blitter stays GREEN.

### R2.2. [P2] fb_clear 24bpp arm computes pixel count as (lfb_pitch*lfb_height)/3 with stride-3 writes, ignoring per-scanline pitch — scrambled BGR on padded LFBs
- **bead:** initech-gv2x  **subsystem:** FLAIR/present (os/milton/kmain.c fb_clear)
- **evidence:** kmain.c:166 total=lfb_pitch*lfb_height; :190 pixels=total/3u; writes fb[i*3+k]=b/g/r treat the whole buffer as a flat BGR-triple array with no row stride. For a padded 24bpp LFB (pitch>width*3), row 1 starts at byte 'pitch' in the LFB but width*3 in the flat index, so every row after row 0 gets rotated BGR components (seafoam r=0x6F,g=0xA0,b=0x8E -> B=0xA0,G=0x6F on row 1). The 32bpp arm (:183-186, dwords=total>>2) is safe; flair_desktop_present (:788) correctly uses y*pitch + x*bpp.
- **expected:** Per-row loop honoring lfb_pitch (fb[y*pitch + x*3..+2]), like flair_desktop_present.
- **actual:** BGR triples placed at flat i*3; rows 1..N show rotated colors for any pitch>width*3.
- **repro:** 640x480x24 VBE mode with 4-byte row padding (pitch=1924): row 0 seafoam correct, rows 1..479 wrong. Latent under QEMU/Bochs (tight pitch).

### R2.3. [P2] surface_get_pixel returns raw 8bpp palette index, not the documented 0x00RRGGBB
- **bead:** initech-5le3  **subsystem:** FLAIR/surface (os/flair/surface.h)
- **evidence:** surface.h:95 documents the return as 0x00RRGGBB; surface.h:100-108 for bpp==8 returns (uint32_t)bm->base[off] (palette index 0..255), while 24bpp/32bpp return packed RGB. render.c:243-252 already special-cases 8bpp via render_palette_rgb(), confirming the raw return is NOT RGB. Any caller comparing surface_get_pixel to flair_palette_rgb(idx) gets 0x00000002 vs 0x008DDCDC.
- **expected:** Consistent 0x00RRGGBB for all bpp (resolve 8bpp via flair_palette_rgb), OR document the 8bpp exception in the header.
- **actual:** 8bpp silently returns a palette index, breaking the contract for bpp-agnostic callers.
- **repro:** surface_get_pixel on a bpp=8 bitmap with index 2 -> 0x00000002; contract says 0x008DDCDC.

### R2.4. [P2] make_offset_view: unsigned underflow of view->height (dst->height - y_top) with no bounds check — wraps to ~0xFFFFFFFF, base past buffer, defeats surface clip -> OOB framebuffer write
- **bead:** initech-mqsn  **subsystem:** FLAIR/shell (os/flair/shell.c)
- **evidence:** shell.c:148-153 view->base=dst->base+y_top*dst->pitch; view->height=dst->height-y_top (both uint32_t). shell_render guards only dst->height==0 (:303-307), not dst->height<=SHELL_MENUBAR2_TOP(=20). For 0<height<=20 the subtraction wraps to ~UINT32_MAX and base advances past the allocation; surface_fill_span's clip y>=bm->height (surface.c:35) never fires, so DrawMenuBar writes rows 0..19 through a base already past the framebuffer end. Safe today only because the sole runtime caller passes height=480.
- **expected:** SHELL_PANIC (Rule 2) when dst->height < SHELL_MENUBARS_H (40) / y_top >= dst->height, before the subtraction.
- **actual:** Silent uint32_t underflow; OOB write reachable with any small surface.
- **repro:** shell_render with a height=10 bitmap -> make_offset_view(y_top=20) yields height=0xFFFFFFF6, base 20 rows past a 10-row buffer; DrawMenuBar writes rows 20..39 past the end.

### R2.5. [P2] FindWindow grow-box hit zone is 1x1 px — gb derived from the content/struct frame gap (1px) not FLAIR_CHROME_GROW (16px)
- **bead:** initech-cjfr  **subsystem:** FLAIR/window (os/flair/window.c FindWindow)
- **evidence:** window.c:504 gb=s.bottom-c.bottom. With shell content rects c.bottom=b.bottom-FLAIR_CHROME_FRAME(1), so gb=1 not FLAIR_CHROME_GROW=16 (chrome_metrics.h:114). The clamped zone (:506-508) becomes the single corner pixel [s.right-1,s.right)x[s.bottom-1,s.bottom).
- **expected:** Hit zone [s.right-FLAIR_CHROME_GROW, s.right)x[s.bottom-FLAIR_CHROME_GROW, s.bottom); gb from FLAIR_CHROME_GROW.
- **actual:** Grow box functionally non-clickable except 1 corner pixel; other clicks fall through to inDrag/inContent.
- **repro:** documentProc window; FindWindow at {v=s.bottom-8,h=s.right-8} -> inDrag, expected inGrow.
- **related:** round-1 'documentProc window missing DrawGrowIcon' is the drawing bug; this is the orthogonal hit-test bug

### R2.6. [P2] reaffirm_active() flips hilited but never invalidates the deactivated window — no chrome repaint on deactivation
- **bead:** initech-v6t2  **subsystem:** FLAIR/window (os/flair/window.c reaffirm_active)
- **evidence:** window.c:175-182 iterates and flips p->hilited but calls no WindowMgr_invalidate / seeds no updateRgn for the window whose hilited goes 1->0. Called from NewWindow(:301), HideWindow(:321), DisposeWindow(:342), ShowWindow(:354), SelectWindow(:370) — every front-window change.
- **expected:** The deactivated window's visible region added to its updateRgn so chrome repaints in inactive state at the next damage cycle.
- **actual:** Only the in-memory flag changes; the ex-front window keeps active-style chrome until an unrelated expose covers/re-exposes it.
- **repro:** Two visible windows; SelectWindow the back one; the previously-front window keeps active racing-stripe pixels indefinitely.
- **related:** round-1 'No inactive-window chrome (chrome always active)' is the drawing half; this is the damage-model half

### R2.7. [P2] FindWindow never returns inMenuBar — both chimera menu bars fall through to inDesk; Photoshop-bar clicks (rows [20,40)) silently swallowed
- **bead:** initech-n6vr  **subsystem:** FLAIR/window (os/flair/window.c FindWindow)
- **evidence:** window.c:466-516 has no menu-bar guard; any point outside a strucRgn exits at :515 return inDesk. kmain.c:2422-2426 acknowledges this and works around v<FLAIR_MENUBAR_H (rows [0,20)), but the second bar at [20,40) (SHELL_MENUBAR2_TOP=40) has no handler.
- **expected:** FindWindow returns inMenuBar (part 1) for v in [0, SHELL_MENUBAR2_TOP), covering both bars (IM-I).
- **actual:** Returns inDesk; the kmain workaround partially covers [0,20) but [20,40) clicks are unhandled.
- **repro:** FindWindow({v=10,h=320}) -> inDesk; {v=30,h=320} -> inDesk, no handler.

### R2.8. [P2] FindWindow close/zoom-box hit zones are tb-sized (full title-band, ~20px) squares pinned to the structure edges — misaligned ~9px from the rendered 11px boxes; outer-frame/gap clicks wrongly fire inGoAway/inZoomIn
- **bead:** initech-ci4o  **subsystem:** FLAIR/window+chrome (os/flair/window.c FindWindow:489-499 vs os/flair/chrome.c:362-365)
- **evidence:** window.c:485 tb=c.top-s.top (19 or 20); :490 close test h in [s.left, s.left+tb); :497 zoom test h in [s.right-tb, s.right). chrome.c:362-364 renders close at [left+9,left+20) and zoom at [right-20,right-9), each 11px (FLAIR_CHROME_WBOX_RENDER=11; spec/chrome_metrics.h:90 WBOX_DELTA=13). So 9 spurious columns left of the close box (and right of the zoom box) fire the gadget code, the box's far column is missed, and vertically the zone covers ~19-20 rows vs the rendered 11.
- **expected:** Close zone [left+9,left+20)x[top+4,top+15); zoom mirror at the right; gap/frame pixels return inDrag.
- **actual:** Clicking the plain title bar near the left edge triggers go-away; near the right edge triggers zoom.
- **repro:** FindWindow at {v=s.top+10,h=s.left} -> inGoAway (expected inDrag); {v=s.top+10,h=s.right-5} -> inZoomIn though rendered zoom box ends at right-9.

### R2.9. [P2] Oracle gap: test_window.c FindWindow suite tests no inGrow or inZoomIn point — grow/zoom hit-test branches mutation-unproven (Rule 6)
- **bead:** initech-ekde  **subsystem:** FLAIR/window oracle (harness/proptest/test_window.c)
- **evidence:** test_window.c:399-436 covers inContent(:410), inDrag(:414), inGoAway(:418), inDesk(:422), ordering(:431,:434) — no inGrow/inZoomIn. With its s/c rects gb clamps to 1, so even an inGrow point is one pixel. Removing 'return inGrow;' at window.c:508 fails zero tests.
- **expected:** Directed inGrow (documentProc, 16x16 corner) and inZoomIn (zoomDocProc, right title end) cases; mutating the grow branch goes RED.
- **actual:** Grow/zoom paths fully uncovered; the 1x1 grow-box bug is invisible to the oracle.
- **repro:** Comment out window.c:508 'return inGrow;'; make test-window stays GREEN.
- **related:** round-1 'FlairLAddRow overflow path never exercised' — same Rule-6 category, different code path

### R2.10. [P2] flair_alloc has no upper-bound size guard: huge sizes overflow flair_align_up/span arithmetic -> cap=0 or zero-advance block returned non-NULL (heap alias/corruption); the 'overflow-safe' comment is false
- **bead:** initech-r081  **subsystem:** FLAIR/heap (os/flair/heap.c)
- **evidence:** Only guard is heap.c:84-86 (size==0 || !flair_class_ok) — no size<=UINT32_MAX-HDR-(ALIGN-1) check, yet :30-31 comments 'overflow-safe ... passed the request-sanity check below' (no such check). (a) flair_align_up (:32-35) (n+15)&~15: for size in [0xFFFFFFF1,0xFFFFFFFF], n+15 wraps -> need=0, span=16, avail passes, blk->cap=0 (:140), used+=16 — a non-NULL ptr to a cap=0 block for a ~4GB request. (b) span (:122) 16+need: for size in [0xFFFFFFE1,0xFFFFFFF0], need=0xFFFFFFF0, span=0x100000000->0, avail check '0>avail' false -> passes, used+=0 — cursor never advances, so two consecutive allocs both write headers at base+0 and return base+16, aliasing live allocations.
- **expected:** Return NULL (Rule 2) for any size whose aligned span overflows uint32_t; correct the false comment.
- **actual:** Non-NULL returns with cap=0 (write-past-block) or zero-advance aliasing (two live allocs share one address; second clobbers first's header).
- **repro:** flair_alloc(h,GENERAL,0xFFFFFFE1) twice -> same non-NULL ptr, used stays 0; flair_alloc(h,GENERAL,0xFFFFFFFF) -> non-NULL, stored cap=0 (verified by uint32_t simulation).

### R2.11. [P2] Heap REUSE walks the entire class free-list instead of head-only — violates documented LIFO spec, strands undersized head blocks permanently
- **bead:** initech-zhjo  **subsystem:** FLAIR/heap (os/flair/heap.c)
- **evidence:** heap.h:166-168 documents head-only ('if the free-list HEAD has cap>=request, pop it'), but heap.c:95-111 is a while(blk!=0) walk with a prev pointer that unlinks the first fitting node anywhere; a too-small head is skipped and a deeper block reused, and small head blocks never return to the bump allocator. The oracle (test_flair_heap.c:168-189, Property 2) tests only a single-element free-list, so head-vs-walk divergence is never exercised.
- **expected:** If freelist[klass]->cap>=need pop head, else fall through to BUMP; a too-small head must not trigger a deeper search.
- **actual:** Walks all nodes and reuses any fit regardless of position.
- **repro:** Alloc HANDLE/128, HANDLE/8; free 8 (head), free 128 -> list [8->128]; flair_alloc(HANDLE,64): spec says bump (head 8<64), code reuses the 128 block. Absent from test_flair_heap.c.

### R2.12. [P2] Oracle gap: no test/mutant for the heap align_up or span integer-overflow paths — both failure modes uninstrumented (Rule 6)
- **bead:** initech-5l5n  **subsystem:** FLAIR/heap oracle (harness/proptest/test_flair_heap.c + Makefile)
- **evidence:** Makefile:9160-9193 defines only FLAIR_HEAP_MUTATE_NO_BOUNDS and _NO_REUSE; neither drives size>UINT32_MAX-15 (align_up overflow) or size in [0xFFFFFFE1,0xFFFFFFF0] (span overflow). test_flair_heap.c Property 0 (:133-140) tests only size==0; exhaustion (Property 4) uses 512-byte blocks. The 'two named mutants prove it BITES' claim (heap.c:17) doesn't cover overflow.
- **expected:** A mutant + Property-0 asserts flair_alloc(h,k,UINT32_MAX)==NULL and flair_alloc(h,k,0xFFFFFFE1)==NULL, RED without the guard.
- **actual:** Oracle stays GREEN for a build that aliases memory on any size >= 0xFFFFFFE1.
- **repro:** Inspect Makefile:9181-9193 (only NO_BOUNDS/NO_REUSE) and test_flair_heap.c:133-141 (only size==0).

### R2.13. [P2] Chrome title text: 16-row Chicago cell overflows the 15-row stripe — surface_blit knocks PIN_LIGHT bg over the bevel-shadow row at every title-character column
- **bead:** initech-srd1  **subsystem:** FLAIR/chrome (os/flair/chrome.c:402, surface.c:76-87)
- **evidence:** chrome.c:386 cell_h=CHICAGO_CELL_H=16 (chicago8x16.h:45); stripe_rows=15 (chrome_metrics.h:75). :402 ty=stripe_top+(15-16)/2; C truncation makes (-1)/2=0, so ty=stripe_top and the :403 guard never fires. The 16-row cell spans [stripe_top, stripe_bot] (stripe_bot=stripe_top+15, :249). surface_blit (surface.c:76-87) writes bg=knock=PIN_LIGHT (chrome.c:407, #F3F3F3) for all 16 rows including stripe_bot, overpainting BEVEL_SHADOW drawn earlier at :316-318. Validated vs s7_701_titlebar_8bpp_zoom golden (continuous bevel-lo).
- **expected:** Clip/shift so the cell stays in [stripe_top, stripe_bot); bevel-shadow row uninterrupted. Fix: blit height min(cell_h, stripe_rows), or draw the bevel-shadow row after the title text.
- **actual:** A near-white (idx7) band under the title text at the bevel-lo row, flanked by BEVEL_SHADOW — a 3-segment discontinuity.
- **repro:** Any document window with a non-empty title; inspect stripe_bot under the text -> PIN_LIGHT instead of BEVEL_SHADOW.

### R2.14. [P2] text_draw hardcodes cell_w=8 for Geneva 9 but advance('W')=9 — column x+8 left unpainted, stale background at every 'W' right bearing
- **bead:** initech-n74i  **subsystem:** FLAIR/text (os/flair/text.h, spec/assets/geneva9.h)
- **evidence:** text.h:168 cell_w=8u unconditional for FONT_GENEVA9; :170 adv=geneva9_advance_w(c); geneva9.h:117 'W'=9. surface_blit paints columns x..x+7 (8 wide), then x advances by 9; column x+8 is written by neither this glyph nor the next, retaining pre-text framebuffer content (not bg). 'W' is the only Geneva-9 advance>8.
- **expected:** Paint the right-bearing column x+8 with bg (track paint_w=max(cell_w,adv), fill [x+cell_w,x+paint_w) with bg).
- **actual:** Column x+8 after 'W' holds stale content; over a pinstripe a 1px dark stripe shows at every 'W' right bearing. Latent until FONT_GENEVA9 is used in OS rendering.
- **repro:** Render 'Win' in FONT_GENEVA9 over a pinstripe; column x+8 after 'W' = background pattern, not knockout bg.

### R2.15. [P2] Oracle gap: test_text.c pixel-placement test uses only advances < cell_w ('Hi') — the advance>cell_w case ('W', adv=9) is unproven (Rule 6)
- **bead:** initech-6qln  **subsystem:** FLAIR/text oracle (harness/proptest/test_text.c)
- **evidence:** test_text.c:243-316 renders only 'Hi' ('H' adv=7, 'i' adv=4, both <8); no character with advance>8 and it never probes column x+8 after a wide glyph, so it cannot detect the unpainted 'W' right-bearing column.
- **expected:** Render 'Wi' (FONT_GENEVA9) and assert column x+8 is bg and the next glyph starts at x+9.
- **actual:** Oracle is GREEN on both the buggy and a fixed build; mutation-blind for advance>cell_w.
- **repro:** test_text.c:258 draws 'Hi' only; patching out a right-bearing fill leaves the suite GREEN.

### R2.16. [P2] Geneva 9 spec comment ('baseline row 8, descenders rows 9-10') contradicts the glyph data (x-height bottoms at row 7, descenders end at row 8)
- **bead:** initech-uct9  **subsystem:** FLAIR/text spec (spec/assets/geneva9.h)
- **evidence:** geneva9.h:35 and :162 document baseline row 8 / descenders rows 9-10. Data: 'a'(:1159),'o'(:1369),'n','u','x','z' bottom at row 7; descenders 'g'(:1249),'y'(:1519),'p'(:1384),'q'(:1399),'j'(:1294) all have r8 ink and r9=r10=0. Actual baseline=row 7, descender extent=row 8 (1 row below).
- **expected:** Correct the comment to 'baseline row 7; descenders reach row 8', or shift all glyphs down 1 and extend descenders to 9-10. Any consumer using the documented baseline places text 1 row too low.
- **actual:** Locked-data inconsistency; descenders are a single pixel vs the documented 2-3 rows.
- **repro:** Draw 'o' and 'p' with a guide at y+8 (documented baseline): 'o' bottoms at y+7 (above), 'p' barely reaches y+8.

### R2.17. [P2] Chrome title text has a left-edge clamp but no right-edge clamp — long titles overrun the zoom box and right frame
- **bead:** initech-v513  **subsystem:** FLAIR/chrome (os/flair/chrome.c:395-408)
- **evidence:** chrome.c:395-408 clamps tx left to left+box_clear (~left+23) but has no right guard; text_draw (text.h:147-186) clips only at bm->width (full screen), not the window right edge. Zoom box at zoom_x=right-20 (:364). Title overruns when tw>w-43. 'Saving tables to disk...' (~200px Chicago) in a window narrower than ~243px overwrites the zoom cbox glyph.
- **expected:** Clip title to [left+box_clear, right-zoom_clear) or truncate/elide; never paint over gadget boxes.
- **actual:** Text writes tx..tx+tw clipped only at screen width, overwriting the zoom box glyph and into regions to the right.
- **repro:** Document window, 25-char title, width <=240 -> zoom box nested-square glyph overwritten by title pixels.

### R2.18. [P2] flair_draw_menu_panel renders disabled items in full black — no gray dimming
- **bead:** initech-36pm  **subsystem:** FLAIR/menu (os/flair/menu.c flair_draw_menu_panel)
- **evidence:** menu.c:443-461 sets row_fg=fg,row_bg=bg and overrides color only for the hilite band (:444); it->enabled is never read in the draw loop (only at :218 in MenuInfo_item_selectable). A disabled item draws via text_draw with the same black-on-white as enabled (:461).
- **expected:** Disabled items dimmed (gray/50% stipple), per Inside Macintosh.
- **actual:** Disabled items indistinguishable from enabled.
- **repro:** Panel with disabled item 5 ('Revert', enabled=0), hilite != 5 -> row identical to enabled rows.

### R2.19. [P2] flair_draw_menu_panel never draws cmdChar command-key equivalents — RPAD column always blank
- **bead:** initech-iqhh  **subsystem:** FLAIR/menu (os/flair/menu.c flair_draw_menu_panel)
- **evidence:** menu.c:454-461 reads it->mark (:456) and it->text (:461) but never it->cmdChar (cmdChar appears only at :264-266 in MenuKey). FLAIR_MENU_ITEM_RPAD=12 (menu.h:97, 'cmd-key column gap') is reserved but never written. Fixture sets cmdChars 'N','O','W','S' (test_menu.c:94-97).
- **expected:** Cmd-symbol+letter in the right margin for items with cmdChar.
- **actual:** RPAD column always panel background; no shortcut legend.
- **repro:** Render a panel with cmdChar items; RPAD-zone pixels are all body bg.

### R2.20. [P1] flair_menu_track fixes the menu index at click time — cross-menu drag is structurally impossible
- **bead:** initech-rl4v  **subsystem:** FLAIR/menu (os/flair/menu.c flair_menu_track)
- **evidence:** menu.c:297 captures mi=MenuBar_hit(bar,startPt.h) once; the tracking loop (:305-307) and release check (:318-321) always pass the original mi to MenuInfo_item_at. Dragging into another title returns -1 (outside that panel). MenuBar_hit is never re-called.
- **expected:** Dragging into another menu title closes the current panel and opens that menu's (IM MenuSelect).
- **actual:** Only the clicked menu can open; releasing over another title yields out_hi=-1, result 0.
- **repro:** flair_menu_track startPt on menu 0 ('File'), pts ending inside menu 1 ('Edit') panel -> result 0, out_hi -1.

### R2.21. [P2] Oracle gap: test_menu.c Property 4 has no pixel assertion on the disabled-item row — the disabled-draw path is mutation-unproven (Rule 6)
- **bead:** initech-47qw  **subsystem:** FLAIR/menu oracle (harness/proptest/test_menu.c)
- **evidence:** test_menu.c:388-407 checks the hilited band (item 2, idx0) and a non-disabled gutter (item 0, idx1) but samples no pixel in the disabled item 5 ('Revert', :101) row (center y~101). MENU_MUTATE_SELECT_DISABLED (menu.c:211-214) targets selectability, not rendering.
- **expected:** A CHECK sampling the disabled row asserting the dimmed color, with a mutant subverting disabled rendering going RED.
- **actual:** Disabled-draw path unexercised; adding dimming logic cannot be verified.
- **repro:** Add dimming to flair_draw_menu_panel; make test-menu still passes.

### R2.22. [P2] TestControl never returns 0 for a disabled control (contrlHilite==255) — grayed controls still report hits
- **bead:** initech-ct5n  **subsystem:** FLAIR/control (os/flair/control.c TestControl)
- **evidence:** control.c:655-660 guards only ctrl==0 || !contrlVis — no contrlHilite==255 check (control.h:174 documents 255=inactive/grayed). The part-code switch (:666-719) runs for all types; test_control.c has no disabled-state test.
- **expected:** contrlHilite==255 returns part 0 regardless of position (IM-I p.I-329).
- **actual:** A disabled button/checkbox/scrollbar/radio still returns inButton/inCheckBox/etc., letting TrackControl fire on grayed controls.
- **repro:** pushButton with contrlHilite=255; TestControl inside rect -> inButton (10), expected 0.

### R2.23. [P2] All five DrawControl draw paths ignore contrlHilite==255 — disabled controls render pixel-identical to active ones
- **bead:** initech-qg22  **subsystem:** FLAIR/control (os/flair/control.c draw_push_button/check_box/radio_button/scrollbar/progress_bar)
- **evidence:** draw_push_button (control.c:355) only checks ==inButton (:367); draw_check_box(:405)/draw_radio_button(:455) never read contrlHilite; draw_scrollbar(:514) checks inThumb/inUp/inDown only; no draw function branches on 255 (grep '255' -> zero hits in draw funcs).
- **expected:** contrlHilite==255 draws dimmed/gray (IM-I p.I-318, MTE Ch5).
- **actual:** Disabled controls bit-identical to active.
- **repro:** DrawControl with contrlHilite=255 vs 0 -> identical render.

### R2.24. [P2] draw_check_box and draw_radio_button never read contrlHilite — no pressed (hilited) feedback during TrackControl
- **bead:** initech-ucch  **subsystem:** FLAIR/control (os/flair/control.c draw_check_box:400-448, draw_radio_button:455-495)
- **evidence:** draw_push_button reads contrlHilite==inButton and inverts face/ink (control.c:367,370,385), but draw_check_box and draw_radio_button never read contrlHilite. TrackControl sets contrlHilite=initial_part=inCheckBox(11) at :743; redraws during tracking show no change.
- **expected:** contrlHilite==inCheckBox(11) inverts the box/circle during tracking, like pushButton.
- **actual:** Checkbox/radio are visually static under the mouse — no press feedback.
- **repro:** checkBox with contrlHilite=11; DrawControl identical to contrlHilite=0.

### R2.25. [P2] Control draw paths hardcode CTRL_DESKTOP (teal) as a parent-background stand-in — push-button corners and checkbox/radio label cells render teal inside white dialogs
- **bead:** initech-whs3  **subsystem:** FLAIR/control (os/flair/control.c draw_push_button:379-382, draw_check_box:443, draw_radio_button:491)
- **evidence:** draw_push_button corner pixels (control.c:379-382) use CTRL_DESKTOP (=FLAIR_PART_DESKTOP, teal) for TL/TR/BL/BR rounding; draw_check_box:439-444 and draw_radio_button:490-494 pass bg=ctrl_px(port,CTRL_DESKTOP) to text_draw as the label background, and text_draw fills per-glyph cells with bg -> teal behind each label character. Buttons/checkboxes live on white dialog backgrounds, not the desktop.
- **expected:** Use the actual parent/container background (e.g. CTRL_WHITE) — corners erase to parent fill, labels knock to dialog bg.
- **actual:** 4 teal corner notches on every push button, and a teal strip behind every checkbox/radio label, against white dialogs.
- **repro:** DrawControl a pushButton and a labeled checkBox over CTRL_WHITE -> teal corner pixels and teal label cells.

### R2.26. [P2] TestControl returns inButton (push-button part code) for a degenerate (too-short) scrollBar
- **bead:** initech-44la  **subsystem:** FLAIR/control (os/flair/control.c TestControl scrollBar branch:683-685)
- **evidence:** control.c:683-685 'if (h < 2*btn) return inButton;' — inButton=10 (control.h:122) is the pushButton code; scrollbars should return only scrollbar codes (20/21/22/23/129) or 0. Callers switching on inButton run push-button action on a scrollbar.
- **expected:** A degenerate scrollbar returns 0 (consistent with draw_scrollbar emitting nothing).
- **actual:** Any hit in a scrollbar with h < 2*SB_ARROW (32) returns inButton.
- **repro:** scrollBar height=20; TestControl inside -> inButton(10), expected 0.

### R2.27. [P2] draw_scrollbar (min height 50) and TestControl (min height 32) disagree — scrollbars with h in [32,49] draw nothing but report live hit regions
- **bead:** initech-mktj  **subsystem:** FLAIR/control (os/flair/control.c draw_scrollbar:523, TestControl:683)
- **evidence:** draw_scrollbar:523 'if (h < 2*btn + SB_THUMB_MIN + 2)' -> threshold 50, returns early (draws nothing). TestControl:683 degenerate branch triggers only h<32. For h in [32,49]: nothing drawn, but TestControl falls through to full part logic returning inUpButton/inDownButton/inPageUp/inPageDown/inThumb.
- **expected:** TestControl and DrawControl agree on 'live'; a scrollbar that draws nothing returns 0.
- **actual:** h=40 emits zero pixels but TestControl returns inUpButton for a top-region click -> action fires on an invisible control.
- **repro:** scrollBar height=40; DrawControl draws nothing; TestControl at top+8 -> inUpButton(20), expected 0.

### R2.28. [P2] Control Manager NULL-pointer guards are documented fail-loud (panic) but silently return — Rule 2 and the SetControlValue contract violated
- **bead:** initech-gxo2  **subsystem:** FLAIR/control (os/flair/control.c SetControlValue:195-213 + control_init/DrawControl/TestControl/TrackControl/GetControlValue)
- **evidence:** control.h:229-232 says SetControlValue 'panics (flair_panic/DEBUG_ASSERT) if ctrl is NULL'; control.c:197-199 just returns, labeled 'fail-loud', but there is no panic (grep flair_panic/FLAIR_PANIC/DEBUG_ASSERT in control.c -> zero hits). Same silent pattern in control_init(:170), DrawControl(:615), TestControl(:657), TrackControl(:731). GetControlValue(:218-223) returns 0 on NULL, aliasing a control at contrlMin==0.
- **expected:** Panic on NULL (Rule 2) per the contract.
- **actual:** Every routine no-ops/returns 0 on NULL; NULL propagates with no diagnostic, and GetControlValue cannot distinguish NULL from a min-value control.
- **repro:** SetControlValue(NULL,50) -> silent return; GetControlValue(NULL) -> 0.

### R2.29. [P2] ModalDialog's updateEvt -> DrawDialog case is unreachable — event.c WNE never synthesizes updateEvt; covered dialog regions never repaint in the modal loop
- **bead:** initech-z0od  **subsystem:** FLAIR/dialog (os/flair/dialog.c:681-684, os/flair/event.c)
- **evidence:** dialog.c:681-684 handles case updateEvt: DrawDialog(dp), but event.c cook_raw (:324-491) emits only keyDown/keyUp (KEYBOARD), mouseDown/mouseUp/null (MOUSE), nullEvent (TICK) — no updateEvt and no FLAIR_RAW_UPDATE kind. event_model.h:71 claims synthetic updateEvt but event.c implements none. ModalDialog uses WNE directly (:566), not a router that injects updates.
- **expected:** WNE delivers updateEvt when covered dialog pixels are re-exposed; DrawDialog repaints.
- **actual:** The updateEvt case is unreachable; the :683 comment is false; dialog content exposed by window movement is never repainted in the modal loop.
- **repro:** Enter ModalDialog (sleepTicks>0); drag another window across the dialog -> exposed area not repainted.

### R2.30. [P2] shell_build_scene silently clamps negative n_sys_menus to 0 and truncates >65535 via uint16 cast — Rule 2 violation, inconsistent with the n_windows panic
- **bead:** initech-uskw  **subsystem:** FLAIR/shell (os/flair/shell.c:268)
- **evidence:** shell.c:268 s->bar_sys.n_menus=(uint16_t)((n_sys_menus<0)?0:n_sys_menus) — negative -> 0, >65535 silently truncated. Contrast :184-186 which SHELL_PANICs on n_windows<1 || >SHELL_MAX_WINDOWS.
- **expected:** SHELL_PANIC on negative or >UINT16_MAX n_sys_menus, like n_windows.
- **actual:** n_sys_menus=-1 -> a 0-menu bar (Apple slot only); 70000 -> 4464 menus; no diagnostic.
- **repro:** shell_build_scene(n_sys_menus=-1, others valid) -> no panic, bar_sys.n_menus==0.

### R2.31. [P1] dos_read lacks a Carry-Flag check — read errors are returned as byte counts, emitting/writing garbage in TYPE, COPY and batch loading
- **bead:** initech-winh  **subsystem:** MILTON/COMMAND.COM (os/milton/command.c dos_read)
- **evidence:** command.c:1439-1448 uses plain int $0x21 with no sbb %1,%1 to capture CF, unlike dos_open(:1427), dos_creat(:1469), dos_open_mode(:1511), dos_write_h(:1491). On AH=3Fh error (CF=1, AX=error code) dos_read returns the error code as a byte count. builtin_type(:2108-2113) then dos_write(chunk,err) emits err uninitialized bytes; builtin_copy(:2334-2344) writes them to the destination; batch_load_file(:2991-2995) advances total by err.
- **expected:** Capture CF with sbb, return 0/sentinel on error; callers break on failure.
- **actual:** A read error (e.g. ACCESS_DENIED=5) makes TYPE print 5 stack bytes, COPY corrupt the destination, batch mis-size the script.
- **repro:** Trigger an AH=3Fh error mid-TYPE/COPY -> garbage bytes in output/destination.

### R2.32. [P2] DEL wildcard silently deletes only the first 16 matches — no diagnostic when the roster cap is exceeded
- **bead:** initech-p4h7  **subsystem:** MILTON/COMMAND.COM (os/milton/command.c builtin_del)
- **evidence:** command.c:2385 char roster[16][13]; the FINDNEXT loop (:2394-2403) only records if(count<16) but keeps iterating; only the 16 recorded names are unlinked (:2405-2407); no message when the cap is hit.
- **expected:** Delete all matches, or print a truncation warning ('only 16 of N deleted').
- **actual:** DEL *.* with 17+ files deletes 16, leaves the rest silently, returns to prompt normally.
- **repro:** Create 20 files, DEL *.* -> 4+ survive with no error.

### R2.33. [P0] COPY <file> <file> (same source and destination) truncates the file to zero bytes and reports '1 file(s) copied' — silent data loss
- **bead:** initech-ojxn  **subsystem:** MILTON/COMMAND.COM (os/milton/command.c builtin_copy)
- **evidence:** command.c:2321-2350: dos_open(pair.first) read-only (:2321) then dos_creat(pair.second) create/truncate (:2326) with no src==dst check. When both name the same dir entry, dos_creat truncates to 0 before any read; the dos_read loop (:2334) hits EOF immediately; both handles close and it prints '1 file(s) copied' (:2350).
- **expected:** Detect src==dst and print 'File cannot be copied onto itself' (real DOS 3.3), or kernel sharing semantics prevent truncating an open file.
- **actual:** COPY FOO.TXT FOO.TXT zeroes FOO.TXT and reports success; original content permanently lost.
- **repro:** Write A:\TEST.TXT, COPY TEST.TXT TEST.TXT -> success message, then TYPE TEST.TXT is empty.

### R2.34. [P2] DIR header hardcodes 'Directory of A:\' regardless of CWD
- **bead:** initech-f2cz  **subsystem:** MILTON/COMMAND.COM (os/milton/command.c builtin_dir)
- **evidence:** command.c:2058 prints the literal 'Directory of A:\' with no CWD interpolation; cwd_display() (used by run_external/command_repl) is never called from builtin_dir. The listing body is correct (FINDFIRST uses the implicit CWD).
- **expected:** Compose the header from the live AH=47h CWD ('Directory of A:\SUB' after CD SUB).
- **actual:** Header always 'Directory of A:\' however deep the CWD; it misreports the location.
- **repro:** MD SUB, CD SUB, DIR -> header 'Directory of A:\'.

### R2.35. [P2] PROMPT $D always expands to '00-00-0000' — date never fetched (AH=2Ah) before cmd_render_prompt
- **bead:** initech-absm  **subsystem:** MILTON/COMMAND.COM (os/milton/command.c command_repl, batch_echo_line)
- **evidence:** command.c:3531-3533 zeros pctx.year/month/day with no AH=2Ah GET DATE, while time fields are populated via dos_gettime (:3525-3530); same zeroing in batch_echo_line (:2910). cmd_render_prompt's $D case (:806-820) formats the zeros as '00-00-0000'.
- **expected:** $D resolves to the current date via AH=2Ah (e.g. '06-28-2026'); $T already works.
- **actual:** $D always '00-00-0000' at every REPL iteration and batch echo.
- **repro:** PROMPT $D$P$G -> '00-00-0000A:\>'.

### R2.36. [P2] MZ loader entry_off bounds check is off-by-one (`>` not `>=`) — entry == load_module_len passes, JMP target one byte past the module
- **bead:** initech-jy7o  **subsystem:** LOADER/MZ (os/milton/loader.c)
- **evidence:** loader.c:310 if(img.entry_off > img.load_module_len); when equal the check is false and :314 sets out->entry=PROGRAM_IMAGE+entry_off, one byte past the module. The :311 comment says 'must point INTO the loaded module'.
- **expected:** Reject with LOADER_ERR_BAD_FORMAT; the check should be `>=`.
- **actual:** entry_off==load_module_len passes; entry one byte past the code (uninitialized BSS/guard).
- **repro:** InitechMZ with load_module_len=16, cs=0, ip=16 -> loader_prepare_mz returns LOADER_OK, plan.entry=PROGRAM_IMAGE+16.

### R2.37. [P2] loader.h documents loader_prepare_mz as memmove-then-relocs, but the code (correctly) does relocs-then-memmove — the contract would clobber a header-resident reloc table
- **bead:** initech-d2ub  **subsystem:** LOADER/MZ contract (os/milton/loader.h)
- **evidence:** loader.h:158-165 lists step 3=memmove the load module down, step 4=apply relocs (with a self-contradictory '(pre-move offset)' note). loader.c:250-291 does the opposite: relocs at the original file offset first (:250-281, with a comment at :252-258 explaining a header-gap reloc table would be overwritten by a move-first), then memmove (:283-291). test_mzload.c case 1b exercises exactly a header-resident reloc table.
- **expected:** The header should document relocs-first (at file_at+load_module_off), then memmove down.
- **actual:** A reader re-implementing from the header would memmove first, clobbering a header-resident reloc table before fixups are read.
- **repro:** Implement per loader.h steps 3->4; a header-resident-reloc MZ misrelocates and returns LOADER_OK.

### R2.38. [P2] Baked load_program() never sets g_load_active — a baked program calling AH=4Bh EXEC nests a FAT load that clobbers g_loader_ctx and hangs the system on the outer program's exit
- **bead:** initech-4jic  **subsystem:** LOADER/reentrancy (os/milton/loader.c)
- **evidence:** load_program_from_fat guards on g_load_active (:1009) and sets it (:1109), but load_program() (:800-822, the baked path) calls loader_run_plan() without touching g_load_active. A baked program's AH=4Bh sees g_load_active==0, proceeds, and the nested loader_run_plan sets g_loader_ctx=&ctx_inner over the outer's; after the inner terminates g_loader_ctx=0 (:735); the outer's AH=4Ch then dereferences NULL in loader_exit_hook -> cli;hlt forever. Comment at :509-510 notes the baked path 'must keep working unchanged'.
- **expected:** load_program() sets/clears g_load_active, or a nested-EXEC guard covers the baked path; a baked EXEC gets LOADER_ERR_BUSY.
- **actual:** Nested EXEC corrupts g_loader_ctx and PROGRAM_IMAGE; the outer program's exit hangs (cli;hlt).
- **repro:** Baked program issuing AH=4Bh then AH=4Ch -> hang.

### R2.39. [P2] Oracle gap: test_mzload.c has no entry_off == load_module_len case — the MZ entry off-by-one boundary is mutation-unproven (Rule 6)
- **bead:** initech-8obm  **subsystem:** LOADER/MZ oracle (os/milton/test_mzload.c)
- **evidence:** Cases cover entry_off=0, 0x10 (both < 0x40 module), FOREIGN_MZ tag, oversized len — none with e_cs*16+e_ip == load_module_len. The bounds check (loader.c:310) uses `>`; the equal case is undetected, and the existing cases differ from the boundary only by a constant.
- **expected:** A case with entry_off==load_module_len asserting LOADER_ERR_BAD_FORMAT, RED under a `>`-vs-`>=` mutation.
- **actual:** Oracle passes on both correct and regressed builds; boundary unproven.
- **repro:** Add mod_len=0x40, cs=0, ip=0x40 case -> current `>` returns LOADER_OK, corrected `>=` returns BAD_FORMAT.
- **related:** the MZ entry_off off-by-one is the code defect; this is the distinct oracle gap

### R2.40. [P2] psp.c comment claims the INT 20h trap gate is 'the loader's job (deferred)', but sysinit.c already installs it at init
- **bead:** initech-tpwp  **subsystem:** PSP/sysinit (os/milton/psp.c:88, os/milton/sysinit.c:213)
- **evidence:** psp.c:88 says the 0x20 trap gate is 'the LOADER's job (deferred)'; sysinit.c:213 idt_install_trap(0x20u,(void*)int20_entry) installs it at system init. mzexec_fixture.asm relies on int 0x20 working at exit.
- **expected:** The comment should state sysinit installs INT 20h (not the loader, not deferred).
- **actual:** The stale 'deferred' note contradicts sysinit and could lead a maintainer to add a redundant loader-side install or assume the gate is missing.
- **repro:** Compare sysinit.c:213 (0x20u install) with psp.c:88.

### R2.41. [P2] FLAIR_LIVE_TENANTS menu-bar click always dispatches to bar_sys even after a foreground switch — visible bar shows the tenant's menus but clicks drop System-7 items
- **bead:** initech-t1rv  **subsystem:** FLAIR/menu pump (os/milton/kmain.c WNE pump)
- **evidence:** kmain.c:2261 calls flair_live_do_menu(..., &ctx.scene->bar_sys, ...) unconditionally, but the post-activation swap (:2291-2293) paints ten_plist.head->menubar (bar_photoshop after switching to HELLO, set at :1961) into the band. MenuBar_hit/flair_draw_menu_panel/MenuInfo_item_at/MenuSelect all get the wrong MenuBar*.
- **expected:** Dispatch passes ten_plist.head->menubar, matching the painted bar.
- **actual:** After switching to HELLO, File/Edit clicks hit bar_sys geometry and drop System-7 items/IDs to serial.
- **repro:** Boot FLAIR_LIVE_TENANTS; click HELLO's window to foreground it; click the menu bar -> FLAIR-MENU reports bar_sys menuID, not bar_photoshop.
- **related:** distinct from round-1 app-switch static-bar clobber; this is post-switch click-dispatch routing

### R2.42. [P2] flair_live_do_drag calls DragWindow without a prior SelectWindow — an inactive window drags while staying behind in z-order (ghost drag)
- **bead:** initech-haaq  **subsystem:** FLAIR/compositor pump (os/milton/kmain.c flair_live_do_drag)
- **evidence:** kmain.c:1153 DragWindow(ctx->wm,w,dh,dv) with no SelectWindow; flair_app_dispatch returns early for inDrag (process.c:540-543, no activation); DragWindow only MoveWindows (window.c:420-426); SelectWindow (window.c:357-371, relinks to front) is never called. Both pump arms (kmain.c:2390, :2415) share this. IM (MTE 6) requires SelectWindow before DragWindow for inactive windows.
- **expected:** Clicking an inactive window's title bar raises it (SelectWindow) then drags at the front.
- **actual:** The inactive window is dragged in place behind the foreground window; chrome repaints clipped to the occluded slice — a background ghost drag with no activation.
- **repro:** Boot FLAIR_LIVE_*; drag HELLO's title bar (below NOTES) -> HELLO moves but stays behind NOTES, no FLAIR-DISPATCH.
- **related:** distinct from round-1 'peer window erased from compositor after drag'

### R2.43. [P2] flair_live_do_close hides the window but never updates the tenant process list — the closed foreground tenant stays at list->head and keeps receiving key events
- **bead:** initech-8fhu  **subsystem:** FLAIR/process pump (os/milton/kmain.c flair_live_do_close)
- **evidence:** kmain.c:1190-1200 calls HideWindow + desktop_paint_damage + flair_desktop_present and returns — no FlairProcess_terminate, no ten_plist relink, no head demotion. process.c:574-578 then delivers keyDown to list->head (still the closed, now-invisible tenant). HideWindow's reaffirm_active (window.c:321) sets a different wm active window, mismatching ten_plist.head.
- **expected:** Close promotes the remaining visible tenant to list->head; keys route to it.
- **actual:** Keyboard input goes to the invisible closed tenant until the user clicks the other window.
- **repro:** Boot FLAIR_LIVE_TENANTS INTERACTIVE; close NOTES (foreground); type -> keyDown delivered to invisible NOTES, not HELLO.

### R2.44. [P2] Dragging the already-foreground tenant repaints chrome but never re-seeds content updates — the moved window's interior shows stale offscreen pixels at the new position
- **bead:** initech-5wbf  **subsystem:** FLAIR/compositor pump (os/milton/kmain.c flair_live_do_drag)
- **evidence:** kmain.c:1153-1168: DragWindow + WindowMgr_invalidate + desktop_paint_damage (chrome only, then clears all updateRgns at desktop.c:233-236) + flair_desktop_present. The post-activation block (:2273) fires only when ten_plist.head != ten_prev_fg; dragging the foreground window leaves head unchanged (inDrag returns early, process.c:540-543), so no flair_route_updates/re-invalidate follows. The moved contRgn keeps prior offscreen content.
- **expected:** After the drag, re-invalidate the moved window's content and route an updateEvt so the tenant repaints its fill at the new position.
- **actual:** New title bar/frame correct but content interior shows pre-drag offscreen (teal/other window) instead of the tenant fill.
- **repro:** Boot FLAIR_LIVE_TENANTS INTERACTIVE with NOTES foreground; drag NOTES over teal -> content shows teal instead of gray.
- **related:** distinct from round-1 'drag erases a peer window'; this is the dragged window's own missing content

### R2.45. [P2] ref_tenant.c content rect top uses the stale top+FRAME+TITLEBAR_H (=top+20) formula; chrome now draws content at top+19 — the top content row falls outside contRgn (clicks drag instead of route; 1px white seam on NOTES)
- **bead:** initech-l0mh  **subsystem:** FLAIR/apps (os/flair/ref_tenant.c:288-289)
- **evidence:** chrome.c:420-426 records content_top shifted up by 1 (new=top+19; old=top+1+19); ref_tenant.c:288-289 still uses bounds.top+FLAIR_CHROME_FRAME+FLAIR_CHROME_TITLEBAR_H=top+20. So contRgn.top=top+20 while chrome paints content from top+19: row top+19 is chrome-painted white but not in contRgn -> FindWindow routes it to inDrag (or inGoAway/inZoomIn in the wings); window.c:485 tb becomes 20 vs 19, amplifying the close/zoom hit-zone error by 1px; NOTES shows a 1px white seam between the black shared-frame line (top+18) and the gray fill (top+20).
- **expected:** ref_tenant.c:289 = bounds.top + FLAIR_CHROME_TITLEBAR_H (=top+19); TITLEBAR_H already includes both frame rows (chrome_metrics.h:320-321).
- **actual:** contRgn.top one row too low; row top+19 is a phantom title-bar row; clicking it drags the window; NOTES has a 1px white separator.
- **repro:** Click (center_x, top+19) in a document window -> inDrag/DragWindow instead of inContent; inspect NOTES top content row in flair_appswitch_pre.png -> 1px white stripe.
- **related:** round-1 'tenant content rect extends into scrollbar column' is the horizontal extent; this is the vertical top boundary — distinct axis


---

# Round 3 -- SAMIR/dBASE + seed compiler + ANSI/console + region/FAT

54 raw -> 48 verified REAL -> 44 deduped (1 P0, 11 P1, 32 P2). Label `gui-bughunt`.

## Summary

Round 3 verified 44 distinct defects across the previously-uninspected subsystems — SAMIR (xBase interpreter, .dbf/.dbt/.ndx codecs, dot-prompt nav/query/flow/mutate), the seed Pascal->x86 compiler, ANSI/console, and the region/FAT engines — after collapsing four cross-lens duplicate pairs (fn_round, CTOD, FILE(), and the ROUND oracle gap, each found independently by the samir-interp and samir-builtins lenses). The set is 1 P0 (a stack-buffer overflow in the region xmerge scratch when two maximally-dense scanlines merge), 11 P1 correctness bugs (negative ROUND truncation, CTOD calendar rollover, +2-form .dbf record reads, ndx duplicate-key delete unreachability, SKIP/LOCATE scope errors, ?/.T. logical rendering, SET DATE/CENTURY ignored, PACK leaving NDX stale, and the seed string-pool false rejection), and 32 P2s spanning latent overflows, codec/coercion corruption, and 13 explicitly mutation-unproven oracle gaps (Rule 6). None of the 44 overlap the 94 round-1/2 FLAIR/DOS findings and none touch enforced canon (SAMIR Y2K mis-aging, pie==116%, the trailing-minus, hourglass cursor).

## Bugs

### R3.1. [P0] region_op: per-band xmerge scratch[RGN_ROW_X_MAX=256] too small — silent stack buffer overflow when na+nb > 256
- **bead:** initech-mswo  **subsystem:** FLAIR/region (os/flair/atkinson/region.c)
- **evidence:** region.c:425 `int16_t scratch[RGN_ROW_X_MAX]` (256) is passed to xmerge at region.c:451; xmerge writes `out[no++]=x` (region.c:391-395) with no bound on `no`. region_normalize caps each input row to 256 entries, so an XOR/merge of two maximally-dense rows yields no<=na+nb<=512, overrunning the 256-slot stack buffer. The scratch needs 2*RGN_ROW_X_MAX, not RGN_ROW_X_MAX.
- **expected:** scratch sized for na+nb (>=512), or xmerge checks `if (no >= cap) fail-loud` before each write.
- **actual:** xmerge writes entries 256..511 into the next stack frame when two normalized rows together exceed 256 inversion points; stack corruption.
- **repro:** Two regions each a single scanline of 128+ disjoint spans (256 inversion points each); region_op(&out,&A,&B,RGN_OP_XOR) overruns scratch[256..511].

### R3.2. [P1] fn_round uses (int64_t) truncation-toward-zero instead of floor — wrong result for all negative non-integer inputs
- **bead:** initech-eyig  **subsystem:** SAMIR/builtins (os/samir/core/fn_builtins.c)
- **evidence:** fn_builtins.c:845 `rounded=(double)(int64_t)(v*scale+0.5)/scale;` and 848 (negative-dec arm) `(int64_t)(v/scale+0.5)*scale`. (int64_t) truncates toward zero. For v=-2.7,dec=0: v+0.5=-2.2, (int64_t)(-2.2)=-2 → -2.0 (should be -3.0). dec_format (rt.c:247) correctly uses the static rt_floor64 (rt.c:207-213); fn_round skips it. The comment at fn_builtins.c:818-819 falsely claims it uses 'the same idiom as dec_format'.
- **expected:** ROUND(-2.7,0)=-3.0; ROUND(-1234,-2)=-1200.0 — both arms should use rt_floor64.
- **actual:** ROUND(-2.7,0)=-2.0; ROUND(-27,-1)=-20.0; ROUND(-1234,-2)=-1100.0 — every negative value rounds one unit toward zero.
- **repro:** Evaluate ROUND(-2.7,0) → -2.0 (expected -3.0).

### R3.3. [P1] CTOD validates only range (dd<=31), not calendar correctness — Feb 31 / Apr 31 / non-leap Feb 29 roll over instead of returning blank date
- **bead:** initech-9u0f  **subsystem:** SAMIR/builtins (os/samir/core/fn_builtins.c)
- **evidence:** fn_builtins.c:517 `if (mm<1||mm>12||dd<1||dd>31||yy<1900||yy>2155)` is the only guard before jdn_from_ymd(yy,mm,dd). dd in [29..31] for short months passes; jdn_from_ymd(1990,2,31) arithmetically rolls to Mar 3 1990. No per-month day cap.
- **expected:** CTOD('02/31/90'), CTOD('04/31/85'), CTOD('02/29/85') return blank date (JDN 0.0) — III+ returns blank for calendar-invalid input.
- **actual:** Returns a non-zero JDN for the rolled-over date; DTOC/MONTH/DAY then propagate wrong values with no error.
- **repro:** CTOD('02/30/90') → JDN 2447953; DTOC(...) → '03/02/90' instead of '        '.

### R3.4. [P2] FILE() requires an open database cursor (need_dbcur) — always fails XBEE_NO_DATABASE(#52) when no table is in USE
- **bead:** initech-52ph  **subsystem:** SAMIR/builtins (os/samir/core/fn_builtins.c)
- **evidence:** fn_builtins.c:1989 `if (!need_dbcur(ctx,err)) return *err;` inside fn_file before reading ctx->dbcur->file_exists. need_dbcur (1786-1791) returns 0/XBEE_NO_DATABASE(52) when ctx->dbcur==NULL. FILE() is a PAL/environment function (system-and-database-functions.md) with no open-database prerequisite; file_exists was placed on the dbcur vtable instead of a PAL-level hook.
- **expected:** FILE('X.DBF') returns .T./.F. by filesystem existence regardless of any USE.
- **actual:** Returns XBEE_NO_DATABASE(#52) with ctx->dbcur==NULL, breaking the common 'check then USE' pattern.
- **repro:** Evaluate FILE('ANY.DBF') with ctx.dbcur=NULL → rc=52.

### R3.5. [P2] concat_plus/concat_minus missing XB_STR_MAX(254) guard — C+C and C-C silently produce strings exceeding the III+ limit
- **bead:** initech-8w5e  **subsystem:** SAMIR/eval (os/samir/core/eval.c)
- **evidence:** eval.c:148-163 (concat_plus) and 178-200 (concat_minus) compute tot=la+lb and store without checking tot>254. Every string-producing builtin guards: fn_builtins.c:1123 (REPLICATE), :264 (SPACE), :350 (STR), with `#define XB_STR_MAX 254` at fn_builtins.c:145. xb_val.u.c.len is uint16_t (>65535 truncates silently).
- **expected:** SPACE(130)+SPACE(130) (260 bytes) should fail XBEE_SPACE_LARGE(#59) like the other string operators.
- **actual:** Produces an arbitrary-length result bounded only by scratch_cap; no error.
- **repro:** Evaluate SPACE(130)+SPACE(130) → rc=0, v.u.c.len=260 (expected XBEE_SPACE_LARGE).

### R3.6. [P1] dbf_open_rw normalizes header_length to +1 form before reads use it — breaks every record read on +2-form files (BANK.DBF, TAX.DBF)
- **bead:** initech-jf8p  **subsystem:** SAMIR/.dbf codec (os/samir/fs/dbf.c)
- **evidence:** dbf.c:459 sets tbl->header_length to on-disk value; records loaded from local header_length at 581; then dbf.c:602 overwrites tbl->header_length to (DBF_HDR_SIZE+descbytes+1), one byte short for a +2 file. dbf_read_rec (735) computes rec_off from the normalized (short) header_length, lands on the extra 0x00 of the 0x0D 0x00 terminator → not 0x20/0x2A → -DBF_ERR_BAD_REC (750-757). +2 fixtures: dbf_format.h:54.
- **expected:** dbf_read_rec on a +2-form file reads record 1 at the original header_length offset, returning the live flag and decoded fields.
- **actual:** Seeks one byte early, reads 0x00 as delete flag, returns -DBF_ERR_BAD_REC for every record. Read-only dbf_open skips line 602 and is unaffected.
- **repro:** dbf_open_rw(BANK.DBF); dbf_read_rec(tbl,1,out,&del) → -DBF_ERR_BAD_REC(12).

### R3.7. [P1] ndx_delete_key first-match descent returns -NDX_ERR_NOTFOUND for valid (key,recno) entries in any non-leftmost duplicate-key leaf
- **bead:** initech-0g22  **subsystem:** SAMIR/.ndx B-tree (os/samir/fs/ndx.c)
- **evidence:** ndx.c:2287-2293 descent breaks at the FIRST separator with key>=target (identical to insert). For a bulk-built non-unique index where >kpp records share key K, delete(K,recno-in-leaf_2) descends only into leaf_1, scans, fails NOTFOUND though the entry exists in leaf_2 (reachable by ndx_inorder but unreachable by delete/seek).
- **expected:** ndx_delete_key removes any existing (key,recno) regardless of which equal-key leaf holds it.
- **actual:** Scans only the leftmost matching leaf; entries in later equal-key leaves are permanently undeletable; returns NOTFOUND for valid entries.
- **repro:** ndx_build 32 records key='SMITH' (kpp=31); ndx_delete_key('SMITH',recno=32) → -NDX_ERR_NOTFOUND while ndx_inorder still visits recno=32.

### R3.8. [P2] ndx_insert_key duplicate-key descent always picks leftmost matching leaf — ndx_inorder yields unsorted recno sequence within a key after a split
- **bead:** initech-nrkx  **subsystem:** SAMIR/.ndx B-tree (os/samir/fs/ndx.c)
- **evidence:** ndx.c:1815-1821 descent breaks on the FIRST separator with key>=new_key. After a leaf split on K-entries (left and trailing both have separator K), every subsequent insert of (K,large_recno) goes to the leftmost K-child. Trace: insert (K,33) after split lands in left leaf among (K,1)..(K,16); inorder visits ...(K,16),(K,33),(K,17)... — not ascending recno within K.
- **expected:** Each (key,recno) placed in the leaf whose subtree range contains it; inorder monotonic by (key,recno).
- **actual:** Late-recno entries inserted before early-recno entries during traversal; 'inorder is sorted' invariant broken for any non-unique index with a K-split followed by a K insert.
- **repro:** Insert (K,1)..(K,31), then (K,32) (split), then (K,33); inorder shows 33 before 17-32.

### R3.9. [P1] SKIP -1 from physical EOF lands at nrec-1 instead of nrec (off-by-one under-skip)
- **bead:** initech-dmrw  **subsystem:** SAMIR/nav (os/samir/cmd/nav.c, os/samir/cmd/workarea.c)
- **evidence:** nav.c:373-385: SKIP past last sets wa->recno=nrec then eof=1. SKIP -1 reads cur via wa_recno (nav.c:359) = raw wa->recno = nrec (workarea.c:1044-1049 returns the raw field), so next=nrec-1. The external wac_recno (881-883) correctly returns nrec+1 at EOF, but the skip path uses wa_recno. Oracle gap: test_interp_nav.c:222-224 gates this path, never tests SKIP -1 from EOF.
- **expected:** From virtual EOF (nrec+1), SKIP -1 lands at record nrec.
- **actual:** Lands at nrec-1 because internal recno is nrec, not the virtual nrec+1.
- **repro:** GO BOTTOM; SKIP (to EOF); SKIP -1; RECNO() returns nrec-1.

### R3.10. [P1] LOCATE NEXT n ignores the scope record-count limit — scans entire remainder to EOF
- **bead:** initech-qw4e  **subsystem:** SAMIR/query (os/samir/cmd/query.c)
- **evidence:** q_locate (query.c:921) parses qc.scope_n; for QS_NEXT lines 939-943 fall through to q_locate_scan without passing the limit. q_locate_scan (885) has no limit parameter; its loop (898) runs `while(!wa_eof(...))`. No LOCATE NEXT test exists in any oracle.
- **expected:** LOCATE NEXT n FOR <cond> scans at most n records then stops regardless of match.
- **actual:** Scans from current position all the way to EOF, ignoring n.
- **repro:** 100-rec table at record 1; LOCATE NEXT 3 FOR <true only at 50> lands at 50 instead of not-found at record 3.

### R3.11. [P1] LOCATE RECORD n FOR <cond> scans past record n when FOR is false, continuing to EOF
- **bead:** initech-wcb7  **subsystem:** SAMIR/query (os/samir/cmd/query.c)
- **evidence:** For QS_RECORD q_locate (query.c:935) calls wa_nav_goto(area,scope_n) then falls into q_locate_scan with start_here=1 (941). q_locate_scan loop (898) `while(!wa_eof)`; FOR false at n calls wa_nav_skip(...,1) (914) and continues to EOF. scope_n is never used as a scan bound. No LOCATE RECORD test in any oracle.
- **expected:** LOCATE RECORD n checks only record n; FOR false → FOUND()=.F., cursor stays at n.
- **actual:** Scans n..EOF, may match a later record and report FOUND()=.T. at the wrong position.
- **repro:** 5-rec table, record 3 AMT=99; LOCATE RECORD 2 FOR AMT=99 lands at 3 with FOUND()=.T.

### R3.12. [P1] ? / ?? renders logical XB_L as 'T'/'F' instead of '.T.'/'.F.' — oracle heresy masks the bug
- **bead:** initech-3p9e  **subsystem:** SAMIR/query (os/samir/cmd/query.c)
- **evidence:** q_render_val case XB_L (query.c:409-411) `q_append(...,v->u.l?"T":"F",1u)`. ? (q_question, 840) routes through the same q_render_val, so ? logicals also print T/F. Oracle test_samir_repl.c documents '-> .T.' in comments (line 29,395-407) but asserts cap_has('\nT') at line 349 — agrees with the wrong code by construction.
- **expected:** III+ '? <logical>' prints '.T.'/'.F.'; only LIST columns use 'T'/'F'.
- **actual:** Both ? and LIST emit 'T'/'F'; ? never produces the dotted form.
- **repro:** ? .T. prints 'T' (expected '.T.').

### R3.13. [P1] q_render_val always formats XB_D as AMERICAN MM/DD/YY, ignoring SET DATE and SET CENTURY
- **bead:** initech-ue7y  **subsystem:** SAMIR/query (os/samir/cmd/query.c, os/samir/cmd/set.c)
- **evidence:** query.c XB_D case (392-407) hardcodes MM/DD/YY two-digit-year byte arithmetic; neither ctx->set_date_fmt nor ctx->set_century is read (grep of query.c finds zero matches). set.c:277,300 correctly maintain those fields on xb_ctx.
- **expected:** SET DATE BRITISH → DD/MM/YY; SET CENTURY ON → 4-digit year.
- **actual:** All dates display MM/DD/YY two-digit-year regardless of SET DATE/CENTURY.
- **repro:** SET DATE BRITISH; ? CTOD('01/15/93') prints '01/15/93' instead of '15/01/93'.

### R3.14. [P1] PACK does not update or rebuild open NDX indexes — silently corrupts them after record renumbering
- **bead:** initech-x87g  **subsystem:** SAMIR/mutate (os/samir/cmd/mutate.c)
- **evidence:** m_pack (mutate.c:884-905) calls dbf_pack (renumbers survivors), dbf_flush, wa_refresh, wa_nav_reset — no NDX call. ndx.h:636 states the S5.5 contract 'After DBF DELETE/PACK, call ndx_delete_key for each deleted recno.' Open NDX B-trees keep stale physical recnos; wa_nav_reset only drops the in-memory order cache, not the on-disk tree.
- **expected:** PACK rebuilds all open NDX files (REINDEX) after renumbering, or at minimum ndx_delete_key per deleted recno.
- **actual:** NDX retains stale physical recnos; post-PACK SEEK/FIND return wrong records.
- **repro:** 5-rec table, NDX on CODE; DELETE rec 2; PACK; SEEK of an old record's key returns wrong results.

### R3.15. [P1] scan_string pool-wrap fires only when out==0 — valid strings falsely rejected 'too long' when the pool tail is small
- **bead:** initech-2hg9  **subsystem:** seed/lexer (seed/lexer.c)
- **evidence:** lexer.c:268-278: `if (out>=avail){ if (lx->strpos!=0 && out==0){ wrap } else return make_error("string literal too long") }`. The wrap to pool-start only triggers when no bytes are yet stored (out==0). If >=1 byte is stored and the tail is exhausted, the error is returned despite free space at pool start.
- **expected:** Strings up to LEXER_STRBUF_CAP always succeed regardless of pool cursor, by wrapping before/while scanning.
- **actual:** A string starting at strpos in (0,CAP) whose length exceeds the remaining tail is rejected even though it fits from position 0.
- **repro:** Drive strpos to 1020 (avail=4), lex 'hello' (5 chars): 5th char hits out(4)>=avail(4) with out!=0 → false 'too long'.

### R3.16. [P2] Invariant-2 file-size check overflows uint32_t for large nrec — malformed .dbf admitted as within-bounds
- **bead:** initech-ptqc  **subsystem:** SAMIR/.dbf codec (os/samir/fs/dbf.c)
- **evidence:** dbf.c:449 `body = header_length + nrec*(uint32_t)record_length` wraps silently (nrec uint32_t). nrec=0x80000001, rl=4 → product=4, body~36, so the check `(uint32_t)filesz < body` (450) passes for any file >=36 bytes. Same overflow at dbf.c:577 `total=nrec*rlen` undersizes the record-load buffer.
- **expected:** Reject when nrec*record_length+header_length exceeds 2^32 with -DBF_ERR_BAD_FILESZ.
- **actual:** Overflow makes body tiny; table opens with a wildly wrong nrec (e.g. 2,147,483,649).
- **repro:** Header nrec=0x80000001, record_length=4, header_length=32, file>=36 bytes; dbf_open succeeds.

### R3.17. [P2] coerce_and_write_field 'M' branch writes raw character bytes into the 10-byte .dbt block-pointer slot — memo pointer unreadable on re-read
- **bead:** initech-1pd7  **subsystem:** SAMIR/.dbf codec (os/samir/fs/dbf.c)
- **evidence:** dbf.c:1530-1543 (M case in coerce_and_write_field, called by dbf_replace) copies v->u.c.p left-justified into the 10-byte block-number field instead of appending to .dbt and storing a block number. xbase_coercion.json:94 (target M, expr C) = 'ok; memo stores text'. On re-read dbf.c:883 dec_parse(raw,10) on non-digit text → 0.0 → block_num<=0.5 → xb_u() (no memo). The code comment admits 'full .dbt integration is S2.2; here we treat M like C'.
- **expected:** Write text to .dbt via dbt_append, store the returned block number right-justified; or leave the field untouched and fail if .dbt is unimplemented.
- **actual:** Raw chars stored in the pointer slot; subsequent dbf_read_rec returns xb_u(), silently discarding the value.
- **repro:** dbf_replace(memo_idx, xb_c('hello',5)); flush; read → vals[memo_idx].t==XB_U.

### R3.18. [P2] coerce_and_write_field D-type writes 8 bytes unconditionally without flen guard — corrupts adjacent bytes on malformed descriptors
- **bead:** initech-ifpd  **subsystem:** SAMIR/.dbf codec (os/samir/fs/dbf.c)
- **evidence:** dbf.c:1499-1507 (D branch) writes dst[0..7] with i in 0..7 regardless of flen. fmt_field's D branch (985) correctly guards `i<8u && i<(uint32_t)flen`. dbf_create enforces D len==8 (1076), but dbf_open_rw copies field_len verbatim (549), so an externally-sourced .dbf with a D descriptor flen=6 + dbf_replace writes 2 bytes past the slot.
- **expected:** Mirror fmt_field: write min(8,flen) digits then space-pad to flen.
- **actual:** Always writes 8 bytes; for flen<8 overwrites the next field (or past the record buffer).
- **repro:** Open_rw a .dbf with D field_len=6; dbf_replace a date; bytes at date_start+6/+7 overwritten.

### R3.19. [P2] dbt_read: rt_memcpy(final_buf, work, work_len) overreads the work buffer when work_len exceeds work_cap+1 on corrupt .dbt input
- **bead:** initech-jy6y  **subsystem:** SAMIR/.dbt codec (os/samir/fs/dbt.c)
- **evidence:** dbt.c:542 work=alloc(work_cap+1); writes guarded by `if (work_len<work_cap)` (585,603,614) but work_len++ is unconditional. If the 0x1A 0x1A terminator lies beyond (next_free-blockno)*512 bytes, work_len exceeds work_cap+1; dbt.c:633-635 allocs work_len and rt_memcpy's work_len bytes from a work_cap+1 buffer, reading stale arena bytes past the region.
- **expected:** Cap the copy to min(work_len,work_cap), or fail -DBT_ERR_NO_TERM when no terminator within the live block span.
- **actual:** For corrupt overflowing memo, rt_memcpy reads beyond work into adjacent arena memory; final_buf carries stale bytes.
- **repro:** next_free=2 (1 live block), write 513 bytes then 0x1A 0x1A; dbt_read copies 1 byte past work.

### R3.20. [P2] ndx_open_impl: zero-byte file returns -NDX_ERR_IO instead of -NDX_ERR_BAD_SIZE, conflating I/O failure with size violation
- **bead:** initech-f6q7  **subsystem:** SAMIR/.ndx open/close (os/samir/fs/ndx.c)
- **evidence:** ndx.c:310-313 `fsize_raw=seek(...,END); if (fsize_raw<=0) return -NDX_ERR_IO;`. An empty file: seek succeeds, returns 0; 0<=0 → -NDX_ERR_IO. A 0-byte file is structurally too small (the BAD_SIZE condition two lines later). Only fsize_raw<0 is a genuine I/O error.
- **expected:** Distinguish fsize_raw<0 (-NDX_ERR_IO) from fsize_raw==0 (-NDX_ERR_BAD_SIZE).
- **actual:** 0-byte file returns -NDX_ERR_IO, indistinguishable from a seek failure.
- **repro:** ndx_open on a 0-byte file → -NDX_ERR_IO (expected BAD_SIZE); a 513-byte file correctly returns BAD_SIZE.

### R3.21. [P2] ndx_build: nrec*key_len key-buffer size and nrec+kpp-1 leaf count computed without overflow guards
- **bead:** initech-xiqz  **subsystem:** SAMIR/.ndx build (os/samir/fs/ndx.c)
- **evidence:** ndx.c:1320 `keys_bytes=nrec*(uint32_t)key_len` wraps (nrec=50,000,001, key_len=100 → 5e9 → ~705,032,804); alloc gets ~670MB, the loop at 1332 writes ~5GB, overrunning the arena. ndx.c:1347 `nleaf=(nrec+kpp-1u)/kpp` also wraps for nrec near UINT32_MAX (nleaf→0, forced to 1).
- **expected:** Check nrec<=UINT32_MAX/key_len and nrec<=UINT32_MAX-kpp+1 before the arithmetic; else -NDX_ERR_NOROOM.
- **actual:** Both multiplications wrap; undersized alloc → arena overrun, or a single-leaf root for billions of records.
- **repro:** ndx_build(nrec=50000001, key_len=100): keys_bytes wraps to 705,032,804; loop writes past the buffer.

### R3.22. [P2] memvar arena permanently exhausted by repeated C/M string-variable updates in DO WHILE loops
- **bead:** initech-frei  **subsystem:** SAMIR/flow (os/samir/cmd/flow.c)
- **evidence:** memvar_set (flow.c:308): for XB_C/XB_M, lines 343-357 always append to the bump arena (`dst=fs->arena+fs->arena_used; arena_used+=len`) without reclaiming the prior bytes at vars[slot].val.u.c.p when updating an existing var (found at 318). FLOW_MEMVAR_ARENA=16384 (55). RELEASE ALL resets arena_used (972) but RELEASE <name> (975-987) only marks live=0 without compaction.
- **expected:** Updating an existing C/M memvar reuses/reclaims old arena bytes; arena must not grow per-update.
- **actual:** Each STORE to an existing string var leaks arena bytes; -INTERP_ERR_NOMEM after ~ARENA/len updates.
- **repro:** STORE SPACE(200) TO BUF; loop STORE BUF+' ' TO BUF 100x → NOMEM before iteration 82.

### R3.23. [P2] scan_number: no 32-bit range check on integer literals — silent overflow (32-bit host) or unlocated NASM error (64-bit host)
- **bead:** initech-veav  **subsystem:** seed/lexer (seed/lexer.c, seed/codegen.c)
- **evidence:** lexer.c:217-228 `long value=0; while(is_digit) value=value*10+(...);` with no guard. On a 32-bit host `long` is 32-bit → signed overflow (UB) for literals >=2^31; codegen.c:200 emits `mov eax,%ld` with the wrong immediate. On 64-bit, values >0xFFFFFFFF are emitted and NASM rejects them in 32-bit mode with no seed source location. codegen.c:199-200 comment 'lexer/parser keep these in range' is false.
- **expected:** A decimal literal outside the Pascal integer range produces a located seed-compiler error at the token position.
- **actual:** 32-bit: wrong immediate emitted silently; 64-bit: deferred NASM 'value out of range' with no source location.
- **repro:** writeln(2147483649): 32-bit emits `mov eax,-2147483647`; 64-bit emits an immediate NASM rejects.

### R3.24. [P2] parse_var_section has no redeclaration check — duplicate var names emit duplicate NASM labels; codegen comment falsely claims the front end rejects them
- **bead:** initech-p0pa  **subsystem:** seed/parser + seed/codegen (seed/parser.c, seed/codegen.c)
- **evidence:** parser.c:327-346 pushes each AST_VARDECL with no duplicate check across decls or within a comma list. codegen.c:52-55 claims 'the front end already rejects redeclarations' (false); emit_bss (278-289) emits `v_<name>: resd 1` per name. `var x:integer; x:integer;` produces two `v_x: resd 1`; NASM errors 'symbol v_x defined twice' with no source line.
- **expected:** Duplicate variable declaration → located parser error ('variable x already declared').
- **actual:** Parser accepts silently; codegen emits duplicate labels; NASM fails without a Pascal source location.
- **repro:** `program P; var x:integer; x:integer; begin end.` exits 0; nasm rejects duplicate v_x.

### R3.25. [P2] Undeclared variable reference not detected — codegen emits an undefined NASM symbol with no source-level error
- **bead:** initech-ni34  **subsystem:** seed/parser + seed/codegen (seed/parser.c, seed/codegen.c)
- **evidence:** parser.c:124-129 (parse_factor) and 230-244 (parse_assignment) accept identifiers as AST_VARREF/AST_ASSIGN with no symbol-table lookup (none exists). codegen.c:204-207 emits `mov eax,[v_<name>]` and 255-258 `mov [v_<name>],eax` for any name; an undeclared name has no `v_<name>: resd 1` in .bss, so the link fails on undefined symbol with no Pascal location.
- **expected:** Read/assign of an undeclared variable → located error ('variable z not declared').
- **actual:** Codegen references [v_z]; link fails undefined v_z with no source position.
- **repro:** `program P; begin z:=1 end.` exits 0; nasm+ld fails on undefined v_z.

### R3.26. [P2] ANSI digit accumulator has no overflow cap — signed-int UB for large param values
- **bead:** initech-5y5r  **subsystem:** ANSI/console (os/milton/ansi.c)
- **evidence:** ansi.c:544 `st->param_current = st->param_current*10 + digit;` with param_current an int (ansi.h:169). ESC[3000000000A makes 3e9>INT_MAX → signed overflow UB. ANSI_PARAM_MAX(16) caps the param count, not magnitude (ansi.h:71-74).
- **expected:** Accumulation saturates/clamps (e.g. 9999) per MS-DOS 3.3 Tech Ref Ch4 (large params truncated to screen dimension).
- **actual:** Uncapped signed multiply wraps negative; CURSOR_REL delta becomes wrong/garbage with no later clamp.
- **repro:** Feed ESC[3000000000A; accumulator overflows before 'A'; delta_row is garbage.

### R3.27. [P2] Explicit param 0 for CUU/CUD/CUF/CUB not substituted with default 1 — ESC[0A moves 0 rows instead of 1
- **bead:** initech-mybc  **subsystem:** ANSI/console (os/milton/ansi.c)
- **evidence:** ansi.c:285/293/302/310 use ansi_param(st,0,1); ansi_param (123-130) substitutes the default only when v==ANSI_PARAM_DEFAULT(-1), so an explicit 0 returns 0. The CUP 'H' case (325-326) clamps r/c<1→1, but the four directional cases have no such guard. MS-DOS 3.3 Tech Ref Ch4: 'if n is 0 or omitted it defaults to 1.'
- **expected:** ESC[0A emits delta_row=-1 (same as ESC[A).
- **actual:** ESC[0A emits delta_row=0 — no movement.
- **repro:** Feed ESC[0A; CURSOR_REL delta_row==0 (expected -1).

### R3.28. [P2] SGR 8 (conceal) preserves the bright bit — concealed bright-fg text stays visible
- **bead:** initech-0u02  **subsystem:** ANSI/console (os/milton/ansi.c)
- **evidence:** ansi.c:218 `st->attr=(st->attr&0xF8u)|bg3;`. Mask 0xF8 keeps bit3 (bright) and clears only fg bits 2:0. bg3=(attr>>4)&0x07. attr=0x0F (bright white/black): 0x0F&0xF8=0x08 | bg3(0) = 0x08 = dark grey on black — visible. Comment at ansi.c:156 says 'set fg=bg so text is invisible' — contradicted.
- **expected:** Mask 0xF0 so the whole fg nibble (incl. bright) equals bg → truly invisible.
- **actual:** Bright-fg conceal renders the bright variant of the bg colour; only non-bright fg conceals correctly.
- **repro:** attr=0x0F then ESC[8m → SET_ATTR 0x08 (expected 0x00).

### R3.29. [P2] Relative cursor moves (CUU/CUD/CUF/CUB) do not update FSM row/col — DSR (ESC[6n) reports stale position
- **bead:** initech-s7cm  **subsystem:** ANSI/console (os/milton/ansi.c, os/milton/int21.c)
- **evidence:** ansi.c:284-311 cases A/B/C/D emit CURSOR_REL but never modify st->row/st->col; case 'n' (DSR, 397-398) carries st->row/st->col. Two contradictory comments: ansi.c:559-566 says 'apply relative deltas here so st->row/st->col stay coherent' (no code follows); 567-569 says 'relative actions leave it to the caller'. int21.c currently drops DEVICE_STATUS, so latent.
- **expected:** Either A/B/C/D update st->row/st->col (clamped), or the DSR wiring reads the live position.
- **actual:** st->row/st->col track only absolute escapes; ESC[3B then ESC[6n reports the pre-move row.
- **repro:** Feed ESC[3B then ESC[6n; DEVICE_STATUS action carries row=0 not 3.

### R3.30. [P2] region_op: CombineRgn(dst==src1,...) silently wrong — out reset before A_empty/B_empty are read
- **bead:** initech-bz8f  **subsystem:** FLAIR/region GDI facade (os/flair/atkinson/region.c)
- **evidence:** region.c:410-413 resets out (n_rows=0,is_empty=1) BEFORE 418-419 read A_empty/B_empty from A/B. When out==A, the reset zeroes A, so A_empty=1 regardless of A's content; RGN_OR then yields src2 alone, and RGN_COPY (self-union with dst==src1) early-returns empty. GDI documents dst may alias a source; no alias guard exists.
- **expected:** Compute A_empty/B_empty from original values before resetting out; CombineRgn(dst,dst,src2,OR) yields dst∪src2.
- **actual:** out==A path discards A's content; RGN_OR returns src2, RGN_COPY returns empty.
- **repro:** Non-empty dst; CombineRgn(&dst,&dst,&src2,OR) → dst==src2 (original lost).

### R3.31. [P2] fat12_grow_dir: locality hint fat12_find_free(cur+1) gives false NO_SPACE when all free clusters precede the directory tail
- **bead:** initech-fov8  **subsystem:** MILTON/FAT (os/milton/fat12.c)
- **evidence:** fat12.c:1610 `newc=fat12_find_free(vol,fat,fat_len,(uint16_t)(cur+1u))` where cur is the dir chain tail (1589-1607). fat12_find_free (1487-1506) searches `for(c=from;c<=last;c++)` and never wraps to FAT12_FIRST_DATA_CLUSTER=2. If free clusters are all below cur, it returns 0 → fat12_grow_dir returns NO_SPACE (1612-1614). fat12_mkdir (3355) correctly searches from cluster 2.
- **expected:** Fall back to searching from FAT12_FIRST_DATA_CLUSTER when the locality hint finds nothing.
- **actual:** Subdirectory growth fails with NO_SPACE on a non-full but fragmented volume.
- **repro:** Fill clusters 2..50, make subdir (tail ~75), delete the big file (frees 2..50), grow the full subdir → NO_SPACE despite free clusters.

### R3.32. [P2] spec/region_algebra.h: RGN_ROW_X_MAX=256 comment falsely claims it covers 640 inversion points
- **bead:** initech-m6c1  **subsystem:** FLAIR/region spec (spec/region_algebra.h)
- **evidence:** region_algebra.h:280-283 `/* 640px wide => 640 inversion points; this cap covers that with headroom */ #define RGN_ROW_X_MAX 256u`. 256<640. The locked spec propagates a false invariant that underpins the undersized scratch[RGN_ROW_X_MAX] at region.c:425 (the P0 overflow).
- **expected:** RGN_ROW_X_MAX>=640, or the comment states the true constraint and that the merge scratch must be 2*RGN_ROW_X_MAX.
- **actual:** 256 with a comment claiming 640-point coverage; arithmetically false.
- **repro:** Read region_algebra.h:280-283: 256<640.

### R3.33. [P2] FILE() oracle heresy: no-workarea test asserts the wrong #52 behavior as the pass condition
- **bead:** initech-u5mw  **subsystem:** SAMIR/oracle (harness/diff/dbf_diff/test_xbase_fn_d.c)
- **evidence:** test_xbase_fn_d.c:179-196 test_no_workarea iterates {RECNO,RECCOUNT,EOF,BOF,FOUND,DELETED,FIELD(1),DBF,FILE('X.DBF')} and asserts each returns XBEE_NO_DATABASE (line 190 good=(rc!=0 && ec==XBEE_NO_DATABASE)). FILE() is a PAL query that runs without an open table, so including it encodes the III+ violation as canon; fixing fn_file would turn this test RED.
- **expected:** Exclude FILE() from the #52 list; add a test asserting FILE() works without USE.
- **actual:** Oracle passes when FILE() returns #52 and blocks the correct fix.
- **repro:** Fix fn_file to use a PAL vtable; make test-xbase-fn-d goes RED on the FILE('X.DBF') assertion.

### R3.34. [P2] Oracle gap: ROUND oracle asserts only positive inputs — the negative (int64_t)-truncation bug is mutation-unproven (Rule 6)
- **bead:** initech-y78r  **subsystem:** SAMIR/oracle (harness/diff/dbf_diff/test_xbase_fn_b.c)
- **evidence:** test_xbase_fn_b.c:222-234 ROUND D-section covers only positive first args (3.14159, 3.145, 3.0, 1234, 1234/-2); lines 229-230 skip_gated the two tie cases. No negative non-tie assertion (ROUND(-2.7,0) etc.). A mutation swapping (int64_t) for floor leaves every D-section test passing.
- **expected:** Assert at least ROUND(-2.7,0)=-3 and ROUND(-1234,-2)=-1200 with a mutation that flips negative rounding direction.
- **actual:** No negative non-tie assertions; the fn_builtins.c:845-848 bug is undetectable by the suite.
- **repro:** Add ok_n('ROUND(-2.7,0)',-3.0,...) — it fails on the current code.

### R3.35. [P2] Oracle gap: CTOD oracle tests only out-of-range month/day, not calendar-impossible days within range
- **bead:** initech-kn6t  **subsystem:** SAMIR/oracle (harness/diff/dbf_diff/test_xbase_fn_a.c)
- **evidence:** test_xbase_fn_a.c:243 ok_d("CTOD('13/40/85')",0.0,...) uses mm=13, caught by the mm>12 branch — never reaches jdn_from_ymd. No test for a valid-range month with an impossible day (CTOD('02/30/85'), CTOD('04/31/85'), CTOD('02/29/85') non-leap). The missing per-month cap (fn_builtins.c:517) is mutation-unproven.
- **expected:** Assert CTOD('02/30/85'), CTOD('04/31/85'), CTOD('02/29/85') all return blank.
- **actual:** All asserted error cases fall into mm>12/dd>31; the rollover path survives the full oracle.
- **repro:** Add CTOD('02/30/85')=blank — it fails on the current code.

### R3.36. [P2] Oracle gap: C+C / C-C concatenation >254 bytes has zero coverage — the missing XB_STR_MAX guard is mutation-unproven (Rule 6)
- **bead:** initech-u468  **subsystem:** SAMIR/eval oracle (harness/diff/dbf_diff/test_xbase_eval.c)
- **evidence:** test_xbase_eval.c:204-231 concat tests use only short strings ('AB'+'CD'=4, 'AB  '-'CD'=6). No total exceeds 254; no XB_MUTATE hook exists for the absent size check in concat_plus (eval.c:148-163) / concat_minus (178-200). The SPACE/REPLICATE guards are mutation-proven but the concat path is not.
- **expected:** Assert SPACE(130)+SPACE(130) fails XBEE_SPACE_LARGE(#59) with a perturb proving the test bites.
- **actual:** Zero coverage of the 254-byte concat boundary.
- **repro:** Add the SPACE(130)+SPACE(130) assertion; it passes (bug: 260-byte success) until the guard is added.

### R3.37. [P2] S4.5 oracle gap: no duplicate-key leaf-split test and order_cb checks key order only (not (key,recno)) — ndx insert/delete bugs mutation-unproven
- **bead:** initech-68ox  **subsystem:** SAMIR/.ndx oracle (harness/diff/dbf_diff/test_ndx_maintain.c)
- **evidence:** test_ndx_maintain.c:429-436 (test_num_split) inserts kpp distinct keys + one more distinct key; no test builds >kpp records sharing one key. order_cb (123-141) flags only `ndx_key_cmp(prev,cur)>0` (strict key decrease); equal-key out-of-order recnos (cmp==0) never trip it. NDX_MUTATE_INSERT_NOSORT (23-28) bites within-leaf sort only, not cross-leaf ordering.
- **expected:** Add a non-unique >kpp same-key build/split/insert test; order_cb must flag `cmp==0 && recno<=prev_recno`.
- **actual:** Both ndx duplicate-key bugs pass the current oracle with exit 0.
- **repro:** ndx_build(32 key='K'); insert (K,33); run assert_inorder with (key,recno)-aware order_cb — fails on the unmodified code.

### R3.38. [P2] Oracle gap: test_lexer.c has no integer literal outside the 32-bit signed range — scan_number overflow path mutation-unproven
- **bead:** initech-cfc5  **subsystem:** seed/lexer oracle (seed/test_lexer.c)
- **evidence:** test_lexer.c:52-62 largest literal tested is 1000; no literal >=2^31. The `long value` accumulation overflow path (lexer.c:220-223) is unexercised; loosening accumulation fails no test.
- **expected:** A case asserting a literal >=2^31 produces TOK_ERROR, with a mutation that makes it fail.
- **actual:** No such test; the path is mutation-unproven (Rule 6).
- **repro:** make test-seed passes; perturbing lexer.c:221 to drop the digit add leaves test_lexer green.

### R3.39. [P2] Oracle gap: test_lexer.c never exercises scan_string pool near-capacity — the wrap-to-pool-start branch and false-rejection bug are mutation-unproven
- **bead:** initech-99p1  **subsystem:** seed/lexer oracle (seed/test_lexer.c)
- **evidence:** test_lexer.c:103-123 uses only short string literals; none fill the pool near LEXER_STRBUF_CAP before lexing another string. The wrap branch lexer.c:268-278 (`strpos!=0 && out==0`) is never reached; removing lines 271-274 fails no test.
- **expected:** Sequence strings to push strpos near CAP, then assert a subsequent short string returns successfully.
- **actual:** No such test; the wrap/near-capacity path is unproven.
- **repro:** Comment out lexer.c:271-274; make test-seed still passes.

### R3.40. [P2] Oracle gap: test_parser.c has no duplicate variable declaration test — the missing redeclaration check is mutation-unproven
- **bead:** initech-mqwr  **subsystem:** seed/parser oracle (seed/test_parser.c)
- **evidence:** test_parser.c:39-210 has no `var x:integer; x:integer;` or `var x,x:integer;` case. The absent redeclaration guard in parse_var_section (parser.c:327-346) is undetected; adding then deleting a check would not be caught.
- **expected:** A test feeding a duplicate decl and asserting rc!=0 with the var name in the message.
- **actual:** No such test; duplicates pass silently.
- **repro:** make test-seed passes with `var x:integer; x:integer;` returning rc==0.

### R3.41. [P2] Oracle gap: test_ansi.c cursor-move suite never tests explicit param 0 — ESC[0A bug mutation-unproven
- **bead:** initech-ng8n  **subsystem:** ANSI/console oracle (os/milton/test_ansi.c)
- **evidence:** test_ansi.c:301-366 (test_cursor_up) tests ESC[A(-1), ESC[3A(-3), ESC[15A(-15) but never ESC[0A; same gap for 0B/0C/0D. The explicit-0 path (ansi.c:285/293/302/310) is unexercised.
- **expected:** An ESC[0A case asserting delta_row==-1 (RED on current code, GREEN after the ansi_param `v<=0→def` fix).
- **actual:** Explicit-zero path for CUU/CUD/CUF/CUB is untested.
- **repro:** Add ESC[0A test; it fails against the current ansi.c.

### R3.42. [P2] Oracle gap: test_ansi.c SGR 8 conceal suite tests only non-bright fg — bright-fg conceal bug mutation-unproven
- **bead:** initech-i427  **subsystem:** ANSI/console oracle (os/milton/test_ansi.c)
- **evidence:** test_ansi.c:639-676 sets attr with no bright bit (ESC[37;42m→0x27), reverses, then conceals asserting 0x77 (correct only because bright=0). No ESC[8m test with bit3 set; the 0xF8-vs-0xF0 distinction (ansi.c:218) is never exercised.
- **expected:** A sequence ESC[1;32;40m (0x0A) → ESC[8m expecting 0x00; current code yields 0x08.
- **actual:** Bright-fg conceal path unproven; the never-cleared bright bit passes the full suite.
- **repro:** Extend test_sgr_reverse_and_conceal: attr 0x0F, ESC[8m, assert 0x00 — RED against ansi.c:218.

### R3.43. [P2] Oracle gap: test_ansi_wire.c never drives ESC[A/B/C/D through the wiring — the cursor_rel dispatch in ansi_apply is mutation-unproven
- **bead:** initech-dzum  **subsystem:** ANSI/console wiring oracle (os/milton/test_ansi_wire.c)
- **evidence:** test_ansi_wire.c:184 registers ops.cursor_rel=mock_cursor_rel; the main body (252-347) emits only ESC[2J, ESC[5;10H, ESC[31;1m, ESC[s/u — never an A/B/C/D. mock_cursor_rel (126-130) is never invoked; int21.c:570-574 ANSI_ACT_CURSOR_REL dispatch is dead code in every oracle run.
- **expected:** A test emitting ESC[3B asserting the mock moved down 3.
- **actual:** The CURSOR_REL action path and mock are never exercised; deleting the dispatch leaves all tests green.
- **repro:** Add emit_str("[3B"); CHECK(g_mock.row==3) after block 3.

### R3.44. [P2] Oracle gap: test_region.c MAX_ITEMS=6 never approaches RGN_ROW_X_MAX row density — the xmerge scratch overflow is mutation-unproven (Rule 6)
- **bead:** initech-ccv9  **subsystem:** FLAIR/region oracle (harness/proptest/test_region.c)
- **evidence:** test_region.c:158 `enum { MAX_ITEMS=6 };` caps each region to ~12 inversion points per row, far below 256. The homomorphism suite (458-479, 4000 cases/op) never produces na+nb>256, so the xmerge overflow at region.c:393-395 is never reached and no mutant exercises the `no>=cap` path.
- **expected:** A generator producing rows approaching RGN_ROW_X_MAX entries; a mutant truncating the merge should go RED.
- **actual:** MAX_ITEMS=6 keeps the overflow invisible to the oracle.
- **repro:** Raise MAX_ITEMS to 130+; the generator produces 256+ entries/row and xmerge overruns scratch.

