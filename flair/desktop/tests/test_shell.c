/* Desktop composition properties and retained window/modal pixel oracles.
 * Single-bar expectations follow the operator ruling of 2026-11-13:
 * CLAUDE.md, Hazards that are still true; GOLDEN-MASTER item 3.
 * The frame's other chrome and FILE COPY geometry retain their captured
 * Platinum oracles. Photoshop titles belong to an application resource.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* host render skeleton (-Iharness/render)     */
#include "region_algebra.h"     /* the LOCKED region contract (-Ispec)         */
#include "region.h"             /* engine constructors (-Ios/flair/region)   */
#include "window_record.h"      /* WindowRecord, part-codes (-Ispec)           */
#include "finder_menu.h"
#include "shell.h"              /* the DESKTOP SHELL under test (-Ios/flair)   */
#include "desktop.h"            /* FLAIR_DESKTOP_BG_INDEX (-Ios/flair)         */
#include "menu_canon.h"         /* FROZEN Photoshop canon string (-Ispec/assets)*/
#include "dialog.h"             /* FLAIR_CANON_FILECOPY_MSG (-Ios/flair)       */
#include "chrome_metrics.h"     /* FLAIR_CHROME_* (-Ispec)                     */
#include "color_canon.h"        /* named sampled Platinum canon indices         */
#include "palette.h"            /* INITECH_DESKTOP_BG_RGB (-Ispec/assets)      */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

TEST_HARNESS();

/* Strict Platinum title classifier for the composed-shell structural legs.
 * Ref: ADR-0004-AMENDMENT-DEC-10 Sec 4 and
 * ../system7-decomp/specs/sys8/window-chrome.md Sec 2.1-2.2. */
static int shell_title_row_index(int row)
{
    if (row == 0 || row == FLAIR_CHROME_TITLEBAR_H - 1) return CIDX_BLACK;
    if (row == 1) return CIDX_WHITE;
    if (row < FLAIR_CHROME_TITLE_STRIPE_TOP_OFF) return CIDX_PLAT_FRAME_FACE;
    if (row < FLAIR_CHROME_TITLE_STRIPE_TOP_OFF +
              FLAIR_CHROME_TITLE_BAND_STRIPE_ROWS) {
        return ((row - FLAIR_CHROME_TITLE_STRIPE_TOP_OFF) & 1)
               ? CIDX_PLAT_STRIPE_DARK : CIDX_WHITE;
    }
    if (row < FLAIR_CHROME_TITLEBAR_H - 2) return CIDX_PLAT_FRAME_FACE;
    return CIDX_PLAT_FRAME_SHADOW;
}

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

    /* InitechPaint-style test application resource; never a shell fallback. */
    MenuBar paint_bar;
    MenuInfo ps_menus[FLAIR_CANON_PHOTOSHOP_MENU_COUNT];
    MenuItem ps_items[FLAIR_CANON_PHOTOSHOP_MENU_COUNT][2];

    /* The FILE COPY modal storage. */
    DialogRecord       dlg;
    DialogItem         dlg_items[2];
    ControlRecord      dlg_progress;
    rgn_store_t        dlg_struc, dlg_cont, dlg_upd;

    /* The always-on-top overlay occluder region (beads initech-pipa). Only the
     * companion drag-survival sub-test activates it; the M4 gate scene passes NULL
     * so shell_render stays byte-identical. */
    rgn_store_t        overlay;
} shell_store_t;

/* The two document windows, placed per the frame: window 0 (back, upper-left)
 * overlaps the centered FILE COPY box; window 1 (front, lower-right). Both sit
 * BELOW the single 20-row menu bar. */
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

/* Build the scene into the caller bundle. `enable_overlay` != 0 activates the
 * always-on-top occluder (bars [+ modal]) so the companion drag-survival sub-test
 * can exercise the live compositor path; the M4 gate passes 0 (byte-identical). */
static void build_shell(shell_store_t *S, int show_modal, int enable_overlay)
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

    /* Test application's Photoshop titles, required by the operator ruling. */
    for (int mi = 0; mi < FLAIR_CANON_PHOTOSHOP_MENU_COUNT; mi++) {
        S->ps_items[mi][0] = (MenuItem){ "New",  0, 'N', 0, 1, 0 };
        S->ps_items[mi][1] = (MenuItem){ "Open", 0, 'O', 0, 1, 0 };
        S->ps_menus[mi].title     = FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[mi];
        S->ps_menus[mi].menuID    = (int16_t)(256 + mi);
        S->ps_menus[mi].items     = S->ps_items[mi];
        S->ps_menus[mi].n_items   = 2;
        S->ps_menus[mi].menuWidth = 0;
    }

    S->paint_bar = (MenuBar){ S->ps_menus, 8, 0 };

    store_attach(&S->dlg_struc);
    store_attach(&S->dlg_cont);
    store_attach(&S->dlg_upd);
    store_attach(&S->overlay);

    rgn_rect_t frame = { 0, 0, SCRH, SCRW };
    shell_build_scene(&S->scene,
                      frame,
                      S->wins, N_WINDOWS, g_win_bounds, g_win_titles,
                      &S->desk.r, &S->sa.r, &S->sb.r, &S->sc.r, &S->comp.r,
                      finder_menu_bar(),
                      &S->dlg, S->dlg_items, &S->dlg_progress,
                      &S->dlg_struc.r, &S->dlg_cont.r, &S->dlg_upd.r,
                      enable_overlay ? &S->overlay.r : (region_t *)0,
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
#include "expect_abort.h"
int main(int argc, char **argv)
{
    char msg[256];

    /* The canonical composed scene WITH the modal up (the Office Space frame).
     * enable_overlay=0: the M4 structural gate renders EXACTLY as before (the
     * overlay fold is a no-op when wm.overlay_rgn==NULL; initech-pipa). */
    static shell_store_t S;
    build_shell(&S, 1 /* show_modal */, 0 /* enable_overlay */);

    render_ctx_t ctx;
    int rc = init_ctx(&ctx);
    CHECK(rc == 0, "render_ctx_init(8bpp 640x480) must succeed (AM-1 geometry param)");
    if (rc != 0) {
        return TEST_SUMMARY("test-shell");
    }

    shell_render(&S.scene, &ctx.fb.bm);
    /* Operator ruling 2026-11-13; CLAUDE.md, Hazards that are still true:
     * the former second band's rows belong to the desktop. These literal
     * probes are clear of the scene's windows and modal. */
    {
        int bad = 0;
        for (int y = 20; y < 40; y++)
            for (int x = 0; x < 640; x++)
                bad += idx_at(&ctx, x, y) != (int)FLAIR_DESKTOP_BG_INDEX;
        CHECK(bad == 0, "one menu bar: rows 20..39 are desktop, not menu chrome");
    }
    EXPECT_ABORT({ bitmap_t small = ctx.fb.bm; small.height = 10;
                   shell_render(&S.scene, &small); },
                 "mqsn surface shorter than one menu bar is rejected");

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

    /* One top bar; its Platinum profile remains captured period chrome. */
    {
        CHECK(FLAIR_MENUBAR_H == 20 && FLAIR_CHROME_MENUBAR_H == 20,
              "(2) one menu bar is 20 rows high");
        CHECK(idx_at(&ctx, 600, 5) == 231 && idx_at(&ctx, 300, 19) == 0,
              "(2) top bar retains Platinum face and baseline");
        CHECK(S.scene.bar.menus == finder_menu_bar()->menus &&
              S.scene.bar.n_menus == 5 && S.scene.bar.has_apple,
              "(2) Finder titles and Initech slot occupy the top bar");
    }

    /* ======================================================================
     * 3. WINDOW CHROME present (the front window 1, lower-right, clear of the
     * modal): exact Platinum title band, outer frame/raised rail, and 16 px
     * scrollbar -- vs chrome_metrics. Window 1 is fully visible (front, and its
     * right/bottom edges land over bare desktop, right of the modal x<500).
     * ====================================================================== */
    {
        const int fr = FLAIR_CHROME_FRAME;
        const int title_bot = W1_T + FLAIR_CHROME_TITLEBAR_H;
        const int mid_x = (W1_L + W1_R) / 2;
        /* The pinstripe is scanned at a clear column -- right of the close box and
         * LEFT of the centered window title ("untitled-2"), so the title knockout/
         * glyphs do not interrupt the stripe run (beads initech-lxg9). */
        const int pin_x = W1_L + 24;

        /* Replace the old two-shade-only check with the stronger full profile.
         * Ref: DEC-10 Sec 4; sys8/window-chrome.md Sec 2.1-2.2. */
        int profile_ok = 1;
        for (int row = 0; row < FLAIR_CHROME_TITLEBAR_H; row++) {
            profile_ok = profile_ok &&
                idx_at(&ctx, pin_x, W1_T + row) ==
                    shell_title_row_index(row);
        }
        CHECK(profile_ok,
              "(3) front window title matches exact 22-row Platinum profile");

        /* The row just below the title bar is the white body (idx 1) -- proves
         * the title bar is EXACTLY FLAIR_CHROME_TITLEBAR_H tall. */
        CHECK(idx_at(&ctx, mid_x, title_bot) == 1,
              "(3) front window row below title bar is white body -- title height exact");

        /* Frame: exactly 1 px, followed by the live one-pixel shadow band. */
        int content_top = title_bot;
        int content_bot = W1_B - fr;
        int row = (content_top + content_bot) / 2;
        CHECK(idx_at(&ctx, W1_R - 1, row) != (int)FLAIR_DESKTOP_BG_INDEX,
              "(3) front window right frame column is painted");
        CHECK(idx_at(&ctx, W1_R, row) == CIDX_BLACK,
              "(3) pixel just right of the frame is the black shadow band");
        CHECK(idx_at(&ctx, W1_R + 1, row) == (int)FLAIR_DESKTOP_BG_INDEX,
              "(3) pixel just right of the shadow is bare seafoam");

        /* The 16-pixel band ends at the inner line inside the raised rail.
         * Ref: DEC-10 Sec 4; sys8/scrollbars.md Sec 1 and
         * sys8/window-chrome.md Sec 4. */
        int sb_right_exclusive = W1_R - fr - FLAIR_CHROME_BODY_BAR;
        int sb_left = sb_right_exclusive - FLAIR_CHROME_SCROLLBAR_W;
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
     * (centered on 640x480: center (320,240); 360x80). The chrome is the
     * MOVEABLE TITLED modal (movableDBoxProc; beads initech-zvo6): the exact
     * Platinum 22-row title profile over [dt,dt+FLAIR_CHROME_TITLEBAR_H), then a
     * PLAIN 1-px frame (idx0) around the whole box -- NOT the old dBoxProc
     * 7-px solid border.
     * ====================================================================== */
    {
        const int dl = 140, dt = 200, dr = 500, db = 280;
        const int fr = FLAIR_CHROME_FRAME;               /* 1 px            */

        /* Centered: the box center is the screen center. */
        CHECK((dl + dr) / 2 == SCRW / 2 && (dt + db) / 2 == SCRH / 2,
              "(4) FILE COPY box is centered on the 640x480 desktop");

        /* Full-profile replacement for the retained bevel/stripe/shared-line
         * triplet. Ref: DEC-10 Sec 4; sys8/window-chrome.md Sec 2.1-2.2. */
        {
            int profile_ok = 1;
            for (int row = 0; row < FLAIR_CHROME_TITLEBAR_H; row++) {
                profile_ok = profile_ok &&
                    idx_at(&ctx, dl + 5, dt + row) ==
                        shell_title_row_index(row);
            }
            CHECK(profile_ok,
                  "(4) FILE COPY title matches exact 22-row Platinum profile");
        }

        /* The frame is PLAIN 1-px. The sampled content field is E7 with a
         * white left inset and C0 right inset; under the OLD 7px dBoxProc
         * border both edge probes would still be solid BLACK.
         * SHELL_MUTATE_NO_MODAL / SHELL_MUTATE_MODAL_BEHIND do not affect this
         * (they are dialog-presence/z-order mutants, not chrome mutants); this
         * leg's own mutation coverage is DIALOG_MUTATE_TITLELESS_MODAL in
         * test-dialog (Rule 6 scoping: the chrome defect lives in dialog.c). */
        CHECK(idx_at(&ctx, dl + fr, 240) == 1,
              "(4) FILE COPY content just inside the LEFT frame (x=141,y=240) "
              "is white -- proves a 1px frame, not the old 7px border "
              "(initech-zvo6)");
        CHECK(idx_at(&ctx, dr - fr - 1, 240) == CIDX_PLAT_WELL,
              "(4) FILE COPY content just inside the RIGHT frame (x=498,y=240) "
              "is sampled C0 inset shadow");
        CHECK(idx_at(&ctx, dl + fr + 1, 240) == CIDX_PLAT_FACE,
              "(4) FILE COPY content body is sampled E7 face");
        CHECK(idx_at(&ctx, 320, dt + FLAIR_CHROME_TITLEBAR_H) == CIDX_WHITE &&
              idx_at(&ctx, 320, db - 2) == CIDX_PLAT_WELL,
              "(4) FILE COPY content carries white top / C0 bottom inset legs");
        CHECK(idx_at(&ctx, dl, 240) == 0,
              "(4) FILE COPY outer LEFT frame column (x=140,y=240) is black");
        CHECK(idx_at(&ctx, dr - 1, 240) == 0,
              "(4) FILE COPY outer RIGHT frame column (x=499,y=240) is black");
        CHECK(idx_at(&ctx, 320, db - 1) == 0,
              "(4) FILE COPY outer BOTTOM frame row (x=320,y=279) is black");

        /* The byte-EXACT canon string "Saving tables to disk..." is the dialog's
         * static text item (Law 4). Assert it on the BUILT dialog (byte-exact). */
        char buf[256];
        GetDialogItemText(S.scene.dlg, 1, buf, sizeof buf);
        snprintf(msg, sizeof msg,
                 "(4) FILE COPY item 1 text byte-EXACT '%.40s' (Law 4 canon)",
                 FLAIR_CANON_FILECOPY_MSG);
        CHECK(strcmp(buf, FLAIR_CANON_FILECOPY_MSG) == 0, msg);

        /* The window TITLE is the byte-EXACT canon "FILE COPY" (Law 4,
         * initech-zvo6). */
        snprintf(msg, sizeof msg,
                 "(4) FILE COPY window title byte-EXACT '%s' (Law 4 canon)",
                 FLAIR_CANON_FILECOPY_TITLE);
        CHECK(strcmp(S.scene.dlg->window.titleHandle,
                     FLAIR_CANON_FILECOPY_TITLE) == 0, msg);

        /* And the statText body is RENDERED: the static-text item rect
         * (left=154, top=225 -- below the Platinum title band, was
         * top=212 under the old 7px-border layout) has painted text ink
         * within the text band -- proves the modal's text is drawn, not just
         * stored. DrawDialog draws statText with DLG_TEXT_INK (index 4; the
         * title-ink shade, dialog.c) on the E7 body. Require the exact ink
         * index so the new face cannot make this presence leg vacuous. */
        int tx0 = 154, ty0 = 225;
        int tw  = text_measure(FONT_CHICAGO, FLAIR_CANON_FILECOPY_MSG);
        int text_ink = 0;
        for (int x = tx0; x < tx0 + tw && !text_ink; x++)
            for (int y = ty0; y < ty0 + 16; y++) {
                int v = idx_at(&ctx, x, y);
                if (v == CIDX_TITLE_INK) { text_ink = 1; break; }
            }
        CHECK(text_ink,
              "(4) FILE COPY 'Saving tables to disk...' text is RENDERED as ink");

        /* The progress bar item is a progressBar control at the CANON non-zero
         * value (beads initech-a90f), and its rendered black frame + a
         * NON-ZERO fill are present. bar_rect: left=154, top=249, right=486,
         * bottom=263 -- the Platinum CDEF 5 etched bar (initech-bf2): fill
         * rung #5454B3 -> the dark teal role (idx 63), empty track #CDCDCD.
         * SHELL/DIALOG_MUTATE_PROGRESS_ZERO reverts to value=0 -> the fill
         * check goes RED (this leg's own mutation coverage is
         * DIALOG_MUTATE_PROGRESS_ZERO in test-dialog). */
        CHECK(S.scene.dlg->items[1].ctrl != 0 &&
              S.scene.dlg->items[1].ctrl->contrlType == progressBar &&
              S.scene.dlg->items[1].ctrl->contrlMax == 100,
              "(4) FILE COPY item 2 is a progressBar (0..100)");
        snprintf(msg, sizeof msg,
                 "(4) FILE COPY progress bar initial value must be "
                 "FLAIR_CANON_FILECOPY_PROGRESS (%d), got %d (initech-a90f)",
                 FLAIR_CANON_FILECOPY_PROGRESS,
                 (int)S.scene.dlg->items[1].ctrl->contrlValue);
        CHECK(S.scene.dlg->items[1].ctrl->contrlValue ==
              FLAIR_CANON_FILECOPY_PROGRESS, msg);
        CHECK(idx_at(&ctx, 155, 259) == 0,
              "(4) FILE COPY progress bar black frame is painted (x=155)");
        CHECK(idx_at(&ctx, 160, 259) == 63,
              "(4) FILE COPY progress bar FILL (x=160,y=259) is the dark "
              "teal rung idx63 -- a non-zero fill (initech-a90f, bf2)");
        CHECK(idx_at(&ctx, 400, 259) == 205,
              "(4) FILE COPY progress bar empty track (x=400,y=259) "
              "is #CDCDCD idx205 (bf2)");
    }

    /* ======================================================================
     * 5. The MODAL is ON TOP (z-order / clip correct). The probe point
     * (140,240) lies BOTH on the modal's plain 1-px LEFT frame column AND
     * inside document window 0 (x in [60,360), y in [80,300)). With correct
     * z-order the modal occludes the window -> the pixel is the modal's BLACK
     * frame (idx 0). SHELL_MUTATE_MODAL_BEHIND / SHELL_MUTATE_NO_MODAL make the
     * window show through here instead (white body / chrome) -> RED.
     * ====================================================================== */
    {
        /* Confirm the probe is geometrically over BOTH the modal frame and the
         * window (the check is meaningful, not vacuous). */
        int in_modal_frame = (140 >= 140 && 140 < 141 && 240 >= 200 && 240 < 280);
        int in_window0 = (140 >= W0_L && 140 < W0_R && 240 >= W0_T && 240 < W0_B);
        CHECK(in_modal_frame && in_window0,
              "(5) probe (140,240) is over BOTH the modal frame and window 0 (meaningful)");
        CHECK(idx_at(&ctx, 140, 240) == 0,
              "(5) the modal OCCLUDES the window behind it -- modal frame on top (z-order)");

        /* A second probe inside the modal's E7 interior, also over window 0. */
        CHECK(idx_at(&ctx, 160, 240) == CIDX_PLAT_FACE,
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
        build_shell(&S2, 1, 0);
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
     * 7. COMPANION (beads initech-pipa): the LIVE compositor keeps the modal
     * INTACT when a window is DRAGGED ACROSS it. With the always-on-top overlay
     * installed (the bars + modal folded into fronts_union), MoveWindow's damage
     * NEVER classifies the modal as newly-exposed (no seafoam-erase, defect a) and
     * no window's repaint includes the modal (no overpaint, defect b), so the
     * modal survives from the initial shell_render. Drag window 1 (front) 260 px
     * LEFT so its OLD struct {300,120,560,360} -- which overlapped the modal
     * {140,200,500,280} -- is vacated (the pipa erase case), then minimal-repaint
     * the damage and assert the modal is untouched. This is the FAST host proof of
     * the fix on the SHARED window.c/desktop.c spine (no emulator); the EMU oracle
     * initech-dc4v is the independent-golden screendump grade.
     *
     * MUTANT window.c -DWINDOW_MUTATE_IGNORE_OVERLAY compiles the fold out, so the
     * modal here is seafoam-erased -> these checks go RED (Rule 6).
     * ====================================================================== */
    {
        static shell_store_t S3;
        build_shell(&S3, 1 /* show_modal */, 1 /* enable_overlay */);
        render_ctx_t ctx3;
        int rc3 = init_ctx(&ctx3);
        CHECK(rc3 == 0, "(7) companion render context init");
        if (rc3 == 0) {
            /* Initial full composite: seafoam + windows + bars + modal on top. */
            shell_render(&S3.scene, &ctx3.fb.bm);
            CHECK(S3.scene.wm.overlay_rgn != (region_t *)0,
                  "(7) always-on-top overlay occluder is installed (wm.overlay_rgn)");
            /* Right frame column is now x=499 (dr-1; the PLAIN 1px frame, was
             * x=496 under the old 7px dBoxProc border, initech-zvo6). */
            CHECK(idx_at(&ctx3, 499, 240) == 0,
                  "(7) pre-drag: modal right frame (499,240) is black idx0");

            /* Drag window 1 (the front doc window) 260 px LEFT and mirror the live
             * pump: MoveWindow + seed the moved window's own repaint + minimal
             * damage repaint (flair_live_do_drag idiom). */
            WindowPtr w1 = &S3.wins[1].rec;
            MoveWindow(&S3.scene.wm, w1, W1_L - 260, W1_T);
            WindowMgr_invalidate(&S3.scene.wm, w1, region_get_bbox(w1->strucRgn));
            desktop_paint_damage(&S3.scene.wm, &ctx3.fb.bm, &S3.comp.r);
            /* DQ1: the compositor no longer validates window updateRgns; this
             * ownerless host scene closes the cycle explicitly (the live
             * non-tenant pump's content phase). */
            desktop_validate_all(&S3.scene.wm);

            /* The modal SURVIVES: right frame still black, interior still E7,
             * and the vacated right-half band shows NO seafoam teal. */
            CHECK(idx_at(&ctx3, 499, 240) == 0,
                  "(7) post-drag: modal right frame (499,240) STILL black idx0 (not erased)");
            CHECK(idx_at(&ctx3, 450, 240) == CIDX_PLAT_FACE,
                  "(7) post-drag: modal interior (450,240) STILL E7 idx231 (not erased)");
            int teal = 0, tot = 0;
            for (int y = 206; y < 278; y++)
                for (int x = 366; x < 498; x++) {
                    tot++;
                    if (idx_at(&ctx3, x, y) == (int)FLAIR_DESKTOP_BG_INDEX) teal++;
                }
            CHECK(teal * 100 <= tot * 5,
                  "(7) post-drag: modal right-half x[366,498]y[206,278] <=5% seafoam (modal survived the drag)");
            if (teal * 100 > tot * 5)
                fprintf(stderr, "    (7) %d/%d modal-band pixels erased to seafoam "
                        "-- the overlay fold is not occluding\n", teal, tot);
            render_ctx_free(&ctx3);
        }
    }

    /* ======================================================================
     * 8. R1.4 REVERSE SURVIVAL (initech-zn61 orchestrator re-key): move the
     * standalone FILE COPY modal 140 px LEFT across window 1, rebuild the
     * overlay, restore its old footprint through the damage spine, then draw
     * the modal last at the new rect. This is the HOST coordinate/probe proof
     * behind the re-keyed ppm_flair_dc4v_check; the emulator leg stays unrun in
     * this lane. Geometry is literal and independent of the PPM grader.
     * ====================================================================== */
    {
        static shell_store_t S4;
        build_shell(&S4, 1 /* show_modal */, 1 /* enable_overlay */);
        render_ctx_t ctx4;
        int rc4 = init_ctx(&ctx4);
        CHECK(rc4 == 0, "(8) reverse-modal render context init");
        if (rc4 == 0) {
            rgn_rect_t old = region_get_bbox(S4.scene.dlg->window.strucRgn);
            shell_render(&S4.scene, &ctx4.fb.bm);
            MoveDialog(S4.scene.dlg, -140, 0);
            shell_sync_overlay(&S4.scene);
            WindowMgr_invalidate_desktop(&S4.scene.wm, old);
            desktop_paint_damage(&S4.scene.wm, &ctx4.fb.bm, &S4.comp.r);
            desktop_validate_all(&S4.scene.wm);

            S4.scene.dlg->window.port.portBits.bm = ctx4.fb.bm;
            S4.scene.dlg->window.port.visRgn = (region_t *)0;
            S4.scene.dlg->window.port.clipRgn = (region_t *)0;
            S4.scene.dlg->window.port.portRect =
                region_get_bbox(S4.scene.dlg->window.strucRgn);
            DrawDialog(S4.scene.dlg);

            CHECK(region_get_bbox(S4.scene.dlg->window.strucRgn).left == 0 &&
                  region_get_bbox(S4.scene.dlg->window.strucRgn).top == 200,
                  "(8) modal committed to literal new origin (0,200)");
            CHECK(idx_at(&ctx4, 359, 240) == CIDX_BLACK,
                  "(8) moved modal right frame (359,240) is black");
            CHECK(idx_at(&ctx4, 330, 240) == CIDX_PLAT_FACE,
                  "(8) moved modal interior (330,240) is E7");

            int white = 0, teal = 0, tot = 0;
            for (int y = 222; y < 278; y++)
                for (int x = 366; x < 498; x++) {
                    tot++;
                    if (idx_at(&ctx4, x, y) == CIDX_WHITE) white++;
                    if (idx_at(&ctx4, x, y) ==
                        (int)FLAIR_DESKTOP_BG_INDEX) teal++;
                }
            CHECK(white * 100 >= tot * 90,
                  "(8) vacated old-modal band is >=90% restored window-white");
            CHECK(teal * 100 <= tot * 5,
                  "(8) vacated old-modal band is <=5% teal (no background hole)");
            if (!(white * 100 >= tot * 90 && teal * 100 <= tot * 5))
                fprintf(stderr, "    (8) vacated band white=%d/%d teal=%d/%d\n",
                        white, tot, teal, tot);
            render_ctx_free(&ctx4);
        }
    }

    /* Operator 2026-11-13: Finder-only, Finder window, app, return to Finder.
     * Literal rows [20,40) must remain desktop/window pixels in every scene.
     * Compare full top pixels against a separately drawn owner's MenuBar,
     * preserving an app resource without has_apple and its original identity. */
    {
        static shell_store_t live;
        static snap_t finder, app;
        build_shell(&live, 0, 1);
        DisposeWindow(&live.scene.wm, &live.wins[0].rec);
        DisposeWindow(&live.scene.wm, &live.wins[1].rec);
        render_ctx_t lc, reference;
        int lr = init_ctx(&lc), rr = init_ctx(&reference);
        CHECK(lr == 0 && rr == 0, "(9) single-bar contexts initialize");
        if (lr == 0 && rr == 0) {
            CHECK(MenuClock_set(9, 0), "(9) deterministic clock set");
            for (int which = 0; which < 3; which++) {
                const MenuBar *owner = which == 1 ? &live.paint_bar : finder_menu_bar();
                shell_render_with_menubar(&live.scene, &lc.fb.bm, owner);
                MenuBar expected = *owner;
                expected.has_apple = 1;
                MenuClock_attach(&expected);
                DrawMenuBar(&reference.port, &expected, -1, NULL);
                int top_bad = 0, lower_bad = 0;
                for (int y = 0; y < 20; y++) for (int x = 0; x < 640; x++)
                    top_bad += idx_at(&lc, x, y) != idx_at(&reference, x, y);
                for (int y = 20; y < 40; y++) for (int x = 0; x < 640; x++)
                    lower_bad += idx_at(&lc, x, y) != (int)FLAIR_DESKTOP_BG_INDEX;
                CHECK(top_bad == 0, "(9) front owner's titles, glyph and clock in top bar");
                CHECK(lower_bad == 0, "(9) no second band in Finder or app scene");
                CHECK(!region_contains_point(live.scene.overlay_rgn, 300, 20),
                      "(9) reclaimed rows are not menu occlusion");
                if (which == 0) snapshot(&lc, &finder);
                if (which == 1) snapshot(&lc, &app);
                if (which == 2) {
                    static snap_t restored;
                    snapshot(&lc, &restored);
                    CHECK(memcmp(finder.px, restored.px, sizeof finder.px) == 0,
                          "(9) returning to Finder restores titles and clock");
                }

                /* First title, first item: pull down, track, and select. */
                const MenuBar *shown = shell_front_menubar(&live.scene, owner);
                MenuTrackState track;
                flair_point_t title = { 10, 60 }, item = { 28, 60 };
                MenuTrack_begin(shown, title, &track);
                flair_draw_menu_panel(&lc.port, shown, track.mi, -1, NULL);
                MenuTrack_step(shown, &track, item);
                CHECK(MenuTrack_result(shown, &track) ==
                      MenuResult(which == 1 ? 256 : 512, 1),
                      "(9) front menu pulls down and tracks at row zero");
                CHECK(owner->menus == shown->menus && owner != shown,
                      "(9) display view shares items without replacing owner identity");
                shell_render_with_menubar(&live.scene, &lc.fb.bm, owner);
                int erased = 1;
                for (int y = 20; y < 40; y++) for (int x = 0; x < 640; x++)
                    erased &= idx_at(&lc, x, y) == (int)FLAIR_DESKTOP_BG_INDEX;
                CHECK(erased, "(9) menu dismissal restores desktop below row 19");
            }
            CHECK(memcmp(finder.px, app.px, 20 * 640) != 0,
                  "(9) application activation changes top-band titles");
            CHECK(live.paint_bar.has_apple == 0,
                  "(9) shell leaves original application menu resource unchanged");
            CHECK(live.paint_bar.n_menus == 8 &&
                  strcmp(live.paint_bar.menus[3].title, "Layer") == 0 &&
                  strcmp(live.paint_bar.menus[5].title, "View") == 0,
                  "(9) Photoshop titles survive as front application menus");

            /* A window may occupy the former band, and live damage repaint
             * must neither clip it nor overwrite the top bar. */
            NewDocumentWindow(&live.scene.wm, &live.wins[0].rec,
                              (rgn_rect_t){20, 60, 240, 420}, documentKind, 1);
            shell_render(&live.scene, &lc.fb.bm);
            CHECK(idx_at(&lc, 60, 20) == CIDX_BLACK &&
                  idx_at(&lc, 300, 19) == CIDX_BLACK,
                  "(9) Finder window can begin immediately below the sole bar");
            MoveWindow(&live.scene.wm, &live.wins[0].rec, 80, 20);
            desktop_paint_damage(&live.scene.wm, &lc.fb.bm, &live.comp.r);
            CHECK(idx_at(&lc, 60, 20) == (int)FLAIR_DESKTOP_BG_INDEX &&
                  idx_at(&lc, 80, 20) == CIDX_BLACK &&
                  idx_at(&lc, 300, 19) == CIDX_BLACK,
                  "(9) reclaimed rows participate in window exposure and repaint");
            live.scene.modal_up = 1;
            MoveDialog(live.scene.dlg, 0, -190);
            shell_sync_overlay(&live.scene);
            shell_render_with_menubar(&live.scene, &lc.fb.bm, &live.paint_bar);
            CHECK(idx_at(&lc, 499, 15) == CIDX_BLACK &&
                  idx_at(&lc, 499, 30) == CIDX_BLACK,
                  "(9) moved modal occludes menu and reclaimed desktop rows");
        }
        if (lr == 0) render_ctx_free(&lc);
        if (rr == 0) render_ctx_free(&reference);
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
