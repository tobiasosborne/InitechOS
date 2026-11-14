/* Adversarial CONFIG.SYS properties: bounded output, read-only input,
 * valid numeric ranges and last accepted FILES token. Host libc conversion
 * is independent of the product accumulator. Accepted/refused boundaries
 * come from genuine DOS 3.30 captures, not product arithmetic. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "config_sys.h"

/* ---- Deterministic PRNG: splitmix64 seed -> xorshift128+ stream (the repository contract). --
 * Mirrors kernel/tests/fat12_fuzz.c so the factory has one PRNG idiom. */
typedef struct { uint64_t s0, s1; } prng_t;

static uint64_t splitmix64(uint64_t *x)
{
    uint64_t z = (*x += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static void prng_seed(prng_t *p, uint64_t seed)
{
    uint64_t x = seed + 0x1234567890ABCDEFull; /* fixed offset; no clock */
    p->s0 = splitmix64(&x);
    p->s1 = splitmix64(&x);
}

static uint64_t prng_next(prng_t *p)
{
    uint64_t s1 = p->s0;
    uint64_t s0 = p->s1;
    uint64_t r  = s0 + s1;
    p->s0 = s0;
    s1 ^= s1 << 23;
    p->s1 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return r;
}

/* Uniform-ish in [0, n) (n > 0); fine for fuzz selection (not crypto). */
static uint32_t prng_below(prng_t *p, uint32_t n)
{
    return (uint32_t)(prng_next(p) % (uint64_t)n);
}

/* ---- The fuzz buffer: a CONFIG.SYS text canvas with GUARD brackets. ---------
 * The parser reads [buf, buf+len) and writes ONLY into *out. We bracket BOTH the
 * input buffer and the output struct with magic guards and verify them untouched
 * (input must be read-only; output must never overflow). */
#define GUARD_MAGIC 0xA5u
#define GUARD_LEN   64u
#define TEXT_CAP    4096u     /* max synthesized CONFIG.SYS bytes (room for >255-char lines) */

typedef struct {
    uint8_t lo_guard[GUARD_LEN];
    char    text[TEXT_CAP];
    uint8_t hi_guard[GUARD_LEN];
} guarded_text_t;

typedef struct {
    uint8_t      lo_guard[GUARD_LEN];
    dos_config_t cfg;
    uint8_t      hi_guard[GUARD_LEN];
} guarded_cfg_t;

static void fill_guard(uint8_t *p, uint32_t n) { for (uint32_t i = 0; i < n; i++) p[i] = GUARD_MAGIC; }
static int  guard_ok(const uint8_t *p, uint32_t n) { for (uint32_t i = 0; i < n; i++) if (p[i] != GUARD_MAGIC) return 0; return 1; }

/* Append a single byte to the text canvas if room remains. */
static void emit(char *t, uint32_t *len, char c)
{
    if (*len < TEXT_CAP) {
        t[(*len)++] = c;
    }
}

static void emit_str(char *t, uint32_t *len, const char *s)
{
    for (; *s; s++) emit(t, len, *s);
}

/* A small pool of keywords: honored ones the parser recognizes, plus deliberate
 * unknowns the parser must skip leniently. */
static const char *const KW_HONORED[] = { "FILES", "BUFFERS", "LASTDRIVE",
                                          "DEVICE", "INSTALL", "SHELL" };
static const char *const KW_UNKNOWN[] = { "FOOBAR", "STACKS", "FCBS", "BREAK",
                                          "COUNTRY", "REM", "DOS", "XYZ" };

/* Mixed-case mangle: randomly lower-case some letters so the parser's
 * case-insensitive match is exercised on hostile casing too. */
static void emit_kw_mangled(char *t, uint32_t *len, prng_t *p, const char *kw)
{
    for (; *kw; kw++) {
        char c = *kw;
        if (c >= 'A' && c <= 'Z' && (prng_next(p) & 1u)) {
            c = (char)(c - 'A' + 'a');
        }
        emit(t, len, c);
    }
}

/* Whitespace noise: 0..3 spaces/tabs. */
static void emit_ws(char *t, uint32_t *len, prng_t *p)
{
    uint32_t n = prng_below(p, 4);
    for (uint32_t i = 0; i < n; i++) {
        emit(t, len, (prng_next(p) & 1u) ? ' ' : '\t');
    }
}

/* A line terminator chosen at random: LF, CRLF, bare CR, or (rarely) none
 * (the last line may be unterminated -- the parser must handle that). */
static void emit_eol(char *t, uint32_t *len, prng_t *p)
{
    switch (prng_below(p, 4)) {
        case 0: emit(t, len, '\n'); break;
        case 1: emit(t, len, '\r'); emit(t, len, '\n'); break;
        case 2: emit(t, len, '\r'); break;
        default: emit(t, len, '\n'); break; /* bias toward LF */
    }
}

/* What a NUMBER token looks like for FILES=/BUFFERS=. We emit one of:
 *  - a small in-range value;
 *  - an out-of-range-but-uint32 value (e.g. 9999, 0, 1000000);
 *  - a HUGE decimal that overflows the uint32 accumulator (a fixed >2^32 family,
 *    or 10..29 random digits). digits_out records the decimal string verbatim so
 *    the differential checker can replay it through ref_files_value.
 *  - a NON-numeric token (letters / punctuation) so cfg_parse_uint sees 0 digits.
 * Returns 1 if a (possibly-empty) numeric token was emitted, 0 for non-numeric. */
static int emit_value_token(char *t, uint32_t *len, prng_t *p, char *digits_out,
                            uint32_t digits_cap)
{
    digits_out[0] = '\0';
    uint32_t pick = prng_below(p, 10);

    if (pick == 0) {
        /* purely non-numeric: a-z + punctuation -> 0 digits parsed */
        uint32_t n = 1 + prng_below(p, 6);
        for (uint32_t i = 0; i < n; i++) {
            const char *junk = "abcXYZ_/.-:";
            emit(t, len, junk[prng_below(p, 11)]);
        }
        return 0;
    }

    if (pick == 1) {
        /* Values whose wrap would masquerade as an accepted small FILES token. */
        static const char *const OVR[] = {
            "4294967296",            /* 2^32        */
            "8589934592",            /* 2^33        */
            "12884901888",           /* 3 * 2^32    */
            "42949672960",           /* 10 * 2^32   */
            "4294967296000000",      /* 2^32 * 10^6 */
            "18446744073709551616",  /* 2^64        */
            "4294967316",            /* 2^32+20: a wrap falsely accepts20 */
            "8589934612",            /* 2^33+20 */
            "18446744073709551636"   /* 2^64+20 */
        };
        const char *s = OVR[prng_below(p, sizeof OVR / sizeof OVR[0])];
        uint32_t di = 0;
        for (const char *q = s; *q; q++) {
            emit(t, len, *q);
            if (di + 1u < digits_cap) digits_out[di++] = *q;
        }
        digits_out[di] = '\0';
        return 1;
    }

    /* Build a decimal string of L digits (pick in 2..9 here). */
    uint32_t L;
    if (pick <= 4)      L = 1 + prng_below(p, 3);   /* short: 1..3 digits */
    else if (pick <= 6) L = 4 + prng_below(p, 4);   /* mid:   4..7 digits */
    else                L = 10 + prng_below(p, 20);  /* HUGE:  10..29 digits -> overflow */

    uint32_t di = 0;
    for (uint32_t i = 0; i < L; i++) {
        /* First digit 1..9 (avoid an all-zero leading run muddying the size
         * comparison); subsequent digits 0..9. Bias the huge case toward big
         * leading digits so the lexical-size test is unambiguous. */
        char d;
        if (i == 0) {
            d = (char)('1' + prng_below(p, 9));
        } else {
            d = (char)('0' + prng_below(p, 10));
        }
        emit(t, len, d);
        if (di + 1u < digits_cap) {
            digits_out[di++] = d;
        }
    }
    digits_out[di] = '\0';
    return 1;
}

/* Independently parse the generated decimal token using host libc. */
static uint16_t ref_files_value(const char *digits)
{
    /* Host libc is independent of the product's decimal accumulator. The
     * accepted range is grounded by the genuine0/7/8/255/256 captures. */
    char *end;
    errno=0;
    unsigned long long n=strtoull(digits,&end,10);
    if (errno || end==digits || *end || n<5 || n>255) return 0;
    return (uint16_t)n;
}

/* Generate ONE adversarial CONFIG.SYS into the text canvas. Returns the byte
 * length. Records, for the invariant checker, the LAST honored FILES= numeric
 * token's digit string (the parser keeps the last present value). */
typedef struct {
    int  files_seen;        /* a FILES= line with a numeric token was emitted */
    char files_digits[64];  /* the LAST such token's decimal digits           */
} gen_meta_t;

static uint32_t generate(char *t, prng_t *p, uint32_t max_lines, gen_meta_t *meta)
{
    uint32_t len = 0;
    meta->files_seen = 0;
    meta->files_digits[0] = '\0';

    uint32_t lines = 1 + prng_below(p, max_lines);
    for (uint32_t li = 0; li < lines && len < TEXT_CAP - 512u; li++) {
        uint32_t shape = prng_below(p, 12);

        if (shape == 0) {
            /* blank line (maybe with whitespace) */
            emit_ws(t, &len, p);
            emit_eol(t, &len, p);
            continue;
        }
        if (shape == 1) {
            /* comment line */
            emit_ws(t, &len, p);
            emit(t, &len, ';');
            emit_str(t, &len, " hostile comment ;= FILES=999");
            emit_eol(t, &len, p);
            continue;
        }
        if (shape == 2) {
            /* a line with NO '=' (unknown directive form -> must be skipped) */
            emit_ws(t, &len, p);
            emit_kw_mangled(t, &len, p, KW_UNKNOWN[prng_below(p, 8)]);
            emit_ws(t, &len, p);
            emit_str(t, &len, "nonsense without an equals");
            emit_eol(t, &len, p);
            continue;
        }
        if (shape == 3) {
            /* a >255-char line: long run of junk before/around an '=' so the
             * line-isolation + trim path is stressed with an oversized line. */
            emit_ws(t, &len, p);
            uint32_t run = 256 + prng_below(p, 120);
            for (uint32_t i = 0; i < run && len < TEXT_CAP - 16u; i++) {
                emit(t, &len, (char)('A' + (int)prng_below(p, 26)));
            }
            if (prng_next(p) & 1u) {
                emit(t, &len, '=');
                emit_str(t, &len, "value");
            }
            emit_eol(t, &len, p);
            continue;
        }
        if (shape == 4) {
            /* unknown keyword = value (recognized as a line, skipped leniently) */
            emit_ws(t, &len, p);
            emit_kw_mangled(t, &len, p, KW_UNKNOWN[prng_below(p, 8)]);
            emit_ws(t, &len, p);
            emit(t, &len, '=');
            emit_ws(t, &len, p);
            char junk[64];
            (void)emit_value_token(t, &len, p, junk, sizeof(junk));
            emit_eol(t, &len, p);
            continue;
        }

        /* shapes 5..11: a HONORED keyword with a (possibly hostile) value. */
        const char *kw = KW_HONORED[prng_below(p, 6)];
        int is_files = (strcmp(kw, "FILES") == 0);

        emit_ws(t, &len, p);
        emit_kw_mangled(t, &len, p, kw);
        emit_ws(t, &len, p);
        /* sometimes drop the '=' to make even a honored keyword a no-'=' line */
        int has_eq = (prng_below(p, 8) != 0);
        if (has_eq) {
            emit(t, &len, '=');
            emit_ws(t, &len, p);
        }
        char digits[64];
        int numeric = emit_value_token(t, &len, p, digits, sizeof(digits));
        emit_eol(t, &len, p);

        /* Track the LAST present FILES= numeric value (the parser keeps it). A
         * FILES line only honors when it has '=' AND a numeric token with >=1
         * digit (cfg_parse_uint sets any=1). */
        if (is_files && has_eq && numeric && ref_files_value(digits)) {
            meta->files_seen = 1;
            strncpy(meta->files_digits, digits, sizeof(meta->files_digits) - 1);
            meta->files_digits[sizeof(meta->files_digits) - 1] = '\0';
        }
    }
    return len;
}

/* Run one seed: synthesize, parse, assert all invariants. Returns 0 on success;
 * on the FIRST violation prints the seed + the offending CONFIG.SYS and returns
 * non-zero (replayable). */
static int run_seed(uint64_t seed, uint32_t max_lines)
{
    prng_t prng;
    prng_seed(&prng, seed);

    guarded_text_t *gt = malloc(sizeof(*gt));
    guarded_cfg_t  *gc = malloc(sizeof(*gc));
    if (!gt || !gc) {
        fprintf(stderr, "  FATAL: out of memory\n");
        free(gt); free(gc);
        return 2;
    }

    fill_guard(gt->lo_guard, GUARD_LEN);
    fill_guard(gt->hi_guard, GUARD_LEN);
    memset(gt->text, 0, sizeof(gt->text));

    gen_meta_t meta;
    uint32_t len = generate(gt->text, &prng, max_lines, &meta);

    /* Snapshot the input so we can prove the parser did not mutate it. */
    static char snapshot[TEXT_CAP];
    memcpy(snapshot, gt->text, sizeof(snapshot));

    fill_guard(gc->lo_guard, GUARD_LEN);
    fill_guard(gc->hi_guard, GUARD_LEN);

    /* THE call to the REAL artifact parser. */
    int recognized = config_sys_parse(gt->text, len, &gc->cfg);

    int fail = 0;
    const char *why = "";

    /* (2) never crashes -- if we are here it returned. recognized must be >= 0. */
    if (recognized < 0) { fail = 1; why = "parser returned negative (treated user content as fatal)"; }

    /* (1) no overflow: every guard byte intact (output struct + input brackets). */
    if (!fail && !guard_ok(gc->lo_guard, GUARD_LEN)) { fail = 1; why = "dos_config_t LOW guard clobbered (underflow write)"; }
    if (!fail && !guard_ok(gc->hi_guard, GUARD_LEN)) { fail = 1; why = "dos_config_t HIGH guard clobbered (OVERFLOW write past struct)"; }
    if (!fail && !guard_ok(gt->lo_guard, GUARD_LEN)) { fail = 1; why = "input LOW guard clobbered"; }
    if (!fail && !guard_ok(gt->hi_guard, GUARD_LEN)) { fail = 1; why = "input HIGH guard clobbered"; }

    /* input must be read-only. */
    if (!fail && memcmp(snapshot, gt->text, len) != 0) { fail = 1; why = "parser MUTATED its read-only input buffer"; }

    /* (3) FILES clamp: whenever present, value in [MIN, MAX]. */
    if (!fail && gc->cfg.files_present) {
        if (gc->cfg.files < CONFIG_SYS_FILES_MIN || gc->cfg.files > CONFIG_SYS_FILES_MAX) {
            fail = 1; why = "FILES present but OUTSIDE [MIN,MAX]";
        }
    }
    /* BUFFERS clamp likewise. */
    if (!fail && gc->cfg.buffers_present) {
        if (gc->cfg.buffers < CONFIG_SYS_BUFFERS_MIN || gc->cfg.buffers > CONFIG_SYS_BUFFERS_MAX) {
            fail = 1; why = "BUFFERS present but OUTSIDE [MIN,MAX]";
        }
    }

    /* Compare the last accepted token with the independent host conversion. */
    uint16_t ref_files = 0;
    if (meta.files_seen) {
        ref_files = ref_files_value(meta.files_digits);
        if (!fail && !gc->cfg.files_present) {
            fail = 1; why = "FILES= numeric token was dropped (expected present)";
        } else if (!fail && gc->cfg.files != ref_files) {
            fail = 1; why = "live FILES differs from independent accepted token";
        }
    }
    if (!fail && (gc->cfg.files_present != (meta.files_seen != 0))) {
        fail=1; why="FILES presence differs from independent accepted-token model";
    }

    /* (4) leniency: counts are bounded + sane; device/install counts cannot
     * exceed their caps (else a count overran its array). */
    if (!fail && gc->cfg.device_count > CONFIG_SYS_MAX_DEVICES) { fail = 1; why = "device_count exceeds CONFIG_SYS_MAX_DEVICES"; }
    if (!fail && gc->cfg.install_count > CONFIG_SYS_MAX_INSTALL) { fail = 1; why = "install_count exceeds CONFIG_SYS_MAX_INSTALL"; }

    if (fail) {
        fprintf(stderr, "  FAIL seed=%llu: %s\n", (unsigned long long)seed, why);
        fprintf(stderr, "  recognized=%d files_present=%u files=%u ref_files=%u digits='%s'\n",
                recognized, (unsigned)gc->cfg.files_present, (unsigned)gc->cfg.files,
                (unsigned)ref_files, meta.files_digits);
        fprintf(stderr, "  ---- offending CONFIG.SYS (%u bytes; replay: --seed %llu) ----\n",
                len, (unsigned long long)seed);
        fwrite(gt->text, 1, len, stderr);
        fprintf(stderr, "\n  ---- end ----\n");
        free(gt); free(gc);
        return 1;
    }

    free(gt); free(gc);
    return 0;
}

int main(int argc, char **argv)
{
    uint64_t seed_lo = 0, seed_hi = 0;
    uint32_t max_lines = 24;
    int sweep = 0, have_seed = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed_lo = seed_hi = strtoull(argv[++i], NULL, 10);
            have_seed = 1;
        } else if (strcmp(argv[i], "--sweep") == 0 && i + 2 < argc) {
            seed_lo = strtoull(argv[++i], NULL, 10);
            seed_hi = strtoull(argv[++i], NULL, 10);
            sweep = 1;
        } else if (strcmp(argv[i], "--ops") == 0 && i + 1 < argc) {
            max_lines = (uint32_t)strtoul(argv[++i], NULL, 10);
            if (max_lines == 0) max_lines = 1;
        } else {
            fprintf(stderr, "usage: %s --seed N | --sweep A B [--ops K]\n", argv[0]);
            return 2;
        }
    }
    if (!sweep && !have_seed) { seed_lo = 1; seed_hi = 200; sweep = 1; } /* default sweep */
    if (seed_hi < seed_lo) { uint64_t t = seed_lo; seed_lo = seed_hi; seed_hi = t; }

    uint64_t count = 0;
    for (uint64_t s = seed_lo; s <= seed_hi; s++) {
        int rc = run_seed(s, max_lines);
        if (rc != 0) {
            return rc;
        }
        count++;
        if (s == seed_hi) break; /* guard against uint64 wrap at the top */
    }

    printf("test_config_sys_fuzz: %llu seeds, %u lines max each, ALL invariants held\n",
           (unsigned long long)count, max_lines);
    return 0;
}
