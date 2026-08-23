/*
 * os/flair/cursor.h -- FLAIR Cursor Manager save-under overlay.
 *
 * beads: initech-tdnl.1 (GUI remediation R0.1).
 * Ref: Inside Macintosh: Imaging With QuickDraw, cursor ('CURS') resource
 *      format and hotspot semantics; PRD Sec 6.3 (FLAIR Event/Window Managers);
 *      docs/plans/GUI-remediation-plan.md R0.1 (CursorMgr + ShieldCursor);
 *      os/milton/kmain.c:769 flair_desktop_present (final LFB present seam),
 *      :1054 final-surface bind, and :2459/:2549/:2683 FLAIR-LIVE-READY plus
 *      the two WaitNextEvent pump tracking calls (mouse-position source).
 *
 * DEC -- BOOT VISIBILITY (initech-tdnl.1): CursorMgr binds at boot but does not
 * draw until the first real mouse packet reaches the live pump. This preserves
 * every locked no-input boot screendump byte-for-byte. After that first mouse
 * activity the period Mac arrow is visible and tracks EventRecord.where. The
 * bounded oracle pump erases it only before its terminal FLAIR-LIVE-OK still;
 * interactive images retain it for the lifetime of the desktop.
 *
 * The manager binds the FINAL surface presented to the display, not a window or
 * compositor offscreen, so the cursor is always topmost. Repaint clients bracket
 * final-surface writes with flair_cursor_shield/unshield; nesting is supported.
 * Artifact C per ADR-0002: freestanding, deterministic, ASCII-only.
 */
#ifndef INITECH_OS_FLAIR_CURSOR_H
#define INITECH_OS_FLAIR_CURSOR_H

#include <stdint.h>

/* Bind one live framebuffer. pitch is bytes/row; bpp is 8, 24, or 32. */
void flair_cursor_init(void *fb, uint32_t pitch,
                       uint32_t w, uint32_t h, uint32_t bpp);

/* x,y are global screen coordinates of the arrow-tip hotspot. */
void flair_cursor_show(int x, int y);
void flair_cursor_hide(void);
void flair_cursor_move(int x, int y);

/* ShieldCursor analogue. The outermost shield erases the sprite; the matching
 * final unshield redraws it at its latest logical position. */
void flair_cursor_shield(void);
void flair_cursor_unshield(void);

#endif /* INITECH_OS_FLAIR_CURSOR_H */
