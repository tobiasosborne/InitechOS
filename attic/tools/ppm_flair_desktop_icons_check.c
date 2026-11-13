/*
 * ppm_flair_desktop_icons_check.c -- the R3.2 Finder DESKTOP MANAGER
 *                                    screendump STRUCTURE oracle (factory, C).
 *
 * beads: initech-tdnl.9 (GUI remediation R3.2 "DesktopMgr", emu-wiring half).
 *
 * WHAT THIS IS.  A STRUCTURE-only grader for one QEMU screendump (P6 PPM,
 * 640x480) of the booted $(FLAIRTENANTS_IMG) desktop, in one of three states:
 *
 *   default   the boot scene: both desktop icons drawn at their seeded origins,
 *             both label bands un-inverted, bare desktop beside them.
 *   selected  after a click on the volume icon: its label band is INVERTED
 *             (black band, white text -- design F1.1); the Trash is untouched.
 *   moved     after the icon drag: the volume icon (and its label) live at the
 *             NEW origin, its OLD cell has been repainted back to bare desktop,
 *             and the Trash is untouched.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2 -- this is the whole
 * point).  Every probe below is HAND-AUTHORED from two INDEPENDENT sources:
 *
 *   (1) the ASCII pixel maps in spec/assets/desk_icons.h -- the authored
 *       artifact and stated source of truth for those strikes ("The ASCII map
 *       in each block below IS the authored artifact ...; if the two ever
 *       disagree, the map wins").  Each probe names its map row/col and the
 *       legend character read off that map by eye; the absolute screen
 *       coordinate is (origin_x + col, origin_y + row).  This file does NOT
 *       include desk_icons.h and never reads the packed strike words, so a
 *       silent edit to a packed word that the map does not justify goes RED
 *       here -- the same discipline harness/proptest/test_desk_icons.c applies
 *       on the host side.
 *
 *   (2) the tone->role->canon chain, resolved BY HAND rather than by calling
 *       the renderer's policy seam:
 *           map '#' = DESK_TONE_INK   -> FLAIR_PART_ICON_INK   -> CIDX_BLACK
 *           map 'w' = DESK_TONE_FACE  -> FLAIR_PART_ICON_FACE  -> CIDX_WHITE
 *           map 'g' = DESK_TONE_SHADE -> FLAIR_PART_ICON_SHADE -> CIDX_CONTROL
 *           map '.' = DESK_TONE_CLEAR -> nothing drawn         -> CIDX_DESKTOP
 *       (os/flair/flair_look.c, the Finder desktop-icon tone rows.)  The RGB for
 *       each CIDX comes from flair_canon_rgb (spec/assets/color_canon.h), the
 *       INDEPENDENTLY decomp-graded canon that test-color-canon vouches for --
 *       never flair_palette_rgb, the source the renderer paints from
 *       (ADR-0010 CD-5; HER-02 hard-revoke; ppm_flair_check.c documents the
 *       same re-key at length).
 *
 * The geometry (sprite origins, label-band rows, the drop cell) is likewise
 * hand-carried from spec/flair_desktop_icons_traces.mk's delta arithmetic and
 * os/flair/finder_desktop.h Sec 1, NOT computed by linking finder_desktop.c.
 *
 * WHAT IT IS NOT.  Not a color-VALUE oracle (that is test-color-canon), not an
 * SSIM/fidelity judge, and not a substitute for the host property suite
 * (harness/proptest/test_finder_desktop.c) which owns the pure trackers.
 *
 * THE LABEL-BAND TEST IS A RELATION, NOT A PIXEL.  A band probe counts CIDX_
 * BLACK and CIDX_WHITE pixels inside a sub-rect that lies wholly within the
 * label band for every label this gate sees, and asserts (a) ZERO desktop-teal
 * pixels -- the band was actually filled -- (b) BOTH tones present -- band AND
 * glyphs -- and (c) which tone is in the MAJORITY.  Un-inverted: white
 * majority.  Selected: black majority.  That relation is invariant to the exact
 * glyph strike and to the exact label width, and it flips the moment the
 * selection inversion is wrong in either direction.
 *
 * Usage: ppm_flair_desktop_icons_check <default|selected|moved> <dump.ppm>
 * Exit 0 iff every assertion passes; non-zero + a loud message naming the
 * assertion, the coordinate, and sampled-vs-expected RGB otherwise (Rule 2).
 *
 * Ref: CLAUDE.md Law 1, Law 2, Law 4, Rule 2, Rule 6, Rule 11, Rule 12.
 *      spec/assets/desk_icons.h (the LOCKED strikes + their ASCII maps),
 *      spec/assets/color_canon.h (the independent canon),
 *      os/flair/flair_look.c (tone -> canon row),
 *      os/flair/finder_desktop.h Sec 1/4/10 (geometry + the inversion rule),
 *      spec/flair_desktop_icons_traces.mk (the locked traces these legs grade).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + CIDX_* (-Ispec/assets) */
#include "icon_capture.h"  /* the mint-008 golden (-Iharness/render)   */

#ifndef ICON_CAPTURES_DIR
#error "build with -DICON_CAPTURES_DIR='\"<system7-decomp>/goldens/captures\"'"
#endif

/* Tight per-channel tolerance -- CAPTURE NOISE only, never a value fudge; the
 * same constant and the same reasoning as tools/ppm_flair_check.c.  The four
 * canon rows this file uses (black / white / teal / #C0C0C0) are separated by
 * far more than 2 per channel, so the grader still discriminates. */
#define TOL 2

enum { SCRW = 640, SCRH = 480 };

/* ---------------------------------------------------------------------------
 * Hand-carried geometry (spec/flair_desktop_icons_traces.mk; finder_desktop.h)
 * ------------------------------------------------------------------------- */
#define ICON_DIM        32
#define LABEL_GAP        2
#define LABEL_ROWS      13   /* GENEVA9_CELL_H (11) + 1 pad above + 1 below   */
#define LABEL_TOP_OFF   (ICON_DIM + LABEL_GAP)          /* 34 */
#define LABEL_BOT_OFF   (LABEL_TOP_OFF + LABEL_ROWS)    /* 47 */

/* Seeded origins (finder_desktop.h Sec 1: VOLUME top-right, TRASH bottom-right
 * of the usable desktop (0,40)-(640,480)). */
#define VOL_X0   584
#define VOL_Y0    48
#define TRASH_X0 584
#define TRASH_Y0 404

/* The drop cell the locked FLAIR_ICON_DRAGDROP_SPEC commits: sprite origin
 * (584,48) + the clamped delta (-184,+352).  See that file's clearance note. */
#define VOL_MOVED_X0 400
#define VOL_MOVED_Y0 400

/* The label-band sub-rect, expressed as an inset from the SPRITE columns.  Every
 * label band this gate sees is wider than the 32-px sprite and centred on it
 * ("INITECH" 47 wide, "Trash" 34 wide), so [x0+2, x0+30) is inside the band in
 * all three states -- and inside the drawn text run as well. */
#define BAND_INSET_L   2
#define BAND_INSET_R  30

/* ---------------------------------------------------------------------------
 * THE SPRITES (bead initech-tdnl.53): graded CELL FOR CELL, zero tolerance,
 * against the INDEPENDENT mint-008 Mac OS 8.1 captures, decoded at run time by
 * harness/render/icon_capture.h from ICON_CAPTURES_DIR (a build-time path to
 * ../system7-decomp/goldens/captures; a missing capture FAILS). An opaque
 * cell must carry the capture's RGB for the icon's state (a SELECTED volume
 * is the captured selected form); a clear cell the desktop teal, CIDX_DESKTOP
 * via the independent canon. This replaces the v1 hand-read ASCII-map probes:
 * the v2 strikes are transcribed from those captures, so the captures -- not
 * the strike's own map -- are the expected values (Law 2).
 * ------------------------------------------------------------------------- */
static icap_cell_t g_cap[ICAP__KINDS];

/* ---------------------------------------------------------------------------
 * The four bare-teal control points.  Each is adjacent to an icon cell but
 * outside it, outside both tenant windows (NOTES right edge 560), outside the
 * cursor PARK rect (620,460)-(636,476), and outside the drop cell
 * (393,400)-(440,447).  Cell rects: VOLUME (577,48)-(624,95),
 * TRASH (583,404)-(617,451).
 * ------------------------------------------------------------------------- */
typedef struct { int x; int y; const char *why; } Point;
static const Point CONTROL_TEAL[] = {
    { 570,  60, "left of the VOLUME cell (left 577), right of NOTES (right 560)" },
    { 630,  60, "right of the VOLUME cell (right 624), inside the 640-px screen" },
    { 570, 420, "left of the TRASH cell (left 583)" },
    { 632, 420, "right of the TRASH cell (right 617), above the PARK rect (top 460)" }
};
#define CONTROL_TEAL_N ((int)(sizeof CONTROL_TEAL / sizeof CONTROL_TEAL[0]))

/* The VOLUME's OLD footprint, sampled in the `moved` leg: every one of these
 * carried icon pixels in the `default` leg and must read bare desktop once the
 * drop's invalidate(old)+invalidate(new) repaint has run. */
static const Point OLD_FOOTPRINT[] = {
    { VOL_X0 + 10, VOL_Y0 +  3, "old sprite row 3 col 10" },
    { VOL_X0 + 10, VOL_Y0 +  4, "old sprite row 4 col 10" },
    { VOL_X0 + 15, VOL_Y0 + 19, "old sprite row 19 col 15" },
    { VOL_X0 + 15, VOL_Y0 + 28, "old sprite row 28 col 15" },
    { VOL_X0 +  3, VOL_Y0 + LABEL_TOP_OFF + 6, "old label band, left third" },
    { VOL_X0 + 16, VOL_Y0 + LABEL_TOP_OFF + 6, "old label band, centre" },
    { VOL_X0 + 29, VOL_Y0 + LABEL_TOP_OFF + 6, "old label band, right third" }
};
#define OLD_FOOTPRINT_N ((int)(sizeof OLD_FOOTPRINT / sizeof OLD_FOOTPRINT[0]))

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
                "ppm_flair_desktop_icons_check: FAIL leg %s -- probe (%d,%d) "
                "is off the %ldx%ld raster (%s)\n",
                g_leg, x, y, g_w, g_h, what);
        g_fail = 1;
        return;
    }
    if (!is_rgb(x, y, IDX(idx))) {
        const unsigned char *p = at(x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s\n"
                "    at (%d,%d): sampled RGB(%d,%d,%d)=#%02X%02X%02X, "
                "expected canon idx %d = RGB(%u,%u,%u)=#%06X (tol +/-%d)\n",
                g_leg, what, x, y, p[0], p[1], p[2], p[0], p[1], p[2],
                idx, (e >> 16) & 0xFFu, (e >> 8) & 0xFFu, e & 0xFFu, e, TOL);
        g_fail = 1;
    }
}

/* Grade one 32x32 sprite cell at `x0,y0` against the mint-008 capture. */
static void check_sprite(const char *label, int x0, int y0, int kind, int state)
{
    char what[96];
    int d;
    snprintf(what, sizeof what, "%s %s", label, state ? "SEL" : "UNSEL");
    d = icap_grade_frame(g_buf, (int)g_w, (int)g_h, x0, y0, &g_cap[kind], state,
                         IDX(CIDX_DESKTOP), what);
    if (d) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s sprite @(%d,%d): "
                "%d/1024 cells differ from the mint-008 capture\n",
                g_leg, what, x0, y0, d);
        g_fail = 1;
    } else {
        printf("    %-6s sprite @(%d,%d): %s, 0/1024 cells differ from the capture\n",
               label, x0, y0, state ? "SELECTED" : "unselected");
    }
}

/* Grade one label band as a RELATION (see the header note). */
static void check_band(const char *label, int x0, int y0, int inverted)
{
    long black = 0, white = 0, teal = 0, other = 0;
    int x, y;
    int bx0 = x0 + BAND_INSET_L, bx1 = x0 + BAND_INSET_R;
    int by0 = y0 + LABEL_TOP_OFF, by1 = y0 + LABEL_BOT_OFF;

    if (bx0 < 0 || by0 < 0 || bx1 > (int)g_w || by1 > (int)g_h) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s label band "
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
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) shows %ld DESKTOP-teal pixels: the band was "
                "never filled (black=%ld white=%ld other=%ld)\n",
                g_leg, label, bx0, by0, bx1, by1, teal, black, white, other);
        g_fail = 1;
        return;
    }
    if (black == 0 || white == 0) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) is a single tone (black=%ld white=%ld "
                "other=%ld): band OR glyphs are missing\n",
                g_leg, label, bx0, by0, bx1, by1, black, white, other);
        g_fail = 1;
        return;
    }
    if (inverted && black <= white) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) is NOT inverted: black=%ld white=%ld "
                "(a SELECTED icon draws a black band with white text; "
                "design F1.1)\n",
                g_leg, label, bx0, by0, bx1, by1, black, white);
        g_fail = 1;
        return;
    }
    if (!inverted && white <= black) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: FAIL leg %s -- %s label band "
                "(%d,%d)-(%d,%d) is inverted when it should not be: "
                "black=%ld white=%ld\n",
                g_leg, label, bx0, by0, bx1, by1, black, white);
        g_fail = 1;
        return;
    }
    printf("    %-6s label band (%d,%d)-(%d,%d): %s (black=%ld white=%ld)\n",
           label, bx0, by0, bx1, by1,
           inverted ? "INVERTED" : "normal", black, white);
}

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

static void check_old_footprint(void)
{
    char what[256];
    int i;
    for (i = 0; i < OLD_FOOTPRINT_N; i++) {
        snprintf(what, sizeof what,
                 "the VOLUME's OLD cell must be repainted to bare desktop -- %s",
                 OLD_FOOTPRINT[i].why);
        assert_idx(OLD_FOOTPRINT[i].x, OLD_FOOTPRINT[i].y, CIDX_DESKTOP, what);
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[3] = {0, 0, 0};
    long maxv = 0;
    size_t want, got;
    int leg_default, leg_selected, leg_moved;

    if (argc != 3) {
        fprintf(stderr,
                "usage: %s <default|selected|moved> <dump.ppm>\n", argv[0]);
        return 2;
    }
    g_leg       = argv[1];
    leg_default  = (strcmp(g_leg, "default")  == 0);
    leg_selected = (strcmp(g_leg, "selected") == 0);
    leg_moved    = (strcmp(g_leg, "moved")    == 0);
    if (!leg_default && !leg_selected && !leg_moved) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: unknown leg '%s' "
                "(want default|selected|moved)\n", g_leg);
        return 2;
    }

    f = fopen(argv[2], "rb");
    if (!f) {
        fprintf(stderr, "ppm_flair_desktop_icons_check: cannot open %s\n",
                argv[2]);
        return 2;
    }
    if (fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6') {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: not a P6 PPM (got '%s')\n",
                magic);
        fclose(f);
        return 2;
    }
    if (read_uint(f, &g_w) != 0 || read_uint(f, &g_h) != 0 ||
        read_uint(f, &maxv) != 0) {
        fprintf(stderr, "ppm_flair_desktop_icons_check: malformed PPM header\n");
        fclose(f);
        return 2;
    }
    if (g_w != SCRW || g_h != SCRH || maxv != 255) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: unexpected dims/maxval "
                "w=%ld h=%ld max=%ld (want %dx%d/255)\n",
                g_w, g_h, maxv, SCRW, SCRH);
        fclose(f);
        return 2;
    }
    want = (size_t)g_w * (size_t)g_h * 3u;
    g_buf = (unsigned char *)malloc(want);
    if (!g_buf) {
        fprintf(stderr, "ppm_flair_desktop_icons_check: OOM\n");
        fclose(f);
        return 2;
    }
    got = fread(g_buf, 1, want, f);
    fclose(f);
    if (got != want) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: short raster (%zu of %zu "
                "bytes)\n", got, want);
        free(g_buf);
        return 2;
    }

    printf("ppm_flair_desktop_icons_check: leg %s on %s\n", g_leg, argv[2]);

    if (icap_load(ICON_CAPTURES_DIR, g_cap)) {
        fprintf(stderr, "ppm_flair_desktop_icons_check: FAIL -- the mint-008 golden "
                "(%s) is unavailable\n", ICON_CAPTURES_DIR);
        free(g_buf);
        return 1;
    }

    if (leg_moved) {
        check_sprite("VOLUME", VOL_MOVED_X0, VOL_MOVED_Y0, ICAP_VOLUME, ICAP_UNSEL);
        check_band("VOLUME", VOL_MOVED_X0, VOL_MOVED_Y0, 0);
        check_old_footprint();
    } else {
        check_sprite("VOLUME", VOL_X0, VOL_Y0, ICAP_VOLUME,
                     leg_selected ? ICAP_SEL : ICAP_UNSEL);
        check_band("VOLUME", VOL_X0, VOL_Y0, leg_selected ? 1 : 0);
    }

    /* The Trash is untouched in every leg: fixed position, never selected by
     * any of these traces (design F1.1 -- the Trash is not draggable, and the
     * volume click is a select_only, which CLEARS every other icon). */
    check_sprite("TRASH", TRASH_X0, TRASH_Y0, ICAP_TRASH, ICAP_UNSEL);
    check_band("TRASH", TRASH_X0, TRASH_Y0, 0);

    check_control_teal();

    free(g_buf);
    if (g_fail) {
        fprintf(stderr,
                "ppm_flair_desktop_icons_check: leg %s FAILED\n", g_leg);
        return 1;
    }
    printf("ppm_flair_desktop_icons_check: leg %s PASS "
           "(VOLUME + TRASH cells == the mint-008 captures, 2 label-band relations, "
           "%d bare-teal controls%s)\n",
           g_leg, CONTROL_TEAL_N,
           leg_moved ? ", 7 old-footprint restores" : "");
    return 0;
}
