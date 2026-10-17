/*
 * ppm_flair_trash_check.c -- the EMPTY TRASH screendump STRUCTURE oracle
 *                            (factory, C).
 *
 * bead: initech-6k12 (Special > Empty Trash: the confirm alert, the FULL Trash
 *       icon and its swap back).
 *
 * Legs (one QEMU screendump, P6 PPM 640x480, of the booted $(FLAIRTENANTS_IMG)):
 *   alert    README.TXT staged, Special > Empty Trash chosen, the alert UP:
 *              the alert frame at the corpus geometry, the message in three
 *              inked lines in the text column, OK with the default ring and
 *              Cancel without, and the desktop Trash drawn FULL.
 *   emptied  after OK: the alert gone (its footprint repainted by what lies
 *              beneath) and the desktop Trash drawn EMPTY again.
 *   tenant3  bead initech-fvnj: a disk tenant's TBX_ALERT with THREE buttons
 *              (Initech 123's Save / Cancel / Don't Save) up over its white
 *              document window: the same frame and message column, the
 *              default (item 1) in OK's measured place WITH the ring, Cancel
 *              in Cancel's place without, and the third button at x 211..294
 *              (spec/toolbox_gate.h Sec 11 -- a stated product adaptation,
 *              the corpus has no three-button alert) without a ring.
 *   tenant3-gone  after the answer: the footprint repainted by the white
 *              document content beneath (the worksheet's empty cells).
 *   exitdos  bead initech-3p1a: Special > Exit to InitechDOS's confirmation
 *              over the open root window -- the SAME alert (frame bands,
 *              Cancel bevel + corners against the capture), the message in
 *              exactly TWO inked lines ("Are you sure you want to exit to" /
 *              "InitechDOS?", test-menu-handlers H5 derives the wrap), the
 *              default (Exit) in OK's place WITH the ring, Cancel without.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2):
 *   - the alert geometry: MEASURED off the corpus capture
 *     ../system7-decomp/goldens/captures/s8_alert_modal.png (frame x=133..506
 *     y=87..190; text ink from x 211, cap tops y 100/116/132; Cancel
 *     x=363..421, OK x=435..493, ring x=432..496, buttons y=158..177 --
 *     specs/sys8/INDEX-sys8.md agrees), restated here by hand;
 *   - the Trash strikes and the caution icon: the mint-008 captures, cell for
 *     cell (harness/render/icon_capture.h; this file never includes a strike);
 *   - the colours: flair_canon_rgb (spec/assets/color_canon.h), the
 *     independently decomp-graded canon, never the renderer's palette.
 *   - L023 / M011 / M012: frame perimeter, Cancel bevel rows and corners
 *     are compared EXACTLY to the historical PNG decoded at runtime.
 *
 * Usage: ppm_flair_trash_check <alert|emptied> <dump.ppm>
 * Exit 0 iff every assertion passes; otherwise a loud message per failure.
 * Ref: CLAUDE.md Law 1, Law 2, Law 4, Rule 2, Rule 6, Rule 12.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + CIDX_* (-Ispec/assets) */
#include "../harness/render/alert_capture.h"
#include "../harness/render/icon_capture.h"

#ifndef ICON_CAPTURES_DIR
#error "build with -DICON_CAPTURES_DIR='\"<system7-decomp>/goldens/captures\"'"
#endif

#define TOL 2
enum { SCRW = 640, SCRH = 480 };

/* The alert, measured (see the banner). */
#define AL_L 133
#define AL_T  87
#define AL_R 507             /* exclusive: the capture's last column is 506   */
#define AL_B 191
#define AL_BORDER 7          /* retained layout inset, NOT a black paint band */
#define TXT_X0 211
#define TXT_X1 496           /* 211 + the 285-px column                        */
#define TXT_Y0  97           /* line k occupies [97+16k, 113+16k)               */
#define OK_L 435
#define OK_R 494
#define CA_L 363
#define CA_R 422
#define BT_T 158
#define BT_B 178
#define OT_L 211             /* the third button (spec Sec 11, authored)       */
#define OT_R 295

/* The desktop Trash sprite origin (spec/flair_trash_traces.mk: (584,404)). */
#define TR_X 584
#define TR_Y 404

static unsigned char *g_buf;
static int g_fail;
static const char *g_leg;

/* Independent runtime capture comparisons (Rule 6: booted LABEL_OPAQUE,
 * BTN_FRINGE and ALERT_SLAB images must fail the named respective class).
 * Ref: s8_alert_modal.png x133..506,y87..190 (frame); x365..418,y160/175
 * (bevel rows); x363..366/418..421,y158..161/174..177 (corner patches).
 */
static void alert_chrome(int button)
{
    unsigned char *gold = alert_capture_load();
    int frame_bad = 0, row_bad = 0, corner_bad = 0;
    if (!gold) { g_fail = 1; return; }
    for (int y = AL_T; y < AL_B; y++) {
        for (int x = AL_L; x < AL_R; x++) {
            int frame = x < 140 || x >= 500 || y < 94 || y >= 184;
            int row = button && x >= 365 && x <= 418 && (y == 160 || y == 175);
            int corner = button && ((x >= 363 && x <= 366) || (x >= 418 && x <= 421)) &&
                         ((y >= 158 && y <= 161) || (y >= 174 && y <= 177));
            if (!frame && !row && !corner) continue;
            uint32_t got = alert_capture_rgb(g_buf, x, y);
            int bad = got != alert_capture_rgb(gold, x, y);
            if (frame) frame_bad += bad;
            if (row) row_bad += bad;
            if (corner) corner_bad += bad;
        }
    }
    free(gold);
    printf("    ALERT-BANDS: %d/6496 matched; BUTTON-LABEL-BEVEL: %d/108; BUTTON-CORNERS: %d/64\n",
           6496 - frame_bad, button ? 108 - row_bad : 0, button ? 64 - corner_bad : 0);
    if (frame_bad) fprintf(stderr, "ppm_flair_trash_check: FAIL ALERT-BANDS: %d pixels\n", frame_bad);
    if (row_bad) fprintf(stderr, "ppm_flair_trash_check: FAIL BUTTON-LABEL-BEVEL: %d pixels\n", row_bad);
    if (corner_bad) fprintf(stderr, "ppm_flair_trash_check: FAIL BUTTON-CORNERS: %d pixels\n", corner_bad);
    if (frame_bad || row_bad || corner_bad) g_fail = 1;
}

static int read_uint(FILE *f, long *out)
{
    int c;
    long v = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

static int is_idx(int x, int y, int idx)
{
    const unsigned char *p = g_buf + ((long)y * SCRW + x) * 3;
    unsigned int e = (unsigned int)(flair_canon_rgb((unsigned char)idx) & 0x00FFFFFFu);
    return abs((int)p[0] - (int)((e >> 16) & 0xFFu)) <= TOL &&
           abs((int)p[1] - (int)((e >> 8) & 0xFFu)) <= TOL &&
           abs((int)p[2] - (int)(e & 0xFFu)) <= TOL;
}

static void want(int x, int y, int idx, const char *what)
{
    if (!is_idx(x, y, idx)) {
        const unsigned char *p = g_buf + ((long)y * SCRW + x) * 3;
        fprintf(stderr, "ppm_flair_trash_check: FAIL leg %s -- %s at (%d,%d): "
                "sampled #%02X%02X%02X, want canon idx %d\n",
                g_leg, what, x, y, p[0], p[1], p[2], idx);
        g_fail = 1;
    }
}

static long ink(int x0, int y0, int x1, int y1)
{
    long n = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (is_idx(x, y, CIDX_BLACK)) n++;
    return n;
}

static void want_ink(int x0, int y0, int x1, int y1, int some, const char *what)
{
    long n = ink(x0, y0, x1, y1);
    printf("    %-52s [%d,%d)x[%d,%d): %ld ink px\n", what, x0, x1, y0, y1, n);
    if (some ? (n == 0) : (n != 0)) {
        fprintf(stderr, "ppm_flair_trash_check: FAIL leg %s -- %s: %ld ink "
                "pixels in [%d,%d)x[%d,%d), want %s\n",
                g_leg, what, n, x0, x1, y0, y1, some ? "some" : "none");
        g_fail = 1;
    }
}

/* bead initech-tdnl.53 / tdnl.74 / tdnl.87: the Trash strikes and the alert's
 * caution icon are graded CELL FOR CELL, zero tolerance, against the
 * INDEPENDENT mint-008 Mac OS 8.1 captures (harness/render/icon_capture.h,
 * the build-time ICON_CAPTURES_DIR; a missing capture FAILS) -- replacing the
 * v1 hand-read ASCII-map probes. Clear cells must show the desktop teal (the
 * Trash) or the alert face (the caution icon). */
static icap_cell_t g_cap[ICAP__KINDS];
static int g_cap_ok;

static void icon_cell(int kind, int state, int x0, int y0, unsigned int bg, const char *what)
{
    int d;
    if (!g_cap_ok) {
        if (icap_load(ICON_CAPTURES_DIR, g_cap)) {
            fprintf(stderr, "ppm_flair_trash_check: FAIL leg %s -- the mint-008 golden "
                    "(%s) is unavailable\n", g_leg, ICON_CAPTURES_DIR);
            g_fail = 1;
            return;
        }
        g_cap_ok = 1;
    }
    d = icap_grade_frame(g_buf, SCRW, SCRH, x0, y0, &g_cap[kind], state, bg, what);
    printf("    %-52s @(%d,%d): %d/1024 cells differ from the capture\n", what, x0, y0, d);
    if (d) {
        fprintf(stderr, "ppm_flair_trash_check: FAIL leg %s -- %s: %d/1024 cells differ "
                "from the mint-008 capture\n", g_leg, what, d);
        g_fail = 1;
    }
}

static void trash_full(void)
{
    /* FLAIR_EMPTY_OK_SPEC double-clicks Trash before the mixed purge; the
     * locked-full notice leaves that selection intact. FLAIR_EMPTY_ALERT_SPEC
     * only stages README and opens the menu, so alert uses the plain can.
     * Ref: spec/flair_trash_traces.mk Sec 3; mint-008 Sec 3 selected colours.
     * Grade the traced state against its independent capture, zero tolerance. */
    int state = strcmp(g_leg, "locked-full") == 0 ? ICAP_SEL : ICAP_UNSEL;
#ifdef TRASH_CHECK_MUT_LOCKED_UNSELECTED
    state = ICAP_UNSEL; /* Rule 6: restore the stale locked-full expectation. */
#endif
    icon_cell(ICAP_TRASH_FULL, state, TR_X, TR_Y, (flair_canon_rgb((unsigned char)CIDX_DESKTOP) & 0x00FFFFFFu), "FULL Trash (TRASH_FULL)");
}

/* `state`: the emptied leg's Trash was double-clicked open, so it is the
 * SELECTED can (the captured selected form); the locked leg's is not. */
static void trash_empty(int state)
{
    icon_cell(ICAP_TRASH, state, TR_X, TR_Y, (flair_canon_rgb((unsigned char)CIDX_DESKTOP) & 0x00FFFFFFu), "EMPTY Trash (TRASH)");
}

/* The Empty Trash confirm's caution icon (tdnl.87): the iconItem at the
 * capture's icon cell (156,100), on the alert face; nothing else is inked
 * left of the text column around it. */
#define CAU_X 156
#define CAU_Y 100
static void caution(void)
{
    icon_cell(ICAP_CAUTION, ICAP_UNSEL, CAU_X, CAU_Y, (flair_canon_rgb((unsigned char)CIDX_PLAT_FACE) & 0x00FFFFFFu), "the CAUTION icon");
    want_ink(AL_L + AL_BORDER + 1, TXT_Y0, CAU_X, TXT_Y0 + 48, 0, "nothing left of the icon");
    want_ink(CAU_X + 32, TXT_Y0, TXT_X0 - 2, TXT_Y0 + 48, 0, "nothing between the icon and the text");
    want_ink(CAU_X, TXT_Y0, CAU_X + 32, CAU_Y, 0, "nothing above the icon");
    want_ink(CAU_X, CAU_Y + 32, CAU_X + 32, TXT_Y0 + 48, 0, "nothing below the icon");
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[3] = {0, 0, 0};
    long w = 0, h = 0, maxv = 0;
    size_t n = (size_t)SCRW * SCRH * 3u;

    if ((argc != 3 && !(argc == 4 && !strcmp(argv[1], "pressed"))) || (strcmp(argv[1], "alert") && strcmp(argv[1], "emptied") &&
                     strcmp(argv[1], "locked") && strcmp(argv[1], "locked-full") &&
                     strcmp(argv[1], "tenant3") && strcmp(argv[1], "tenant3-gone") &&
                     strcmp(argv[1], "chrome") && strcmp(argv[1], "notice") && strcmp(argv[1], "pressed") && strcmp(argv[1], "exitdos"))) {
        fprintf(stderr, "usage: %s <alert|emptied|locked|locked-full|tenant3|tenant3-gone|chrome|exitdos> <dump.ppm>\n", argv[0]);
        return 2;
    }
    g_leg = argv[1];
    f = fopen(argv[2], "rb");
    if (!f || fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6' ||
        read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv) ||
        w != SCRW || h != SCRH || maxv != 255) {
        fprintf(stderr, "ppm_flair_trash_check: %s is not a 640x480/255 P6 PPM\n", argv[2]);
        if (f) fclose(f);
        return 2;
    }
    g_buf = (unsigned char *)malloc(n);
    if (!g_buf || fread(g_buf, 1, n, f) != n) {
        fprintf(stderr, "ppm_flair_trash_check: short raster\n");
        fclose(f);
        return 2;
    }
    fclose(f);
    printf("ppm_flair_trash_check: leg %s on %s\n", g_leg, argv[2]);

    if (strcmp(g_leg,"pressed") == 0) {
        unsigned char *gold;
        int bad=0;
        if(argc!=4 || !(gold=alert_capture_load_path(argv[3]))) { free(g_buf); return 1; }
        /* Default OK's left bevel/face: native rect x435..493,y158..177.
         * Keep clear of its label and the pointer held at (464,168). The
         * EXPECTED pixels are solely the native pressed capture, never a
         * palette or control-drawing table. Missing capture is a hard RED. */
        for(int y=158;y<178;y++) for(int x=435;x<451;x++)
            bad+=alert_capture_rgb(g_buf,x,y)!=alert_capture_rgb(gold,x,y);
        free(gold);
        if(bad) {fprintf(stderr,"FAIL PRESSED-CAPTURE: %d/320 pixels differ\n",bad);g_fail=1;}
        else printf("PRESSED-CAPTURE: 320/320 pixels matched native pressed OK\n");
    } else if (strcmp(g_leg, "notice") == 0) {
        alert_chrome(0);
    } else if (strcmp(g_leg, "chrome") == 0) {
        alert_chrome(1);
    } else if (strcmp(g_leg, "exitdos") == 0) {
        alert_chrome(1);
        want(AL_L - 1, 140, CIDX_WHITE, "just outside the alert (root window content)");
        want(300, 150, CIDX_PLAT_FACE, "alert face between the message and the buttons");
        for (int k = 0; k < 2; k++) {
            char what[64];
            snprintf(what, sizeof what, "message line %d", k + 1);
            want_ink(TXT_X0, TXT_Y0 + 16 * k, TXT_X1, TXT_Y0 + 16 * (k + 1), 1, what);
        }
        want_ink(TXT_X0, TXT_Y0 + 32, CA_L - 3, BT_B, 0, "no third message line");
        want_ink(AL_L + AL_BORDER + 1, TXT_Y0, TXT_X0 - 2, TXT_Y0 + 32, 0,
                 "nothing left of the text column");
        /* line 2 is the one word "InitechDOS?" (78 px): no ink past x 211+78+4 */
        want_ink(TXT_X0 + 82, TXT_Y0 + 16, TXT_X1, TXT_Y0 + 32, 0,
                 "line 2 is the single wrapped word");
        want_ink(OK_L + 4, BT_T + 3, OK_R - 4, BT_B - 2, 1, "default (Exit) button label");
        want_ink(CA_L + 4, BT_T + 3, CA_R - 4, BT_B - 2, 1, "Cancel button label");
        want((OK_L + OK_R) / 2, BT_T - 3, CIDX_BLACK, "Exit default ring (InsetRect -3)");
        want((CA_L + CA_R) / 2, BT_T - 3, CIDX_PLAT_FACE, "Cancel has NO ring");
    } else if (strcmp(g_leg, "tenant3") == 0) {
        alert_chrome(1);
        want(AL_L - 1, 150, CIDX_WHITE, "just outside the alert (the document content)");
        want(300, 150, CIDX_PLAT_FACE, "alert face between the message and the buttons");
        for (int k = 0; k < 3; k++) {
            char what[64];
            snprintf(what, sizeof what, "message line %d", k + 1);
            want_ink(TXT_X0, TXT_Y0 + 16 * k, TXT_X1, TXT_Y0 + 16 * (k + 1), 1, what);
        }
        want_ink(TXT_X0, TXT_Y0 + 48, TXT_X1, BT_T - 4, 0, "no fourth message line");
        want_ink(AL_L + AL_BORDER + 1, TXT_Y0, TXT_X0 - 2, BT_B, 0,
                 "nothing left of the text column");
        want_ink(OK_L + 4, BT_T + 3, OK_R - 4, BT_B - 2, 1, "default (Save) button label");
        want_ink(CA_L + 4, BT_T + 3, CA_R - 4, BT_B - 2, 1, "Cancel button label");
        want_ink(OT_L + 4, BT_T + 3, OT_R - 4, BT_B - 2, 1, "third (Don't Save) button label");
        want((OK_L + OK_R) / 2, BT_T - 3, CIDX_BLACK, "default ring (InsetRect -3)");
        want((CA_L + CA_R) / 2, BT_T - 3, CIDX_PLAT_FACE, "Cancel has NO ring");
        want((OT_L + OT_R) / 2, BT_T - 3, CIDX_PLAT_FACE, "the third button has NO ring");
        want((OT_L + OT_R) / 2, BT_T, CIDX_BLACK, "the third button's top edge");
        want(OT_R + 4, (BT_T + BT_B) / 2, CIDX_PLAT_FACE, "face right of the third button");
    } else if (strcmp(g_leg, "tenant3-gone") == 0) {
        want(AL_L + AL_BORDER / 2, 150, CIDX_WHITE, "the alert is gone: left band repainted");
        want(AL_R - 1 - AL_BORDER / 2, 150, CIDX_WHITE, "the alert is gone: right band repainted");
        want(300, AL_B - 1 - AL_BORDER / 2, CIDX_WHITE, "the alert is gone: bottom band repainted");
        want((OK_L + OK_R) / 2, BT_T, CIDX_WHITE, "the alert is gone: no button edge");
    } else if (strcmp(g_leg, "alert") == 0 || strcmp(g_leg, "locked") == 0 ||
        strcmp(g_leg, "locked-full") == 0) {
        alert_chrome(strcmp(g_leg, "alert") == 0);
        want(AL_L - 1, 140, CIDX_WHITE, "just outside the alert (root window content)");
        want(300, 150, CIDX_PLAT_FACE, "alert face between the message and the buttons");
        /* the message: three inked lines in the text column, nothing below */
        for (int k = 0; k < 3; k++) {
            char what[64];
            snprintf(what, sizeof what, "message line %d", k + 1);
            want_ink(TXT_X0, TXT_Y0 + 16 * k, TXT_X1, TXT_Y0 + 16 * (k + 1), 1, what);
        }
        want_ink(TXT_X0, TXT_Y0 + 48, CA_L - 3, BT_B, 0, "no fourth message line");
        if (strcmp(g_leg, "alert") == 0)
            caution();   /* tdnl.87: the confirm is the captured CAUTION alert */
        else
            want_ink(AL_L + AL_BORDER + 1, TXT_Y0, TXT_X0 - 2, TXT_Y0 + 48, 0,
                     "nothing left of the text column (the locked notice has no icon)");
        /* the buttons: labels inked; OK carries the default ring, Cancel not */
        want_ink(OK_L + 4, BT_T + 3, OK_R - 4, BT_B - 2, 1, "OK button label");
        want_ink(CA_L + 4, BT_T + 3, CA_R - 4, BT_B - 2,
                 strcmp(g_leg, "alert") == 0, "Cancel only in confirmation");
        want((OK_L + OK_R) / 2, BT_T - 3, CIDX_BLACK, "OK default ring (InsetRect -3)");
        want((CA_L + CA_R) / 2, BT_T - 3, CIDX_PLAT_FACE, "Cancel has NO ring");
        if (strcmp(g_leg, "locked") == 0) trash_empty(ICAP_UNSEL); else trash_full();
    } else {
        want(AL_L + AL_BORDER / 2, 140, CIDX_WHITE,
             "the alert is gone: its left band repainted by the Trash window content");
        want(300, AL_B - 1 - AL_BORDER / 2, CIDX_WHITE,
             "the alert is gone: its bottom band repainted by the Trash window content");
        trash_empty(ICAP_SEL);
    }
    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_trash_check: leg %s FAILED\n", g_leg);
        return 1;
    }
    printf("ppm_flair_trash_check: leg %s PASS\n", g_leg);
    return 0;
}
