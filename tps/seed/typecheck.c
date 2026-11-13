/*
 * typecheck.c -- minimal static type checker for the seed Pascal subset.
 *
 * beads: initech-f0uc (B1 of docs/plans/TPS-M7-subset-plan.md)
 * Ref:   see typecheck.h for the full rule set and the DECISION note on
 *        checking strictness (ADR-0007 is silent on this; this module is
 *        the "minimal sound checking" the bead asked for). CLAUDE.md Law 1
 *        / Rule 2 (fail loud on internal contract breaks) / Rule 12.
 *
 * Two passes over the AST, mirroring the parser's own single-error style
 * (see parser.c fail_at):
 *   1. collect_decls  -- build a flat, insertion-ordered symbol table from
 *      the program's var-sections (name -> declared AstVarType), rejecting
 *      duplicate names.
 *   2. check_stmt / check_expr -- walk the block, resolving every VARREF
 *      against the table, checking assignment/operator/write-arg type
 *      agreement, and annotating every expression node's `type` field.
 *
 * The symbol table is a small fixed-capacity array (TC_MAX_SYMBOLS), not a
 * hash map -- insertion-ordered linear scan, no nondeterministic iteration
 * (consistent with the determinism discipline ADR-0007 DEC-04 states for
 * Turbo Initech; the seed is host tooling, not the artifact, but there is
 * no reason to diverge from a simpler-and-deterministic structure here).
 * Overflow is a located, fail-loud diagnostic (Rule 2), never silent
 * truncation.
 */
#include "typecheck.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TC_MAX_SYMBOLS 256
#define TC_SYM_NAME_CAP 128
/* B4 (beads initech-63ce): fixed-capacity, fail-loud-on-overflow tables for
 * the routine table and per-routine local scope -- the same "no hashing,
 * insertion-ordered, deterministic" discipline TC_MAX_SYMBOLS already uses
 * (ADR-0007 DEC-04 spirit). */
#define TC_MAX_PROCS   128
#define TC_MAX_PARAMS   32
#define TC_MAX_LOCALS   64
/* B6 (beads initech-rug7): fixed-capacity, fail-loud-on-overflow tables for
 * record TYPES and their fields -- the same "no hashing, insertion-ordered,
 * deterministic" discipline every other table in this file already uses
 * (ADR-0007 DEC-04 spirit). */
#define TC_MAX_RECTYPES      32
#define TC_MAX_RECORD_FIELDS 32

typedef struct {
    char       name[TC_SYM_NAME_CAP]; /* case-folded (lower) declared name */
    AstVarType type;
    /* B3 (beads initech-7mo3): 1 if this name was declared `const`, 0 if
     * `var`. const and var share ONE flat, case-insensitive namespace (the
     * same one-declaration rule) -- see collect_decls -- and is_const is
     * how check_stmt's AST_ASSIGN case rejects "assign to a constant". */
    int        is_const;
    /* B5 (beads initech-54uu): 1 if this GLOBAL is a static array. When set,
     * `type` holds the array's ELEMENT type (reused, same convention as
     * ast.h's AST_VARDECL.vtype) and lo/hi are its declared inclusive
     * bounds. An array is never a const (B3's const grammar is scalar-only)
     * so is_array and is_const are never both set. */
    int        is_array;
    long       lo, hi;
    /* B6 (beads initech-rug7): 1 if `type` (or, for an array, the ELEMENT
     * type) is AST_TY_RECORD; `rectype` then names WHICH record type
     * (case-folded, looked up in Tc.rectypes). A record is never a const
     * (B3's const grammar is scalar-only) so is_record and is_const are
     * never both set. */
    int        is_record;
    char       rectype[TC_SYM_NAME_CAP];
    /* B7 (beads initech-39k2): the declared ShortString capacity (1..255) when
     * `type` is AST_TY_STRING, else 0. `string` folds to 255. Consumed by
     * check_stmt to stamp assign.strcap (codegen's string-assign cap + string-
     * target marker). */
    int        strcap;
} TcSym;

/* B4 (beads initech-63ce): one entry in a routine's local scope. `kind`
 * distinguishes a value parameter, a var (by-reference) parameter, a local
 * variable, and the function-result variable; `is_lvalue` marks everything
 * that may be an assignment target or a var-parameter argument (i.e. not a
 * constant -- but there are no local consts in this subset, so every local
 * is an lvalue). */
typedef enum {
    TC_KIND_VALPARAM,
    TC_KIND_VARPARAM,
    TC_KIND_LOCAL,
    TC_KIND_RESULT
} TcLocalKind;

typedef struct {
    char        name[TC_SYM_NAME_CAP];
    AstVarType  type;
    TcLocalKind kind;
    /* B5 (beads initech-54uu): 1 iff this LOCAL (always TC_KIND_LOCAL --
     * array parameters/results are out of scope, see ast.h's B5 DECISION
     * note) is a static array; `type` is then its ELEMENT type and lo/hi its
     * declared inclusive bounds. */
    int         is_array;
    long        lo, hi;
    /* B6 (beads initech-rug7): 1 iff `type` (or, for an array, the ELEMENT
     * type) is AST_TY_RECORD; `rectype` names which record type. Valid on a
     * TC_KIND_LOCAL (scalar or array-of-record) or a TC_KIND_VARPARAM (a
     * `var` parameter of record type, address-passed) -- never on
     * TC_KIND_VALPARAM (rejected at parse time) or TC_KIND_RESULT (record
     * results are out of scope, ast.h's B6 note). */
    int         is_record;
    char        rectype[TC_SYM_NAME_CAP];
    /* B7 (beads initech-39k2): the ShortString capacity when `type` is
     * AST_TY_STRING (255 for a `var string` parameter -- the only string
     * param form -- or 1..255 for a local string), else 0. */
    int         strcap;
} TcLocal;

/* B4: one procedure/function signature. Params are flattened (one entry per
 * name). `has_result`/`rettype` describe a function; `is_defined` flips from
 * 0 (a `forward` declaration seen) to 1 (the defining occurrence seen). */
typedef struct {
    char       name[TC_SYM_NAME_CAP];
    int        has_result;
    AstVarType rettype;
    struct {
        char       name[TC_SYM_NAME_CAP];
        AstVarType type;
        int        is_var;
        /* B6 (beads initech-rug7): non-empty iff type == AST_TY_RECORD
         * (names which record type). Only ever set on a `var` parameter --
         * a value parameter of record type is rejected at parse time. */
        int        is_record;
        char       rectype[TC_SYM_NAME_CAP];
        /* B7 (beads initech-39k2): the ShortString capacity (255) when a `var`
         * parameter is string-typed, else 0. */
        int        strcap;
    } params[TC_MAX_PARAMS];
    int        nparams;
    int        is_defined;
    int        line, col; /* of the (first) declaration, for diagnostics */
} TcProc;

/* B6 (beads initech-rug7): one field of a record type -- a case-folded name
 * plus its SCALAR type (integer/boolean/char only -- ast.h's B6 AST_TYPEDECL
 * comment: no nested records, no array-typed fields, enforced by the parser
 * already, re-asserted nowhere further here since the parser's contract is
 * trusted the same way array bound lo<=hi is). */
typedef struct {
    char       name[TC_SYM_NAME_CAP];
    AstVarType type;
} TcField;

/* One named record type: fields in DECLARATION ORDER (flattened across
 * field-groups -- "kind, x: integer; ch: char;" is TWO groups but THREE
 * fields, field_index 0/1/2 respectively) -- see ast.h's B6 layout-rule
 * comment ("fields ... consecutive uniform 4-byte slots in DECLARATION
 * ORDER"). `nfields` is this record type's SIZE in dwords
 * (sizeof(record) = 4 * nfields) -- consumed by codegen.c for frame-slot/
 * .bss sizing and the array-of-record element STRIDE. */
typedef struct {
    char    name[TC_SYM_NAME_CAP];
    TcField fields[TC_MAX_RECORD_FIELDS];
    int     nfields;
    int     line, col; /* of the `type` declaration, for diagnostics */
} TcRecType;

typedef struct {
    TcSym syms[TC_MAX_SYMBOLS];   /* program-level vars + consts (globals) */
    int   nsyms;

    TcProc procs[TC_MAX_PROCS];   /* B4: routine signatures */
    int    nprocs;

    /* B6 (beads initech-rug7): every declared record TYPE (name -> flattened
     * field list), collected by collect_types BEFORE collect_decls/
     * collect_procs run (a var/param/array-element can name a record type,
     * so its field list must already be known). */
    TcRecType rectypes[TC_MAX_RECTYPES];
    int       nrectypes;

    /* B4: the local scope of the routine body currently being checked.
     * nlocal == 0 (and in_routine == 0) while checking the main program
     * block, so resolution falls through to globals only. */
    TcLocal local[TC_MAX_PARAMS + TC_MAX_LOCALS];
    int     nlocal;
    int     in_routine;

    /* B7 (beads initech-39k2): set to 1 the moment any string type appears
     * (a string var/const/param/local declaration or an AST_STRLIT expression)
     * during this existing walk -- NO new pass. Copied to
     * program->as.program.uses_strings so codegen emits the __str_* intrinsic
     * prelude iff the program actually uses strings (the byte-identity guard
     * for pre-B7 fixtures). */
    int   uses_strings;

    /* B8 (beads initech-ogxv): set only by a validated call to one of the
     * five thin file-I/O builtins. Declarations alone do not pull RTL externs
     * into emitted assembly. */
    int   uses_fileio;

    int   failed;
    char  errmsg[TYPECHECK_ERRMSG_CAP];
    int   errline;
    int   errcol;
} Tc;

/* Record the first error only (parser.c fail_at idiom: keep the most
 * relevant, earliest-encountered location). */
static void fail_at(Tc *tc, int line, int col, const char *msg)
{
    if (tc->failed)
        return;
    tc->failed = 1;
    tc->errline = line;
    tc->errcol = col;
    snprintf(tc->errmsg, sizeof(tc->errmsg), "%s", msg);
}

/* Case-fold `src` into `dst` (NUL-terminated, truncated at cap-1 bytes --
 * Pascal identifiers this short in the seed's own bootstrap corpus are not
 * expected to collide post-truncation; a real collision would surface as a
 * spurious "duplicate variable declaration" or "undeclared variable",
 * which is a fail-loud symptom, never a silent misresolution). */
static void lower_copy(char *dst, size_t cap, const char *src)
{
    size_t i = 0;
    for (; src[i] != '\0' && i + 1 < cap; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

typedef enum {
    FILEBI_NONE = 0,
    FILEBI_ASSIGN,
    FILEBI_RESET,
    FILEBI_REWRITE,
    FILEBI_BLOCKREAD,
    FILEBI_BLOCKWRITE
} FileBuiltin;

static FileBuiltin file_builtin_kind(const char *name_lc)
{
    if (strcmp(name_lc, "assign") == 0)     return FILEBI_ASSIGN;
    if (strcmp(name_lc, "reset") == 0)      return FILEBI_RESET;
    if (strcmp(name_lc, "rewrite") == 0)    return FILEBI_REWRITE;
    if (strcmp(name_lc, "blockread") == 0)  return FILEBI_BLOCKREAD;
    if (strcmp(name_lc, "blockwrite") == 0) return FILEBI_BLOCKWRITE;
    return FILEBI_NONE;
}

static int sym_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nsyms; i++)
        if (strcmp(tc->syms[i].name, name_lc) == 0)
            return i;
    return -1;
}

/* B4 (beads initech-63ce): find a name in the ACTIVE routine's local scope
 * (params + locals + result). -1 if not present (or no routine is active). */
static int local_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nlocal; i++)
        if (strcmp(tc->local[i].name, name_lc) == 0)
            return i;
    return -1;
}

/* B4: find a routine by (case-folded) name. -1 if not declared. */
static int proc_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nprocs; i++)
        if (strcmp(tc->procs[i].name, name_lc) == 0)
            return i;
    return -1;
}

/* B6 (beads initech-rug7): find a record TYPE by (case-folded) name. -1 if
 * not declared (should not happen for a well-typed program -- the parser's
 * own registry already gated every rectype-name USE against a real prior
 * declaration; a miss here is an internal contract break). */
static int rectype_find(const Tc *tc, const char *name_lc)
{
    for (int i = 0; i < tc->nrectypes; i++)
        if (strcmp(tc->rectypes[i].name, name_lc) == 0)
            return i;
    return -1;
}

/* Find a FIELD by (case-folded) name within one record type. -1 if unknown
 * (a real, checked, fail-loud user error -- "unknown field"). */
static int rectype_field_find(const TcRecType *rt, const char *field_lc)
{
    for (int i = 0; i < rt->nfields; i++)
        if (strcmp(rt->fields[i].name, field_lc) == 0)
            return i;
    return -1;
}

/*
 * B4: resolve a name to its type, checking the active routine's local scope
 * FIRST (so a param/local/result shadows a global), then globals. Returns the
 * type via *out_type and (for globals) whether it is a constant via
 * *out_is_const / whether it is an lvalue via *out_is_lvalue. Returns 1 on
 * success, 0 if the name is undeclared (no error recorded here -- the caller
 * words the diagnostic for its context). A local is always an lvalue (no
 * local consts in this subset); a global var is an lvalue, a global const is
 * not.
 *
 * B5 (beads initech-54uu): also reports whether the resolved name is a
 * static ARRAY (*out_is_array) and, when it is, its declared bounds
 * (*out_lo and *out_hi). For an array, *out_type is the ELEMENT type (same
 * "reuse the type field" convention as ast.h's AST_VARDECL.vtype and this
 * file's TcSym/TcLocal). Callers that only handle SCALAR names (the
 * pre-B5 call sites) must check *out_is_array themselves and reject a bare
 * array reference with their own located diagnostic -- this function does
 * not decide whether an array hit is an error, only reports the fact.
 *
 * B6 (beads initech-rug7): also reports whether the resolved TYPE (*out_type
 * -- the scalar type, OR the array's element type when *out_is_array) is a
 * RECORD (*out_is_record) and, when it is, WHICH record type
 * (*out_rectype_lc, a case-folded name into a caller-owned buffer of at
 * least TC_SYM_NAME_CAP bytes). Exactly like *out_is_array, this function
 * only REPORTS the fact -- callers decide whether a record hit is legal in
 * their context (e.g. check_expr's AST_VARREF case allows a bare record var
 * as a value; a plain scalar array name is instead rejected there).
 */
/* B7 (beads initech-39k2): `out_strcap` receives the resolved name's declared
 * ShortString capacity (0 when it is not a string), so callers can stamp
 * assign.strcap and validate a `var string` argument's cap-255 identity. */
static int resolve_name(const Tc *tc, const char *name_lc,
                        AstVarType *out_type, int *out_is_lvalue,
                        int *out_is_array, long *out_lo, long *out_hi,
                        int *out_is_record, char *out_rectype_lc,
                        int *out_strcap)
{
    int li = local_find(tc, name_lc);
    if (li >= 0) {
        *out_type = tc->local[li].type;
        *out_is_lvalue = 1; /* every local (param/local/result) is writable */
        *out_is_array = tc->local[li].is_array;
        *out_lo = tc->local[li].lo;
        *out_hi = tc->local[li].hi;
        *out_is_record = tc->local[li].is_record;
        snprintf(out_rectype_lc, TC_SYM_NAME_CAP, "%s", tc->local[li].rectype);
        *out_strcap = tc->local[li].strcap;
        return 1;
    }
    int si = sym_find(tc, name_lc);
    if (si >= 0) {
        *out_type = tc->syms[si].type;
        *out_is_lvalue = !tc->syms[si].is_const;
        *out_is_array = tc->syms[si].is_array;
        *out_lo = tc->syms[si].lo;
        *out_hi = tc->syms[si].hi;
        *out_is_record = tc->syms[si].is_record;
        snprintf(out_rectype_lc, TC_SYM_NAME_CAP, "%s", tc->syms[si].rectype);
        *out_strcap = tc->syms[si].strcap;
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* B6 (beads initech-rug7): Pass 0 -- collect record TYPES               */
/* ------------------------------------------------------------------ */
/* Runs BEFORE collect_decls/collect_procs: a var/param/array-element may
 * name a record type, so the full record-type table (name -> flattened
 * field list) must exist before anything else is resolved. Record type
 * names live in their OWN namespace (not cross-checked against var/const/
 * proc names in this subset -- DECISION, report, mirrors parser.c's own
 * rectype registry, which makes the identical simplifying choice). */
static void collect_types(Tc *tc, const AstList *decls)
{
    for (size_t i = 0; i < decls->count && !tc->failed; i++) {
        const AstNode *td = decls->items[i];
        if (td->kind != AST_TYPEDECL)
            continue;

        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), td->as.typedecl.name);
        if (rectype_find(tc, lc) >= 0) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "duplicate record type declaration: %s",
                     td->as.typedecl.name);
            fail_at(tc, td->line, td->col, msg);
            return;
        }
        if (tc->nrectypes >= TC_MAX_RECTYPES) {
            fail_at(tc, td->line, td->col,
                    "too many record type declarations "
                    "(TC_MAX_RECTYPES exceeded)");
            return;
        }
        TcRecType *rt = &tc->rectypes[tc->nrectypes];
        lower_copy(rt->name, sizeof(rt->name), td->as.typedecl.name);
        rt->nfields = 0;
        rt->line = td->line;
        rt->col = td->col;

        for (size_t g = 0; g < td->as.typedecl.fields.count && !tc->failed;
             g++) {
            const AstNode *fg = td->as.typedecl.fields.items[g];
            if (fg->kind != AST_VARDECL) {
                fail_at(tc, fg->line, fg->col,
                        "internal: non-vardecl field group in a record type "
                        "(typecheck)");
                return;
            }
            for (size_t j = 0; j < fg->as.vardecl.names.count && !tc->failed;
                 j++) {
                const AstNode *fr = fg->as.vardecl.names.items[j];
                char flc[TC_SYM_NAME_CAP];
                lower_copy(flc, sizeof(flc), fr->as.varref.name);
                if (rectype_field_find(rt, flc) >= 0) {
                    char msg[TYPECHECK_ERRMSG_CAP];
                    snprintf(msg, sizeof(msg),
                             "duplicate field '%s' in record type '%s'",
                             fr->as.varref.name, td->as.typedecl.name);
                    fail_at(tc, fr->line, fr->col, msg);
                    return;
                }
                if (rt->nfields >= TC_MAX_RECORD_FIELDS) {
                    char msg[TYPECHECK_ERRMSG_CAP];
                    snprintf(msg, sizeof(msg),
                             "too many fields in record type '%s' "
                             "(TC_MAX_RECORD_FIELDS exceeded)",
                             td->as.typedecl.name);
                    fail_at(tc, fr->line, fr->col, msg);
                    return;
                }
                lower_copy(rt->fields[rt->nfields].name,
                           sizeof(rt->fields[rt->nfields].name),
                           fr->as.varref.name);
                rt->fields[rt->nfields].type = fg->as.vardecl.vtype;
                rt->nfields++;
            }
        }
        tc->nrectypes++;
    }
}

/* ------------------------------------------------------------------ */
/* Pass 1: collect declarations                                       */
/* ------------------------------------------------------------------ */
static void collect_decls(Tc *tc, const AstList *decls)
{
    for (size_t i = 0; i < decls->count && !tc->failed; i++) {
        const AstNode *vd = decls->items[i];

        /* B3 (beads initech-7mo3): a const-decl contributes exactly ONE
         * name (unlike AST_VARDECL, which names a whole list). It shares
         * the SAME case-insensitive one-declaration rule as vars (this is
         * the requirement typecheck.h's B3 contract states): a const
         * colliding with an existing var name -- or with another const --
         * is the identical "duplicate variable declaration" diagnostic
         * used for two colliding var-decls. */
        if (vd->kind == AST_CONSTDECL) {
            char lc[TC_SYM_NAME_CAP];
            lower_copy(lc, sizeof(lc), vd->as.constdecl.name);
            if (file_builtin_kind(lc) != FILEBI_NONE) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "variable/constant name collides with a file-I/O "
                         "builtin: %s", vd->as.constdecl.name);
                fail_at(tc, vd->line, vd->col, msg);
                return;
            }
            if (sym_find(tc, lc) >= 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "duplicate variable declaration: %s",
                         vd->as.constdecl.name);
                fail_at(tc, vd->line, vd->col, msg);
                return;
            }
            if (tc->nsyms >= TC_MAX_SYMBOLS) {
                fail_at(tc, vd->line, vd->col,
                        "too many variable declarations "
                        "(TC_MAX_SYMBOLS exceeded)");
                return;
            }
            lower_copy(tc->syms[tc->nsyms].name,
                       sizeof(tc->syms[tc->nsyms].name),
                       vd->as.constdecl.name);
            tc->syms[tc->nsyms].type = vd->as.constdecl.ctype;
            tc->syms[tc->nsyms].is_const = 1;
            tc->syms[tc->nsyms].is_array = 0; /* B3 consts are scalar-only */
            tc->syms[tc->nsyms].lo = 0;
            tc->syms[tc->nsyms].hi = 0;
            tc->syms[tc->nsyms].is_record = 0; /* B3 consts are scalar-only */
            tc->syms[tc->nsyms].rectype[0] = '\0';
            /* B7 (beads initech-39k2): a STRING const (folded to a literal at
             * use sites) still marks the program as using strings. */
            if (vd->as.constdecl.ctype == AST_TY_STRING)
                tc->uses_strings = 1;
            tc->nsyms++;
            continue;
        }

        /* B4 (beads initech-63ce): procedure/function declarations live in
         * the SAME decls list (see parser.c) but are collected separately by
         * collect_procs; skip them in the globals pass. */
        if (vd->kind == AST_PROCDECL || vd->kind == AST_FUNCDECL)
            continue;

        /* B6 (beads initech-rug7): `type` declarations were already fully
         * collected by collect_types (Pass 0, run before this pass); skip
         * them here exactly as const/proc decls are skipped. */
        if (vd->kind == AST_TYPEDECL)
            continue;

        if (vd->kind != AST_VARDECL) {
            fail_at(tc, vd->line, vd->col,
                    "internal: non-vardecl/constdecl in program decls "
                    "(typecheck)");
            return;
        }
        for (size_t j = 0; j < vd->as.vardecl.names.count && !tc->failed; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (vr->kind != AST_VARREF) {
                fail_at(tc, vr->line, vr->col,
                        "internal: non-varref in vardecl names (typecheck)");
                return;
            }
            char lc[TC_SYM_NAME_CAP];
            lower_copy(lc, sizeof(lc), vr->as.varref.name);
            if (file_builtin_kind(lc) != FILEBI_NONE) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "variable/constant name collides with a file-I/O "
                         "builtin: %s", vr->as.varref.name);
                fail_at(tc, vr->line, vr->col, msg);
                return;
            }
            if (sym_find(tc, lc) >= 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "duplicate variable declaration: %s",
                         vr->as.varref.name);
                fail_at(tc, vr->line, vr->col, msg);
                return;
            }
            if (tc->nsyms >= TC_MAX_SYMBOLS) {
                fail_at(tc, vr->line, vr->col,
                        "too many variable declarations "
                        "(TC_MAX_SYMBOLS exceeded)");
                return;
            }
            lower_copy(tc->syms[tc->nsyms].name,
                       sizeof(tc->syms[tc->nsyms].name),
                       vr->as.varref.name);
            /* B5 (beads initech-54uu): vd->as.vardecl.vtype already holds
             * the ELEMENT type when is_array is set (parser.c's
             * parse_one_vardecl) -- the same "reuse vtype" convention this
             * struct's own comment documents. */
            tc->syms[tc->nsyms].type = vd->as.vardecl.vtype;
            tc->syms[tc->nsyms].is_const = 0;
            tc->syms[tc->nsyms].is_array = vd->as.vardecl.is_array;
            tc->syms[tc->nsyms].lo = vd->as.vardecl.lo;
            tc->syms[tc->nsyms].hi = vd->as.vardecl.hi;
            /* B6 (beads initech-rug7): a RECORD-typed global (scalar or
             * array-of-record) names its record type; the parser's own
             * rectype registry already gated `rectype` to a real prior
             * `type` declaration, so rectype_find below should never miss --
             * a miss is an internal contract break, not a user error. */
            if (vd->as.vardecl.vtype == AST_TY_RECORD) {
                tc->syms[tc->nsyms].is_record = 1;
                lower_copy(tc->syms[tc->nsyms].rectype,
                           sizeof(tc->syms[tc->nsyms].rectype),
                           vd->as.vardecl.rectype);
                if (rectype_find(tc, tc->syms[tc->nsyms].rectype) < 0) {
                    fail_at(tc, vr->line, vr->col,
                            "internal: unknown record type in vardecl "
                            "(typecheck)");
                    return;
                }
            } else {
                tc->syms[tc->nsyms].is_record = 0;
                tc->syms[tc->nsyms].rectype[0] = '\0';
            }
            /* B7 (beads initech-39k2): a string global carries its declared
             * capacity; any string type appearing marks the program as
             * using strings (codegen's intrinsic-prelude gate). */
            if (vd->as.vardecl.vtype == AST_TY_STRING) {
                tc->syms[tc->nsyms].strcap = vd->as.vardecl.strcap;
                tc->uses_strings = 1;
            }
            tc->nsyms++;
        }
    }
}

/* ------------------------------------------------------------------ */
/* B4 (beads initech-63ce): collect procedure/function signatures        */
/* ------------------------------------------------------------------ */
/* Flatten one AST_PROCDECL/AST_FUNCDECL's parameter list into `out`,
 * rejecting duplicate parameter names within the routine and capacity
 * overflow (Rule 2). Returns 1 on success, 0 (with an error recorded) on
 * failure. */
static int flatten_signature(Tc *tc, const AstNode *pf, TcProc *out)
{
    lower_copy(out->name, sizeof(out->name), pf->as.procfunc.name);
    out->has_result = pf->as.procfunc.has_result;
    out->rettype = pf->as.procfunc.rettype;
    out->nparams = 0;
    out->line = pf->line;
    out->col = pf->col;
    out->is_defined = 0;

    for (size_t i = 0; i < pf->as.procfunc.params.count; i++) {
        const AstNode *pn = pf->as.procfunc.params.items[i];
        if (pn->kind != AST_PARAM) {
            fail_at(tc, pn->line, pn->col,
                    "internal: non-param in routine parameter list");
            return 0;
        }
        if (out->nparams >= TC_MAX_PARAMS) {
            fail_at(tc, pn->line, pn->col,
                    "too many parameters (TC_MAX_PARAMS exceeded)");
            return 0;
        }
        char plc[TC_SYM_NAME_CAP];
        lower_copy(plc, sizeof(plc), pn->as.param.name);
        for (int k = 0; k < out->nparams; k++) {
            if (strcmp(out->params[k].name, plc) == 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "duplicate parameter name: %s", pn->as.param.name);
                fail_at(tc, pn->line, pn->col, msg);
                return 0;
            }
        }
        lower_copy(out->params[out->nparams].name,
                   sizeof(out->params[out->nparams].name), pn->as.param.name);
        out->params[out->nparams].type = pn->as.param.ptype;
        out->params[out->nparams].is_var = pn->as.param.is_var;
        /* B6 (beads initech-rug7): a RECORD-typed parameter names its record
         * type. DEFENSIVE re-assertion (Rule 2 -- an internal contract
         * break, never a user error): parser.c's parse_param_list already
         * rejects a value (non-`var`) parameter of record type at parse
         * time, so `ptype == AST_TY_RECORD && !is_var` reaching here would
         * mean the parser/typecheck contract broke, not a real source bug. */
        if (pn->as.param.ptype == AST_TY_RECORD) {
            if (!pn->as.param.is_var) {
                fail_at(tc, pn->line, pn->col,
                        "internal: value parameter of record type reached "
                        "typecheck (parser should have rejected this)");
                return 0;
            }
            out->params[out->nparams].is_record = 1;
            lower_copy(out->params[out->nparams].rectype,
                       sizeof(out->params[out->nparams].rectype),
                       pn->as.param.rectype);
            if (rectype_find(tc, out->params[out->nparams].rectype) < 0) {
                fail_at(tc, pn->line, pn->col,
                        "internal: unknown record type in parameter "
                        "(typecheck)");
                return 0;
            }
        } else {
            out->params[out->nparams].is_record = 0;
            out->params[out->nparams].rectype[0] = '\0';
        }
        /* B7 (beads initech-39k2): a `var string` parameter (always cap 255 --
         * parser.c rejects any other string formal) carries its capacity;
         * seeing it marks the program as using strings. */
        if (pn->as.param.ptype == AST_TY_STRING) {
            out->params[out->nparams].strcap = pn->as.param.strcap;
            tc->uses_strings = 1;
        }
        out->nparams++;
    }
    return 1;
}

/* Do two signatures agree on the parts that matter for a `forward` <->
 * defining match: result-ness, result type, arity, and each parameter's type
 * and by-reference flag? (Parameter NAMES may differ -- only the interface
 * shape is load-bearing.) */
static int signatures_match(const TcProc *a, const TcProc *b)
{
    if (a->has_result != b->has_result)
        return 0;
    if (a->has_result && a->rettype != b->rettype)
        return 0;
    if (a->nparams != b->nparams)
        return 0;
    for (int i = 0; i < a->nparams; i++) {
        if (a->params[i].type != b->params[i].type)
            return 0;
        if (a->params[i].is_var != b->params[i].is_var)
            return 0;
        /* B6 (beads initech-rug7): two RECORD-typed parameters must name the
         * SAME record type -- two different record types both tagged
         * AST_TY_RECORD are not the same parameter shape. */
        if (a->params[i].type == AST_TY_RECORD
            && strcmp(a->params[i].rectype, b->params[i].rectype) != 0)
            return 0;
    }
    return 1;
}

static void collect_procs(Tc *tc, const AstList *decls)
{
    for (size_t i = 0; i < decls->count && !tc->failed; i++) {
        const AstNode *pf = decls->items[i];
        if (pf->kind != AST_PROCDECL && pf->kind != AST_FUNCDECL)
            continue;

        TcProc sig;
        memset(&sig, 0, sizeof(sig));
        if (!flatten_signature(tc, pf, &sig))
            return;

        /* B8: these names are the fixed thin-RTL surface. Allowing a user
         * declaration with the same case-folded name would make every call
         * resolve to the builtin while the declaration remained dead. */
        if (file_builtin_kind(sig.name) != FILEBI_NONE) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "procedure/function name collides with a file-I/O "
                     "builtin: %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }

        /* A routine name may not collide with a program-level var/const
         * (they would be indistinguishable at a bare-identifier use site). */
        if (sym_find(tc, sig.name) >= 0) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "procedure/function name collides with a variable or "
                     "constant: %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }

        int idx = proc_find(tc, sig.name);
        if (idx < 0) {
            if (tc->nprocs >= TC_MAX_PROCS) {
                fail_at(tc, pf->line, pf->col,
                        "too many procedures/functions "
                        "(TC_MAX_PROCS exceeded)");
                return;
            }
            sig.is_defined = pf->as.procfunc.is_forward ? 0 : 1;
            tc->procs[tc->nprocs++] = sig;
            continue;
        }

        /* A second declaration of an already-known name. The only legal case
         * is: an earlier `forward` now being DEFINED with a matching
         * signature. */
        TcProc *ex = &tc->procs[idx];
        if (pf->as.procfunc.is_forward) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "duplicate declaration of %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }
        if (ex->is_defined) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "duplicate definition of %s", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }
        if (!signatures_match(ex, &sig)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "defining declaration of %s does not match its "
                     "forward declaration (parameter types/count or result "
                     "type differ)", pf->as.procfunc.name);
            fail_at(tc, pf->line, pf->col, msg);
            return;
        }
        ex->is_defined = 1;
    }
}

/* ------------------------------------------------------------------ */
/* Pass 2: expressions + statements                                    */
/* ------------------------------------------------------------------ */
static AstVarType check_expr(Tc *tc, AstNode *e);

/*
 * B6 (beads initech-rug7): like check_expr, but ALSO reports the record TYPE
 * NAME (case-folded, into `out_rectype_lc` -- a caller-owned buffer of at
 * least TC_SYM_NAME_CAP bytes, set to "" when the result is not a record)
 * when the expression's checked type is AST_TY_RECORD. Used ONLY at the
 * handful of call sites that need to know WHICH record type -- AST_FIELD's
 * base, a whole-record ASSIGNMENT's value, and a `var`-parameter CALL
 * argument -- since a record-typed EXPRESSION can only ever be an AST_VARREF
 * (a plain record variable) or an AST_INDEX (an array-of-record element) in
 * this subset (no record-returning functions, no record fields that are
 * themselves records -- ast.h's B6 AST_TYPEDECL comment). Re-resolves the
 * designator's name via a SEPARATE (cheap, linear-scan) resolve_name call
 * rather than threading a new out-parameter through check_expr's own dozen
 * pre-existing call sites -- report: minimal footprint over exhaustive
 * signature-plumbing, since only these two node kinds ever yield RECORD.
 */
static AstVarType check_expr_recinfo(Tc *tc, AstNode *e, char *out_rectype_lc,
                                     size_t cap)
{
    AstVarType t = check_expr(tc, e);
    if (cap > 0)
        out_rectype_lc[0] = '\0';
    if (tc->failed || t != AST_TY_RECORD)
        return t;
    const char *name = NULL;
    if (e->kind == AST_VARREF)
        name = e->as.varref.name;
    else if (e->kind == AST_INDEX)
        name = e->as.arrayindex.name;
    if (!name)
        return t; /* unreachable: only VARREF/INDEX ever check_expr to RECORD */
    char lc[TC_SYM_NAME_CAP];
    lower_copy(lc, sizeof(lc), name);
    AstVarType rt;
    int is_lvalue, is_array, is_record, strcap;
    long lo, hi;
    char rectype[TC_SYM_NAME_CAP];
    if (resolve_name(tc, lc, &rt, &is_lvalue, &is_array, &lo, &hi, &is_record,
                     rectype, &strcap)
        && is_record)
        snprintf(out_rectype_lc, cap, "%s", rectype);
    return t;
}

/* B8 (beads initech-ogxv; ADR-0007 DEC-05): the five thin file-I/O calls are
 * predefined statement procedures, not user-declared routines. Their AST is
 * still AST_CALL, but their signatures include storage-only FILE values and a
 * contiguous ShortString byte designator, which the general B4 scalar-call
 * checker deliberately cannot express. Keep the exceptional surface here,
 * in one exact-name dispatcher, so no typed-file/Text machinery leaks in. */
static int check_file_designator(Tc *tc, AstNode *arg, const char *verb)
{
    if (arg->kind != AST_VARREF) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s argument 1 must be a file variable", verb);
        fail_at(tc, arg->line, arg->col, msg);
        return 0;
    }
    char lc[TC_SYM_NAME_CAP];
    lower_copy(lc, sizeof(lc), arg->as.varref.name);
    AstVarType t;
    int is_lvalue, is_array, is_record, strcap;
    long lo, hi;
    char rectype[TC_SYM_NAME_CAP];
    if (!resolve_name(tc, lc, &t, &is_lvalue, &is_array, &lo, &hi,
                      &is_record, rectype, &strcap)
        || !is_lvalue || is_array || t != AST_TY_FILE) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s argument 1 must be a declared file variable", verb);
        fail_at(tc, arg->line, arg->col, msg);
        return 0;
    }
    arg->type = AST_TY_FILE;
    return 1;
}

static int check_integer_lvalue(Tc *tc, AstNode *arg, const char *verb)
{
    if (arg->kind != AST_VARREF) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s actual-count argument must be an integer variable",
                 verb);
        fail_at(tc, arg->line, arg->col, msg);
        return 0;
    }
    char lc[TC_SYM_NAME_CAP];
    lower_copy(lc, sizeof(lc), arg->as.varref.name);
    AstVarType t;
    int is_lvalue, is_array, is_record, strcap;
    long lo, hi;
    char rectype[TC_SYM_NAME_CAP];
    if (!resolve_name(tc, lc, &t, &is_lvalue, &is_array, &lo, &hi,
                      &is_record, rectype, &strcap)
        || !is_lvalue || is_array || t != AST_TY_INTEGER) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s actual-count argument must be an integer variable",
                 verb);
        fail_at(tc, arg->line, arg->col, msg);
        return 0;
    }
    arg->type = AST_TY_INTEGER;
    return 1;
}

static int check_file_buffer(Tc *tc, AstNode *arg, const char *verb)
{
    /* A ShortString byte (s[i]) is contiguous with the following content
     * bytes. Existing array-of-char storage is dword-strided, so accepting an
     * array element here would make AH=3Fh/40h silently read padding/corrupt
     * neighbors. Reject that representation mismatch loudly. */
    AstVarType t = check_expr(tc, arg);
    if (tc->failed)
        return 0;
    if (arg->kind != AST_INDEX || !arg->as.arrayindex.is_string
        || t != AST_TY_CHAR) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s buffer must be a ShortString byte designator (s[index])",
                 verb);
        fail_at(tc, arg->line, arg->col, msg);
        return 0;
    }

    /* Re-resolve the base to ensure it is writable. BlockWrite only reads it,
     * but the same concrete designator contract on both verbs keeps the ABI
     * small and BlockRead can never target a folded constant. */
    char lc[TC_SYM_NAME_CAP];
    lower_copy(lc, sizeof(lc), arg->as.arrayindex.name);
    AstVarType bt;
    int is_lvalue, is_array, is_record, strcap;
    long lo, hi;
    char rectype[TC_SYM_NAME_CAP];
    if (!resolve_name(tc, lc, &bt, &is_lvalue, &is_array, &lo, &hi,
                      &is_record, rectype, &strcap)
        || !is_lvalue || is_array || bt != AST_TY_STRING) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s buffer must belong to a writable ShortString variable",
                 verb);
        fail_at(tc, arg->line, arg->col, msg);
        return 0;
    }
    return 1;
}

static AstVarType check_file_builtin(Tc *tc, AstNode *call,
                                     FileBuiltin bi, int want_value)
{
    const char *verb = call->as.call.name;
    size_t n = call->as.call.args.count;
    if (want_value) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s is a file-I/O procedure and has no result", verb);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }

    if ((bi == FILEBI_ASSIGN && n != 2)
        || ((bi == FILEBI_RESET || bi == FILEBI_REWRITE)
            && n != 1 && n != 2)
        || ((bi == FILEBI_BLOCKREAD || bi == FILEBI_BLOCKWRITE) && n != 4)) {
        char msg[TYPECHECK_ERRMSG_CAP];
        if (bi == FILEBI_ASSIGN)
            snprintf(msg, sizeof(msg), "%s expects 2 arguments", verb);
        else if (bi == FILEBI_RESET || bi == FILEBI_REWRITE)
            snprintf(msg, sizeof(msg), "%s expects 1 or 2 arguments", verb);
        else
            snprintf(msg, sizeof(msg), "%s expects 4 arguments", verb);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }

    if (!check_file_designator(tc, call->as.call.args.items[0], verb))
        return AST_TY_UNKNOWN;

    if (bi == FILEBI_ASSIGN) {
        AstVarType nt = check_expr(tc, call->as.call.args.items[1]);
        if (tc->failed)
            return AST_TY_UNKNOWN;
        if (nt != AST_TY_STRING && nt != AST_TY_CHAR) {
            fail_at(tc, call->as.call.args.items[1]->line,
                    call->as.call.args.items[1]->col,
                    "assign filename must be a string or char");
            return AST_TY_UNKNOWN;
        }
        /* A char filename is materialized through the B7 ShortString helper. */
        tc->uses_strings = 1;
    } else if (bi == FILEBI_RESET || bi == FILEBI_REWRITE) {
        if (n == 2) {
            AstVarType rt = check_expr(tc, call->as.call.args.items[1]);
            if (tc->failed)
                return AST_TY_UNKNOWN;
            if (rt != AST_TY_INTEGER) {
                fail_at(tc, call->as.call.args.items[1]->line,
                        call->as.call.args.items[1]->col,
                        "reset/rewrite record size must be integer (B8 only "
                        "supports the byte size 1)");
                return AST_TY_UNKNOWN;
            }
        }
    } else {
        if (!check_file_buffer(tc, call->as.call.args.items[1], verb))
            return AST_TY_UNKNOWN;
        AstVarType ct = check_expr(tc, call->as.call.args.items[2]);
        if (tc->failed)
            return AST_TY_UNKNOWN;
        if (ct != AST_TY_INTEGER) {
            fail_at(tc, call->as.call.args.items[2]->line,
                    call->as.call.args.items[2]->col,
                    "blockread/blockwrite byte count must be integer");
            return AST_TY_UNKNOWN;
        }
        if (!check_integer_lvalue(tc, call->as.call.args.items[3], verb))
            return AST_TY_UNKNOWN;
    }

    tc->uses_fileio = 1;
    return AST_TY_UNKNOWN;
}

/* B4: check one call node against the callee's signature. `want_value` is 1
 * in an expression context (the callee must be a FUNCTION and the call yields
 * its result type) and 0 in a statement context (the callee must be a
 * PROCEDURE). Returns the result type in an expression context, else
 * AST_TY_UNKNOWN. */
static AstVarType check_call(Tc *tc, AstNode *call, int want_value)
{
    char lc[TC_SYM_NAME_CAP];
    lower_copy(lc, sizeof(lc), call->as.call.name);
    FileBuiltin bi = file_builtin_kind(lc);
    if (bi != FILEBI_NONE)
        return check_file_builtin(tc, call, bi, want_value);
    int idx = proc_find(tc, lc);
    if (idx < 0) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg), "call to undeclared procedure/function: %s",
                 call->as.call.name);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }
    TcProc *pr = &tc->procs[idx];

    if (want_value && !pr->has_result) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "procedure %s has no result and cannot be used in an "
                 "expression", call->as.call.name);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }
    if (!want_value && pr->has_result) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "function %s must be called in an expression; its result "
                 "may not be discarded in this subset", call->as.call.name);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }

    if ((int)call->as.call.args.count != pr->nparams) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s expects %d argument(s) but got %d", call->as.call.name,
                 pr->nparams, (int)call->as.call.args.count);
        fail_at(tc, call->line, call->col, msg);
        return AST_TY_UNKNOWN;
    }

    for (int i = 0; i < pr->nparams && !tc->failed; i++) {
        AstNode *arg = call->as.call.args.items[i];
        /* A var (by-reference) parameter requires a VARIABLE argument (an
         * lvalue) -- never a literal, constant, or compound expression. Check
         * this BEFORE type-checking the arg so the diagnostic is specific
         * (the DEEP-BUG locus: a var-param argument that is not a variable
         * would otherwise silently take the address of a temporary).
         *
         * B5 (beads initech-54uu): a single INDEXED ARRAY ELEMENT (`a[i]`,
         * AST_INDEX) is ALSO a valid var-parameter argument -- its ELEMENT
         * address is passed (codegen.c's gen_addr_of/gen_elem_addr). A bare
         * ARRAY NAME (no index) is NOT valid here -- whole-array-by-
         * reference parameters are out of scope in this subset (ast.h's B5
         * DECISION note); resolving a plain AST_VARREF to an array name is
         * therefore rejected below, not silently accepted as "the array's
         * address".
         *
         * B6 (beads initech-rug7): a bare AST_VARREF resolving to a whole
         * RECORD variable IS a valid var-parameter argument (its base
         * address is passed -- needed for symbol-table-style helpers that
         * mutate a caller's record in place); is_lvalue is NOT zeroed for
         * that case (only for a bare ARRAY name). A single FIELD
         * (`r.f`/`arr[i].f`, AST_FIELD) is ALSO valid -- a field of a real
         * record is always a scalar lvalue; its own base/field validity is
         * checked by the check_expr call below (which gives a field-specific
         * diagnostic, e.g. "unknown field", on failure -- better than this
         * generic var-param message). */
        if (pr->params[i].is_var) {
            AstVarType at = AST_TY_UNKNOWN;
            int is_lvalue = 0;
            int resolved = 0;
            int is_array = 0;
            long lo = 0, hi = 0;
            int is_record = 0;
            int arg_strcap = 0;
            char rectype[TC_SYM_NAME_CAP];
            rectype[0] = '\0';
            if (arg->kind == AST_VARREF) {
                char alc[TC_SYM_NAME_CAP];
                lower_copy(alc, sizeof(alc), arg->as.varref.name);
                resolved = resolve_name(tc, alc, &at, &is_lvalue,
                                        &is_array, &lo, &hi, &is_record,
                                        rectype, &arg_strcap);
                if (resolved && is_array)
                    is_lvalue = 0; /* a bare array name is not a scalar lvalue */
            } else if (arg->kind == AST_INDEX) {
                char alc[TC_SYM_NAME_CAP];
                lower_copy(alc, sizeof(alc), arg->as.arrayindex.name);
                int base_is_lvalue = 0;
                resolved = resolve_name(tc, alc, &at, &base_is_lvalue,
                                        &is_array, &lo, &hi, &is_record,
                                        rectype, &arg_strcap);
                is_lvalue = resolved && is_array; /* an element of a REAL array */
            } else if (arg->kind == AST_FIELD) {
                resolved = 1;
                is_lvalue = 1;
            }
            if (!resolved || !is_lvalue) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "argument %d of %s is a `var` parameter and requires "
                         "a variable, array element, or field (not a "
                         "literal, constant, whole array, or expression)",
                         i + 1, call->as.call.name);
                fail_at(tc, arg->line, arg->col, msg);
                return AST_TY_UNKNOWN;
            }
            /* B7 (beads initech-39k2; D6): a `var string` formal (always cap
             * 255) binds ONLY to a cap-255 string designator -- passing a
             * `string[N<255]` by var would let the callee write up to 255
             * bytes into an N-byte buffer (TP var-param type identity). */
            if (pr->params[i].type == AST_TY_STRING && at == AST_TY_STRING
                && arg_strcap != 255) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "argument %d of %s binds a string[%d] to a `var "
                         "string` parameter, but only a cap-255 `string` may "
                         "be passed by var (TP var-param type identity)",
                         i + 1, call->as.call.name, arg_strcap);
                fail_at(tc, arg->line, arg->col, msg);
                return AST_TY_UNKNOWN;
            }
            if (arg->kind != AST_FIELD)
                arg->type = at; /* annotate so codegen knows its type */
        }
        char arg_rectype[TC_SYM_NAME_CAP];
        AstVarType at = check_expr_recinfo(tc, arg, arg_rectype,
                                           sizeof(arg_rectype));
        if (tc->failed)
            return AST_TY_UNKNOWN;
        if (at != pr->params[i].type) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "argument %d of %s has type %s but parameter expects %s",
                     i + 1, call->as.call.name, ast_vartype_name(at),
                     ast_vartype_name(pr->params[i].type));
            fail_at(tc, arg->line, arg->col, msg);
            return AST_TY_UNKNOWN;
        }
        /* B6 (beads initech-rug7): for a RECORD-typed argument/parameter,
         * also require the SAME record type NAME -- two different record
         * types that happen to share a field layout are NOT interchangeable
         * (this subset checks record compatibility by name, never by
         * structural layout). */
        if (at == AST_TY_RECORD
            && strcmp(arg_rectype, pr->params[i].rectype) != 0) {
            /* A larger local buffer than TYPECHECK_ERRMSG_CAP (report: GCC's
             * -Wformat-truncation can see the fixed TC_SYM_NAME_CAP bounds of
             * both %s-record-type-name arguments and, combined with the
             * unbounded call->as.call.name, cannot prove 256 bytes always
             * suffices; fail_at's own snprintf into the final located
             * diagnostic already truncates safely, so this is purely a
             * silence-the-static-worst-case buffer, not a real overflow). */
            char msg[400];
            snprintf(msg, sizeof(msg),
                     "argument %d of %s is record type '%s' but parameter "
                     "expects record type '%s'",
                     i + 1, call->as.call.name, arg_rectype,
                     pr->params[i].rectype);
            fail_at(tc, arg->line, arg->col, msg);
            return AST_TY_UNKNOWN;
        }
    }

    if (pr->has_result) {
        call->type = pr->rettype;
        return call->type;
    }
    return AST_TY_UNKNOWN;
}

static AstVarType check_binop(Tc *tc, AstNode *e)
{
    AstVarType lt = check_expr(tc, e->as.binop.lhs);
    AstVarType rt = check_expr(tc, e->as.binop.rhs);
    if (tc->failed)
        return AST_TY_UNKNOWN;

    switch (e->as.binop.op) {
    case OP_ADD:
        /* B7 (beads initech-39k2; D7): `+` is CONCAT iff >= 1 operand is
         * string-typed (the other operand may be a char, coerced to a len-1
         * string); the result is a string. char + char (NO string operand)
         * falls through to the arithmetic path below and is a located type
         * error -- a deliberate TP/fpc divergence (fpc would concatenate two
         * chars; this subset does not). */
        if (lt == AST_TY_STRING || rt == AST_TY_STRING) {
            int lok = (lt == AST_TY_STRING || lt == AST_TY_CHAR);
            int rok = (rt == AST_TY_STRING || rt == AST_TY_CHAR);
            if (!lok || !rok) {
                fail_at(tc, e->line, e->col,
                        "'+' concat operands must both be string or char "
                        "(no implicit coercion from integer/boolean)");
                return AST_TY_UNKNOWN;
            }
            tc->uses_strings = 1;
            e->type = AST_TY_STRING;
            return e->type;
        }
        /* FALLTHROUGH to arithmetic. */
        /* fall through */
    case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
        if (lt != AST_TY_INTEGER || rt != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col,
                    "arithmetic operator requires integer operands");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    case OP_AND: case OP_OR:
        if (lt != AST_TY_BOOLEAN || rt != AST_TY_BOOLEAN) {
            fail_at(tc, e->line, e->col,
                    "'and'/'or' require boolean operands (complete "
                    "evaluation -- ADR-0007 DEC-03; both sides always "
                    "evaluate, so both must already be boolean)");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case OP_EQ: case OP_NE: case OP_LT: case OP_LE: case OP_GT: case OP_GE:
        /* B3 (beads initech-7mo3): char joins integer/boolean here for
         * free -- the rule is already generic ("both sides the same
         * type"), so char==char / char<char / etc. Just Work the moment
         * AST_TY_CHAR exists as a real type; no relational-specific change
         * was needed beyond this diagnostic wording.
         *
         * B6 (beads initech-rug7): record COMPARISON is REJECTED LOUDLY,
         * explicitly, BEFORE the generic "both sides the same type" check
         * below -- Turbo Pascal does not define '='/'<>'/etc. on records
         * either. This guard is load-bearing, not decoration: without it,
         * two operands of the IDENTICAL record type would satisfy
         * `lt == rt` (both AST_TY_RECORD) and fall through to the generic
         * same-type acceptance, silently permitting record `=` -- exactly
         * the deep bug the task calls out ("no record comparisons"). */
        if (lt == AST_TY_RECORD || rt == AST_TY_RECORD) {
            fail_at(tc, e->line, e->col,
                    "record types cannot be compared with relational "
                    "operators in this subset (Turbo Pascal does not define "
                    "'=' on records either; compare individual fields "
                    "instead)");
            return AST_TY_UNKNOWN;
        }
        /* B7 (beads initech-39k2; D7/D8): a relop is a STRING compare iff >= 1
         * operand is string-typed (the char side coerces to a len-1 string);
         * result boolean. Both-char stays the existing ordinal compare below.
         * __str_cmp does an unsigned bytewise compare with the shorter string
         * ordering before the longer when one is a prefix of the other. */
        if (lt == AST_TY_STRING || rt == AST_TY_STRING) {
            int lok = (lt == AST_TY_STRING || lt == AST_TY_CHAR);
            int rok = (rt == AST_TY_STRING || rt == AST_TY_CHAR);
            if (!lok || !rok) {
                fail_at(tc, e->line, e->col,
                        "string comparison operands must both be string or "
                        "char (no implicit coercion from integer/boolean)");
                return AST_TY_UNKNOWN;
            }
            tc->uses_strings = 1;
            e->type = AST_TY_BOOLEAN;
            return e->type;
        }
        if (lt == AST_TY_UNKNOWN || rt == AST_TY_UNKNOWN || lt != rt) {
            fail_at(tc, e->line, e->col,
                    "relational operands must be the same type "
                    "(both integer, both boolean, or both char)");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case OP_NEG: case OP_NOT: case OP_ORD: case OP_CHR: case OP_LENGTH:
        fail_at(tc, e->line, e->col,
                "internal: unary operator reached check_binop");
        return AST_TY_UNKNOWN;
    }
    fail_at(tc, e->line, e->col, "internal: unknown binary operator");
    return AST_TY_UNKNOWN;
}

static AstVarType check_unop(Tc *tc, AstNode *e)
{
    AstVarType t = check_expr(tc, e->as.unop.operand);
    if (tc->failed)
        return AST_TY_UNKNOWN;

    switch (e->as.unop.op) {
    case OP_NEG:
        if (t != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col,
                    "unary '-' requires an integer operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    case OP_NOT:
        if (t != AST_TY_BOOLEAN) {
            fail_at(tc, e->line, e->col,
                    "'not' requires a boolean operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_BOOLEAN;
        return e->type;
    /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "ord, chr"). */
    case OP_ORD:
        /* DECISION (report): ord() is TOTAL over every ordinal type this
         * subset has -- char, boolean, AND integer. ord(anInteger) is the
         * identity (real Pascal: Integer already IS an ordinal type, so
         * Ord(anInteger) = anInteger -- Borland Turbo Pascal 7.0 Language
         * Guide, "Ord": "Ord returns a value of type Longint that is the
         * ordinal number of X" for any ordinal X). ord(boolean) is
         * IMPLEMENTED, not rejected (report per the bead): TP allows it and
         * gives 0/1, and the runtime already represents boolean as 0/1 in
         * eax (B1), so ord(boolean) costs zero extra codegen -- exactly the
         * same "value no-op" as ord(char)/ord(integer) (see codegen.c's
         * OP_ORD case). */
        if (t != AST_TY_CHAR && t != AST_TY_BOOLEAN && t != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col,
                    "'ord' requires a char, boolean, or integer operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    case OP_CHR:
        /* chr() is defined only on an integer argument (ISO 7185 / Turbo
         * Pascal; there is no chr(char) or chr(boolean)). Out-of-range
         * (0..255) behaviour: see codegen.c's OP_CHR case for the 8-bit
         * TRUNCATION decision -- this pass only enforces the operand type,
         * not the range (a compile-time-unknown value can't be range-
         * checked here anyway; only a runtime concern). */
        if (t != AST_TY_INTEGER) {
            fail_at(tc, e->line, e->col, "'chr' requires an integer operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_CHAR;
        return e->type;
    /* B7 (beads initech-39k2; ADR-0007 DEC-02 "length"): length(s) requires a
     * string argument; the result is an integer (the ShortString length
     * prefix, 0..255). */
    case OP_LENGTH:
        if (t != AST_TY_STRING) {
            fail_at(tc, e->line, e->col,
                    "'length' requires a string operand");
            return AST_TY_UNKNOWN;
        }
        e->type = AST_TY_INTEGER;
        return e->type;
    default:
        fail_at(tc, e->line, e->col, "internal: unknown unary operator");
        return AST_TY_UNKNOWN;
    }
}

static AstVarType check_expr(Tc *tc, AstNode *e)
{
    if (tc->failed || !e)
        return AST_TY_UNKNOWN;

    switch (e->kind) {
    case AST_INTLIT:
        e->type = AST_TY_INTEGER;
        return e->type;
    case AST_BOOLLIT:
        e->type = AST_TY_BOOLEAN;
        return e->type;
    case AST_CHARLIT:
        /* B3 (beads initech-7mo3). */
        e->type = AST_TY_CHAR;
        return e->type;
    case AST_VARREF: {
        /* B4 (beads initech-63ce): resolve in the active routine's local
         * scope first (a param/local/result shadows a global), then globals.
         * A bare identifier is ALWAYS a variable/const/result reference -- a
         * call always has parentheses (parser.c). */
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), e->as.varref.name);
        AstVarType t;
        int is_lvalue;
        int is_array;
        long lo, hi;
        int is_record;
        int strcap;
        char rectype[TC_SYM_NAME_CAP];
        if (!resolve_name(tc, lc, &t, &is_lvalue, &is_array, &lo, &hi,
                          &is_record, rectype, &strcap)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     e->as.varref.name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        (void)lo; (void)hi; /* a bare-name reference never needs the bounds */
        (void)is_record; (void)rectype; /* codegen re-derives sizing itself */
        (void)strcap;                   /* a bare string value needs no cap */
        /* B5 (beads initech-54uu): a bare array name is not a value in this
         * subset -- whole-array use (read, write, pass-by-value/reference)
         * is out of scope; the only legal use of an array is indexed
         * (AST_INDEX, handled below). B6 (beads initech-rug7): a bare RECORD
         * name IS a legal value here (unlike an array) -- it is the base
         * designator for a field access, a whole-record assignment's RHS, or
         * a `var`-parameter argument; nothing about a plain record reference
         * needs rejecting. */
        if (is_array) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "array '%s' used without an index (expected %s[expr])",
                     e->as.varref.name, e->as.varref.name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        if (t == AST_TY_FILE) {
            fail_at(tc, e->line, e->col,
                    "file variables are storage-only and may be used only "
                    "with assign/reset/rewrite/blockread/blockwrite");
            return AST_TY_UNKNOWN;
        }
        e->type = t;
        return e->type;
    }
    /* B5 (beads initech-54uu; ADR-0007 DEC-02 "static arrays ... indexed as
     * both l-value and r-value"): `name[index]` as an r-value. */
    case AST_INDEX: {
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), e->as.arrayindex.name);
        AstVarType elem;
        int is_lvalue, is_array;
        long lo, hi;
        int is_record;
        int strcap;
        char rectype[TC_SYM_NAME_CAP];
        if (!resolve_name(tc, lc, &elem, &is_lvalue, &is_array, &lo, &hi,
                          &is_record, rectype, &strcap)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     e->as.arrayindex.name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        (void)is_lvalue; (void)lo; (void)hi; /* codegen re-resolves bounds */
        (void)is_record; (void)rectype; /* codegen re-derives sizing itself */
        /* B7 (beads initech-39k2; D5): `s[i]` on a STRING is a 1-based BYTE
         * read (a char r-value), NOT an array element. Disambiguated here by
         * the base designator's TYPE (the highest-risk AST_INDEX reuse --
         * ast.h's B7 block); the index must be integer, s[i] is char-typed,
         * and arrayindex.is_string is set so codegen byte-loads instead of
         * dword-loading. NO bounds check ({$R-}). */
        if (elem == AST_TY_STRING) {
            (void)strcap;
            AstVarType sit = check_expr(tc, e->as.arrayindex.index);
            if (tc->failed)
                return AST_TY_UNKNOWN;
            if (sit != AST_TY_INTEGER) {
                fail_at(tc, e->as.arrayindex.index->line,
                        e->as.arrayindex.index->col,
                        "string index must be an integer expression");
                return AST_TY_UNKNOWN;
            }
            e->as.arrayindex.is_string = 1;
            e->type = AST_TY_CHAR;
            return e->type;
        }
        if (!is_array) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "'%s' is not an array (only array variables may be "
                     "indexed)", e->as.arrayindex.name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        AstVarType it = check_expr(tc, e->as.arrayindex.index);
        if (tc->failed)
            return AST_TY_UNKNOWN;
        if (it != AST_TY_INTEGER) {
            fail_at(tc, e->as.arrayindex.index->line,
                    e->as.arrayindex.index->col,
                    "array index must be an integer expression");
            return AST_TY_UNKNOWN;
        }
        /* B6 (beads initech-rug7): elem may now be AST_TY_RECORD (an
         * array-of-record element) -- no special handling needed here
         * beyond passing it through: an element used bare (no `.field`) is
         * the whole-record-element value, legal as a designator exactly
         * like a plain record var (see AST_VARREF above). */
        e->type = elem;
        return e->type;
    }
    /* B6 (beads initech-rug7; ADR-0007 DEC-02 "field access"): `base.field`
     * as an r-value. `base` is always AST_VARREF or AST_INDEX (never
     * AST_FIELD -- fields are scalar-only, ast.h's B6 note). */
    case AST_FIELD: {
        char rectype_lc[TC_SYM_NAME_CAP];
        AstVarType bt = check_expr_recinfo(tc, e->as.field.base, rectype_lc,
                                           sizeof(rectype_lc));
        if (tc->failed)
            return AST_TY_UNKNOWN;
        if (bt != AST_TY_RECORD) {
            fail_at(tc, e->line, e->col,
                    "'.' field access requires a record variable or "
                    "array-of-record element");
            return AST_TY_UNKNOWN;
        }
        int rti = rectype_find(tc, rectype_lc);
        if (rti < 0) {
            fail_at(tc, e->line, e->col,
                    "internal: unknown record type in field access "
                    "(typecheck)");
            return AST_TY_UNKNOWN;
        }
        char flc[TC_SYM_NAME_CAP];
        lower_copy(flc, sizeof(flc), e->as.field.field);
        int fi = rectype_field_find(&tc->rectypes[rti], flc);
        if (fi < 0) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "unknown field '%s' in record type "
                     "'%s'", e->as.field.field, tc->rectypes[rti].name);
            fail_at(tc, e->line, e->col, msg);
            return AST_TY_UNKNOWN;
        }
        e->as.field.field_index = fi;
        e->type = tc->rectypes[rti].fields[fi].type;
        return e->type;
    }
    /* B7 (beads initech-39k2; D7): a multi-char or empty '...' reached in an
     * EXPRESSION context (parse_factor built an AST_STRLIT, or a string const
     * folded to one) is a string-typed r-value. (A write-argument bare literal
     * is skipped in check_stmt's WRITE case and never reaches here.) */
    case AST_STRLIT:
        tc->uses_strings = 1;
        e->type = AST_TY_STRING;
        return e->type;
    case AST_CALL:
        /* B4: a function call used as an expression value. */
        return check_call(tc, e, /*want_value=*/1);
    case AST_BINOP:
        return check_binop(tc, e);
    case AST_UNOP:
        return check_unop(tc, e);
    default:
        fail_at(tc, e->line, e->col,
                "internal: non-expression node in expression context "
                "(typecheck)");
        return AST_TY_UNKNOWN;
    }
}

static void check_stmt(Tc *tc, AstNode *n)
{
    if (tc->failed || !n)
        return;

    switch (n->kind) {
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count && !tc->failed; i++)
            check_stmt(tc, n->as.block.stmts.items[i]);
        return;
    case AST_ASSIGN: {
        /* B4 (beads initech-63ce): resolve the target in local-then-global
         * scope. Assigning to a function's own name writes its RESULT
         * variable (a local of kind TC_KIND_RESULT); a `var` parameter,
         * value parameter, and local are all writable. A constant is not. */
        char lc[TC_SYM_NAME_CAP];
        lower_copy(lc, sizeof(lc), n->as.assign.name);
        AstVarType vt;
        int is_lvalue;
        int is_array;
        long lo, hi;
        int is_record;
        int strcap;
        char rectype[TC_SYM_NAME_CAP];
        if (!resolve_name(tc, lc, &vt, &is_lvalue, &is_array, &lo, &hi,
                          &is_record, rectype, &strcap)) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "undeclared variable: %s",
                     n->as.assign.name);
            fail_at(tc, n->line, n->col, msg);
            return;
        }

        if (vt == AST_TY_FILE) {
            fail_at(tc, n->line, n->col,
                    "file variables cannot be assigned; use assign/reset/"
                    "rewrite and blockread/blockwrite");
            return;
        }

        /* B7 (beads initech-39k2; D5): a STRING target -- handled FIRST (it
         * takes over the "how is the target validated" question from the
         * field/index/record/scalar paths below). `strcap>0` stamped on the
         * assign node is codegen's string-target marker AND the truncation
         * cap. `index` NULL => a WHOLE-string assign (`s := <string|char>`,
         * silent truncation to strcap); `index` non-NULL => an s[i] BYTE write
         * (`s[i] := <char>`, no length change). A string has no fields. */
        if (vt == AST_TY_STRING) {
            if (n->as.assign.field) {
                fail_at(tc, n->line, n->col,
                        "'.field' cannot be applied to a string "
                        "(strings have no fields in this subset)");
                return;
            }
            if (!is_lvalue) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg), "cannot assign to a constant: %s",
                         n->as.assign.name);
                fail_at(tc, n->line, n->col, msg);
                return;
            }
            if (n->as.assign.index) {
                AstVarType it = check_expr(tc, n->as.assign.index);
                if (tc->failed)
                    return;
                if (it != AST_TY_INTEGER) {
                    fail_at(tc, n->as.assign.index->line,
                            n->as.assign.index->col,
                            "string index must be an integer expression");
                    return;
                }
                AstVarType vvt = check_expr(tc, n->as.assign.value);
                if (tc->failed)
                    return;
                if (vvt != AST_TY_CHAR) {
                    fail_at(tc, n->line, n->col,
                            "s[i] := <expr>: the right side must be a char "
                            "(a string element is a single byte)");
                    return;
                }
                n->as.assign.strcap = strcap;
                return;
            }
            AstVarType vvt = check_expr(tc, n->as.assign.value);
            if (tc->failed)
                return;
            if (vvt != AST_TY_STRING && vvt != AST_TY_CHAR) {
                fail_at(tc, n->line, n->col,
                        "string assignment: the right side must be a string "
                        "or a char (no implicit coercion from integer/"
                        "boolean)");
                return;
            }
            n->as.assign.strcap = strcap;
            return;
        }

        /* B6 (beads initech-rug7): a FIELD assignment -- `name.field := v`
         * (index NULL) or `name[index].field := v` (index non-NULL,
         * composing the array case) -- checked FIRST since it takes over
         * the "how is the base validated" question entirely from the
         * ordinary index/bare-name paths below. */
        if (n->as.assign.field) {
            AstVarType base_type = vt;
            int base_is_record = is_record;
            char base_rectype[TC_SYM_NAME_CAP];
            snprintf(base_rectype, sizeof(base_rectype), "%s", rectype);
            if (n->as.assign.index) {
                if (!is_array) {
                    char msg[TYPECHECK_ERRMSG_CAP];
                    snprintf(msg, sizeof(msg),
                             "'%s' is not an array (indexed field assignment "
                             "requires an array-of-record variable)",
                             n->as.assign.name);
                    fail_at(tc, n->line, n->col, msg);
                    return;
                }
                AstVarType it = check_expr(tc, n->as.assign.index);
                if (tc->failed)
                    return;
                if (it != AST_TY_INTEGER) {
                    fail_at(tc, n->as.assign.index->line,
                            n->as.assign.index->col,
                            "array index must be an integer expression");
                    return;
                }
                /* base_type/base_is_record/base_rectype already describe
                 * the ARRAY's ELEMENT (resolve_name's "reuse type for the
                 * element" convention) -- unchanged from the vt/is_record/
                 * rectype captured above. */
            } else if (is_array) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "array '%s' needs an index before '.field' "
                         "(expected %s[expr].field)",
                         n->as.assign.name, n->as.assign.name);
                fail_at(tc, n->line, n->col, msg);
                return;
            }
            if (!base_is_record || base_type != AST_TY_RECORD) {
                fail_at(tc, n->line, n->col,
                        "'.' field access requires a record variable or "
                        "array-of-record element");
                return;
            }
            int rti = rectype_find(tc, base_rectype);
            if (rti < 0) {
                fail_at(tc, n->line, n->col,
                        "internal: unknown record type in field assignment "
                        "(typecheck)");
                return;
            }
            char flc[TC_SYM_NAME_CAP];
            lower_copy(flc, sizeof(flc), n->as.assign.field);
            int fi = rectype_field_find(&tc->rectypes[rti], flc);
            if (fi < 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "unknown field '%s' in record type '%s'",
                         n->as.assign.field, tc->rectypes[rti].name);
                fail_at(tc, n->line, n->col, msg);
                return;
            }
            n->as.assign.field_index = fi;
            AstVarType ft = tc->rectypes[rti].fields[fi].type;
            AstVarType vvt = check_expr(tc, n->as.assign.value);
            if (tc->failed)
                return;
            if (vvt != ft) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "type mismatch in field assignment to %s.%s: "
                         "expected %s, got %s (no implicit coercion in this "
                         "subset)",
                         n->as.assign.name, n->as.assign.field,
                         ast_vartype_name(ft), ast_vartype_name(vvt));
                fail_at(tc, n->line, n->col, msg);
            }
            return;
        }

        /* B5 (beads initech-54uu): an INDEXED target (`name[index] := v`)
         * takes a DIFFERENT path from a plain scalar assignment -- the name
         * must be a real array, the index must be integer, and the value
         * must match the ELEMENT type (vt, from resolve_name's "reuse type
         * for the element" convention). B6 (beads initech-rug7): when the
         * ELEMENT type is a record, this is a WHOLE-RECORD ELEMENT
         * assignment (`toks[i] := t`) instead of an ordinary scalar one. */
        if (n->as.assign.index) {
            if (!is_array) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "'%s' is not an array (indexed assignment requires "
                         "an array variable)", n->as.assign.name);
                fail_at(tc, n->line, n->col, msg);
                return;
            }
            (void)lo; (void)hi; /* codegen re-resolves bounds for addressing */
            AstVarType it = check_expr(tc, n->as.assign.index);
            if (tc->failed)
                return;
            if (it != AST_TY_INTEGER) {
                fail_at(tc, n->as.assign.index->line, n->as.assign.index->col,
                        "array index must be an integer expression");
                return;
            }
            if (vt == AST_TY_RECORD) {
                char value_rectype[TC_SYM_NAME_CAP];
                AstVarType vvt = check_expr_recinfo(tc, n->as.assign.value,
                                                    value_rectype,
                                                    sizeof(value_rectype));
                if (tc->failed)
                    return;
                if (vvt != AST_TY_RECORD
                    || strcmp(value_rectype, rectype) != 0) {
                    char msg[TYPECHECK_ERRMSG_CAP];
                    snprintf(msg, sizeof(msg),
                             "type mismatch in whole-record assignment to "
                             "%s[...]: expected record type '%s'",
                             n->as.assign.name, rectype);
                    fail_at(tc, n->line, n->col, msg);
                    return;
                }
                int rti = rectype_find(tc, rectype);
                n->as.assign.rec_fields = (rti >= 0)
                                         ? tc->rectypes[rti].nfields : 0;
                return;
            }
            AstVarType vvt = check_expr(tc, n->as.assign.value);
            if (tc->failed)
                return;
            if (vvt != vt) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "type mismatch in indexed assignment to %s: "
                         "expected %s, got %s (no implicit coercion in this "
                         "subset)",
                         n->as.assign.name, ast_vartype_name(vt),
                         ast_vartype_name(vvt));
                fail_at(tc, n->line, n->col, msg);
            }
            return;
        }
        /* B5: a bare array name is never a valid SCALAR assignment target --
         * whole-array assignment is out of scope in this subset (see ast.h's
         * B5 DECISION note). */
        if (is_array) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "cannot assign to array '%s' as a whole (this subset "
                     "requires an index: %s[expr] := ...)",
                     n->as.assign.name, n->as.assign.name);
            fail_at(tc, n->line, n->col, msg);
            return;
        }
        /* B6 (beads initech-rug7): a bare RECORD-typed target -- WHOLE-RECORD
         * assignment (`r1 := r2`). Checked by TYPE NAME, not structural
         * layout (two different record types with the same field shape are
         * NOT assignment-compatible). */
        if (vt == AST_TY_RECORD) {
            if (!is_lvalue) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg), "cannot assign to a constant: %s",
                         n->as.assign.name);
                fail_at(tc, n->line, n->col, msg);
                return;
            }
            char value_rectype[TC_SYM_NAME_CAP];
            AstVarType vvt = check_expr_recinfo(tc, n->as.assign.value,
                                                value_rectype,
                                                sizeof(value_rectype));
            if (tc->failed)
                return;
            if (vvt != AST_TY_RECORD || strcmp(value_rectype, rectype) != 0) {
                char msg[TYPECHECK_ERRMSG_CAP];
                snprintf(msg, sizeof(msg),
                         "type mismatch in whole-record assignment to %s: "
                         "expected record type '%s'",
                         n->as.assign.name, rectype);
                fail_at(tc, n->line, n->col, msg);
                return;
            }
            int rti = rectype_find(tc, rectype);
            n->as.assign.rec_fields = (rti >= 0) ? tc->rectypes[rti].nfields
                                                 : 0;
            return;
        }
        /* B3 (beads initech-7mo3): a const shares the var namespace but may
         * never be an assignment TARGET -- "cannot assign to a constant" is
         * the same diagnostic real Pascal compilers give. */
        if (!is_lvalue) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg), "cannot assign to a constant: %s",
                     n->as.assign.name);
            fail_at(tc, n->line, n->col, msg);
            return;
        }
        AstVarType et = check_expr(tc, n->as.assign.value);
        if (tc->failed)
            return;
        if (et != vt) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "type mismatch in assignment to %s: expected %s, "
                     "got %s (no implicit coercion in this subset)",
                     n->as.assign.name, ast_vartype_name(vt),
                     ast_vartype_name(et));
            fail_at(tc, n->line, n->col, msg);
        }
        return;
    }
    case AST_CALL:
        /* B4: a procedure call used as a statement. */
        check_call(tc, n, /*want_value=*/0);
        return;
    case AST_WRITE:
    case AST_WRITELN:
        for (size_t i = 0; i < n->as.write.args.count && !tc->failed; i++) {
            AstNode *arg = n->as.write.args.items[i];
            if (arg->kind == AST_STRLIT)
                continue; /* string args are never type-checked as exprs */
            AstVarType t = check_expr(tc, arg);
            if (tc->failed)
                return;
            /* B3 (beads initech-7mo3): char joins integer/boolean as a
             * valid write/writeln expression-argument type -- codegen's
             * gen_write picks the byte-emitting path (serial_putc) for it,
             * never decimal formatting. */
            /* B7 (beads initech-39k2): a STRING expression argument (a string
             * variable or a concat) is written via __str_write; a bare quoted
             * write LITERAL keeps the pre-B7 serial_puts path (skipped above
             * as AST_STRLIT). */
            if (t != AST_TY_INTEGER && t != AST_TY_BOOLEAN
                && t != AST_TY_CHAR && t != AST_TY_STRING) {
                fail_at(tc, arg->line, arg->col,
                        "write/writeln argument must be integer, boolean, "
                        "char, or string");
                return;
            }
        }
        return;
    /* B2 (beads initech-80iw; ADR-0007 DEC-02). if/while are the only new
     * statement kinds -- for/repeat are desugared by the parser into
     * assign/while/block/not nodes BEFORE typecheck ever runs, so their
     * "guard must be boolean" (repeat, via the desugared 'not') and
     * "bounds must be integer" (for, via the desugared assignments and the
     * relational guard) requirements are enforced by the ordinary
     * AST_ASSIGN/AST_BINOP/AST_UNOP rules above -- no extra cases needed. */
    case AST_IF: {
        AstNode *cond = n->as.ifstmt.cond;
        AstVarType ct = check_expr(tc, cond);
        if (tc->failed)
            return;
        if (ct != AST_TY_BOOLEAN) {
            fail_at(tc, cond->line, cond->col,
                    "'if' condition must be boolean");
            return;
        }
        check_stmt(tc, n->as.ifstmt.then_stmt);
        if (tc->failed)
            return;
        check_stmt(tc, n->as.ifstmt.else_stmt);
        return;
    }
    case AST_WHILE: {
        AstNode *cond = n->as.whilestmt.cond;
        AstVarType ct = check_expr(tc, cond);
        if (tc->failed)
            return;
        if (ct != AST_TY_BOOLEAN) {
            fail_at(tc, cond->line, cond->col,
                    "'while' condition must be boolean");
            return;
        }
        check_stmt(tc, n->as.whilestmt.body);
        return;
    }
    default:
        fail_at(tc, n->line, n->col,
                "internal: unexpected statement node in typecheck");
        return;
    }
}

/* ------------------------------------------------------------------ */
/* B4 (beads initech-63ce): per-routine body checking                   */
/* ------------------------------------------------------------------ */
/* Add one entry to the active local scope, rejecting a duplicate WITHIN the
 * routine (a param/local/result colliding with another) and capacity
 * overflow (Rule 2). A local may freely shadow a global (not checked here).
 * B5 (beads initech-54uu): is_array/lo/hi are 0 for every non-array entry
 * (params and the result are never arrays -- ast.h's B5 DECISION note); a
 * LOCAL array passes its declared bounds through.
 * B6 (beads initech-rug7): `rectype` (arena-owned, or NULL for a scalar
 * entry) names a record type -- valid for a `var` parameter or a local
 * (scalar or array-of-record); never for a value parameter (rejected at
 * parse time) or the function result (record results out of scope). */
static void add_local(Tc *tc, int line, int col, const char *name_lc,
                      AstVarType type, TcLocalKind kind, const char *what,
                      int is_array, long lo, long hi, const char *rectype,
                      int strcap)
{
    if (tc->failed)
        return;
    if (file_builtin_kind(name_lc) != FILEBI_NONE) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "%s collides with a file-I/O builtin in a routine scope: %s",
                 what, name_lc);
        fail_at(tc, line, col, msg);
        return;
    }
    if (local_find(tc, name_lc) >= 0) {
        char msg[TYPECHECK_ERRMSG_CAP];
        snprintf(msg, sizeof(msg),
                 "duplicate %s in a routine scope: %s", what, name_lc);
        fail_at(tc, line, col, msg);
        return;
    }
    if (tc->nlocal >= (int)(sizeof(tc->local) / sizeof(tc->local[0]))) {
        fail_at(tc, line, col,
                "too many parameters/locals in one routine "
                "(scope table exceeded)");
        return;
    }
    lower_copy(tc->local[tc->nlocal].name,
               sizeof(tc->local[tc->nlocal].name), name_lc);
    tc->local[tc->nlocal].type = type;
    tc->local[tc->nlocal].kind = kind;
    tc->local[tc->nlocal].is_array = is_array;
    tc->local[tc->nlocal].lo = lo;
    tc->local[tc->nlocal].hi = hi;
    if (type == AST_TY_RECORD && rectype) {
        tc->local[tc->nlocal].is_record = 1;
        lower_copy(tc->local[tc->nlocal].rectype,
                   sizeof(tc->local[tc->nlocal].rectype), rectype);
        if (rectype_find(tc, tc->local[tc->nlocal].rectype) < 0) {
            fail_at(tc, line, col,
                    "internal: unknown record type in local scope "
                    "(typecheck)");
            return;
        }
    } else {
        tc->local[tc->nlocal].is_record = 0;
        tc->local[tc->nlocal].rectype[0] = '\0';
    }
    /* B7 (beads initech-39k2): a string local/`var` parameter carries its
     * capacity; seeing one marks the program as using strings. */
    tc->local[tc->nlocal].strcap = (type == AST_TY_STRING) ? strcap : 0;
    if (type == AST_TY_STRING)
        tc->uses_strings = 1;
    tc->nlocal++;
}

/* Build the routine's local scope (params, then the function result
 * variable, then locals) and type-check its body. The main program block is
 * checked with an empty local scope (globals only). */
static void check_routine(Tc *tc, const AstNode *pf)
{
    if (pf->as.procfunc.is_forward)
        return; /* a forward declaration has no body to check */

    tc->nlocal = 0;
    tc->in_routine = 1;

    for (size_t i = 0; i < pf->as.procfunc.params.count && !tc->failed; i++) {
        const AstNode *pn = pf->as.procfunc.params.items[i];
        char plc[TC_SYM_NAME_CAP];
        lower_copy(plc, sizeof(plc), pn->as.param.name);
        add_local(tc, pn->line, pn->col, plc, pn->as.param.ptype,
                  pn->as.param.is_var ? TC_KIND_VARPARAM : TC_KIND_VALPARAM,
                  "parameter", /*is_array=*/0, 0, 0, pn->as.param.rectype,
                  pn->as.param.strcap);
    }

    if (!tc->failed && pf->as.procfunc.has_result) {
        char rlc[TC_SYM_NAME_CAP];
        lower_copy(rlc, sizeof(rlc), pf->as.procfunc.name);
        add_local(tc, pf->line, pf->col, rlc, pf->as.procfunc.rettype,
                  TC_KIND_RESULT, "name (a parameter shadows the result)",
                  /*is_array=*/0, 0, 0, /*rectype=*/NULL, /*strcap=*/0);
    }

    for (size_t i = 0; i < pf->as.procfunc.decls.count && !tc->failed; i++) {
        const AstNode *vd = pf->as.procfunc.decls.items[i];
        if (vd->kind != AST_VARDECL) {
            fail_at(tc, vd->line, vd->col,
                    "internal: non-vardecl in routine locals (typecheck)");
            break;
        }
        for (size_t j = 0; j < vd->as.vardecl.names.count && !tc->failed; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            char vlc[TC_SYM_NAME_CAP];
            lower_copy(vlc, sizeof(vlc), vr->as.varref.name);
            /* B5 (beads initech-54uu): a LOCAL array (frame-resident --
             * codegen.c's cg_build_scope allocates it a contiguous run of
             * frame slots). vd->as.vardecl.vtype already holds the ELEMENT
             * type when is_array is set (parser.c). */
            add_local(tc, vr->line, vr->col, vlc, vd->as.vardecl.vtype,
                      TC_KIND_LOCAL, "local variable",
                      vd->as.vardecl.is_array, vd->as.vardecl.lo,
                      vd->as.vardecl.hi, vd->as.vardecl.rectype,
                      vd->as.vardecl.strcap);
        }
    }

    if (!tc->failed)
        check_stmt(tc, pf->as.procfunc.body);

    tc->nlocal = 0;
    tc->in_routine = 0;
}

/* ------------------------------------------------------------------ */
/* Public entry                                                        */
/* ------------------------------------------------------------------ */
int typecheck_program(AstNode *program, TypeCheckResult *out)
{
    Tc tc;
    memset(&tc, 0, sizeof(tc));

    if (!program || program->kind != AST_PROGRAM) {
        out->ok = 0;
        snprintf(out->error, sizeof(out->error),
                 "internal: typecheck root is not a program");
        out->line = 0;
        out->col = 0;
        return 1;
    }

    /* B6 (beads initech-rug7): record TYPES first (Pass 0) -- a var/param/
     * array-element declaration can name a record type, so the full
     * name->fields table must exist before collect_decls resolves any of
     * them. Then globals, then routine signatures (so a routine body --
     * checked next -- can see every global and can call any routine,
     * including ones declared later, which is why mutual recursion needs
     * the `forward` declaration to register the signature ahead of the call
     * site). */
    collect_types(&tc, &program->as.program.decls);
    if (!tc.failed)
        collect_decls(&tc, &program->as.program.decls);
    if (!tc.failed)
        collect_procs(&tc, &program->as.program.decls);

    /* Each routine body, in source order, in its own local scope. */
    for (size_t i = 0; i < program->as.program.decls.count && !tc.failed; i++) {
        AstNode *d = program->as.program.decls.items[i];
        if (d->kind == AST_PROCDECL || d->kind == AST_FUNCDECL)
            check_routine(&tc, d);
    }

    /* The main program block, with an empty local scope (globals only). */
    if (!tc.failed) {
        tc.nlocal = 0;
        tc.in_routine = 0;
        check_stmt(&tc, program->as.program.block);
    }

    /* Every `forward` declaration must have been defined (Rule 2: a missing
     * definition is a link-time undefined symbol, caught here as a located
     * source error instead). */
    for (int i = 0; i < tc.nprocs && !tc.failed; i++) {
        if (!tc.procs[i].is_defined) {
            char msg[TYPECHECK_ERRMSG_CAP];
            snprintf(msg, sizeof(msg),
                     "forward declaration never defined: %s", tc.procs[i].name);
            fail_at(&tc, tc.procs[i].line, tc.procs[i].col, msg);
        }
    }

    if (tc.failed) {
        out->ok = 0;
        snprintf(out->error, sizeof(out->error), "%s", tc.errmsg);
        out->line = tc.errline;
        out->col = tc.errcol;
        return 1;
    }

    /* B7 (beads initech-39k2): hand codegen the "does this program use
     * strings?" verdict computed during the walk above (no new pass), so it
     * emits the __str_* intrinsic prelude only when needed -- keeping a
     * stringless program's .s byte-identical to pre-B7. */
    program->as.program.uses_strings = tc.uses_strings;
    /* B8 (beads initech-ogxv): same deterministic verdict pattern as B7's
     * uses_strings, but for the external hand-assembled file RTL. */
    program->as.program.uses_fileio = tc.uses_fileio;

    out->ok = 1;
    out->error[0] = '\0';
    out->line = 0;
    out->col = 0;
    return 0;
}
