/* Host acceptance for the kit contract; no emulator or running app required.
 * cc -std=c11 -Wall -Wextra -Werror -DSAMIR_W4 -Ios/samir/include \
 *   os/samir/ui/kit.c harness/diff/dbf_diff/xbase_prog_diff/oneshot/kit/kit_host_test.c \
 *   -o build/kit_host_test && build/kit_host_test
 * Source: specs/full-screen/assist-ui-and-editors.md and
 * specs/full-screen/picture-function-templates.md in dbase3-decomp. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "samir/kit.h"
#include "samir/rt.h"

static samir_pal_t fake;
static char cells[KIT_ROWS][KIT_COLS];
static uint8_t attrs[KIT_ROWS][KIT_COLS], attr, cursor_row, cursor_col;
static int keys[2048], nkeys, atkey, calls, disabled;
static kit screen;
static void (*observe)(void);

void *rt_memcpy(void *d, const void *s, uint32_t n) { return memcpy(d, s, n); }
void *rt_memset(void *d, int c, uint32_t n) { return memset(d, c, n); }
uint32_t rt_strlen(const char *s) { return (uint32_t)strlen(s); }
samir_pal_t *xb_interp_pal(xb_interp *ip) { assert(ip); return &fake; }
static int paint(samir_pal_t *p, uint8_t row, uint8_t col, const char *s, uint32_t n)
{
    uint32_t i;
    assert(p == &fake && row < KIT_ROWS && col < KIT_COLS);
    assert(n <= (uint32_t)(KIT_COLS - col));
    for (i = 0; i < n; i++) {
        assert((unsigned char)s[i] >= 32);
        cells[row][col + i] = s[i]; attrs[row][col + i] = attr;
    }
    calls++;
    return 0;
}
static void set_attr(samir_pal_t *p, uint8_t a) { assert(p == &fake); attr = a; }
static void gotoxy(samir_pal_t *p, uint8_t row, uint8_t col)
{
    assert(p == &fake && row < KIT_ROWS && col < KIT_COLS);
    cursor_row = row; cursor_col = col;
}
static int32_t conin_char(samir_pal_t *p)
{
    assert(p == &fake);
    if (observe) observe();
    return atkey < nkeys ? keys[atkey++] : -1;
}
static void status(samir_pal_t *p, int on, const char *file, uint32_t r, uint32_t n)
{ assert(p == &fake && on == 0 && file && r == 0 && n == 0); disabled++; }
static int curdrive(samir_pal_t *p) { assert(p == &fake); return 'B'; }
static void sequence(const int *s, int n)
{
    assert(n <= (int)(sizeof(keys) / sizeof(keys[0])));
    memcpy(keys, s, (size_t)n * sizeof(keys[0])); nkeys = n; atkey = 0;
}
#define SEQ(...) do { const int seq_[] = { __VA_ARGS__ }; \
    sequence(seq_, (int)(sizeof(seq_) / sizeof(seq_[0]))); } while (0)
static void matches(void)
{
    assert(!memcmp(cells, screen.ch, sizeof(cells)));
    assert(!memcmp(attrs, screen.at, sizeof(attrs)));
    assert(cursor_row == screen.cur_row && cursor_col == screen.cur_col);
}
static void drawing(void)
{
    kit_saved saved;
    int r, c, before;
    kit_open(&screen, (xb_interp *)(void *)&screen);
    assert(disabled == 1);
    for (r = 0; r < KIT_ROWS; r++) for (c = 0; c < KIT_COLS; c++) {
        assert(cells[r][c] == ' ' && attrs[r][c] == KIT_ATTR_NORMAL);
    }
    kit_put(&screen, 2, -2, "ABCDE", -1);
    assert(!memcmp(cells[2], "CDE", 3));
    kit_put_attr(&screen, 24, 79, KIT_ATTR_REVERSE, "AB", -1);
    assert(cells[24][79] == 'A' && attrs[24][79] == KIT_ATTR_REVERSE);
    before = calls;
    kit_put(&screen, -1, 0, "bad", -1); kit_put(&screen, 25, 0, "bad", -1);
    kit_fill(&screen, 0, -5, 3, '?', KIT_ATTR_NORMAL);
    kit_fill(&screen, 25, 0, 3, '?', KIT_ATTR_NORMAL);
    assert(calls == before);
    kit_fill(&screen, 3, -2, 5, '#', KIT_ATTR_BRIGHT);
    assert(!memcmp(cells[3], "###", 3));
    kit_cursor(&screen, 99, -5); assert(cursor_row == 24 && cursor_col == 0);
    matches();
    kit_save(&screen, &saved);
    kit_clear(&screen); kit_restore(&screen, &saved); matches();
    assert(!memcmp(screen.ch, saved.ch, sizeof(saved.ch)));
    kit_box(&screen, 4, 2, 8, 12, 0);
    assert((unsigned char)cells[4][2] == 218 && (unsigned char)cells[8][12] == 217);
    kit_box(&screen, -2, -2, 2, 2, 1);
    assert((unsigned char)cells[2][2] == 188);
    kit_message(&screen, 24, "Hello"); assert(!memcmp(cells[24] + 37, "Hello", 5));
    kit_status(&screen, "BROWSE", "A:\\TEST\\clients.dbf", 1, 3, 1);
    assert(!memcmp(cells[22] + 1, "BROWSE", 6));
    assert(!memcmp(cells[22] + 17, "<A:>", 4));
    assert(!memcmp(cells[22] + 23, "CLIENTS", 7));
    assert(!memcmp(cells[22] + 46, "Rec: 1/3", 8));
    assert(!memcmp(cells[22] + 67, "Ins", 3));
    for (c = 0; c < KIT_COLS; c++) assert(attrs[22][c] == KIT_ATTR_REVERSE);
    kit_clear_rows(&screen, -1, 1); kit_flush_row(&screen, 24); matches();
    kit_close(&screen); assert(cursor_row == 0 && cursor_col == 0 && attr == KIT_ATTR_NORMAL);
    matches();
}
static void decoding(void)
{
    static const int scans[] = {0x48,0x50,0x4B,0x4D,0x49,0x51,0x47,0x4F,0x52,0x53,0x3B,0x44,0x75,0x77};
    static const int wanted[] = {KIT_K_UP,KIT_K_DOWN,KIT_K_LEFT,KIT_K_RIGHT,KIT_K_PGUP,KIT_K_PGDN,
        KIT_K_HOME,KIT_K_END,KIT_K_INS,KIT_K_DEL,KIT_K_F1,KIT_K_F10,KIT_K_SAVE,KIT_K_CTRL_HOME};
    static const int controls[] = {5,24,19,4,18,3,23,17,21,14,22,7,25};
    static const int control_keys[] = {KIT_K_UP,KIT_K_DOWN,KIT_K_LEFT,KIT_K_RIGHT,KIT_K_PGUP,KIT_K_PGDN,
        KIT_K_SAVE,KIT_K_ABORT,KIT_K_TOGGLE_DEL,KIT_K_CTRL_N,KIT_K_INS,KIT_K_DEL,KIT_K_CTRL_Y};
    unsigned i;
    for (i = 0; i < sizeof(scans) / sizeof(scans[0]); i++) {
        SEQ(0, scans[i]); assert(kit_key(&screen) == wanted[i]);
    }
    for (i = 0; i < sizeof(controls) / sizeof(controls[0]); i++) {
        SEQ(controls[i]); assert(kit_key(&screen) == control_keys[i]);
    }
    SEQ(0, -1); assert(kit_key(&screen) == KIT_K_EOF);
    SEQ(0, 0x7F); assert(kit_key(&screen) == KIT_K_NONE);
    SEQ('a', 13, 27, 8, 9);
    assert(kit_key(&screen) == 'a'); assert(kit_key(&screen) == KIT_K_ENTER);
    assert(kit_key(&screen) == KIT_K_ESC); assert(kit_key(&screen) == KIT_K_BKSP);
    assert(kit_key(&screen) == KIT_K_TAB); assert(kit_key(&screen) == KIT_K_EOF);
}
static void fields(void)
{
    char b[256], prompt[17]; int changed, rc;
    memset(b, ' ', sizeof(b)); SEQ('a','b','c');
    rc = kit_edit_field(&screen, 2, 3, b, 3, 'C', 0, &changed);
    assert(rc == KIT_K_ENTER && changed && !memcmp(b, "abc", 3));
    memcpy(b, "abcd", 4); SEQ(0,0x4D,0,0x53,13);
    rc = kit_edit_field(&screen, 2, 3, b, 4, 'C', 0, &changed);
    assert(rc == KIT_K_ENTER && changed && !memcmp(b, "acd ", 4));
    memcpy(b, "abcd", 4); SEQ(0,0x4D,22,'Z',13);
    kit_edit_field(&screen, 2, 3, b, 4, 'C', 0, &changed); assert(!memcmp(b, "aZbc", 4));
    memcpy(b, "abcd", 4); SEQ(0,0x4D,8,13);
    kit_edit_field(&screen, 2, 3, b, 4, 'C', 0, &changed); assert(!memcmp(b, "bcd ", 4));
    memcpy(b, "abcd", 4); SEQ(25,13);
    kit_edit_field(&screen, 2, 3, b, 4, 'C', 0, &changed); assert(!memcmp(b, "    ", 4));
    memcpy(b, "abcd", 4); SEQ(27);
    rc = kit_edit_field(&screen, 2, 3, b, 4, 'C', 0, &changed);
    assert(rc == KIT_K_ESC && !changed && !memcmp(b, "abcd", 4));
    memset(b, ' ', 4); SEQ('a','b',13);
    kit_edit_field(&screen, 2, 3, b, 4, 'C', "@!", &changed); assert(!memcmp(b, "AB  ", 4));
    memset(b, ' ', 4); SEQ('1','a','2','b','3','4');
    kit_edit_field(&screen, 2, 3, b, 4, 'C', "AA99", &changed); assert(!memcmp(b, "ab34", 4));
    memset(b, ' ', 8); SEQ('1','2','3','1');
    kit_edit_field(&screen, 2, 3, b, 8, 'C', "99/99/85", &changed); assert(!memcmp(b, "12/31/85", 8));
    memset(b, ' ', 8); SEQ('x','y');
    kit_edit_field(&screen, 2, 3, b, 1, 'L', 0, &changed); assert(b[0] == 'T');
    memset(b, ' ', 8); SEQ('f');
    kit_edit_field(&screen, 2, 3, b, 1, 'L', "Y", &changed); assert(b[0] == 'N');
    memset(b, ' ', 8); SEQ('a','-','1','2','.','5',13);
    kit_edit_field(&screen, 2, 3, b, 8, 'N', 0, &changed); assert(!memcmp(b, "-12.5   ", 8));
    memcpy(b, "12345.67", 8); SEQ('2','5','.','5',13);
    kit_edit_field(&screen, 2, 3, b, 8, 'N', 0, &changed); assert(!memcmp(b, "25.5    ", 8));
    memset(b, ' ', 6); SEQ('1','2','.','3','4');
    kit_edit_field(&screen, 2, 3, b, 6, 'N', "999.99", &changed); assert(!memcmp(b, " 12.34", 6));
    memset(b, ' ', 6); SEQ('-','1','.','5',13);
    kit_edit_field(&screen, 2, 3, b, 6, 'N', "999.99", &changed); assert(!memcmp(b, " -1.5 ", 6));
    memset(b, ' ', 6); SEQ('1','2',13);
    kit_edit_field(&screen, 2, 3, b, 6, 'N', "999.99", &changed); assert(!memcmp(b, " 12.  ", 6));
    memset(b, ' ', 8); SEQ('x','1','2','/','3','1','/','8','6');
    kit_edit_field(&screen, 2, 3, b, 8, 'D', 0, &changed); assert(!memcmp(b, "12/31/86", 8));
    memcpy(b, "  /  /  ", 8); SEQ('1','2','3','1','8','6');
    kit_edit_field(&screen, 2, 3, b, 8, 'D', 0, &changed); assert(!memcmp(b, "12/31/86", 8));
    memcpy(b, "    -  -  ", 10); SEQ('1','9','8','6','0','7','0','4');
    kit_edit_field(&screen, 2, 3, b, 10, 'D', 0, &changed); assert(!memcmp(b, "1986-07-04", 10));
    memset(b, ' ', 4); SEQ('1','a','b','C','d');
    kit_edit_field(&screen, 2, 3, b, 4, 'C', "@!A", &changed); assert(!memcmp(b, "ABCD", 4));
    memset(b, ' ', 4); SEQ('@','a','1','-','2','b');
    kit_edit_field(&screen, 2, 3, b, 4, 'C', "NNNN", &changed); assert(!memcmp(b, "a12b", 4));
    memset(b, ' ', 4); SEQ('a','1',' ','-','2');
    kit_edit_field(&screen, 2, 3, b, 4, 'C', "####", &changed); assert(!memcmp(b, "1 -2", 4));
    memset(b, ' ', 10); SEQ('8','0','0','8','6','7','5','3','0','9');
    kit_edit_field(&screen, 2, 3, b, 10, 'C', "@R (999) 999-9999", &changed);
    assert(!memcmp(b, "8008675309", 10));
    assert(!memcmp(cells[2] + 3, "(800) 867-5309", 14));
    memset(b, ' ', 6); kit_fill(&screen, 2, 3, 20, '.', KIT_ATTR_NORMAL);
    SEQ('a','b','c','d','e','f');
    kit_edit_field(&screen, 2, 3, b, 6, 'C', "@!S3", &changed);
    assert(!memcmp(b, "ABCDEF", 6) && !memcmp(cells[2] + 3, "DEF", 3));
    assert(cells[2][6] == '.');
    memset(b, ' ', sizeof(b));
    { int s[255], i; for (i = 0; i < 255; i++) s[i] = 'a' + i % 26; sequence(s, 255); }
    assert(kit_edit_field(&screen, 2, 72, b, 255, 'C', 0, &changed) == KIT_K_ENTER);
    assert(b[0] == 'a' && b[254] == 'u'); matches();
    prompt[0] = 0; SEQ('b','o','b',13);
    assert(kit_prompt(&screen, 24, "Name:", prompt, 16) == 1 && !strcmp(prompt, "bob"));
    SEQ(27); assert(kit_prompt(&screen, 24, "Name:", prompt, 16) == 0);
    prompt[0] = 0; SEQ(0,0x48,'j','o',13);
    assert(kit_prompt(&screen, 24, "Name:", prompt, 16) == 1 && !strcmp(prompt, "jo"));
}
static void menu_observer(void)
{
    assert(attrs[0][1] == KIT_ATTR_REVERSE || attrs[0][7] == KIT_ATTR_REVERSE);
    assert(screen.ch[23][0] == ' '); matches();
}
static void menus(void)
{
    const char *titles[] = {"File", "Edit"};
    const char *items[] = {"Open\0Save\0Quit\0", "Copy\0Paste\0"};
    const char *help[] = {"File commands", "Edit commands"};
    const char *pick[] = {"Alpha", "Beta", "Gamma"};
    kit_saved saved;
    kit_status(&screen, "ASSIST", "CLIENTS", 1, 3, 0); kit_save(&screen, &saved);
    observe = menu_observer;
    SEQ(0,0x4D,0,0x50,13); assert(kit_menu(&screen, titles, items, 2, 0, help) == 0x101);
    observe = 0;
    assert(!memcmp(screen.ch, saved.ch, sizeof(saved.ch))); matches();
    SEQ(27); assert(kit_menu(&screen, titles, items, 2, 0, help) == -1);
    SEQ('q',13); assert(kit_menu(&screen, titles, items, 2, 0, help) == 2);
    SEQ(0,0x50,13); assert(kit_pick(&screen, 3, 4, pick, 3, 0) == 1);
    SEQ(27); assert(kit_pick(&screen, 3, 4, pick, 3, 0) == -1);
    SEQ('g',13); assert(kit_pick(&screen, 19, 76, pick, 3, 0) == 2);
    assert(!memcmp(screen.ch, saved.ch, sizeof(saved.ch))); matches();
}
static void scrolling(void)
{
    char strings[31][16], list[512];
    const char *pick[31], *titles[] = {"Many"}, *menus_[1];
    kit_saved saved;
    int i, n = 0;
    for (i = 0; i < 31; i++) {
        snprintf(strings[i], sizeof(strings[i]), "Item %02d", i);
        pick[i] = strings[i];
        memcpy(list + n, strings[i], strlen(strings[i]) + 1);
        n += (int)strlen(strings[i]) + 1;
    }
    list[n] = 0; menus_[0] = list;
    kit_save(&screen, &saved);
    SEQ(0,0x4F,13); assert(kit_menu(&screen, titles, menus_, 1, 0, 0) == 30);
    SEQ(0,0x49,13); assert(kit_menu(&screen, titles, menus_, 1, 30, 0) == 0);
    SEQ(0,0x4F,13); assert(kit_pick(&screen, 2, 3, pick, 31, 0) == 30);
    SEQ(0,0x49,13); assert(kit_pick(&screen, 19, 3, pick, 31, 30) == 29);
    assert(!memcmp(screen.ch, saved.ch, sizeof(saved.ch))); matches();
}
static void exercise_bounds(void)
{
    uint32_t state = 12345;
    int t, i, changed;
    char buf[256];
    const char *pics[] = {0, "@!", "99/99/85", "ANX!9#", "@A AAAA"};
    for (t = 0; t < 200; t++) {
        int seq[160], count = 0, width = 1 + t % 254;
        for (i = 0; i < 60; i++) {
            state = state * 1664525u + 1013904223u;
            if ((state & 7u) == 0) { seq[count++] = 0; seq[count++] = 0x47 + (int)(state % 14); }
            else if ((state & 7u) == 1) seq[count++] = (int)(state % 26) + 1;
            else seq[count++] = (int)(state % 95) + 32;
        }
        memset(buf, ' ', sizeof(buf)); sequence(seq, count);
        (void)kit_edit_field(&screen, t % 35 - 5, t % 100 - 10, buf, width,
                             "CNDL"[t % 4], pics[t % 5], &changed);
        matches();
    }
}
int main(void)
{
    fake.paint = paint; fake.set_attr = set_attr; fake.gotoxy = gotoxy;
    fake.conin_char = conin_char; fake.status = status; fake.curdrive = curdrive;
    drawing(); decoding(); fields(); menus(); scrolling(); exercise_bounds();
    puts("PASS: kit rendering, key decoding, fields, picture literals, menus, prompts, restore");
    return 0;
}
