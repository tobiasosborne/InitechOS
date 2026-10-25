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

#include "samir/lifetime.h"
#include <stdint.h>

#include "samir/interp.h"
#include "samir/workarea.h"
#include "samir/eval.h"
#include "samir/value.h"
#include "samir/rt.h"
#include "samir/pal.h"
#include "samir/set.h"
#include "samir/ndx.h"
#include "samir/nav.h"
#include "samir/ovl.h"

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

/* Using III+ U5-193: close owns the handle and appends the DOS EOF byte.
 * Replacing a log closes its predecessor; ON/OFF only pauses capture. */
void set_close_alternate(xb_interp *ip)
{
    int i;samir_pal_t *pal=xb_interp_pal(ip);
    for(i=0;i<g_n;i++)if(g_key[i]==ip && g_val[i].alternate>=0) {
        char eof=26;
        pal->write(pal,g_val[i].alternate,&eof,1);
        pal->close(pal,g_val[i].alternate);
        g_val[i].alternate=-1;g_val[i].alternate_on=0;
    }
}
#ifdef SAMIR_OVL_MUTATE_ESCAPE_CALLBACK
static SAMIR_OVL_TEXT(set) void alternate_output(
#else
static void alternate_output(
#endif
void *user,const char *s,uint32_t n)
{
    xb_interp *ip=user;samir_pal_t *pal=xb_interp_pal(ip);int i;
    for(i=0;i<g_n;i++)if(g_key[i]==ip && g_val[i].alternate_on && g_val[i].alternate>=0) {
#ifndef SAMIR_FNS_MUTANT_alternate
        if(pal->write(pal,g_val[i].alternate,s,n)!=(int32_t)n) {
            g_val[i].alternate_on=0;
            /* Preserve a failed log rather than continuing silent partial I/O. */
            xb_interp_ctx(ip)->error_number=8;
        }
#else
        (void)pal;(void)s;(void)n;
#endif
        return;
    }
}

/*
 * set_state_for: look up (or create) the set_state for `ip`. Returns a pointer
 * to the entry on success, NULL if the registry is full. The caller is
 * responsible for initialising the entry on first creation (is_new output).
 */
static set_state *set_state_for(xb_interp *ip, int *is_new)
{
    SAMIR_RETAIN(ip);
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
    SAMIR_RETAIN(ip);
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

/* skip an optional whole-word "TO" (any case) and the blanks after it. */
static SAMIR_OVL_TEXT(set) const char *s_skip_to(const char *s)
{
    if (rt_kw_word(s, "TO\0", (int *)0) == 0)
        s = s_skip_ws(s + 2);
    return s;
}

/* parse an unsigned integer at *s; advance *s past it. */
static SAMIR_OVL_TEXT(set) uint32_t s_parse_uint(const char **s)
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
static SAMIR_OVL_TEXT(set) void s_copy_trim(char *dst, uint32_t cap, const char *src)
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

static SAMIR_OVL_TEXT(set) int parse_on_off(const char *args, int *val)
{
    /* "OFF" is entry 0 and "ON" entry 1: the index is the value. */
    int k = rt_kw_index(s_skip_ws(args), "OFF\0ON\0");
    if (k < 0) return -1;
    *val = k;
    return 0;
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
static SAMIR_OVL_TEXT(set) int do_set_exact(xb_interp *ip, const char *args, int *err_code)
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
static SAMIR_OVL_TEXT(set) int do_set_decimals(xb_interp *ip, set_state *ss,
                           const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    uint8_t v;

    /* consume optional "TO" keyword */
    s = s_skip_to(s);
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
static SAMIR_OVL_TEXT(set) int do_set_date(xb_interp *ip, set_state *ss,
                       const char *args, int *err_code)
{
    set_date_fmt fmt;
    int k;
    const char *s = s_skip_ws(args);

    /* consume optional "TO" keyword */
    s = s_skip_to(s);
    /* The keyword's table position IS its set_date_fmt (set.h: AMERICAN=0
     * .. USA=7, in this order). */
    k = rt_kw_index(s, "AMERICAN\0ANSI\0BRITISH\0ITALIAN\0FRENCH\0GERMAN\0"
                       "JAPAN\0USA\0");
    if (k < 0) {
        if (err_code) *err_code = 0;
        return -INTERP_ERR_SYNTAX;
    }
    fmt = (set_date_fmt)k;
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
static SAMIR_OVL_TEXT(set) int do_set_century(xb_interp *ip, set_state *ss,
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
static SAMIR_OVL_TEXT(set) int do_set_order(xb_interp *ip, const char *args, int *err_code)
{
    wa_env *env = xb_interp_env(ip);
    int area;
    uint32_t n = 0u;
    int rc;
    const char *s = s_skip_ws(args);

    /* consume "TO" keyword if present */
    s = s_skip_to(s);

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
        /* SET ORDER TO 9 with no ninth index: #106, real III+ SF-errors. */
        if (err_code) *err_code = 106;
        return -INTERP_ERR_SYNTAX;
    }

    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_index: SET INDEX TO [<file list>].
 * Replace the selected area's open indexes; retain text for old accessors.
 * Ref: Using III+ U5-239 and real SF-index transcript.
 */
static SAMIR_OVL_TEXT(set) int do_set_index(xb_interp *ip, set_state *ss, const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    /* consume "TO" keyword if present */
    s = s_skip_to(s);
    s_copy_trim(ss->index_text, (uint32_t)SET_GATED_TEXT_CAP, s);
    ss->have_index = 1;
    /* Using III+ U5-239; SF-index: replace this area's list, first is master,
     * omitted extensions are NDX, and position at the master beginning. */
#ifndef SAMIR_FNS_MUTANT_index
    {
        wa_env *env=xb_interp_env(ip);
        int area=wa_selected(env),count=0;
        extern void wa_close_indexes(wa_env *,int);
        if(!wa_table(env,area)) {if(err_code)*err_code=52;return -INTERP_ERR_EVAL;}
        wa_close_indexes(env,area);wa_nav_reset(area);
        while(*s) {
            char name[128],display[128];uint32_t n=0;int dot=0;ndx_index *ix;
            s=s_skip_ws(s);
            while(*s && *s!=',' && !s_isspace(*s)) {
                if(n>=sizeof name-5){if(err_code)*err_code=10;return -INTERP_ERR_SYNTAX;}
                if(*s=='/' || *s=='\\' || *s==':')dot=0;
                else if(*s=='.')dot=1;
                name[n++]=*s++;
            }
            name[n]=0;
            if(!n || count>=NDX_PER_AREA) {if(err_code)*err_code=count>=NDX_PER_AREA?28:10;return -INTERP_ERR_SYNTAX;}
            if(!dot)rt_memcpy(name+n,".NDX",5);
            rt_memcpy(display,name,rt_strlen(name)+1u);
            for(uint32_t j=0;name[j];j++)name[j]=s_up1(name[j]);
            if(ndx_open_rw(xb_interp_pal(ip),name,&ix)!=NDX_OK) {
                if(err_code)*err_code=1;
                return -INTERP_ERR_EVAL;
            }
            /* III+ ndx/GOLD.OUT: SET INDEX displays the new operand's case. */
            ndx_set_name_defaulted(ix,!dot);
            if(ndx_set_display_name(ix,display)!=NDX_OK) {ndx_close(ix);if(err_code)*err_code=114;return -INTERP_ERR_EVAL;}
            if(wa_attach_index(env,area,ix)!=WA_OK) {ndx_close(ix);if(err_code)*err_code=28;return -INTERP_ERR_EVAL;}
            count++;
            s=s_skip_ws(s);if(*s==',')s=s_skip_ws(s+1);
        }
        if(count) {
            wa_set_order(env,area,1);
            if(wa_nav_index_top(env,area)!=NAV_OK) {if(err_code)*err_code=114;return -INTERP_ERR_EVAL;}
        }
    }
#else
    (void)ip;
#endif
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_filter: SET FILTER TO [<condition>].
 * Installs the selected area condition; see Using III+ U5-229.
 * Ref: set-commands.md Sec 3.5 (SET FILTER TO <condition>).
 */
static SAMIR_OVL_TEXT(set) int do_set_filter(xb_interp *ip, set_state *ss, const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    /* consume "TO" keyword if present */
    s = s_skip_to(s);
    /* Ref: Using dBase III Plus.pdf U5-229: selected work area, lazy movement. */
    {
        int rc=wa_set_filter(ip, wa_selected(xb_interp_env(ip)), s, err_code);
        if(rc) return rc;
    }
    s_copy_trim(ss->filter_text, (uint32_t)SET_GATED_TEXT_CAP, s);
    ss->have_filter = (s[0] != '\0');
    /* Text accessor retained for existing callers; runtime lives on the area. */
    if (err_code) *err_code = 0;
    return INTERP_OK;
}

/*
 * do_set_relation: SET RELATION TO [<key expr> INTO <alias> ...].
 * GATED -- relation plumbing not yet present. Stores raw text.
 * Ref: set-commands.md Sec 3.5 (SET RELATION TO <key> INTO <alias>).
 */
static SAMIR_OVL_TEXT(set) int do_set_relation(xb_interp *ip, set_state *ss, const char *args, int *err_code)
{
    const char *s = s_skip_ws(args);
    /* consume "TO" keyword if present */
    s = s_skip_to(s);
    s_copy_trim(ss->relation_text, (uint32_t)SET_GATED_TEXT_CAP, s);
    ss->have_relation = 1;
    wa_set_relation_text(xb_interp_env(ip),wa_selected(xb_interp_env(ip)),ss->relation_text);
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
static SAMIR_OVL_TEXT(set) int do_set_talk(set_state *ss, const char *args, int *err_code)
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
 * Stores in set_state; ZAP consumes it. Other file-overwrite guards deferred.
 * Ref: set-commands.md Sec 2 (default ON) [verified: sample programs idiom].
 */
static SAMIR_OVL_TEXT(set) int do_set_safety(set_state *ss, const char *args, int *err_code)
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
    ss->alternate=-1;ss->alternate_on=0;
    ss->alternate_name[0]=0;
#ifndef SAMIR_OVL_MUTATE_ESCAPE_CALLBACK
    samir_bind_output(xb_interp_ctx(ip),alternate_output,ip);
#endif

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
    xb_interp_ctx(ip)->set_heading=1;
    xb_interp_ctx(ip)->set_console=1;
    xb_interp_ctx(ip)->set_status=0;

    /* SET SAFETY default = ON [verified: set-commands.md Sec 2]. */
    ss->safety = 1;
    /* III+ HELP.DBS SET COLOR: white/black, black/white, black border. */
    ss->color_standard=0x07;ss->color_enhanced=0x70;
    ss->color_border=ss->color_background=0;
    /* SET ESCAPE default = ON (Using III+ U5-220 "normally ON"). */
    ss->escape = 1;

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

/* Ref: ../dbase3-decomp/specs/commands/set-commands.md SET COLOR,
 * HELP.DBS lines 1390-1398; CGA attribute layout in os/milton/console.c.
 * U/I/X are monochrome underline/inverse/blank attributes. RG and GR
 * denote the same red+green mixture; RB denotes red+blue. */
static SAMIR_OVL_TEXT(set) int color_name(const char **text)
{
    const char *s=*text;
    int a=s_up1(*s), value;
    if(!a)return -1;
    s++;
    if(a=='N'||a=='X') value=0;
    else if(a=='B') { value=1;if(s_up1(*s)=='G'){value=3;s++;} }
    else if(a=='G') { value=2;if(s_up1(*s)=='R'){value=6;s++;} }
    else if(a=='R') {
        value=4;
        if(s_up1(*s)=='B'){value=5;s++;}
        else if(s_up1(*s)=='G'){value=6;s++;}
    }
    else if(a=='W')value=7;
    else if(a=='U')value=1;
    else if(a=='I')value=0x70;
    else return -1;
    *text=s;
    return value;
}

static SAMIR_OVL_TEXT(set) int color_modifiers(const char **text,int *bits)
{
    while(**text=='+'||**text=='*') {
        int bit=**text=='+'?8:128;
        if(*bits&bit)return -1;
        *bits|=bit;
        (*text)++;
    }
    return 0;
}

static SAMIR_OVL_TEXT(set) int color_attribute(const char **text,uint8_t *attr)
{
    const char *s=s_skip_ws(*text);
    int mono=s_up1(*s)=='U'||s_up1(*s)=='I'||s_up1(*s)=='X';
    int fg=color_name(&s), bg=0, bits=0;
    if(fg<0||color_modifiers(&s,&bits))return -1;
    s=s_skip_ws(s);
    if(*s=='/') {
        s=s_skip_ws(s+1);bg=color_name(&s);
        if(bg<0||bg>7||color_modifiers(&s,&bits))return -1;
        s=s_skip_ws(s);
    } else if(!mono)return -1;
    if(*s && *s!=',')return -1;
#ifdef SET_COLOR_MUTANT_BG
    bg=0; /* Rule 6: drop only the parsed background nibble. */
#endif
    *attr=(uint8_t)(fg|(bg<<4)|bits);
    *text=s;
    return 0;
}

static SAMIR_OVL_TEXT(set) int do_set_color(xb_interp *ip,set_state *ss,const char *s,int *err)
{
    uint8_t attrs[4];unsigned field=0;
    samir_pal_t *pal=xb_interp_pal(ip);
    /* The TO grammar is literal attributes, not an expression or DOS SET.
     * Malformed literals -> #10 Syntax error (catalogue, spec Sec 1.4).
     * Parse into locals first: even a bad fourth field changes nothing. */
    if(rt_kw_word(s,"TO\0",(int *)0)!=0)goto invalid;
    s=s_skip_ws(s+2);
    attrs[0]=ss->color_standard;attrs[1]=ss->color_enhanced;
    attrs[2]=ss->color_border;attrs[3]=ss->color_background;
    if(!*s) {
        attrs[0]=0x07;attrs[1]=0x70;attrs[2]=attrs[3]=0;
    } else for(;;) {
        s=s_skip_ws(s);
        if(*s && *s!=',' && color_attribute(&s,&attrs[field]))goto invalid;
        if(!*s)break;
        if(*s!=','||++field==4)goto invalid;
        s++;
    }
    if(!pal->set_attr)goto invalid;
    ss->color_standard=attrs[0];ss->color_enhanced=attrs[1];
    ss->color_border=attrs[2];ss->color_background=attrs[3];
    pal->set_attr(pal,attrs[0]);
    return CMD_OK;
invalid:
    if(err)*err=10;
    return -INTERP_ERR_SYNTAX;
}

/* ===================================================================== */
/* SET option table (F4, initech-0wox2)                                   */
/* ===================================================================== */
/*
 * One handler per SET option, one row per option in set_options.def (an
 * X-macro list; .gitattributes marks it merge=union, so two lanes adding
 * rows avoid positional conflicts; test-samir-registries validates the merge).
 * A handler is
 *     int fn(xb_interp *ip, set_state *ss, const char *s, int *err_code)
 * with `s` the text after the option keyword, and returns exactly what
 * set_cmd_hook returns (CMD_OK or a negative error).  The keyword is matched
 * by rt_kw_index (whole token, III+ four-letter abbreviation); where two
 * keywords share an abbreviation the EARLIER row wins, so rows are only ever
 * ordered for that tie-break -- never numbered.
 */
typedef int (*set_opt_fn)(xb_interp *ip, set_state *ss, const char *s, int *err_code);
typedef struct { const char *name; set_opt_fn fn; } set_opt_row;

static SAMIR_OVL_TEXT(set) int set_opt_alternate(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int on;
    if(!parse_on_off(s,&on)) { ss->alternate_on=on;return CMD_OK; }
    if(rt_kw_word(s,"TO\0",(int *)0)==0) {
        char name[128];uint32_t n;samir_pal_t *pal=xb_interp_pal(ip);
        s=s_skip_ws(s+2);set_close_alternate(ip);
        if(!*s)return CMD_OK;
        s_copy_trim(name,sizeof name,s);n=rt_strlen(name);
        if(n>1 && ((name[0]=='"' && name[n-1]=='"')||(name[0]=='\'' && name[n-1]=='\''))) {
            uint32_t j;for(j=0;j<n-2;j++)name[j]=name[j+1];name[n-2]=0;
        }
        {uint32_t j;int dot=0;for(j=0;name[j];j++)if(name[j]=='.')dot=1;
         if(!dot && j+4<sizeof name)rt_memcpy(name+j,".TXT",5);}
        ss->alternate=pal->open(pal,name,PAL_WR|PAL_CREATE|PAL_TRUNC);
        if(ss->alternate<0){if(err_code)*err_code=8;return -INTERP_ERR_EVAL;}
        s_copy_trim(ss->alternate_name,sizeof ss->alternate_name,name);
        return CMD_OK;
    }
    if(err_code)*err_code=10;
    return -INTERP_ERR_SYNTAX;
}

static SAMIR_OVL_TEXT(set) int set_opt_exact(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_exact(ip, s, err_code);
    (void)ss;
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_decimals(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_decimals(ip, ss, s, err_code);
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_date(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_date(ip, ss, s, err_code);
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_century(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_century(ip, ss, s, err_code);
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_order(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_order(ip, s, err_code);
    (void)ss;
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_index(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_index(ip, ss, s, err_code);
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_deleted(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int on;
    (void)ss;
    if (parse_on_off(s, &on)) { if(err_code) *err_code=10; return -INTERP_ERR_SYNTAX; }
    wa_set_deleted(xb_interp_env(ip),on);
    if(err_code) *err_code=0;
    return CMD_OK;
}

static SAMIR_OVL_TEXT(set) int set_opt_filter(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_filter(ip, ss, s, err_code);
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_relation(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_relation(ip, ss, s, err_code);
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_talk(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_talk(ss, s, err_code);
    (void)ip;
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

static SAMIR_OVL_TEXT(set) int set_opt_safety(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int rc = do_set_safety(ss, s, err_code);
    (void)ip;
    return (rc == INTERP_OK) ? CMD_OK : rc;
}

/* SET ESCAPE ON|OFF (Using III+ U5-220): OFF disables the Esc program
 * interrupt and ON ESCAPE (U5-160). Before initech-39e5 it was #10. */
static SAMIR_OVL_TEXT(set) int set_opt_escape(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int on;
    (void)ip;
    if (parse_on_off(s, &on)) { if (err_code) *err_code = 10; return -INTERP_ERR_SYNTAX; }
    ss->escape = on;
    return CMD_OK;
}

/*
 * SET NEAR: NOT a III+ option. Fail loud as an unrecognised SET sub-option.
 * [Ref: set-commands.md Sec 5 completeness: NEAR absent from HELP_topics.txt
 *  III+ index; it is a Clipper/dBASE IV addition.]
 */
/* Using III+ U5-210/PDF 322, U5-235/PDF 347, U5-259/PDF 371. */
static SAMIR_OVL_TEXT(set) int set_opt_onoff_ctx(xb_interp *ip, const char *s, int *err_code, int which)
{
    int on;
    xb_ctx *ctx=xb_interp_ctx(ip);
    if(parse_on_off(s,&on)) { if(err_code) *err_code=10; return -INTERP_ERR_SYNTAX; }
    if(which==0) ctx->set_heading=(uint8_t)on;
    else if(which==1) ctx->set_console=(uint8_t)on;
    else ctx->set_status=(uint8_t)on;
    return CMD_OK;
}
static SAMIR_OVL_TEXT(set) int set_opt_heading(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{ (void)ss; return set_opt_onoff_ctx(ip,s,err_code,0); }
static SAMIR_OVL_TEXT(set) int set_opt_console(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{ (void)ss; return set_opt_onoff_ctx(ip,s,err_code,1); }
static SAMIR_OVL_TEXT(set) int set_opt_status(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{ (void)ss; return set_opt_onoff_ctx(ip,s,err_code,2); }

static SAMIR_OVL_TEXT(set) int set_opt_color(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    return do_set_color(ip,ss,s,err_code);
}

/* SET DEVICE: set-commands.md section 3.4; only @SAY. Parsing is cold
 * (docs/design/samir-overlays.md Sec 3); device/cursor state stays resident. */
static SAMIR_OVL_TEXT(set) int set_opt_device(xb_interp *ip, set_state *ss, const char *s, int *err_code)
{
    int device;
    (void)ss;
    if(rt_kw_word(s,"TO\0",(int *)0)!=0) {
        if(err_code) *err_code=10;
        return -INTERP_ERR_SYNTAX;
    }
    device=rt_kw_index(s_skip_ws(s+2),"SCREEN\0PRINT\0");
    if(device<0) { if(err_code) *err_code=10; return -INTERP_ERR_SYNTAX; }
    xb_interp_ctx(ip)->device_print=(uint8_t)device;
    return CMD_OK;
}

/* Rows come from set_options.def: SET_OPT(KEYWORD, handler). */
#define SET_OPT(kw, fn) { kw, fn },
static const set_opt_row set_opts[] SAMIR_OVL_DATA(set) = {
#include "set_options.def"
    { (const char *)0, (set_opt_fn)0 }
};
#undef SET_OPT
/* The keyword list in rt_kw_index form, in row order. */
#define SET_OPT(kw, fn) kw "\0"
static const char set_opt_kw[] =
#include "set_options.def"
    ;
#undef SET_OPT

/* The row index of `opt` (upper-cased), or -1: rt_kw_index over the rows. */
static SAMIR_OVL_TEXT(set) int set_opt_lookup(const char *opt)
{
    return rt_kw_index(opt, set_opt_kw);
}

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
/* Overlay SET (initech-5etom.1; docs/design/samir-overlays.md Sec 3): the
 * SET sub-option parser. set_cmd_hook (resident) claims the verb exactly as
 * before; everything after the claim runs here, unchanged. k is unused. */
/* Deliberate retained-pointer regressions for the runtime gates. */
#ifdef SAMIR_OVL_MUTATE_ESCAPE_HOOK
static SAMIR_OVL_TEXT(set) int ovl_bad_hook(void *u,xb_interp *ip,const char *v,const char *a,int *e)
{ (void)u;(void)ip;(void)v;(void)a;(void)e;return CMD_UNKNOWN; }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_PAL
static SAMIR_OVL_TEXT(set) void ovl_bad_pal(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p;(void)s;(void)n; }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_SAY
static SAMIR_OVL_TEXT(set) int ovl_bad_say(samir_pal_t *p,uint8_t r,uint8_t c,const char *s,uint32_t n)
{ (void)p;(void)r;(void)c;(void)s;(void)n;return 0; }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_PRINT
static SAMIR_OVL_TEXT(set) int ovl_bad_print(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p;(void)s;(void)n;return 0; }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_CURSOR
static SAMIR_OVL_TEXT(set) uint32_t ovl_bad_cursor(void *u) { (void)u;return 1; }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_ERROR
static SAMIR_OVL_TEXT(set) int ovl_bad_error(void *u,int e) { (void)u;(void)e;return 0; }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_SUSPEND
static SAMIR_OVL_TEXT(set) int ovl_bad_suspend(xb_interp *ip) { (void)ip;return 0; }
extern void proc_set_suspend(xb_interp *,int (*)(xb_interp *));
#endif
static SAMIR_OVL_TEXT(set) int set_ovl_entry(int k_unused, xb_interp *ip,
                                             const char *args, int *err_code)
{
    set_state  *ss;
    const char *s, *opt_start;
    char        opt[32];
    int         i, k, opt_len;

    (void)k_unused;
#ifdef SAMIR_OVL_MUTATE_ESCAPE_HOOK
    xb_interp_add_cmd_hook(ip,ovl_bad_hook,ip);
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_PAL
    { samir_pal_t hooks=*xb_interp_pal(ip);hooks.conout=ovl_bad_pal;
      samir_bind_pal(xb_interp_pal(ip),&hooks); }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_SAY
    { samir_pal_t hooks=*xb_interp_pal(ip);hooks.say_screen=ovl_bad_say;
      samir_bind_pal(xb_interp_pal(ip),&hooks); }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_PRINT
    { samir_pal_t hooks=*xb_interp_pal(ip);hooks.printout=ovl_bad_print;
      samir_bind_pal(xb_interp_pal(ip),&hooks); }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_CURSOR
    { static xb_dbcursor bad;xb_ctx *c=xb_interp_ctx(ip);bad=*c->dbcur;
      bad.recno=ovl_bad_cursor;samir_bind_cursor(c,&bad,c->dbcur_user); }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_ERROR
    { xb_ctx *c=xb_interp_ctx(ip);samir_bind_errors(c,ovl_bad_error,c->on_escape,c->on_error_text,c->on_escape_text); }
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_SUSPEND
    proc_set_suspend(ip,ovl_bad_suspend);
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_CALLBACK
    /* Rule 6: a real callback escape, formerly accepted by the static graph. */
    samir_bind_output(xb_interp_ctx(ip),alternate_output,ip);
#endif
#ifdef SAMIR_OVL_MUTATE_ESCAPE_RAW
    xb_interp_ctx(ip)->mirror_user=(void *)(uintptr_t)set_ovl_entry;
#endif
#ifdef SAMIR_OVL_MUTATE_LIVE
    /* Indirect ON ERROR is invisible to static relocation reachability. */
    xb_interp_ctx(ip)->on_error(ip,10);
#endif
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

    k = set_opt_lookup(opt);
    if (k >= 0)
        return set_opts[k].fn(ip, ss, s, err_code);

    /* An unknown SET option (NEAR is dBASE IV / Clipper, not III+) is a
     * syntax error, #10: real III+ 1.1 SF-unknown and SF-errors. */
#ifdef SAMIR_FNS_MUTANT_unknown
    if (err_code) *err_code = 16;
#else
    if (err_code) *err_code = 10;
#endif
    return -INTERP_ERR_SYNTAX;
}
SAMIR_OVL_HEADER(set, SAMIR_OVL_SET, set_ovl_entry);

static int set_cmd_hook(void *user, xb_interp *ip,
                        const char *verb, const char *args, int *err_code)
{
    (void)user;
    if (err_code) *err_code = 0;
    if (!ip || !verb) return CMD_UNKNOWN;

    /* Only handle the "SET" verb. */
    if (!s_ci_eq(verb, "SET")) return CMD_UNKNOWN;
    return SAMIR_OVL_CALL(SAMIR_OVL_SET, set_ovl_entry, 0, ip, args, err_code);
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

    /* set_register is the per-interpreter construction step, so it ALWAYS
     * applies the III+ defaults. The registry is keyed by address, and a new
     * interpreter made on a reused arena gets the SAME address as a freed one:
     * a hit is not "this interpreter's" state (initech-x5cc -- SET CENTURY ON
     * leaked from one host session into the next). */
#ifdef SET_MUTATE_KEEP_STALE
    if (is_new)
#else
    (void)is_new;
#endif
        set_init_defaults(ip, ss);

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

int set_get_escape(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return 1;
    ss = set_state_for(ip, (int *)0);
    return ss ? ss->escape : 1;
}

const set_state *set_get_state(xb_interp *ip)
{
    set_state *ss;
    if (!ip) return (const set_state *)0;
    ss = set_state_for(ip, (int *)0);
    return ss;
}
