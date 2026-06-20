/* test_dialog.c -- the FLAIR Dialog Manager property suite (THE ORACLE).
 *
 * beads: (Dialog Manager oracle: layout, ModalDialog, draw, named mutants)
 * Ref:   ADR-0004 D-3 ("Dialog Manager -- DialogRecord + item lists;
 *          ModalDialog; the modal FILE COPY box ('Saving tables to disk...',
 *          the comedic centerpiece, PRD Sec 6.5 / Appendix B).");
 *        os/flair/dialog.{c,h} (the unit under test; the artifact freestanding C).
 *        spec/chrome_metrics.h (FLAIR_CHROME_DIALOG_BORDER=7).
 *        spec/window_record.h (dBoxProc=1, dialogKind=2).
 *        harness/render/render.{c,h} (host render skeleton; dual-compile path).
 *        harness/proptest/test_control.c + test_window.c (the harness idiom
 *          this suite mirrors: TEST_HARNESS/CHECK/TEST_SUMMARY + render).
 *        CLAUDE.md Law 2 (oracle is truth), Law 4 (FILE COPY canon must be
 *          byte-exact; DIALOG_MUTATE_FILECOPY_MSG MUST go RED), Rule 6
 *          (mutation-proven), Rule 11 (seeded LCG + deterministic), Rule 12
 *          (ASCII-clean source).
 *
 * THE PROPERTIES (in order of decisiveness):
 *
 *  1. LAYOUT:
 *     (a) dBoxProc border == FLAIR_CHROME_DIALOG_BORDER (7 px) -- checked
 *         via rendered pixel positions.
 *     (b) Items are placed at their declared rects.
 *     (c) FILE COPY dialog:
 *         - statText item contains EXACTLY FLAIR_CANON_FILECOPY_MSG (byte-exact).
 *         - Progress bar control item has type progressBar, value=0, max=100.
 *         - Two items total; layout matches canonical dimensions.
 *
 *  2. MODALDIALOG event routing:
 *     (a) A recorded mouseDown event in an enabled non-statText item returns
 *         that item's 1-based index via *itemHit.
 *     (b) Return keypress -> defaultItem.
 *     (c) Escape keypress -> cancelItem.
 *     (d) Click in a DISABLED item -> NOT returned (itemHit remains 0 /
 *         loop continues until ring drains with sleepTicks=0).
 *     (e) Click in a statText item (even if enabled=1) -> NOT returned
 *         (statText is always non-interactive; IM-I Ch 6).
 *
 *  3. DRAW:
 *     Render the FILE COPY dialog into a host 8bpp offscreen via the render
 *     skeleton. Assert:
 *     (a) 7-px border frame: the outermost 7 rows/columns are DLG_BLACK (0).
 *         Specifically: pixel at (left, top+3) == 0 (left border interior);
 *         pixel at (left+3, top) == 0 (top border interior).
 *     (b) Content interior: pixel just inside the border is DLG_WHITE (1).
 *     (c) Progress bar band: the ctrl_progress_fill_px region within the
 *         progress bar item rect matches the expected pixel value.
 *
 * MUTANTS (Rule 6), each MUST drive this oracle RED:
 *   DIALOG_MUTATE_BORDER       -- DrawDialog uses wrong border width (8 px).
 *                                 => property 1a (border width) and 3a go RED.
 *   DIALOG_MUTATE_HIT_STATIC   -- ModalDialog returns statText items.
 *                                 => property 2e goes RED.
 *   DIALOG_MUTATE_FILECOPY_MSG -- FileCopyDialog uses a wrong message string.
 *                                 => property 1c (byte-exact canon) goes RED
 *                                    (Law 4 REQUIRED).
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* host render skeleton (-Iharness/render)     */
#include "dialog.h"             /* the Dialog Manager under test (-Ios/flair)  */
#include "control.h"            /* ControlRecord, progressBar (-Ios/flair)     */
#include "chrome_metrics.h"     /* FLAIR_CHROME_DIALOG_BORDER (-Ispec)         */
#include "text.h"               /* text_measure, FONT_CHICAGO (-Ios/flair)     */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

/* Region storage for dialog tests (pool sizes mirror test_control.c). */
#include "region.h"             /* rgn_store_t + helpers (-Ios/flair/atkinson) */

TEST_HARNESS();

/* ===========================================================================
 * Seeded LCG (Rule 11 -- deterministic; mirrors test_control.c / test_window.c)
 * ===========================================================================*/
static uint32_t g_seed = 0x44494147u; /* "DIAG" */
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return g_seed;
}

/* ===========================================================================
 * REGION STORAGE HELPERS  (caller-supplied, no malloc in artifact code)
 * Mirrors the idiom from test_window.c / test_blitter.c.
 * ===========================================================================*/
#define RGN_ROW_CAP   64
#define RGN_XPOOL_CAP 512

typedef struct {
    region_t  r;
    rgn_row_t rows[RGN_ROW_CAP];
    int16_t   xpool[RGN_XPOOL_CAP];
} rgn_store_t;

static void rgn_store_init(rgn_store_t *s)
{
    s->r.rows         = s->rows;
    s->r.cap_rows     = (uint16_t)RGN_ROW_CAP;
    s->r.x_pool       = s->xpool;
    s->r.x_pool_cap   = (uint32_t)RGN_XPOOL_CAP;
    region_set_empty(&s->r);
}

/* ===========================================================================
 * RENDER HELPERS  (mirrors test_control.c / test_chrome.c render_one idiom)
 * ===========================================================================*/
static int render_one(render_ctx_t *ctx, uint32_t bpp)
{
    render_boot_info_t boot;
    memset(&boot, 0, sizeof boot);
    boot.lfb_bpp    = bpp;
    boot.lfb_width  = 640u;
    boot.lfb_height = 480u;
    boot.lfb_pitch  = 0u;      /* tight */
    return render_ctx_init(ctx, &boot);
}

/* Palette index at (x,y) in the 8bpp offscreen. */
static uint32_t pidx(const render_ctx_t *ctx, uint32_t x, uint32_t y)
{
    return render_pixel_index(ctx, x, y);
}

/* ===========================================================================
 * PROPERTY 1a + 1b -- LAYOUT: border width and item placement
 *
 * Build a minimal dialog (1 enabled ctrlItem button) and verify:
 *   - defaultItem / cancelItem are stored correctly.
 *   - Item rects are stored at the declared positions.
 *   - FLAIR_CHROME_DIALOG_BORDER == 7 (compile-time).
 *   - dBoxProc variant is set.
 * ===========================================================================*/
static void test_layout_basic(void)
{
    char msg[200];

    /* --- FLAIR_CHROME_DIALOG_BORDER is 7 (from the compile-time static assert
     * in dialog.h; also checked at runtime for the mutant oracle to fire). */
    snprintf(msg, sizeof msg,
             "FLAIR_CHROME_DIALOG_BORDER must be 7 (WDEF dBoxBorderSize EQU 7), got %d",
             FLAIR_CHROME_DIALOG_BORDER);
    CHECK(FLAIR_CHROME_DIALOG_BORDER == 7, msg);

    /* --- Build a minimal dialog with 2 items. */
    DialogRecord dr;
    DialogItem   items[2];
    ControlRecord ctrl_btn;

    rgn_store_t s_struc, s_cont, s_upd;
    rgn_store_init(&s_struc);
    rgn_store_init(&s_cont);
    rgn_store_init(&s_upd);

    /* Button control (item 1, enabled). */
    rgn_rect_t btn_rect;
    btn_rect.top = 230; btn_rect.left = 260; btn_rect.bottom = 250; btn_rect.right = 360;
    control_init(&ctrl_btn, pushButton, btn_rect, 0, 0, 1, 1, "OK");

    items[0].type    = ctrlItem;
    items[0].rect    = btn_rect;
    items[0].text    = 0;
    items[0].ctrl    = &ctrl_btn;
    items[0].enabled = 1;
    items[0]._pad[0] = items[0]._pad[1] = items[0]._pad[2] = 0;

    /* Static text (item 2, not enabled). */
    rgn_rect_t txt_rect;
    txt_rect.top = 210; txt_rect.left = 160; txt_rect.bottom = 225; txt_rect.right = 480;
    items[1].type    = statText;
    items[1].rect    = txt_rect;
    items[1].text    = "Dialog test label";
    items[1].ctrl    = 0;
    items[1].enabled = 0;
    items[1]._pad[0] = items[1]._pad[1] = items[1]._pad[2] = 0;

    rgn_rect_t bounds;
    bounds.top = 200; bounds.left = 150; bounds.bottom = 260; bounds.right = 490;

    DialogPtr dp = NewDialog(&dr, bounds, "Test Dialog",
                             items, 2, 1 /* default=item 1 */, 0 /* no cancel */,
                             0 /* no WindowMgr */,
                             &s_struc.r, &s_cont.r, &s_upd.r);

    CHECK(dp != 0, "NewDialog must return non-NULL");
    if (!dp) { return; }

    /* Window kind and variant. */
    snprintf(msg, sizeof msg,
             "dialog windowKind must be dialogKind=2, got %d",
             (int)dp->window.windowKind);
    CHECK(dp->window.windowKind == (int16_t)dialogKind, msg);

    snprintf(msg, sizeof msg,
             "dialog windowDefProcVariant must be dBoxProc=1, got %d",
             (int)dp->window.windowDefProcVariant);
    CHECK(dp->window.windowDefProcVariant == (int16_t)dBoxProc, msg);

    /* Default / cancel items stored correctly. */
    CHECK(dp->defaultItem == 1, "defaultItem must be 1");
    CHECK(dp->cancelItem  == 0, "cancelItem must be 0 (none)");

    /* Item rects are at the declared positions. */
    flair_dialog_item_type_t type1;
    rgn_rect_t               rect1;
    GetDialogItem(dp, 1, &type1, &rect1);
    CHECK(type1 == ctrlItem, "item 1 type must be ctrlItem");
    CHECK(rect1.top    == btn_rect.top    &&
          rect1.left   == btn_rect.left   &&
          rect1.bottom == btn_rect.bottom &&
          rect1.right  == btn_rect.right,
          "item 1 rect must match the declared btn_rect");

    flair_dialog_item_type_t type2;
    rgn_rect_t               rect2;
    GetDialogItem(dp, 2, &type2, &rect2);
    CHECK(type2 == statText, "item 2 type must be statText");
    CHECK(rect2.left == txt_rect.left && rect2.top == txt_rect.top,
          "item 2 rect top-left must match declared txt_rect");

    /* Visible after NewDialog. */
    CHECK(dp->window.visible == 1, "dialog window must be visible after NewDialog");
}

/* ===========================================================================
 * PROPERTY 1c -- FILE COPY dialog: byte-exact FLAIR_CANON_FILECOPY_MSG +
 *                progressBar item.
 *
 * MUTATION: DIALOG_MUTATE_FILECOPY_MSG alters the canon string -> RED (Law 4).
 * ===========================================================================*/
static void test_filecopy_layout(void)
{
    char msg[300];

    DialogRecord  dr;
    DialogItem    items[2];
    ControlRecord ctrl_bar;

    rgn_store_t s_struc, s_cont, s_upd;
    rgn_store_init(&s_struc);
    rgn_store_init(&s_cont);
    rgn_store_init(&s_upd);

    DialogPtr dp = FileCopyDialog(&dr, items, &ctrl_bar,
                                  &s_struc.r, &s_cont.r, &s_upd.r);

    CHECK(dp != 0, "FileCopyDialog must return non-NULL");
    if (!dp) { return; }

    /* Exactly 2 items. */
    CHECK(dp->itemCount == 2, "FILE COPY dialog must have exactly 2 items");

    /* Item 1: statText with EXACTLY FLAIR_CANON_FILECOPY_MSG.
     * Law 4: this check MUST be byte-exact. */
    flair_dialog_item_type_t type1;
    rgn_rect_t               rect1;
    GetDialogItem(dp, 1, &type1, &rect1);
    CHECK(type1 == statText, "FILE COPY item 1 must be statText");
    CHECK(dp->items[0].enabled == 0, "FILE COPY statText item must be enabled=0");

    char item1_text[256];
    GetDialogItemText(dp, 1, item1_text, sizeof item1_text);
    snprintf(msg, sizeof msg,
             "FILE COPY item 1 text must be EXACTLY '%.64s' (Law 4 canon byte-exact), got '%.128s'",
             FLAIR_CANON_FILECOPY_MSG, item1_text);
    CHECK(strcmp(item1_text, FLAIR_CANON_FILECOPY_MSG) == 0, msg);

    /* Item 2: ctrlItem with progressBar, value=0, max=100. */
    flair_dialog_item_type_t type2;
    rgn_rect_t               rect2;
    GetDialogItem(dp, 2, &type2, &rect2);
    CHECK(type2 == ctrlItem, "FILE COPY item 2 must be ctrlItem");
    CHECK(dp->items[1].ctrl != 0, "FILE COPY item 2 ctrl pointer must be non-NULL");
    if (dp->items[1].ctrl) {
        snprintf(msg, sizeof msg,
                 "FILE COPY progress bar contrlType must be progressBar (%d), got %d",
                 (int)progressBar, (int)dp->items[1].ctrl->contrlType);
        CHECK(dp->items[1].ctrl->contrlType == progressBar, msg);

        CHECK(dp->items[1].ctrl->contrlValue == 0,
              "FILE COPY progress bar initial value must be 0");
        CHECK(dp->items[1].ctrl->contrlMax == 100,
              "FILE COPY progress bar max must be 100");
        CHECK(dp->items[1].ctrl->contrlMin == 0,
              "FILE COPY progress bar min must be 0");
    }

    /* Dialog window kind is dialogKind, variant is dBoxProc. */
    CHECK(dp->window.windowKind == (int16_t)dialogKind,
          "FILE COPY dialog windowKind must be dialogKind=2");
    CHECK(dp->window.windowDefProcVariant == (int16_t)dBoxProc,
          "FILE COPY dialog windowDefProcVariant must be dBoxProc=1");

    /* Bounds: the canonical 360x80 box (per FileCopyDialog layout). */
    CHECK(dp->window.port.portRect.left   == 140 &&
          dp->window.port.portRect.top    == 200 &&
          dp->window.port.portRect.right  == 500 &&
          dp->window.port.portRect.bottom == 280,
          "FILE COPY dialog bounds must be {top=200,left=140,bottom=280,right=500}");
}

/* ===========================================================================
 * PROPERTY 2 -- MODALDIALOG event routing
 *
 * We feed pre-built raw events into a flair_raw_ring_t, then call ModalDialog
 * with sleepTicks=0 (returns immediately on empty ring). This makes the loop
 * deterministic: it drains events, acts on the first dismissing one, or
 * returns itemHit=0 if nothing dismisses it (e.g. disabled item click).
 * ===========================================================================*/

/* Helper: post a mouseDown at absolute global screen position (h, v).
 * Synthesizes raw FLAIR_RAW_MOUSE events that WaitNextEvent will cook into
 * a mouseDown EventRecord at (h, v).
 *
 * Ref: spec/event_model.h flair_raw_event_t MOUSE payload layout:
 *   bits 0..7   = button byte (bit 0 = left button down).
 *   bits 8..15  = signed delta X as int8_t.
 *   bits 16..23 = signed delta Y as int8_t.
 *
 * Ref: event.c flair_event_init -- cursor starts at screen CENTER:
 *   g_cursor_h = FLAIR_SCREEN_W / 2 = 320
 *   g_cursor_v = FLAIR_SCREEN_H / 2 = 240
 * (ADR-0004 OD-3; event.c line ~138).
 * So to reach absolute position (h, v) we post delta (h - 320, v - 240)
 * from the initial center, in steps of <=127 px.
 * IMPORTANT: call flair_event_init() before each test to reset cursor.
 */
static void post_mousedown(flair_raw_ring_t *ring, int h, int v)
{
    /* Walk cursor from screen center (320, 240) to (h, v). */
    int remaining_h = h - (FLAIR_SCREEN_W / 2);
    int remaining_v = v - (FLAIR_SCREEN_H / 2);
    while (remaining_h != 0 || remaining_v != 0) {
        int dh = remaining_h;
        int dv = remaining_v;
        /* Clamp to int8_t range [-127, 127]. */
        if (dh >  127) { dh =  127; }
        if (dh < -127) { dh = -127; }
        if (dv >  127) { dv =  127; }
        if (dv < -127) { dv = -127; }
        /* Post a mouse-move event (no button down). */
        flair_raw_event_t mov;
        memset(&mov, 0, sizeof mov);
        mov.kind    = (uint32_t)FLAIR_RAW_MOUSE;
        mov.payload = (uint32_t)((uint8_t)0 |                        /* no button */
                                 ((uint8_t)(int8_t)dh << 8) |
                                 ((uint8_t)(int8_t)dv << 16));
        flair_raw_post(ring, &mov);
        remaining_h -= dh;
        remaining_v -= dv;
    }
    /* Now post the button-down event (left button, no additional delta). */
    flair_raw_event_t raw;
    memset(&raw, 0, sizeof raw);
    raw.kind    = (uint32_t)FLAIR_RAW_MOUSE;
    raw.payload = (uint32_t)(1u |    /* bit 0 = left button down */
                             (0u << 8) |   /* dx=0 */
                             (0u << 16));  /* dy=0 */
    flair_raw_post(ring, &raw);
}

/* Helper: post a keyDown with character `ch`.
 * Ref: spec/event_model.h KEYBOARD payload: low byte = raw PS/2 scancode;
 * bit 8 = break flag (0 for key-down).
 */
static void post_keydown(flair_raw_ring_t *ring, uint8_t ch)
{
    flair_raw_event_t raw;
    memset(&raw, 0, sizeof raw);
    raw.kind    = (uint32_t)FLAIR_RAW_KEYBOARD;
    raw.payload = (uint32_t)(ch & 0xFFu); /* scancode, bit 8 = 0 (key-down) */
    flair_raw_post(ring, &raw);
}

/* Setup: a simple dialog with 3 items:
 *   item 1 = ctrlItem (pushButton), enabled=1 -> can be returned
 *   item 2 = statText, enabled=0               -> must NOT be returned
 *   item 3 = ctrlItem (pushButton), enabled=0  -> must NOT be returned (disabled)
 * defaultItem = 1, cancelItem = 0 (no cancel for first sub-test)
 */
static DialogRecord  g_mdl_dr;
static DialogItem    g_mdl_items[3];
static ControlRecord g_mdl_ctrl[2];
static rgn_store_t   g_mdl_sruc, g_mdl_scont, g_mdl_supd;

static DialogPtr setup_modal_dialog(uint16_t defaultItem, uint16_t cancelItem)
{
    rgn_store_init(&g_mdl_sruc);
    rgn_store_init(&g_mdl_scont);
    rgn_store_init(&g_mdl_supd);

    rgn_rect_t bounds;
    bounds.top = 100; bounds.left = 100; bounds.bottom = 300; bounds.right = 500;

    /* Item 1: enabled push button at [120,120) to [220,150). */
    rgn_rect_t r1;
    r1.top = 120; r1.left = 120; r1.bottom = 150; r1.right = 220;
    control_init(&g_mdl_ctrl[0], pushButton, r1, 0, 0, 1, 1, "OK");
    g_mdl_items[0].type    = ctrlItem;
    g_mdl_items[0].rect    = r1;
    g_mdl_items[0].text    = 0;
    g_mdl_items[0].ctrl    = &g_mdl_ctrl[0];
    g_mdl_items[0].enabled = 1;
    g_mdl_items[0]._pad[0] = g_mdl_items[0]._pad[1] = g_mdl_items[0]._pad[2] = 0;

    /* Item 2: statText (never returned). */
    rgn_rect_t r2;
    r2.top = 170; r2.left = 120; r2.bottom = 185; r2.right = 400;
    g_mdl_items[1].type    = statText;
    g_mdl_items[1].rect    = r2;
    g_mdl_items[1].text    = "Status text";
    g_mdl_items[1].ctrl    = 0;
    g_mdl_items[1].enabled = 0;
    g_mdl_items[1]._pad[0] = g_mdl_items[1]._pad[1] = g_mdl_items[1]._pad[2] = 0;

    /* Item 3: disabled push button. */
    rgn_rect_t r3;
    r3.top = 200; r3.left = 120; r3.bottom = 230; r3.right = 220;
    control_init(&g_mdl_ctrl[1], pushButton, r3, 0, 0, 1, 1, "Cancel");
    g_mdl_items[2].type    = ctrlItem;
    g_mdl_items[2].rect    = r3;
    g_mdl_items[2].text    = 0;
    g_mdl_items[2].ctrl    = &g_mdl_ctrl[1];
    g_mdl_items[2].enabled = 0;
    g_mdl_items[2]._pad[0] = g_mdl_items[2]._pad[1] = g_mdl_items[2]._pad[2] = 0;

    return NewDialog(&g_mdl_dr, bounds, "",
                     g_mdl_items, 3u,
                     defaultItem, cancelItem,
                     0, &g_mdl_sruc.r, &g_mdl_scont.r, &g_mdl_supd.r);
}

static flair_raw_ring_t g_ring;

static void test_modaldialog_click_ok(void)
{
    /* (a) mouseDown in enabled ctrlItem -> returns itemHit = 1. */
    flair_event_init(&g_ring);

    DialogPtr dp = setup_modal_dialog(1, 0);
    CHECK(dp != 0, "setup_modal_dialog must succeed");
    if (!dp) { return; }

    /* Post a mouseDown inside item 1's rect.
     * Ref: event.c flair_event_init -- cursor starts at SCREEN CENTER (320, 240).
     * portRect = bounds = {top=100, left=100, ...}.
     * ModalDialog: local.h = global.h - portRect.left; local.v = global.v - portRect.top.
     * r1 (local): left=120, right=220, top=120, bottom=150.
     * To hit local (200, 135): global (300, 235).
     * post_mousedown posts delta from center (320,240) -> (-20, -5); small int8_t.
     */
    post_mousedown(&g_ring, 300, 235);

    uint16_t itemHit = 0;
    ModalDialog(dp, &g_ring, 0, 0, &itemHit);
    CHECK(itemHit == 1,
          "ModalDialog: mouseDown in enabled ctrlItem (item 1) -> itemHit=1");
}

static void test_modaldialog_return_key(void)
{
    /* (b) Return keypress -> defaultItem.
     * PS/2 SET-1 Return make code = 0x1C.
     * The pump maps: sc=0x1C -> sc_unshifted[0x1C] = '\n' (0x0A) as ASCII,
     * vkey = 0x1C. ModalDialog checks (ascii==0x0A || vkey==0x1C). */
    flair_event_init(&g_ring);

    DialogPtr dp = setup_modal_dialog(1, 0);
    CHECK(dp != 0, "setup_modal_dialog must succeed");
    if (!dp) { return; }

    /* PS/2 SET-1 Return scancode = 0x1C. */
    post_keydown(&g_ring, 0x1Cu);

    uint16_t itemHit = 0;
    ModalDialog(dp, &g_ring, 0, 0, &itemHit);
    CHECK(itemHit == 1,
          "ModalDialog: Return key (scancode 0x1C) -> defaultItem=1");
}

static void test_modaldialog_escape_key(void)
{
    /* (c) Escape -> cancelItem.
     * PS/2 SET-1 Escape make code = 0x01.
     * The pump maps: sc=0x01 -> sc_unshifted[0x01] = 0 (ASCII 0, no char),
     * vkey = 0x01. ModalDialog checks (ascii==0x1B || vkey==0x01). */
    flair_event_init(&g_ring);

    /* Set up with cancelItem=3 (item 3 is the disabled button, but cancelItem
     * is the item INDEX to return on Escape, regardless of enabled state). */
    DialogPtr dp = setup_modal_dialog(1, 3);
    CHECK(dp != 0, "setup_modal_dialog must succeed");
    if (!dp) { return; }

    /* PS/2 SET-1 Escape scancode = 0x01. */
    post_keydown(&g_ring, 0x01u);

    uint16_t itemHit = 0;
    ModalDialog(dp, &g_ring, 0, 0, &itemHit);
    CHECK(itemHit == 3,
          "ModalDialog: Escape key -> cancelItem=3");
}

static void test_modaldialog_click_disabled(void)
{
    /* (d) Click in a DISABLED item -> NOT returned. ModalDialog drains and
     * returns itemHit=0 when sleepTicks=0 and ring is empty. */
    flair_event_init(&g_ring);

    DialogPtr dp = setup_modal_dialog(1, 0);
    CHECK(dp != 0, "setup_modal_dialog must succeed");
    if (!dp) { return; }

    /* Click in item 3 (disabled button) at local (170, 215) ->
     * global (100+170, 100+215) = (270, 315).
     * Cursor starts at screen center (320, 240); delta = (-50, 75). */
    post_mousedown(&g_ring, 270, 315);

    uint16_t itemHit = 99; /* start with non-zero to prove it's cleared */
    ModalDialog(dp, &g_ring, 0, 0, &itemHit);
    /* sleepTicks=0: after draining the ring without hitting an enabled item,
     * ModalDialog returns with itemHit unchanged at its initial *itemHit value,
     * which is 0 (set to 0 at entry). The ring is now empty. */
    CHECK(itemHit == 0,
          "ModalDialog: click in DISABLED item -> itemHit=0 (not returned)");
}

static void test_modaldialog_click_stattext(void)
{
    /* (e) Click in statText item -> NOT returned.
     *
     * MUTATION: DIALOG_MUTATE_HIT_STATIC causes ModalDialog to return statText
     * items, making this check go RED. Without the mutant, statText clicks are
     * silently ignored. (IM-I Ch 6: "statText items cannot be enabled.")
     */
    flair_event_init(&g_ring);

    DialogPtr dp = setup_modal_dialog(1, 0);
    CHECK(dp != 0, "setup_modal_dialog must succeed");
    if (!dp) { return; }

    /* Click in item 2 (statText) at local (200, 177) -> global (300, 277).
     * r2: top=170, left=120, bottom=185, right=400 (local).
     * Global: h=100+200=300, v=100+177=277. */
    post_mousedown(&g_ring, 300, 277);

    uint16_t itemHit = 0;
    ModalDialog(dp, &g_ring, 0, 0, &itemHit);

#if defined(DIALOG_MUTATE_HIT_STATIC) && DIALOG_MUTATE_HIT_STATIC
    /* NAMED MUTANT: statText returned as enabled -> this check MUST FAIL (RED). */
    CHECK(itemHit == 0,
          "DIALOG_MUTATE_HIT_STATIC: statText click must NOT return itemHit "
          "(this MUST fail -- the oracle is RED as required by Rule 6)");
#else
    CHECK(itemHit == 0,
          "ModalDialog: click in statText item -> itemHit=0 (statText cannot be enabled)");
#endif
}

/* ===========================================================================
 * PROPERTY 3 -- DRAW: render FILE COPY dialog and assert pixel values
 *
 * Draw the FILE COPY dialog into an 8bpp offscreen. Assert:
 *   (a) Border: pixels within the first 7 rows/columns from the dialog edge
 *       are DLG_BLACK (palette index 0).
 *   (b) Content interior: pixel just inside the border is DLG_WHITE (index 1).
 *   (c) Progress bar: the fill region and border are drawn correctly.
 *
 * MUTATION: DIALOG_MUTATE_BORDER draws 8-px border instead of 7; the check
 * for the 8th interior pixel being DLG_WHITE goes RED (it would be BLACK).
 * ===========================================================================*/
static void draw_filecopy_dialog(GrafPort *port)
{
    static DialogRecord  s_dr;
    static DialogItem    s_items[2];
    static ControlRecord s_ctrl;

    /* We need region storage; use static pools for the draw callback. */
    static rgn_store_t s_sruc, s_scont, s_supd;
    static int s_inited = 0;
    if (!s_inited) {
        rgn_store_init(&s_sruc);
        rgn_store_init(&s_scont);
        rgn_store_init(&s_supd);
        s_inited = 1;
    } else {
        /* Re-init each draw to start clean. */
        rgn_store_init(&s_sruc);
        rgn_store_init(&s_scont);
        rgn_store_init(&s_supd);
    }

    DialogPtr dp = FileCopyDialog(&s_dr, s_items, &s_ctrl,
                                  &s_sruc.r, &s_scont.r, &s_supd.r);
    if (!dp) {
        return;
    }

    /* Point dialog port at the render port (same offscreen). */
    dp->window.port = *port;
    /* Reset portRect to dialog bounds (overwrite the render port's full-screen rect). */
    dp->window.port.portRect = dp->window.strucRgn->bbox;

    DrawDialog(dp);
}

static void test_draw_filecopy(void)
{
    render_ctx_t ctx;
    int rc = render_one(&ctx, 8u);
    CHECK(rc == 0, "render_ctx_init(8bpp) for FILE COPY draw test must succeed");
    if (rc != 0) { return; }

    render_run(&ctx, draw_filecopy_dialog);

    char msg[300];

    /* Dialog bounds: top=200, left=140, right=500, bottom=280.
     * Border width: FLAIR_CHROME_DIALOG_BORDER = 7 px (DLG_BLACK = 0).
     *
     * (a) Border check: the first 7 rows/columns from each edge are BLACK.
     *
     * Check: top border interior row at y=200+3=203, x=200 (center of dialog):
     *   -> must be DLG_BLACK (palette index 0). */
    snprintf(msg, sizeof msg,
             "top border at (x=200,y=203) [3 rows below top=200] must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 200u, 203u));
    CHECK(pidx(&ctx, 200u, 203u) == 0u, msg);

    /* Left border at x=140+3=143, y=240 (center of dialog height). */
    snprintf(msg, sizeof msg,
             "left border at (x=143,y=240) [3 cols right of left=140] must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 143u, 240u));
    CHECK(pidx(&ctx, 143u, 240u) == 0u, msg);

    /* Right border at x=500-4=496, y=240. */
    snprintf(msg, sizeof msg,
             "right border at (x=496,y=240) [4 cols left of right=500] must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 496u, 240u));
    CHECK(pidx(&ctx, 496u, 240u) == 0u, msg);

    /* Bottom border at y=280-4=276, x=200. */
    snprintf(msg, sizeof msg,
             "bottom border at (x=200,y=276) [4 rows above bottom=280] must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 200u, 276u));
    CHECK(pidx(&ctx, 200u, 276u) == 0u, msg);

    /* (b) Content interior: pixel at border+1 inside must be DLG_WHITE (1).
     * The content starts at top+border = 200+7=207, left+border = 140+7=147.
     * Check: (x=147, y=207) -> DLG_WHITE (1).
     *
     * MUTATION CHECK (DIALOG_MUTATE_BORDER = 8 px):
     *   With the mutant, the border covers 8 rows: top..top+7. So the pixel
     *   at y=207 (= top+7) would still be BLACK (inside the 8-px border),
     *   making this check fail -> oracle goes RED.
     *   With correct 7-px border, y=207 (= top+7) is the first WHITE row
     *   (border covers [top, top+7) = [200, 207), content starts at 207). */
    snprintf(msg, sizeof msg,
             "content interior at (x=147,y=207) [border+0=7 rows from top] must be "
             "DLG_WHITE (1), got %u",
             (unsigned)pidx(&ctx, 147u, 207u));
    CHECK(pidx(&ctx, 147u, 207u) == 1u, msg);

    /* (c) Progress bar item rect: top=200+36=236, left=140+14=154, right=140+346=486, bottom=200+56=256.
     * Progress bar at value=0: no fill; inner region should be DLG_WHITE (1).
     * Border of progress bar at x=154, y=246 (center height) -> DLG_BLACK (0). */
    snprintf(msg, sizeof msg,
             "progress bar left border (x=154,y=246) must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 154u, 246u));
    CHECK(pidx(&ctx, 154u, 246u) == 0u, msg);

    /* Progress bar interior at value=0: x=160, y=246 -> DLG_WHITE (1). */
    snprintf(msg, sizeof msg,
             "progress bar interior at value=0 (x=160,y=246) must be DLG_WHITE (1), got %u",
             (unsigned)pidx(&ctx, 160u, 246u));
    CHECK(pidx(&ctx, 160u, 246u) == 1u, msg);

    render_ctx_free(&ctx);
}

/* ===========================================================================
 * PROPERTY 3b -- DRAW: verify the border-frame pixel at position (border-1)
 * is BLACK and (border) is WHITE, proving FLAIR_CHROME_DIALOG_BORDER=7.
 *
 * This is an INDEPENDENT structural check that does NOT rely on a single
 * y-coordinate arithmetic calculation -- it samples a swept range.
 *
 * For the top border: pixels at y in [top, top+bw) should all be BLACK;
 * pixels at y = top+bw should be WHITE.
 * ===========================================================================*/
static void test_draw_border_sweep(void)
{
    render_ctx_t ctx;
    int rc = render_one(&ctx, 8u);
    CHECK(rc == 0, "render_ctx_init(8bpp) for border sweep test must succeed");
    if (rc != 0) { return; }

    render_run(&ctx, draw_filecopy_dialog);

    char msg[300];
    int bw = FLAIR_CHROME_DIALOG_BORDER;  /* 7 */
    int top  = 200;
    int left = 140;
    int x_probe = 200;  /* well inside the dialog horizontally */

    /* Sweep top border rows. */
    for (int row = top; row < top + bw; row++) {
        snprintf(msg, sizeof msg,
                 "top border pixel at (x=%d,y=%d) must be DLG_BLACK (0), got %u",
                 x_probe, row, (unsigned)pidx(&ctx, (uint32_t)x_probe, (uint32_t)row));
        CHECK(pidx(&ctx, (uint32_t)x_probe, (uint32_t)row) == 0u, msg);
    }

    /* The row immediately after the border must be WHITE. */
    snprintf(msg, sizeof msg,
             "first content row at (x=%d,y=%d) [= top+bw=%d] must be DLG_WHITE (1), got %u",
             x_probe, top + bw, top + bw, (unsigned)pidx(&ctx, (uint32_t)x_probe, (uint32_t)(top + bw)));
    CHECK(pidx(&ctx, (uint32_t)x_probe, (uint32_t)(top + bw)) == 1u, msg);

    /* Sweep left border columns. */
    int y_probe = 240;
    for (int col = left; col < left + bw; col++) {
        snprintf(msg, sizeof msg,
                 "left border pixel at (x=%d,y=%d) must be DLG_BLACK (0), got %u",
                 col, y_probe, (unsigned)pidx(&ctx, (uint32_t)col, (uint32_t)y_probe));
        CHECK(pidx(&ctx, (uint32_t)col, (uint32_t)y_probe) == 0u, msg);
    }

    /* First content column after the left border must be WHITE. */
    snprintf(msg, sizeof msg,
             "first content col at (x=%d,y=%d) [= left+bw=%d] must be DLG_WHITE (1), got %u",
             left + bw, y_probe, left + bw,
             (unsigned)pidx(&ctx, (uint32_t)(left + bw), (uint32_t)y_probe));
    CHECK(pidx(&ctx, (uint32_t)(left + bw), (uint32_t)y_probe) == 1u, msg);

    render_ctx_free(&ctx);

    (void)lcg(); /* ensure LCG is exercised so the seed isn't dead weight */
}

/* ===========================================================================
 * PROPERTY: GetDialogItem / SetDialogItem / GetDialogItemText / SetDialogItemText
 * ===========================================================================*/
static void test_item_accessors(void)
{
    DialogRecord dr;
    DialogItem   items[1];
    ControlRecord ctrl;

    rgn_store_t s_struc, s_cont, s_upd;
    rgn_store_init(&s_struc);
    rgn_store_init(&s_cont);
    rgn_store_init(&s_upd);

    rgn_rect_t bounds;
    bounds.top = 100; bounds.left = 100; bounds.bottom = 200; bounds.right = 300;
    rgn_rect_t r1;
    r1.top = 120; r1.left = 110; r1.bottom = 140; r1.right = 290;
    control_init(&ctrl, pushButton, r1, 0, 0, 1, 1, "OK");

    items[0].type    = ctrlItem;
    items[0].rect    = r1;
    items[0].text    = 0;
    items[0].ctrl    = &ctrl;
    items[0].enabled = 1;
    items[0]._pad[0] = items[0]._pad[1] = items[0]._pad[2] = 0;

    DialogPtr dp = NewDialog(&dr, bounds, "", items, 1u, 1u, 0u, 0,
                             &s_struc.r, &s_cont.r, &s_upd.r);
    CHECK(dp != 0, "NewDialog for accessor test must succeed");
    if (!dp) { return; }

    /* GetDialogItem: check type and rect. */
    flair_dialog_item_type_t type;
    rgn_rect_t               rect;
    GetDialogItem(dp, 1, &type, &rect);
    CHECK(type == ctrlItem, "GetDialogItem(1): type must be ctrlItem");
    CHECK(rect.top == 120 && rect.left == 110,
          "GetDialogItem(1): rect top/left must match");

    /* SetDialogItem: change to statText. */
    rgn_rect_t new_rect;
    new_rect.top = 130; new_rect.left = 110; new_rect.bottom = 145; new_rect.right = 290;
    SetDialogItem(dp, 1, statText, new_rect, "Hello", 0, 0);
    GetDialogItem(dp, 1, &type, &rect);
    CHECK(type == statText, "SetDialogItem -> statText: GetDialogItem must reflect change");
    CHECK(rect.top == 130, "SetDialogItem -> rect.top must be updated to 130");

    /* GetDialogItemText on a statText item. */
    char buf[64];
    GetDialogItemText(dp, 1, buf, sizeof buf);
    CHECK(strcmp(buf, "Hello") == 0, "GetDialogItemText: must return 'Hello'");

    /* SetDialogItemText on the statText item. */
    SetDialogItemText(dp, 1, "World");
    GetDialogItemText(dp, 1, buf, sizeof buf);
    CHECK(strcmp(buf, "World") == 0, "SetDialogItemText: GetDialogItemText must return 'World'");
}

/* ===========================================================================
 * PROPERTY: FindDialogItem hit-testing
 * ===========================================================================*/
static void test_find_dialog_item(void)
{
    DialogRecord dr;
    DialogItem   items[2];

    rgn_store_t s_struc, s_cont, s_upd;
    rgn_store_init(&s_struc);
    rgn_store_init(&s_cont);
    rgn_store_init(&s_upd);

    rgn_rect_t bounds;
    bounds.top = 100; bounds.left = 100; bounds.bottom = 300; bounds.right = 500;

    rgn_rect_t r1;
    r1.top = 110; r1.left = 110; r1.bottom = 130; r1.right = 200;
    items[0].type = statText; items[0].rect = r1;
    items[0].text = "A"; items[0].ctrl = 0; items[0].enabled = 0;
    items[0]._pad[0] = items[0]._pad[1] = items[0]._pad[2] = 0;

    rgn_rect_t r2;
    r2.top = 140; r2.left = 110; r2.bottom = 160; r2.right = 250;
    items[1].type = statText; items[1].rect = r2;
    items[1].text = "B"; items[1].ctrl = 0; items[1].enabled = 0;
    items[1]._pad[0] = items[1]._pad[1] = items[1]._pad[2] = 0;

    DialogPtr dp = NewDialog(&dr, bounds, "", items, 2u, 0u, 0u, 0,
                             &s_struc.r, &s_cont.r, &s_upd.r);
    CHECK(dp != 0, "NewDialog for find-item test must succeed");
    if (!dp) { return; }

    /* Point inside r1 -> item 1. */
    flair_point_t pt1;
    pt1.h = 150; pt1.v = 120;
    CHECK(FindDialogItem(dp, pt1) == 1,
          "FindDialogItem: point in r1 must return 1");

    /* Point inside r2 -> item 2. */
    flair_point_t pt2;
    pt2.h = 150; pt2.v = 150;
    CHECK(FindDialogItem(dp, pt2) == 2,
          "FindDialogItem: point in r2 must return 2");

    /* Point outside all items -> 0. */
    flair_point_t pt0;
    pt0.h = 50; pt0.v = 50;
    CHECK(FindDialogItem(dp, pt0) == 0,
          "FindDialogItem: point outside all items -> 0");

    /* Point at exact left edge of r1 (half-open: left <= h < right). */
    flair_point_t ptl;
    ptl.h = r1.left; ptl.v = 120;
    CHECK(FindDialogItem(dp, ptl) == 1,
          "FindDialogItem: point at left edge of r1 (half-open: included) -> 1");

    /* Point at exact right edge of r1 (half-open: NOT included). */
    flair_point_t ptr;
    ptr.h = r1.right; ptr.v = 120;
    CHECK(FindDialogItem(dp, ptr) == 0,
          "FindDialogItem: point at right edge of r1 (half-open: excluded) -> 0");
}

/* ===========================================================================
 * main -- run all properties; report.
 * ===========================================================================*/
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("test-dialog: FLAIR Dialog Manager oracle\n");

    test_layout_basic();
    test_filecopy_layout();
    test_modaldialog_click_ok();
    test_modaldialog_return_key();
    test_modaldialog_escape_key();
    test_modaldialog_click_disabled();
    test_modaldialog_click_stattext();
    test_draw_filecopy();
    test_draw_border_sweep();
    test_item_accessors();
    test_find_dialog_item();

    return TEST_SUMMARY("test-dialog");
}
