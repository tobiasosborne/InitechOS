/*
 * os/flair/chrome.h -- the FLAIR System-7 window chrome drawer (THE ARTIFACT).
 *
 * beads: initech-k8o5.8 (first rendered System-7 window chrome + test-chrome).
 *
 * THE ARTIFACT (Law 3): this is freestanding C that will run in the kernel. It
 * draws one System-7 documentProc window's chrome -- pinstripe title bar, close
 * box, zoom box, the 1 px double-line frame, the content area, and a 16 px
 * vertical scrollbar -- into a GrafPort, using ONLY:
 *   - the LOCKED native metrics in spec/chrome_metrics.h,
 *   - the ONE surface module (os/flair/surface.h) for pixel writes, and
 *   - the ATKINSON regions (visRgn INTERSECT clipRgn) for clipping (D-1/D-2).
 * NO libc, NO direct framebuffer poking outside the surface module, NO 2026-isms.
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall
 * -Wextra -Werror) AND hosted (cc -std=c11, for the test-chrome oracle via the
 * host render skeleton harness/render). It does NO allocation -- it draws into a
 * caller-supplied GrafPort whose portBits/visRgn/clipRgn are already set up.
 *
 * Ref: spec/chrome_metrics.json (LOCKED v1) + spec/chrome_metrics.h (its C form);
 *      ADR-0004 D-1/D-2 (all drawing through a GrafPort clipped by
 *      visRgn INTERSECT clipRgn; one surface module); D-3 (verbatim Mac chrome);
 *      docs/research/gui-ground-truth.md Sec 3.3/4.2 (the chimera element map:
 *      the title bar, close/zoom box, scrollbar, frame are mac-system7 chrome);
 *      StandardWDEF.a (the WDEF constants the metrics encode -- see chrome_metrics.h).
 *      CLAUDE.md Law 1 (ground truth), Law 2 (oracle is truth), Law 4 (look like
 *      the frame), Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_OS_FLAIR_CHROME_H
#define INITECH_OS_FLAIR_CHROME_H

#include "grafport.h"           /* GrafPort (-Ispec)                          */
#include "region_algebra.h"     /* rgn_rect_t (-Ispec)                        */

/* ---------------------------------------------------------------------------
 * flair_draw_document_window -- draw one System-7 documentProc window's chrome.
 *
 * Draws into the GrafPort `port` (the current port), with the window occupying
 * the rectangle `frame` (in port-local coordinates: top,left,bottom,right). The
 * chrome is composed top-to-bottom:
 *
 *   - the 1 px window frame (outer black line) + 1 px inner groove,
 *   - the pinstripe title bar (FLAIR_CHROME_TITLEBAR_H tall, alternating light/
 *     dark scanlines at FLAIR_CHROME_PINSTRIPE_PERIOD using shade indices
 *     LIGHT/DARK),
 *   - a hollow close box at the top-left and a hollow zoom box at the top-right
 *     (each ~FLAIR_CHROME_WBOX_DELTA geometry),
 *   - the white content area,
 *   - a FLAIR_CHROME_SCROLLBAR_W-wide vertical scrollbar on the right (up/down
 *     arrow buttons + a pattern track).
 *
 * EVERY pixel is written through the surface module and clipped to
 * visRgn INTERSECT clipRgn (D-1/D-2). The window must fit inside the port; a
 * window smaller than the chrome it must hold is a no-op (fail-soft, the caller
 * is responsible for a sane frame -- the Window Manager sizes it).
 *
 * `title` is the window's name (the WindowRecord titleHandle), drawn CENTERED in
 * the title bar in Chicago over a knocked-out light gap (System 7; beads
 * initech-lxg9). A NULL or empty title draws no text. The ink + knockout resolve
 * through the C-8 policy seam (flair_look_pixel), never a color literal.
 *
 * `hilited` is the WindowRecord's wHilited flag (spec/window_record.h
 * WindowRecord.hilited; StandardWDEF_a.txt DrawTitleBar, L679-744): non-zero for
 * the frontmost/active window, zero for a background window. The WDEF branches
 * the title-bar INTERIOR on this flag (../system7-decomp/specs/chrome/title-bar.md
 * Sec 2.1 "Active vs inactive (the hilite split)"): hilited!=0 draws the
 * pinstripe + 3-D bevel exactly as before (byte-identical); hilited==0 draws a
 * FLAT solid interior (FLAIR_PART_CONTENT white; no pinstripe, no bevel rows) --
 * the StandardWDEF inactive appearance. The close/zoom gadgets and title text
 * are still drawn in both states (the flat backdrop alone de-emphasizes them
 * relative to the racing-stripe active state). Ref: beads initech-a9iq.
 *
 * SCOPE (Law 1 honesty): this fix covers the title-bar INTERIOR fill only. The
 * decomp ground truth (title-bar.md Sec 2.1) also documents an inactive FRAME
 * line recolor (wHiliteShadeA, gray, vs black) and a dimmed title-ink shade
 * (wHiliteShade7) that this drawer does NOT yet reproduce -- both are
 * candidate follow-up fidelity items, not graded by the initech-a9iq oracle
 * leg. close-zoom-box.md additionally documents the gadgets being fully
 * ABSENT (not merely dimmed) on a real System-7 inactive title bar; this
 * drawer keeps them present per the bd initech-a9iq EXPECTED text
 * ("de-emphasized gadgets"), a deliberate, narrower scope than strict
 * System-7 accuracy -- also a follow-up fidelity candidate.
 * ------------------------------------------------------------------------- */
void flair_draw_document_window(GrafPort *port, rgn_rect_t frame,
                                const char *title, int hilited);

/* ---------------------------------------------------------------------------
 * flair_draw_movable_dbox_chrome -- draw one System-7 movableDBoxProc (5)
 * window's chrome: a moveable TITLED modal dialog box (beads initech-zvo6).
 *
 * Draws into `port` over the rectangle `frame`, composed of:
 *   - the SAME title-bar band flair_draw_document_window draws (pinstripe +
 *     bevel + shared frame line + centered Chicago `title`), reusing the
 *     shared internal composer -- NOT a hand-rolled second chrome path;
 *   - a PLAIN 1 px frame around the whole window (FLAIR_CHROME_FRAME).
 *
 * Deliberately OMITS (per MTE Table 4-1 / the movableDBoxProc variant, which
 * forces these off): close box, zoom box, grow box, scrollbar, drop shadow.
 * The caller is responsible for the dialog's own content fill; this function
 * draws ONLY the band + the outer frame (mirrors DrawDialog's existing
 * dBoxProc division of labor: caller fills content, this draws the border).
 *
 * `title` is drawn ALWAYS-HILITED (this variant's window is always the
 * frontmost overlay when drawn -- ADR-0004 D-5 -- so there is no inactive
 * state). A NULL or empty title draws no text (same contract as
 * flair_draw_document_window).
 *
 * Ref: ../system7-decomp/specs/toolbox/window-manager.md line 173
 *      ("movableDBoxProc | 5 | movable modal dialog box (title bar, no
 *      grow/zoom)"); ../system7-decomp/specs/chrome/wdef-variant-geometry.md
 *      Sec 1 (variant catalog) + Sec 4 (the RENDERED s7_about.png golden:
 *      plain 1px frame + documentProc-identical title band, no close/zoom/
 *      grow); spec/window_record.h (movableDBoxProc = 5); CLAUDE.md Law 1/2/4.
 * ------------------------------------------------------------------------- */
void flair_draw_movable_dbox_chrome(GrafPort *port, rgn_rect_t frame,
                                    const char *title);

#endif /* INITECH_OS_FLAIR_CHROME_H */
