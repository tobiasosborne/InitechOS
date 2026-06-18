/*
 * os/samir/include/samir/set.h -- SAMIR (InitechBase) SET state.
 *                                  Step S5.6 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> and the samir/ engine headers. No libc,
 * no int 0x21 -- all OS contact goes through the PAL. ASCII-clean (Rule 12).
 * Fail loud (Rule 2). Reproducible (Rule 11).
 *
 * dBASE III PLUS 1.1 ONLY (plan Sec 2.C).
 *
 * WHAT THIS IS (the S5.6 contract):
 *   The global SET state for one interpreter session. Covers the options named
 *   in the S5.6 task:
 *
 *     EXACT     toggle  OFF   write xb_ctx.set_exact; governs C= begins-with
 *     DECIMALS  value   2     stored; formatter-honoring DEFERRED (follow-up)
 *     DATE      value   AMERICAN  stored keyword; formatter-honoring DEFERRED
 *     CENTURY   toggle  OFF   stored; formatter-honoring DEFERRED
 *     NEAR      toggle  OFF   stored; (III+ has no SOFTSEEK; NEAR is Clipper-ism
 *                                      => fail-loud parse error on SET NEAR)
 *     ORDER     value   --    calls wa_set_order on the selected work area
 *     INDEX     value   --    GATED (work-area plumbing not yet present): parse,
 *                             store intent, loud-skip runtime effect
 *     FILTER    value   --    GATED: parse, store intent, loud-skip runtime effect
 *     RELATION  value   --    GATED: parse, store intent, loud-skip runtime effect
 *     TALK      toggle  ON    stored; runtime effect (REPLACE/APPEND counts) DEFERRED
 *     SAFETY    toggle  ON    stored; file-overwrite guard DEFERRED
 *
 *   SET NEAR: "NEAR" is not a III+ SET option (it is a Clipper/dBASE IV addition;
 *   III+ has no SOFTSEEK). The parser fails loud #16 ("Unrecognized command verb."
 *   mapped to an INTERP_ERR_SYNTAX) on "SET NEAR ON|OFF" so we never silently
 *   accept IV syntax. Ref: set-commands.md section 5 (completeness oracle vs
 *   HELP_topics.txt: NEAR absent from III+ topic list).
 *
 * DEFERRED (formatter-honoring):
 *   SET DECIMALS/DATE/CENTURY effects on formatted output require fn_builtins.c
 *   and rt.c changes, which are owned by a parallel lane. Those effects are
 *   loud-skipped in the oracle with a note citing the follow-up bead.
 *
 * GATED (runtime effect):
 *   SET INDEX/FILTER/RELATION need work-area plumbing not yet present. The parser
 *   accepts the syntax, stores the raw text, and loud-skips the runtime effect
 *   in the oracle. Each is noted as a follow-up bead.
 *
 * set_register(ip): install the SET command hook into ip's command-hook chain.
 *   Called once after xb_interp_make (before samir_do / samir_repl). Returns
 *   INTERP_OK or a negative interp_err (chain full or out of state slots).
 *
 * Getters: expose stored state so the oracle can assert defaults + overrides
 *   without reaching into the struct directly.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.6 (contract + oracle).
 *   - ../dbase3-decomp/specs/commands/set-commands.md (full SET grammar, defaults,
 *     III+ vs IV flags, SET NEAR absent from III+ HELP_topics).
 *   - ../dbase3-decomp/re/mint-results-002.md (EXACT=OFF default [verified],
 *     DECIMALS=2 default [verified]).
 *   - ../dbase3-decomp/re/mint-results-003.md (DATE=AMERICAN, CENTURY=OFF [verified]).
 *   - os/samir/include/samir/interp.h (xb_cmd_hook + chain; INTERP_OK/ERR codes).
 *   - os/samir/include/samir/eval.h   (xb_ctx.set_exact field).
 *   - os/samir/include/samir/workarea.h (wa_set_order -- called for SET ORDER).
 */

#ifndef INITECH_SAMIR_SET_H
#define INITECH_SAMIR_SET_H

#include <stdint.h>
#include "samir/interp.h"

/* -----------------------------------------------------------------------
 * SET DATE format keywords (the III+ keyword set from set-commands.md Sec 3.2).
 * [verified: harbour set.txt:130-140 + HELP_topics SET DATE]
 * ----------------------------------------------------------------------- */
typedef enum {
    SET_DATE_AMERICAN = 0,   /* MM/DD/YY  -- the III+ default */
    SET_DATE_ANSI     = 1,   /* YY.MM.DD  */
    SET_DATE_BRITISH  = 2,   /* DD/MM/YY  */
    SET_DATE_ITALIAN  = 3,   /* DD-MM-YY  */
    SET_DATE_FRENCH   = 4,   /* DD/MM/YY  */
    SET_DATE_GERMAN   = 5,   /* DD.MM.YY  */
    SET_DATE_JAPAN    = 6,   /* YY/MM/DD  */
    SET_DATE_USA      = 7    /* MM-DD-YY  */
} set_date_fmt;

/* -----------------------------------------------------------------------
 * set_state: the S5.6 global SET environment record.
 *
 * Stored in a bounded static registry keyed by xb_interp* (the engine is
 * single-threaded / cooperative -- no malloc, no concurrency). The registry is
 * bounded by SET_REGISTRY (max concurrent live interpreters). Overflow is
 * fail-loud: set_register returns -INTERP_ERR_NOMEM if the registry is full.
 *
 * set_exact is NOT stored here: it is written directly into xb_ctx (the field
 * xb_interp_ctx(ip)->set_exact) so the evaluator picks it up immediately
 * without an additional indirection. The getter set_get_exact reads it back
 * from the ctx for symmetry.
 *
 * Fields for GATED options (INDEX/FILTER/RELATION) store the raw option text
 * (NUL-terminated, bounded) so the oracle can assert they were stored without
 * triggering the deferred runtime plumbing.
 * ----------------------------------------------------------------------- */

/* Maximum raw text retained for GATED options (INDEX/FILTER/RELATION). */
#define SET_GATED_TEXT_CAP  128

typedef struct {
    /* --- Stored toggles (non-EXACT) --- */
    int  century;           /* 0 = OFF (default), 1 = ON */
    int  talk;              /* 1 = ON (default), 0 = OFF */
    int  safety;            /* 1 = ON (default), 0 = OFF */

    /* --- Stored value options --- */
    int          decimals;  /* 2 = default; >= 0 */
    set_date_fmt date_fmt;  /* SET_DATE_AMERICAN = default */

    /* --- GATED options: parsed + stored, runtime effect deferred --- */
    char index_text   [SET_GATED_TEXT_CAP]; /* SET INDEX TO <list>    */
    char filter_text  [SET_GATED_TEXT_CAP]; /* SET FILTER TO <cond>   */
    char relation_text[SET_GATED_TEXT_CAP]; /* SET RELATION TO <args> */
    int  have_index;         /* 1 if SET INDEX was called   */
    int  have_filter;        /* 1 if SET FILTER was called  */
    int  have_relation;      /* 1 if SET RELATION was called */
} set_state;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/*
 * set_register: install the SET command hook into ip's command-hook chain.
 * Initialises the set_state for this interpreter to the III+ 1.1 defaults:
 *   set_exact = 0 (OFF) -- written to xb_interp_ctx(ip)->set_exact
 *   decimals  = 2
 *   date_fmt  = SET_DATE_AMERICAN
 *   century   = 0 (OFF)
 *   talk      = 1 (ON)
 *   safety    = 1 (ON)
 *   GATED texts empty / have_* = 0
 *
 * Returns INTERP_OK on success, or:
 *   -INTERP_ERR_NOMEM  if the set-state registry is full
 *   -INTERP_ERR_DEPTH  if the command-hook chain is full
 *
 * Ref: interp.h xb_interp_add_cmd_hook.
 */
int set_register(xb_interp *ip);

/*
 * Getters: read the set_state for `ip`. Return the current stored value, or the
 * default if set_register has not been called for this interpreter.
 *
 * set_get_exact: reads xb_interp_ctx(ip)->set_exact directly (the evaluator's
 *   live field). Returns 0 (OFF) if ip is NULL.
 * set_get_decimals: returns the stored DECIMALS count (2 default).
 * set_get_date_fmt: returns the stored date format (SET_DATE_AMERICAN default).
 * set_get_century:  returns 0 (OFF) or 1 (ON).
 * set_get_talk:     returns 1 (ON) or 0 (OFF).
 * set_get_safety:   returns 1 (ON) or 0 (OFF).
 */
int          set_get_exact   (xb_interp *ip);
int          set_get_decimals(xb_interp *ip);
set_date_fmt set_get_date_fmt(xb_interp *ip);
int          set_get_century (xb_interp *ip);
int          set_get_talk    (xb_interp *ip);
int          set_get_safety  (xb_interp *ip);

/*
 * set_get_state: return a pointer to the set_state for `ip`, or NULL if
 * set_register has not been called for this interpreter. Used by the oracle
 * to inspect GATED fields (index_text, filter_text, relation_text).
 */
const set_state *set_get_state(xb_interp *ip);

#endif /* INITECH_SAMIR_SET_H */
