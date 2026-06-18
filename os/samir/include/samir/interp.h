/*
 * os/samir/include/samir/interp.h -- SAMIR (InitechBase) interpreter handle.
 *                                     Step S5.1 (Phase 5 interpreter foundation).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers. No libc, no
 * int 0x21 -- all OS contact is through the PAL vtable carried by the env.
 *
 * dBASE III PLUS 1.1 ONLY (docs/plans/SAMIR-implementation-plan.md Sec 2.C).
 *
 * WHAT THIS IS:
 *   xb_interp is the resident interpreter's whole runtime state: the work-area
 *   environment (workarea.h: 10 areas + the selected one + the RECNO cursors +
 *   the resolve glue), the global SET state, the scratch arena the evaluator
 *   uses for synthesized C results, and the injected today() date for DATE().
 *   It is the single object threaded through the Phase-5 surface
 *   (samir_repl / samir_do) per plan Sec 8.2.
 *
 * S5.1 SCOPE (this step IMPLEMENTS):
 *   - xb_interp_make / xb_interp_free : construct/teardown the interpreter over
 *     a PAL (builds the wa_env, the scratch arena, the eval ctx with the resolve
 *     hook bound).
 *   - xb_interp_env  : the work-area environment (USE/CLOSE/SELECT live there).
 *   - xb_interp_ctx  : the evaluation context, with ctx->resolve already bound to
 *     the work-area resolve hook (wa_bind_ctx). The executor (S5.3) and query
 *     (S5.4) reuse this ctx to evaluate FOR/WHILE/index-key expressions against
 *     the selected area's current record.
 *   - xb_interp_eval_str : convenience -- lex+parse+eval one expression string
 *     against the current area's current record, using this interp's ctx. This
 *     is the smallest end-to-end exercise of the convergence (codec + eval +
 *     resolve) and is what the S5.1 oracle drives. The fuller statement executor
 *     (command dispatch, DO WHILE/IF, LIST/REPLACE) is S5.3..S5.5; they build on
 *     this same lex/parse/eval-through-ctx path.
 *
 * S5.1 -> S5.2..S5.8 BOUNDARY (DEFERRED -- declared here, NOT implemented in S5.1):
 *   - samir_do  (run a .prg) : the statement executor + control flow are S5.3.
 *   - samir_repl(the dot prompt loop) : S5.8 (samir_main.c).
 *   These two are the plan Sec 8.2 top-level surface. S5.1 DECLARES them so the
 *   handle/contract is fixed for the later steps but does NOT define them; the
 *   S5.1 translation unit provides everything above the dashed line and leaves
 *   samir_do/samir_repl to S5.3/S5.8. (A test linking only the S5.1 object will
 *   not reference samir_do/samir_repl, so no undefined-symbol arises.)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h> and samir/ headers.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md Sec 8.2 (interp.h surface:
 *     samir_repl / samir_do); S5.1 contract; Phase 5 header; S5.8 (REPL).
 *   - os/samir/include/samir/workarea.h (wa_env + the resolve glue).
 *   - os/samir/include/samir/eval.h (xb_ctx + xb_lex/xb_parse/xb_eval).
 *   - os/samir/include/samir/pal.h (the ONLY OS surface; arena + today()).
 */

#ifndef INITECH_SAMIR_INTERP_H
#define INITECH_SAMIR_INTERP_H

#include <stdint.h>

#include "samir/pal.h"
#include "samir/value.h"
#include "samir/eval.h"
#include "samir/workarea.h"

/* -----------------------------------------------------------------------
 * Error codes (fail loud; Rule 2)
 * Negated on return (return -INTERP_ERR_NOMEM, etc.). INTERP_OK == 0.
 * ----------------------------------------------------------------------- */
typedef enum {
    INTERP_OK         = 0,  /* success */
    INTERP_ERR_NOMEM  = 1,  /* PAL arena exhausted building the interpreter */
    INTERP_ERR_LEX    = 2,  /* xb_interp_eval_str: lex error (see *err_code) */
    INTERP_ERR_PARSE  = 3,  /* xb_interp_eval_str: parse error (see *err_code) */
    INTERP_ERR_EVAL   = 4   /* xb_interp_eval_str: eval error (see *err_code) */
} interp_err;

/*
 * Tunables for the interpreter's internal arenas. Chosen generously for the
 * host oracle; the artifact PAL sizes its single arena to cover them. These are
 * the bytes set aside for the evaluator's synthesized-C scratch (concat / fn
 * results). The work-area record caches allocate from the env's PAL arena
 * directly, not from here.
 */
#define INTERP_SCRATCH_CAP  4096u  /* eval ctx scratch arena (C+C / fn results) */

/* -----------------------------------------------------------------------
 * Opaque interpreter handle. Layout lives in os/samir/cmd/workarea.c
 * (the S5.1 translation unit). Reach state through the accessors below.
 * ----------------------------------------------------------------------- */
typedef struct xb_interp xb_interp;

/* -----------------------------------------------------------------------
 * Lifecycle
 * ----------------------------------------------------------------------- */

/*
 * xb_interp_make: construct the interpreter over a PAL.
 *
 * Allocates (from the PAL arena): the xb_interp, the wa_env (10 work areas), and
 * the eval scratch arena (INTERP_SCRATCH_CAP bytes). Initialises the eval ctx:
 *   - set_exact = 0 (III+ default OFF; SET EXACT is S5.6),
 *   - scratch / scratch_cap pointed at the scratch arena,
 *   - ctx_today = the env PAL's injected today() converted to a JDN double
 *     (reproducible; DATE() reads it -- S3.5),
 *   - resolve / user bound to the work-area resolve hook (wa_bind_ctx).
 *
 * Returns a non-NULL xb_interp* on success, NULL on arena exhaustion. The PAL is
 * retained for the interpreter's lifetime.
 *
 * Ref: plan S5.1; eval.h xb_ctx; workarea.h wa_env_make + wa_bind_ctx; pal.h today().
 */
xb_interp *xb_interp_make(samir_pal_t *pal);

/*
 * xb_interp_free: tear down the interpreter -- close all open work areas and
 * reset the PAL arena to the mark taken at make time. NULL is a no-op.
 */
void xb_interp_free(xb_interp *ip);

/* -----------------------------------------------------------------------
 * Accessors
 * ----------------------------------------------------------------------- */

/* xb_interp_env: the work-area environment (USE/CLOSE/SELECT/wa_resolve). */
wa_env *xb_interp_env(xb_interp *ip);

/* xb_interp_ctx: the evaluation context (resolve hook already bound). The
 * executor (S5.3+) reuses this to evaluate expressions against the current
 * area's current record. scratch_used is reset by xb_eval per call. */
xb_ctx *xb_interp_ctx(xb_interp *ip);

/* xb_interp_pal: the PAL the interpreter was made over. */
samir_pal_t *xb_interp_pal(xb_interp *ip);

/* -----------------------------------------------------------------------
 * Expression evaluation (the S5.1 end-to-end convergence path)
 * ----------------------------------------------------------------------- */

/*
 * xb_interp_eval_str: lex + parse + evaluate one expression string against the
 * SELECTED work area's CURRENT record.
 *
 *   ip       : the interpreter.
 *   expr     : the expression source bytes (need not be NUL-terminated).
 *   len      : byte length of `expr`.
 *   out      : receives the result xb_val on success.
 *   err_code : output detail. On INTERP_ERR_LEX it is an xb_lex_err; on
 *              INTERP_ERR_PARSE an xb_parse_err; on INTERP_ERR_EVAL an
 *              xb_eval_err (the dBASE catalog ordinal, e.g. 9 mismatch, or the
 *              negative XBEE_UNBOUND if a field name has no open selected area).
 *              Set to 0 on success.
 *
 * The expression's field-name identifiers resolve through the bound work-area
 * hook (workarea.h wa_resolve): they bind to the selected area's current record.
 * Thus "LASTNAME + ' ' + FIRSTNAME" over a USEd table yields the concatenation,
 * and a bare "DEPARTURE" yields the date value of the current record.
 *
 * Returns INTERP_OK (0) with *out set, or -(interp_err) with *err_code naming
 * the stage-specific reason. Uses bounded internal token / node pools sized for
 * the expressions S5.1..S5.8 produce (see the implementation).
 *
 * Ref: eval.h xb_lex/xb_parse/xb_eval + xb_ctx.resolve; workarea.h wa_resolve;
 *      plan S5.1 (resolve glue) / Sec 8.2 (this is the seed of samir_do's
 *      per-statement expression evaluation).
 */
int xb_interp_eval_str(xb_interp *ip, const char *expr, uint32_t len,
                       xb_val *out, int *err_code);

/* -----------------------------------------------------------------------
 * Plan Sec 8.2 top-level surface -- DECLARED in S5.1, DEFINED in later steps.
 * (S5.1 does NOT implement these; the executor is S5.3, the REPL is S5.8.)
 * ----------------------------------------------------------------------- */

/*
 * samir_repl: the dot-prompt read/eval/print loop. Reads a line via the PAL,
 * parses + executes it, renders results / the 151-code error catalog, until EOF
 * or QUIT. Returns INTERP_OK on a clean exit. IMPLEMENTED IN S5.8 (samir_main.c).
 * Ref: plan Sec 8.2; S5.8.
 */
int samir_repl(samir_pal_t *pal, xb_interp *ip);

/*
 * samir_do: run a .prg program (a sequence of statements with control flow).
 * `prg` is the program source (NUL-terminated). Returns INTERP_OK on completion.
 * IMPLEMENTED IN S5.3 (the statement executor + control flow). DECLARED here so
 * the handle/contract is stable for the swarm. Ref: plan Sec 8.2; S5.3.
 *
 * On a run-time error (a guard that is not Logical, a bad expression, a verb the
 * dispatch chain does not recognise, or malformed control-flow structure) the
 * executor STOPS and returns a negative interp_err; the dBASE catalog ordinal /
 * detail is available through samir_last_error(ip). Fail loud (Rule 2): no
 * silent skip of a broken statement.
 */
int samir_do(xb_interp *ip, const char *prg);

/* =======================================================================
 * S5.3 -- statement executor + control-flow SPINE (initech-7az.4)
 *
 * This is the dot-prompt interpreter's STATEMENT layer: it tokenises a program
 * into lines, dispatches each command line on its leading verb, and runs the
 * structured control-flow constructs (DO WHILE/ENDDO, IF/ELSE/ENDIF,
 * DO CASE/CASE/OTHERWISE/ENDCASE, LOOP, EXIT). It owns the memory-variable
 * (memvar) table and the STORE / "<name> = <expr>" verbs.
 *
 * DESIGN FOR EXTENSION (S5.4 query, S5.5 mutation, S5.6 SET, S5.7 procedures):
 *   The executor dispatches a command in two stages:
 *     1. BUILT-IN spine verbs handled directly here: the control-flow keywords,
 *        STORE, "<name> = <expr>" memvar assignment, and the structural verbs
 *        that already have an engine (SELECT n / SELECT <alias>). RELEASE is
 *        handled for the memvar table this step owns.
 *     2. Anything else is handed to a registered COMMAND HOOK
 *        (xb_interp_set_cmd_hook). A later step registers ONE hook that adds its
 *        verbs (LIST/DISPLAY/?, REPLACE/APPEND, SET ..., DO/PROCEDURE) without
 *        editing flow.c. The hook returns CMD_OK (handled), CMD_UNKNOWN (not my
 *        verb -- the executor then fails loud #16), or a negative error.
 *   This keeps S5.3 the stable SPINE: new verbs are additive registrations, the
 *   control-flow engine never changes.
 *
 * MEMVARS (kept OFF workarea.c -- a parallel lane owns that file):
 *   At samir_do entry the executor SAVES the interp ctx's existing resolve/user
 *   (the work-area field resolver bound by xb_interp_make) and installs a COMPOSED
 *   resolver that checks the memvar table first, then delegates to the saved
 *   work-area resolver. STORE/= create or update memvars; field references in the
 *   SAME expression still resolve through the delegate. ALL OTHER ctx fields
 *   (set_exact, scratch, ctx_today, and any cursor hook a parallel lane adds) are
 *   left untouched -- only resolve/user are swapped, and they are restored on
 *   return. Memvar storage (names + C bytes) lives in the PAL arena.
 *
 * GUARD-MUST-BE-LOGICAL (the headline S5.3 rule; plan S5.3 + dBASE semantics):
 *   The condition of IF, DO WHILE and each DO CASE CASE is evaluated through the
 *   evaluator and MUST be Logical (XB_L). A non-Logical guard is a FAIL-LOUD
 *   error #37 "Not a Logical expression." (dbase_msg_codes.tsv line 45,
 *   XBEE_NOT_LOGICAL) -- there is NO C/Clojure-style truthiness (0/""/blank are
 *   NOT "false"). This is enforced, not optional.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.3 ("command dispatch; DO WHILE/
 *     ENDDO, IF/ELSE/ENDIF, DO CASE, LOOP/EXIT; guard-must-be-Logical").
 *   - os/samir/include/samir/eval.h (xb_ctx + xb_lex/xb_parse/xb_eval;
 *     XBEE_NOT_LOGICAL = #37).
 *   - spec/samir/dbase_msg_codes.tsv (#37 "Not a Logical expression.").
 *   - os/samir/include/samir/workarea.h (wa_resolve; the delegate).
 * ======================================================================= */

/*
 * Command-hook return convention. A registered command hook returns:
 *   CMD_OK      ( 0): the hook recognised + executed the verb.
 *   CMD_UNKNOWN ( 1): not a verb this hook handles; the executor fails loud.
 *   < 0          : a run-time error (negative); the executor stops and surfaces
 *                  it through samir_last_error. The hook sets *err_code to the
 *                  dBASE catalog ordinal where one applies.
 */
typedef enum {
    CMD_OK      = 0,
    CMD_UNKNOWN = 1
} cmd_status;

/*
 * xb_cmd_hook: a registered command-line handler (S5.4..S5.7 each supply one).
 *
 *   user : the hook's own state (xb_interp_set_cmd_hook stores it).
 *   ip   : the interpreter (the hook reaches the env / ctx / memvars through it).
 *   verb : the upper-cased leading keyword of the line (NUL-terminated).
 *   args : the remainder of the line after the verb (the operands), trimmed of
 *          leading blanks; NUL-terminated; may be empty. NOT a copy you own --
 *          valid only for the duration of the call.
 *   err_code : set to the dBASE catalog ordinal on a negative return.
 *
 * Returns a cmd_status (>=0) or a negative interp/eval error.
 */
typedef int (*xb_cmd_hook)(void *user, xb_interp *ip,
                           const char *verb, const char *args, int *err_code);

/*
 * COMMAND-HOOK CHAIN (S5.4 extension; was a single hook in S5.3).
 *
 * The executor (flow.c exec_command) now tries a CHAIN of registered command
 * hooks in registration order. Each is offered the verb; it returns CMD_OK
 * (handled -> stop), CMD_UNKNOWN (not my verb -> try the next hook), or a
 * negative error (a real run-time error -> stop + surface it). If every hook
 * returns CMD_UNKNOWN the executor fails loud #16 (Unrecognized command verb).
 *
 * This lets S5.4 (query), S5.5 (mutation), S5.6 (SET) and S5.7 (procedures)
 * each register their OWN module via xb_interp_add_cmd_hook WITHOUT editing
 * flow.c or each other -- the spine stays the stable S5.3 control-flow engine
 * and new verbs are additive registrations. The chain is bounded
 * (INTERP_MAX_CMD_HOOKS); overflow is a no-op (fail-loud is the caller's, since
 * a verb then simply goes unrecognized -- never silently mis-dispatched).
 *
 * Ref: plan S5.4..S5.7 (each adds a module); CLAUDE.md Rule 2 (fail loud).
 */
#define INTERP_MAX_CMD_HOOKS 8

/*
 * xb_interp_add_cmd_hook: APPEND (hook,user) to the command-hook chain. The
 * hook is tried after all earlier-registered hooks. Returns INTERP_OK, or
 * -INTERP_ERR_DEPTH if the chain is full (fail loud; Rule 2). A NULL hook is a
 * no-op returning INTERP_OK. The S5.4..S5.7 modules each call this once before
 * samir_do.
 */
int xb_interp_add_cmd_hook(xb_interp *ip, xb_cmd_hook hook, void *user);

/*
 * xb_interp_set_cmd_hook: CLEAR the chain, then register (hook,user) as its sole
 * entry. Passing NULL just clears the chain (the spine-only set remains).
 *
 * Retained for the S5.3 single-hook callers and tests. New modules (S5.4..S5.7)
 * compose via xb_interp_add_cmd_hook instead, so they do not clobber each other.
 */
void xb_interp_set_cmd_hook(xb_interp *ip, xb_cmd_hook hook, void *user);

/*
 * xb_interp_store: create-or-update a memvar `name` (NUL-terminated, case-
 * insensitive, folded to upper-case) with value `v`. This is the engine behind
 * STORE <expr> TO <name> and "<name> = <expr>". A Character/Memo value's bytes
 * are COPIED into the memvar arena so the memvar outlives the eval scratch.
 *
 * Returns INTERP_OK or a negative interp_err (arena exhaustion / table full).
 * Available to S5.5 (REPLACE assignment also stores) and S5.7 (PARAMETERS /
 * PRIVATE seed the table).
 */
int xb_interp_store(xb_interp *ip, const char *name, const xb_val *v);

/*
 * xb_interp_get_memvar: look up memvar `name` (NUL-terminated, case-insensitive).
 * On success returns 0 and fills *out with the stored value (C/M bytes point into
 * the memvar arena, stable until the memvar is overwritten/released). Returns
 * non-zero if no such memvar exists. Used by the oracle and by S5.7 scope checks.
 */
int xb_interp_get_memvar(xb_interp *ip, const char *name, xb_val *out);

/*
 * xb_interp_release_all: drop every memvar (RELEASE ALL). The arena bytes are not
 * individually reclaimed (bump arena), but the names become invisible to resolve
 * and to xb_interp_get_memvar. S5.7 narrows this to PRIVATE/PUBLIC scopes.
 */
void xb_interp_release_all(xb_interp *ip);

/*
 * samir_last_error: the dBASE catalog ordinal (or engine code) of the last
 * run-time error raised by samir_do, or 0 if the last run completed cleanly.
 * E.g. after "IF 1" it is 37 (XBEE_NOT_LOGICAL); after an unmatched ENDIF it is
 * INTERP_ERR_STRUCT. Lets a caller / oracle assert the precise failure (Law 2).
 */
int samir_last_error(xb_interp *ip);

/* =======================================================================
 * S5.7 -- procedure / memvar SCOPE model (initech-7az.8)
 *
 * The memvar table (owned by flow.c, S5.3) grows a DOWNWARD-stacking scope:
 * each DO-call (cmd/proc.c) brackets the called routine's body between a
 * xb_interp_scope_enter() / xb_interp_scope_leave() pair. A memvar carries the
 * DO-call LEVEL at which it was created; on scope_leave every var at the leaving
 * level is released and any caller/PUBLIC var it shadowed is restored. This is
 * the III+ dynamic, stack-based scoping model (memory-variables.md sec 3):
 *
 *   - dot-prompt / main level = level 0.
 *   - PUBLIC <list>  : declares level-0 GLOBALS that survive RETURN
 *     (xb_interp_declare_public). GATED: the uninitialized PUBLIC VALUE
 *     (Clipper inits to .F.; III+ 1.1 parity unconfirmed -- memory-variables.md
 *     Open-question 3) -- we init to XB_U and the oracle loud-skips its value.
 *   - PRIVATE <list> : creates a FRESH level-current var that HIDES any visible
 *     same-named caller/PUBLIC var until scope_leave (xb_interp_declare_private).
 *   - PARAMETERS <list> + DO ... WITH : binds the WITH values BY POSITION into
 *     fresh level-current PRIVATE vars (xb_interp_store_param). GATED: by-REF
 *     write-back exactness (memory-variables.md Open-question 5) -- not modelled;
 *     the oracle loud-skips it. The bound values are by-VALUE copies.
 *   - auto-private: a STORE/= to a name NOT currently visible creates it at the
 *     current level (handled inside xb_interp_store; sec 3.1 rule 2). A STORE/=
 *     to a VISIBLE name modifies that var (rule 1) -- this is how a callee writes
 *     a caller's still-visible var.
 *
 * These are the additive hooks cmd/proc.c uses; the executor + every S5.1..S5.6
 * behavior is unchanged (a program that never DO-calls runs entirely at level 0).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/language/memory-variables.md sec 3 (PUBLIC/PRIVATE/
 *     PARAMETERS scope, the auto-private rule, hiding+restore on RETURN).
 *   - ../dbase3-decomp/specs/commands/control-flow-and-procedures.md sec 7-8
 *     (DO/PROCEDURE/PARAMETERS/RETURN/DO WITH; by-ref/by-value).
 * ======================================================================= */

/*
 * xb_interp_scope_enter: push one DO-call level (cmd/proc.c calls this before
 * running a procedure body). Subsequent PRIVATE / PARAMETERS / auto-private
 * memvars are created AT this new level. Returns the new level (>=1), or a
 * negative interp_err on depth overflow (fail loud; Rule 2).
 */
int xb_interp_scope_enter(xb_interp *ip);

/*
 * xb_interp_scope_leave: pop the current DO-call level (cmd/proc.c calls this on
 * RETURN / end-of-procedure). Releases EVERY level-current memvar and RESTORES
 * any caller/PUBLIC var a PRIVATE/PARAMETERS declaration shadowed at this level.
 * PUBLIC globals are untouched. NULL/at-level-0 is a no-op.
 */
void xb_interp_scope_leave(xb_interp *ip);

/*
 * xb_interp_declare_public: declare `name` (NUL-terminated, case-insensitive) a
 * PUBLIC global. If a same-named visible var already exists it is left as-is
 * (PUBLIC must precede the first assignment); otherwise a level-0 PUBLIC var is
 * created with an UNINITIALIZED value (XB_U). Returns INTERP_OK or a negative
 * interp_err. (memory-variables.md sec 3.2.)
 */
int xb_interp_declare_public(xb_interp *ip, const char *name);

/*
 * xb_interp_declare_private: declare `name` PRIVATE to the current level. Any
 * currently-visible same-named var (a caller's or a PUBLIC) is HIDDEN until the
 * current level is left; a fresh, UNINITIALIZED (XB_U) level-current var of that
 * name is created. Returns INTERP_OK or a negative interp_err.
 * (memory-variables.md sec 3.3.)
 */
int xb_interp_declare_private(xb_interp *ip, const char *name);

/*
 * xb_interp_store_param: bind a DO ... WITH argument value into a PARAMETERS
 * name BY POSITION, as a fresh level-current PRIVATE var (a by-VALUE copy; the
 * by-reference write-back is GATED -- see the scope header). Same storage
 * contract as xb_interp_store (C/M bytes copied to the memvar arena). Returns
 * INTERP_OK or a negative interp_err. (control-flow-and-procedures.md sec 8.)
 */
int xb_interp_store_param(xb_interp *ip, const char *name, const xb_val *v);

/* Additional interp_err ordinals introduced by S5.3 (additive; INTERP_OK==0 and
 * the S5.1 codes are unchanged). Returned negated from samir_do, recorded by
 * samir_last_error. */
enum {
    INTERP_ERR_STRUCT  = 5,  /* malformed control structure: unmatched ELSE/
                              * ENDIF/ENDDO/CASE/OTHERWISE/ENDCASE, EXIT/LOOP
                              * outside a loop, missing terminator (fail loud) */
    INTERP_ERR_UNKNOWN_CMD = 6,  /* a verb no spine rule and no hook handled
                                  * (dBASE #16 "Unrecognized command verb.") */
    INTERP_ERR_SYNTAX  = 7,  /* a statement's operands are syntactically wrong
                              * (e.g. STORE without TO, bad assignment target) */
    INTERP_ERR_DEPTH   = 8   /* control-flow / line-buffer capacity exceeded */
};

#endif /* INITECH_SAMIR_INTERP_H */
