/*
 * spec/progress_platinum.h -- the Mac OS 8.1 Platinum DETERMINATE PROGRESS
 * BAR (Appearance CDEF 5) as locked spec-data (Rule 8).
 *
 * bead: initech-bf2 (the FILE COPY dialog). Deliberate Rule-8 act: this file
 *       is NEW; it replaces the authored flat bar (black frame, white well,
 *       one navy fill) that docs/design/GUI-remediation-D1-D2-D3-design.md
 *       row 60 called "unverifiable against Platinum -- leave to FILE COPY
 *       canon grading, do not invent Platinum art".
 *
 * SOURCE (Law 1): ../system7-decomp/re/mint-results-s8-progress.md -- a
 * one-shot Basilisk II mint of the real Mac OS 8.1 Finder copy window
 * (`Copy to "Initech81"`), captures goldens/captures/
 * s8_progress_determinate{_start,,_full}.png (+ _start_run2, the repro run).
 * Every row/column class below is TRANSCRIBED from that ledger's run-length
 * scans; the host oracle (harness/proptest/test_control.c PROPERTY 6) and the
 * emu oracle (tools/ppm_flair_file_copy_check.c) never read this file -- they
 * read the PNGs (Law 2: an independent golden, not the artifact's table).
 *
 * GEOMETRY (golden x=190..377, y=133..146 -> rect-relative):
 *   contrlRect = the ETCHED OUTER bounds, FLAIR_PROGRESS_H rows tall.
 *   row 0, cols 0..w-2          etch light  'L' (#C0C0C0)   [top]
 *   col 0, rows 0..h-2          etch light  'L'             [left]
 *   row h-1, cols 1..w-1        etch white  'W' (#FFFFFF)   [bottom]
 *   col w-1, rows 1..h-1        etch white  'W'             [right]
 *   (the corners (w-1,0) and (0,h-1) are NOT drawn: the face shows)
 *   frame rect cols 1..w-2 x rows 1..h-2, 1 px  'K' (#000000)
 *   interior cols 2..w-3 x rows 2..h-3 (10 rows): fill | edge | track.
 *
 * VALUE -> FILL (the one [inferred] rule; Finder value units are not visible
 * in pixels): f = W * value / max fill columns (W = interior width), clamped
 * to 0..W. f > 0: the EDGE column E = L + f - 1 is black ('K'), the fill
 * occupies L..E-1 and the TRACK starts at T = E + 1. f == 0: no edge, the
 * track starts at T = L [inferred; the value-0 bar was never on screen].
 *
 * FILL CLASSES (L = interior left; golden L = 192):
 *   f <= FLAIR_PROGRESS_SOLID_MAX: cols L..E-1 solid rung '4'
 *     [verified E-L = 1 and 3; E-L = 4, 5 inferred]
 *   else: col L      = rung '3' every row
 *         col L+1    = FLAIR_PROGRESS_COL1[row]
 *         cols L+2..E-4 = FLAIR_PROGRESS_BODY[row]
 *         col E-3    = FLAIR_PROGRESS_CAP3[row]
 *         col E-2    = FLAIR_PROGRESS_CAP2[row]
 *         col E-1    = FLAIR_PROGRESS_CAP1[row]
 *     [verified E = 198 (body one column wide), 274, 332, 373, 375]
 * TRACK (T..R, R = interior right): col T = '7', col T+1 = 'A' (every row,
 *   verified at T = R-1); cols >= T+2: row 0 'A' (but col R 'C'),
 *   rows 1..8 'C' (but col R 'E'), row 9 'E'.
 *
 * RUNG CODES -- the measured RGBs (sampled domain, platinum-palette.md
 * Sec 1). Lavender rungs are clut 208 (platinum-palette.md Sec 3) and are
 * SUBSTITUTED at the policy seam by the ratified lavender->teal rule
 * (ADR-0004-AMENDMENT-DEC-10 Sec 6 OQ-2/OQ-3), using the SAME H/P/S/R role
 * mapping the scroll thumb ratified (tdnl.96/.97, os/flair/flair_look.c):
 *   'W' #FFFFFF  '1' #DADAFF (H)  '2' #B3B3FF (P)  '3' #8787DA (S)
 *   '4' #5454B3 (R)  'D' #000077 (the dark cap; the menu-title dark edge)
 *   'K' #000000  '7' #777777  'A' #A5A5A5  'C' #CDCDCD  'E' #E7E7E7
 *   'L' #C0C0C0
 *
 * ASCII-clean (Rule 12). Freestanding: no includes.
 */
#ifndef INITECH_SPEC_PROGRESS_PLATINUM_H
#define INITECH_SPEC_PROGRESS_PLATINUM_H

#define FLAIR_PROGRESS_H          14   /* etched outer rows (golden y=133..146) */
#define FLAIR_PROGRESS_ROWS       10   /* interior rows (golden y=135..144)     */
#define FLAIR_PROGRESS_INSET       2   /* etch + frame                          */
#define FLAIR_PROGRESS_SOLID_MAX   6   /* f <= 6 draws the solid '4' run        */

/* Interior row r = 0..9 (golden y = 135 + r). */
static const char FLAIR_PROGRESS_COL1[FLAIR_PROGRESS_ROWS + 1] = "321WWW1234";
static const char FLAIR_PROGRESS_BODY[FLAIR_PROGRESS_ROWS + 1] = "4321W1234D";
static const char FLAIR_PROGRESS_CAP3[FLAIR_PROGRESS_ROWS + 1] = "432111234D";
static const char FLAIR_PROGRESS_CAP2[FLAIR_PROGRESS_ROWS + 1] = "434444444D";
static const char FLAIR_PROGRESS_CAP1[FLAIR_PROGRESS_ROWS + 1] = "4DDDDDDDDD";

#endif /* INITECH_SPEC_PROGRESS_PLATINUM_H */
