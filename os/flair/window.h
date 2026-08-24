/*
 * os/flair/window.h -- the FLAIR Window Manager (THE ARTIFACT).
 *
 * beads: initech-9qf ("Window Manager: z-order, drag, update regions"). The
 *        load-bearing M3/M4 logic behind the live, draggable desktop: the
 *        z-ordered window list, FindWindow part-code hit-testing, the visible-
 *        region overlap computation, and the region-difference DAMAGE model that
 *        drives minimal repaint (ADR-0004 D-5). Depends on the ATKINSON region
 *        engine (initech-b5g) and the locked WindowRecord (spec/window_record.h).
 *        Blocks the Control / Menu / Dialog Managers and the desktop shell.
 *
 * WHAT THIS IS: the Manager (FLAIR Layer 3, ADR-0004 D-1/D-3) that owns the
 * window list and maintains, for every window, the three regions the rest of the
 * Toolbox rides on:
 *
 *   strucRgn  -- the whole WDEF structure (frame/chrome/content plus the exact
 *                document shadow band where the variant has one).
 *   contRgn   -- the content area (inside the chrome).
 *   updateRgn -- the damaged area still owing a repaint (the DiffRgn payoff).
 *
 * and computes, on demand, the VISIBLE region of each window:
 *
 *   visible(W) = strucRgn(W)  DIFF  ( UNION of strucRgn(F) for every F in front )
 *
 * via the ATKINSON algebra (spec/region_algebra.h: region_op DIFF/UNION). This
 * is the core overlap computation -- "the portion of the window not obscured by
 * windows in front of it" (MTE p. 4-15; ADR-0004 D-5).
 *
 * THE DAMAGE MODEL (ADR-0004 D-5, the decisive logic; PRD Sec 6.2):
 *   When a window MOVES, is DISPOSED, or changes z-order, the newly-EXPOSED area
 *   of every window behind it is computed by REGION DIFFERENCE:
 *       newly-exposed(W) = (W covered before)  DIFF  (W covered now)
 *   and ACCUMULATED into W's updateRgn. The desktop background is damaged where
 *   the moved window's old structure no longer overlaps its new structure. NO
 *   window's updateRgn ever exceeds its actual damage -- no over-repaint (the
 *   authentic minimal-update behaviour; D-5, Rationale 4.3).
 *
 * NO MALLOC (Law 3, freestanding; ADR-0004 DEC-03 FLAIR heap): the Window
 * Manager owns no storage. Like the region engine, EVERY region is caller-
 * supplied from the FLAIR arena: NewWindow takes the WindowRecord and its three
 * region_t's (strucRgn / contRgn / updateRgn), while WindowMgr owns three
 * scratch regions, all with their
 * rows[]/x_pool pools already attached. A cap overflow inside the algebra FAILS
 * LOUD in the engine (Rule 2). The desktop's damage is delivered through a
 * caller-supplied desktop updateRgn the same way.
 *
 * DUAL-COMPILE (the region.c / blitter.c / surface.c pattern; Law 3): window.c
 * compiles BOTH freestanding for the kernel (gcc -m32 -ffreestanding -nostdlib
 * -std=c11 -Wall -Wextra -Werror) AND hosted for the property suite
 * (harness/proptest/test_window.c). It uses only <stdint.h> + <stddef.h> + the
 * locked headers; no host malloc; no libc.
 *
 * Ref: PRD Sec 6.2 (region algebra -- the load-bearing math), Sec 6.3 (Toolbox).
 *      ADR-0004 D-1 (5-layer stack), D-3 (Window Manager: WindowRecord +
 *      FindWindow part-codes), D-5 (region-difference damage model).
 *      spec/window_record.h (the LOCKED WindowRecord + part-codes),
 *      spec/grafport.h (embedded GrafPort + visRgn), spec/region_algebra.h
 *      (region_op UNION/DIFF, region_set_empty, region_contains_point),
 *      spec/chrome_metrics.json (title-bar / box geometry -- via window.c).
 *      CLAUDE.md Law 2 (oracle is truth), Law 3 (freestanding dual-compile),
 *      Rule 2 (fail loud), Rule 3 (all bugs deep), Rule 11 (deterministic),
 *      Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_WINDOW_H
#define INITECH_OS_FLAIR_WINDOW_H

#include <stdint.h>
#include <stddef.h>

#include "region_algebra.h"   /* region_t, rgn_rect_t, region_op (-Ispec)        */
#include "window_record.h"    /* WindowRecord, WindowPtr, flair_part_code_t      */

/* ===========================================================================
 * 1. THE WINDOW MANAGER STATE
 * ---------------------------------------------------------------------------
 * The Window Manager owns NO storage of its own beyond this tiny descriptor:
 * a pointer to the FRONT of the z-order list (lowest z = top of screen) and a
 * scratch region (caller-supplied) it uses to accumulate "windows in front"
 * unions during the visible-region / damage computations. Each WindowRecord is
 * caller-allocated (FLAIR arena) and threaded through `nextWindow` (front at the
 * head, back at the tail; spec/window_record.h INVARIANTS).
 *
 * The desktop background is modeled as a caller-supplied `desktop_frame` rect
 * (typically the full screen) plus a caller-supplied `desktop_update` region
 * that ACCUMULATES the background's damage exactly like a window's updateRgn.
 * ===========================================================================*/
typedef struct WindowMgr {
    WindowPtr   front;          /* head of the z-order list (frontmost window)   */
    rgn_rect_t  desktop_frame;  /* the desktop bounds (background extent)        */
    region_t   *desktop_update; /* desktop background damage (accumulated)       */

    /* THREE caller-supplied scratch regions the algebra writes into. region_op
     * requires `out` distinct from BOTH inputs, so a chained computation
     * (fronts-union -> diff -> clip -> distribute) needs three rotating
     * temporaries so no engine op ever aliases its output with an input. They
     * are NOT any window's strucRgn/contRgn/updateRgn. Attached (rows[]/x_pool)
     * by the caller from the FLAIR arena. */
    region_t   *scratch_a;      /* general scratch (fronts-union)                */
    region_t   *scratch_b;      /* general scratch (the live "exposed" carrier)  */
    region_t   *scratch_c;      /* general scratch (per-step distinct out)       */

    /* ALWAYS-ON-TOP OVERLAY (beads initech-pipa; ADR-0005 region spine). An
     * OPTIONAL caller-supplied region of pixels that are drawn OVER every window
     * by a layer OUTSIDE the WindowMgr z-order -- the desktop shell's two menu
     * bars (rows [0,40)) and, when up, the modal FILE COPY dialog. fronts_union
     * folds it in so every visible-region / damage computation treats the overlay
     * as occluding: a dragged window never overpaints it (defect b) AND its pixels
     * are never classified as newly-exposed desktop, so desktop_update never
     * seafoam-fills over it (defect a). Both defects close in the ONE shared
     * occlusion primitive every paint path routes through. NULL == no overlay (the
     * host harnesses + any bare desktop): the fold is then a no-op. Caller-supplied,
     * arena-backed, rows[]/x_pool attached; the WindowMgr owns no storage (Law 3).
     *
     * WINDOW_MUTATE_IGNORE_OVERLAY (Rule 6): window.c compiles the fold out so
     * fronts_union ignores a SET overlay_rgn -- reproducing the pre-fix compositor
     * (windows overpaint / desktop seafoam-erases the bars + modal). The EMU
     * survival oracle (initech-dc4v, flair_live_mut_overlay.img) goes RED. NEVER
     * define in a real build. */
    region_t   *overlay_rgn;    /* always-on-top occluder (bars + modal); may be NULL */
} WindowMgr;

/* ===========================================================================
 * 2. LIFECYCLE
 * ---------------------------------------------------------------------------
 * WindowMgr_init binds the manager to its scratch + desktop regions and an empty
 * window list. NewWindow inserts a caller-supplied WindowRecord (with its three
 * region_t's already attached) at the FRONT; DisposeWindow removes it and damages
 * what it exposes. SelectWindow brings to front + (de)activates; Show/Hide toggle
 * visibility and damage accordingly.
 * ===========================================================================*/

/* Initialize the manager: empty list, the given desktop frame + the three
 * scratch regions + the desktop-update region (all caller-supplied, arena-
 * backed). The desktop-update and scratch regions are set empty. FAIL-LOUD on a
 * NULL manager, a NULL scratch, or an unattached region (Rule 2). */
void WindowMgr_init(WindowMgr *wm, rgn_rect_t desktop_frame,
                    region_t *desktop_update,
                    region_t *scratch_a, region_t *scratch_b,
                    region_t *scratch_c);

/* NewWindow -- install `w` at the FRONT of the z-order list and activate it.
 *
 *   bounds    -- the window's outer FRAME rectangle in global coords. For a
 *                document WDEF, strucRgn also includes the sampled (+1,+1)
 *                shadow band outside this rectangle.
 *   content   -- the content rectangle (inside the chrome) in global coords;
 *                must be within `bounds`. (window.c computes the chrome bands
 *                between them for FindWindow; see Section 4.)
 *   wKind     -- windowKind (documentKind / dialogKind; spec/window_record.h).
 *   wVariant  -- windowDefProcVariant (documentProc / dBoxProc / ...).
 *   goAway    -- 1 if the window has a close box.
 *
 * `w` and its three regions (strucRgn/contRgn/updateRgn) must be caller-supplied
 * with pools attached (the region engine never mallocs). strucRgn := the WDEF
 * structure (frame plus document shadow), contRgn := content, updateRgn :=
 * empty on entry. The previously-front window
 * is deactivated (hilited=0, deactivation repaint seeded); `w` becomes active
 * (hilited=1) and its first full paint is SEEDED by the activation transition
 * (initech-rqz5 / DQ3): on return `w`'s updateRgn holds its visible region --
 * a compositor/route pass (or an explicit from-scratch paint + validate)
 * services it. The newly-covered area of the windows behind `w` is NOT damaged
 * (covering exposes nothing). */
void NewWindow(WindowMgr *wm, WindowPtr w, rgn_rect_t bounds, rgn_rect_t content,
               int16_t wKind, int16_t wVariant, uint8_t goAway);

/* CalcDocContentRect -- the single document-WDEF content derivation seam.
 * The title boundary is frame.top+22 exactly (TITLEBAR_H already includes both
 * frame rows); the right edge stops at the actual outer line of the 16px
 * vertical scrollbar, after the four-pixel body rail. Ref: sys8/window-
 * chrome.md Sec 2.1/Sec 4 and scrollbars.md Sec 1; beads initech-javs/l0mh. */
rgn_rect_t CalcDocContentRect(rgn_rect_t frame);

/* NewDocumentWindow -- CalcDocContentRect + NewWindow(zoomDocProc). Production
 * Platinum documents carry close/zoom/collapse/grow through one record flag set;
 * callers use this WDEF seam instead of repeating chrome formulas. */
void NewDocumentWindow(WindowMgr *wm, WindowPtr w, rgn_rect_t frame,
                       int16_t wKind, uint8_t goAway);

/* WindowFrameRect -- recover the drawer/hit-test frame from strucRgn. A
 * document strucRgn includes its shadow; the frame does not. */
rgn_rect_t WindowFrameRect(const WindowPtr w);

/* SetWTitle -- the ONE write seam for the kernel-owned inline title buffer.
 * Copies an ASCIZ title into WindowRecord.titleHandle, always terminates it,
 * updates titleWidth from the Chicago strike metrics, and invalidates the
 * title band when `wm` is non-NULL.  A NULL title means "".  `wm` may be NULL
 * only while constructing an unlinked standalone record (Dialog Manager host
 * use); linked/live callers pass the owning manager so a future
 * FLAIR_SETWTITLE trap targets this exact storage + repaint seam.
 *
 * Ref: docs/design/GUI-remediation-D1-D2-D3-design.md D2-3; PRD Sec 6.4;
 * spec/window_record.h titleHandle/titleWidth. */
void SetWTitle(WindowMgr *wm, WindowPtr w, const char *title);

/* DisposeWindow -- remove `w` from the list and DAMAGE everything it exposes:
 * for every window that was (partly) under `w`, accumulate the area `w`'s
 * structure covered (intersected with that window's now-visible region) into
 * its updateRgn; the desktop background is damaged where `w`'s structure no
 * longer overlaps any remaining window. The front window after removal is
 * activated. */
void DisposeWindow(WindowMgr *wm, WindowPtr w);

/* SelectWindow -- bring `w` to the FRONT of the z-order and make it active.
 * Windows that `w` newly covers are unaffected (covering damages nothing); the
 * windows `w` USED to be behind are unchanged. The previously-front window is
 * deactivated (repaint seeded); `w` is activated and its OWN repaint -- the
 * previously-covered part plus the active-chrome flip -- is seeded by the 0->1
 * activation transition (initech-rqz5 / DQ3). If `w` was already front, this
 * is a no-op beyond re-affirming activation. (Bringing a window forward never
 * EXPOSES a window -- exposure happens on hide/dispose/move, not on raise.) */
void SelectWindow(WindowMgr *wm, WindowPtr w);

/* ShowWindow -- make a hidden window visible (it was excluded from the z-order
 * paint). Covering windows behind it are unaffected; `w` itself needs a full
 * paint. If the show ACTIVATES `w` (front-most visible), the 0->1 transition
 * seeds it (initech-rqz5 / DQ3); a re-shown BACKGROUND window is still the
 * caller's seed (WindowMgr_invalidate) -- no hilited transition fires. */
void ShowWindow(WindowMgr *wm, WindowPtr w);

/* HideWindow -- make `w` invisible. Everything `w` was covering is EXPOSED:
 * damage is accumulated exactly as in DisposeWindow (windows behind get the
 * newly-exposed area; the desktop gets the rest). `w` stays in the list (so it
 * can be shown again) but `visible` becomes 0. */
void HideWindow(WindowMgr *wm, WindowPtr w);

/* ===========================================================================
 * 3. MOVE / DRAG -- the damage payoff (ADR-0004 D-5)
 * ---------------------------------------------------------------------------
 * MoveWindow moves `w`'s structure (and content) by (dh, dv) -- or to an absolute
 * position via the rect form -- recomputes its regions, and computes the DAMAGE:
 *
 *   For the moved window itself: it must repaint the part of its new structure
 *   that was NOT visible before (it was off-window or covered) -- newly-exposed
 *   self-damage -- which the caller drives by seeding updateRgn; we focus the
 *   damage on the OTHER windows + desktop, which is the load-bearing oracle.
 *
 *   For every OTHER window behind `w`:
 *       newly-exposed(W) = ( W intersect w_old_struc )  DIFF  ( w_new_struc )
 *     i.e. the area W lost to w's OLD position that w's NEW position no longer
 *     covers, clipped to W (and to W's own front-occlusion is handled by the
 *     fact that we only ever expose pixels that were under w_old). Accumulated
 *     into W.updateRgn.
 *
 *   For the desktop background:
 *       desktop damage += ( w_old_struc DIFF w_new_struc ) DIFF (every other
 *       window's structure) -- the bare-desktop pixels w used to cover and now
 *       does not, that no other window covers.
 *
 * The UNION of all accumulated updateRgns (windows + desktop) EXACTLY equals the
 * symmetric region "covered by w_old but not by w_new" (restricted to the
 * desktop frame) -- the test_window oracle verifies this bit-exactly. No window
 * is told to repaint a pixel that did not actually change owner (no over-repaint,
 * D-5 / Rationale 4.3).
 * ===========================================================================*/

/* MoveWindow -- move `w` so its structure top-left becomes (newLeft, newTop),
 * keeping its size. Recomputes strucRgn/contRgn and accumulates the exposure
 * damage of the move into every affected window's updateRgn and the desktop
 * update region (Section 3 model). */
void MoveWindow(WindowMgr *wm, WindowPtr w, int16_t newLeft, int16_t newTop);

/* DragWindow -- alias for MoveWindow expressed as a delta (dh, dv) from the
 * window's current position; the interactive drag loop calls this each step. */
void DragWindow(WindowMgr *wm, WindowPtr w, int16_t dh, int16_t dv);

/* ConstrainWindowSize -- shared grow-track/commit policy. Width/height are
 * clamped to the locked 96x64 minimum and the part of the native desktop still
 * available from the window origin. The live outline and SizeWindow commit both
 * call this seam, so feedback cannot disagree with the final frame. */
void ConstrainWindowSize(const WindowMgr *wm, const WindowPtr w,
                         int16_t *width, int16_t *height);

/* SizeWindow -- resize at the current top-left, recompute document structure +
 * content (including scrollbar/grow geometry), distribute exact old exposure,
 * and seed the resized window's new visible footprint. */
void SizeWindow(WindowMgr *wm, WindowPtr w, int16_t width, int16_t height);

/* ZoomWindow -- toggle exact userState <-> deterministic standard state below
 * the two menu bands. Returns nonzero in standard/zoomed state after the op. */
int ZoomWindow(WindowMgr *wm, WindowPtr w);

/* CollapseWindow -- toggle expanded frame <-> the exact 22px title band.
 * contRgn is empty while collapsed. Returns nonzero when collapsed. */
int CollapseWindow(WindowMgr *wm, WindowPtr w);

/* ===========================================================================
 * 4. FINDWINDOW -- hit-testing (verbatim Inside Macintosh part-codes)
 * ---------------------------------------------------------------------------
 * FindWindow walks the z-order FRONT-to-BACK, hit-testing each VISIBLE window's
 * structure against the point. The first window whose strucRgn contains the
 * point wins (it is the front-most window there). The part-code is then resolved
 * by which sub-band of the chrome the point falls in (spec/window_record.h
 * Section 1; MTE Table 4-2):
 *
 *   point in contRgn ......................... inContent
 *   point in title-bar band (struc above cont) inDrag
 *   point in the go-away (close) box .......... inGoAway   (if goAwayFlag)
 *   point in the zoom box ..................... inZoomIn/Out (if zoom flag)
 *   point in the collapse box ................. inCollapse  (if collapse flag)
 *   point in the grow box (bottom-right) ...... inGrow      (if a grow variant)
 *   any other chrome pixel .................... inDrag      (frame is draggable)
 *   no window contains the point .............. inDesk      (whichWindow := NULL)
 *
 * Returns the part-code; *whichWindow receives the hit window (or NULL for
 * inDesk). The chrome sub-bands are derived from WindowFrameRect/content using
 * spec/chrome_metrics geometry (close/zoom box sizes, grow box size).
 * ===========================================================================*/
flair_part_code_t FindWindow(const WindowMgr *wm, flair_point_t pt,
                             WindowPtr *whichWindow);

/* ===========================================================================
 * 5. VISIBLE REGION + INVALIDATE (helpers the Managers / shell call)
 * ---------------------------------------------------------------------------
 * ComputeVisible writes W's visible region (strucRgn DIFF union-of-fronts) into
 * the caller-supplied `out` region. The desktop shell installs this into each
 * GrafPort's visRgn before the app draws (ADR-0004 D-1/D-2). It is the SAME
 * computation the damage model uses internally; exposed here so the oracle (and
 * the shell) can read it without a move.
 * ===========================================================================*/

/* Compute W's visible region into `out` (out must be a distinct, attached
 * caller region). out := strucRgn(W) DIFF ( union of strucRgn of all VISIBLE
 * windows in front of W ). If W is not visible, out := empty. */
void ComputeVisible(const WindowMgr *wm, const WindowPtr w, region_t *out);

/* WindowMgr_clip_update_to_visible -- BeginUpdate-fidelity delivery clip.
 * Replaces W's pending updateRgn with updateRgn INTERSECT W's CURRENT visible
 * region, then fails loud if any post-intersect pixel lies outside visibility.
 * This is intentionally a delivery-time verb: a seed that was visible when
 * queued can become covered after a later z-order operation. Uses only the
 * manager's three scratch regions; no caller scratch or allocation is needed.
 * Ref: ADR-0013 Sec 3.3 updateEvt contract; PRD Sec 6.2; bead initech-wlzp. */
void WindowMgr_clip_update_to_visible(WindowMgr *wm, WindowPtr w);

/* WindowMgr_invalidate -- accumulate `rect` (clipped to W's visible region) into
 * W's updateRgn. The shell / an app calls this to request a repaint of a sub-area
 * (e.g. the whole content on first show). Uses the manager scratch. */
void WindowMgr_invalidate(WindowMgr *wm, WindowPtr w, rgn_rect_t rect);

/* WindowMgr_invalidate_desktop -- invalidate an arbitrary screen rectangle
 * painted above the WindowMgr composition (for example a dropped menu panel).
 * The rectangle is clipped to desktop_frame, then every pixel is assigned to
 * its frontmost visible window's updateRgn or, if no window owns it, to
 * desktop_update. The subsequent DQ2 cycle (desktop_paint_damage -> owner
 * content route -> present) therefore reconstructs the exact composited scene;
 * this is not a bare-background fill and must not be replaced by shell_render.
 * Uses the manager scratch regions. Ref: beads initech-b3hl/-j0vt. */
void WindowMgr_invalidate_desktop(WindowMgr *wm, rgn_rect_t rect);

/* BeginUpdate / EndUpdate analogue: clear W's updateRgn after the app has
 * repainted it (the pump's EndUpdate). Sets updateRgn := empty. */
void WindowMgr_validate(WindowPtr w);

/* WindowMgr_window_index -- zero-based position in the REAL WindowMgr list,
 * including hidden windows (HideWindow preserves list membership).  Returns
 * -1 for NULL or an unlinked/disposed record.  Indices are deterministic for
 * a given list: hide keeps them stable; dispose compacts the suffix; select
 * reflects the new z-order.
 *
 * Live serial ABI consumer (os/milton/kmain.c):
 *   FLAIR-DRAG win <id> (<old-left>,<old-top>)->(<new-left>,<new-top>)
 *   FLAIR-CLOSE win <id>
 * The shell's legacy store IDs remain primary; tenant records missing from
 * that store use this real-list index, so a linked tenant never reports -1.
 * Ref: beads initech-883x; window.h list invariant above. */
int WindowMgr_window_index(const WindowMgr *wm, const WindowPtr w);

#endif /* INITECH_OS_FLAIR_WINDOW_H */
