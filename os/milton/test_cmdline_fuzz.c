/* test_cmdline_fuzz.c -- deterministic, seeded generative fuzzer for the
 * command-line tokenizer (os/milton/command.c) and the PSP command-tail builder
 * (os/milton/psp.c). beads initech-hjv.
 *
 * FACTORY host tool (CLAUDE.md Law 3): libc OK; the SUBJECTS under test are the
 * UNMODIFIED artifact command.c (cmd_parse, the pure logic) and psp.c (psp_build)
 * -- the SAME code the kernel runs (both TUs compile freestanding AND hosted).
 * command.c is built here WITHOUT -DCOMMAND_KERNEL_REPL so only the pure,
 * asm-free shell logic is linked (the host-testability seam command.h documents).
 *
 * Why (beads initech-hjv): test_command.c and test_psp.c are fixed ENUMERATED
 * matrices -- strong, but they do not explore the deep space of HOSTILE command
 * lines (Rule 3). This is the standing GENERATIVE fuzzer: a splitmix64-seeded
 * PRNG synthesizes adversarial command lines -- leading/trailing/multiple spaces,
 * tabs, embedded quotes, control bytes, and lines FAR longer than the 127-char
 * PSP command tail -- then drives BOTH:
 *
 *   (A) the shell tokenizer cmd_parse(): the command/arg tokens must NEVER
 *       overflow cmd_line_t (a GUARD byte after the struct must survive), the
 *       command word stays <= CMD_TOKEN_MAX-1, the arg <= CMD_LINE_MAX-1, and
 *       both stay NUL-terminated.
 *
 *   (B) the PSP builder psp_build() with the parsed arg AS the command tail,
 *       AND with deliberately OVER-LONG raw tails: the 128-byte cmd_tail region
 *       (PSP offset 0x80) must be CLAMPED -- count byte == min(len, 126), the
 *       text in [1..count], the 0x0D CR at offset count+1 <= 127, NOTHING past
 *       cmd_tail[127] (a GUARD byte immediately after the 256-byte psp_t must
 *       survive), and the returned dropped-count == max(0, len - 126).
 *
 * This is the invariant the -DPSP_MUTATE_NO_TAIL_CLAMP mutant violates: with the
 * clamp removed, an over-long tail copies all `len` bytes + CR past cmd_tail[127],
 * stomping the guard -> the fuzzer goes RED.
 *
 * Determinism (Rule 11): a splitmix64-seeded xorshift128+ PRNG; NO time()/rand().
 * Same --seed => identical command line => identical result; failures REPLAY by
 * seed. The gate runs a fixed seed sweep.
 *
 * Ref (Law 1): os/milton/command.h (CMD_LINE_MAX=128, CMD_TOKEN_MAX=64; the
 *   tokenizer clamp contract); os/milton/psp.h (PSP_CMD_TAIL_MAX_TEXT=126, the
 *   loud clamp returning the dropped count); spec/dos_structs.h (psp_t cmd_tail
 *   @ 0x80, 128 bytes, the LAST field); ADR-0003 App B.2; CLAUDE.md Law 2,
 *   Rule 6 (mutation-prove), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * Exit code: 0 if every seed's every invariant held; non-zero on the FIRST
 * violation (after printing the seed + the offending command line, replayable)
 * so the Makefile gate goes RED (Law 2).
 *
 * Usage:
 *   test_cmdline_fuzz --seed N  [--ops K]
 *   test_cmdline_fuzz --sweep A B [--ops K]
 * (--ops bounds the synthesized command-line length in bytes; default 320 so
 *  >127-char lines are routine.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "command.h"
#include "psp.h"

/* ---- Deterministic PRNG: splitmix64 seed -> xorshift128+ stream (Rule 11). -- */
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

static uint32_t prng_below(prng_t *p, uint32_t n) { return (uint32_t)(prng_next(p) % (uint64_t)n); }

/* ---- Guard brackets (Rule 2): magic bytes that must survive every op. ------ */
#define GUARD_MAGIC 0xA5u
#define LINE_CAP    1024u    /* max synthesized command-line bytes (room for >>127) */

/* A cmd_line_t with a trailing guard byte: cmd_parse must never write past it. */
typedef struct { cmd_line_t parsed; uint8_t guard; } guarded_cmdline_t;

/* A psp_t with a trailing guard byte: psp_build must never write past cmd_tail
 * (the LAST field of psp_t, so the guard sits immediately at offset 256). */
typedef struct { psp_t psp; uint8_t guard; } guarded_psp_t;

/* Synthesize ONE adversarial command line into `line` (a raw byte buffer of
 * `cap`). Returns the byte length (NOT NUL-terminated; raw_len is authoritative
 * for the cmd_tail leg). A separate ASCIIZ copy is built for the cmd_parse leg
 * (cmd_parse takes a C string). Hostile features: leading/trailing/multiple
 * spaces+tabs, quotes, control bytes, and lines FAR exceeding 127 chars. */
static uint32_t generate_line(char *line, uint32_t cap, prng_t *p, uint32_t target)
{
    /* Pool of bytes: lots of whitespace + a command-ish word + quotes + the odd
     * control byte (never NUL -- a NUL would truncate the ASCIIZ cmd_parse leg;
     * NUL handling is covered by the empty-line case below). */
    static const char *const WORDS[] = { "dir", "TYPE", "echo", "greet",
                                         "command.com", "A:\\FOO.COM", "x" };
    uint32_t len = 0;

    /* Leading whitespace run. */
    uint32_t lead = prng_below(p, 6);
    for (uint32_t i = 0; i < lead && len < cap; i++) {
        line[len++] = (prng_next(p) & 1u) ? ' ' : '\t';
    }

    /* Body: keep appending tokens + whitespace until we hit the target length. */
    while (len < target && len < cap - 2u) {
        uint32_t pick = prng_below(p, 10);
        if (pick <= 5) {
            const char *w = WORDS[prng_below(p, 7)];
            for (; *w && len < cap; w++) line[len++] = *w;
        } else if (pick == 6) {
            if (len < cap) line[len++] = '"';   /* a quote */
        } else if (pick == 7) {
            if (len < cap) line[len++] = '\'';  /* a quote */
        } else if (pick == 8) {
            /* a printable punctuation/control-ish byte (never NUL/CR/LF/space) */
            static const char junk[] = "/\\.-_:*?=+@#$%^&!~|<>[]{}";
            if (len < cap) line[len++] = junk[prng_below(p, (uint32_t)(sizeof(junk) - 1))];
        } else {
            /* whitespace run (multiple spaces/tabs) */
            uint32_t n = 1 + prng_below(p, 5);
            for (uint32_t i = 0; i < n && len < cap; i++) {
                line[len++] = (prng_next(p) & 1u) ? ' ' : '\t';
            }
        }
    }

    /* Trailing whitespace run. */
    uint32_t trail = prng_below(p, 6);
    for (uint32_t i = 0; i < trail && len < cap; i++) {
        line[len++] = (prng_next(p) & 1u) ? ' ' : '\t';
    }

    /* Occasionally produce a degenerate line: empty, or all-whitespace. */
    uint32_t deg = prng_below(p, 16);
    if (deg == 0) {
        len = 0;
    } else if (deg == 1) {
        len = 0;
        uint32_t n = prng_below(p, 8);
        for (uint32_t i = 0; i < n && len < cap; i++) {
            line[len++] = (prng_next(p) & 1u) ? ' ' : '\t';
        }
    }
    return len;
}

static int run_seed(uint64_t seed, uint32_t target_len)
{
    prng_t prng;
    prng_seed(&prng, seed);

    static char raw[LINE_CAP];
    uint32_t raw_len = generate_line(raw, LINE_CAP, &prng, target_len);

    /* ASCIIZ copy for the tokenizer leg (cmd_parse takes a C string). */
    static char asciiz[LINE_CAP + 1];
    memcpy(asciiz, raw, raw_len);
    asciiz[raw_len] = '\0';

    int fail = 0;
    const char *why = "";

    /* ---- (A) the shell tokenizer cmd_parse() -------------------------------- */
    guarded_cmdline_t gcl;
    gcl.guard = GUARD_MAGIC;
    /* Poison the struct so we can confirm NUL-termination is written, not lucky. */
    memset(&gcl.parsed, 0x7F, sizeof(gcl.parsed));
    cmd_parse(asciiz, &gcl.parsed);

    if (gcl.guard != GUARD_MAGIC) { fail = 1; why = "cmd_parse wrote past cmd_line_t (guard clobbered)"; }

    /* command word: NUL-terminated within CMD_TOKEN_MAX, length <= CMD_TOKEN_MAX-1. */
    if (!fail) {
        size_t cl = 0;
        while (cl < (size_t)CMD_TOKEN_MAX && gcl.parsed.command[cl] != '\0') cl++;
        if (cl >= (size_t)CMD_TOKEN_MAX) { fail = 1; why = "cmd_parse command word not NUL-terminated in CMD_TOKEN_MAX"; }
        else if (cl > (size_t)(CMD_TOKEN_MAX - 1)) { fail = 1; why = "cmd_parse command word exceeds CMD_TOKEN_MAX-1"; }
    }
    /* arg tail: NUL-terminated within CMD_LINE_MAX, length <= CMD_LINE_MAX-1. */
    if (!fail) {
        size_t al = 0;
        while (al < (size_t)CMD_LINE_MAX && gcl.parsed.arg[al] != '\0') al++;
        if (al >= (size_t)CMD_LINE_MAX) { fail = 1; why = "cmd_parse arg not NUL-terminated in CMD_LINE_MAX"; }
        else if (al > (size_t)(CMD_LINE_MAX - 1)) { fail = 1; why = "cmd_parse arg exceeds CMD_LINE_MAX-1"; }
    }

    /* ---- (B) the PSP command-tail builder psp_build() ----------------------- *
     * Two tails per seed: (b1) the parsed arg (the authentic COMMAND.COM flow:
     * the child PSP carries the argument tail), and (b2) the RAW line itself,
     * which is routinely > 126 bytes so the clamp + no-overflow path is exercised
     * directly. Both must clamp into the 128-byte region with the guard intact. */
    struct { const char *tail; uint32_t len; const char *label; } legs[2] = {
        { gcl.parsed.arg, (uint32_t)strlen(gcl.parsed.arg), "parsed-arg" },
        { raw,            raw_len,                          "raw-line"   },
    };

    for (int li = 0; li < 2 && !fail; li++) {
        guarded_psp_t gp;
        gp.guard = GUARD_MAGIC;

        psp_params_t params;
        params.alloc_end_linear  = 0x00070000u;
        params.env_linear        = 0x00020200u;
        params.parent_psp_linear = 0x00010000u;
        params.cmd_tail          = legs[li].tail;
        params.cmd_tail_len      = legs[li].len;

        uint32_t dropped = psp_build(&gp.psp, &params);

        uint32_t want = legs[li].len;
        uint32_t expect_copy = (want > PSP_CMD_TAIL_MAX_TEXT) ? PSP_CMD_TAIL_MAX_TEXT : want;
        uint32_t expect_drop = (want > PSP_CMD_TAIL_MAX_TEXT) ? (want - PSP_CMD_TAIL_MAX_TEXT) : 0u;

        /* (1) NO overflow: guard byte immediately past the 256-byte psp_t intact. */
        if (gp.guard != GUARD_MAGIC) { fail = 1; why = "psp_build wrote PAST cmd_tail[127] (guard clobbered)"; }

        /* (2) loud clamp: returned dropped count == max(0, len - 126). */
        if (!fail && dropped != expect_drop) { fail = 1; why = "psp_build dropped-count != max(0, len-126)"; }

        /* (3) count byte == clamped copy length (<= 126, fits a byte). */
        if (!fail && gp.psp.cmd_tail[0] != (uint8_t)expect_copy) { fail = 1; why = "cmd_tail[0] count byte != clamped length"; }
        if (!fail && gp.psp.cmd_tail[0] > (uint8_t)PSP_CMD_TAIL_MAX_TEXT) { fail = 1; why = "cmd_tail[0] count exceeds 126"; }

        /* (4) the text bytes match the (clamped) tail, in [1..count]. */
        if (!fail) {
            for (uint32_t i = 0; i < expect_copy; i++) {
                if (gp.psp.cmd_tail[1 + i] != (uint8_t)legs[li].tail[i]) {
                    fail = 1; why = "cmd_tail text byte mismatch"; break;
                }
            }
        }
        /* (5) the CR (0x0D) lands at offset count+1, which must be <= 127. */
        if (!fail) {
            uint32_t cr_off = 1u + expect_copy;
            if (cr_off > 127u) { fail = 1; why = "CR offset would exceed 127 (clamp failed)"; }
            else if (gp.psp.cmd_tail[cr_off] != 0x0Du) { fail = 1; why = "CR (0x0D) not at offset count+1"; }
        }

        if (fail) {
            fprintf(stderr, "  FAIL seed=%llu leg=%s: %s\n",
                    (unsigned long long)seed, legs[li].label, why);
            fprintf(stderr, "  tail_len=%u expect_copy=%u expect_drop=%u dropped=%u count=%u\n",
                    want, expect_copy, expect_drop, dropped, (unsigned)gp.psp.cmd_tail[0]);
        }
    }

    if (fail) {
        fprintf(stderr, "  ---- offending command line (%u bytes; replay: --seed %llu) ----\n",
                raw_len, (unsigned long long)seed);
        /* Render visibly (tabs as \t) so whitespace-only lines are legible. */
        for (uint32_t i = 0; i < raw_len; i++) {
            char c = raw[i];
            if (c == '\t') fputs("\\t", stderr);
            else if (c == ' ') fputc('_', stderr);
            else fputc(c, stderr);
        }
        fprintf(stderr, "\n  ---- end ----\n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    uint64_t seed_lo = 0, seed_hi = 0;
    uint32_t target_len = 320;   /* default > 127 so the clamp path is routine */
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
            target_len = (uint32_t)strtoul(argv[++i], NULL, 10);
            if (target_len == 0) target_len = 1;
            if (target_len > LINE_CAP - 16u) target_len = LINE_CAP - 16u;
        } else {
            fprintf(stderr, "usage: %s --seed N | --sweep A B [--ops K]\n", argv[0]);
            return 2;
        }
    }
    if (!sweep && !have_seed) { seed_lo = 1; seed_hi = 200; sweep = 1; }
    if (seed_hi < seed_lo) { uint64_t t = seed_lo; seed_lo = seed_hi; seed_hi = t; }

    uint64_t count = 0;
    for (uint64_t s = seed_lo; s <= seed_hi; s++) {
        int rc = run_seed(s, target_len);
        if (rc != 0) return rc;
        count++;
        if (s == seed_hi) break;
    }

    printf("test_cmdline_fuzz: %llu seeds, target_len=%u, ALL invariants held "
           "(cmd_parse no-overflow + psp_build clamp/no-overflow)\n",
           (unsigned long long)count, target_len);
    return 0;
}
