/* Pure CONFIG.SYS syntax reader. Sources: genuine DOS 3.30 CFGA..CFGF
 * captures and the retained numeric/suffix probes for initech-hp0tb.
 * Invalid lines report through an optional callback and do not abort boot.
 * The apply phase visibly refuses unsupported directives. */

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

/* Parse a decimal token, stopping at whitespace. Non-digit contamination is
 * invalid. Saturate overflow so it is refused by the directive range check. */
static uint32_t cfg_parse_uint(const char *s, uint32_t len, int *out_any)
{
    uint32_t v = 0;
    int any = 0;
    for (uint32_t i = 0; i < len; i++) {
        char c = s[i];
        if (c < '0' || c > '9') {
            /* DOS accepts a whitespace-delimited suffix, not numeric junk.
             * Captures: FILES=20 20 applies20; FILES=40garbage refuses. */
            if (!cfg_is_space(c)) any = 0;
            break;
        }
        any = 1;
        uint32_t digit = (uint32_t)(c - '0');
        /* Overflow must not turn a large refused token into a valid small one. */
        if (v > (0xFFFFFFFFu - digit) / 10u) { v = 0xFFFFFFFFu; continue; }
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

int config_sys_parse_report(const char *buf, uint32_t len, dos_config_t *out,
                            config_sys_diagnostic_fn report, void *context)
{
    if (out == 0) {
        /* Programmer bug, NOT user content -> fail loud (the repository contract). */
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

        /* ---- split KEYWORD '=' VALUE. A line with no '=' is unknown -> skip. */
        uint32_t eq = p;
        while (eq < line_end && buf[eq] != '=') {
            eq++;
        }
        if (eq >= line_end) {
            if (report) report(context);
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

        int before = recognized;
        /* ---- dispatch on the (case-insensitive) keyword ---- */
        if (cfg_kw_eq(kw, kw_len, "FILES")) {
            int any = 0;
            uint32_t n = cfg_parse_uint(val, v_len, &any);
            if (any && n >= CONFIG_SYS_FILES_MIN && n <= CONFIG_SYS_FILES_MAX) {
                out->files = (uint16_t)n;
                out->files_present = 1u;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "BUFFERS")) {
            int any = 0;
            uint32_t n = cfg_parse_uint(val, v_len, &any);
            if (any && n >= CONFIG_SYS_BUFFERS_MIN && n <= CONFIG_SYS_BUFFERS_MAX) {
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
                if (tok >= CONFIG_SYS_NAME_MAX) {
                    if (report) report(context);
                    continue;
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
                if (tok >= CONFIG_SYS_NAME_MAX) {
                    if (report) report(context);
                    continue;
                }
                char *slot = out->install[out->install_count];
                cfg_copy_clamped(slot, CONFIG_SYS_NAME_MAX, val, tok);
                out->install_count++;
                recognized++;
            }
        } else if (cfg_kw_eq(kw, kw_len, "SHELL")) {
            if (v_len >= 1u && v_len < CONFIG_SYS_SHELL_MAX) {
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
            /* Genuine DOS 3.30 accepts ONjunk as ON, unlike numeric junk. */
            if (v_len >= 2u && cfg_kw_eq(val, 2u, "ON")) {
                out->break_on = 1u;
                out->break_present = 1u;
                recognized++;
            } else if (v_len >= 3u && cfg_kw_eq(val, 3u, "OFF")) {
                out->break_on = 0u;
                out->break_present = 1u;
                recognized++;
            }
            /* else: unrecognized BREAK= value -> ignored (lenient), not fatal. */
        } else {
            /* Unknown directive (keyword had no match) -> DOS ignores it. */
        }
        if (recognized == before && report) report(context);
    }

    return recognized;
}

int config_sys_parse(const char *buf, uint32_t len, dos_config_t *out)
{
    return config_sys_parse_report(buf,len,out,0,0);
}
