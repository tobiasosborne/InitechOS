/* About This Computer; reference: legacy-final About dialog and PRD 6.3/6.5.
 * Ordinary desktop entry, using public DOS memory accounting. */
#ifndef INITECH_ABOUT_BOX_H
#define INITECH_ABOUT_BOX_H
#include <stdint.h>
#include "boot_info.h"

struct flair_live_ctx;   /* kmain.c's live-desktop context, opaque here */

/* Desktop entry. banner = ADR-0003 Appendix D.1 line 1, passed by the
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
