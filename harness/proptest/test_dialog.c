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

    /* Item 2: ctrlItem with progressBar, canon value, max=100.
     * MUTATION: DIALOG_MUTATE_PROGRESS_ZERO reverts to the old bug (value=0)
     * -> RED (Law 4, beads initech-a90f). */
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

        snprintf(msg, sizeof msg,
                 "FILE COPY progress bar initial value must be "
                 "FLAIR_CANON_FILECOPY_PROGRESS (%d; ~65-70%% per bug-hunt #25, "
                 "initech-a90f), got %d",
                 FLAIR_CANON_FILECOPY_PROGRESS, (int)dp->items[1].ctrl->contrlValue);
        CHECK(dp->items[1].ctrl->contrlValue == FLAIR_CANON_FILECOPY_PROGRESS, msg);
        CHECK(dp->items[1].ctrl->contrlValue != 0,
              "FILE COPY progress bar initial value must NOT be 0 (initech-a90f "
              "the old bug: value=0 renders zero fill)");
        CHECK(dp->items[1].ctrl->contrlMax == 100,
              "FILE COPY progress bar max must be 100");
        CHECK(dp->items[1].ctrl->contrlMin == 0,
              "FILE COPY progress bar min must be 0");
    }

    /* Dialog window kind is dialogKind; variant is movableDBoxProc (the
     * moveable titled modal, beads initech-zvo6) -- NOT dBoxProc (the old
     * titleless 7px-border box). MUTATION: DIALOG_MUTATE_TITLELESS_MODAL
     * reverts to dBoxProc -> RED (Law 4). */
    CHECK(dp->window.windowKind == (int16_t)dialogKind,
          "FILE COPY dialog windowKind must be dialogKind=2");
    snprintf(msg, sizeof msg,
             "FILE COPY dialog windowDefProcVariant must be movableDBoxProc=5 "
             "(the moveable titled modal, initech-zvo6), got %d",
             (int)dp->window.windowDefProcVariant);
    CHECK(dp->window.windowDefProcVariant == (int16_t)movableDBoxProc, msg);

    /* The window title is the byte-exact canon "FILE COPY" (Law 4). */
    snprintf(msg, sizeof msg,
             "FILE COPY dialog title must be EXACTLY '%.32s' (Law 4 canon), "
             "got '%.64s'",
             FLAIR_CANON_FILECOPY_TITLE, dp->window.titleHandle);
    CHECK(strcmp(dp->window.titleHandle, FLAIR_CANON_FILECOPY_TITLE) == 0, msg);

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

/* ===========================================================================
 * PROPERTY 3 -- DRAW: the FILE COPY modal renders the MOVEABLE TITLED chrome
 * (beads initech-zvo6): a pinstripe title bar + a PLAIN 1-px frame -- NOT the
 * old titleless dBoxProc 7-px solid border. Dialog bounds: top=200, left=140,
 * right=500, bottom=280. Title band geometry (chrome_metrics.h):
 *   y=200            outer top frame line          -> DLG_BLACK (0)
 *   y=201            bevel-hi                       -> idx 2 (CIDX_DESKTOP)
 *   y=202..216       15-row pinstripe                -> idx 7 or 8
 *   y=217            bevel-lo                        -> idx 4 (CIDX_TITLE_INK)
 *   y=218            shared frame line                -> DLG_BLACK (0)
 *   y=219.. (=top+FLAIR_CHROME_TITLEBAR_H) content    -> DLG_WHITE (1)
 * MUTATION: DIALOG_MUTATE_TITLELESS_MODAL reverts to the OLD titleless
 * dBoxProc box (thick 7px border, no title bar) -> these checks go RED.
 * ===========================================================================*/
static void test_draw_filecopy(void)
{
    render_ctx_t ctx;
    int rc = render_one(&ctx, 8u);
    CHECK(rc == 0, "render_ctx_init(8bpp) for FILE COPY draw test must succeed");
    if (rc != 0) { return; }

    render_run(&ctx, draw_filecopy_dialog);

    char msg[300];

    /* (a) Title bar band present: bevel-hi (y=201) is idx 2, NOT black/white.
     * Probed at x=145: comfortably left of the title-text clamp (left+23=163,
     * close-zoom-box.md clearance), so never inside the text knockout. */
    snprintf(msg, sizeof msg,
             "title bevel-hi row (x=145,y=201) must be idx 2 (CIDX_DESKTOP "
             "bevel-light), got %u -- proves a title bar is drawn (initech-zvo6)",
             (unsigned)pidx(&ctx, 145u, 201u));
    CHECK(pidx(&ctx, 145u, 201u) == 2u, msg);

    /* (b) The pinstripe band (y=202..216) contains BOTH light (7) and dark (8)
     * shades -- proves it is NOT the old solid-black 7px border (which would
     * read idx 0 for every one of these rows). Probed at x=145 (left of the
     * title-text knockout, which would otherwise mask the alternation). */
    {
        int saw_light = 0, saw_dark = 0;
        for (unsigned y = 202u; y <= 216u; y++) {
            uint32_t v = pidx(&ctx, 145u, y);
            if (v == 7u) { saw_light = 1; }
            if (v == 8u) { saw_dark  = 1; }
        }
        CHECK(saw_light && saw_dark,
              "title bar pinstripe rows (x=145,y=202..216) must show BOTH idx7 "
              "(light) and idx8 (dark) -- proves a pinstripe title bar, not "
              "a solid black border (initech-zvo6)");
    }

    /* (c) The shared frame line (y=218, the bottom of the title band) is black.
     * Probed at x=145 (left of the title-text knockout). */
    snprintf(msg, sizeof msg,
             "title band shared frame line (x=145,y=218) must be DLG_BLACK (0), "
             "got %u", (unsigned)pidx(&ctx, 145u, 218u));
    CHECK(pidx(&ctx, 145u, 218u) == 0u, msg);

    /* (d) 'FILE COPY' title text is RENDERED: a CIDX_TITLE_INK (idx 4) pixel
     * appears somewhere in the pinstripe band (y=202..216) -- idx 4 never
     * otherwise appears there (only 7/8), so its presence proves ink, not
     * just a knockout gap. Scanning stops BEFORE y=217 (the bevel-lo row,
     * which is idx 4 by construction and would be a false positive). */
    {
        int saw_ink = 0;
        for (int x = 140; x < 500 && !saw_ink; x++) {
            for (unsigned y = 202u; y <= 216u; y++) {
                if (pidx(&ctx, (uint32_t)x, y) == 4u) { saw_ink = 1; break; }
            }
        }
        CHECK(saw_ink,
              "FILE COPY title text must be RENDERED as ink (idx 4) somewhere "
              "in the pinstripe band (initech-zvo6)");
    }

    /* (e) The frame is PLAIN 1-px, NOT the old 7px dBoxProc border: the pixel
     * ONE column inside the left edge (x=141), at a CONTENT row (y=240, well
     * below the 19px title band), is WHITE -- under the old 7px border this
     * pixel (x=140+1) would still be solid BLACK (border spans x=140..146).
     * MUTATION: DIALOG_MUTATE_TITLELESS_MODAL reverts to the 7px border ->
     * this pixel goes BLACK -> RED. */
    snprintf(msg, sizeof msg,
             "content just inside the LEFT frame (x=141,y=240) must be "
             "DLG_WHITE (1) -- proves a 1px frame, not the old 7px dBoxProc "
             "border (initech-zvo6), got %u",
             (unsigned)pidx(&ctx, 141u, 240u));
    CHECK(pidx(&ctx, 141u, 240u) == 1u, msg);

    /* Symmetric check on the right edge (x=498, one col inside x=499). */
    snprintf(msg, sizeof msg,
             "content just inside the RIGHT frame (x=498,y=240) must be "
             "DLG_WHITE (1), got %u",
             (unsigned)pidx(&ctx, 498u, 240u));
    CHECK(pidx(&ctx, 498u, 240u) == 1u, msg);

    /* The outermost columns/rows themselves are still the frame ink (black). */
    snprintf(msg, sizeof msg,
             "outer left frame column (x=140,y=240) must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 140u, 240u));
    CHECK(pidx(&ctx, 140u, 240u) == 0u, msg);
    snprintf(msg, sizeof msg,
             "outer bottom frame row (x=300,y=279) must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 300u, 279u));
    CHECK(pidx(&ctx, 300u, 279u) == 0u, msg);

    /* (f) Progress bar: item rect left=154, top=249, right=486, bottom=269
     * (re-based below the 19px title band; was top=236/bottom=256 under the
     * old 7px-border layout). inner_w = (486-154)-2 = 330; at
     * FLAIR_CANON_FILECOPY_PROGRESS (68), filled_px = 330*68/100 = 224
     * (integer division), so x in [155, 155+224)=[155,379) reads CTRL_ACCENT
     * (idx 5, navy blue "solid blue fill"); x >= 379 (still inside the bar)
     * reads white (idx 1). MUTATION: DIALOG_MUTATE_PROGRESS_ZERO reverts
     * value to 0 -> filled_px=0 -> the fill-side check goes RED (Law 4,
     * initech-a90f). */
    snprintf(msg, sizeof msg,
             "progress bar left border (x=154,y=259) must be DLG_BLACK (0), got %u",
             (unsigned)pidx(&ctx, 154u, 259u));
    CHECK(pidx(&ctx, 154u, 259u) == 0u, msg);

    snprintf(msg, sizeof msg,
             "progress bar FILL at (x=160,y=259) must be idx 5 (CTRL_ACCENT "
             "navy blue) -- proves a non-zero fill (initech-a90f), got %u",
             (unsigned)pidx(&ctx, 160u, 259u));
    CHECK(pidx(&ctx, 160u, 259u) == 5u, msg);

    snprintf(msg, sizeof msg,
             "progress bar UNFILLED remainder at (x=400,y=259) must be "
             "DLG_WHITE (1), got %u",
             (unsigned)pidx(&ctx, 400u, 259u));
    CHECK(pidx(&ctx, 400u, 259u) == 1u, msg);

    render_ctx_free(&ctx);
}

/* ===========================================================================
 * PROPERTY 3b -- DRAW: a GENERIC dBoxProc dialog (built directly via
 * NewDialog, NOT FileCopyDialog) still gets the classic 7-px solid border --
 * verifying that the movableDBoxProc upgrade in FileCopyDialog (initech-zvo6)
 * did NOT change the default chrome every OTHER dialog gets. This is also the
 * DIALOG_MUTATE_BORDER mutant's target (unaffected by the FILE COPY chrome
 * change, since it lives in the dBoxProc branch of DrawDialog).
 *
 * Sweep: pixels at y in [top, top+bw) should all be BLACK; y = top+bw WHITE.
 * ===========================================================================*/
static void draw_generic_dbox_dialog(GrafPort *port)
{
    static DialogRecord  s_dr;
    static DialogItem    s_items[1];
    static ControlRecord s_ctrl;
    static rgn_store_t   s_sruc, s_scont, s_supd;

    rgn_store_init(&s_sruc);
    rgn_store_init(&s_scont);
    rgn_store_init(&s_supd);

    rgn_rect_t bounds;
    bounds.top = 200; bounds.left = 140; bounds.bottom = 280; bounds.right = 500;
    rgn_rect_t r1;
    r1.top = 236; r1.left = 154; r1.bottom = 256; r1.right = 486;
    control_init(&s_ctrl, progressBar, r1, 50, 0, 100, 1, "");
    s_items[0].type = ctrlItem; s_items[0].rect = r1;
    s_items[0].text = 0; s_items[0].ctrl = &s_ctrl; s_items[0].enabled = 0;
    s_items[0]._pad[0] = s_items[0]._pad[1] = s_items[0]._pad[2] = 0;

    DialogPtr dp = NewDialog(&s_dr, bounds, "", s_items, 1u, 0u, 0u, 0,
                             &s_sruc.r, &s_scont.r, &s_supd.r);
    if (!dp) {
        return;
    }
    dp->window.port = *port;
    dp->window.port.portRect = dp->window.strucRgn->bbox;
    DrawDialog(dp);
}

static void test_draw_border_sweep(void)
{
    render_ctx_t ctx;
    int rc = render_one(&ctx, 8u);
    CHECK(rc == 0, "render_ctx_init(8bpp) for border sweep test must succeed");
    if (rc != 0) { return; }

    render_run(&ctx, draw_generic_dbox_dialog);

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
