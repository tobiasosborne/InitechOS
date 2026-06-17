/*
 * os/samir/include/samir/nav.h -- SAMIR (InitechBase) navigation commands.
 *                                  Step S5.2 (Phase 5 interpreter).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus samir/ headers. No libc, no
 * int 0x21.
 *
 * dBASE III PLUS 1.1 ONLY (docs/plans/SAMIR-implementation-plan.md Sec 2.C).
 *
 * WHAT THIS IS:
 *   The navigation layer that S5.4 (LIST/LOCATE) and S5.5 (REPLACE scope) will
 *   consume. Implements GO TOP / GO BOTTOM / GOTO n / SKIP +n / -n in PHYSICAL
 *   order (master == 0) and INDEX order (master > 0).
 *
 *   Index order: the full ascending (key -> recno) sequence is materialised from
 *   the master .ndx at GO TOP time via ndx_inorder and cached in a per-area
 *   nav_state (internal to nav.c). SKIP / GO BOTTOM operate by position in that
 *   sequence. This approach uses ONLY the existing ndx_inorder primitive and
 *   does NOT add any new ndx cursor primitive (plan S5.2 CRITICAL constraint).
 *
 * EOF / BOF semantics (dBASE III+):
 *   GO TOP clears both flags. GO BOTTOM clears both (empty table re-sets EOF).
 *   SKIP +n past the last -> EOF=1, BOF=0. SKIP -n before the first -> BOF=1,
 *   EOF=0. Read via wa_eof(env, area) / wa_bof(env, area) (workarea.h).
 *
 * GATED edges (plan sec7 / GAPS secP): SKIP at EOF/BOF exact error-vs-silent and
 *   GO to a record hidden by SET DELETED/FILTER are loud-skipped in the oracle.
 *   The implementation silently clamps at EOF/BOF.
 *
 * ASCII-clean (Rule 12). Reproducible (Rule 11). Fail loud (Rule 2).
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.2 contract + sec7 GATED.
 *   - os/samir/include/samir/workarea.h (the 10-area env + cursor accessors).
 *   - os/samir/include/samir/ndx.h (ndx_inorder -- the ONLY ndx API used here).
 *   - HELP.DBS @GO / @GOTO / @SKIP / @EOF / @BOF.
 */

#ifndef INITECH_SAMIR_NAV_H
#define INITECH_SAMIR_NAV_H

#include <stdint.h>

#include "samir/workarea.h"

/* -----------------------------------------------------------------------
 * Error codes (fail loud; Rule 2)
 * Negated on return (return -NAV_ERR_RANGE). NAV_OK == 0.
 * ----------------------------------------------------------------------- */
typedef enum {
    NAV_OK         = 0,  /* success */
    NAV_ERR_RANGE  = 1,  /* GOTO n: n out of range (< 1 or > nrec);
                          * or area out of range; or no such index slot.
                          * Corresponds to dBASE "Record out of range". */
    NAV_ERR_EMPTY  = 2,  /* navigation on a closed area */
    NAV_ERR_NOMEM  = 3   /* PAL arena exhausted materialising the index seq */
} nav_err;

/* -----------------------------------------------------------------------
 * wa_nav_reset: invalidate the cached index sequence for area `area`.
 *
 * Call when:
 *   - wa_close(env, area) is called (so a re-USE starts fresh).
 *   - wa_set_order(env, area, ...) changes the master order.
 *   - Any call that makes the cached sequence stale.
 *
 * Silently ignored for out-of-range area (no return code -- callers never
 * need to distinguish; the next navigation call re-materialises). This is
 * a no-op if the sequence was not yet built.
 *
 * Ref: nav.c; plan S5.2 (index-order approach).
 * ----------------------------------------------------------------------- */
void wa_nav_reset(int area);

/* -----------------------------------------------------------------------
 * GO TOP: position at the first record in the active order.
 *
 *   Physical (master==0): recno 1.
 *   Index   (master>0):   first key's recno (materialises seq if needed).
 *   Empty table: table stays at EOF (wa_eof(env,area)==1); returns NAV_OK.
 *
 * Clears EOF and BOF on a non-empty table.
 * Returns NAV_OK or -(nav_err).
 *
 * Ref: HELP.DBS @GO TOP; plan S5.2 contract.
 * ----------------------------------------------------------------------- */
int wa_nav_go_top(wa_env *env, int area);

/* -----------------------------------------------------------------------
 * GO BOTTOM: position at the last record in the active order.
 *
 *   Physical: recno nrec. Index: last key's recno.
 *   Empty table: NAV_OK (EOF stays set).
 *
 * Clears EOF and BOF on a non-empty table.
 * Returns NAV_OK or -(nav_err).
 *
 * Ref: HELP.DBS @GO BOTTOM; plan S5.2 contract.
 * ----------------------------------------------------------------------- */
int wa_nav_go_bottom(wa_env *env, int area);

/* -----------------------------------------------------------------------
 * GOTO n: position at physical record n (1-based).
 *
 * GOTO is ALWAYS by physical recno regardless of the master order.
 * Ref: HELP.DBS @GOTO "go to record number".
 *
 * n out of range (< 1 or > nrec) -> return -NAV_ERR_RANGE (fail loud).
 * Clears EOF and BOF.
 * Returns NAV_OK or -(nav_err).
 * ----------------------------------------------------------------------- */
int wa_nav_goto(wa_env *env, int area, uint32_t recno);

/* -----------------------------------------------------------------------
 * SKIP n: advance (n > 0) or retreat (n < 0) n records in the ACTIVE order.
 * SKIP with n=0 is a no-op. Default usage: SKIP = SKIP +1.
 *
 *   Physical:
 *     SKIP +n: recno += n; if > nrec -> EOF (recno = nrec; EOF=1, BOF=0).
 *     SKIP -n: recno -= n; if < 1   -> BOF (recno = 1;    BOF=1, EOF=0).
 *   Index:
 *     SKIP +n: ordinal += n; if >= seq_len -> EOF (ordinal=seq_len; EOF=1).
 *     SKIP -n: ordinal -= n; if < 0       -> BOF (ordinal=0; BOF=1).
 *
 * GATED (loud-skipped in the oracle): exact error-vs-silent at EOF/BOF.
 * Our implementation silently clamps (no error on SKIP at EOF/BOF).
 *
 * Mutation hook (Rule 6 -- -DNAV_MUTATE_SKIP):
 *   Physical: advances by n+1 (off-by-one step). The walk assertions go RED.
 *   Index: uses physical recno order instead of the materialised index seq,
 *   so the index-walk recno sequence diverges from ndx_inorder's order.
 *
 * Returns NAV_OK or -(nav_err) (never NAV_ERR_RANGE on EOF/BOF -- gated).
 *
 * Ref: HELP.DBS @SKIP; plan S5.2 contract + sec7 GATED.
 * ----------------------------------------------------------------------- */
int wa_nav_skip(wa_env *env, int area, int32_t n);

/* -----------------------------------------------------------------------
 * Index sequence accessors (for the oracle -- read the cached seq).
 *
 * wa_nav_ordpos  : current 0-based ordinal position in the index seq.
 *                  seq_len means "at virtual EOF".
 * wa_nav_seqlen  : total entries in the materialized index seq (== nrec for
 *                  a fully-populated index, <= nrec if NAV_MAX_RECS capped).
 * wa_nav_seq_recno: the recno at ordinal `ord` in the cached seq, or 0 if
 *                   out of range.
 *
 * These are internal to the nav+oracle layer; the main interpreter accesses
 * the cursor state via wa_recno / wa_eof / wa_bof (workarea.h).
 *
 * Ref: nav.c; plan S5.2 oracle ("walk in both orders").
 * ----------------------------------------------------------------------- */
uint32_t wa_nav_ordpos  (int area);
uint32_t wa_nav_seqlen  (int area);
uint32_t wa_nav_seq_recno(int area, uint32_t ord);

#endif /* INITECH_SAMIR_NAV_H */
