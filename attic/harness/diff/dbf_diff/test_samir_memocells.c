/* SET MEMOWIDTH: a LIST row with more wrapped memo columns than the row's
 * continuation cells must fail loud (error 46), never drop lines (Rule 2).
 * Run in a directory holding MEMO.DBF/MEMO.DBT (build/w2_fixture <dir>). */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "samir/interp.h"

TEST_HARNESS();
struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static char output[16384];
static unsigned used;
static void capture(samir_pal_t *p, const char *s, uint32_t n)
{
    (void)p;
    if (n < sizeof output - used) { memcpy(output + used, s, n); used += n; output[used] = 0; }
}
static unsigned input_line;
static const char *const lines[] = {
    "USE MEMO", "SET MEMOWIDTH TO 20", "LIST NOTE,NOTE,NOTE,NOTE",
    "? \"MARK\"", "ON ERROR ? \"TRAP\", ERROR()",
    "LIST NOTE,NOTE,NOTE,NOTE,NOTE", "? \"AFTER\"", "QUIT"
};
static int32_t input(samir_pal_t *p, char *s, uint32_t cap)
{
    const char *v = input_line < sizeof lines / sizeof lines[0] ? lines[input_line++] : "QUIT";
    (void)p;
    snprintf(s, cap, "%s", v);
    return (int32_t)strlen(s);
}
static unsigned count(const char *needle)
{
    unsigned n = 0;
    const char *p = output;
    while ((p = strstr(p, needle)) != NULL) { n++; p += strlen(needle); }
    return n;
}
int main(void)
{
    const char *mark;
    samir_pal_t *pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 2*1024*1024});
    xb_interp *ip = pal ? xb_interp_make(pal) : NULL;
    CHECK(ip != NULL, "interpreter constructed");
    if (!ip) return TEST_SUMMARY("test-samir-memocells");
    pal->conout = capture;
    pal->conin_line = input;
    CHECK(samir_repl(pal, ip) == INTERP_OK, "REPL ran the script");
    mark = strstr(output, "MARK");
    CHECK(mark != NULL && count("papa.") == 4, "four wrapped memo columns print in full (within the cell limit)");
    CHECK(strstr(output, "TRAP         46") != NULL, "a fifth wrapped memo column fails loud with error 46");
    CHECK(count("papa.") == 4 && strstr(output, "AFTER") != NULL, "no partial row is printed and the session continues");
    if (g_fails) fprintf(stderr, "Captured REPL output:\n%s\n", output);
    xb_interp_free(ip);
    pal_host_free(pal);
    return TEST_SUMMARY("test-samir-memocells");
}
