/* A document window without scroll bars or grow box (frame-owner request W1):
 * graded against the Mac OS 8.1 Get Info window, an independent capture
 * decoded here (../system7-decomp/goldens/captures/s8_get_info_doc.png).
 * Capture frame, measured by scans at y=240 and x=290: outline x=7 and x=305,
 * title band from y=25 (shared line y=46), bottom outline y=370, shadow
 * x=306 / y=371; content x=13..299, y=47..364. The expected values are the
 * capture's own pixels and these measured coordinates, never the renderer's. */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"
#include "chrome.h"
#include "window.h"
#include "flair_look.h"
#include "surface.h"
#include "test_assert.h"

TEST_HARNESS();

#ifndef CHROME_FID_CAPTURE_DIR
#define CHROME_FID_CAPTURE_DIR "../system7-decomp/goldens/captures"
#endif

typedef struct rgn_store { region_t r; rgn_row_t rows[RGN_ROWS_CAP]; int16_t pool[RGN_X_POOL_CAP]; } rgn_store_t;
static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows = s->rows; s->r.cap_rows = RGN_ROWS_CAP;
    s->r.x_pool = s->pool; s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}
static rgn_store_t g_desk, g_sa, g_sb, g_sc, g_struc, g_cont, g_upd;
static WindowMgr g_wm;
static WindowRecord g_w;
static unsigned char *g_cap;

static unsigned char *load_capture(const char *name)
{
    char path[512];
    png_image im;
    snprintf(path, sizeof path, "%s/%s", CHROME_FID_CAPTURE_DIR, name);
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) {
        fprintf(stderr, "FAIL missing independent capture %s\n", path);
        exit(2);
    }
    im.format = PNG_FORMAT_RGB;
    unsigned char *rgb = malloc(PNG_IMAGE_SIZE(im));
    if (!rgb || !png_image_finish_read(&im, NULL, rgb, 0, NULL) ||
        im.width != 640 || im.height != 480) {
        fprintf(stderr, "FAIL decode %s\n", path);
        exit(2);
    }
    return rgb;
}

static uint32_t cap(int x, int y)
{
    const unsigned char *q = g_cap + 3u * ((unsigned)y * 640u + (unsigned)x);
    return ((uint32_t)q[0] << 16) | ((uint32_t)q[1] << 8) | q[2];
}

static void draw_plain(GrafPort *port)
{
    rgn_rect_t frame = WindowFrameRect(&g_w), c = region_get_bbox(g_w.contRgn);
    flair_draw_document_window(port, flair_look_default_skin(), frame, "", 1,
                               g_w.widgetFlags);
    uint32_t white = flair_look_pixel_for_skin(port, flair_look_default_skin(),
                                               FLAIR_PART_CONTENT);
    for (int y = c.top; y < c.bottom; y++)   /* the owner's content fill */
        surface_fill_span(&port->portBits.bm, (uint32_t)c.left, (uint32_t)y,
                          (uint32_t)(c.right - c.left), white);
}

static void draw_chrome_only(GrafPort *port)
{
    flair_draw_document_window(port, flair_look_default_skin(), WindowFrameRect(&g_w),
                               "", 1, g_w.widgetFlags);
}

static int section(const render_ctx_t *r, const char *edge, int x0, int y0,
                   int dx, int dy, int n)
{
    int ok = 1;
    printf("  %-6s capture:", edge);
    for (int k = 0; k < n; k++) printf(" %06X", (unsigned)cap(x0 + k * dx, y0 + k * dy));
    printf("\n  %-6s ours   :", edge);
    for (int k = 0; k < n; k++) {
        uint32_t o = render_pixel_rgb(r, (uint32_t)(x0 + k * dx), (uint32_t)(y0 + k * dy)) & 0xFFFFFFu;
        printf(" %06X", (unsigned)o);
        ok = ok && o == cap(x0 + k * dx, y0 + k * dy);
    }
    printf("%s\n", ok ? " (match)" : " (DIFF)");
    return ok;
}

int main(void)
{
    g_cap = load_capture("s8_get_info_doc.png");

    store_attach(&g_desk); store_attach(&g_sa); store_attach(&g_sb); store_attach(&g_sc);
    store_attach(&g_struc); store_attach(&g_cont); store_attach(&g_upd);
    WindowMgr_init(&g_wm, (rgn_rect_t){0, 0, 480, 640}, &g_desk.r, &g_sa.r, &g_sb.r, &g_sc.r);
    g_w.strucRgn = &g_struc.r; g_w.contRgn = &g_cont.r; g_w.updateRgn = &g_upd.r;
    rgn_rect_t frame = { 25, 7, 371, 306 };   /* top, left, bottom, right */
    NewPlainDocumentWindow(&g_wm, &g_w, frame, documentKind, 1);

    rgn_rect_t c = region_get_bbox(g_w.contRgn);
    CHECK(c.left == 13 && c.top == 47 && c.right == 300 && c.bottom == 365,
          "content is the capture's x=13..299, y=47..364");
    CHECK((g_w.widgetFlags & (FLAIR_WINDOW_WIDGET_ZOOM | FLAIR_WINDOW_WIDGET_GROW)) == 0 &&
          (g_w.widgetFlags & FLAIR_WINDOW_WIDGET_CLOSE) &&
          (g_w.widgetFlags & FLAIR_WINDOW_WIDGET_COLLAPSE),
          "close and collapse, no zoom or grow box (Get Info)");
    CHECK(WindowScrollBand(&g_w, (flair_point_t){ .v = 200, .h = 290 }) < 0 &&
          WindowScrollBand(&g_w, (flair_point_t){ .v = 355, .h = 100 }) < 0,
          "no scroll-bar band to hit where a document's gutters would be");

    render_boot_info_t boot = { 0, 0, 8u, 640u, 480u };
    render_ctx_t r;
    CHECK(render_ctx_init(&r, &boot) == 0, "render context");
    render_run(&r, draw_plain);
    CHECK(section(&r, "left", 7, 240, 1, 0, 6), "left rail matches the capture");
    CHECK(section(&r, "right", 306, 240, -1, 0, 7), "right rail and shadow match the capture");
    CHECK(section(&r, "bottom", 290, 371, 0, -1, 7), "bottom rail and shadow match the capture");

    /* The chrome alone. Inside the content inset (1-px white top/left, shadow
     * bottom/right, window-chrome.md Sec 4) the body is one uniform fill:
     * no gutter or grow-box ink. The inset shadow is graded against the other
     * gutterless 8.1 window, the Appearance control panel (s8_controls_dialog
     * hscan y=200: shadow x=510, inner line x=511). */
    render_ctx_t bare;
    CHECK(render_ctx_init(&bare, &boot) == 0, "render context");
    render_run(&bare, draw_chrome_only);
    uint32_t body = render_pixel_rgb(&bare, 14, 48);
    int stray = 0;
    for (int y = 48; y < 364; y++)
        for (int x = 14; x < 299; x++)
            stray += render_pixel_rgb(&bare, (uint32_t)x, (uint32_t)y) != body;
    printf("  body pixels differing from the content fill: %d\n", stray);
    CHECK(stray == 0, "no scroll bar or grow box drawn inside the plain body");
    unsigned char *cp = load_capture("s8_controls_dialog.png");
    uint32_t inset = ((uint32_t)cp[3u*(200u*640u+510u)] << 16) |
                     ((uint32_t)cp[3u*(200u*640u+510u)+1] << 8) | cp[3u*(200u*640u+510u)+2];
    int inset_ok = 1;
    for (int y = 60; y < 360; y += 50)
        inset_ok = inset_ok && (render_pixel_rgb(&bare, 299, (uint32_t)y) & 0xFFFFFFu) == inset;
    for (int x = 20; x < 299; x += 50)
        inset_ok = inset_ok && (render_pixel_rgb(&bare, (uint32_t)x, 364) & 0xFFFFFFu) == inset;
    printf("  inset shadow capture %06X\n", (unsigned)inset);
    CHECK(inset_ok, "right and bottom content inset are the captured shadow");

    /* Collapse and expand: the restored content is still the plain body. */
    CHECK(CollapseWindow(&g_wm, &g_w) >= 0 && CollapseWindow(&g_wm, &g_w) >= 0,
          "collapse round trip");
    c = region_get_bbox(g_w.contRgn);
    CHECK(c.left == 13 && c.top == 47 && c.right == 300 && c.bottom == 365,
          "expanded content is the plain body again");
    return TEST_SUMMARY("test-plain-window");
}
