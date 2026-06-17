/*
 * os/samir/include/samir/workarea.h -- SAMIR (InitechBase) work-area model.
 *                                       Step S5.1 (Phase 5 interpreter foundation).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers (pal.h, value.h,
 * dbf.h, dbt.h, ndx.h, eval.h). No libc, no int 0x21 -- all OS contact is
 * through the PAL vtable carried by each open table.
 *
 * dBASE III PLUS 1.1 ONLY (docs/plans/SAMIR-implementation-plan.md Sec 2.C).
 *
 * WHAT THIS IS (the Phase-5 convergence point):
 *   The work-area model is where the four storage/eval subsystems meet. A work
 *   area is the runtime binding of ONE open table:
 *       .dbf record codec      (dbf.h)   -- the rows + field descriptors
 *     + .dbt memo store        (dbt.h)   -- the M-field text, opened iff has_memo
 *     + up to NDX_PER_AREA .ndx indexes  (ndx.h) -- with one designated MASTER
 *     + a current-record cursor (RECNO)  -- 1-based; initialised to 1 on USE
 *   The interpreter's expression evaluator (eval.h) binds field names in
 *   expressions to the CURRENT record of the SELECTED area through the
 *   wa_resolve hook (the load-bearing glue, see "THE RESOLVE GLUE" below).
 *
 * dBASE III PLUS has TEN work areas (1..10), aliased A..J, only ONE of which is
 * "selected" at a time (SELECT n / SELECT <alias>). Commands act on the selected
 * area; an unselected area keeps its open files + cursor but is dormant. This
 * matches the dBASE III+ "10 work areas, SELECT 1..10 / A..J" model.
 *   Ref: docs/plans/SAMIR-implementation-plan.md S5.1 ("10 work areas,
 *        SELECT/alias; open .dbf+.dbt+indexes; master order; RECNO=1");
 *        ../dbase3-decomp HELP.DBS @SELECT (work areas 1-10, A-J).
 *
 * S5.1 SCOPE (this step):
 *   - wa_set_open  : USE <file> [ALIAS x] [INDEX a,b,...] in a given area.
 *   - wa_close     : CLOSE the area (free its .dbf/.dbt/.ndx; reset RECNO).
 *   - wa_select    : SELECT an area by number (1..10).
 *   - wa_select_alias : SELECT an area by alias name (case-insensitive).
 *   - wa_set_order : set the MASTER index order (0 = natural/none; 1..N = the
 *                    N-th attached index). Index-ORDERED navigation is S5.2.
 *   - wa_resolve   : the xb_ctx.resolve callback -- bind a field name in the
 *                    SELECTED area's CURRENT record to its xb_val.
 *   - accessors    : selected area, RECNO, nrec, alias, master order, the table
 *                    / memo / index handles (read-only, for S5.2..S5.8).
 *
 * S5.1 -> S5.2..S5.8 BOUNDARY (DEFERRED -- the contract is shaped for them):
 *   - Navigation (GO/SKIP/TOP/BOTTOM, physical vs index order, EOF/BOF):  S5.2.
 *     S5.1 sets RECNO=1 and leaves it there; it does NOT walk records. The
 *     wa_eof / wa_bof flags exist in the model and are initialised, but S5.1
 *     never sets them true (a freshly USEd non-empty table is at RECNO 1, not
 *     EOF). For an EMPTY table (nrec==0) S5.1 sets RECNO=1 and eof=1 (dBASE
 *     positions an empty table at EOF); resolve then yields blank/default values.
 *   - Statement executor + control flow:                                  S5.3.
 *   - LIST/DISPLAY/LOCATE/SEEK query+display:                             S5.4.
 *   - REPLACE/APPEND/DELETE/PACK/ZAP mutation verbs:                      S5.5.
 *   - SET state (EXACT/ORDER/FILTER/RELATION/...):                        S5.6.
 *   - procedures + ACCEPT/INPUT/WAIT + ON ERROR:                          S5.7.
 *   - the dot-prompt REPL (samir_main.c):                                 S5.8.
 *   These steps consume this header's accessors + the wa_resolve hook; they add
 *   NO new fields to the public work-area handle (the cursor, eof/bof, master
 *   order, and the resolve glue are all present now).
 *
 * THE RESOLVE GLUE (why S5.1 is the convergence point):
 *   The evaluator (eval.h xb_ctx) is given an OPTIONAL resolve hook:
 *       int (*resolve)(void *user, const char *name, uint16_t len, xb_val *out);
 *   Phase 3 left it NULL (an identifier was XBEE_UNBOUND). S5.1 supplies it:
 *   wa_bind_ctx(env, ctx) points ctx->resolve at wa_resolve and ctx->user at the
 *   interpreter env. When the evaluator meets a field name, wa_resolve:
 *     1. takes the SELECTED area (env->cur),
 *     2. matches `name` (case-insensitive, dBASE folds case) against the area's
 *        .dbf field descriptors,
 *     3. reads the area's CURRENT record (RECNO) via dbf_read_rec into a
 *        per-area decoded-record cache, and
 *     4. returns that field's xb_val.
 *   A memo (M) field resolves THROUGH the .dbt: the decoded record exposes the
 *   10-byte block pointer; wa_resolve reads the block via dbt_read and returns an
 *   XB_M value whose bytes live in the area's memo scratch (arena). Block 0 / no
 *   memo -> an empty XB_M (or XB_U, see wa_resolve doc). This is exactly the
 *   eval.h resolve contract: name points into the ORIGINAL expression source
 *   (NOT NUL-terminated), len is its byte length, the callee folds case.
 *   Ref: os/samir/include/samir/eval.h xb_ctx.resolve; plan S5.1 ("the work area
 *        binds field names in expressions to the current record's values").
 *
 * RECORD CACHE LIFETIME (a load-bearing decision):
 *   dbf_read_rec's C/M xb_val pointers reference an INTERNAL dbf buffer that is
 *   overwritten on the next dbf_read_rec on the SAME table (dbf.h lifetime note).
 *   The work area therefore caches the decoded record for the CURRENT RECNO in
 *   its own arena-backed xb_val array (rec_cache) and a private copy of the raw
 *   C/M bytes (rec_bytes), refreshed only when RECNO changes (wa_touch_record).
 *   This makes a resolve result stable across multiple field references within
 *   ONE expression evaluation (e.g. "LASTNAME + FIRSTNAME") -- both C slices
 *   must remain valid simultaneously, which the raw dbf buffer cannot guarantee.
 *   Ref: dbf.h "Record buffer lifetime" (overwritten per dbf_read_rec).
 *
 * Fail loud (CLAUDE.md Rule 2): every structural violation (area out of range,
 * USE into an occupied area without prior CLOSE, too many indexes, master order
 * out of range, resolve on a closed/empty area) returns a distinct WA_ERR_* /
 * is reported through the documented sentinel. A silently-wrong area binding
 * would make every command in Phase 5 act on the wrong table or record.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h> and samir/ headers.
 *
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S5.1 contract + Sec 8.2
 *     (interp.h / workarea.h is the convergence point) + Phase 5 header.
 *   - os/samir/include/samir/dbf.h  (dbf_open/dbf_close/dbf_read_rec/dbf_field/
 *     dbf_nfields/dbf_nrec/dbf_has_memo; dbf_field_t).
 *   - os/samir/include/samir/dbt.h  (dbt_open/dbt_read; is_iv_dialect=0 for III+).
 *   - os/samir/include/samir/ndx.h  (ndx_open/ndx_close/ndx_key_expr; master order).
 *   - os/samir/include/samir/eval.h (xb_ctx.resolve hook -- THE glue).
 *   - os/samir/include/samir/value.h (xb_val; xb_c/xb_n/xb_d/xb_l/xb_m/xb_u).
 *   - os/samir/include/samir/pal.h  (the ONLY OS surface; alloc/reset arena).
 */

#ifndef INITECH_SAMIR_WORKAREA_H
#define INITECH_SAMIR_WORKAREA_H

#include <stdint.h>

#include "samir/pal.h"
#include "samir/value.h"
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/ndx.h"
#include "samir/eval.h"

/* -----------------------------------------------------------------------
 * Constants (dBASE III PLUS 1.1 limits)
 * ----------------------------------------------------------------------- */

/*
 * WA_NAREAS: dBASE III PLUS has exactly TEN work areas, 1..10, aliased A..J.
 * The model stores them 0-indexed internally [0..9]; the public API uses the
 * 1-based area NUMBER everywhere a number is taken (matching SELECT 1..10).
 * Ref: HELP.DBS @SELECT ("work areas 1 through 10, or letters A through J").
 */
#define WA_NAREAS        10

/*
 * NDX_PER_AREA: maximum .ndx index files attachable to one area via
 * USE ... INDEX a,b,c (dBASE III+ allows up to 7 open index files per area).
 * Ref: HELP.DBS @USE ("INDEX <index file list>", up to 7 in III+).
 */
#define NDX_PER_AREA     7

/*
 * WA_ALIAS_CAP: capacity of the per-area alias buffer (NUL-terminated). dBASE
 * aliases are <= 10 significant chars; 12 leaves room + the NUL. An explicit
 * ALIAS clause names the area; absent it, the alias defaults to the file's base
 * name (sans extension), upper-cased. Ref: HELP.DBS @USE ALIAS.
 */
#define WA_ALIAS_CAP     12

/* -----------------------------------------------------------------------
 * Error codes (fail loud; Rule 2)
 * Int-returning work-area functions return WA_OK (0) on success or a NEGATED
 * wa_err on failure (return -WA_ERR_BAD_AREA, etc.). Callers compare
 * symbolically. Underlying dbf/dbt/ndx error codes (negative) are propagated
 * verbatim (their magnitudes never collide with these because wa_err codes are
 * a disjoint small range checked first; callers that need the codec detail
 * inspect the returned value directly).
 * ----------------------------------------------------------------------- */
typedef enum {
    WA_OK            = 0,  /* success */
    WA_ERR_BAD_AREA  = 1,  /* area number < 1 or > WA_NAREAS */
    WA_ERR_OCCUPIED  = 2,  /* USE into an area that already has an open table
                            * (the caller must CLOSE first; dBASE auto-closes,
                            * but S5.1 fails loud so the interpreter is explicit
                            * about the close -- wa_set_open does NOT silently
                            * clobber an open table) */
    WA_ERR_EMPTY     = 3,  /* operation needs an open table in the area but it
                            * is closed (e.g. wa_select_alias finds no match;
                            * wa_resolve on a closed selected area) */
    WA_ERR_TOO_MANY  = 4,  /* more than NDX_PER_AREA indexes requested */
    WA_ERR_BAD_ORDER = 5,  /* master order < 0 or > number of attached indexes */
    WA_ERR_NO_FIELD  = 6,  /* wa_resolve: name not a field of the selected table */
    WA_ERR_NOMEM     = 7,  /* PAL arena exhausted building the area / cache */
    WA_ERR_IO        = 8,  /* a dbf/dbt/ndx open or read failed (detail in the
                            * propagated codec code when wa_set_open returns it) */
    WA_ERR_NO_ALIAS  = 9   /* wa_select_alias: no open area carries that alias */
} wa_err;

/* -----------------------------------------------------------------------
 * Opaque handles
 *   work_area : ONE open table + its memo + its indexes + the RECNO cursor.
 *   wa_env    : the interpreter's whole work-area state (10 areas + selected).
 * Both layouts live in os/samir/cmd/workarea.c. Callers reach state only
 * through the accessors below. interp.h wraps wa_env in the interpreter handle.
 * ----------------------------------------------------------------------- */
typedef struct work_area work_area;
typedef struct wa_env    wa_env;

/* -----------------------------------------------------------------------
 * USE clause: the index list for USE ... INDEX a,b,c
 *
 * names[]  : NUL-terminated .ndx file names (full path or PAL-relative), in the
 *            order given on the USE line. names[0] becomes index slot 1 and, if
 *            any index is attached, the DEFAULT master order (order 1) -- dBASE
 *            makes the FIRST index in the USE list the controlling index.
 *            Ref: HELP.DBS @USE ("the first index named becomes the master").
 * count    : number of names (0 = no index; 1..NDX_PER_AREA otherwise).
 * ----------------------------------------------------------------------- */
typedef struct {
    const char *names[NDX_PER_AREA];
    int         count;
} wa_index_list;

/* -----------------------------------------------------------------------
 * Environment lifecycle
 * ----------------------------------------------------------------------- */

/*
 * wa_env_make: construct the work-area environment over a PAL.
 *
 * All ten areas start CLOSED; the selected area defaults to area 1 (dBASE
 * starts in work area 1). The wa_env and all per-area state are allocated from
 * the PAL arena; a NULL alloc fails loud.
 *
 * Returns a non-NULL wa_env* on success, NULL on arena exhaustion. The `pal`
 * pointer is retained for the lifetime of the env (every USE opens through it).
 *
 * Ref: plan S5.1; HELP.DBS (default work area = 1 at start).
 */
wa_env *wa_env_make(samir_pal_t *pal);

/*
 * wa_env_pal: the PAL the env was made over (used by USE to open files).
 */
samir_pal_t *wa_env_pal(const wa_env *env);

/* -----------------------------------------------------------------------
 * USE / CLOSE / SELECT
 * ----------------------------------------------------------------------- */

/*
 * wa_set_open: USE <name> [ALIAS alias] [INDEX <idx>] into area `area` (1-based).
 *
 *   env   : the environment.
 *   area  : 1..WA_NAREAS. Fail loud (WA_ERR_BAD_AREA) on out-of-range.
 *   name  : NUL-terminated .dbf file name opened via the env's PAL.
 *   alias : NUL-terminated alias, or NULL to derive from the file base name
 *           (upper-cased, extension stripped). Truncated to WA_ALIAS_CAP-1.
 *   idx   : the index list (may be NULL or {count:0} for no index). The FIRST
 *           index becomes master order 1 (dBASE rule). Fail loud
 *           (WA_ERR_TOO_MANY) if count > NDX_PER_AREA.
 *
 * Opens the .dbf (dbf_open). If dbf_has_memo() is true, also opens the sibling
 * .dbt (dbt_open, is_iv_dialect=0). The .dbt file name is derived by replacing
 * the .dbf extension with .dbt (see wa_set_open implementation). Opens each
 * named index (ndx_open). Sets RECNO=1 (the S5.1 contract). For a non-empty
 * table eof=bof=0; for an EMPTY table (nrec==0) eof=1 (dBASE positions an empty
 * table at EOF).
 *
 * The area MUST be closed first: a USE into an occupied area returns
 * -WA_ERR_OCCUPIED (the interpreter calls wa_close before re-USEing). This makes
 * the close explicit rather than silently leaking the previous open handles.
 *
 * Returns WA_OK and the area is open + becomes addressable. On any failure
 * returns -(wa_err) (or the propagated negative codec error for a dbf/dbt/ndx
 * open fault), and the area is left CLOSED (any partially opened handles are
 * closed). wa_set_open does NOT change which area is SELECTED -- USE in dBASE
 * does select the area it opens into; the interpreter (S5.3) calls wa_select
 * after a successful USE when the command form is bare "USE <file>" (current
 * area). The S5.1 oracle drives wa_select explicitly to exercise both.
 *
 * Ref: dbf.h dbf_open/dbf_has_memo; dbt.h dbt_open; ndx.h ndx_open; plan S5.1.
 */
int wa_set_open(wa_env *env, int area, const char *name,
                const char *alias, const wa_index_list *idx);

/*
 * wa_close: CLOSE the table in area `area` (1-based).
 *
 * Closes the .ndx indexes, the .dbt (if open), and the .dbf, in that order,
 * and resets the area to CLOSED with RECNO=0. A CLOSE on an already-closed area
 * is a no-op returning WA_OK (dBASE CLOSE on an empty area is harmless).
 *
 * Returns WA_OK or -WA_ERR_BAD_AREA for an out-of-range area. A codec close
 * failure is swallowed (the area is freed regardless; Rule 2 fail-loud applies
 * to corruption-on-open, not to a benign close).
 *
 * Ref: dbf.h dbf_close; dbt.h dbt_close; ndx.h ndx_close; plan S5.1.
 */
int wa_close(wa_env *env, int area);

/*
 * wa_close_all: CLOSE every open area (CLOSE DATABASES / QUIT cleanup).
 * Returns WA_OK. S5.8 uses this on REPL exit.
 */
int wa_close_all(wa_env *env);

/*
 * wa_select: SELECT area `area` (1-based) as the current (selected) area.
 *
 * The area need NOT be open -- dBASE allows SELECTing an empty area (a later
 * USE opens it). Returns WA_OK or -WA_ERR_BAD_AREA.
 *
 * Mutation hook (Rule 6 -- -DWA_MUTATE_SELECT): wa_select stores (area) instead
 * of (area-1) as the 0-indexed current area, so SELECT n selects area n+1 (and
 * SELECT 10 overflows). The resolve hook then reads the WRONG area's record ->
 * the oracle's "resolve follows the selected area" checks go RED. This is the
 * canonical S5.1 mutant: it breaks the SELECT->resolve binding that is the whole
 * point of the work-area model.
 *
 * Ref: HELP.DBS @SELECT; plan S5.1; ARB rider (a) (+mutant sibling).
 */
int wa_select(wa_env *env, int area);

/*
 * wa_select_alias: SELECT the open area whose alias == `alias` (case-insensitive).
 *
 *   alias : NUL-terminated alias name (e.g. "TRAVEL", "B"). Matching folds case
 *           (dBASE aliases are case-insensitive) and also matches the single
 *           letter form A..J against the area NUMBER (A=1..J=10) per dBASE.
 *
 * Returns WA_OK with the area selected, or -WA_ERR_NO_ALIAS if no open area
 * carries that alias (and no A..J letter resolves to an open area).
 *
 * Ref: HELP.DBS @SELECT (by alias or letter); plan S5.1.
 */
int wa_select_alias(wa_env *env, const char *alias);

/*
 * wa_set_order: set the MASTER index order for area `area`.
 *
 *   order : 0 = natural (physical) order, no master index; 1..N selects the
 *           N-th attached index as master (N = wa_index_count(env,area)).
 *
 * Fail loud (WA_ERR_BAD_ORDER) if order < 0 or order > index count, or
 * WA_ERR_EMPTY if the area is closed. Index-ORDERED navigation (walking records
 * in master-key order) is S5.2; S5.1 only records which index is master.
 *
 * Ref: HELP.DBS @SET ORDER; plan S5.1 ("master order"); navigation deferred S5.2.
 */
int wa_set_order(wa_env *env, int area, int order);

/* -----------------------------------------------------------------------
 * Cursor (S5.1: init only; full navigation is S5.2)
 * ----------------------------------------------------------------------- */

/*
 * wa_goto: position the cursor of area `area` at 1-based `recno`.
 *
 * S5.1 provides this minimal primitive so the resolve hook + the oracle can
 * read a chosen record (the full GO/GOTO/SKIP/TOP/BOTTOM verb set with EOF/BOF
 * edge semantics is S5.2). Setting RECNO invalidates the record cache (the next
 * wa_resolve re-reads). recno must be in [1, nrec]; out of range -> WA_ERR_IO
 * is NOT used -- instead recno is clamped per S5.2's job and S5.1 returns
 * -DBF_ERR_BAD_RECNO propagated, leaving the cursor unchanged. For an empty
 * table any recno fails the same way.
 *
 * Returns WA_OK (cursor moved, eof/bof cleared) or a negative error.
 *
 * Ref: dbf.h dbf_read_rec recno contract; plan S5.1 (RECNO init) / S5.2 (nav).
 */
int wa_goto(wa_env *env, int area, uint32_t recno);

/* -----------------------------------------------------------------------
 * The RESOLVE GLUE: bind a field name to the selected area's current record
 * ----------------------------------------------------------------------- */

/*
 * wa_resolve: the xb_ctx.resolve callback (eval.h signature).
 *
 *   user : the wa_env* (passed as ctx->user by wa_bind_ctx).
 *   name : field name bytes, pointing into the ORIGINAL expression source
 *          (NOT NUL-terminated; case-preserved). Matching folds case.
 *   len  : byte length of `name`.
 *   out  : receives the bound xb_val on success.
 *
 * Behaviour:
 *   1. Take the SELECTED area (env->cur). If it is closed -> return non-zero
 *      (the evaluator reports XBEE_UNBOUND); *out is left untouched.
 *   2. Match `name` (case-insensitive) against the .dbf field descriptors. No
 *      match -> return non-zero (XBEE_UNBOUND). [memvars are S5.7; S5.1 binds
 *      ONLY fields.]
 *   3. Ensure the current record (RECNO) is decoded into the area's cache
 *      (wa_touch_record). For an EMPTY table (eof) yield the field's BLANK value
 *      (C -> empty string, N -> 0, D -> blank/XB_U, L -> false), matching dBASE
 *      resolve-at-EOF semantics.
 *   4. For a memo (M) field: read the memo text through the .dbt (dbt_read at the
 *      record's block number) and return XB_M over the memo scratch bytes; block
 *      0 / no memo / no .dbt -> an empty XB_M. The 10-byte block pointer comes
 *      from the decoded record cache (xb_m raw bytes -> dec_parse -> block no).
 *   5. Otherwise return the cached field xb_val (C/N/D/L), whose backing bytes
 *      live in the area's stable rec_bytes copy (valid until the next
 *      wa_touch_record on that area).
 *
 * Returns 0 (bound; *out set) or non-zero (unbound/error). This is exactly the
 * eval.h resolve contract (0 = bound, non-zero = not found).
 *
 * Mutation hook (Rule 6 -- -DWA_MUTATE_RESOLVE_AREA): wa_resolve reads area 0
 * (internal index) unconditionally instead of env->cur, so resolve ignores
 * SELECT. With two areas open at different records/tables, a field reference
 * after SELECT returns the wrong area's value -> the oracle goes RED. (The
 * primary S5.1 mutant is -DWA_MUTATE_SELECT; this is an alternative bite point.)
 *
 * Ref: eval.h xb_ctx.resolve; dbf.h dbf_read_rec; dbt.h dbt_read; rt.h
 *      dec_parse (block number); value.h constructors; plan S5.1 resolve glue.
 */
int wa_resolve(void *user, const char *name, uint16_t len, xb_val *out);

/*
 * wa_bind_ctx: wire an xb_ctx to resolve through this environment.
 *
 * Sets ctx->resolve = wa_resolve and ctx->user = env. Leaves every other ctx
 * field (set_exact, scratch arena, ctx_today) untouched -- the caller owns those
 * (the interpreter sets SET EXACT in S5.6 and the scratch arena per eval call).
 * After this, xb_eval over an AST referencing field names binds them to the
 * selected area's current record.
 *
 * Ref: eval.h xb_ctx; plan S5.1 ("the resolve glue").
 */
void wa_bind_ctx(wa_env *env, xb_ctx *ctx);

/* -----------------------------------------------------------------------
 * Accessors (read-only; S5.2..S5.8 consume these)
 * All take the env; the per-area accessors take a 1-based area number.
 * Passing an out-of-range area or a closed area returns the documented
 * sentinel (0 / NULL) rather than crashing (the caller checks wa_is_open first
 * where it matters). RECNO/nrec sentinels are 0 for a closed area.
 * ----------------------------------------------------------------------- */

/* wa_selected: the 1-based number of the currently SELECTED area (1..10). */
int wa_selected(const wa_env *env);

/* wa_is_open: 1 if area `area` (1-based) has an open table, else 0 (or for an
 * out-of-range area). */
int wa_is_open(const wa_env *env, int area);

/* wa_recno: the 1-based current record number of area `area`, or 0 if the area
 * is closed. S5.1 sets this to 1 on USE (the headline contract). */
uint32_t wa_recno(const wa_env *env, int area);

/* wa_nrec: the logical record count of area `area` (dbf_nrec), or 0 if closed. */
uint32_t wa_nrec(const wa_env *env, int area);

/* wa_eof / wa_bof: the end/begin-of-file flags of area `area`. S5.1 sets eof=1
 * only for an empty (nrec==0) table; navigation (S5.2) drives them otherwise. */
int wa_eof(const wa_env *env, int area);
int wa_bof(const wa_env *env, int area);

/* wa_alias: NUL-terminated alias of area `area`, or "" (empty) if closed.
 * Pointer is owned by the area; valid until the area is closed/re-USEd. */
const char *wa_alias(const wa_env *env, int area);

/* wa_master_order: the master index order of area `area` (0 = natural; 1..N).
 * 0 for a closed area. */
int wa_master_order(const wa_env *env, int area);

/* wa_index_count: number of .ndx indexes attached to area `area` (0 if closed). */
int wa_index_count(const wa_env *env, int area);

/* wa_table: the dbf_table* of area `area`, or NULL if closed. Read-only handle
 * for S5.2..S5.8 (navigation, query, mutation all reach the codec through it). */
dbf_table *wa_table(const wa_env *env, int area);

/* wa_memo: the dbt_file* of area `area`, or NULL if the table has no memo / is
 * closed. */
dbt_file *wa_memo(const wa_env *env, int area);

/* wa_index: the i-th (0-based) attached ndx_index* of area `area`, or NULL if
 * out of range / closed. i in [0, wa_index_count). */
ndx_index *wa_index(const wa_env *env, int area, int i);

#endif /* INITECH_SAMIR_WORKAREA_H */
