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
 */
int samir_do(xb_interp *ip, const char *prg);

#endif /* INITECH_SAMIR_INTERP_H */
