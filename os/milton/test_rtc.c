/* test_rtc.c -- host unit oracle for the PURE RTC conversion logic (beads
 * initech-yv9). Factory test: libc OK, reuses seed/test_assert.h. Compiles
 * HOSTED against the REAL artifact rtc.c with -DRTC_HOST_TEST so the kernel-only
 * port-I/O paths (rtc_now/rtc_set) are excluded -- only the pure
 * rtc_decode/rtc_encode/rtc_day_of_week are linked + exercised.
 *
 * Ref (Law 1): MC146818A datasheet -- registers 0x00..0x09, SRB bit1=24h,
 *   bit2=DM(1=binary,0=BCD), hour bit7=PM in 12h mode, century reg 0x32.
 *   See os/milton/rtc.h. CLAUDE.md Law 2 (oracle is truth), Rule 1
 *   (RED->GREEN), Rule 6 (mutation-prove), Rule 11 (deterministic), Rule 12
 *   (ASCII).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DRTC_MUTATE_SKIP_BCD        : decode skips the BCD->binary step -> the BCD
 *                                  fixtures decode to garbage -> RED.
 *   -DRTC_MUTATE_MONTH_OFFBYONE  : month decodes +1 -> the month assertions RED.
 * A mutant that PASSES means the oracle is decoration.
 */

#include <stdint.h>
#include "rtc.h"
#include "test_assert.h"

TEST_HARNESS();

/* Build a raw BCD snapshot (the common PC-AT mode: BCD, 24-hour). */
static rtc_raw_t bcd24(uint8_t cc, uint8_t yy, uint8_t mon, uint8_t day,
                       uint8_t hh, uint8_t mm, uint8_t ss)
{
    rtc_raw_t r;
    r.second  = ss;
    r.minute  = mm;
    r.hour    = hh;
    r.day     = day;
    r.month   = mon;
    r.year    = yy;
    r.century = cc;
    r.status_b = 0u;   /* DM=0 (BCD), 24h bit clear => 12-hour... set below */
    r.status_b |= RTC_SRB_24H;
    return r;
}

/* Encode a binary value as BCD (the on-wire register form). */
static uint8_t b(uint8_t v) { return (uint8_t)(((v / 10u) << 4) | (v % 10u)); }

int main(void)
{
    /* ===== BCD, 24-hour, century register present (the QEMU default) ====== */
    {
        /* 2026-06-09 12:34:56, century reg = 0x20. Tuesday. */
        rtc_raw_t r = bcd24(b(20), b(26), b(6), b(9), b(12), b(34), b(56));
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "BCD/24h pinned reading decodes in-range");
        CHECK(t.year == 2026u, "year == 2026 (century reg 0x20 + yy 26)");
        CHECK(t.month == 6u, "month == 6 (June)");
        CHECK(t.day == 9u, "day == 9");
        CHECK(t.hour == 12u, "hour == 12");
        CHECK(t.minute == 34u, "minute == 34");
        CHECK(t.second == 56u, "second == 56");
        CHECK(t.day_of_week == 2u, "2026-06-09 is a Tuesday (DOW==2)");
    }

    /* ===== BCD, century register ABSENT -> pivot rule (yy<80 => 20yy) ===== */
    {
        rtc_raw_t r = bcd24(0u, b(26), b(6), b(9), b(0), b(0), b(0));
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "no century reg: decodes");
        CHECK(t.year == 2026u, "pivot: yy=26 (<80) => 2026");
    }
    {
        rtc_raw_t r = bcd24(0u, b(85), b(1), b(1), b(0), b(0), b(0));
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "no century reg, yy=85: decodes");
        CHECK(t.year == 1985u, "pivot: yy=85 (>=80) => 1985");
    }

    /* ===== BINARY data mode (SRB DM bit set) ============================== */
    {
        rtc_raw_t r;
        r.second = 56u; r.minute = 34u; r.hour = 12u;
        r.day = 9u; r.month = 6u; r.year = 26u;
        r.century = 0x20u;   /* binary century reg: 32 decimal... use 20 below */
        r.century = 20u;
        r.status_b = (uint8_t)(RTC_SRB_24H | RTC_SRB_BINARY);
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "binary mode decodes");
        CHECK(t.year == 2026u && t.month == 6u && t.day == 9u,
              "binary mode date == 2026-06-09");
        CHECK(t.hour == 12u && t.minute == 34u && t.second == 56u,
              "binary mode time == 12:34:56");
    }

    /* ===== 12-hour mode: PM flag + midnight/noon edge cases =============== */
    {
        /* 11:00:00 PM -> 23:00:00. Hour reg = BCD 11 | 0x80. 24h bit clear. */
        rtc_raw_t r = bcd24(b(20), b(26), b(6), b(9), b(11), b(0), b(0));
        r.status_b = 0u;                 /* 12-hour mode (24h bit clear) */
        r.hour = (uint8_t)(b(11) | RTC_HOUR_PM);
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "12h PM decodes");
        CHECK(t.hour == 23u, "11 PM -> 23:00");
    }
    {
        /* 12:00:00 AM (midnight) -> 00:00:00. Hour reg = BCD 12, no PM. */
        rtc_raw_t r = bcd24(b(20), b(26), b(6), b(9), b(12), b(0), b(0));
        r.status_b = 0u;
        r.hour = b(12);                  /* 12 AM, PM flag clear */
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "12h midnight decodes");
        CHECK(t.hour == 0u, "12 AM -> 00:00 (midnight)");
    }
    {
        /* 12:00:00 PM (noon) -> 12:00:00. Hour reg = BCD 12 | PM. */
        rtc_raw_t r = bcd24(b(20), b(26), b(6), b(9), b(12), b(0), b(0));
        r.status_b = 0u;
        r.hour = (uint8_t)(b(12) | RTC_HOUR_PM);
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 1, "12h noon decodes");
        CHECK(t.hour == 12u, "12 PM -> 12:00 (noon)");
    }

    /* ===== day-of-week spot checks (Sakamoto), 0=Sun..6=Sat ============== */
    {
        CHECK(rtc_day_of_week(2026, 6, 9) == 2, "2026-06-09 Tuesday");
        CHECK(rtc_day_of_week(2000, 1, 1) == 6, "2000-01-01 Saturday");
        CHECK(rtc_day_of_week(2024, 2, 29) == 4, "2024-02-29 Thursday (leap)");
        CHECK(rtc_day_of_week(1985, 1, 1) == 2, "1985-01-01 Tuesday");
    }

    /* ===== out-of-range readings fail loud (Rule 2) ====================== */
    {
        rtc_raw_t r = bcd24(b(20), b(26), b(13), b(9), b(12), b(0), b(0));
        rtc_time_t t;
        CHECK(rtc_decode(&r, &t) == 0, "month 13 is rejected (invalid)");
    }
    {
        rtc_raw_t r = bcd24(b(20), b(25), b(2), b(29), b(0), b(0), b(0));
        rtc_time_t t;   /* 2025 is NOT a leap year: Feb 29 invalid */
        CHECK(rtc_decode(&r, &t) == 0, "2025-02-29 rejected (non-leap)");
    }

    /* ===== encode round-trip (SET DATE/TIME encode path, AH=2Bh/2Dh) ===== */
    {
        rtc_time_t in;
        in.year = 2026u; in.month = 6u; in.day = 9u;
        in.hour = 12u; in.minute = 34u; in.second = 56u; in.day_of_week = 0u;

        /* BCD/24h encode then decode back. */
        rtc_raw_t r;
        CHECK(rtc_encode(&in, RTC_SRB_24H, &r) == 1, "BCD/24h encode succeeds");
        CHECK(r.hour == b(12) && r.minute == b(34) && r.second == b(56),
              "BCD/24h time encodes to BCD register bytes");
        CHECK(r.year == b(26) && r.month == b(6) && r.day == b(9),
              "BCD/24h date encodes to BCD register bytes");
        CHECK(r.century == b(20), "century reg encodes 0x20 (BCD 20)");
        rtc_time_t back;
        CHECK(rtc_decode(&r, &back) == 1, "encode->decode round-trips");
        CHECK(back.year == 2026u && back.month == 6u && back.day == 9u &&
              back.hour == 12u && back.minute == 34u && back.second == 56u,
              "round-trip preserves the full date/time");
    }
    {
        /* 12-hour encode of 23:00 -> BCD 11 | PM. */
        rtc_time_t in;
        in.year = 2026u; in.month = 6u; in.day = 9u;
        in.hour = 23u; in.minute = 0u; in.second = 0u; in.day_of_week = 0u;
        rtc_raw_t r;
        CHECK(rtc_encode(&in, 0u, &r) == 1, "12h encode succeeds");
        CHECK(r.hour == (uint8_t)(b(11) | RTC_HOUR_PM),
              "23:00 encodes to BCD 11 with PM flag set");
    }
    {
        /* Invalid input is rejected. */
        rtc_time_t in;
        in.year = 2026u; in.month = 0u; in.day = 9u;
        in.hour = 12u; in.minute = 0u; in.second = 0u; in.day_of_week = 0u;
        rtc_raw_t r;
        CHECK(rtc_encode(&in, RTC_SRB_24H, &r) == 0, "month 0 rejected by encode");
    }

    return TEST_SUMMARY("test_rtc");
}
