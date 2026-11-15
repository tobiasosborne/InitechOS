/* @ SAY/GET + READ against the twice-minted real III+ forms captures
 * (tests/xbase_prog_diff/screen-mint/forms-f01..f17; initech-p3nn0).
 * Expected values are literal from those captures, never from this code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_assert.h"
#include "screen_file.h"
#include "w2_host.h"
#include "ibase/screen.h"
TEST_HARNESS();
struct pal_host_cfg {uint8_t yy,mm,dd;uint32_t heap;};
extern ibase_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(ibase_pal_t *);
extern int set_register(xb_interp *);
extern int query_register(xb_interp *);

/* Whole-page capture through the same PAL paint contract as the product. */
static screen_file page;
static uint8_t cur_attr = 7, cur_r, cur_c;
static void page_reset(void)
{
    for (int i = 0; i < 2000; i++) { page.bytes[2*i] = ' '; page.bytes[2*i+1] = 7; }
    page.bytes[4000] = page.bytes[4001] = 0; cur_attr = 7; cur_r = cur_c = 0;
}
static int paint(ibase_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{
    (void)p;
    if (!n) { if (r > 25 || c >= 80) return -PAL_EIO; cur_r = r; cur_c = c; return 0; }
    if (r >= 25 || c + n > 80) return -PAL_EIO;
    for (uint32_t i = 0; i < n; i++) {
        page.bytes[2*(r*80+c+i)] = (uint8_t)s[i];
        page.bytes[2*(r*80+c+i)+1] = cur_attr;
    }
    cur_r = r; cur_c = (uint8_t)(c + n < 80 ? c + n : 79);
    return 0;
}
static void set_attr(ibase_pal_t *p, uint8_t a) { (void)p; cur_attr = a; }
static void gotoxy(ibase_pal_t *p, uint8_t r, uint8_t c) { (void)p; cur_r = r; cur_c = c; }
static int say_screen(ibase_pal_t *p, uint8_t r, uint8_t c, const char *s, uint32_t n)
{ return paint(p, r, c, s, n); }

/* Raw key byte queue: 0x00 then scan byte for extended keys, as DOS gives. */
static const uint8_t *keys; static uint32_t keys_n, keys_i;
static int32_t conin_char(ibase_pal_t *p)
{ (void)p; return keys_i < keys_n ? keys[keys_i++] : -1; }
static void feed(const uint8_t *k, uint32_t n) { keys = k; keys_n = n; keys_i = 0; }
#define FEED(s) feed((const uint8_t *)(s), (uint32_t)(sizeof(s) - 1))

static xb_interp *make_ip(ibase_pal_t *pal)
{
    xb_interp *ip = xb_interp_make(pal);
    pal->paint = paint; pal->say_screen = say_screen;
    pal->set_attr = set_attr; pal->gotoxy = gotoxy; pal->conin_char = conin_char;
    CHECK(set_register(ip) == 0 && query_register(ip) == 0, "modules register");
    CHECK(ibase_do(ip, "SET TALK OFF\nSET STATUS OFF\nSET SCOREBOARD OFF\n") == INTERP_OK,
          "capture baseline sets");
    return ip;
}
static void value_n(xb_interp *ip, const char *expr, double want)
{
    xb_val v; int ec = 0;
    int ok = xb_interp_eval(ip, expr, &v, &ec) == INTERP_OK && v.t == XB_N && v.u.n == want;
    if (!ok) fprintf(stderr, "value %s: ec=%d\n", expr, ec);
    CHECK(ok, expr);
}
static void value_c(xb_interp *ip, const char *expr, const char *want)
{
    xb_val v; int ec = 0;
    int ok = xb_interp_eval(ip, expr, &v, &ec) == INTERP_OK && v.t == XB_C &&
             v.u.c.len == strlen(want) && !memcmp(v.u.c.p, want, v.u.c.len);
    if (!ok) fprintf(stderr, "value %s: ec=%d got=[%.*s]\n", expr, ec,
                     v.t == XB_C ? (int)v.u.c.len : 0, v.t == XB_C ? v.u.c.p : "");
    CHECK(ok, expr);
}
static void cell(int r, int c, int ch, int attr)
{
    CHECK(page.bytes[2*(r*80+c)] == ch && page.bytes[2*(r*80+c)+1] == attr, "declared cell");
}

static void f01_memvar(void)
{
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    /* F01: declaration paints X at (5,10), field reversed at cols 12-19. */
    CHECK(ibase_do(ip, "VALUE=SPACE(8)\n@ 5,10 SAY \"X\" GET VALUE\n") == INTERP_OK, "declare combined SAY/GET");
    cell(5, 10, 'X', 7);
    for (int c = 12; c <= 19; c++) cell(5, c, ' ', 0x70);
    value_n(ip, "ROW()", 5); value_n(ip, "COL()", 20);
    FEED("HELLO\r");
    CHECK(ibase_do(ip, "READ\n") == INTERP_OK, "READ runs");
    value_c(ip, "VALUE", "HELLO   ");
    value_n(ip, "READKEY()", 271);
    /* Typed page: HELLO reversed from col 12 (existing mint get/read-typed). */
    CHECK(!memcmp(&page.bytes[2*(5*80+12)], "H", 1), "typed cell text");
    cell(5, 12, 'H', 0x70); cell(5, 16, 'O', 0x70);
    /* F01 second READ: list consumed, empty READ still waits for Enter. */
    FEED("\r");
    CHECK(ibase_do(ip, "READ\n") == INTERP_OK, "second empty READ returns on Enter");
    value_n(ip, "READKEY()", 15);
    /* F12: CLEAR GETS drops the list; typing goes nowhere. */
    CHECK(ibase_do(ip, "@ 5,10 GET VALUE\nCLEAR GETS\n") == INTERP_OK, "declare and CLEAR GETS");
    FEED("ZZ\r");
    CHECK(ibase_do(ip, "READ\n") == INTERP_OK, "READ after CLEAR GETS");
    value_c(ip, "VALUE", "HELLO   ");
    value_n(ip, "READKEY()", 15);
    xb_interp_free(ip); pal_host_free(pal);
}

static void f03_exits(void)
{
    static const uint8_t pgdn[] = {'A','B','\r','C','D',0,0x51};
    static const uint8_t down2[] = {0,0x50,0,0x50};
    static const uint8_t up1[] = {0,0x48};
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    const char *decl = "A=SPACE(4)\nB=SPACE(4)\n@ 5,10 GET A\n@ 7,10 GET B\nREAD\n";
    FEED("AB\rCD\x1b"); /* F03a Esc */
    CHECK(ibase_do(ip, decl) == INTERP_OK, "esc form");
    value_c(ip, "A", "AB  "); value_c(ip, "B", "    "); value_n(ip, "READKEY()", 12);
    feed(pgdn, sizeof pgdn); /* F03b PgDn saves */
    CHECK(ibase_do(ip, decl) == INTERP_OK, "pgdn form");
    value_c(ip, "A", "AB  "); value_c(ip, "B", "CD  "); value_n(ip, "READKEY()", 263);
    feed(down2, sizeof down2); /* F02b Down past last exits */
    CHECK(ibase_do(ip, decl) == INTERP_OK, "down-exit form");
    value_n(ip, "READKEY()", 5);
    feed(up1, sizeof up1); /* F02b Up on first exits */
    CHECK(ibase_do(ip, decl) == INTERP_OK, "up-exit form");
    value_n(ip, "READKEY()", 4);
    xb_interp_free(ip); pal_host_free(pal);
}

static void f15_refusals(void)
{
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    static const struct { const char *cmd; int ec; } bad[] = {
        {"@ 5,10 GET UNDEFINED\n", 12},
        {"@ 5,10 GET 1+2\n", 10},
        {"@ 5,10 GET V RANGE 1,9\n", 36},
        {"@ 5,10 GET V VALID V=\"X\"\n", 36},
        {"@ 5,10 GET V WHEN .T.\n", 36},
        {"@ 5,10 GET V PICTURE 9\n", 45},
        {"READ SAVE\n", 36},
    };
    CHECK(ibase_do(ip, "V=SPACE(4)\n") == INTERP_OK, "seed V");
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        int rc = ibase_do(ip, bad[i].cmd);
        if (rc == INTERP_OK || ibase_last_error(ip) != bad[i].ec)
            fprintf(stderr, "refusal %s rc=%d ec=%d want %d\n", bad[i].cmd, rc, ibase_last_error(ip), bad[i].ec);
        CHECK(rc != INTERP_OK && ibase_last_error(ip) == bad[i].ec, "captured refusal code");
    }
    xb_interp_free(ip); pal_host_free(pal);
}

static void f07_f08_f09_types(void)
{
    static const uint8_t num[] = {25,'-','1','2','.','3','4','5','\r'}; /* Ctrl-Y then type */
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    feed(num, sizeof num);
    CHECK(ibase_do(ip, "N=12.50\n@ 5,10 GET N\nREAD\n") == INTERP_OK, "numeric form");
    value_c(ip, "STR(N,10,2)", "    -12.35");
    value_n(ip, "READKEY()", 271);
    FEED("013100\r");
    CHECK(ibase_do(ip, "D=CTOD(\"\")\n@ 5,10 GET D\nREAD\n") == INTERP_OK, "date form");
    value_n(ip, "YEAR(D)", 1900);
    value_c(ip, "DTOC(D)", "01/31/00");
    FEED("T\r");
    CHECK(ibase_do(ip, "L=.F.\n@ 5,10 GET L\nREAD\n") == INTERP_OK, "logical form");
    { xb_val v; int ec = 0;
      CHECK(xb_interp_eval(ip, "L", &v, &ec) == INTERP_OK && v.t == XB_L && v.u.l, "logical true"); }
    value_n(ip, "READKEY()", 271);
    xb_interp_free(ip); pal_host_free(pal);
}

static void copy_file(const char *from, const char *to)
{
    FILE *a = screen_open(from, "rb"), *b = screen_open(to, "wb"); int ch;
    while ((ch = fgetc(a)) != EOF) if (fputc(ch, b) == EOF) screen_die("copy failed");
    fclose(a); fclose(b);
}
static void f13_field(void)
{
    char root[2048]; if (!getcwd(root, sizeof root)) return;
    char temp[] = "build/gets-target.XXXXXX", work[4200], path[8400];
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    if (!pal || !mkdtemp(temp)) return;
    snprintf(work, sizeof work, "%s/%s", root, temp);
    snprintf(path, sizeof path, "%s/FORM.DBF", work);
    copy_file("tests/xbase_prog_diff/screen-mint/forms-f13a-bind/SETUP/FORM.DBF", path);
    snprintf(path, sizeof path, "%s/FORM.DBT", work);
    copy_file("tests/xbase_prog_diff/screen-mint/forms-f13a-bind/SETUP/FORM.DBT", path);
    struct w2_host_env env;
    struct w2_host_config cfg = {work, work, 16*1024*1024, 16*1024*1024, 'C'};
    CHECK(w2_host_bind(&env, pal, &cfg) == 0, "field test binding");
    pal->paint = paint; pal->say_screen = say_screen;
    pal->set_attr = set_attr; pal->gotoxy = gotoxy; pal->conin_char = conin_char;
    xb_interp *ip = xb_interp_make(pal); page_reset();
    snprintf(path, sizeof path, "%s/MINT.PRG", work);
    FILE *pf = screen_open(path, "wb");
    fputs("SET TALK OFF\r\nSET STATUS OFF\r\nUSE FORM\r\n"
          "@ 5,10 SAY \"FC:\" GET FC\r\nREAD\r\n"
          "SET ALTERNATE TO RESULT\r\nSET ALTERNATE ON\r\n"
          "? \"R1=\",FC,RECNO(),READKEY()\r\nGO 2\r\n? \"R2=\",FC\r\n"
          "SET ALTERNATE OFF\r\nSET ALTERNATE TO\r\nCLOSE DATABASES\r\nRETURN\r\n", pf);
    fclose(pf);
    static const uint8_t keys13[] = {25,'K','E','Y','2','\r'};
    feed(keys13, sizeof keys13);
    int rc = w2_host_startup(pal, ip, "MINT.PRG");
    if (rc) fprintf(stderr, "field form rc=%d ec=%d\n", rc, ibase_last_error(ip));
    CHECK(rc == 0, "field form");
    snprintf(path, sizeof path, "%s/RESULT.TXT", work);
    FILE *rf = fopen(path, "rb");
    char result[512] = ""; size_t rn = rf ? fread(result, 1, sizeof result - 1, rf) : 0;
    if (rf) fclose(rf);
    result[rn] = 0;
    /* F13a RESULT shapes: edited record 1, record 2 untouched. */
    CHECK(strstr(result, "R1= KEY2") != NULL, "F13 result record 1 edited");
    CHECK(strstr(result, "R2= WXYZ") != NULL, "F13 result record 2 untouched");
    CHECK(strstr(result, "271") != NULL, "F13 result readkey");
    snprintf(path, sizeof path, "%s/FORM.DBF", work);
    FILE *f = screen_open(path, "rb");
    unsigned char rec[38];
    CHECK(fseek(f, 193, SEEK_SET) == 0 && fread(rec, 1, 38, f) == 38, "read record bytes");
    fclose(f);
    CHECK(!memcmp(rec + 1, "KEY2    ", 8), "F13 record 1 FC bytes on disk");
    CHECK(!memcmp(rec + 9, "     12.50", 10), "F13 record 1 FN untouched");
    xb_interp_free(ip); w2_host_unbind(&env); pal_host_free(pal);
}

static unsigned bell_count;
static void conout_count(ibase_pal_t *p, const char *s, uint32_t n)
{ (void)p; for (uint32_t i = 0; i < n; i++) if (s[i] == '\a') bell_count++; }
static void row_text(int r, int col, const char *want)
{
    int ok = 1;
    for (int i = 0; want[i]; i++) if (page.bytes[2*(r*80+col+i)] != (uint8_t)want[i]) ok = 0;
    if (!ok) { char got[41]; int i; for (i = 0; i < 40; i++) got[i] = (char)page.bytes[2*(r*80+col+i)]; got[40] = 0;
               fprintf(stderr, "row %d col %d want [%s] got [%s]\n", r, col, want, got); }
    CHECK(ok, "painted row text");
}

static void f05_f06_look_and_bell(void)
{
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    pal->conout = conout_count;
    /* F05b: CONFIRM ON overwrites the last column of a full field. */
    FEED("ABCDE\r\r");
    CHECK(ibase_do(ip, "SET CONFIRM ON\nA=SPACE(4)\nB=SPACE(4)\n@ 5,10 GET A\n@ 7,10 GET B\nREAD\n") == INTERP_OK, "confirm form");
    value_c(ip, "A", "ABCE"); value_c(ip, "B", "    ");
    /* F06a: default delimiters bracket the field; cursor after bracket. */
    page_reset();
    CHECK(ibase_do(ip, "SET DELIMITERS ON\n@ 5,10 GET A\n") == INTERP_OK, "delim declare");
    cell(5, 10, ':', 7); cell(5, 11, 'A', 0x70); cell(5, 15, ':', 7);
    value_n(ip, "COL()", 16);
    CHECK(ibase_do(ip, "CLEAR GETS\nSET DELIMITERS TO \"[]\"\n@ 5,10 GET A\n") == INTERP_OK, "delim pair declare");
    cell(5, 10, '[', 7); cell(5, 15, ']', 7);
    /* F06b: INTENSITY OFF paints fields in the standard attribute. */
    page_reset();
    CHECK(ibase_do(ip, "CLEAR GETS\nSET INTENSITY OFF\n@ 5,10 GET A\n") == INTERP_OK, "intensity off declare");
    cell(5, 11, 'A', 7);
    CHECK(ibase_do(ip, "CLEAR GETS\nSET INTENSITY ON\nSET DELIMITERS OFF\n") == INTERP_OK, "look reset");
    /* F14: a refused key rings only with BELL ON (speaker graded on image). */
    bell_count = 0;
    FEED("X5\r");
    CHECK(ibase_do(ip, "SET BELL ON\nN=0\n@ 5,10 GET N\nREAD\n") == INTERP_OK, "bell form");
    CHECK(bell_count == 1, "bell once for refused X with BELL ON");
    bell_count = 0;
    FEED("X5\r");
    CHECK(ibase_do(ip, "SET BELL OFF\nN=0\n@ 5,10 GET N\nREAD\n") == INTERP_OK, "nobell form");
    CHECK(bell_count == 0, "silent with BELL OFF");
    xb_interp_free(ip); pal_host_free(pal);
}

static void f10_masks(void)
{
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    /* The F10 mint ran under the capture baseline SET CONFIRM ON. */
    CHECK(ibase_do(ip, "SET CONFIRM ON\n") == INTERP_OK, "confirm on for masks");
    FEED("aB1x\r");
    CHECK(ibase_do(ip, "C=SPACE(4)\n@ 5,10 GET C PICTURE \"!!!!\"\nREAD\n") == INTERP_OK, "mask bang");
    value_c(ip, "C", "AB1X");
    FEED("1234567\r");
    CHECK(ibase_do(ip, "C=SPACE(7)\n@ 5,10 GET C PICTURE \"999-9999\"\nREAD\n") == INTERP_OK, "mask phone");
    value_c(ip, "C", "123-457");
    FEED("a1bc\r");
    CHECK(ibase_do(ip, "C=SPACE(4)\n@ 5,10 GET C PICTURE \"AAAA\"\nREAD\n") == INTERP_OK, "mask alpha");
    value_c(ip, "C", "abc ");
    FEED("1234\r");
    CHECK(ibase_do(ip, "N=0\n@ 5,10 GET N PICTURE \"999.99\"\nREAD\n") == INTERP_OK, "mask numeric");
    value_c(ip, "STR(N,10,2)", "    123.40");
    xb_interp_free(ip); pal_host_free(pal);
}

static void f11_say_pictures(void)
{
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    CHECK(ibase_do(ip,
        "@ 2,5 SAY 12.5 PICTURE \"@B 999.99\"\n"
        "@ 3,5 SAY -12.5 PICTURE \"@B 999.99\"\n"
        "@ 4,5 SAY -12.5 PICTURE \"@C 999.99\"\n"
        "@ 5,5 SAY -12.5 PICTURE \"@X 999.99\"\n"
        "@ 6,5 SAY -12.5 PICTURE \"@( 999.99\"\n"
        "@ 7,5 SAY 0 PICTURE \"@C 999.99\"\n"
        "@ 8,5 SAY \"ab\" PICTURE \"@!\"\n"
        "@ 9,5 SAY -12.5 PICTURE \"999.99\"\n") == INTERP_OK, "F11 says");
    row_text(2, 5, "12.50");
    row_text(3, 5, "-12.50");
    row_text(4, 5, "-12.50");
    row_text(5, 5, " 12.50 DB");
    row_text(6, 5, "( 12.50)");
    row_text(8, 5, "AB");
    row_text(9, 5, "-12.50");
    FEED("cd\r");
    CHECK(ibase_do(ip, "C=\"ab  \"\n@ 11,5 GET C PICTURE \"@!\"\nREAD\n") == INTERP_OK, "F11 get at-bang");
    value_c(ip, "C", "CD  ");
    xb_interp_free(ip); pal_host_free(pal);
}

static void f02_order_and_f04_edit(void)
{
    static const uint8_t order_keys[] = {
        0,0x50,0,0x50,0,0x48,'A','B',0,0x4B,0,0x47,'\r','\r'
    };
    static const uint8_t edit_keys[] = {
        '\r',0,0x4D,0,0x52,'x',0,0x53,8,0,0x4F,25,'Z','W','\r'
    };
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    /* F02: navigation follows declaration order, not screen geometry. */
    CHECK(ibase_do(ip, "C1=SPACE(4)\nC2=\"wxyz\"\nC3=SPACE(4)\n@ 8,10 GET C1\n@ 4,30 GET C2\n@ 12,10 GET C3\n") == INTERP_OK, "F02 declare");
    value_n(ip, "ROW()", 12); value_n(ip, "COL()", 14);
    feed(order_keys, sizeof order_keys);
    CHECK(ibase_do(ip, "READ\n") == INTERP_OK, "F02 read");
    value_c(ip, "C1", "    "); value_c(ip, "C2", "AByz"); value_c(ip, "C3", "    ");
    value_n(ip, "READKEY()", 271);
    /* F04: insert/delete/backspace/end/Ctrl-Y sequence ABCD -> ACZW. */
    feed(edit_keys, sizeof edit_keys);
    CHECK(ibase_do(ip, "A=SPACE(4)\nV=\"ABCD\"\n@ 5,10 GET A\n@ 7,10 GET V\nREAD\n") == INTERP_OK, "F04 read");
    value_c(ip, "V", "ACZW");
    /* F12: plain CLEAR also empties the pending list (RK stays 15). */
    FEED("ZZ\r");
    CHECK(ibase_do(ip, "A2=SPACE(4)\n@ 5,10 GET A2\nCLEAR\nREAD\n") == INTERP_OK, "F12 clear read");
    value_c(ip, "A2", "    ");
    value_n(ip, "READKEY()", 15);
    xb_interp_free(ip); pal_host_free(pal);
}

static void f07_f08_more(void)
{
    static const uint8_t invdate[] = {'0','2','2','9','0','0','\r',25,'0','1','3','1','0','0','\r'};
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    FEED("X5\r"); /* F07 N2: alphabetic key refused, digit lands */
    CHECK(ibase_do(ip, "N=0\n@ 5,10 GET N\nREAD\n") == INTERP_OK, "F07 N2");
    value_c(ip, "STR(N,10,2)", "      5.00");
    static const uint8_t blank_num[] = {25,'\r'};
    feed(blank_num, sizeof blank_num); /* F07 N3: cleared numeric confirms 0 */
    CHECK(ibase_do(ip, "N=7\n@ 5,10 GET N\nREAD\n") == INTERP_OK, "F07 N3");
    value_c(ip, "STR(N,10,2)", "      0.00");
    feed(invdate, sizeof invdate); /* F08b: invalid date refuses Enter, correct it */
    CHECK(ibase_do(ip, "D=CTOD(\"\")\n@ 5,10 GET D\nREAD\n") == INTERP_OK, "F08b");
    value_c(ip, "DTOC(D)", "01/31/00");
    FEED("013100\r"); /* F17: the Y2K entry lands in 1900 */
    CHECK(ibase_do(ip, "ASOF=CTOD(\"\")\n@ 5,10 SAY \"AS OF:\" GET ASOF\nREAD\n") == INTERP_OK, "F17");
    value_n(ip, "YEAR(ASOF)", 1900);
    value_n(ip, "ASOF-CTOD(\"01/31/00\")", 0);
    xb_interp_free(ip); pal_host_free(pal);
}

static void f18_keys_and_masks(void)
{
    static const uint8_t home_key[] = {0,0x47};
    static const uint8_t end_keys[] = {'\r',0,0x4F,0,0x4F};
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    feed(home_key, sizeof home_key); /* F18a R3: Home at the very start exits */
    CHECK(ibase_do(ip, "A=\"wxyz\"\nB=SPACE(4)\n@ 5,10 GET A\n@ 7,10 GET B\nREAD\n") == INTERP_OK, "home-exit form");
    value_n(ip, "READKEY()", 2);
    value_c(ip, "A", "wxyz");
    feed(end_keys, sizeof end_keys); /* F18c: End at the very end exits */
    CHECK(ibase_do(ip, "A=SPACE(4)\nB=\"wxyz\"\n@ 5,10 GET A\n@ 7,10 GET B\nREAD\n") == INTERP_OK, "end-exit form");
    value_n(ip, "READKEY()", 3);
    /* F18b SAY rows through the shared picture engine. */
    page_reset();
    CHECK(ibase_do(ip,
        "@ 3,5 SAY 1234.5 PICTURE \"#,###.##\"\n"
        "@ 4,5 SAY 12.5 PICTURE \"$9,999.99\"\n"
        "@ 5,5 SAY 12.5 PICTURE \"*999.99\"\n"
        "@ 6,5 SAY -7 PICTURE \"##9\"\n"
        "@ 7,5 SAY CTOD(\"01/31/00\") PICTURE \"99/99/99\"\n") == INTERP_OK, "F18b says");
    row_text(3, 5, "1,234.50");
    row_text(4, 5, "$$$$12.50");
    row_text(5, 5, "**12.50");
    row_text(6, 5, " -7");
    row_text(7, 5, "01/31/00");
    FEED("a1-2\r"); /* F18b G1: N mask refuses '-' */
    CHECK(ibase_do(ip, "SET CONFIRM ON\nC=SPACE(4)\n@ 9,5 GET C PICTURE \"NNNN\"\nREAD\n") == INTERP_OK, "N mask");
    value_c(ip, "C", "a12 ");
    FEED("T\r"); /* F18b: logical L picture */
    CHECK(ibase_do(ip, "L=.F.\n@ 5,10 GET L PICTURE \"L\"\nREAD\n") == INTERP_OK, "L picture");
    { xb_val v; int ec = 0;
      CHECK(xb_interp_eval(ip, "L", &v, &ec) == INTERP_OK && v.u.l, "L picture true"); }
    FEED("N\r"); /* F18b: logical Y picture displays Y, N stores false */
    CHECK(ibase_do(ip, "L=.T.\n@ 5,10 GET L PICTURE \"Y\"\nREAD\n") == INTERP_OK, "Y picture");
    { xb_val v; int ec = 0;
      CHECK(xb_interp_eval(ip, "L", &v, &ec) == INTERP_OK && !v.u.l, "Y picture false"); }
    xb_interp_free(ip); pal_host_free(pal);
}

static void f19_home_chain_and_badcombo(void)
{
    static const uint8_t chain[] = {'\r',0,0x47,'Z',0,0x51};
    ibase_pal_t *pal = pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    xb_interp *ip = make_ip(pal); page_reset();
    /* F19 P1: Home at the start of B lands on A's last cell. */
    feed(chain, sizeof chain);
    CHECK(ibase_do(ip, "A=SPACE(4)\nB=\"wxyz\"\n@ 5,10 GET A\n@ 7,10 GET B\nREAD\n") == INTERP_OK, "home chain form");
    value_c(ip, "A", "   Z");
    value_c(ip, "B", "wxyz");
    /* F19: combined SAY with an invalid GET paints nothing. */
    page_reset();
    CHECK(ibase_do(ip, "@ 5,10 SAY \"LBL\" GET UNDEFINED\n") != INTERP_OK &&
          ibase_last_error(ip) == 12, "bad combo refused");
    cell(5, 10, ' ', 7); cell(5, 11, ' ', 7); cell(5, 12, ' ', 7);
    xb_interp_free(ip); pal_host_free(pal);
}

int main(void)
{
    f01_memvar();
    f03_exits();
    f15_refusals();
    f07_f08_f09_types();
    f13_field();
    f05_f06_look_and_bell();
    f10_masks();
    f11_say_pictures();
    f02_order_and_f04_edit();
    f07_f08_more();
    f18_keys_and_masks();
    f19_home_chain_and_badcombo();
    return TEST_SUMMARY("test-ibase-gets");
}
