/*
 * harness/diff/dbf_diff/test_interp_setfmt.c -- host oracle for the SET
 * DATE / SET CENTURY formatter wiring in fn_builtins.c, and for proving that
 * STR() is UNAFFECTED by SET DECIMALS (the verified contract).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK here. Uses the seed
 * test_assert.h harness (CHECK / TEST_HARNESS / TEST_SUMMARY). A non-zero exit
 * on any failed check ensures make gates cannot false-green (Law 2).
 *
 * WHAT THIS PROVES:
 *   1. DEFAULT behavior (no SET called): DTOC/CTOD output is identical to
 *      the III+ defaults (AMERICAN mm/dd/yy, CENTURY OFF).
 *   2. STR() ALWAYS uses dec=0 regardless of SET DECIMALS (the verified contract).
 *      STR(3.14159) == "         3" (width=10, dec=0) even after SET DECIMALS TO 4.
 *      STR(-570,4)  == "-570"       (2-arg, dec=0) -- confirms the no-dec form.
 *      [verified: ../dbase3-decomp/specs/runtime/numeric-and-string-formatting.md
 *       lines 11-13 "default decimals 0"; line 33 "SET DECIMALS scope is
 *       division/SQRT/LOG/VAL/EXP -- does NOT affect STR";
 *       re/mint-results-001.md STR(-570,4)='-570']
 *   3. SET CENTURY ON -> DTOC(date) emits 10-char 4-digit year.
 *      SET CENTURY OFF -> reverts to 8-char 2-digit year.
 *   4. SET DATE ANSI -> DTOC outputs YY.MM.DD; CTOD parses YY.MM.DD.
 *      SET DATE AMERICAN -> reverts to MM/DD/YY.
 *   5. SET DATE BRITISH  -> DTOC outputs DD/MM/YY; CTOD parses DD/MM/YY.
 *   6. SET DATE GERMAN   -> DTOC outputs DD.MM.YY; CTOD parses DD.MM.YY.
 *   7. SET DATE JAPAN    -> DTOC outputs YY/MM/DD; CTOD parses YY/MM/DD.
 *   8. SET DATE USA      -> DTOC outputs MM-DD-YY; CTOD parses MM-DD-YY.
 *   9. SET DATE ANSI + SET CENTURY ON -> DTOC outputs YYYY.MM.DD (10 chars).
 *  10. GATED behaviors: loud-skip with fprintf(stderr, "GATE: ...").
 *
 * MUTATION PROOF (Rule 6 / CLAUDE.md Rule 6):
 *   Build with -DXB_MUTATE_SETFMT_IGNORE: fn_builtins.c forces AMERICAN +
 *   CENTURY OFF in fn_ctod_impl/fn_dtoc_impl regardless of ctx state.
 *   After SET CENTURY ON, DTOC still returns 8-char year; after SET DATE ANSI,
 *   DTOC still returns MM/DD/YY. The SET DATE/CENTURY assertions go RED,
 *   proving the oracle catches missing ctx reads in the date formatter.
 *   A non-zero exit is EXPECTED from the mutant build.
 *   NOTE: STR assertions are NOT part of the mutant signal (STR correctly uses
 *   dec=0 always and reads no ctx state).
 *
 * GATED / LOUD-SKIP:
 *   - SET DATE ITALIAN / FRENCH: both produce DD/MM/YY or DD-MM-YY and have no
 *     minted ground truth distinguishing ITALIAN from BRITISH or FRENCH from
 *     BRITISH. Loud-skipped with GATE note.
 *   - CTOD with 4-digit year strings under CENTURY OFF: acceptance/rejection
 *     is [oracle-resolves] (dates-and-century.md Open Question). Loud-skipped.
 *   - SET DECIMALS effect on division/SQRT/LOG/VAL/computed-display: the
 *     verified scope (numeric-and-string-formatting.md line 33) is a SEPARATE
 *     follow-up bead. Loud-skipped here with a GATE note.
 *
 * Goldens base resolves from argv[1] (orchestrator passes corpus path);
 * default "../dbase3-decomp".
 *
 * ASCII-clean (Rule 12). No timestamps / host paths baked in (Rule 11).
 * Freestanding target code (fn_builtins.c) is tested via host harness (Law 3).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/runtime/numeric-and-string-formatting.md lines 11-13
 *     (STR default decimals=0 [verified]); line 33 (SET DECIMALS scope =
 *     division/SQRT/LOG/VAL/EXP; does NOT include STR [verified]).
 *   - ../dbase3-decomp/re/mint-results-001.md (STR(-570,4)='-570' [verified],
 *     confirming 0-dec form on real III+).
 *   - ../dbase3-decomp/specs/commands/set-commands.md Sec 3.2 (DATE format table),
 *     Sec 2 (CENTURY default OFF).
 *   - ../dbase3-decomp/re/mint-results-003.md (DATE=AMERICAN, CENTURY=OFF [verified]).
 *   - ../dbase3-decomp/specs/runtime/dates-and-century.md (format table; base-1900;
 *     blank-date = spaces; CENTURY ON width 10).
 *   - os/samir/include/samir/eval.h  (xb_ctx.set_decimals/set_date_fmt/set_century;
 *     XB_DATE_* constants).
 *   - os/samir/include/samir/set.h   (set_register; set_date_fmt enum).
 *   - os/samir/cmd/set.c             (do_set_decimals/date/century write ctx fields).
 *   - os/samir/core/fn_builtins.c    (fn_dtoc_impl/fn_ctod_impl ctx reads; fn_str
 *     uses dec=0 unconditionally and reads no ctx state).
 *   - seed/test_assert.h (harness idiom: CHECK / TEST_HARNESS / TEST_SUMMARY).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_assert.h"          /* seed/, on -Iseed */
#include "samir/set.h"            /* os/samir/include/ */
#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/dbf.h"
#include "samir/ndx.h"

TEST_HARNESS();

/* pal_host.c surface. */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* =====================================================================
 * Helpers
 * ===================================================================== */

/*
 * eval_char_via_do: run a "? <expr>" command through samir_do and capture the
 * last Character result via xb_interp_eval_str. Returns the result in buf[cap]
 * NUL-terminated. Returns the xb_val length, or -1 on error.
 *
 * We use xb_interp_eval_str directly (bypassing the ? display path) to get the
 * raw xb_val without involving the REPL output layer. This is the correct
 * approach for a formatter oracle: we want the xb_val bytes, not what the REPL
 * might do to them.
 */
static int eval_char(xb_interp *ip, const char *expr, char *buf, int cap)
{
    xb_val   out;
    int      ec = 0;
    uint32_t len = (uint32_t)strlen(expr);
    int rc = xb_interp_eval_str(ip, expr, len, &out, &ec);
    if (rc != 0 || out.t != XB_C) return -1;
    if ((int)out.u.c.len >= cap) return -1;
    memcpy(buf, out.u.c.p, out.u.c.len);
    buf[out.u.c.len] = '\0';
    return (int)out.u.c.len;
}

/* eval_date: evaluate an expression returning a Date; returns JDN as int32,
 * or -1 on error. */
static int eval_date(xb_interp *ip, const char *expr)
{
    xb_val   out;
    int      ec = 0;
    uint32_t len = (uint32_t)strlen(expr);
    int rc = xb_interp_eval_str(ip, expr, len, &out, &ec);
    if (rc != 0 || out.t != XB_D) return -1;
    return (int)out.u.d;
}

/* do_set: run a SET command through samir_do; return samir_do rc. */
static int do_set(xb_interp *ip, const char *cmd)
{
    return samir_do(ip, cmd);
}

/* =====================================================================
 * Tier 0: pure formatter oracle (no corpus goldens needed).
 *
 * All assertions grounded against:
 *   - set-commands.md Sec 3.1/3.2 (DECIMALS/DATE format table; defaults)
 *   - mint-results-002.md (DECIMALS=2 default [verified])
 *   - mint-results-003.md (DATE=AMERICAN, CENTURY=OFF [verified])
 *   - dates-and-century.md (format widths; blank-date rule; base-1900)
 * ===================================================================== */

static int run_setfmt_tests(const char *corpus_path)
{
    struct pal_host_cfg cfg = { 85, 8, 5, 512*1024 };
    samir_pal_t  *pal = pal_host_make(cfg);
    xb_interp    *ip  = xb_interp_make(pal);
    char          buf[32];
    int           len, jdn;
    int           rc;

    /* JDN for 1985-08-05 (the corpus TOURS date; verified: rt.c + ndx.md).
     * jdn_from_ymd(1985,8,5) = 2446283. Used for all format tests -- this date
     * is representable as a 2-digit year (85) under CENTURY OFF. */
    const int jdn_850805 = 2446283;

    (void)corpus_path;   /* Tier 0 needs no corpus */

    if (!ip) {
        fprintf(stderr, "  FAIL: xb_interp_make returned NULL\n");
        g_fails++;
        pal_host_free(pal);
        return 1;
    }

    /* Register SET hooks (applies III+ defaults to ctx). */
    rc = set_register(ip);
    CHECK(rc == INTERP_OK, "set_register returned INTERP_OK");

    /* ------------------------------------------------------------------ */
    /* 1. DEFAULT BEHAVIOR (no SET commands issued yet).                   */
    /* Must produce identical output to the hardcoded-default era.         */
    /* [verified: mint-results-002.md DECIMALS=2; mint-results-003.md      */
    /*  DATE=AMERICAN, CENTURY=OFF]                                        */
    /* ------------------------------------------------------------------ */

    /* 1a. DTOC defaults: AMERICAN mm/dd/yy, 8 chars.
     * DTOC(CTOD('08/05/85')) -> "08/05/85".
     * Ref: dates-and-century.md AMERICAN + CENTURY OFF -> MM/DD/YY (8). */
    len = eval_char(ip, "DTOC(CTOD('08/05/85'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "08/05/85") == 0,
          "default: DTOC(CTOD('08/05/85')) == '08/05/85' (AMERICAN 8-char)");

    /* 1b. CTOD default: parse AMERICAN format.
     * CTOD('08/05/85') -> JDN 2446283.
     * [verified: rt.c JDN(1985-08-05)=2446283] */
    jdn = eval_date(ip, "CTOD('08/05/85')");
    CHECK(jdn == jdn_850805,
          "default: CTOD('08/05/85') == JDN 2446283 (1985-08-05)");

    /* 1c. DTOC(blank) -> 8 spaces.
     * Ref: dates-and-century.md "blank date -> (width) spaces". */
    len = eval_char(ip, "DTOC(CTOD('  /  /  '))", buf, (int)sizeof(buf));
    CHECK(len == 8 && memcmp(buf, "        ", 8) == 0,
          "default: DTOC(blank) -> 8 spaces (CENTURY OFF width=8)");

    /* 1d. STR(3.14159) 1-arg: dec=0 ALWAYS (STR is NOT governed by SET DECIMALS).
     * Result: "         3" (width=10, 0 decimals, right-justified).
     * Ref [verified]: numeric-and-string-formatting.md:11-13 "STR() ... default
     *   width 10, default decimals 0"; line 33 "SET DECIMALS scope is division/
     *   SQRT/LOG/VAL/computed-display -- NOT STR()"; mint-results-001.md
     *   STR(-570,4)='-570' (the no-dec form -> 0 decimals). */
    len = eval_char(ip, "STR(3.14159)", buf, (int)sizeof(buf));
    CHECK(len == 10 && strcmp(buf, "         3") == 0,
          "default: STR(3.14159) == '         3' (width=10, dec=0; STR ignores SET DECIMALS)");

    /* 1e. STR(42, 4) 2-arg form: dec=0 always (no explicit decimals -> 0).
     * Result: "  42" (4-char, integer).
     * Ref: numeric-and-string-formatting.md:11-13; mint-001 STR(-570,4)='-570'. */
    len = eval_char(ip, "STR(42,4)", buf, (int)sizeof(buf));
    CHECK(len == 4 && strcmp(buf, "  42") == 0,
          "default: STR(42,4) == '  42' (2-arg: dec=0, unaffected by SET DECIMALS)");

    /* ------------------------------------------------------------------ */
    /* 2. SET DECIMALS is STORED but does NOT affect STR() (verified).     */
    /* Its real scope (division/SQRT/LOG/VAL/computed-display) is a        */
    /* separate, partly-GATED step -- loud-skipped here.                   */
    /* [verified: numeric-and-string-formatting.md:33]                     */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DECIMALS TO 4");
    CHECK(rc == 0, "SET DECIMALS TO 4: samir_do returned 0");
    CHECK(set_get_decimals(ip) == 4, "SET DECIMALS TO 4: getter == 4 (stored)");

    /* 2a. STR(3.14159) is UNCHANGED by SET DECIMALS TO 4: still '         3'
     * (dec=0). This is THE verified behavior -- STR ignores SET DECIMALS.
     * Ref: numeric-and-string-formatting.md:11-13 + :33. */
    len = eval_char(ip, "STR(3.14159)", buf, (int)sizeof(buf));
    CHECK(len == 10 && strcmp(buf, "         3") == 0,
          "SET DECIMALS 4: STR(3.14159) STILL == '         3' (STR ignores SET DECIMALS)");

    /* 2b. STR(42, 4) 2-arg: dec=0 (unchanged). */
    len = eval_char(ip, "STR(42,4)", buf, (int)sizeof(buf));
    CHECK(len == 4 && strcmp(buf, "  42") == 0,
          "SET DECIMALS 4: STR(42,4) == '  42' (2-arg, dec=0)");

    /* 2c. GATED: the verified SET DECIMALS scope (division/SQRT/LOG/VAL/
     * computed-display) is a separate step; loud-skip its effect here. */
    fprintf(stderr, "  GATE: SET DECIMALS effect on division/VAL/computed display "
            "is a separate step (numeric-and-string-formatting.md:33); not asserted.\n");

    rc = do_set(ip, "SET DECIMALS TO 0");
    CHECK(rc == 0, "SET DECIMALS TO 0: samir_do returned 0");
    CHECK(set_get_decimals(ip) == 0, "SET DECIMALS TO 0: getter == 0 (stored)");

    /* 3a. STR(3.14159) still '         3' (dec=0) -- STR never varied. */
    len = eval_char(ip, "STR(3.14159)", buf, (int)sizeof(buf));
    CHECK(len == 10 && strcmp(buf, "         3") == 0,
          "SET DECIMALS 0: STR(3.14159) == '         3' (STR always dec=0)");

    /* ------------------------------------------------------------------ */
    /* 4. SET CENTURY ON -> DTOC emits 10-char 4-digit year.               */
    /* [verified: set-commands.md Sec 2 + dates-and-century.md format table */
    /*  "CENTURY ON: 4-digit year, width=10"]                              */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET CENTURY ON");
    CHECK(rc == 0, "SET CENTURY ON: samir_do returned 0");
    CHECK(set_get_century(ip) == 1, "SET CENTURY ON: getter == 1");

    /* 4a. DTOC after SET CENTURY ON: AMERICAN 10-char MM/DD/YYYY.
     * Date 1985-08-05 -> "08/05/1985".
     * Ref: dates-and-century.md AMERICAN + CENTURY ON -> MM/DD/YYYY (10). */
    len = eval_char(ip, "DTOC(CTOD('08/05/85'))", buf, (int)sizeof(buf));
    CHECK(len == 10 && strcmp(buf, "08/05/1985") == 0,
          "SET CENTURY ON: DTOC(date) == '08/05/1985' (AMERICAN 10-char) [MUTANT]");

    /* 4b. DTOC(blank) after CENTURY ON -> 10 spaces.
     * Ref: dates-and-century.md "blank date -> (width) spaces". */
    len = eval_char(ip, "DTOC(CTOD('  /  /  '))", buf, (int)sizeof(buf));
    CHECK(len == 10 && memcmp(buf, "          ", 10) == 0,
          "SET CENTURY ON: DTOC(blank) -> 10 spaces (CENTURY ON width=10)");

    /* 4c. SET CENTURY OFF restores 8-char output. */
    rc = do_set(ip, "SET CENTURY OFF");
    CHECK(rc == 0, "SET CENTURY OFF: samir_do returned 0");

    len = eval_char(ip, "DTOC(CTOD('08/05/85'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "08/05/85") == 0,
          "SET CENTURY OFF (restored): DTOC == '08/05/85' (8-char) [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* 5. SET DATE ANSI -> DTOC outputs YY.MM.DD; CTOD parses YY.MM.DD.   */
    /* [verified: set-commands.md Sec 3.2 ANSI -> YY.MM.DD]               */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DATE ANSI");
    CHECK(rc == 0, "SET DATE ANSI: samir_do returned 0");
    CHECK(set_get_date_fmt(ip) == SET_DATE_ANSI, "SET DATE ANSI: getter == SET_DATE_ANSI");

    /* 5a. DTOC under ANSI: 1985-08-05 -> "85.08.05".
     * Ref: set-commands.md Sec 3.2 ANSI = YY.MM.DD (CENTURY OFF). */
    len = eval_char(ip, "DTOC(CTOD('08/05/85'))", buf, (int)sizeof(buf));
    /* NOTE: CTOD('08/05/85') was parsed as AMERICAN (8,5,85); the JDN is
     * now rendered with the ANSI format. We use the stored JDN directly. */
    {
        xb_val dv; int ec = 0;
        char dtoc_in[32];
        /* Re-CTOD from ANSI format so CTOD uses the current parser. */
        /* Under ANSI, CTOD expects YY.MM.DD. So '85.08.05'. */
        (void)dv; (void)ec;
        len = eval_char(ip, "DTOC(CTOD('85.08.05'))", buf, (int)sizeof(buf));
        CHECK(len == 8 && strcmp(buf, "85.08.05") == 0,
              "SET DATE ANSI: DTOC(CTOD('85.08.05')) == '85.08.05' (YY.MM.DD) [MUTANT]");
        (void)dtoc_in;
    }

    /* 5b. CTOD under ANSI: parse '85.08.05' -> JDN 2446283 (1985-08-05).
     * Ref: set-commands.md Sec 3.2 ANSI = YY.MM.DD; base-1900 2-digit year. */
    jdn = eval_date(ip, "CTOD('85.08.05')");
    CHECK(jdn == jdn_850805,
          "SET DATE ANSI: CTOD('85.08.05') == JDN 2446283 (1985-08-05) [MUTANT]");

    /* 5c. CTOD of AMERICAN format under ANSI: digits 08,05,85 -> ANSI interprets
     * as YY=08, MM=05, DD=85 -> DD=85 out-of-range -> blank date.
     * Proves format switching is live (not hardcoded).
     * [Ref: dates-and-century.md "invalid day -> blank date"] */
    jdn = eval_date(ip, "CTOD('08/05/85')");
    CHECK(jdn == 0,
          "SET DATE ANSI: CTOD('08/05/85') with ANSI parser -> blank (invalid DD=85) [MUTANT]");

    /* 5d. SET DATE ANSI + SET CENTURY ON -> DTOC outputs YYYY.MM.DD (10 chars).
     * Ref: set-commands.md Sec 3.2 ANSI CENTURY ON = YYYY.MM.DD (10). */
    rc = do_set(ip, "SET CENTURY ON");
    CHECK(rc == 0, "SET DATE ANSI + SET CENTURY ON");

    len = eval_char(ip, "DTOC(CTOD('1985.08.05'))", buf, (int)sizeof(buf));
    CHECK(len == 10 && strcmp(buf, "1985.08.05") == 0,
          "SET DATE ANSI + CENTURY ON: DTOC(CTOD('1985.08.05')) == '1985.08.05' [MUTANT]");

    rc = do_set(ip, "SET CENTURY OFF");
    CHECK(rc == 0, "restore SET CENTURY OFF");

    /* ------------------------------------------------------------------ */
    /* 6. SET DATE BRITISH -> DD/MM/YY                                     */
    /* [verified: set-commands.md Sec 3.2 BRITISH = DD/MM/YY]             */
    /* Use 1985-08-05 (jdn_850805=2446283); 2-digit year 85 fits CENTURY OFF. */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DATE BRITISH");
    CHECK(rc == 0, "SET DATE BRITISH: samir_do returned 0");

    /* 6a. CTOD under BRITISH: parse DD/MM/YY. '05/08/85' -> 1985-08-05.
     * Ref: set-commands.md Sec 3.2 BRITISH = DD/MM/YY; base-1900 2-digit year. */
    jdn = eval_date(ip, "CTOD('05/08/85')");
    CHECK(jdn == jdn_850805,
          "SET DATE BRITISH: CTOD('05/08/85') == JDN 2446283 (1985-08-05) [MUTANT]");

    /* 6b. DTOC: 1985-08-05 -> "05/08/85" (DD/MM/YY). */
    len = eval_char(ip, "DTOC(CTOD('05/08/85'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "05/08/85") == 0,
          "SET DATE BRITISH: DTOC(date) == '05/08/85' (DD/MM/YY) [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* 7. SET DATE GERMAN -> DD.MM.YY                                      */
    /* [verified: set-commands.md Sec 3.2 GERMAN = DD.MM.YY]              */
    /* Use 1985-08-05 (jdn_850805=2446283); 2-digit year 85 fits CENTURY OFF. */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DATE GERMAN");
    CHECK(rc == 0, "SET DATE GERMAN: samir_do returned 0");

    /* 7a. CTOD: '05.08.85' -> 1985-08-05. */
    jdn = eval_date(ip, "CTOD('05.08.85')");
    CHECK(jdn == jdn_850805,
          "SET DATE GERMAN: CTOD('05.08.85') == JDN 2446283 (1985-08-05) [MUTANT]");

    /* 7b. DTOC: 1985-08-05 -> "05.08.85". */
    len = eval_char(ip, "DTOC(CTOD('05.08.85'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "05.08.85") == 0,
          "SET DATE GERMAN: DTOC(date) == '05.08.85' (DD.MM.YY) [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* 8. SET DATE JAPAN -> YY/MM/DD                                       */
    /* [verified: set-commands.md Sec 3.2 JAPAN = YY/MM/DD]               */
    /* Use 1985-08-05 (jdn_850805=2446283); JAPAN year-first: '85/08/05'. */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DATE JAPAN");
    CHECK(rc == 0, "SET DATE JAPAN: samir_do returned 0");

    /* 8a. CTOD: '85/08/05' -> 1985-08-05 (JAPAN: YY/MM/DD, year-first). */
    jdn = eval_date(ip, "CTOD('85/08/05')");
    CHECK(jdn == jdn_850805,
          "SET DATE JAPAN: CTOD('85/08/05') == JDN 2446283 (1985-08-05) [MUTANT]");

    /* 8b. DTOC: 1985-08-05 -> "85/08/05". */
    len = eval_char(ip, "DTOC(CTOD('85/08/05'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "85/08/05") == 0,
          "SET DATE JAPAN: DTOC(date) == '85/08/05' (YY/MM/DD) [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* 9. SET DATE USA -> MM-DD-YY                                         */
    /* [verified: set-commands.md Sec 3.2 USA = MM-DD-YY]                 */
    /* Use 1985-08-05 (jdn_850805=2446283); USA: '08-05-85'. */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DATE USA");
    CHECK(rc == 0, "SET DATE USA: samir_do returned 0");

    /* 9a. CTOD: '08-05-85' -> 1985-08-05 (USA: MM-DD-YY). */
    jdn = eval_date(ip, "CTOD('08-05-85')");
    CHECK(jdn == jdn_850805,
          "SET DATE USA: CTOD('08-05-85') == JDN 2446283 (1985-08-05) [MUTANT]");

    /* 9b. DTOC: 1985-08-05 -> "08-05-85". */
    len = eval_char(ip, "DTOC(CTOD('08-05-85'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "08-05-85") == 0,
          "SET DATE USA: DTOC(date) == '08-05-85' (MM-DD-YY) [MUTANT]");

    /* ------------------------------------------------------------------ */
    /* Restore AMERICAN (the III+ default) before final checks.            */
    /* ------------------------------------------------------------------ */

    rc = do_set(ip, "SET DATE TO AMERICAN");
    CHECK(rc == 0, "restore SET DATE AMERICAN: samir_do returned 0");
    rc = do_set(ip, "SET DECIMALS TO 2");
    CHECK(rc == 0, "restore SET DECIMALS TO 2");
    rc = do_set(ip, "SET CENTURY OFF");
    CHECK(rc == 0, "restore SET CENTURY OFF");

    /* Verify restored defaults. */
    len = eval_char(ip, "DTOC(CTOD('08/05/85'))", buf, (int)sizeof(buf));
    CHECK(len == 8 && strcmp(buf, "08/05/85") == 0,
          "restored defaults: DTOC(CTOD('08/05/85')) == '08/05/85'");

    len = eval_char(ip, "STR(3.14159)", buf, (int)sizeof(buf));
    CHECK(len == 10 && strcmp(buf, "         3") == 0,
          "restored defaults: STR(3.14159) == '         3' (STR always dec=0)");

    /* ------------------------------------------------------------------ */
    /* GATED: behaviors that are [oracle-resolves] or not grounded.        */
    /* ------------------------------------------------------------------ */

    fprintf(stderr, "GATE: SET DATE ITALIAN: picture DD-MM-YY is same as GERMAN "
            "but with '/' separator confusion -- exact minted behavior "
            "[oracle-resolves]. Loud-skipping ITALIAN/FRENCH format assertions.\n");

    fprintf(stderr, "GATE: CTOD with 4-digit year string under CENTURY OFF: "
            "acceptance behavior of 10-char AMERICAN string ('08/05/1985') "
            "under CENTURY OFF is [oracle-resolves] per dates-and-century.md "
            "Open Question. Loud-skipping.\n");

    fprintf(stderr, "GATE: STR(n) 2-arg form SET DECIMALS interaction: "
            "2-arg uses dec=0 (settled design choice; see set-commands.md Sec 3.1). "
            "No oracle-resolves needed -- already asserted above.\n");

    /* ------------------------------------------------------------------ */
    /* Teardown.                                                            */
    /* ------------------------------------------------------------------ */
    xb_interp_free(ip);
    pal_host_free(pal);
    return 0;
}

/* =====================================================================
 * main
 * ===================================================================== */

int main(int argc, char **argv)
{
    const char *corpus = (argc > 1) ? argv[1] : "../dbase3-decomp";

    printf("test_interp_setfmt: SET DECIMALS/DATE/CENTURY formatter wiring oracle\n");

    run_setfmt_tests(corpus);

    return TEST_SUMMARY("test_interp_setfmt");
}
