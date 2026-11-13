/* config_sys.c -- PURE CONFIG.SYS parser (beads initech-509.2).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib),
 * <stdint.h> only, NO libc / NO malloc. Pure text -> dos_config_t transformation,
 * so it compiles BOTH freestanding (the kernel SYSINIT) and HOSTED (test_config_sys.c).
 *
 * Ref (Law 1): spec/dos_config_sys_baseline.txt (LOCKED, Rule 8); DOS 3.3
 *   CONFIG.SYS directive semantics (case-insensitive keyword=value; blank /
 *   ';'-comment / unknown lines skipped, NOT fatal; numeric clamp). config_sys.h
 *   for the struct + bounds. Rule 2 (fail loud on a PROGRAMMER bug only),
 *   Rule 11 (deterministic), Rule 12 (ASCII).
 */

#include "config_sys.h"

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stdlib.h> /* abort */
#define CONFIG_FAIL_LOUD() abort()
#else
#define CONFIG_FAIL_LOUD() __builtin_trap()
#endif

/* --- tiny freestanding string helpers (no <string.h>) -------------------- */

static void cfg_zero(uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        p[i] = 0;
    }
}

static char cfg_upper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

static int cfg_is_space(char c)
{
    /* CONFIG.SYS uses ' ' and '\t' as separators; CR/LF terminate the line. */
    return c == ' ' || c == '\t';
}

/* Case-insensitive compare of `s` (len chars) against a NUL-terminated keyword.
 * Returns 1 iff every char matches (case-folded) AND the keyword is fully
 * consumed by exactly `len` chars. */
static int cfg_kw_eq(const char *s, uint32_t len, const char *kw)
{
    uint32_t i = 0;
    for (; i < len; i++) {
        if (kw[i] == '\0') {
            return 0; /* `s` longer than the keyword */
        }
        if (cfg_upper(s[i]) != cfg_upper(kw[i])) {
            return 0;
        }
    }
    return kw[i] == '\0'; /* keyword exactly consumed */
}

/* Parse a non-negative decimal from s[0..len). Stops at the first non-digit.
 * Saturates at 0xFFFFFFFF (then the caller clamps). *out_any = 1 if >=1 digit. */
static uint32_t cfg_parse_uint(const char *s, uint32_t len, int *out_any)
{
    uint32_t v = 0;
    int any = 0;
    for (uint32_t i = 0; i < len; i++) {
        char c = s[i];
        if (c < '0' || c > '9') {
            break;
        }
        any = 1;
        uint32_t digit = (uint32_t)(c - '0');
#if defined(CONFIG_MUTATE_NO_OVERFLOW_GUARD)
        /* Rule-6 mutant (make test-config-fuzz-mutant): REMOVE the overflow guard
         * entirely, so a 10+ digit FILES=/BUFFERS= value silently wraps the uint32
         * accumulator (modular *10+d) instead of saturating -> the fuzzer's
         * saturate-and-clamp-to-MAX invariant goes RED. NEVER in a real build. */
#elif defined(CONFIG_MUTATE_OVERFLOW_OFFBYONE)
        /* Rule-6 mutant (make test-config-fuzz-mutant): the ORIGINAL off-by-one
         * guard (initech-zfo). `v > 429496729` lets v==429496729 with a trailing
         * digit >=6 fall through and wrap past UINT32_MAX (e.g. "4294967296" -> 0
         * -> clamps to FILES_MIN instead of saturating to MAX). Proves the
         * corrected guard below actually bites the 2^32-family. NEVER in a real build. */
        if (v > 429496729u) { v = 0xFFFFFFFFu; continue; }
#else
        /* Saturate at 0xFFFFFFFF (the documented contract). Guard the EXACT
         * overflow of v*10+digit: the previous `v > 429496729` form let
         * v==429496729 with digit>=6 wrap past UINT32_MAX (bug initech-zfo --
         * "4294967296" wrapped to 0 and clamped to FILES_MIN, not MAX). */
        if (v > (0xFFFFFFFFu - digit) / 10u) { v = 0xFFFFFFFFu; continue; }
#endif
        v = v * 10u + digit;
    }
    if (out_any) {
        *out_any = any;
    }
    return v;
}

/* Copy s[0..len) into dst (capacity cap incl. NUL), truncating + NUL-terminating.
 * Returns the number of chars copied (excl. NUL). */
static uint32_t cfg_copy_clamped(char *dst, uint32_t cap, const char *s, uint32_t len)
{
    uint32_t n = 0;
    if (cap == 0) {
        return 0;
    }
    while (n < len && n + 1u < cap) {
        dst[n] = s[n];
        n++;
    }
    dst[n] = '\0';
    return n;
}

/* --- the parser ---------------------------------------------------------- */

int config_sys_parse(const char *buf, uint32_t len, dos_config_t *out)
{
    if (out == 0) {
        /* Programmer bug, NOT user content -> fail loud (Rule 2). */
        CONFIG_FAIL_LOUD();
        return 0; /* not reached */
    }

    cfg_zero((uint8_t *)out, (uint32_t)sizeof(*out));

    if (buf == 0 || len == 0) {
        return 0; /* empty file: every directive absent */
    }

    int recognized = 0;
    uint32_t i = 0;

    while (i < len) {
        /* ---- isolate one line [line_start, line_end) (LF/CRLF terminated) --- */
        uint32_t line_start = i;
        uint32_t line_end = i;
        while (line_end < len && buf[line_end] != '\n' && buf[line_end] != '\r') {
            line_end++;
        }
        /* advance i past the line + its terminator (handle CRLF + bare CR/LF). */
        i = line_end;
        if (i < len && buf[i] == '\r') {
            i++;
        }
        if (i < len && buf[i] == '\n') {
            i++;
        }

        /* ---- trim leading whitespace ---- */
        uint32_t p = line_start;
        while (p < line_end && cfg_is_space(buf[p])) {
            p++;
        }
        if (p >= line_end) {
            continue; /* blank line -> skip */
        }
        if (buf[p] == ';') {
            continue; /* comment line -> skip (DOS ignores) */
        }

        /* ---- split KEYWORD '=' VALUE. A line with no '=' is unknown -> skip. */
        uint32_t eq = p;
        while (eq < line_end && buf[eq] != '=') {
            eq++;
        }
        if (eq >= line_end) {
#ifdef CONFIG_MUTATE_FAIL_ON_UNKNOWN
            /* Rule-6 mutant target (make test-config-sys-mutant): treat an
             * unrecognized line as FATAL (real DOS does NOT) -> the "unknown
             * ignored" assertions go RED. NEVER define in a real build. */
            return -1;
#endif
            continue; /* no '=' -> unrecognized directive, skip (NOT fatal) */
        }

        /* keyword = [kw_start, kw_end), right-trimmed of spaces before '='. */
        uint32_t kw_start = p;
        uint32_t kw_end = eq;
        while (kw_end > kw_start && cfg_is_space(buf[kw_end - 1])) {
            kw_end--;
        }
        uint32_t kw_len = kw_end - kw_start;
        const char *kw = &buf[kw_start];

        /* value = (eq+1 .. line_end), left-trimmed of spaces, right-trimmed of
         * trailing spaces. (SHELL keeps interior spaces -- e.g. the switches.) */
        uint32_t v_start = eq + 1u;
        while (v_start < line_end && cfg_is_space(buf[v_start])) {
            v_start++;
        }
        uint32_t v_end = line_end;
        while (v_end > v_start && cfg_is_space(buf[v_end - 1])) {
            v_end--;
        }
        uint32_t v_len = v_end - v_start;
        const char *val = &buf[v_start];

        /* ---- dispatch on the (case-insensitive) keyword ---- */
        if (cfg_kw_eq(kw, kw_len, "FILES")) {
            int any = 0;
            uint32_t n = cfg_parse_uint(val, v_len, &any);
            if (any) {
                if (n < CONFIG_SYS_FILES_MIN) { n = CONFIG_SYS_FILES_MIN; }
                if (n > CONFIG_SYS_FILES_MAX) { n = CONFIG_SYS_FILES_MAX; }
#ifdef CONFIG_MUTATE_FILES_OFFBYONE
                /* Rule-6 mutant target (make test-config-sys-mutant): off-by-one
                 * on the parsed FILES value -> test goes RED on files==20. NEVER
                 * define in a real build. */
                n += 1u;
#endif
                out->files = (uint16_t)n;
                out->files_present = 1u;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "BUFFERS")) {
            int any = 0;
            uint32_t n = cfg_parse_uint(val, v_len, &any);
            if (any) {
                if (n < CONFIG_SYS_BUFFERS_MIN) { n = CONFIG_SYS_BUFFERS_MIN; }
                if (n > CONFIG_SYS_BUFFERS_MAX) { n = CONFIG_SYS_BUFFERS_MAX; }
                out->buffers = (uint16_t)n;
                out->buffers_present = 1u;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "LASTDRIVE")) {
            if (v_len >= 1u) {
                char d = cfg_upper(val[0]);
                if (d >= 'A' && d <= 'Z') {
                    out->lastdrive = d;
                    out->lastdrive_present = 1u;
                    recognized++;
                }
            }
        } else if (cfg_kw_eq(kw, kw_len, "DEVICE")) {
            if (v_len >= 1u && out->device_count < (uint8_t)CONFIG_SYS_MAX_DEVICES) {
                /* The DEVICE= value may carry a path + driver switches; we retain
                 * the FIRST whitespace-delimited token (the 8.3 driver name), which
                 * is what the device milestone (509.7) loads. */
                uint32_t tok = 0;
                while (tok < v_len && !cfg_is_space(val[tok])) {
                    tok++;
                }
                char *slot = out->devices[out->device_count];
                cfg_copy_clamped(slot, CONFIG_SYS_NAME_MAX, val, tok);
                out->device_count++;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "INSTALL")) {
            if (v_len >= 1u && out->install_count < (uint8_t)CONFIG_SYS_MAX_INSTALL) {
                uint32_t tok = 0;
                while (tok < v_len && !cfg_is_space(val[tok])) {
                    tok++;
                }
                char *slot = out->install[out->install_count];
                cfg_copy_clamped(slot, CONFIG_SYS_NAME_MAX, val, tok);
                out->install_count++;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "SHELL")) {
            if (v_len >= 1u) {
                /* SHELL keeps the full tail (path + switches) -- e.g.
                 * "COMMAND.COM /P /E:512" -- for 509.10/k6x. */
                cfg_copy_clamped(out->shell, CONFIG_SYS_SHELL_MAX, val, v_len);
                out->shell_present = 1u;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "BREAK")) {
            /* BREAK=ON|OFF (beads initech-er3h; ADR-0003 Amendment DEC-16). The
             * value is the literal token ON or OFF (case-insensitive). SYSINIT
             * flows break_present/break_on into the kernel g_break_flag via
             * int21_set_break_flag (DEC-16 Sec 3.3 / C-4). PERIOD-CORRECT
             * LENIENCY: a malformed value (e.g. BREAK=MAYBE) is NOT recognized
             * -- break_present stays 0 so the boot default ON stands, exactly as
             * DOS ignores a directive value it does not understand. */
            if (cfg_kw_eq(val, v_len, "ON")) {
                out->break_on = 1u;
                out->break_present = 1u;
                recognized++;
            } else if (cfg_kw_eq(val, v_len, "OFF")) {
                out->break_on = 0u;
                out->break_present = 1u;
                recognized++;
            }
            /* else: unrecognized BREAK= value -> ignored (lenient), not fatal. */
        } else {
            /* Unknown directive (keyword had no match) -> DOS ignores it. */
#ifdef CONFIG_MUTATE_FAIL_ON_UNKNOWN
            /* Rule-6 mutant: an unknown keyword is FATAL -> RED. */
            return -1;
#endif
        }
    }

    return recognized;
}
