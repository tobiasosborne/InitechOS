/* about_box.h -- Apple > About This Computer, OVERLAID (ADR-0014 DEC-14-10:
 * "The About box joins the lane if the measured net is not negative"; epic
 * initech-x8gsi lane 1, measured -396 B resident). The box lives in section
 * COLD of C:\FLAIR.OVL with the JTC mat; the resident shell reaches it only
 * through the generated stub flair_about_box (DEC-14-6.1). The kernel services
 * it imports are declared here (DEC-14-6.3: overlaid code calls the kernel
 * directly through checked imports). Ref: PRD 6.3/6.5; ADR-0003 DEC-12 +
 * Appendix D.1 (the banner line). ASCII only (Rule 12). */
#ifndef INITECH_ABOUT_BOX_H
#define INITECH_ABOUT_BOX_H
#include <stdint.h>
#include "boot_info.h"

struct flair_live_ctx;   /* kmain.c's live-desktop context, opaque here */

/* Overlaid entry (stub). banner = ADR-0003 Appendix D.1 line 1, passed by the
 * shell so the locked literal has exactly one home (kmain.c BANNER_LINE1). */
void flair_about_box(struct flair_live_ctx *ctx, const boot_info_t *bi,
                     const char *banner);

/* Resident kmain.c services the overlaid box imports. */
void flair_live_alert_open(struct flair_live_ctx *ctx, const boot_info_t *bi,
                           const char *const *text, int n,
                           const char *const *btn, int nb, int draw);
uint16_t flair_live_alert_run(struct flair_live_ctx *ctx, const boot_info_t *bi);
void serial_puts(const char *s);
void serial_putu(uint32_t v);
#endif
