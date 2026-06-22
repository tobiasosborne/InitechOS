/*
 * test_flair_mechanism_colorblind.c -- the C-8 SENTINEL-RENDER boundary proof
 *                                       (THE ORACLE; the behavioral tooth).
 *
 * beads: initech-6bq2 (C-8 MECHANISM/POLICY enforcement); epic initech-qipc.
 * Ref:   ADR-0004-AMENDMENT-DEC-09 Sec 3.10 (DEC-09-D4 #2); CLAUDE.md Law 2
 *        (the oracle is the truth), Law 4 (the live arrangement), Rule 6
 *        (mutation-proven), Rule 11 (deterministic), Rule 12 (ASCII-clean).
 *
 * WHAT IT PROVES (behavior, NOT source): it compiles the REAL artifact
 * decoration drawer (os/flair/chrome.c) against a STUB policy
 * (flair_look_sentinel_stub.c) where the ONE policy seam (flair_look_pixel)
 * returns a SENTINEL (magenta 0x00FF00FF) for EVERY PART.  It renders a
 * System-7 window via the host render skeleton (32bpp) onto a canvas pre-filled
 * with the desktop background, then asserts that, inside the window's bounding
 * box, EVERY pixel is either:
 *   - the untouched background (the canvas the skeleton filled), OR
 *   - the sentinel (a pixel the decoration drew through the seam).
 * If ANY pixel is a THIRD color, a mechanism/decoration module hard-coded (or
 * computed) a color and wrote it WITHOUT going through the seam -> RED.  This
 * is the behavioral teeth the source grep (test-mech-policy) cannot give: it
 * catches an obfuscated or computed color literal the scanner misses.
 *
 * It is NOT valid-by-construction (HER-02 boundary): the grader reads the
 * rendered framebuffer; the artifact's color comes from the stub seam, not the
 * grader -- a hard-coded color in the artifact appears as a third color the
 * grader did not put there.
 *
 * MUTATION (Rule 6): built with -DFLAIR_COLORBLIND_MUTANT, chrome.c draws one
 * span with a COMPUTED non-sentinel color straight to the surface, bypassing
 * the seam (the obfuscated-literal case the source scanner cannot see).  This
 * oracle MUST then go RED.  Default builds never define it.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* the host render skeleton (-Iharness/render) */
#include "chrome.h"             /* flair_draw_document_window (-Ios/flair)     */
#include "test_assert.h"        /* TEST_HARNESS / CHECK / TEST_SUMMARY (-Iseed) */

TEST_HARNESS();

/* The sentinel the stub seam returns for EVERY PART (must match the stub). */
#define SENTINEL_RGB  0x00FF00FFu

/* The window we render -- a generous documentProc window away from the edges. */
enum {
    WIN_LEFT   = 40,
    WIN_TOP    = 30,
    WIN_RIGHT  = 360,
    WIN_BOTTOM = 300
};

static rgn_rect_t win_frame(void)
{
    rgn_rect_t r;
    r.top = WIN_TOP; r.left = WIN_LEFT; r.bottom = WIN_BOTTOM; r.right = WIN_RIGHT;
    return r;
}

static void draw_window(GrafPort *port)
{
    flair_draw_document_window(port, win_frame());
}

int main(void)
{
    printf("test-flair-mechanism-colorblind: C-8 sentinel-render boundary proof "
           "(every chrome pixel == sentinel)\n");

    /* 32bpp host context (the render skeleton fills the canvas with the desktop
     * background RGB; the sentinel stub paints sentinel for every PART). */
    render_boot_info_t boot;
    boot.lfb_addr   = 0;
    boot.lfb_pitch  = 0;          /* tight: width * bpp/8 */
    boot.lfb_bpp    = 32;
    boot.lfb_width  = 400;
    boot.lfb_height = 320;

    render_ctx_t ctx;
    if (render_ctx_init(&ctx, &boot) != 0) {
        CHECK(0, "render_ctx_init failed (geometry/alloc)");
        return TEST_SUMMARY("test-flair-mechanism-colorblind");
    }

    /* The background the skeleton filled (desktop RGB) -- the "untouched" color.
     * Sample a corner well outside the window before drawing. */
    uint32_t bg = render_pixel_rgb(&ctx, 5, 5) & 0x00FFFFFFu;

    render_run(&ctx, draw_window);

    /* Scan the window's bounding box (plus a 1 px margin).  Every pixel must be
     * the sentinel (drawn via the seam) OR the untouched background.  A third
     * color == a hard-coded/computed color that bypassed the seam -> RED. */
    uint32_t sentinel = SENTINEL_RGB & 0x00FFFFFFu;
    long sentinel_pixels = 0;
    long third_color_pixels = 0;
    uint32_t first_bad = 0;
    int first_bad_x = -1, first_bad_y = -1;

    int y0 = WIN_TOP - 1, y1 = WIN_BOTTOM + 1;
    int x0 = WIN_LEFT - 1, x1 = WIN_RIGHT + 1;
    for (int y = y0; y < y1; y++) {
        if (y < 0 || (uint32_t)y >= ctx.fb.bm.height) {
            continue;
        }
        for (int x = x0; x < x1; x++) {
            if (x < 0 || (uint32_t)x >= ctx.fb.bm.width) {
                continue;
            }
            uint32_t px = render_pixel_rgb(&ctx, (uint32_t)x, (uint32_t)y) & 0x00FFFFFFu;
            if (px == sentinel) {
                sentinel_pixels++;
            } else if (px != bg) {
                third_color_pixels++;
                if (first_bad_x < 0) {
                    first_bad = px;
                    first_bad_x = x;
                    first_bad_y = y;
                }
            }
        }
    }

    char msg[256];

    /* (1) The decoration MUST have painted -- at least some sentinel pixels --
     * so the proof is non-vacuous (a no-op draw would trivially "pass"). */
    snprintf(msg, sizeof msg,
             "decoration drew NO sentinel pixels (vacuous render -- %ld sentinel)",
             sentinel_pixels);
    CHECK(sentinel_pixels > 0, msg);

    /* (2) The boundary proof: ZERO third-color pixels.  Any non-sentinel,
     * non-background pixel == a color that bypassed the C-8 seam. */
    snprintf(msg, sizeof msg,
             "EVERY chrome pixel must be the sentinel: found %ld third-color "
             "pixel(s); first 0x%06X @ (%d,%d) (bg=0x%06X sentinel=0x%06X)",
             third_color_pixels, first_bad, first_bad_x, first_bad_y,
             bg, sentinel);
    CHECK(third_color_pixels == 0, msg);

    printf("  sentinel pixels = %ld, third-color pixels = %ld "
           "(bg=0x%06X sentinel=0x%06X)\n",
           sentinel_pixels, third_color_pixels, bg, sentinel);

    render_ctx_free(&ctx);
    return TEST_SUMMARY("test-flair-mechanism-colorblind");
}
