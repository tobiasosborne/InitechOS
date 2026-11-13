/* spec/ssim_params.h -- LOCKED factory SSIM guide data, initech-w06.
 * Deliberate Rule-8 replacement of the pre-FLAIR placeholders; see WL-0149.
 * Ref: PRD Sec 3/6.3/8; ADR-0010 CD-7, HER-11. GUIDE ONLY, no thresholds.
 * Independent goldens: ../system7-decomp/goldens/captures, NEVER the film
 * mock-up or our own renderer. No resizing, gamma matching, or alignment fit.
 * Math source: docs/research/ssim-wang2004.md, Wang 2004 pp.604-606.
 * Rectangles are (x,y,width,height), half-open, native pixels in BOTH images.
 * Rows isolate corresponding parts, not unrelated whole-screen arrangements.
 * Captures have different titles, content, and gamma/palette conventions;
 * their effect is deliberately visible in the score (Law 4).
 */
#ifndef INITECH_SPEC_SSIM_PARAMS_H
#define INITECH_SPEC_SSIM_PARAMS_H
#include <stdint.h>
#include <stddef.h>
#define FLAIR_SSIM_WINDOW_SIZE 11u
#define FLAIR_SSIM_SIGMA_TENTHS 15u
#define FLAIR_SSIM_DATA_RANGE 255u
#define FLAIR_SSIM_K1 0.01
#define FLAIR_SSIM_K2 0.03

typedef struct flair_ssim_rect {
    uint16_t x, y, w, h;
} flair_ssim_rect_t;
typedef struct flair_ssim_crop {
    const char *frame, *name, *dump, *golden, *gate;
    flair_ssim_rect_t native, fixture;
} flair_ssim_crop_t;

/* Ground truth for every crop:
 * desktop: tools/ppm_flair_check.c W0/W1 geometry + test-flair-desktop dump;
 *   sys8/window-chrome.md Sec 1/2 (65,70 active; 13,29 inactive), menus.md
 *   Sec 1. The back document is INACTIVE; close box is on the ACTIVE front.
 * grow-box: spec/flair_grow_box_trace.mk + ppm_flair_grow_box_check.c;
 *   sys8/window-chrome.md Sec 5: (461,296) 18x18. Native (420,320).
 * scroll-look: spec/flair_scroll_look_traces.mk + scrollbar_look_golden.h;
 *   sys8/scrollbars.md Sec 2: active Applications enabled H thumb (330,401).
 * alert: harness/render/alert_capture.h + ppm_flair_trash_check.c;
 *   sys8/controls.md Sec 5/6: frame (133,87)..(507,191), OK/default ring.
 * finder: grown APPS frame (40,80), list-view golden frame (8,25), measured
 *   at x=200: y25 black, y26 white, y29..40 stripe, y46 black bottom line;
 *   s8_finder_list_view.png title chrome only. InitechOS shows an ICON view;
 *   list content has no corresponding current dump, so it is not scored.
 * icons: os/flair/finder_desktop.h and spec/flair_desktop_icons_traces.mk;
 *   corpus re/mint-results-008.md Sec 2: HD(576,28), trash(576,412),
 *   32x32 slots. HD art is supplied by the Basilisk driver, not Apple.
 * controls: dialog face sampled in s8_controls_dialog.png (128,200);
 *   native alert face (175,160), per sys8/controls.md Sec 1 and 6.
 * These last face-only patches measure colour/luminance, not control shape.
 */
static const flair_ssim_crop_t flair_ssim_crops[] = {
    {"desktop","MENU_BAR","flair_desktop.ppm","s8_desktop_menubar.png","test-flair-desktop",
     {0,0,640,20},{0,0,640,20}},
    {"desktop","DOCUMENT_TITLE","flair_desktop.ppm","s8_doc_window_inactive.png","test-flair-desktop",
     {60,80,260,22},{13,29,260,22}},
    {"desktop","FRONT_TITLE","flair_desktop.ppm","s8_doc_window_active.png","test-flair-desktop",
     {300,120,260,22},{65,70,260,22}},
    {"desktop","CLOSE_BOX","flair_desktop.ppm","s8_doc_window_active.png","test-flair-desktop",
     {304,124,13,13},{69,74,13,13}},
    {"grow-box","GROW_BOX","flair_grow_box.ppm","s8_doc_window_active.png","test-flair-grow-box",
     {420,320,18,18},{461,296,18,18}},
    {"scroll-look","H_THUMB","scroll_look_shrink.ppm","s8_doc_window_inactive.png","test-flair-scroll-look",
     {41,160,15,14},{330,401,15,14}},
    {"alert","ALERT_FRAME_TOP","flair_alert_chrome.ppm","s8_alert_modal.png","test-flair-alert-chrome",
     {133,87,374,16},{133,87,374,16}},
    {"alert","DEFAULT_BUTTON","flair_alert_chrome.ppm","s8_alert_modal.png","test-flair-alert-chrome",
     {432,155,65,26},{432,155,65,26}},
    {"finder","LIST_VIEW_TITLE_CHROME","flair_grow_box.ppm","s8_finder_list_view.png","test-flair-grow-box",
     {40,80,200,22},{8,25,200,22}},
    {"icons","VOLUME","flair_grow_box.ppm","s8_icons_on_desktop.png","test-flair-grow-box",
     {584,48,32,32},{576,28,32,32}},
    {"icons","TRASH","flair_grow_box.ppm","s8_icons_on_desktop.png","test-flair-grow-box",
     {584,404,32,32},{576,412,32,32}},
    {"controls","DIALOG_FACE","flair_alert_chrome.ppm","s8_controls_dialog.png","test-flair-alert-chrome",
     {175,160,16,16},{128,200,16,16}}
};
#define FLAIR_SSIM_CROP_COUNT (sizeof flair_ssim_crops / sizeof flair_ssim_crops[0])
_Static_assert(FLAIR_SSIM_WINDOW_SIZE == 11u, "Wang 2004 11x11 kernel");
_Static_assert(FLAIR_SSIM_SIGMA_TENTHS == 15u, "Wang 2004 sigma1.5");
_Static_assert(FLAIR_SSIM_DATA_RANGE == 255u, "8-bit samples");
#endif
