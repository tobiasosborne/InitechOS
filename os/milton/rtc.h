/* rtc.h -- InitechDOS MC146818 RTC / CMOS clock source (beads initech-yv9;
 * the clock-source half of initech-509.7).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): shipped InitechDOS kernel code, freestanding
 * (<stdint.h> only). A running OS reading the RTC at runtime is fine -- the
 * NONDETERMINISM ban (Law 3 / Rule 11) is about baking host time into the BUILT
 * artifact, not about a guest reading its emulated RTC. The ORACLE pins the RTC
 * (qemu -rtc base=<stamp>) so the test is deterministic.
 *
 * Ref (Law 1):
 *   - MC146818A datasheet (Motorola, the PC-AT RTC). Address ports: 0x70
 *     (index/NMI), 0x71 (data). Time/date registers 0x00..0x09. Status Reg A
 *     (0x0A) bit 7 = UIP (Update In Progress). Status Reg B (0x0B): bit 1 = 24h
 *     (1 = 24-hour mode), bit 2 = DM (1 = binary data mode, 0 = BCD). Status
 *     Reg D (0x0D) bit 7 = VRT (valid RAM/time). Century register: 0x32 on most
 *     PC-AT clones (and the ACPI-reported index on modern boards / QEMU).
 *   - QEMU emulates a standard MC146818 (hw/rtc/mc146818rtc.c): BCD or binary
 *     per SRB DM bit, 24h per SRB bit 1, century register at 0x32. `-rtc
 *     base=<ISO>` sets a fixed wall-clock base.
 *
 * CLAUDE.md Law 1 (cite hardware ref), Law 2 (oracle), Law 3 (artifact = C),
 * Rule 2 (fail loud / never hang), Rule 11 (deterministic oracle), Rule 12
 * (ASCII).
 */
#ifndef INITECH_RTC_H
#define INITECH_RTC_H

#include <stdint.h>

/* A normalized wall-clock reading. All fields are plain binary (never BCD) and
 * year is the FULL 4-digit year. day_of_week is 0=Sunday .. 6=Saturday (the DOS
 * AH=2Ah convention), computed from the date (the RTC's own weekday register
 * 0x06 is unreliable on many BIOSes, so we derive it). */
typedef struct rtc_time {
    uint16_t year;          /* full 4-digit year, e.g. 2026                 */
    uint8_t  month;         /* 1..12                                        */
    uint8_t  day;           /* 1..31                                        */
    uint8_t  hour;          /* 0..23 (always normalized to 24-hour)         */
    uint8_t  minute;        /* 0..59                                        */
    uint8_t  second;        /* 0..59                                        */
    uint8_t  day_of_week;   /* 0=Sun .. 6=Sat (derived from the date)       */
} rtc_time_t;

/* The raw register snapshot the kernel reads from CMOS (one stable read). The
 * decode (BCD/binary, 12/24h, century) is factored OUT into rtc_decode() so it
 * is PURE and host-testable (Law 3) -- no port I/O in the conversion logic. */
typedef struct rtc_raw {
    uint8_t  second;        /* reg 0x00 (raw: BCD or binary)                */
    uint8_t  minute;        /* reg 0x02                                     */
    uint8_t  hour;          /* reg 0x04 (raw; bit 7 = PM flag in 12h mode)  */
    uint8_t  day;           /* reg 0x07 (day of month)                      */
    uint8_t  month;         /* reg 0x08                                     */
    uint8_t  year;          /* reg 0x09 (two-digit year within century)     */
    uint8_t  century;       /* reg 0x32 (BCD century, e.g. 0x20); 0 if none */
    uint8_t  status_b;      /* reg 0x0B: bit1=24h, bit2=DM(1=binary)        */
} rtc_raw_t;

/* Status Register B bit masks (MC146818A datasheet). */
#define RTC_SRB_24H    0x02u   /* bit 1: 1 = 24-hour mode, 0 = 12-hour      */
#define RTC_SRB_BINARY 0x04u   /* bit 2 (DM): 1 = binary, 0 = BCD           */
/* In 12-hour mode the hour register's bit 7 is the PM flag. */
#define RTC_HOUR_PM    0x80u

/* rtc_decode -- PURE conversion (no I/O): turn a raw register snapshot into a
 * normalized rtc_time_t. Handles BCD<->binary per SRB bit 2, 12h<->24h per SRB
 * bit 1 + the hour PM flag, the century (reg 0x32 if non-zero, else the
 * pivot rule: yy < 80 => 20yy, else 19yy), and derives day_of_week from the
 * date. Returns the decoded value in *out. This is the function the host unit
 * oracle (test-rtc) feeds known register bytes and checks (Law 2 / Rule 6).
 *
 * Returns 1 on success, 0 if the decoded date is out of range (fail loud:
 * a caller treats 0 as "RTC not trustworthy"). */
int rtc_decode(const rtc_raw_t *raw, rtc_time_t *out);

/* rtc_encode -- PURE inverse of the field encoding for SET DATE / SET TIME
 * (AH=2Bh/2Dh): given a normalized rtc_time_t and the CURRENT status_b (so we
 * match the RTC's live BCD/binary + 12/24h mode), produce the raw register
 * bytes to write back. Only the second/minute/hour/day/month/year/century raw
 * fields are filled; status_b is copied through for reference. Returns 1 on
 * success, 0 if the input is out of range (the SET handler then reports
 * invalid). day_of_week is NOT written (we derive it on read). */
int rtc_encode(const rtc_time_t *in, uint8_t status_b, rtc_raw_t *out);

/* rtc_day_of_week -- PURE: 0=Sun..6=Sat for a proleptic-Gregorian date (Sakamoto's
 * method). Exposed so AH=2Ah can fill AL and the unit oracle can check it. */
int rtc_day_of_week(uint16_t year, uint8_t month, uint8_t day);

/* rtc_now -- kernel-only: read the live CMOS RTC (ports 0x70/0x71) with the
 * UIP (update-in-progress) guard, then rtc_decode(). Returns 1 on a stable,
 * in-range read; 0 on failure (caller treats as "no clock"). NEVER hangs: the
 * UIP wait is bounded (Rule 2). NOT host-testable (port I/O); the unit oracle
 * tests rtc_decode/rtc_encode/rtc_day_of_week instead. */
int rtc_now(rtc_time_t *out);

/* rtc_set -- kernel-only: write the date/time back to the live RTC (ports
 * 0x70/0x71), matching its current BCD/binary + 12/24h mode. Returns 1 on
 * success, 0 if `in` is out of range. Used by AH=2Bh/2Dh. The seconds/minutes/
 * hours OR the day/month/year subset to write is chosen by the `which` mask so
 * SET DATE does not clobber the time and vice versa. */
#define RTC_SET_DATE 0x01u
#define RTC_SET_TIME 0x02u
int rtc_set(const rtc_time_t *in, uint8_t which);

#endif /* INITECH_RTC_H */
