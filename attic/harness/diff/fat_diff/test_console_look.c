#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
int main(void)
{
    uint8_t *fb = mmap(NULL, 640*480*4, PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
    uint8_t *font = mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
    boot_info_t bi;
    console_t con;
    if (fb == MAP_FAILED || font == MAP_FAILED) return 2;
    memset(&bi,0,sizeof bi);
    bi.lfb_addr=(uint32_t)(uintptr_t)fb; bi.font_addr=(uint32_t)(uintptr_t)font;
    bi.lfb_width=640; bi.lfb_height=480; bi.lfb_pitch=640*4; bi.lfb_bpp=32;
    if (console_init(&con,&bi)) return 2;
    console_set_cursor(&con,24,17);
    console_putc(&con,'\f');
    if (con.cur_row || con.cur_col) {
        fprintf(stderr,"CONSOLE LOOK RED: CLS cursor=(%u,%u)\n",con.cur_row,con.cur_col);
        return 1;
    }
    /* Independent phase points: 16/60 s = 26 2/3 PIT ticks, so the
     * transitions are at 27,54,80. Restore exact pixels under a glyph. */
    const unsigned rates[] = {8,24,32};
    for (unsigned b = 0; b < 3; b++) {
        bi.lfb_bpp=rates[b]; bi.lfb_pitch=640*(rates[b]/8);
        if (console_init(&con,&bi)) return 2;
        console_set_cursor(&con,3,4);
        size_t len=bi.lfb_pitch*480u;
        unsigned char *saved=malloc(len);
        if (!saved) return 2;
        /* Distinct pixels in the underline area test restoration, not just
         * filling background (the cursor may cover text with ANSI colors). */
        memset(fb+62u*bi.lfb_pitch+32u*(rates[b]/8),0x43,8*(rates[b]/8));
        memcpy(saved,fb,len);
        console_cursor_wait(&con,0,1);
        if (!con.cursor_on) { fprintf(stderr,"CURSOR RED: no on phase\n"); return 1; }
        for (unsigned y=0;y<480;y++) for (unsigned x=0;x<640;x++) {
            size_t off=y*bi.lfb_pitch+x*(rates[b]/8);
            if (!(x>=32 && x<40 && y>=62 && y<64) &&
                memcmp(fb+off,saved+off,rates[b]/8)) return 1;
        }
        console_cursor_wait(&con,27,1);
        if (con.cursor_on || memcmp(saved,fb,len)) return 1;
        console_cursor_wait(&con,54,1);
        if (!con.cursor_on) return 1;
        console_cursor_wait(&con,80,1);
        if (con.cursor_on || memcmp(saved,fb,len)) return 1;
        console_cursor_wait(&con,160,1);
        console_cursor_wait(&con,160,0);
        if (con.cursor_on || memcmp(saved,fb,len)) return 1;
        free(saved);
    }
    puts("CONSOLE LOOK PASS: CLS home; PIT phases 0/27/54/80; exact pixel restore at 8/24/32 bpp");
    return 0;
}
