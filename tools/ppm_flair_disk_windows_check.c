/*
 * ppm_flair_disk_windows_check.c -- the R3.3 Finder DISK WINDOW screendump
 *                                   STRUCTURE oracle (factory, C).
 *
 * beads: initech-tdnl.10 (GUI remediation R3.3 "disk windows", emu-wiring half).
 *
 * WHAT THIS IS.  A STRUCTURE-only grader for one QEMU screendump (P6 PPM,
 * 640x480) of the booted $(FLAIRTENANTS_IMG) with a REAL Finder disk window
 * open over the REAL FAT12 volume, in one of three states:
 *
 *   rootwin    the root ("Drive A Files") window at its CASCADED DEFAULT frame
 *              (20,60)..(380,280) with the volume's four root entries laid out
 *              row-major on the invisible grid.
 *   movedwin   the SAME window after a title-bar drag to (120,180), CLOSE, and
 *              a REBOOT: it must reappear at the SAVED origin, and the default
 *              rect must be bare desktop again.
 *   newfolder  the default-frame window after Ctrl-N: a FIFTH icon (the real
 *              \NEWFOLD directory) on the grid's second row.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- this is the whole
 * point).  Every probe below is HAND-AUTHORED from INDEPENDENT sources:
 *
 *   (1) the ASCII pixel maps in spec/assets/finder_icons.h -- the authored
 *       artifact and stated source of truth for the FOLDER and DOC strikes
 *       ("The ASCII map in each block below IS the authored artifact ...; if
 *       the two ever disagree, the map wins").  Each probe names its map
 *       row/col and the legend character read off that map by eye; the
 *       absolute screen coordinate is (origin_x + col, origin_y + row).  This
 *       file does NOT include finder_icons.h and never reads the packed strike
 *       words, so a silent edit to a packed word that the map does not justify
 *       goes RED here.  (The APP strike is not probed: no .EXE exists on the
 *       flagship data volume, so no APP icon is ever drawn by these traces --
 *       probing one would be a lie.)
 *
 *   (2) the tone->role->canon chain, resolved BY HAND rather than by calling
 *       the renderer's policy seam (ppm_flair_desktop_icons_check.c documents
 *       the same chain at length):
 *           map '#' = DESK_TONE_INK   -> FLAIR_PART_ICON_INK   -> CIDX_BLACK
 *           map 'w' = DESK_TONE_FACE  -> FLAIR_PART_ICON_FACE  -> CIDX_WHITE
 *           map 'g' = DESK_TONE_SHADE -> FLAIR_PART_ICON_SHADE -> CIDX_CONTROL
 *           map '.' = DESK_TONE_CLEAR -> nothing drawn -> whatever is beneath
 *       with the RGB for each CIDX taken from flair_canon_rgb
 *       (spec/assets/color_canon.h), the INDEPENDENTLY decomp-graded canon that
 *       test-color-canon vouches for -- never flair_palette_rgb, the source the
 *       renderer paints from (ADR-0010 CD-5; HER-02 hard-revoke).
 *
 *       *** THE ONE HONEST WEAKENING, STATED (Law 1).  A desktop icon's CLEAR
 *       pixels show DESKTOP teal, which is a distinct tone, so the R3.2 grader
 *       can assert them.  A WINDOW icon's CLEAR pixels show the window's
 *       CONTENT FILL, which is white -- the SAME canon row as a FACE pixel.  So
 *       a CLEAR probe inside a window asserts only "no ink and no shade here",
 *       which still catches an over-wide or mis-offset strike but cannot
 *       distinguish clear from face.  Those probes are marked T_CLEARW below
 *       and counted separately in the PASS line so nobody mistakes them for
 *       the stronger desktop-side assertion.
 *
 *   (3) the GEOMETRY, hand-carried from os/flair/finder_windows.h Sec 2/3, the
 *       os/flair/window.c :: CalcDocContentRect seam and spec/chrome_metrics.h,
 *       and restated with its arithmetic in spec/flair_disk_windows_traces.mk.
 *       Nothing here links finder_windows.c or reads a constant out of it.
 *
 * WHAT IT IS NOT.  Not a color-VALUE oracle (that is test-color-canon), not a
 * chrome-FIDELITY oracle (that is the chrome_fidelity_golden suite), not an
 * SSIM judge, and not a substitute for harness/proptest/test_finder_windows.c,
 * which owns the pure helpers.
 *
 * Usage: ppm_flair_disk_windows_check <rootwin|movedwin|newfolder> <dump.ppm>
 * Exit 0 iff every assertion passes; non-zero + a loud message naming the
 * assertion, the coordinate, and sampled-vs-expected RGB otherwise (Rule 2).
 *
 * Ref: CLAUDE.md Law 1, Law 2, Law 4, Rule 2, Rule 6, Rule 11, Rule 12.
 *      spec/assets/finder_icons.h (the LOCKED strikes + their ASCII maps),
 *      spec/assets/color_canon.h (the independent canon),
 *      spec/chrome_metrics.h (the widget + title metrics the chrome probes use),
 *      os/flair/finder_windows.h (the grid, the default frame, the markers),
 *      spec/flair_disk_windows_traces.mk (the locked traces these legs grade).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + CIDX_* (-Ispec/assets) */

/* Tight per-channel tolerance -- CAPTURE NOISE only, never a value fudge; the
 * same constant and the same reasoning as tools/ppm_flair_check.c. */
#define TOL 2

enum { SCRW = 640, SCRH = 480 };

/* ---------------------------------------------------------------------------
 * Hand-carried geometry.  Every number below is DERIVED in
 * spec/flair_disk_windows_traces.mk ("THE ROOT WINDOW'S GEOMETRY, DERIVED"),
 * from os/flair/finder_windows.h Sec 2/3 + CalcDocContentRect + chrome_metrics.
 * ------------------------------------------------------------------------- */
#define WIN_W            360   /* FINDER_WIN_DEFAULT_W                         */
#define WIN_H            220   /* FINDER_WIN_DEFAULT_H                         */
#define TITLEBAR_H        22   /* FLAIR_CHROME_TITLEBAR_H                      */
#define CHROME_FRAME       1   /* FLAIR_CHROME_FRAME                           */
#define BODY_BAR           4   /* FLAIR_CHROME_BODY_BAR                        */
#define SCROLLBAR_W       16   /* FLAIR_CHROME_SCROLLBAR_W                     */
#define CLOSE_LEFT_OFF     4   /* FLAIR_CHROME_CLOSE_LEFT_OFF                  */
#define WIDGET_TOP_OFF     4   /* FLAIR_CHROME_WIDGET_TOP_OFF                  */
#define WIDGET_BOX        12   /* FLAIR_CHROME_WIDGET_BOX                      */
#define TITLE_CELL_W       8   /* FLAIR_CHROME_TITLE_CELL_W                    */

/* content rect, relative to the window frame origin (L,T):
 *   left = L + 1, top = T + 22, right = L + 339, bottom = T + 219            */
#define CONT_DX      (CHROME_FRAME)                                  /*   1 */
#define CONT_DY      (TITLEBAR_H)                                    /*  22 */
#define CONT_W       (WIN_W - CHROME_FRAME - CHROME_FRAME - BODY_BAR - SCROLLBAR_W)
                                                                     /* 338 */
#define CONT_H       (WIN_H - TITLEBAR_H - CHROME_FRAME)             /* 197 */

/* the invisible grid (finder_windows.h Sec 2) */
#define GRID_PITCH_X      68
#define GRID_PITCH_Y      52
#define GRID_INSET_X      18
#define GRID_INSET_Y       4
#define GRID_COLS    ((CONT_W - GRID_INSET_X) / GRID_PITCH_X)         /*   4 */

/* label geometry, the finder_desktop.h convention this file inherits          */
#define ICON_DIM        32
#define LABEL_GAP        2
#define LABEL_ROWS      13   /* GENEVA9_CELL_H (11) + 1 pad above + 1 below   */
#define LABEL_TOP_OFF   (ICON_DIM + LABEL_GAP)          /* 34 */
#define LABEL_BOT_OFF   (LABEL_TOP_OFF + LABEL_ROWS)    /* 47 */
#define BAND_INSET_L     2
#define BAND_INSET_R    30

/* the root window's title, "Drive A Files" (FINDER_WIN_ROOT_TITLE), 13 chars.
 * SetWTitle sets titleWidth = n * FLAIR_CHROME_TITLE_CELL_W and the WDEF
 * centres that run in the frame:  x0 = L + (WIN_W - n*8) / 2 = L + 128.      */
#define TITLE_CHARS     13
#define TITLE_RUN_W     (TITLE_CHARS * TITLE_CELL_W)                  /* 104 */
#define TITLE_RUN_DX    ((WIN_W - TITLE_RUN_W) / 2)                   /* 128 */

/* the two frames these legs see */
#define ROOT_L    20
#define ROOT_T    60
#define MOVED_L  120
#define MOVED_T  180

/* ---------------------------------------------------------------------------
 * Tone codes -- the ASCII-map legend, carried through by hand.
 * ------------------------------------------------------------------------- */
enum {
    T_INK    = 0,  /* map '#'  -> FLAIR_PART_ICON_INK   -> CIDX_BLACK        */
    T_FACE   = 1,  /* map 'w'  -> FLAIR_PART_ICON_FACE  -> CIDX_WHITE        */
    T_SHADE  = 2,  /* map 'g'  -> FLAIR_PART_ICON_SHADE -> CIDX_CONTROL      */
    T_CLEARW = 3   /* map '.'  -> not drawn -> the window CONTENT FILL, white
                    *             (see the "ONE HONEST WEAKENING" note above) */
};

static int tone_cidx(int tone)
{
    switch (tone) {
    case T_INK:   return CIDX_BLACK;
    case T_SHADE: return CIDX_CONTROL;
    default:      return CIDX_WHITE;   /* FACE and CLEAR-over-white alike */
    }
}

static const char *tone_name(int tone)
{
    switch (tone) {
    case T_INK:    return "INK    (map '#')";
    case T_FACE:   return "FACE   (map 'w')";
    case T_SHADE:  return "SHADE  (map 'g')";
    default:       return "CLEAR  (map '.', over the white content fill)";
    }
}

typedef struct { int col; int row; int tone; const char *why; } MapProbe;

/* ---------------------------------------------------------------------------
 * FOLDER -- the tab-top manila folder (spec/assets/finder_icons.h, FOLDER
 * block).  The map lines each probe was read from, transcribed here so a
 * reviewer can diff words against words:
 *   row  0: ................................
 *   row  3: ...##########...................
 *   row  4: ...#wwwwwwww#...................
 *   row  7: ...###########################..
 *   row 12: ...#wwwwwwwwwwwwwwwwwwwwwwwww#..
 *   row 25: ...#ggggggggggggggggggggggggg#..
 *   row 27: ...###########################..
 * ------------------------------------------------------------------------- */
static const MapProbe FOLDER_PROBES[] = {
    {  3,  3, T_INK,    "row 3 col 3 -- tab top edge, ink run cols 3..12" },
    { 12,  3, T_INK,    "row 3 col 12 -- last ink column of the tab" },
    { 13,  3, T_CLEARW, "row 3 col 13 -- clear right of the tab" },
    {  7,  4, T_FACE,   "row 4 col 7 -- white tab interior, cols 4..11" },
    {  3,  7, T_INK,    "row 7 col 3 -- front-flap rule, ink cols 3..29" },
    { 29,  7, T_INK,    "row 7 col 29 -- last ink column of the flap rule" },
    { 15, 12, T_FACE,   "row 12 col 15 -- white body, cols 4..28" },
    { 15, 25, T_SHADE,  "row 25 col 15 -- shaded flap, gray cols 4..28" },
    { 15, 27, T_INK,    "row 27 col 15 -- body bottom edge, ink cols 3..29" },
    {  0, 12, T_CLEARW, "row 12 col 0 -- clear margin left of the body" },
    { 31,  7, T_CLEARW, "row 7 col 31 -- clear margin right of the flap rule" }
};
#define FOLDER_PROBE_N ((int)(sizeof FOLDER_PROBES / sizeof FOLDER_PROBES[0]))

/* ---------------------------------------------------------------------------
 * DOC -- the dog-eared page (spec/assets/finder_icons.h, DOC block):
 *   row  2: ......###############...........
 *   row  4: ......#wwwwwwwwwwwww#g#.........
 *   row  7: ......#wwwwwwwwwwwww######......
 *   row  8: ......#wwwwwwwwwwwwwwwwww#......
 *   row 12: ......#wwggggggggggggggww#......
 *   row 29: ......####################......
 * ------------------------------------------------------------------------- */
static const MapProbe DOC_PROBES[] = {
    {  6,  2, T_INK,    "row 2 col 6 -- page top edge, ink run cols 6..20" },
    { 20,  2, T_INK,    "row 2 col 20 -- last ink column of the top edge" },
    { 21,  2, T_CLEARW, "row 2 col 21 -- clear beyond the dog-ear fold" },
    { 10,  4, T_FACE,   "row 4 col 10 -- white page body, cols 7..19" },
    { 21,  4, T_SHADE,  "row 4 col 21 -- the folded-back dog-ear, gray" },
    { 25,  7, T_INK,    "row 7 col 25 -- fold rule, ink cols 20..25" },
    { 15,  8, T_FACE,   "row 8 col 15 -- white body under the fold" },
    {  7, 12, T_FACE,   "row 12 col 7 -- white left margin, cols 7..8" },
    { 15, 12, T_SHADE,  "row 12 col 15 -- first ruled text line, gray 9..22" },
    { 15, 29, T_INK,    "row 29 col 15 -- page bottom edge, ink cols 6..25" },
    { 31, 29, T_CLEARW, "row 29 col 31 -- clear margin right of the base" }
};
#define DOC_PROBE_N ((int)(sizeof DOC_PROBES / sizeof DOC_PROBES[0]))

/* The window's icon roster, in the enumeration order the traces file derives
 * from mtools (README.TXT / APPS / DESKTOP.DB / TRASH, + NEWFOLD on the
 * newfolder leg).  KIND is the finder_win_kind_of rule applied by hand. */
enum { K_DOC = 0, K_FOLDER = 1 };
typedef struct { const char *name; int kind; } RosterEntry;
static const RosterEntry ROSTER[] = {
    { "README.TXT", K_DOC    },
    { "APPS",       K_FOLDER },
    { "DESKTOP.DB", K_DOC    },
    { "TRASH",      K_FOLDER },
    { "NEWFOLD",    K_FOLDER }   /* newfolder leg only */
};

/* ---- PPM P6 reader (the ppm_flair_check.c invariant). -------------------- */
static int read_uint(FILE *f, long *out)
{
    int c;
    long v;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    v = 0;
    while (c >= '0' && c <= '9') {
        v = v * 10 + (c - '0');
        c = fgetc(f);
    }
    *out = v;
    return 0;
}

static unsigned char *g_buf;
static long g_w, g_h;
static int g_fail;
static const char *g_leg;

static const unsigned char *at(int x, int y)
{
    return g_buf + ((long)y * g_w + x) * 3;
}

static unsigned int IDX(int i)
{
    return (unsigned int)(flair_canon_rgb((unsigned char)i) & 0x00FFFFFFu);
}

static int is_rgb(int x, int y, unsigned int rgb)
{
    const unsigned char *p = at(x, y);
    int r = (int)((rgb >> 16) & 0xFFu);
    int g = (int)((rgb >> 8) & 0xFFu);
    int b = (int)(rgb & 0xFFu);
    return abs((int)p[0] - r) <= TOL &&
           abs((int)p[1] - g) <= TOL &&
           abs((int)p[2] - b) <= TOL;
}

static void assert_idx(int x, int y, int idx, const char *what)
{
    if (x < 0 || y < 0 || x >= (int)g_w || y >= (int)g_h) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- probe (%d,%d) "
                "is off the %ldx%ld raster (%s)\n",
                g_leg, x, y, g_w, g_h, what);
        g_fail = 1;
        return;
    }
    if (!is_rgb(x, y, IDX(idx))) {
        const unsigned char *p = at(x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- %s\n"
                "    at (%d,%d): sampled RGB(%d,%d,%d)=#%02X%02X%02X, "
                "expected canon idx %d = RGB(%u,%u,%u)=#%06X (tol +/-%d)\n",
                g_leg, what, x, y, p[0], p[1], p[2], p[0], p[1], p[2],
                idx, (e >> 16) & 0xFFu, (e >> 8) & 0xFFu, e & 0xFFu, e, TOL);
        g_fail = 1;
    }
}

/* Grade one strike against its hand-read ASCII-map probes at `x0,y0`. */
static void check_sprite(const char *label, int x0, int y0, int kind)
{
    const MapProbe *probes = (kind == K_FOLDER) ? FOLDER_PROBES : DOC_PROBES;
    int n = (kind == K_FOLDER) ? FOLDER_PROBE_N : DOC_PROBE_N;
    char what[256];
    int i;
    for (i = 0; i < n; i++) {
        snprintf(what, sizeof what,
                 "%s %s sprite @(%d,%d): expected %s -- %s",
                 label, (kind == K_FOLDER) ? "FOLDER" : "DOC", x0, y0,
                 tone_name(probes[i].tone), probes[i].why);
        assert_idx(x0 + probes[i].col, y0 + probes[i].row,
                   tone_cidx(probes[i].tone), what);
    }
}

/*
 * Grade one window-icon label band as a RELATION.
 *
 * The band sub-rect is [x0+2, x0+30) x [y0+34, y0+47), i.e. the same inset the
 * R3.2 desktop grader uses: every label this gate sees is at least as wide as
 * the 32-px sprite and is centred on it, so the sub-rect lies inside the drawn
 * text run.  Asserted: (a) ZERO desktop-teal pixels -- we really are inside a
 * window, not looking at bare desktop where a window failed to open; (b) BOTH
 * black and white present -- the glyphs really drew; (c) WHITE in the majority
 * -- the label is NOT inverted (no window icon is selected in any leg of this
 * gate).  The relation flips the moment the inversion is wrong either way.
 */
static void check_band(const char *label, int x0, int y0)
{
    long black = 0, white = 0, teal = 0, other = 0;
    int x, y;
    int bx0 = x0 + BAND_INSET_L, bx1 = x0 + BAND_INSET_R;
    int by0 = y0 + LABEL_TOP_OFF, by1 = y0 + LABEL_BOT_OFF;

    if (bx0 < 0 || by0 < 0 || bx1 > (int)g_w || by1 > (int)g_h) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- %s label band "
                "sub-rect (%d,%d)-(%d,%d) is off the raster\n",
                g_leg, label, bx0, by0, bx1, by1);
        g_fail = 1;
        return;
    }
    for (y = by0; y < by1; y++) {
        for (x = bx0; x < bx1; x++) {
            if (is_rgb(x, y, IDX(CIDX_BLACK)))        black++;
            else if (is_rgb(x, y, IDX(CIDX_WHITE)))   white++;
            else if (is_rgb(x, y, IDX(CIDX_DESKTOP))) teal++;
            else                                      other++;
        }
    }
    if (teal != 0) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) shows %ld DESKTOP-teal pixels: this is bare "
                "desktop, so the window content was never painted there "
                "(black=%ld white=%ld other=%ld)\n",
                g_leg, label, bx0, by0, bx1, by1, teal, black, white, other);
        g_fail = 1;
        return;
    }
    if (black == 0 || white == 0) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) is a single tone (black=%ld white=%ld "
                "other=%ld): the label glyphs are missing\n",
                g_leg, label, bx0, by0, bx1, by1, black, white, other);
        g_fail = 1;
        return;
    }
    if (white <= black) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) is INVERTED when nothing is selected: "
                "black=%ld white=%ld\n",
                g_leg, label, bx0, by0, bx1, by1, black, white);
        g_fail = 1;
        return;
    }
    printf("    %-10s label band (%d,%d)-(%d,%d): normal "
           "(black=%ld white=%ld)\n",
           label, bx0, by0, bx1, by1, black, white);
}

/* Count CIDX_BLACK pixels in a half-open rect. */
static long count_black(int x0, int y0, int x1, int y1)
{
    long n = 0;
    int x, y;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++)
            if (is_rgb(x, y, IDX(CIDX_BLACK))) n++;
    return n;
}

/*
 * The window CHROME at frame origin (L,T).  Four families, every coordinate
 * derived from spec/chrome_metrics.h + the default-frame constants:
 *
 *   a) the black frame rules: the four corners of (L,T)..(L+360,T+220) and the
 *      title bar's bottom rule at T + TITLEBAR_H - 1.
 *   b) the content fill: (L+1, T+22) is content.left/content.top and must be
 *      the white window body.
 *   c) the go-away box: footprint (L+4, T+4) + 12x12; the widget's DARK RING is
 *      the 1-px inset border of that footprint, so its rails are x = L+5 and
 *      x = L+4+12-1 = L+15, y = T+5 .. T+15 -- CIDX_PLAT_DARK_RING (#3F3F3F).
 *   d) the title run is CENTRED: "Drive A Files" is 13 chars x 8 px = 104, so
 *      it starts at L + (360-104)/2 = L+128.  Assert ink INSIDE that run and
 *      ZERO ink in the gap between the go-away box's shadow and the run
 *      ([L+40, L+120)) -- which is what makes this a CENTRING test rather than
 *      a "some text exists" test.
 */
static void check_chrome(int L, int T)
{
    char what[256];
    long ink_run, ink_gap;
    int trow0 = T + 2, trow1 = T + TITLEBAR_H - 1;   /* title-bar interior */

    snprintf(what, sizeof what, "window frame top-left corner (L,T)");
    assert_idx(L, T, CIDX_BLACK, what);
    snprintf(what, sizeof what, "window frame top-right corner (L+W-1,T)");
    assert_idx(L + WIN_W - 1, T, CIDX_BLACK, what);
    snprintf(what, sizeof what, "window frame bottom-left corner (L,T+H-1)");
    assert_idx(L, T + WIN_H - 1, CIDX_BLACK, what);
    snprintf(what, sizeof what, "window frame bottom-right corner (L+W-1,T+H-1)");
    assert_idx(L + WIN_W - 1, T + WIN_H - 1, CIDX_BLACK, what);
    snprintf(what, sizeof what,
             "title bar bottom rule at T+TITLEBAR_H-1 = T+%d", TITLEBAR_H - 1);
    assert_idx(L + WIN_W / 2, T + TITLEBAR_H - 1, CIDX_BLACK, what);

    snprintf(what, sizeof what,
             "content fill at content.left/top = (L+%d, T+%d)", CONT_DX, CONT_DY);
    assert_idx(L + CONT_DX, T + CONT_DY, CIDX_WHITE, what);
    snprintf(what, sizeof what,
             "content fill deep inside the body, clear of every icon cell");
    assert_idx(L + 280, T + 180, CIDX_WHITE, what);

    snprintf(what, sizeof what,
             "go-away box DARK RING top-left rail (L+%d,T+%d)",
             CLOSE_LEFT_OFF + 1, WIDGET_TOP_OFF + 1);
    assert_idx(L + CLOSE_LEFT_OFF + 1, T + WIDGET_TOP_OFF + 1,
               CIDX_PLAT_DARK_RING, what);
    snprintf(what, sizeof what,
             "go-away box DARK RING bottom-right rail (L+%d,T+%d)",
             CLOSE_LEFT_OFF + WIDGET_BOX - 1, WIDGET_TOP_OFF + WIDGET_BOX - 1);
    assert_idx(L + CLOSE_LEFT_OFF + WIDGET_BOX - 1,
               T + WIDGET_TOP_OFF + WIDGET_BOX - 1,
               CIDX_PLAT_DARK_RING, what);
    snprintf(what, sizeof what, "go-away box DARK RING left rail, mid height");
    assert_idx(L + CLOSE_LEFT_OFF + 1, T + WIDGET_TOP_OFF + WIDGET_BOX / 2,
               CIDX_PLAT_DARK_RING, what);
    snprintf(what, sizeof what, "go-away box DARK RING right rail, mid height");
    assert_idx(L + CLOSE_LEFT_OFF + WIDGET_BOX - 1,
               T + WIDGET_TOP_OFF + WIDGET_BOX / 2,
               CIDX_PLAT_DARK_RING, what);

    ink_run = count_black(L + TITLE_RUN_DX, trow0,
                          L + TITLE_RUN_DX + TITLE_RUN_W, trow1);
    ink_gap = count_black(L + 40, trow0, L + 120, trow1);
    if (ink_run == 0) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- no title ink in "
                "the CENTRED run [L+%d, L+%d) x [T+2, T+%d): the window has no "
                "title (expected \"Drive A Files\", %d chars x %d px = %d, "
                "centred in the %d-px frame)\n",
                g_leg, TITLE_RUN_DX, TITLE_RUN_DX + TITLE_RUN_W, TITLEBAR_H - 1,
                TITLE_CHARS, TITLE_CELL_W, TITLE_RUN_W, WIN_W);
        g_fail = 1;
    }
    if (ink_gap != 0) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: FAIL leg %s -- %ld ink pixels in "
                "the title-bar GAP [L+40, L+120) x [T+2, T+%d), which must be "
                "empty: the title is not centred where "
                "L + (%d - %d)/2 = L+%d says it is\n",
                g_leg, ink_gap, TITLEBAR_H - 1, WIN_W, TITLE_RUN_W,
                TITLE_RUN_DX);
        g_fail = 1;
    }
    printf("    chrome @(%d,%d): frame + title rule + content fill + go-away "
           "ring; title ink=%ld in the centred run, %ld in the gap\n",
           L, T, ink_run, ink_gap);
}

/* Bare-desktop control points, chosen clear of BOTH window frames this gate
 * sees, of both tenant windows, of both desktop icon cells and of the cursor
 * PARK rect (620,460)-(636,476).  See the clearance argument in
 * spec/flair_disk_windows_traces.mk trace 2 step C. */
typedef struct { int x; int y; const char *why; } Point;
static const Point CONTROL_TEAL[] = {
    { 450, 440, "below every window (root bottom 280, moved bottom 400, NOTES "
                "bottom 340) and left of both desktop icon cells" },
    { 600, 300, "right of NOTES (right 560) and of both window frames (right "
                "380 / 480); above the TRASH cell (top 404)" }
};
#define CONTROL_TEAL_N ((int)(sizeof CONTROL_TEAL / sizeof CONTROL_TEAL[0]))

static void check_control_teal(void)
{
    char what[256];
    int i;
    for (i = 0; i < CONTROL_TEAL_N; i++) {
        snprintf(what, sizeof what,
                 "bare-desktop control point -- %s", CONTROL_TEAL[i].why);
        assert_idx(CONTROL_TEAL[i].x, CONTROL_TEAL[i].y, CIDX_DESKTOP, what);
    }
}

/* The default frame must be VACATED on the movedwin leg: the window really
 * moved and its old footprint is bare desktop again.  Both points sit left of
 * HELLO (left 60) so they are desktop in the un-windowed scene. */
static const Point VACATED[] = {
    { ROOT_L,     ROOT_T,     "the default frame's top-left corner" },
    { ROOT_L + 1, ROOT_T + CONT_DY, "the default frame's content top-left" }
};
#define VACATED_N ((int)(sizeof VACATED / sizeof VACATED[0]))

static void check_vacated(void)
{
    char what[256];
    int i;
    for (i = 0; i < VACATED_N; i++) {
        snprintf(what, sizeof what,
                 "the window must NOT be at its cascaded default any more -- %s",
                 VACATED[i].why);
        assert_idx(VACATED[i].x, VACATED[i].y, CIDX_DESKTOP, what);
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[3] = {0, 0, 0};
    long maxv = 0;
    size_t want, got;
    int leg_root, leg_moved, leg_newfolder;
    int L, T, n_icons, i;

    if (argc != 3) {
        fprintf(stderr,
                "usage: %s <rootwin|movedwin|newfolder> <dump.ppm>\n", argv[0]);
        return 2;
    }
    g_leg         = argv[1];
    leg_root      = (strcmp(g_leg, "rootwin")   == 0);
    leg_moved     = (strcmp(g_leg, "movedwin")  == 0);
    leg_newfolder = (strcmp(g_leg, "newfolder") == 0);
    if (!leg_root && !leg_moved && !leg_newfolder) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: unknown leg '%s' "
                "(want rootwin|movedwin|newfolder)\n", g_leg);
        return 2;
    }

    f = fopen(argv[2], "rb");
    if (!f) {
        fprintf(stderr, "ppm_flair_disk_windows_check: cannot open %s\n",
                argv[2]);
        return 2;
    }
    if (fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6') {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: not a P6 PPM (got '%s')\n",
                magic);
        fclose(f);
        return 2;
    }
    if (read_uint(f, &g_w) != 0 || read_uint(f, &g_h) != 0 ||
        read_uint(f, &maxv) != 0) {
        fprintf(stderr, "ppm_flair_disk_windows_check: malformed PPM header\n");
        fclose(f);
        return 2;
    }
    if (g_w != SCRW || g_h != SCRH || maxv != 255) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: unexpected dims/maxval "
                "w=%ld h=%ld max=%ld (want %dx%d/255)\n",
                g_w, g_h, maxv, SCRW, SCRH);
        fclose(f);
        return 2;
    }
    want = (size_t)g_w * (size_t)g_h * 3u;
    g_buf = (unsigned char *)malloc(want);
    if (!g_buf) {
        fprintf(stderr, "ppm_flair_disk_windows_check: OOM\n");
        fclose(f);
        return 2;
    }
    got = fread(g_buf, 1, want, f);
    fclose(f);
    if (got != want) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: short raster (%zu of %zu "
                "bytes)\n", got, want);
        free(g_buf);
        return 2;
    }

    L       = leg_moved ? MOVED_L : ROOT_L;
    T       = leg_moved ? MOVED_T : ROOT_T;
    n_icons = leg_newfolder ? 5 : 4;

    printf("ppm_flair_disk_windows_check: leg %s on %s\n"
           "    frame (%d,%d)..(%d,%d); content (%d,%d)..(%d,%d); "
           "grid cols=%d pitch=%dx%d inset=%d,%d; %d icons\n",
           g_leg, argv[2],
           L, T, L + WIN_W, T + WIN_H,
           L + CONT_DX, T + CONT_DY, L + CONT_DX + CONT_W, T + CONT_DY + CONT_H,
           GRID_COLS, GRID_PITCH_X, GRID_PITCH_Y, GRID_INSET_X, GRID_INSET_Y,
           n_icons);

    check_chrome(L, T);

    for (i = 0; i < n_icons; i++) {
        int gx = L + CONT_DX + GRID_INSET_X + (i % GRID_COLS) * GRID_PITCH_X;
        int gy = T + CONT_DY + GRID_INSET_Y + (i / GRID_COLS) * GRID_PITCH_Y;
        printf("    icon[%d] %-10s cell (%d,%d) [col %d row %d]\n",
               i, ROSTER[i].name, gx, gy, i % GRID_COLS, i / GRID_COLS);
        check_sprite(ROSTER[i].name, gx, gy, ROSTER[i].kind);
        check_band(ROSTER[i].name, gx, gy);
    }

    if (leg_moved) {
        check_vacated();
    }
    check_control_teal();

    free(g_buf);
    if (g_fail) {
        fprintf(stderr,
                "ppm_flair_disk_windows_check: leg %s FAILED\n", g_leg);
        return 1;
    }
    printf("ppm_flair_disk_windows_check: leg %s PASS "
           "(%d icon strikes graded off the finder_icons.h ASCII maps, "
           "%d label-band relations, 11 chrome assertions, %d bare-teal "
           "controls%s)\n",
           g_leg, n_icons, n_icons, CONTROL_TEAL_N,
           leg_moved ? ", 2 vacated-default-rect probes" : "");
    return 0;
}
