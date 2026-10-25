/* PM NDX ruling; Law 1: fns-mint/ndx/GOLD.OUT lines 4-5 and 7-8.
 * Exercise the general statement slot through the real REPL/error handler.
 * Synthetic commands schedule codes without depending on NDX's index logic. */
#include <stdio.h>
#include <string.h>
#include "test_assert.h"
#include "samir/interp.h"

TEST_HARNESS();
struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static char output[16384];
static unsigned used, input_line, entries;
static const char *const lines[] = {
    "DUNTRAP", "? \"CLEAN\"",
    "ON ERROR ? \"TRAP\", ERROR(), MESSAGE()",
    "DOK", "DFAIL", "DPAIR", "DNEST", "DNESTFAIL",
    "? \"EXPR\", NDX(8), 1+\"X\"", "? \"CLEAN\"", "QUIT"
};
static void capture(samir_pal_t *p, const char *s, uint32_t n)
{
    (void)p;
    if (n < sizeof output - used) {
        memcpy(output + used, s, n);
        used += n;
        output[used] = 0;
    }
}
static int32_t input(samir_pal_t *p, char *s, uint32_t cap)
{
    const char *v = input_line < sizeof lines / sizeof lines[0]
                    ? lines[input_line++] : "QUIT";
    (void)p;
    snprintf(s, cap, "%s", v);
    return (int32_t)strlen(s);
}
static int schedule(void *user, xb_interp *ip, const char *verb,
                    const char *args, int *ec)
{
    xb_ctx *ctx = xb_interp_ctx(ip);
    int rc;
    (void)user; (void)args;
    entries++;
    CHECK(ctx->deferred_error == 0, "each statement starts with an empty slot");
    if (strcmp(verb, "DUNTRAP") && strcmp(verb, "DOK") &&
        strcmp(verb, "DFAIL") && strcmp(verb, "DPAIR") &&
        strcmp(verb, "DNEST") && strcmp(verb, "DNESTFAIL"))
        return CMD_UNKNOWN;
    ctx->deferred_error = 87;
    if (!strcmp(verb, "DFAIL")) {
        *ec = 9;
        return -INTERP_ERR_EVAL;
    }
    if (!strcmp(verb, "DPAIR")) {
        xb_val v;
        ctx->deferred_error = 11;
        CHECK(xb_interp_eval(ip, "NDX(8)", &v, ec) == INTERP_OK &&
              v.t == XB_C && v.u.c.len == 0, "function returns a normal empty value");
        CHECK(ctx->deferred_error == 11, "first deferred code wins");
    }
    if (!strcmp(verb, "DNEST") || !strcmp(verb, "DNESTFAIL")) {
        int fail = !strcmp(verb, "DNESTFAIL");
        int (*trap)(void *, int) = ctx->on_error;
        /* Let the child error propagate to this statement's handler. */
        if (fail) ctx->on_error = NULL;
        rc = samir_do(ip, fail ? "INNER=1+\"X\"" : "INNER=1");
        ctx->on_error = trap;
        CHECK(ctx->deferred_error == 87, "nested execution preserves caller's pending code");
        if (fail) {
            CHECK(rc == -INTERP_ERR_EVAL, "nested immediate error propagates");
            *ec = samir_last_error(ip);
            return rc;
        }
        CHECK(rc == INTERP_OK, "nested successful statement finishes");
    }
    xb_interp_output(ip, "DONE", 4);
    return CMD_OK;
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
    samir_pal_t *pal = pal_host_make((struct pal_host_cfg){86, 7, 24, 2*1024*1024});
    xb_interp *ip = pal ? xb_interp_make(pal) : NULL;
    CHECK(ip != NULL, "interpreter constructed");
    if (!ip) return TEST_SUMMARY("test-samir-deferred-errors");
    CHECK(xb_interp_ctx(ip)->deferred_error == 0, "pending slot initialized to zero");
    CHECK(xb_interp_add_cmd_hook(ip, schedule, NULL) == INTERP_OK, "synthetic commands registered");
    pal->conout = capture;
    pal->conin_line = input;
    CHECK(samir_repl(pal, ip) == INTERP_OK, "REPL resumes after each handled/untrapped error");
    CHECK(strstr(output, "DONE87  NDX(): Invalid index number.\n") != NULL,
          "untrapped deferred error uses ordinary renderer after side effects");
    CHECK(count("TRAP         87 NDX(): Invalid index number.") == 2,
          "deferred codes reach ON ERROR, ERROR and MESSAGE exactly once");
    CHECK(count("TRAP          9 Data type mismatch.") == 3,
          "ordinary immediate errors override and clear deferred codes");
    CHECK(count("TRAP         11 Invalid function argument.") == 1,
          "first pending code reaches ordinary handler");
    CHECK(count("CLEAN") == 2 && strstr(output, "EXPR") == NULL,
          "next statements run without stale errors; immediate failure keeps existing output behavior");
    CHECK(xb_interp_ctx(ip)->deferred_error == 0 && entries > 10,
          "slot consumed before handler and subsequent commands");
    {
        static const char *const guards[] = {
            "IF NDX(8)=\"\"\nGUARDBODY=1\nENDIF",
            "DO WHILE NDX(8)=\"\"\nGUARDBODY=1\nEXIT\nENDDO",
            "DO CASE\nCASE NDX(8)=\"\"\nGUARDBODY=1\nENDCASE"
        };
        xb_val value;
        unsigned i;
        xb_interp_ctx(ip)->on_error = NULL;
        CHECK(samir_do(ip, "STORE NDX(8) TO SAVED") == -INTERP_ERR_EVAL &&
              samir_last_error(ip) == 87, "assignment finishes before deferred error");
        CHECK(xb_interp_get_memvar(ip, "SAVED", &value) == INTERP_OK &&
              value.t == XB_C && value.u.c.len == 0, "assigned empty value survives error");
        for (i = 0; i < sizeof guards / sizeof guards[0]; i++) {
            CHECK(samir_do(ip, "GUARDBODY=0") == INTERP_OK, "normal assignment remains successful");
            CHECK(samir_do(ip, guards[i]) == -INTERP_ERR_EVAL &&
                  samir_last_error(ip) == 87, "guard consumes deferred code before body");
            CHECK(xb_interp_get_memvar(ip, "GUARDBODY", &value) == INTERP_OK &&
                  value.t == XB_N && value.u.n == 0, "deferred guard does not enter body");
            CHECK(xb_interp_ctx(ip)->deferred_error == 0, "guard clears consumed slot");
        }
        CHECK(samir_do(ip, "IF NDX(8)\nGUARDBODY=1\nENDIF") == -INTERP_ERR_EVAL &&
              samir_last_error(ip) == 37 && xb_interp_ctx(ip)->deferred_error == 0,
              "immediate guard type error overrides deferred code");
    }
    if (g_fails) fprintf(stderr, "Captured REPL output:\n%s\n", output);
    xb_interp_free(ip);
    pal_host_free(pal);
    return TEST_SUMMARY("test-samir-deferred-errors");
}
