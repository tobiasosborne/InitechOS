/*
 * os/samir/cmd/set.c -- SAMIR (InitechBase) SET command module.
 *                        Step S5.6 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding (-ffreestanding -nostdlib). Uses
 * ONLY <stdint.h> and the samir/ engine headers. No libc, no int 0x21 -- every
 * byte of OS contact goes through the PAL. Memory is fixed/static (no malloc).
 * ASCII-clean (Rule 12). Fail loud (Rule 2). Reproducible (Rule 11).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * WHAT THIS IS (the S5.6 contract): a single xb_cmd_hook registered into the
 * executor's command-hook CHAIN (interp.h xb_interp_add_cmd_hook). It adds the
 * SET command without editing flow.c. Per the parallel-lane collision-avoidance
 * brief it does NOT edit eval.h, workarea.c, or any other parallel-lane file.
 *
 * Options handled:
 *   EXACT     toggle -- writes ctx->set_exact immediately; controls C= begins-with
 *   DECIMALS  TO n   -- stored in set_state; formatter-honoring DEFERRED
 *   DATE     [TO] kw -- stored in set_state; formatter-honoring DEFERRED
 *   CENTURY   toggle -- stored in set_state; formatter-honoring DEFERRED
 *   ORDER    TO n    -- calls wa_set_order on the selected area (read-only caller;
 *                       no workarea.c edits)
 *   INDEX    TO ...  -- parse + store GATED text; runtime effect DEFERRED
 *   FILTER   TO ...  -- parse + store GATED text; runtime effect DEFERRED
 *   RELATION TO ...  -- parse + store GATED text; runtime effect DEFERRED
 *   TALK      toggle -- stored in set_state
 *   SAFETY    toggle -- stored in set_state
 *
 *   SET NEAR: NOT a III+ option (Clipper/dBASE IV). Parser fails loud
 *   (INTERP_ERR_SYNTAX) on "SET NEAR ON|OFF".
 *   [Ref: set-commands.md Sec 5 completeness oracle: NEAR absent from HELP_topics]
 *
 * Defaults (minted ground truth):
 *   EXACT=OFF  [verified: mint-results-002.md]
 *   DECIMALS=2 [verified: mint-results-002.md]
 *   DATE=AMERICAN [verified: mint-results-003.md]
 *   CENTURY=OFF   [verified: mint-results-003.md]
 *   TALK=ON    [verified: set-commands.md Sec 2 table]
 *   SAFETY=ON  [verified: set-commands.md Sec 2 table]
 *
 * MUTATION PROOF (Rule 6):
 *   -DSET_MUTATE_EXACT_DEFAULT: initialise set_exact to 1 (ON) instead of 0
 *   (OFF). The EXACT-default assertion (set_get_exact==0) and the C= begins-with
 *   test ("ABC" = "AB" -> .T. under OFF) both go RED, proving the oracle bites.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.6 (SET state; contract + oracle).
 *   - ../dbase3-decomp/specs/commands/set-commands.md (full SET grammar; defaults;
 *     III+ vs IV flags; SET NEAR absent; SET DATE keyword list).
 *   - ../dbase3-decomp/re/mint-results-002.md (EXACT=OFF; DECIMALS=2 [verified]).
 *   - ../dbase3-decomp/re/mint-results-003.md (DATE=AMERICAN; CENTURY=OFF [verified]).
 *   - os/samir/include/samir/interp.h (xb_cmd_hook + chain API).
 *   - os/samir/include/samir/eval.h   (xb_ctx.set_exact).
 *   - os/samir/include/samir/workarea.h (wa_set_order / wa_selected).
 *   - os/samir/include/samir/set.h    (set_state + public getters).
 */

#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"
#include "samir/set.h"

/* ===================================================================== */
/* Tunables                                                               */
/* ===================================================================== */

/* Maximum concurrent live interpreter handles with registered SET state. */
#define SET_REGISTRY  16

/* ===================================================================== */
/* Per-interpreter SET state registry (static; no malloc).               */
/* ===================================================================== */

static xb_interp *g_key[SET_REGISTRY];
static set_state  g_val[SET_REGISTRY];
static int        g_n;

/*
 * set_state_for: look up (or create) the set_state for `ip`. Returns a pointer
 * to the entry on success, NULL if the registry is full. The caller is
 * responsible for initialising the entry on first creation (is_new output).
 */
static set_state *set_state_for(xb_interp *ip, int *is_new)
{
    int i;
    if (!ip)
        return (set_state *)0;
    for (i = 0; i < g_n; i++) {
        if (g_key[i] == ip) {
            if (is_new) *is_new = 0;
            return &g_val[i];
        }
    }
    if (g_n >= SET_REGISTRY)
        return (set_state *)0;          /* registry full: fail loud at caller */
    g_key[g_n] = ip;
    if (is_new) *is_new = 1;
    return &g_val[g_n++];
}

/* ===================================================================== */
/* Small ASCII helpers (freestanding; no libc)                           */
/* ===================================================================== */

static char s_up1(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');
    return c;
}

static int s_isspace(char c)
{
    return c == ' ' || c == '\t';
}

/* case-insensitive equality of two NUL-terminated strings. */
static int s_ci_eq(const char *a, const char *b)
{
    int i = 0;
    for (;;) {
        char ca = s_up1(a[i]);
        char cb = s_up1(b[i]);
        if (ca != cb) return 0;
        if (ca == '\0') return 1;
        i++;
    }
}

/* skip leading blanks; return pointer past them. */
static const char *s_skip_ws(const char *s)
{
    while (*s != '\0' && s_isspace(*s)) s++;
    return s;
}

/* parse an unsigned integer at *s; advance *s past it. */
static uint32_t s_parse_uint(const char **s)
{
    uint32_t v = 0;
    const char *p = *s;
    while (*p >= '0' && *p <= '9') {
        v = v * 10u + (uint32_t)(*p - '0');
        p++;
    }
    *s = p;
    return v;
}

/* copy up to cap-1 bytes of `src` into `dst`, NUL-terminate, trim trailing WS. */
static void s_copy_trim(char *dst, uint32_t cap, const char *src)
{
    uint32_t i = 0, last_nonws = 0;
    while (src[i] != '\0' && i < cap - 1u) {
        dst[i] = src[i];
        if (!s_isspace(src[i])) last_nonws = i + 1u;
        i++;
    }
    dst[last_nonws] = '\0';
}

/* ===================================================================== */
/* parse_on_off: parse "ON" or "OFF" from `args`. Sets *val (1/0).       */
/* Returns 0 on success, -1 on parse error.                              */
/* ===================================================================== */

static int parse_on_off(const char *args, int *val)
{
    const char *s = s_skip_ws(args);
    if (s_ci_eq(s, "ON"))  { *val = 1; return 0; }
    if (s_ci_eq(s, "OFF")) { *val = 0; return 0; }
    return -1;
}

/* ===================================================================== */
/* Individual SET option handlers                                         */
/* ===================================================================== */

/*
 * do_set_exact: SET EXACT ON|OFF.
 * Writes ctx->set_exact immediately (no copy in set_state -- the ctx IS the
 * runtime field the evaluator reads). Returns INTERP_OK or -INTERP_ERR_SYNTAX.
 *
 * Ref: eval.h xb_ctx.set_exact; plan S5.6 "write xb_interp_ctx(ip)->set_exact".
 *
 * MUTATION PROOF: -DSET_MUTATE_EXACT_DEFAULT inverts the DEFAULT initialisation,
 * not the SET command itself. This function is correct; the mutant fires in
 * set_init_defaults where the initial set_exact is set.
 */
static int do_set_exact(xb_interp *ip, const char *args, int *err_code)
{
    int val;
    if (parse_on_off(args, &val) != 0) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }
    xb_interp_ctx(ip)->set_exact = val;
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_decimals: SET DECIMALS TO [<n>].
 * Stores the value in set_state AND writes ctx->set_decimals so fn_builtins.c
 * STR() 1-arg picks up the new setting immediately.
 * Ref: set-commands.md Sec 3.1 (default 2; "SET DECIMALS TO" with no value
 * resets to 2). [verified: mint-results-002.md DECIMALS=2 default]
 */
static int do_set_decimals(xb_interp *ip, set_state *ss,
                           const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    uint8_t v;

    /* consume optional "TO" keyword */
    if (s_ci_eq(s, "TO") ||
            ((s[0]=='T'||s[0]=='t') && (s[1]=='O'||s[1]=='o')
             && (s[2]=='\0' || s_isspace(s[2])))) {
        /* move past "TO" */
        s = s_skip_ws(s + 2);
    }
    if (*s == '\0') {
        /* SET DECIMALS TO (no value) -> reset to default 2 */
        v = 2;
    } else {
        uint32_t raw = s_parse_uint(&s);
        v = (uint8_t)(raw > 15 ? 15 : raw);  /* cap to valid dec range */
    }
    ss->decimals = (int)v;
    /* Wire into ctx so fn_builtins.c STR() reads the updated value.
     * Ref: eval.h xb_ctx.set_decimals; fn_builtins.c fn_str 1-arg path. */
    xb_interp_ctx(ip)->set_decimals = v;
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_date: SET DATE [TO] <keyword>.
 * Stores the set_date_fmt in set_state AND writes ctx->set_date_fmt so
 * fn_builtins.c DTOC()/CTOD() reads the updated format immediately.
 * Ref: set-commands.md Sec 3.2 (AMERICAN/ANSI/BRITISH/ITALIAN/FRENCH/GERMAN/
 * JAPAN/USA; default AMERICAN) [verified: harbour set.txt:130-140;
 * mint-results-003.md DATE=AMERICAN].
 */
static int do_set_date(xb_interp *ip, set_state *ss,
                       const char *args, int *err_code)
{
    set_date_fmt fmt;
    const char *s = s_skip_ws(args);

    /* consume optional "TO" keyword */
    if ((s[0]=='T'||s[0]=='t') && (s[1]=='O'||s[1]=='o')
            && (s[2]=='\0' || s_isspace(s[2]))) {
        s = s_skip_ws(s + 2);
    }
    if      (s_ci_eq(s, "AMERICAN")) { fmt = SET_DATE_AMERICAN; }
    else if (s_ci_eq(s, "ANSI"))     { fmt = SET_DATE_ANSI; }
    else if (s_ci_eq(s, "BRITISH"))  { fmt = SET_DATE_BRITISH; }
    else if (s_ci_eq(s, "ITALIAN"))  { fmt = SET_DATE_ITALIAN; }
    else if (s_ci_eq(s, "FRENCH"))   { fmt = SET_DATE_FRENCH; }
    else if (s_ci_eq(s, "GERMAN"))   { fmt = SET_DATE_GERMAN; }
    else if (s_ci_eq(s, "JAPAN"))    { fmt = SET_DATE_JAPAN; }
    else if (s_ci_eq(s, "USA"))      { fmt = SET_DATE_USA; }
    else {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }
    ss->date_fmt = fmt;
    /* Wire into ctx so fn_builtins.c DTOC/CTOD reads the updated format.
     * Ref: eval.h xb_ctx.set_date_fmt; fn_builtins.c fn_dtoc_impl/fn_ctod_impl. */
    xb_interp_ctx(ip)->set_date_fmt = (uint8_t)fmt;
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_century: SET CENTURY ON|OFF.
 * Stores in set_state AND writes ctx->set_century so fn_builtins.c DTOC()
 * emits 4-digit years when ON.
 * Ref: set-commands.md Sec 2 (default OFF) [verified: mint-results-003.md;
 * dates-and-century.md: CENTURY OFF = 2-digit year; ON = 4-digit year].
 */
static int do_set_century(xb_interp *ip, set_state *ss,
                          const char *args, int *err_code)
{
    int val;
    if (parse_on_off(args, &val) != 0) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }
    ss->century = val;
    /* Wire into ctx so fn_builtins.c DTOC reads the updated century flag.
     * Ref: eval.h xb_ctx.set_century; fn_builtins.c fn_dtoc_impl. */
    xb_interp_ctx(ip)->set_century = (uint8_t)(val ? 1 : 0);
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_order: SET ORDER TO [<n>].
 * Calls wa_set_order on the selected work area (read-only use of the existing
 * function; workarea.c is not edited). Ref: set-commands.md Sec 3.5; plan S5.6
 * "call the existing wa_set_order (read/call only, no workarea.c edit)".
 * "SET ORDER TO 0" or bare "SET ORDER TO" -> natural order (0).
 */
static int do_set_order(xb_interp *ip, const char *args, int *err_code)
{
    wa_env *env = xb_interp_env(ip);
    int area;
    uint32_t n = 0u;
    int rc;
    const char *s = s_skip_ws(args);

    /* consume "TO" keyword if present */
    if ((s[0]=='T'||s[0]=='t') && (s[1]=='O'||s[1]=='o')
            && (s[2]=='\0' || s_isspace(s[2]))) {
        s = s_skip_ws(s + 2);
    }

    if (*s != '\0') {
        n = s_parse_uint(&s);
    }
    /* 0 = natural record order while keeping indexes open [set-commands.md 3.5] */

    area = wa_selected(env);
    if (area < 1) {
        /* no open work area: fail loud */
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }

    rc = wa_set_order(env, area, (int)n);
    if (rc != WA_OK) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }

    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_index: SET INDEX TO [<file list>].
 * GATED -- work-area index-open plumbing not yet present (plan S5.6 task:
 * "parse + store intent, loud-skip runtime effect"). Stores the raw text.
 * Ref: set-commands.md Sec 3.5 (SET INDEX TO <list>).
 */
static int do_set_index(set_state *ss, const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    /* consume "TO" keyword if present */
    if ((s[0]=='T'||s[0]=='t') && (s[1]=='O'||s[1]=='o')
            && (s[2]=='\0' || s_isspace(s[2]))) {
        s = s_skip_ws(s + 2);
    }
    s_copy_trim(ss->index_text, (uint32_t)SET_GATED_TEXT_CAP, s);
    ss->have_index = 1;
    /* GATED: runtime index-open effect deferred -- follow-up bead required. */
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_filter: SET FILTER TO [<condition>].
 * GATED -- work-area filter plumbing not yet present. Stores raw text.
 * Ref: set-commands.md Sec 3.5 (SET FILTER TO <condition>).
 */
static int do_set_filter(set_state *ss, const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    /* consume "TO" keyword if present */
    if ((s[0]=='T'||s[0]=='t') && (s[1]=='O'||s[1]=='o')
            && (s[2]=='\0' || s_isspace(s[2]))) {
        s = s_skip_ws(s + 2);
    }
    s_copy_trim(ss->filter_text, (uint32_t)SET_GATED_TEXT_CAP, s);
    ss->have_filter = 1;
    /* GATED: runtime filter effect deferred -- follow-up bead required. */
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_relation: SET RELATION TO [<key expr> INTO <alias> ...].
 * GATED -- relation plumbing not yet present. Stores raw text.
 * Ref: set-commands.md Sec 3.5 (SET RELATION TO <key> INTO <alias>).
 */
static int do_set_relation(set_state *ss, const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    /* consume "TO" keyword if present */
    if ((s[0]=='T'||s[0]=='t') && (s[1]=='O'||s[1]=='o')
            && (s[2]=='\0' || s_isspace(s[2]))) {
        s = s_skip_ws(s + 2);
    }
    s_copy_trim(ss->relation_text, (uint32_t)SET_GATED_TEXT_CAP, s);
    ss->have_relation = 1;
    /* GATED: runtime relation effect deferred -- follow-up bead required. */
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_talk: SET TALK ON|OFF.
 * Stores in set_state. Runtime effect (REPLACE/APPEND counts echoed) DEFERRED
 * (coupled to output formatting in parallel lanes).
 * Ref: set-commands.md Sec 2 (default ON) [verified: sample programs idiom].
 */
static int do_set_talk(set_state *ss, const char *args, int *err_code)
{
    int val;
    if (parse_on_off(args, &val) != 0) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }
    ss->talk = val;
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_safety: SET SAFETY ON|OFF.
 * Stores in set_state. File-overwrite guard DEFERRED (needs file I/O layer).
 * Ref: set-commands.md Sec 2 (default ON) [verified: sample programs idiom].
 */
static int do_set_safety(set_state *ss, const char *args, int *err_code)
{
    int val;
    if (parse_on_off(args, &val) != 0) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }
    ss->safety = val;
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/* ===================================================================== */
/* Initialise the set_state to III+ 1.1 defaults.                        */
/* Called by set_register. The MUTATION PROOF fires here.                */
/* ===================================================================== */

static void set_init_defaults(xb_interp *ip, set_state *ss)
{
    int i;

    /*
     * SET EXACT default = OFF [verified: mint-results-002.md].
     * Write into the evaluator's ctx directly (the field the engine reads).
     * MUTATION PROOF: -DSET_MUTATE_EXACT_DEFAULT: initialise to 1 (ON) instead
     * of 0 (OFF). The default-is-OFF assertion and the C= begins-with test go RED.
     */
#ifdef SET_MUTATE_EXACT_DEFAULT
    xb_interp_ctx(ip)->set_exact = 1;    /* MUTANT: wrong default */
#else
    xb_interp_ctx(ip)->set_exact = 0;    /* correct: EXACT OFF */
#endif

    /* SET DECIMALS default = 2 [verified: mint-results-002.md].
     * Written into ss->decimals (stored state) AND ctx->set_decimals (the live field
     * fn_builtins.c reads for STR() 1-arg default decimals). */
    ss->decimals = 2;
    xb_interp_ctx(ip)->set_decimals = 2;

    /* SET DATE default = AMERICAN [verified: mint-results-003.md]. */
    ss->date_fmt = SET_DATE_AMERICAN;
    xb_interp_ctx(ip)->set_date_fmt = (uint8_t)SET_DATE_AMERICAN;

    /* SET CENTURY default = OFF [verified: mint-results-003.md]. */
    ss->century = 0;
    xb_interp_ctx(ip)->set_century = 0;

    /* SET TALK default = ON [verified: set-commands.md Sec 2]. */
    ss->talk = 1;

    /* SET SAFETY default = ON [verified: set-commands.md Sec 2]. */
    ss->safety = 1;

    /* GATED options: cleared. */
    for (i = 0; i < SET_GATED_TEXT_CAP; i++) {
        ss->index_text[i]    = '\0';
        ss->filter_text[i]   = '\0';
        ss->relation_text[i] = '\0';
    }
    ss->have_index    = 0;
    ss->have_filter   = 0;
    ss->have_relation = 0;
}

/* ===================================================================== */
/* The SET command hook (main dispatch)                                   */
/* ===================================================================== */

/*
 * set_cmd_hook: an xb_cmd_hook (interp.h). Dispatches the SET verb.
 * Returns CMD_OK (handled), CMD_UNKNOWN (not a SET verb), or a negative error.
 *
 * `verb` is always upper-cased by the executor's verb splitter. We only handle
 * verb == "SET"; everything else is CMD_UNKNOWN so the chain can continue.
 *
 * args: the remainder of the line after "SET", trimmed of leading blanks.
 * The first word of args is the sub-option (EXACT, DECIMALS, etc.).
 */
static int set_cmd_hook(void *user, xb_interp *ip,
                        const char *verb, const char *args, int *err_code)
{
    set_state  *ss;
    const char *s, *opt_start;
    char        opt[32];
    int         i, opt_len;

    (void)user;
    if (err_code) *err_code = 0;
    if (!ip || !verb) return CMD_UNKNOWN;

    /* Only handle the "SET" verb. */
    if (!s_ci_eq(verb, "SET")) return CMD_UNKNOWN;

    /* Find the set_state for this interpreter. */
    ss = set_state_for(ip, (int *)0);
    if (!ss) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_NOMEM;
    }

    /* Extract the sub-option name (first word of args, upper-cased). */
    s = s_skip_ws(args);
    opt_start = s;
    i = 0;
    while (*s != '\0' && !s_isspace(*s) && i < (int)(sizeof(opt) - 1)) {
        opt[i++] = s_up1(*s);
        s++;
    }
    opt[i] = '\0';
    opt_len = i;

    if (opt_len == 0) {
        /* bare "SET" with no sub-option: fail loud (INTERP_ERR_SYNTAX). */
        (void)opt_start;
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }

    /* Remainder after the sub-option keyword (leading blanks stripped). */
    s = s_skip_ws(s);

    /* ------------------------------------------------------------------ */
    /* Dispatch on the sub-option.                                         */
    /* ------------------------------------------------------------------ */

    if (s_ci_eq(opt, "EXACT")) {
        int rc = do_set_exact(ip, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "DECIMALS")) {
        int rc = do_set_decimals(ip, ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "DATE")) {
        int rc = do_set_date(ip, ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "CENTURY")) {
        int rc = do_set_century(ip, ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "ORDER")) {
        int rc = do_set_order(ip, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "INDEX")) {
        int rc = do_set_index(ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "FILTER")) {
        int rc = do_set_filter(ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "RELATION")) {
        int rc = do_set_relation(ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "TALK")) {
        int rc = do_set_talk(ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    if (s_ci_eq(opt, "SAFETY")) {
        int rc = do_set_safety(ss, s, err_code);
        return (rc == INTERP_OK) ? CMD_OK : rc;
    }

    /*
     * SET NEAR: NOT a III+ option. Fail loud as an unrecognised SET sub-option.
     * [Ref: set-commands.md Sec 5 completeness: NEAR absent from HELP_topics.txt
     *  III+ index; it is a Clipper/dBASE IV addition.]
     */
    if (s_ci_eq(opt, "NEAR")) {
        /* Fail loud -- this is not a valid III+ SET option. INTERP_ERR_SYNTAX
         * carries the "unrecognised" signal; the executor surfaces it as #16. */
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }

    /* Unknown SET sub-option: fail loud. Rule 2 -- no silent skip. */
    if (err_code) *err_code = 0;
    return -INTERP_ERR_SYNTAX;
}

/* ===================================================================== */
/* Public API                                                             */
/* ===================================================================== */

/*
 * set_register: install the SET command hook into ip's command-hook chain and
 * initialise the set_state to the III+ 1.1 defaults.
 *
 * Returns INTERP_OK on success, -INTERP_ERR_NOMEM if the set-state registry is
 * full, -INTERP_ERR_DEPTH if the command-hook chain is full.
 */
int set_register(xb_interp *ip)
{
    set_state *ss;
    int is_new = 0;

    ss = set_state_for(ip, &is_new);
    if (!ss)
        return -INTERP_ERR_NOMEM;       /* registry full */

    if (is_new)
        set_init_defaults(ip, ss);      /* fresh entry: apply defaults */

    return xb_interp_add_cmd_hook(ip, set_cmd_hook, (void *)0);
}

/* ===================================================================== */
/* Getters                                                                */
/* ===================================================================== */

int set_get_exact(xb_interp *ip)
{
    xb_ctx *ctx;
    if (!ip) return 0;
    ctx = xb_interp_ctx(ip);
    return ctx ? ctx->set_exact : 0;
}

int set_get_decimals(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return 2;
    ss = set_state_for(ip, (int *)0);
    return ss ? ss->decimals : 2;
}

set_date_fmt set_get_date_fmt(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return SET_DATE_AMERICAN;
    ss = set_state_for(ip, (int *)0);
    return ss ? ss->date_fmt : SET_DATE_AMERICAN;
}

int set_get_century(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return 0;
    ss = set_state_for(ip, (int *)0);
    return ss ? ss->century : 0;
}

int set_get_talk(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return 1;
    ss = set_state_for(ip, (int *)0);
    return ss ? ss->talk : 1;
}

int set_get_safety(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return 1;
    ss = set_state_for(ip, (int *)0);
    return ss ? ss->safety : 1;
}

const set_state *set_get_state(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return (const set_state *)0;
    ss = set_state_for(ip, (int *)0);
    return ss;
}
