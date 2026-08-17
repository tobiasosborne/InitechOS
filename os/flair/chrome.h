/*
 * os/flair/chrome.h -- the FLAIR Mac OS 8 Platinum chrome drawer (ARTIFACT).
 *
 * beads: initech-k8o5.8 (original heritage renderer); initech-i3si (DEC-10
 *        Platinum render re-key).
 *
 * THE ARTIFACT (Law 3): this is freestanding C that will run in the kernel. It
 * draws one Mac OS 8 Platinum document window -- 22-row title band, three
 * widgets, raised body bar, content inset, notched shadow, scrollbar, and grow
 * box -- into a GrafPort, using ONLY:
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
 * ERA AXIS: Mac OS 8 Platinum (DEC-10) is the BASE; System 7 remains retained
 * heritage under BC-10.10. The mechanism contains no era branch and all color
 * roles resolve through flair_look (C-8).
 *
 * Ref: spec/chrome_metrics.json + spec/chrome_metrics.h (its C form);
 *      ADR-0004 D-1/D-2 (all drawing through a GrafPort clipped by
 *      visRgn INTERSECT clipRgn; one surface module); ADR-0004-AMENDMENT-DEC-10
 *      Sec 4 (Platinum base); ../system7-decomp/specs/sys8/window-chrome.md
 *      Sec 1-6 and scrollbars.md Sec 1-5 (sampled Platinum contract).
 *      CLAUDE.md Law 1 (ground truth), Law 2 (oracle is truth), Law 4 (look like
 *      the frame), Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_OS_FLAIR_CHROME_H
#define INITECH_OS_FLAIR_CHROME_H

#include "grafport.h"           /* GrafPort (-Ispec)                          */
#include "region_algebra.h"     /* rgn_rect_t (-Ispec)                        */
#include "flair_look.h"         /* opaque flair_skin_t policy datum            */

/* ---------------------------------------------------------------------------
 * flair_draw_document_window -- draw one Platinum document window's chrome.
 *
 * Draws into the GrafPort `port` (the current port), with the window occupying
 * the rectangle `frame` (in port-local coordinates: top,left,bottom,right). The
 * chrome is composed top-to-bottom:
 *
 *   - the exact 22-row Platinum title-band profile,
 *   - close, zoom, and rightmost collapse widgets with sampled anatomy,
 *   - the four-pixel raised body bar plus inner line and content inset,
 *   - the disabled active-window scrollbar and active grow grip,
 *   - the one-pixel offset shadow with a two-pixel near-corner notch.
 *
 * EVERY pixel is written through the surface module and clipped to
 * visRgn INTERSECT clipRgn (D-1/D-2). The window must fit inside the port; a
 * window smaller than the chrome it must hold is a no-op (fail-soft, the caller
 * is responsible for a sane frame -- the Window Manager sizes it).
 *
 * `skin` is the caller-selected D-9 data row. The mechanism never inspects its
 * era tag; it passes the pointer beside `port` to flair_look, where matching
 * named color slots are resolved. Desktop setup supplies flair_skin_default().
 *
 * `title` is the window's name (the WindowRecord titleHandle), drawn CENTERED in
 * the title bar in Chicago over a Platinum frame-face gap. A NULL or empty
 * title draws no text. Ink and gap resolve through the C-8 policy seam
 * (flair_look_pixel), never a color literal.
 *
 * `hilited` is the WindowRecord's wHilited flag: non-zero selects the active
 * striped band, widgets, disabled bars, and grow grip. Zero selects the full
 * Platinum inactive delta: flat face fill, inactive frames/ink/shadow, no
 * widgets, hollow bars, and a flat grow box. Ref: window-chrome.md Sec 6 and
 * scrollbars.md Sec 4.
 * ------------------------------------------------------------------------- */
void flair_draw_document_window(GrafPort *port, const flair_skin_t *skin,
                                rgn_rect_t frame,
                                const char *title, int hilited);

/* ---------------------------------------------------------------------------
 * flair_draw_movable_dbox_chrome -- draw one Platinum movableDBoxProc (5)
 * window's chrome: a moveable TITLED modal dialog box (beads initech-zvo6).
 *
 * Draws into `port` over the rectangle `frame`, composed of:
 *   - the SAME Platinum title band flair_draw_document_window draws, reusing the
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
 * `skin` has the same explicit D-9 data-row contract as the document entry.
 *
 * Ref: ../system7-decomp/specs/sys8/window-chrome.md Sec 2 (title band);
 *      ../system7-decomp/specs/toolbox/window-manager.md line 173
 *      ("movableDBoxProc | 5 | movable modal dialog box (title bar, no
 *      grow/zoom)"); ../system7-decomp/specs/chrome/wdef-variant-geometry.md
 *      Sec 1 (variant catalog) + Sec 4 (heritage variant geometry, retained
 *      under BC-10.10); spec/window_record.h (movableDBoxProc = 5);
 *      CLAUDE.md Law 1/2/4.
 * ------------------------------------------------------------------------- */
void flair_draw_movable_dbox_chrome(GrafPort *port, const flair_skin_t *skin,
                                    rgn_rect_t frame,
                                    const char *title);

#endif /* INITECH_OS_FLAIR_CHROME_H */
