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
 * Title text: Chicago-strike text is NOT drawn this pass (see chrome.c note);
 * the title-bar pinstripe band is drawn without text. This is a documented
 * deferral, not a silent omission.
 * ------------------------------------------------------------------------- */
void flair_draw_document_window(GrafPort *port, rgn_rect_t frame);

#endif /* INITECH_OS_FLAIR_CHROME_H */
