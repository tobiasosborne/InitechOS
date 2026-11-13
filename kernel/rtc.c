/* rtc.c -- InitechDOS MC146818 RTC / CMOS clock source (beads initech-yv9).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding kernel C. The port-I/O read
 * (rtc_read_reg / rtc_now / rtc_set) is kernel-only; the conversion logic
 * (rtc_decode / rtc_encode / rtc_day_of_week) is PURE so the unit oracle
 * (os/milton/test_rtc.c) compiles + tests it HOSTED with no I/O dependency.
 *
 * Ref (Law 1): MC146818A datasheet -- ports 0x70 (index) / 0x71 (data); time/
 *   date registers 0x00..0x09; Status Reg A (0x0A) bit 7 = UIP; Status Reg B
 *   (0x0B) bit 1 = 24h, bit 2 = DM (1=binary, 0=BCD); century register 0x32.
 *   QEMU hw/rtc/mc146818rtc.c emulates exactly this. See rtc.h for the full
 *   citation. CLAUDE.md Law 2, Rule 2 (fail loud / never hang), Rule 11, Rule 12.
 */

#include "rtc.h"

/* ---- pure conversion helpers (host-testable) ------------------------------ */

/* BCD packed byte (e.g. 0x59) -> binary (59). Each nibble is a decimal digit. */
static uint8_t bcd_to_bin(uint8_t v)
{
    return (uint8_t)(((v >> 4) & 0x0Fu) * 10u + (v & 0x0Fu));
}

/* binary (0..99) -> BCD packed byte. */
static uint8_t bin_to_bcd(uint8_t v)
{
    return (uint8_t)(((v / 10u) << 4) | (v % 10u));
}

/* rtc_day_of_week: Sakamoto's algorithm (proleptic Gregorian), 0=Sun..6=Sat.
 * Valid for years >= 1. Ref: well-known Sakamoto day-of-week method. */
int rtc_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = (int)year;
    if (month < 1u || month > 12u) {
        return 0;   /* fail-safe; the caller has already range-checked */
    }
    if (month < 3) {
        y -= 1;
    }
    return (y + y / 4 - y / 100 + y / 400 + t[month - 1] + (int)day) % 7;
}

/* Days in a given month (Gregorian leap rule) -- used to range-check dates. */
static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t d[12] = {31, 28, 31, 30, 31, 30,
                                  31, 31, 30, 31, 30, 31};
    if (month < 1u || month > 12u) {
        return 0u;
    }
    if (month == 2u) {
        int leap = ((year % 4u) == 0u && (year % 100u) != 0u) ||
                   ((year % 400u) == 0u);
        return leap ? 29u : 28u;
    }
    return d[month - 1u];
}

/* Validate a normalized date/time is in range (fail loud, Rule 2). */
static int rtc_time_valid(const rtc_time_t *t)
{
    if (t->month < 1u || t->month > 12u) {
        return 0;
    }
    if (t->day < 1u || t->day > days_in_month(t->year, t->month)) {
        return 0;
    }
    if (t->hour > 23u || t->minute > 59u || t->second > 59u) {
        return 0;
    }
    return 1;
}

int rtc_decode(const rtc_raw_t *raw, rtc_time_t *out)
{
    if (raw == 0 || out == 0) {
        return 0;
    }

    int binary = (raw->status_b & RTC_SRB_BINARY) != 0;
    int h24    = (raw->status_b & RTC_SRB_24H) != 0;

    /* In 12-hour BCD mode the PM flag (bit 7) rides on the raw hour byte and
     * MUST be stripped before BCD-decoding the low bits. Capture it first. */
    uint8_t raw_hour = raw->hour;
    int pm = (!h24) && ((raw_hour & RTC_HOUR_PM) != 0);
    uint8_t hour_bits = (uint8_t)(raw_hour & (uint8_t)~RTC_HOUR_PM);

#ifdef RTC_MUTATE_SKIP_BCD
    /* MUTANT (Rule 6; make test-rtc-mutant only): treat the raw bytes as if
     * they were already binary, skipping the BCD decode. With a BCD RTC the
     * decoded fields are garbage (0x59 read as 89) and the unit oracle goes
     * RED. NEVER define in a real build. */
    binary = 1;
#endif
    uint8_t sec = binary ? raw->second : bcd_to_bin(raw->second);
    uint8_t min = binary ? raw->minute : bcd_to_bin(raw->minute);
    uint8_t hr  = binary ? hour_bits   : bcd_to_bin(hour_bits);
    uint8_t day = binary ? raw->day    : bcd_to_bin(raw->day);
    uint8_t mon = binary ? raw->month  : bcd_to_bin(raw->month);
    uint8_t yy  = binary ? raw->year   : bcd_to_bin(raw->year);
#ifdef RTC_MUTATE_MONTH_OFFBYONE
    /* MUTANT (Rule 6; make test-rtc-mutant only): off-by-one the month so a
     * June (6) reading decodes as July (7). The unit oracle's month assertion
     * goes RED. NEVER define in a real build. */
    mon = (uint8_t)(mon + 1u);
#endif

    /* 12-hour -> 24-hour normalization. 12 AM (midnight) = 0; 12 PM (noon) = 12;
     * otherwise add 12 for PM. (MC146818A: 12-hour mode stores 1..12.) */
    if (!h24) {
        if (hr == 12u) {
            hr = 0u;            /* 12 AM -> 00 */
        }
        if (pm) {
            hr = (uint8_t)(hr + 12u);
        }
    }

    /* Century: prefer the century register (reg 0x32) when present (non-zero
     * and BCD-plausible); else the pivot rule (yy < 80 => 20yy, else 19yy --
     * the standard DOS-era two-digit-year window; documented choice). */
    uint16_t year;
    uint8_t cent_bin = binary ? raw->century : bcd_to_bin(raw->century);
    if (raw->century != 0u && cent_bin >= 19u && cent_bin <= 99u) {
        year = (uint16_t)(cent_bin * 100u + yy);
    } else {
        year = (uint16_t)((yy < 80u) ? (2000u + yy) : (1900u + yy));
    }

    out->year        = year;
    out->month       = mon;
    out->day         = day;
    out->hour        = hr;
    out->minute      = min;
    out->second      = sec;
    out->day_of_week = (uint8_t)rtc_day_of_week(year, mon, day);

    return rtc_time_valid(out);
}

int rtc_encode(const rtc_time_t *in, uint8_t status_b, rtc_raw_t *out)
{
    if (in == 0 || out == 0) {
        return 0;
    }
    if (!rtc_time_valid(in)) {
        return 0;
    }

    int binary = (status_b & RTC_SRB_BINARY) != 0;
    int h24    = (status_b & RTC_SRB_24H) != 0;

    out->status_b = status_b;

    /* Hour: convert to 12h + PM flag if the RTC runs in 12-hour mode. */
    uint8_t hr = in->hour;
    uint8_t pm_flag = 0u;
    if (!h24) {
        if (hr == 0u) {
            hr = 12u;                 /* 00 -> 12 AM             */
        } else if (hr == 12u) {
            pm_flag = RTC_HOUR_PM;    /* 12 -> 12 PM             */
        } else if (hr > 12u) {
            hr = (uint8_t)(hr - 12u);
            pm_flag = RTC_HOUR_PM;
        }
    }

    uint8_t yy = (uint8_t)(in->year % 100u);
    uint8_t cc = (uint8_t)(in->year / 100u);

    out->second  = binary ? in->second : bin_to_bcd(in->second);
    out->minute  = binary ? in->minute : bin_to_bcd(in->minute);
    out->hour    = (uint8_t)((binary ? hr : bin_to_bcd(hr)) | pm_flag);
    out->day     = binary ? in->day    : bin_to_bcd(in->day);
    out->month   = binary ? in->month  : bin_to_bcd(in->month);
    out->year    = binary ? yy         : bin_to_bcd(yy);
    out->century = binary ? cc         : bin_to_bcd(cc);
    return 1;
}

/* ---- kernel-only port I/O (excluded from the host unit build) ------------- */
#ifndef RTC_HOST_TEST

#include "io.h"   /* inb/outb (freestanding) */

/* CMOS register indices (MC146818A). */
#define RTC_REG_SECONDS  0x00u
#define RTC_REG_MINUTES  0x02u
#define RTC_REG_HOURS    0x04u
#define RTC_REG_DAY      0x07u
#define RTC_REG_MONTH    0x08u
#define RTC_REG_YEAR     0x09u
#define RTC_REG_STATUS_A 0x0Au
#define RTC_REG_STATUS_B 0x0Bu
#define RTC_REG_CENTURY  0x32u

#define RTC_PORT_INDEX   0x70u
#define RTC_PORT_DATA    0x71u
#define RTC_SRA_UIP      0x80u   /* Status Reg A bit 7: update in progress */

/* The high bit of the index port (0x80) is the NMI-disable bit. Preserve a
 * disabled-NMI state across our accesses (the conventional, safe choice while
 * touching the RTC) by always setting bit 7 when selecting a register. */
static uint8_t rtc_read_reg(uint8_t reg)
{
    outb(RTC_PORT_INDEX, (uint8_t)(0x80u | reg));
    return inb(RTC_PORT_DATA);
}

static void rtc_write_reg(uint8_t reg, uint8_t val)
{
    outb(RTC_PORT_INDEX, (uint8_t)(0x80u | reg));
    outb(RTC_PORT_DATA, val);
}

/* Wait (BOUNDED -- never hang, Rule 2) until the RTC is NOT mid-update, so a
 * snapshot is coherent. Returns 1 if it settled, 0 if it never did (caller
 * reads anyway -- best effort -- but the double-read below also protects us). */
static int rtc_wait_not_updating(void)
{
    /* A real update takes <~2ms; cap the spin at a generous bound. Each
     * iteration is an I/O read (slow on real hardware, plenty of wall time). */
    for (uint32_t i = 0; i < 1000000u; i++) {
        if ((rtc_read_reg(RTC_REG_STATUS_A) & RTC_SRA_UIP) == 0u) {
            return 1;
        }
    }
    return 0;   /* gave up; fail-safe (never hang) */
}

/* Take one raw snapshot of all time/date registers. */
static void rtc_snapshot(rtc_raw_t *raw)
{
    raw->second   = rtc_read_reg(RTC_REG_SECONDS);
    raw->minute   = rtc_read_reg(RTC_REG_MINUTES);
    raw->hour     = rtc_read_reg(RTC_REG_HOURS);
    raw->day      = rtc_read_reg(RTC_REG_DAY);
    raw->month    = rtc_read_reg(RTC_REG_MONTH);
    raw->year     = rtc_read_reg(RTC_REG_YEAR);
    raw->century  = rtc_read_reg(RTC_REG_CENTURY);
    raw->status_b = rtc_read_reg(RTC_REG_STATUS_B);
}

int rtc_now(rtc_time_t *out)
{
    if (out == 0) {
        return 0;
    }

    /* Read-twice-and-compare across the UIP guard: take a snapshot after the
     * clock is not updating, take a second, and accept only when two
     * consecutive snapshots agree (so we never read a register set that ticked
     * over mid-read -- e.g. xx:59:59 -> (xx+1):00:00). Bounded retries. */
    rtc_raw_t a, b;
    for (int attempt = 0; attempt < 10; attempt++) {
        rtc_wait_not_updating();
        rtc_snapshot(&a);
        rtc_wait_not_updating();
        rtc_snapshot(&b);
        if (a.second == b.second && a.minute == b.minute &&
            a.hour == b.hour && a.day == b.day && a.month == b.month &&
            a.year == b.year && a.century == b.century) {
            return rtc_decode(&a, out);
        }
    }
    /* Did not stabilize in the retry budget -- decode the last read anyway
     * (best effort; rtc_decode still range-checks). */
    return rtc_decode(&b, out);
}

int rtc_set(const rtc_time_t *in, uint8_t which)
{
    if (in == 0) {
        return 0;
    }

    /* Encode against the LIVE status register so we write back in the RTC's own
     * BCD/binary + 12/24h mode (do not change the mode). */
    uint8_t srb = rtc_read_reg(RTC_REG_STATUS_B);
    rtc_raw_t raw;
    if (!rtc_encode(in, srb, &raw)) {
        return 0;
    }

    /* Halt updates while we write (SRB bit 7 = SET). Per the datasheet, setting
     * SET freezes the divider so a multi-register write is atomic. Clear it
     * after. We do NOT block IRQ8/periodic here -- they are not enabled. */
    uint8_t srb_set = (uint8_t)(srb | 0x80u);
    rtc_write_reg(RTC_REG_STATUS_B, srb_set);

    if (which & RTC_SET_TIME) {
        rtc_write_reg(RTC_REG_SECONDS, raw.second);
        rtc_write_reg(RTC_REG_MINUTES, raw.minute);
        rtc_write_reg(RTC_REG_HOURS,   raw.hour);
    }
    if (which & RTC_SET_DATE) {
        rtc_write_reg(RTC_REG_DAY,   raw.day);
        rtc_write_reg(RTC_REG_MONTH, raw.month);
        rtc_write_reg(RTC_REG_YEAR,  raw.year);
        if (raw.century != 0u) {
            rtc_write_reg(RTC_REG_CENTURY, raw.century);
        }
    }

    rtc_write_reg(RTC_REG_STATUS_B, srb);   /* resume updates */
    return 1;
}

#endif /* !RTC_HOST_TEST */
