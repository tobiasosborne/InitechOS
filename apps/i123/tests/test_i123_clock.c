/* B08 / initech-6edn: real Face A with injected TBX_CLOCK snapshots.
 * Reference: Lotus Reference p. 13-46 (PDF 334), @NOW changes at recalc;
 * 09-Mar-1992 = 33672 is independently minted MINT02's @DATE cache.
 * Manual-mode entry semantics: MINT04 / Reference pp. 12-31..33.
 */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

static void now_is(uint16_t c, uint16_t r, double want, const char *id)
{
    i123_val_t v;
    i123_cell_value(&g_sheet, c, r, &v);
    CHECK(v.t == I123_V_NUM && v.n == want, id, "NOW cache equals the guest date/time");
}

static void automatic(void)
{
    tks("@NOW\r");
    now_is(0, 0, 33672.5, "N1");
    CHECK(m_clock_reads == 1, "N1", "formula entry samples one guest clock");
    m_clock.hour = 18;
    right(); tks("7\r");
    now_is(0, 0, 33672.75, "N2");
    m_clock.day = 10; m_clock.hour = 0;
    calc_key(); now_is(0, 0, 33673.0, "N3");
    CHECK(m_clock_reads == 3, "N3", "automatic edit and F9 take fresh snapshots");
    /* Command mutations recalculate the candidate bank, too. */
    m_clock.hour = 6;
    tks("/reA2\r"); now_is(0, 0, 33673.25, "N4");
    tks("/fsNOWAUTO\r");
}

static void manual(void)
{
    g_sheet.calcmode = 0;
    tks("@NOW\r"); now_is(0, 0, 33672.5, "N5");
    m_clock.day = 10;
    right(); tks("8\r"); now_is(0, 0, 33672.5, "N5");
    CHECK(g_sheet.calc, "N5", "manual entry marks CALC without recalculating NOW");
    calc_key(); now_is(0, 0, 33673.5, "N6");
    CHECK(!g_sheet.calc, "N6", "F9 clears CALC");
    m_clock_rc = TBX_ERR_BADARG;
    calc_key();
    i123_val_t v; i123_cell_value(&g_sheet, 0, 0, &v);
    CHECK(v.t == I123_V_ERR && !g_cx.have_now, "N7", "clock failure clears stale success and yields ERR");
    m_clock_rc = TBX_OK; m_clock.month = 13;
    calc_key(); i123_cell_value(&g_sheet, 0, 0, &v);
    CHECK(v.t == I123_V_ERR, "N7", "invalid guest date yields ERR");
    m_clock.month = 3; m_clock.second = 30;
    calc_key(); now_is(0, 0, 33673.5 + 30.0 / 86400.0, "N8");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    m_dir = argv[1];
    g_ctr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (g_ctr == MAP_FAILED) return 2;
    launch("automatic clock", automatic);
    launch("manual clock and failure", manual);
    printf("i123-clock: %d checks, %d failures\n", g_ctr[0], g_ctr[1]);
    return g_ctr[1] ? 1 : 0;
}
