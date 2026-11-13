/* test_tsr_io.c -- host oracle for the two device halves of the TSR contract
 * (bead initech-6fdt; spec/tsr_contract.h Sec 3 and Sec 4):
 *
 *   K  the keyboard flag byte: kbd_translate tracks left/right Alt like Ctrl,
 *      and kbd_bios_flags maps the state to the BIOS bit layout a resident
 *      reads (Ctrl+Alt -> 0x0C, the SideKick chord). Alt changes no character.
 *   P  the console text page: after text in two attributes, a scroll and a
 *      pop-up drawn over it, console_restore_page(console_save_page(...))
 *      gives back the FRAMEBUFFER byte for byte and the cursor -- pixels are
 *      compared with a pixel snapshot, not the shadow with itself (Law 2).
 * Mutants (Rule 6, make test-tsr-io-mutant): KBD_MUT_NO_ALT, CONSOLE_MUT_
 * RESTORE_NO_DRAW. Each must go RED. CLAUDE.md Rule 1, Rule 11, Rule 12. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "kbd.h"
#include "console.h"
#include "tsr_contract.h"

static int g_checks = 0, g_fails = 0;
#define CHECK(cond, msg)                                                    \
    do {                                                                    \
        g_checks++;                                                         \
        if (!(cond)) {                                                      \
            g_fails++;                                                      \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
        }                                                                   \
    } while (0)

/* LFB/font addresses are 32-bit in the artifact: map them below 4 GiB. */
static void *alloc32(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED || (uintptr_t)p > 0xFFFFFFFFu) return NULL;
    return p;
}

static void keyboard(void)
{
    kbd_state_t st;
    kbd_state_init(&st);
    CHECK(kbd_bios_flags(&st) == 0u, "K: no keys -> 0");
    (void)kbd_translate(&st, 0x1Du);                   /* left Ctrl make  */
    CHECK(kbd_bios_flags(&st) == TSR_KBD_FLAG_CTRL, "K: Ctrl -> bit 2");
    CHECK(kbd_translate(&st, 0x38u) == 0u, "K: Alt make is no character");
    CHECK(kbd_bios_flags(&st) == TSR_CHORD_DEFAULT, "K: Ctrl+Alt -> 0x0C");
    (void)kbd_translate(&st, 0xB8u);                   /* left Alt break  */
    CHECK(kbd_bios_flags(&st) == TSR_KBD_FLAG_CTRL, "K: Alt break clears bit 3");
    (void)kbd_translate(&st, 0x9Du);
    (void)kbd_translate(&st, 0xE0u);                   /* right Alt make  */
    (void)kbd_translate(&st, 0x38u);
    CHECK(kbd_bios_flags(&st) == TSR_KBD_FLAG_ALT, "K: right Alt (E0 38) -> bit 3");
    CHECK(kbd_translate(&st, 0x1Eu) == 'a', "K: Alt leaves 'a' a character");
    (void)kbd_translate(&st, 0xE0u);
    (void)kbd_translate(&st, 0xB8u);
    CHECK(kbd_bios_flags(&st) == 0u, "K: right Alt break clears it");
    (void)kbd_translate(&st, 0x2Au);
    (void)kbd_translate(&st, 0x36u);
    (void)kbd_translate(&st, 0x3Au);
    CHECK(kbd_bios_flags(&st) == (TSR_KBD_FLAG_LSHIFT | TSR_KBD_FLAG_RSHIFT |
                                  TSR_KBD_FLAG_CAPS), "K: shifts + caps bits");
}

#define FB_W 640u
#define FB_H 480u

static void page(void)
{
    uint8_t *fb = (uint8_t *)alloc32(FB_W * FB_H * 4u);
    uint8_t *font = (uint8_t *)alloc32(CONSOLE_FONT_BYTES);
    uint8_t *snap = malloc(FB_W * FB_H * 4u);
    uint8_t save[CONSOLE_PAGE_BYTES];
    if (!fb || !font || !snap) { CHECK(0, "P: buffers"); return; }
    for (uint32_t i = 0; i < CONSOLE_FONT_BYTES; i++)   /* every glyph distinct */
        font[i] = (uint8_t)((i / CONSOLE_CELL_H) * 37u + (i % CONSOLE_CELL_H) * 11u);
    memset(font + ' ' * CONSOLE_CELL_H, 0, CONSOLE_CELL_H);   /* blank, as the ROM font */
    boot_info_t bi;
    memset(&bi, 0, sizeof bi);
    bi.lfb_addr = (uint32_t)(uintptr_t)fb;
    bi.lfb_bpp = 32u; bi.lfb_pitch = FB_W * 4u;
    bi.lfb_width = FB_W; bi.lfb_height = FB_H;
    bi.font_addr = (uint32_t)(uintptr_t)font;
    console_t con;
    CHECK(console_init(&con, &bi) == CONSOLE_OK, "P: console_init");
    CHECK(CONSOLE_PAGE_BYTES == TSR_SCREEN_BYTES, "P: page size == the contract");

    char line[48];
    for (int i = 0; i < 30; i++) {                     /* 30 lines: scrolls */
        console_set_attr(&con, (uint8_t)(i & 1 ? 0x1E : 0x07));
        snprintf(line, sizeof line, "LINE %02d C:\\INITECH>DIR\r\n", i);
        console_puts(&con, line);
    }
    console_puts(&con, "A:\\>");
    CHECK(con.cells[0] == 'L' && con.cells[1] == 0x07u && con.cells[2 * 5] == '0' &&
          con.cells[2 * 6] == '6' && con.cells[2 * CONSOLE_COLS + 1] == 0x1Eu,
          "P: the shadow scrolled with the screen (row 0 = LINE 06)");
    memcpy(snap, fb, FB_W * FB_H * 4u);
    uint32_t r0, c0;
    console_get_cursor(&con, &r0, &c0);

    console_save_page(&con, save);
    console_set_attr(&con, 0x4Fu);                     /* the pop-up window */
    console_set_cursor(&con, 8u, 20u);
    console_puts(&con, "+------ INITECH SIDEKICK ------+");
    console_set_cursor(&con, 9u, 20u);
    console_puts(&con, "| Notepad Calculator Calendar |");
    console_erase_line(&con, 0);
    CHECK(memcmp(snap, fb, FB_W * FB_H * 4u) != 0, "P: the pop-up changed pixels");

    console_restore_page(&con, save);
    uint32_t r1, c1;
    console_get_cursor(&con, &r1, &c1);
    CHECK(memcmp(snap, fb, FB_W * FB_H * 4u) == 0,
          "P: restore gives back the framebuffer byte for byte");
    CHECK(r1 == r0 && c1 == c0, "P: restore puts the cursor back");
    console_putc(&con, 'X');                           /* attr state intact */
    CHECK(con.cells[(r0 * CONSOLE_COLS + c0) * 2u + 1u] == 0x4Fu,
          "P: restore keeps the caller's current attribute");
    free(snap);
}

int main(void)
{
    keyboard();
    page();
    printf("test_tsr_io: %d checks, %d failures\n", g_checks, g_fails);
    return g_fails ? 1 : 0;
}
