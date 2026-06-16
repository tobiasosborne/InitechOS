/* test_config_sys.c -- host unit oracle for the CONFIG.SYS parser (beads
 * initech-509.2). Factory test: libc OK, reuses seed/test_assert.h. Compiles
 * HOSTED against the REAL artifact config_sys.c (the same code SYSINIT runs;
 * config_sys.c compiles both freestanding and hosted).
 *
 * Ref: spec/dos_config_sys_baseline.txt (the LOCKED baseline, Rule 8 -- passed
 *      in via -DCONFIG_SYS_BASELINE_PATH so the SPEC FILE itself is the contract:
 *      edit the locked file and this oracle re-reads it); ADR-0003 Sec 5.6 /
 *      Appendix D.2; os/milton/config_sys.h. CLAUDE.md Law 2 (oracle is truth),
 *      Rule 1 (RED->GREEN), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Strategy:
 *   1. Read the LOCKED baseline file and assert EVERY field: files==20,
 *      buffers==20, lastdrive=='Z', devices contain ANSI.SYS+INITNET.SYS, install
 *      contains SHARE.EXE, shell=="COMMAND.COM /P /E:512".
 *   2. Edge cases against inline buffers: blank lines, ';'-comments, an unknown
 *      directive (ignored, NOT fatal), lowercase keywords, FILES out-of-range
 *      clamped, CRLF line endings, a missing/empty buffer -> all-absent.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DCONFIG_MUTATE_FILES_OFFBYONE : the parser adds 1 to the FILES value -> the
 *                                    files==20 assertion goes RED.
 *   -DCONFIG_MUTATE_FAIL_ON_UNKNOWN: the parser treats an unknown directive as
 *                                    fatal (returns -1 / stops) -> the "unknown
 *                                    ignored" + baseline-recognized assertions go RED.
 * A mutant that PASSES means the oracle is decoration.
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "config_sys.h"
#include "test_assert.h"

TEST_HARNESS();

#ifndef CONFIG_SYS_BASELINE_PATH
#define CONFIG_SYS_BASELINE_PATH "spec/dos_config_sys_baseline.txt"
#endif

/* Does dos_config_t carry a device/install name equal to `want`? */
static int has_device(const dos_config_t *c, const char *want)
{
    for (uint8_t i = 0; i < c->device_count; i++) {
        if (strcmp(c->devices[i], want) == 0) {
            return 1;
        }
    }
    return 0;
}
static int has_install(const dos_config_t *c, const char *want)
{
    for (uint8_t i = 0; i < c->install_count; i++) {
        if (strcmp(c->install[i], want) == 0) {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    /* ================= the LOCKED baseline (the contract) ================= */
    {
        FILE *f = fopen(CONFIG_SYS_BASELINE_PATH, "rb");
        if (f == 0) {
            fprintf(stderr, "  FAIL: cannot open locked baseline %s\n",
                    CONFIG_SYS_BASELINE_PATH);
            return 1;
        }
        char raw[4096];
        size_t n = fread(raw, 1, sizeof(raw), f);
        fclose(f);

        dos_config_t cfg;
        int recognized = config_sys_parse(raw, (uint32_t)n, &cfg);

        /* The baseline has SEVEN honored directives (FILES, BUFFERS, LASTDRIVE,
         * DEVICE x2, INSTALL, SHELL). */
        CHECK(recognized == 7, "baseline: 7 directives recognized");

        CHECK(cfg.files_present && cfg.files == 20u, "baseline FILES=20");
        CHECK(cfg.buffers_present && cfg.buffers == 20u, "baseline BUFFERS=20");
        CHECK(cfg.lastdrive_present && cfg.lastdrive == 'Z', "baseline LASTDRIVE=Z");

        CHECK(cfg.device_count == 2u, "baseline: two DEVICE= lines");
        CHECK(has_device(&cfg, "ANSI.SYS"), "baseline devices include ANSI.SYS");
        CHECK(has_device(&cfg, "INITNET.SYS"), "baseline devices include INITNET.SYS");

        CHECK(cfg.install_count == 1u, "baseline: one INSTALL= line");
        CHECK(has_install(&cfg, "SHARE.EXE"), "baseline install includes SHARE.EXE");

        CHECK(cfg.shell_present, "baseline SHELL= present");
        CHECK_STR_EQ(cfg.shell, "COMMAND.COM /P /E:512",
                     "baseline SHELL tail = COMMAND.COM /P /E:512");
    }

    /* ================= blank lines / comments / unknown =================== */
    {
        static const char src[] =
            "\n"
            "   \n"
            "; this is a comment line\n"
            "FILES=12\n"
            "REM nonsense without an equals\n"      /* no '=' -> unknown, skipped */
            "FOOBAR=whatever\n"                      /* unknown directive, skipped */
            "BUFFERS=30\n";
        dos_config_t cfg;
        int recognized = config_sys_parse(src, (uint32_t)(sizeof(src) - 1), &cfg);

        /* Only FILES + BUFFERS are honored; the comment/blank/unknown lines do
         * NOT abort the parse and are NOT counted. */
        CHECK(recognized == 2, "blank/comment/unknown: only FILES+BUFFERS recognized");
        CHECK(cfg.files_present && cfg.files == 12u, "FILES=12 parsed past blanks/comments");
        CHECK(cfg.buffers_present && cfg.buffers == 30u, "BUFFERS=30 parsed after an unknown line");
        CHECK(!cfg.lastdrive_present, "no LASTDRIVE -> absent");
        CHECK(!cfg.shell_present, "no SHELL -> absent");
        CHECK(cfg.device_count == 0u, "no DEVICE -> none");
    }

    /* ================= case-insensitive keywords ========================= */
    {
        static const char src[] =
            "files=15\n"
            "LastDrive=m\n"
            "device=mouse.sys\n"
            "Shell=command.com /p\n";
        dos_config_t cfg;
        config_sys_parse(src, (uint32_t)(sizeof(src) - 1), &cfg);

        CHECK(cfg.files_present && cfg.files == 15u, "lowercase 'files=' honored");
        CHECK(cfg.lastdrive_present && cfg.lastdrive == 'M', "mixed-case LASTDRIVE uppercased to M");
        CHECK(cfg.device_count == 1u && has_device(&cfg, "MOUSE.SYS") == 0
              && strcmp(cfg.devices[0], "mouse.sys") == 0,
              "device name preserved verbatim (case as written)");
        CHECK_STR_EQ(cfg.shell, "command.com /p", "lowercase SHELL tail preserved");
    }

    /* ================= numeric range clamping ============================ */
    {
        static const char src_lo[] = "FILES=2\n";   /* below min 8  -> clamp up */
        static const char src_hi[] = "FILES=9999\n"; /* above max 255 -> clamp dn */
        dos_config_t lo, hi;
        config_sys_parse(src_lo, (uint32_t)(sizeof(src_lo) - 1), &lo);
        config_sys_parse(src_hi, (uint32_t)(sizeof(src_hi) - 1), &hi);
        CHECK(lo.files_present && lo.files == CONFIG_SYS_FILES_MIN,
              "FILES=2 clamped UP to the min (8)");
        CHECK(hi.files_present && hi.files == CONFIG_SYS_FILES_MAX,
              "FILES=9999 clamped DOWN to the max (255)");
    }

    /* ================= CRLF line endings ================================= */
    {
        static const char src[] = "FILES=20\r\nLASTDRIVE=Z\r\n";
        dos_config_t cfg;
        config_sys_parse(src, (uint32_t)(sizeof(src) - 1), &cfg);
        CHECK(cfg.files_present && cfg.files == 20u, "CRLF: FILES=20 parsed");
        CHECK(cfg.lastdrive_present && cfg.lastdrive == 'Z', "CRLF: LASTDRIVE=Z parsed");
    }

    /* ================= BREAK=ON|OFF (beads initech-er3h; DEC-16) ========== *
     * The CTRL-BREAK directive SYSINIT flows into g_break_flag. ON/OFF are the
     * only well-formed values (case-insensitive); a malformed value is ignored
     * (lenient), leaving break_present == 0 so the boot default ON stands. */
    {
        dos_config_t on, off, lo, bad, none;
        static const char s_on[]  = "BREAK=ON\n";
        static const char s_off[] = "BREAK=OFF\n";
        static const char s_lo[]  = "break=off\n";   /* case-insensitive keyword + value */
        static const char s_bad[] = "BREAK=MAYBE\n";  /* malformed -> ignored (lenient) */
        static const char s_none[]= "FILES=20\n";     /* no BREAK= line */
        config_sys_parse(s_on,  (uint32_t)(sizeof(s_on)  - 1), &on);
        config_sys_parse(s_off, (uint32_t)(sizeof(s_off) - 1), &off);
        config_sys_parse(s_lo,  (uint32_t)(sizeof(s_lo)  - 1), &lo);
        config_sys_parse(s_bad, (uint32_t)(sizeof(s_bad) - 1), &bad);
        config_sys_parse(s_none,(uint32_t)(sizeof(s_none)- 1), &none);
        CHECK(on.break_present && on.break_on == 1u, "BREAK=ON -> present + break_on==1");
        CHECK(off.break_present && off.break_on == 0u, "BREAK=OFF -> present + break_on==0");
        CHECK(lo.break_present && lo.break_on == 0u, "break=off (lowercase) -> present + break_on==0");
        CHECK(!bad.break_present, "BREAK=MAYBE (malformed) -> NOT present (lenient, default stands)");
        CHECK(!none.break_present, "no BREAK= line -> break_present==0 (boot default ON stands)");
    }

    /* ================= empty / missing input ============================= */
    {
        dos_config_t cfg;
        CHECK(config_sys_parse((const char *)0, 0u, &cfg) == 0,
              "NULL buffer -> 0 recognized");
        CHECK(!cfg.files_present && !cfg.shell_present && cfg.device_count == 0u,
              "NULL buffer -> all directives absent (the baseline fallback)");

        static const char empty[] = "";
        CHECK(config_sys_parse(empty, 0u, &cfg) == 0, "zero-length -> 0 recognized");
    }

    return TEST_SUMMARY("test_config_sys");
}
