/* Kit integration through the real engine + BROWSE, using the host PAL for
 * file/allocator behavior and a recording text console for scripted keys.
 * Run with run_engine_host.sh; fixtures/results remain under build/kit_engine.
 * Ref: dbase3-decomp/specs/full-screen/assist-ui-and-editors.md section 5. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "samir/kit.h"
#include "samir/interp.h"

struct pal_host_cfg { uint8_t date_yy, date_mm, date_dd; uint32_t heap_size; };
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void pal_host_free(samir_pal_t *p);
extern int samir_repl(samir_pal_t *pal, xb_interp *ip);
static char cells[25][80], output[8192];
static uint8_t attrs[25][80], attr, row, col;
static size_t output_used;
static int atkey, atline;
static const int keys[] = {
    0,0x44, /* F10: open the BROWSE menu. */
    0,0x4D,0,0x4D,13, /* Record No. pad. */
    '2',13, /* Prompt: record 2. */
    0,0x4D, /* CITY. */
    'b','e','r','l','i','n',13,
    '2','5','.','5',0,0x75 /* Numeric first-key replacement + Ctrl-End. */
};
static void dump(const char *name)
{
    FILE *f = fopen(name, "wb"); int r;
    assert(f);
    for (r = 0; r < 25; r++) { assert(fwrite(cells[r], 1, 80, f) == 80); fputc('\n', f); }
    assert(!fclose(f));
}
static int32_t input_char(samir_pal_t *p)
{
    (void)p;
    if (atkey == 0) {
        assert(!memcmp(cells[22] + 1, "BROWSE", 6));
        assert(attrs[22][0] == KIT_ATTR_REVERSE); dump("kit-browse.txt");
    }
    if (atkey == 2) {
        assert(attrs[0][1] == KIT_ATTR_REVERSE);
        assert((unsigned char)cells[1][0] == 218); dump("kit-menu.txt");
    }
    if (atkey == 8) { assert(cells[24][13] == '2'); dump("kit-prompt.txt"); }
    if (atkey == 17) dump("kit-field.txt");
    if (atkey == 22) dump("kit-numeric.txt");
    return atkey < (int)(sizeof(keys) / sizeof(keys[0])) ? keys[atkey++] : -1;
}
static int32_t input_line(samir_pal_t *p, char *buf, uint32_t cap)
{
    static const char *const lines[] = { "USE CLIENTS", "BROWSE", "LIST", "QUIT" };
    size_t n;
    (void)p;
    if (atline >= (int)(sizeof(lines) / sizeof(lines[0]))) return -1;
    n = strlen(lines[atline]); assert(n < cap);
    memcpy(buf, lines[atline++], n + 1); return (int32_t)n;
}
static void console(samir_pal_t *p, const char *s, uint32_t n)
{
    (void)p;
    assert(output_used + n < sizeof(output));
    memcpy(output + output_used, s, n); output_used += n; output[output_used] = 0;
}
static int paint(samir_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{
    (void)p;
    assert(r < 25 && c < 80 && n <= (uint32_t)(80 - c));
    memcpy(cells[r] + c, s, n); memset(attrs[r] + c, attr, n);
    return 0;
}
static void set_attr(samir_pal_t *p, uint8_t a) { (void)p; attr = a; }
static void gotoxy(samir_pal_t *p, uint8_t r, uint8_t c)
{ (void)p; assert(r < 25 && c < 80); row = r; col = c; }
int main(void)
{
    struct pal_host_cfg cfg = {86,7,4,8u * 1024u * 1024u};
    samir_pal_t *pal = pal_host_make(cfg);
    xb_interp *ip;
    FILE *f;
    assert(pal); memset(cells, ' ', sizeof(cells));
    pal->conin_char = input_char; pal->conin_line = input_line; pal->conout = console;
    pal->paint = paint; pal->set_attr = set_attr; pal->gotoxy = gotoxy;
    ip = xb_interp_make(pal); assert(ip);
    assert(samir_repl(pal, ip) == INTERP_OK);
    assert(atkey == (int)(sizeof(keys) / sizeof(keys[0])));
    assert(row == 0 && col == 0 && attr == KIT_ATTR_NORMAL);
    assert(strstr(output, "PESTON") && strstr(output, "WADDAMS") && strstr(output, "LUMBERGH"));
    assert(strstr(output, "berlin") && strstr(output, "25.50"));
    f = fopen("kit-after.txt", "wb"); assert(f);
    assert(fwrite(output, 1, output_used, f) == output_used); assert(!fclose(f));
    xb_interp_free(ip); pal_host_free(pal);
    puts("PASS: real-engine BROWSE menu, prompt, C/N editing, Ctrl-End, LIST, and DBF persistence");
    return 0;
}
