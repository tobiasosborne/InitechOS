/* test_shell.c -- the FLAIR DESKTOP SHELL M4 GATE (THE ORACLE; ADR-0004 D-8).
 *
 * beads: initech-k8o5.12 (M4 Manager set + two stacked menu bars + modal FILE
 *        COPY layer + desktop shell) and initech-859 (Desktop shell: reproduce
 *        the frame chrome -- the M4 gate). The mechanical oracle (Law 2) for the
 *        M4 capstone that REPRODUCES THE OFFICE SPACE FRAME: it renders the
 *        composed InitechOS desktop -- seafoam background + document windows +
 *        the TWO STACKED MENU BARS (the chimera) + the modal FILE COPY box -- to
 *        a 640x480 offscreen and asserts the frame STRUCTURE at the pixel level.
 *
 * Ref:   ADR-0004 D-1 (Layer 5 desktop shell), D-2 (one surface module), D-3
 *        (Managers; the Photoshop bar), D-5 (z-order), D-8 (the oracle vector).
 *        os/flair/shell.{c,h} (the unit under test). os/flair/{desktop,menu,
 *        dialog,window,chrome,blitter,surface,control,text}.c (composed).
 *        spec/assets/menu_canon.h (the FROZEN canon string -- asserted byte-
 *        exact, Law 4). os/flair/dialog.h (FLAIR_CANON_FILECOPY_MSG, byte-exact).
 *        spec/chrome_metrics.h (FLAIR_CHROME_* -- title bar 19, frame 1,
 *        scrollbar 16, menubar 20). harness/render/render.h (host skeleton; the
 *        dual-compile path that runs the SAME freestanding shell.c on a host
 *        offscreen, AM-1 geometry parameter). harness/proptest/test_drag.c +
 *        test_menu.c + test_dialog.c (the harness idiom this suite mirrors).
 *        CLAUDE.md Law 2 (oracle is truth), Law 4 (look like the frame -- the PPM
 *        is the human audit), Rule 6 (mutation-proven), Rule 11 (deterministic),
 *        Rule 12 (ASCII-clean).
 *
 * THE STRUCTURAL ASSERTIONS (the M4 "reproduce the frame" gate):
 *
 *   1. SEAFOAM DESKTOP present -- sampled bare-desktop pixels read the seafoam
 *      desktop index (RENDER_DESKTOP_INDEX == FLAIR_DESKTOP_BG_INDEX).
 *   2. BOTH MENU BARS present at their y-bands (each FLAIR_MENUBAR_H = 20 px):
 *      the System-7 bar at rows [0,20), the Photoshop bar at rows [20,40), each
 *      with a painted baseline + bar fill + title ink. The Photoshop bar's title
 *      string is asserted byte-EXACT against menu_canon.h (Law 4 canon chimera).
 *   3. WINDOW CHROME present -- the front window's pinstripe title bar (period 2
 *      alternation), the 1 px frame, and the 16 px scrollbar, vs chrome_metrics.
 *   4. The FILE COPY MODAL present + CENTERED: the dBoxProc 7-px border box at
 *      the canonical centered bounds, the byte-exact "Saving tables to disk..."
 *      static text painted as ink, and the progress bar.
 *   5. The MODAL is ON TOP -- a probe point that lies BOTH inside the modal's
 *      solid border AND inside a document window reads the MODAL's pixel (the
 *      modal occludes the window: z-order / clip correct).
 *   6. DETERMINISM -- two renders of the same scene are byte-identical (Rule 11).
 *
 * MUTANTS (Rule 6; in shell.c, -D guarded), each MUST drive this gate RED:
 *   SHELL_MUTATE_ONE_MENUBAR  -- drop the second (Photoshop) menu bar -> the
 *                                Photoshop-bar band reads as bare desktop ->
 *                                assertion 2 (both bars present) RED.
 *   SHELL_MUTATE_NO_MODAL     -- skip the FILE COPY modal -> the box + the canon
 *                                text are absent and the windows show through ->
 *                                assertions 4 + 5 RED.
 *   SHELL_MUTATE_MODAL_BEHIND -- draw the modal BEHIND the windows/bars -> the
 *                                modal is occluded instead of occluding ->
 *                                assertion 5 (modal on top) RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* host render skeleton (-Iharness/render)     */
#include "region_algebra.h"     /* the LOCKED region contract (-Ispec)         */
#include "region.h"             /* engine constructors (-Ios/flair/atkinson)   */
#include "window_record.h"      /* WindowRecord, part-codes (-Ispec)           */
#include "shell.h"              /* the DESKTOP SHELL under test (-Ios/flair)   */
#include "desktop.h"            /* FLAIR_DESKTOP_BG_INDEX (-Ios/flair)         */
#include "menu_canon.h"         /* FROZEN Photoshop canon string (-Ispec/assets)*/
#include "dialog.h"             /* FLAIR_CANON_FILECOPY_MSG (-Ios/flair)       */
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                     */
#include "palette.h"            /* INITECH_DESKTOP_BG_RGB (-Ispec/assets)      */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

TEST_HARNESS();

/* The native port (ADR-0004 OD-3). The offscreen IS the screen, so global coords
 * == port-local coords == offscreen pixel coords. */
enum { SCRW = 640, SCRH = 480 };

/* ===========================================================================
 * Region storage bundle (the test_drag idiom: the engine never mallocs; the
 * caller supplies rows[]/x_pool). One bundle per region.
 * ===========================================================================*/
typedef struct rgn_store {
    region_t   r;
    rgn_row_t  rows[RGN_ROWS_CAP];
    int16_t    pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows       = s->rows;
    s->r.cap_rows   = RGN_ROWS_CAP;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

/* ===========================================================================
 * The whole shell-scene storage bundle, all caller-supplied (Law 3; no malloc).
 * ===========================================================================*/
enum { N_WINDOWS = 2 };
enum { N_SYS_MENUS = 4 };  /* File / Edit / View / Special (System-7 bar)       */

/* Per-window region trio. */
typedef struct win_rgns {
    rgn_store_t struc, cont, upd;
} win_rgns_t;

typedef struct shell_store {
    shell_scene_t      scene;

    /* Window Manager regions. */
    rgn_store_t        desk, sa, sb, sc, comp;

    /* The document windows + their regions. */
    shell_window_store wins[N_WINDOWS];
    win_rgns_t         winr[N_WINDOWS];

    /* The System-7 bar menus + a tiny item list each. */
    MenuInfo           sys_menus[N_SYS_MENUS];
    MenuItem           sys_items[N_SYS_MENUS][2];

    /* The Photoshop bar menus + a tiny item list each (titles set from canon). */
    MenuInfo           ps_menus[FLAIR_CANON_PHOTOSHOP_MENU_COUNT];
    MenuItem           ps_items[FLAIR_CANON_PHOTOSHOP_MENU_COUNT][2];

    /* The FILE COPY modal storage. */
    DialogRecord       dlg;
    DialogItem         dlg_items[2];
    ControlRecord      dlg_progress;
    rgn_store_t        dlg_struc, dlg_cont, dlg_upd;
} shell_store_t;

/* The two document windows, placed per the frame: window 0 (back, upper-left)
 * overlaps the centered FILE COPY box; window 1 (front, lower-right). Both sit
 * BELOW the two stacked menu bars (rows < SHELL_MENUBARS_H = 40 are the bars). */
#define W0_T  80
#define W0_L  60
#define W0_B  300
#define W0_R  360
#define W1_T  120
#define W1_L  300
#define W1_B  360
#define W1_R  560

static const rgn_rect_t g_win_bounds[N_WINDOWS] = {
    { W0_T, W0_L, W0_B, W0_R },   /* window 0 (back)  */
    { W1_T, W1_L, W1_B, W1_R }    /* window 1 (front) */
};
static const char *const g_win_titles[N_WINDOWS] = {
    "untitled-1",
    "Saving tables to disk"
};

/* The System-7 bar titles (File/Edit/View/Special -- the classic System-7 set,
 * gui-ground-truth.md Sec 4). These are NOT the canon Photoshop bar (that is the
 * second bar, stamped from menu_canon.h by the shell). */
static const char *const g_sys_titles[N_SYS_MENUS] = {
    "File", "Edit", "View", "Special"
};

/* Build the scene into the caller bundle. */
static void build_shell(shell_store_t *S, int show_modal)
{
    memset(S, 0, sizeof *S);

    store_attach(&S->desk);
    store_attach(&S->sa);
    store_attach(&S->sb);
    store_attach(&S->sc);
    store_attach(&S->comp);

    for (int i = 0; i < N_WINDOWS; i++) {
        store_attach(&S->winr[i].struc);
        store_attach(&S->winr[i].cont);
        store_attach(&S->winr[i].upd);
        S->wins[i].strucRgn  = &S->winr[i].struc.r;
        S->wins[i].contRgn   = &S->winr[i].cont.r;
        S->wins[i].updateRgn = &S->winr[i].upd.r;
    }

    /* System-7 bar: each menu gets a 1-item list so it is well-formed. */
    for (int mi = 0; mi < N_SYS_MENUS; mi++) {
        S->sys_items[mi][0] = (MenuItem){ "About", 0, 0, 0, 1, 0 };
        S->sys_items[mi][1] = (MenuItem){ "Quit",  0, 'Q', 0, 1, 0 };
        S->sys_menus[mi].menuID    = (int16_t)(128 + mi);
        S->sys_menus[mi].title     = g_sys_titles[mi];
        S->sys_menus[mi].items     = S->sys_items[mi];
        S->sys_menus[mi].n_items   = 2;
        S->sys_menus[mi].menuWidth = 0;
    }

    /* Photoshop bar: the shell stamps the canon titles; we supply item lists. */
    for (int mi = 0; mi < FLAIR_CANON_PHOTOSHOP_MENU_COUNT; mi++) {
        S->ps_items[mi][0] = (MenuItem){ "New",  0, 'N', 0, 1, 0 };
        S->ps_items[mi][1] = (MenuItem){ "Open", 0, 'O', 0, 1, 0 };
        S->ps_menus[mi].title     = (const char *)0;  /* set by shell from canon */
        S->ps_menus[mi].items     = S->ps_items[mi];
        S->ps_menus[mi].n_items   = 2;
        S->ps_menus[mi].menuWidth = 0;
    }

    store_attach(&S->dlg_struc);
    store_attach(&S->dlg_cont);
    store_attach(&S->dlg_upd);

    rgn_rect_t frame = { 0, 0, SCRH, SCRW };
    shell_build_scene(&S->scene,
                      frame,
                      S->wins, N_WINDOWS, g_win_bounds, g_win_titles,
                      &S->desk.r, &S->sa.r, &S->sb.r, &S->sc.r, &S->comp.r,
                      S->sys_menus, N_SYS_MENUS,
                      S->ps_menus,
                      &S->dlg, S->dlg_items, &S->dlg_progress,
                      &S->dlg_struc.r, &S->dlg_cont.r, &S->dlg_upd.r,
                      show_modal);
}

/* ===========================================================================
 * Independent pixel snapshot (raw index bytes) for determinism + occlusion.
 * ===========================================================================*/
typedef struct snap { uint8_t px[SCRW * SCRH]; } snap_t;

static void snapshot(const render_ctx_t *ctx, snap_t *s)
{
    for (uint32_t y = 0; y < (uint32_t)SCRH; y++)
        for (uint32_t x = 0; x < (uint32_t)SCRW; x++)
            s->px[y * SCRW + x] =
                (uint8_t)(render_pixel_index(ctx, x, y) & 0xFFu);
}

static int idx_at(const render_ctx_t *ctx, int x, int y)
{
    return (int)(render_pixel_index(ctx, (uint32_t)x, (uint32_t)y) & 0xFFu);
}

/* ===========================================================================
 * Init an 8bpp 640x480 render context (AM-1: geometry is a runtime parameter).
 * ===========================================================================*/
static int init_ctx(render_ctx_t *ctx)
{
    render_boot_info_t boot;
    memset(&boot, 0, sizeof boot);
    boot.lfb_addr   = 0xE0000000u;   /* plausible VBE PhysBasePtr (ignored host) */
    boot.lfb_pitch  = 0u;            /* tight: width*bpp/8                       */
    boot.lfb_bpp    = 8u;            /* indexed-8 (OD-2)                         */
    boot.lfb_width  = SCRW;
    boot.lfb_height = SCRH;
    return render_ctx_init(ctx, &boot);
}

/* ===========================================================================
 * MAIN -- the M4 desktop-shell gate.
 * ===========================================================================*/
int main(int argc, char **argv)
{
    char msg[256];

    /* The canonical composed scene WITH the modal up (the Office Space frame). */
    static shell_store_t S;
    build_shell(&S, 1 /* show_modal */);

    render_ctx_t ctx;
    int rc = init_ctx(&ctx);
    CHECK(rc == 0, "render_ctx_init(8bpp 640x480) must succeed (AM-1 geometry param)");
    if (rc != 0) {
        return TEST_SUMMARY("test-shell");
    }

    shell_render(&S.scene, &ctx.fb.bm);

    /* ======================================================================
     * 1. SEAFOAM DESKTOP present. Below both bars and clear of every window +
     * the modal, the bare desktop must read the seafoam index. Probe the
     * bottom-left corner (well below window 0's bottom=300 and clear of all).
     * ====================================================================== */
    {
        /* (20, 460): below all windows (W0 bottom 300, W1 bottom 360) and left
         * of the modal (left 140); below the bars (>= 40). Bare desktop. */
        CHECK(idx_at(&ctx, 20, 460) == (int)FLAIR_DESKTOP_BG_INDEX,
              "(1) bare desktop bottom-left reads the seafoam desktop index");
        CHECK(idx_at(&ctx, 600, 460) == (int)FLAIR_DESKTOP_BG_INDEX,
              "(1) bare desktop bottom-right reads the seafoam desktop index");
        /* The seafoam index maps to the OD-4 seafoam RGB through the palette. */
        CHECK((render_pixel_rgb(&ctx, 20u, 460u) & 0x00FFFFFFu) ==
              (uint32_t)INITECH_DESKTOP_BG_RGB,
              "(1) seafoam index maps to INITECH_DESKTOP_BG_RGB (OD-4)");
    }

    /* ======================================================================
     * 2. BOTH MENU BARS present at their 20px y-bands + the Photoshop canon.
     * ====================================================================== */
    {
        CHECK(FLAIR_MENUBAR_H == 20 && FLAIR_CHROME_MENUBAR_H == 20,
              "(2) menu bar height is the locked 20 px (GetMBarHeight)");

        /* Bar 1 (System-7), rows [0,20). The bar fill is menubar gray (idx 3);
         * the baseline at y=19 is ink (idx 0). Probe a non-title x (far right,
         * past the System-7 titles) for the bar fill. */
        CHECK(idx_at(&ctx, 600, 5) == 3,
              "(2) System-7 bar (rows [0,20)) is painted bar fill (menubar gray)");
        CHECK(idx_at(&ctx, 300, FLAIR_MENUBAR_H - 1) == 0,
              "(2) System-7 bar baseline (y=19) is painted ink");

        /* Bar 2 (Photoshop), rows [20,40). Same fill + baseline, at +20. */
        CHECK(idx_at(&ctx, 600, FLAIR_MENUBAR_H + 5) == 3,
              "(2) Photoshop bar (rows [20,40)) is painted bar fill (menubar gray)");
        CHECK(idx_at(&ctx, 300, 2 * FLAIR_MENUBAR_H - 1) == 0,
              "(2) Photoshop bar baseline (y=39) is painted ink");

        /* The Photoshop bar carries the FROZEN canon string, byte-EXACT (Law 4).
         * The shell stamps the titles from menu_canon.h; assert item-by-item AND
         * as the flat canon string. */
        const MenuBar *pb = &S.scene.bar_photoshop;
        CHECK(pb->n_menus == FLAIR_CANON_PHOTOSHOP_MENU_COUNT,
              "(2) Photoshop bar has exactly 8 menus (menu_canon.h)");
        int item_bad = 0;
        for (int k = 0; k < (int)pb->n_menus; k++)
            if (!pb->menus[k].title ||
                strcmp(pb->menus[k].title,
                       FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[k]) != 0)
                item_bad = 1;
        CHECK(!item_bad,
              "(2) Photoshop bar titles == menu_canon.h item-by-item (Law 4)");

        char flat[128]; size_t pos = 0; flat[0] = 0;
        for (int k = 0; k < (int)pb->n_menus; k++) {
            const char *t = pb->menus[k].title ? pb->menus[k].title : "";
            size_t tl = strlen(t);
            if (k) flat[pos++] = ' ';
            memcpy(flat + pos, t, tl); pos += tl;
        }
        flat[pos] = 0;
        CHECK_STR_EQ(flat, FLAIR_CANON_PHOTOSHOP_MENUBAR_EXPECTED,
                     "(2) Photoshop bar flat string == FROZEN menu_canon.h (Law 4)");
        /* The two chimera tells: 'Layer' at 3 and 'View' at 5 (the canon signal). */
        CHECK(strcmp(pb->menus[3].title, "Layer") == 0,
              "(2) canon item 3 is 'Layer' (the chimera signal -- NOT 'Mode')");
        CHECK(strcmp(pb->menus[5].title, "View") == 0,
              "(2) canon item 5 is 'View' (the chimera signal -- NOT 'Filter')");

        /* The Photoshop bar's 'File' title actually paints INK in its slot at the
         * second bar's y-band (proves the canon bar is RENDERED, not just built).
         * 'File' is the first title; its slot starts at x=0 (no Apple slot on the
         * Photoshop bar). Text ink lands within [PAD, PAD+text] at the bar VPAD. */
        int ps_text_x0 = FLAIR_MENU_TITLE_PAD;
        int ps_text_w  = text_measure(FONT_CHICAGO, "File");
        int ink_found = 0;
        for (int x = ps_text_x0; x < ps_text_x0 + ps_text_w && !ink_found; x++)
            for (int y = SHELL_MENUBAR2_TOP + FLAIR_MENU_TITLE_VPAD;
                 y < SHELL_MENUBAR2_TOP + FLAIR_MENU_TITLE_VPAD + 16; y++)
                if (idx_at(&ctx, x, y) == 0) { ink_found = 1; break; }
        CHECK(ink_found,
              "(2) Photoshop bar 'File' title ink is RENDERED in the second band");
    }

    /* ======================================================================
     * 3. WINDOW CHROME present (the front window 1, lower-right, clear of the
     * modal): pinstripe title bar (period 2), the 1 px frame, the 16 px
     * scrollbar -- vs chrome_metrics. Window 1 is fully visible (front, and its
     * right/bottom edges land over bare desktop, right of the modal x<500).
     * ====================================================================== */
    {
        const int fr = FLAIR_CHROME_FRAME;
        const int title_top = W1_T + fr;
        const int title_bot = title_top + FLAIR_CHROME_TITLEBAR_H;
        const int mid_x = (W1_L + W1_R) / 2;
        /* The pinstripe is scanned at a clear column -- right of the close box and
         * LEFT of the centered window title ("untitled-2"), so the title knockout/
         * glyphs do not interrupt the stripe run (beads initech-lxg9). */
        const int pin_x = W1_L + 24;

        /* Title-bar pinstripe is STRIPED with the two WDEF shades (7/8). The
         * specific System-7 PHASE (the patAlign mod-8 doubled-LIGHT pairs) is NOT
         * asserted here -- it is graded against the INDEPENDENT decomp golden by
         * test-chrome-fidelity (beads initech-hmll). The previous strict-period-2
         * check was WRONG: the real phase-locked stripe is not strict period-2, so
         * it accepted the free-running L,D,L,D bug and would reject the correct
         * render. */
        int saw_light = 0, saw_dark = 0, striped = 0, prev = -1;
        for (int k = 0; k < FLAIR_CHROME_TITLEBAR_H; k++) {
            int s = idx_at(&ctx, pin_x, title_top + k);
            if (s == FLAIR_CHROME_TITLE_SHADE_LIGHT) saw_light = 1;
            if (s == FLAIR_CHROME_TITLE_SHADE_DARK)  saw_dark = 1;
            if (k > 0 && s != prev) striped = 1;
            prev = s;
        }
        CHECK(saw_light && saw_dark,
              "(3) front window title bar shows both WDEF shades (light 7 + dark 8)");
        CHECK(striped, "(3) front window title-bar pinstripe is STRIPED");

        /* The row just below the title bar is the white body (idx 1) -- proves
         * the title bar is EXACTLY FLAIR_CHROME_TITLEBAR_H tall. */
        CHECK(idx_at(&ctx, mid_x, title_bot) == 1,
              "(3) front window row below title bar is white body -- title height exact");

        /* Frame: exactly 1 px. The right frame column is painted; the pixel just
         * outside is the bare seafoam desktop (W1_R=560 < SCRW; right of modal). */
        int content_top = title_bot;
        int content_bot = W1_B - fr;
        int row = (content_top + content_bot) / 2;
        CHECK(idx_at(&ctx, W1_R - 1, row) != (int)FLAIR_DESKTOP_BG_INDEX,
              "(3) front window right frame column is painted");
        CHECK(idx_at(&ctx, W1_R, row) == (int)FLAIR_DESKTOP_BG_INDEX,
              "(3) pixel just right of the frame is bare seafoam (frame 1 px)");

        /* Scrollbar: EXACTLY 16 px. The scrollbar's left divider (ink, idx 0)
         * sits at inner_right - 16; the content just left of it is body (idx 1). */
        int inner_right = W1_R - fr;
        int sb_left = inner_right - FLAIR_CHROME_SCROLLBAR_W;
        snprintf(msg, sizeof msg,
                 "(3) front window scrollbar EXACTLY %d px -- divider (idx 0) at -%d",
                 FLAIR_CHROME_SCROLLBAR_W, FLAIR_CHROME_SCROLLBAR_W);
        CHECK(idx_at(&ctx, sb_left, row) == 0, msg);
        CHECK(idx_at(&ctx, sb_left - 1, row) == 1,
              "(3) content just left of the 16 px scrollbar is body (idx 1)");
    }

    /* ======================================================================
     * 4. The FILE COPY MODAL present + CENTERED + the byte-exact canon text +
     * the progress bar. The dialog is at the canonical bounds {140,200,500,280}
     * (centered on 640x480: center (320,240); 360x80). The dBoxProc border is
     * FLAIR_CHROME_DIALOG_BORDER (7) px of black (idx 0); inside is white (idx 1).
     * ====================================================================== */
    {
        const int dl = 140, dt = 200, dr = 500, db = 280;
        const int bw = FLAIR_CHROME_DIALOG_BORDER;  /* 7 */

        /* Centered: the box center is the screen center. */
        CHECK((dl + dr) / 2 == SCRW / 2 && (dt + db) / 2 == SCRH / 2,
              "(4) FILE COPY box is centered on the 640x480 desktop");

        /* The 7-px border band is black on all four sides. */
        CHECK(idx_at(&ctx, dl + 3, 240) == 0,
              "(4) FILE COPY left border (x=143) is black (dBoxProc 7 px)");
        CHECK(idx_at(&ctx, dr - 4, 240) == 0,
              "(4) FILE COPY right border (x=496) is black (dBoxProc 7 px)");
        CHECK(idx_at(&ctx, 320, dt + 3) == 0,
              "(4) FILE COPY top border (y=203) is black (dBoxProc 7 px)");
        CHECK(idx_at(&ctx, 320, db - 4) == 0,
              "(4) FILE COPY bottom border (y=276) is black (dBoxProc 7 px)");

        /* The first content row after the 7-px border is white (idx 1) -- proves
         * the border is EXACTLY 7 px (the 8th row in is content). */
        CHECK(idx_at(&ctx, 320, dt + bw) == 1,
              "(4) FILE COPY content interior just inside the 7 px border is white");

        /* The byte-EXACT canon string "Saving tables to disk..." is the dialog's
         * static text item (Law 4). Assert it on the BUILT dialog (byte-exact). */
        char buf[256];
        GetDialogItemText(S.scene.dlg, 1, buf, sizeof buf);
        snprintf(msg, sizeof msg,
                 "(4) FILE COPY item 1 text byte-EXACT '%.40s' (Law 4 canon)",
                 FLAIR_CANON_FILECOPY_MSG);
        CHECK(strcmp(buf, FLAIR_CANON_FILECOPY_MSG) == 0, msg);

        /* And it is RENDERED: the static-text item rect (left=154, top=212) has
         * painted text ink within the text band -- proves the modal's text is
         * drawn, not just stored. DrawDialog draws statText with DLG_TEXT_INK
         * (index 4; the title-ink shade, dialog.c) on the white (idx 1) body, so
         * "text ink" here is "a non-body pixel inside the text band". */
        int tx0 = 154, ty0 = 212;
        int tw  = text_measure(FONT_CHICAGO, FLAIR_CANON_FILECOPY_MSG);
        int text_ink = 0;
        for (int x = tx0; x < tx0 + tw && !text_ink; x++)
            for (int y = ty0; y < ty0 + 16; y++) {
                int v = idx_at(&ctx, x, y);
                if (v != 1) { text_ink = 1; break; }   /* not the white body fill */
            }
        CHECK(text_ink,
              "(4) FILE COPY 'Saving tables to disk...' text is RENDERED as ink");

        /* The progress bar item is a progressBar control (value 0, max 100), and
         * its rendered border (black) + interior (white at value 0) are present.
         * bar_rect: left=154, top=236, right=486, bottom=256. */
        CHECK(S.scene.dlg->items[1].ctrl != 0 &&
              S.scene.dlg->items[1].ctrl->contrlType == progressBar &&
              S.scene.dlg->items[1].ctrl->contrlMax == 100,
              "(4) FILE COPY item 2 is a progressBar (value 0..100)");
        CHECK(idx_at(&ctx, 154, 246) == 0,
              "(4) FILE COPY progress bar left border is painted (black)");
        CHECK(idx_at(&ctx, 160, 246) == 1,
              "(4) FILE COPY progress bar interior (value 0) is white");
    }

    /* ======================================================================
     * 5. The MODAL is ON TOP (z-order / clip correct). The probe point (143,240)
     * lies BOTH inside the modal's solid 7-px LEFT border (x in [140,147)) AND
     * inside document window 0 (x in [60,360), y in [80,300)). With correct
     * z-order the modal occludes the window -> the pixel is the modal's BLACK
     * border (idx 0). SHELL_MUTATE_MODAL_BEHIND / SHELL_MUTATE_NO_MODAL make the
     * window show through here instead (white body / chrome) -> RED.
     * ====================================================================== */
    {
        /* Confirm the probe is geometrically over BOTH the modal border and the
         * window (the check is meaningful, not vacuous). */
        int in_modal_border = (143 >= 140 && 143 < 147 && 240 >= 200 && 240 < 280);
        int in_window0 = (143 >= W0_L && 143 < W0_R && 240 >= W0_T && 240 < W0_B);
        CHECK(in_modal_border && in_window0,
              "(5) probe (143,240) is over BOTH the modal border and window 0 (meaningful)");
        CHECK(idx_at(&ctx, 143, 240) == 0,
              "(5) the modal OCCLUDES the window behind it -- modal border on top (z-order)");

        /* A second probe inside the modal's WHITE interior, also over window 0:
         * (160, 240). x in [147,493) interior; over window 0. Reads modal white. */
        CHECK(idx_at(&ctx, 160, 240) == 1,
              "(5) the modal interior occludes the window behind it (modal on top)");
    }

    /* ======================================================================
     * 6. DETERMINISM (Rule 11): two renders of the same scene are byte-identical.
     * ====================================================================== */
    {
        static snap_t a, b;
        snapshot(&ctx, &a);

        /* Re-render a fresh scene into a fresh context; compare byte-for-byte. */
        static shell_store_t S2;
        build_shell(&S2, 1);
        render_ctx_t ctx2;
        int rc2 = init_ctx(&ctx2);
        CHECK(rc2 == 0, "(6) second render context init");
        if (rc2 == 0) {
            shell_render(&S2.scene, &ctx2.fb.bm);
            snapshot(&ctx2, &b);
            int diff = 0, first = -1;
            for (int j = 0; j < SCRW * SCRH; j++)
                if (a.px[j] != b.px[j]) { diff++; if (first < 0) first = j; }
            CHECK(diff == 0,
                  "(6) two renders of the same scene are byte-identical (Rule 11)");
            if (diff != 0)
                fprintf(stderr, "    (6) %d pixels differ; first at (%d,%d)\n",
                        diff, first % SCRW, first / SCRW);
            render_ctx_free(&ctx2);
        }
    }

    /* ======================================================================
     * WRITE the composed scene PPM for the orchestrator's visual audit (Law 4 --
     * "yes, that's it" against the Office Space frame).
     * ====================================================================== */
    {
        const char *path = (argc > 1) ? argv[1] : "build/desktop_scene.ppm";
        if (render_write_ppm(&ctx, path) == 0)
            printf("    wrote composed desktop scene PPM to %s\n", path);
        else
            fprintf(stderr, "    WARN: could not write %s\n", path);
    }

    render_ctx_free(&ctx);
    return TEST_SUMMARY("test-shell");
}
