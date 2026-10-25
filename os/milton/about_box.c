/* about_box.c -- About This Computer, section COLD of C:\FLAIR.OVL (ADR-0014
 * DEC-14-10 lane 1; moved verbatim from kmain.c flair_live_about).
 *
 * About This Computer: the shipped banner's name/version (BANNER_LINE1,
 * spec/dos_banner.txt) and the machine's REAL memory -- the RAM stage2
 * probed (1 MiB + ext_mem_kb) and the InitechDOS arena's largest free block,
 * walked live off the MCB chain (int21_mcb_stats). An unbound arena says so
 * ("none"), it never shows a made-up figure. The Mac OS 8 window of the
 * capture (s8_about_this_computer.png) is shown through the shell's alert
 * (the banner art and the per-application bars are a recorded gap).
 * Serial: APPLE-ABOUT mem_k=<n> arena_k=<n> largest_k=<n> ; APPLE-ABOUT up ;
 * APPLE-ABOUT closed (unchanged bytes; the gates key on them). */
#include "about_box.h"
#include "int21.h"
#include "kmain_cold.h"

void flair_about_box(struct flair_live_ctx *ctx, const boot_info_t *bi,
                     const char *banner)
{
    static char l2[48], l3[48];
    static const char *const ok_label[1] = { "OK" };
    const char *text[3];
    uint32_t mem_k = 1024u + bi->ext_mem_kb, arena = 0u, largest = 0u;
    int bound = int21_mcb_stats(&arena, &largest);
    kmain_about_lines(l2,l3,mem_k,largest,bound);
    text[0] = banner;
    text[1] = l2;
    text[2] = l3;
    serial_puts("APPLE-ABOUT mem_k=");
    serial_putu(mem_k);
    serial_puts(" arena_k=");
    serial_putu(arena / 64u);
    serial_puts(" largest_k=");
    serial_putu(largest / 64u);
    serial_puts("\n");
    flair_live_alert_open(ctx, bi, text, 3, ok_label, 1, 1);
    serial_puts("APPLE-ABOUT up\n");
    (void)flair_live_alert_run(ctx, bi);
    serial_puts("APPLE-ABOUT closed\n");
}
