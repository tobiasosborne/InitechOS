/*
 * codegen.c -- Step B seed codegen: AST -> nasm Intel-syntax 32-bit x86.
 *
 * beads: initech-znb ("Step B of the InitechOS seed cross-compiler").
 * Ref:   PRD Sec 6.7 -- "Single-pass front end; stack-machine codegen
 *        (expression evaluation to x86, fixed register conventions, NO
 *        optimizer, NO register allocator) -- small and sufficient." This
 *        seed pass is an AST walk (correctness over single-pass purity; the
 *        RESIDENT compiler is the single-pass artifact, PRD Sec 4 /
 *        CLAUDE.md "two compilers, never conflated").
 *        CLAUDE.md Law 1 (cite) / Law 2 (oracle is truth) / Rule 2 (fail
 *        loud) / Rule 11 (deterministic, no timestamps) / Rule 12 (ASCII).
 *
 * ============================================================================
 * FIXED CALLING / STACK-MACHINE CONVENTIONS
 * ============================================================================
 * The compiled program is one nasm function `pas_main`, called by the runtime
 * (seed/rt/start.asm) after it sets up a stack. `pas_main` follows the
 * standard cdecl frame (push ebp / mov ebp,esp / ... / leave / ret) and may
 * clobber eax/ecx/edx freely (caller-saved); the runtime expects nothing back.
 *
 * EXPRESSIONS evaluate onto the x86 data stack, postorder:
 *   - Every expression leaves its 32-bit signed result in EAX.
 *   - For a binary op `lhs OP rhs`:
 *         eval lhs  -> eax
 *         push eax                  ; spill lhs to the stack
 *         eval rhs  -> eax          ; (may itself recurse/push)
 *         mov ecx, eax              ; rhs -> ecx
 *         pop eax                   ; lhs -> eax
 *         <combine eax (lhs), ecx (rhs)> -> eax
 *     This spill-to-stack discipline is the "stack machine": no register
 *     allocator, fixed registers, correct for arbitrary nesting.
 *   - EAX = working/result register; ECX = popped rhs scratch.
 *   - EDX is reserved as the idiv high-word / sign-extension register.
 *
 * OPERATORS:
 *   +   add eax, ecx
 *   -   sub eax, ecx                 (lhs - rhs)
 *   *   imul eax, ecx                (signed 32x32 -> low 32)
 *   div cdq ; idiv ecx -> quotient in eax   (Pascal `div`)
 *   mod cdq ; idiv ecx -> remainder in edx ; mov eax, edx   (Pascal `mod`)
 *   unary -  neg eax
 *
 *   B1 (beads initech-f0uc; ADR-0007 DEC-02/DEC-03) relational + boolean:
 *   =   cmp eax, ecx ; sete  al ; movzx eax, al
 *   <>  cmp eax, ecx ; setne al ; movzx eax, al
 *   <   cmp eax, ecx ; setl  al ; movzx eax, al
 *   <=  cmp eax, ecx ; setle al ; movzx eax, al
 *   >   cmp eax, ecx ; setg  al ; movzx eax, al
 *   >=  cmp eax, ecx ; setge al ; movzx eax, al
 *   and and eax, ecx             (both operands are already 0/1)
 *   or  or  eax, ecx
 *   not xor eax, 1               (unary; see gen_expr's AST_UNOP case)
 *
 *   COMPLETE EVALUATION (ADR-0007 DEC-03, NOT short-circuit): gen_binop
 *   below is the ONE shared code path for every AST_BINOP, arithmetic OR
 *   boolean. It always emits, unconditionally: eval lhs -> push -> eval rhs
 *   -> combine. There is NO conditional-branch ("j..") codegen path in
 *   gen_binop for and/or (or for any other operator) that could skip
 *   evaluating an operand -- and/or are bitwise `and`/`or` on the two
 *   already-evaluated 0/1 results, exactly like `add`/`sub`/`imul` are for
 *   arithmetic. This is a STRUCTURAL guarantee (both operands of every
 *   binop, including and/or, always emit code and always execute at
 *   runtime), not a claim resting on any particular test input; the
 *   Makefile's test-seed-codegen target greps gen_binop's SOURCE for the
 *   absence of any jump mnemonic as a mechanical check of this invariant
 *   (Rule 6 spirit: prove it, don't just assert it in a comment). A
 *   dynamic, input-dependent proof (an operand that traps under
 *   short-circuit misuse) is not expressible in this subset until
 *   procedures/functions exist (B4, ADR-0007 DEC-07 B-chain) -- see
 *   seed/examples/arith/bool.pas's header comment for the fixture-level
 *   half of this proof.
 *
 *   Pascal `div`/`mod` are integer division. x86 `idiv` is signed and
 *   TRUNCATES TOWARD ZERO, which matches ISO-Pascal `div` for operands of the
 *   same sign and is the Turbo-Pascal behaviour. (`mod` follows the sign of
 *   the dividend on x86; that, too, is Turbo-Pascal's `mod`.) Negative-operand
 *   semantics are now exercised end-to-end by
 *   seed/examples/arith/negative_divmod.pas (beads initech-tf3c): `cdq` +
 *   `idiv` needs no sign-correction because the x86 instruction's native
 *   trunc-toward-zero quotient / dividend-signed remainder IS Turbo Pascal's
 *   div/mod, so this codegen was correct by construction the whole time --
 *   the gap Rule 6 flagged was test coverage, not a latent bug (confirmed by
 *   running the fixture: differential golden green, no codegen change
 *   needed).
 *
 * VARIABLES: each `integer` var gets a zero-initialised 4-byte slot in .bss
 * labelled `v_<name>` (names are case-insensitive Pascal idents; we lower-case
 * them for a deterministic, collision-free label, and the front end already
 * rejects redeclarations). A var read is `mov eax, [v_name]`; an assignment is
 * `mov [v_name], eax`.
 *
 * STRING LITERALS: each distinct write/writeln string arg is emitted as a
 * NUL-terminated byte array in .rodata labelled `str_<n>` (n = emission order,
 * deterministic). Bytes are emitted as decimal `db` values so any byte is safe
 * and the source stays ASCII-clean (Rule 12).
 *
 * write/writeln LOWERING: each argument is emitted left to right:
 *   - string arg : lea/mov the label into the arg slot, call serial_puts
 *   - int expr   : eval -> eax, call serial_put_int (eax = signed value)
 *   writeln additionally writes a trailing newline (0x0A) via serial_putc.
 *   Helper ABI (defined in the runtime): arg passed in EAX (cdecl-ish but
 *   register-passed for simplicity); callee preserves nothing we rely on, so
 *   no live values straddle a helper call.
 *
 * Determinism (Rule 11): no timestamps, labels are ordinal, output is a pure
 * function of the AST -> the same source always yields byte-identical asm.
 * ============================================================================
 *
 * B2 (beads initech-80iw; ADR-0007 DEC-02/DEC-04) -- if/while control flow:
 *
 *   if <cond> then S1 [else S2]:
 *     <eval cond -> eax>
 *     test eax, eax
 *     jz .Lelse_N        (or .Lendif_N directly if there is no 'else')
 *     <gen S1>
 *     jmp .Lendif_N      (only emitted when an 'else' exists)
 *     .Lelse_N:
 *     <gen S2>
 *     .Lendif_N:
 *
 *   while <cond> do S:
 *     .Lwhile_top_N:
 *     <eval cond -> eax>
 *     test eax, eax
 *     jz .Lwhile_end_N
 *     <gen S>
 *     jmp .Lwhile_top_N
 *     .Lwhile_end_N:
 *
 * `for`/`repeat` are SUGAR (ADR-0007 DEC-02) and are desugared entirely in
 * seed/parser.c into if/while/assign/block/not nodes -- this file never sees
 * a "for" or "repeat" AST node and needed NO new codegen machinery for them,
 * exactly as the ADR requires.
 *
 * ORDINAL LABELS, ONE THREADED COUNTER (DEC-04, binding): `Cg.lbl_count` is
 * the ONE shared counter for every local ordinal label this file emits --
 * the boolean TRUE/FALSE print labels (B1, `.Lbtrue_N`/`.Lbfalse_N`/
 * `.Lbdone_N`) AND the new if/while labels (B2, `.Lelse_N`/`.Lendif_N`/
 * `.Lwhile_top_N`/`.Lwhile_end_N`) draw from the SAME counter, incremented
 * once per node as gen_stmt/gen_write walks the AST in source order, in the
 * ONE emission pass -- never re-derived by a second walk. This is the exact
 * mechanism DEC-04 names as the anti-pattern-avoider (contrasted with the
 * .rodata string-label scheme just above, which re-walks and re-derives).
 *
 * CONDITIONAL JUMPS OFF THE B1 BOOLEAN (DEC-04's other binding point): every
 * guard (if/while) evaluates its condition exactly like any other boolean
 * expression -- gen_expr leaves 0/1 in eax (B1) -- then `test eax, eax` /
 * `jz` branches off it. There is no separate "condition compilation" path;
 * control flow is a thin, uniform layer on top of the B1 boolean.
 *
 * MUTATION HOOK (Rule 6; beads initech-80iw): SEED_MUT_CODEGEN_BRANCH_INVERT
 * flips every guard's `jz` to `jnz` (see guard_jump_mnemonic below), which
 * inverts if/while decisions (an if's then/else swap; a while that should
 * stop keeps looping and vice versa). test-seed-control-mutant asserts this
 * makes control.pas's exact-serial golden go RED.
 * ============================================================================
 *
 * B3 (beads initech-7mo3; ADR-0007 DEC-02 "const declarations", "char, ord,
 * chr") -- const folding, char, ord/chr:
 *
 *   CONST FOLDING is entirely a FRONT-END concern (seed/parser.c): every
 *   const use is replaced by a fresh literal AST node (AST_INTLIT/
 *   AST_CHARLIT/AST_BOOLLIT) at parse time, so codegen NEVER sees an
 *   AST_CONSTDECL reference and NEVER emits a .bss slot for a const
 *   (emit_bss explicitly skips AST_CONSTDECL nodes) -- "front-end FOLD to
 *   literals at use sites, no runtime storage" per the bead.
 *
 *   char = a BYTE, ZERO-EXTENDED in eax and in its .bss slot (a plain 4-byte
 *   `resd 1`, exactly like integer/boolean -- one uniform slot width for
 *   every scalar in this subset, DECISION: simpler than mixed-width memory
 *   access for a compiler this size, and every char-producing operation
 *   (a char literal, chr()) already keeps the value in 0..255, so no
 *   mid-expression masking is needed anywhere except inside chr() itself).
 *
 *   ord(x) / chr(x): both AST_UNOP (OP_ORD / OP_CHR), both VALUE NO-OPS at
 *   the bit level -- ord() only changes typecheck's static type (char/
 *   boolean/integer are already the same zero-extended eax representation);
 *   chr() emits exactly one instruction, `and eax, 0xFF`, which TRUNCATES an
 *   out-of-range operand to its low byte rather than trapping (Borland
 *   Turbo Pascal 7.0 Language Guide, "Chr": out-of-range under the default
 *   {$R-} truncates; this compiler has no range-check trap mechanism at
 *   all, so truncation is the only sound, deterministic choice -- Rule 11).
 *   write/writeln of a char calls serial_putc directly (the raw byte), never
 *   serial_put_int (that would print the char's decimal ORD) and never the
 *   TRUE/FALSE boolean path.
 *
 *   MUTATION HOOK (Rule 6; beads initech-7mo3): SEED_MUT_CODEGEN_CHR_OFF1
 *   makes chr(i) compute (i & 0xFF) + 1 instead of i & 0xFF (see gen_expr's
 *   OP_CHR case). test-seed-char-mutant asserts this makes char.pas's
 *   exact-serial golden go RED.
 * ============================================================================
 *
 * MUTATION HOOK (Rule 6; bead initech-3yv, ADR-0007 FO-5/DEC-06): SEED_MUT_NONDET
 * injects GENUINE process-to-process nondeterminism into the emitted .rodata
 * (see codegen_emit) -- an unreferenced byte-array symbol whose 4 bytes
 * encode getpid(2) of the compiling initechc process. This is not a
 * hand-flipped constant: two separate invocations of the SAME initechc
 * binary on the IDENTICAL .pas input get DIFFERENT PIDs from the kernel
 * essentially every time (real OS-assigned state, never read back from the
 * emission itself, and NOT reproducible by re-running with the same
 * arguments -- exactly the class of "looks-plausible but isn't reproducible"
 * bug Rule 11 exists to catch, e.g. an accidental build-id/timestamp byte
 * leaking into an emitted artifact). The symbol is NEVER referenced by any
 * jump, load, or write/writeln argument -- it is dead data as far as
 * pas_main's control flow and the runtime ABI are concerned, so program
 * BEHAVIOR (and every other SEED_MUT_* / test-seed-codegen single-run gate)
 * is completely unaffected; only the .s/.o/.elf BYTES (never semantics)
 * become build-to-build nondeterministic. test-seed-repro-mutant asserts
 * this makes the reproducible-build gate (test-seed-repro) go RED.
 * ============================================================================
 */
#include "codegen.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef SEED_MUT_NONDET
#include <sys/types.h>
#include <unistd.h>
#endif

/* ============================================================================
 * B4 (beads initech-63ce; ADR-0007 DEC-02/DEC-04) -- THE CODEGEN PIVOT:
 * procedures/functions, real cdecl stack frames, value + var parameters,
 * recursion, forward/mutual recursion. Before B4 every variable was a static
 * .bss slot and there were no calls; B4 stands up the frame model below.
 *
 * CALLING CONVENTION (DECISION, report): cdecl.
 *   - The caller pushes arguments RIGHT-TO-LEFT, so the FIRST (leftmost)
 *     parameter ends up at the LOWEST address [ebp+8] and parameter i at
 *     [ebp+8+4i] (cg_param_offset). The caller cleans the stack after the
 *     call ("add esp, 4*nargs"). Right-to-left is the conventional cdecl
 *     order and keeps parameter i at the clean offset [ebp+8+4i]; it is a
 *     FIXED order, so codegen output stays a pure function of the AST
 *     (Rule 11 / DEC-04 determinism).
 *   - The result of a function is returned in EAX (consistent with the
 *     existing expression stack machine, where EAX is the working/result
 *     register).
 *   - EBP is the frame pointer and the ONLY callee-saved register this seed
 *     touches (push/pop in the prologue/epilogue). EAX/ECX/EDX are
 *     caller-saved scratch, exactly as the pre-B4 expression code already
 *     assumed; the seed never uses EBX/ESI/EDI in emitted program code, so
 *     there is nothing else to preserve across a call. Because every
 *     intermediate expression value is spilled to the DATA STACK (push eax),
 *     not held in a register, a `call` appearing mid-expression cannot
 *     clobber a live value -- the spilled operands sit safely below the
 *     callee's balanced frame.
 *
 * FRAME LAYOUT (a routine `pf_<name>`):
 *       [ebp + 8 + 4*(n-1)]  last parameter
 *       ...
 *       [ebp + 8]            first parameter
 *       [ebp + 4]            return address
 *       [ebp + 0]            saved ebp            <- ebp
 *       [ebp - 4]            slot 0 (function result, or first local)
 *       [ebp - 8]            slot 1
 *       ...
 *   Prologue: push ebp / mov ebp,esp / sub esp, 4*num_slots.
 *   Epilogue: (functions) mov eax,[result slot] / leave / ret.
 *   Locals + the function result live on the frame ([ebp-4k], cg_local_offset);
 *   GLOBALS (program-level vars) stay in .bss (v_<name>) exactly as before --
 *   only routine params/locals/result are frame-resident.
 *
 * VAR PARAMETERS (the DEEP-BUG locus, Rule 3): a `var` parameter's frame slot
 * holds the ADDRESS of the caller's variable. A read loads the pointer then
 * DEREFERENCES it ("mov eax,[ebp+off] / mov eax,[eax]"); a write loads the
 * pointer then stores THROUGH it ("mov edx,[ebp+off] / mov [edx],eax"). This
 * happens FRESH on EVERY access -- the stack machine never caches a value in
 * a register across statements, so a var parameter aliasing a global is read
 * back correctly every time (no staleness). At a call site, a var-parameter
 * ARGUMENT contributes an ADDRESS (cg_gen_addr_of), not a value.
 *
 * MUTATION HOOKS (Rule 6; ADR-0007 DEC-07's per-family obligation for B4):
 *   SEED_MUT_CODEGEN_FRAME_OFF4  -- shifts every local/result slot 4 bytes
 *       too shallow (cg_local_offset), so slot 0 lands on the saved-ebp word
 *       [ebp+0]; a function writing its result then corrupts its own frame
 *       link and the frame math is wrong for every routine with a
 *       local/result. test-seed-func-mutant asserts func.pas goes RED.
 *   SEED_MUT_CODEGEN_VARPARAM_COPY -- forces every parameter's by-reference
 *       flag to 0 (cg_param_is_var), degrading `var` parameters to value
 *       copies uniformly (call site pushes a value; callee reads/writes the
 *       slot directly with no deref). The aliasing/swap clauses of func.pas
 *       then fail (a global modified "through" the param stays unchanged).
 *       test-seed-func-mutant asserts func.pas goes RED.
 * ============================================================================
 */
#define CG_NAME_CAP    128
#define CG_MAX_PARAMS   32
#define CG_MAX_SCOPE    (CG_MAX_PARAMS + 64) /* params + result + locals */
#define CG_MAX_PROCS   128
/* B5 (beads initech-54uu): the GLOBAL array table (parallel to CgProc's
 * "collected once, consulted throughout emission" shape). Fixed capacity,
 * fail-loud on overflow (Rule 2) -- same discipline as CG_MAX_PROCS. */
#define CG_MAX_GLOBAL_ARRAYS 64
/* B6 (beads initech-rug7): the record-TYPE table (name -> field COUNT only --
 * codegen never needs field NAMES, since typecheck.c already resolved every
 * field access to a 0-based `field_index` on the AST; codegen only needs each
 * record type's SIZE, for frame-slot/.bss sizing and the array-of-record
 * element STRIDE). Same fixed-capacity, fail-loud discipline. */
#define CG_MAX_RECTYPES 32

typedef enum {
    CG_GLOBAL,    /* not resolved in a routine scope -> .bss v_<name> */
    CG_VALPARAM,  /* value parameter: [ebp + offset] holds a copy */
    CG_VARPARAM,  /* var parameter:   [ebp + offset] holds an ADDRESS */
    CG_LOCAL,     /* local variable:  [ebp + offset] (offset < 0) */
    CG_RESULT     /* function result: [ebp + offset] (offset < 0) */
} CgKind;

typedef struct {
    char   name[CG_NAME_CAP]; /* case-folded */
    CgKind kind;
    int    offset;            /* ebp displacement (signed). For an array
                               * local (is_array), the offset of ELEMENT 0
                               * (index `lo`) -- see gen_elem_addr. */
    /* B5 (beads initech-54uu): 1 iff this LOCAL is a static array (params
     * and the result are never arrays, ast.h's B5 DECISION note -- always 0
     * for CG_VALPARAM/CG_VARPARAM/CG_RESULT). lo/hi are its declared
     * inclusive bounds, valid iff is_array. */
    int    is_array;
    long   lo, hi;
    /* B6 (beads initech-rug7): 0 for a scalar entry; the record type's FIELD
     * COUNT for a record-typed entry (a `var` parameter of record type, or a
     * LOCAL that is a scalar record or an array-of-record). When is_array is
     * ALSO set, this is the array's ELEMENT word-count (the stride divisor
     * -- see gen_elem_addr/gen_index_byteoff's "elem_words" parameter);
     * when is_array is 0, this is the scalar record's own size in dwords
     * (used by gen_field_addr/gen_record_copy, never by array addressing). */
    int    rec_fields;
} CgScopeEnt;

typedef struct {
    CgScopeEnt ent[CG_MAX_SCOPE];
    int        n;
    int        frame_slots;   /* result + locals; frame size = 4*frame_slots */
} CgScope;

typedef struct {
    char name[CG_NAME_CAP];   /* case-folded routine name */
    int  nparams;
    int  is_var[CG_MAX_PARAMS];
    int  has_result;
} CgProc;

/* B5 (beads initech-54uu): one GLOBAL array's bounds, keyed by its
 * case-folded name -- consulted by gen_expr/gen_stmt/gen_addr_of (via
 * gen_elem_addr) whenever an indexed reference resolves OUTSIDE the active
 * routine scope (cg_resolve returns NULL), i.e. is a program-level global. */
typedef struct {
    char name[CG_NAME_CAP];
    long lo, hi;
    /* B6 (beads initech-rug7): 0 for a scalar-element array; the record
     * type's field count (the element word-count / stride divisor) for an
     * array-of-record. */
    int  rec_fields;
} CgGlobalArr;

/* B6 (beads initech-rug7): one record TYPE's size, keyed by its case-folded
 * name (see CG_MAX_RECTYPES's comment above). */
typedef struct {
    char name[CG_NAME_CAP];
    int  nfields;
} CgRecordType;

typedef struct {
    FILE *out;
    int   str_count;      /* next .rodata string label ordinal */
    /* B4 (beads initech-63ce): the routine table (for call codegen) and the
     * ACTIVE routine's scope (NULL while emitting pas_main -- there, every
     * name is a global). */
    CgProc         proctab[CG_MAX_PROCS];
    int            nproc;
    const CgScope *scope;
    /* B5 (beads initech-54uu): the global array table (see CgGlobalArr). */
    CgGlobalArr    garr[CG_MAX_GLOBAL_ARRAYS];
    int            ngarr;
    /* B6 (beads initech-rug7): the record-type table (see CgRecordType). */
    CgRecordType   rectypes[CG_MAX_RECTYPES];
    int            nrectypes;
    int   lbl_count;      /* next local-label ordinal -- ONE counter shared by
                           * EVERY local label this file emits: the B1
                           * boolean-print labels (beads initech-f0uc,
                           * `.Lbtrue_N`/`.Lbfalse_N`/`.Lbdone_N`) AND the B2
                           * if/while control-flow labels (beads initech-80iw,
                           * `.Lelse_N`/`.Lendif_N`/`.Lwhile_top_N`/
                           * `.Lwhile_end_N`). Threaded through emission in
                           * source order, never re-derived by a second walk
                           * (ADR-0007 DEC-04's determinism mechanism,
                           * applied here at seed scale even though DEC-04
                           * itself binds the resident compiler, not the
                           * seed). Renamed from `bool_lbl_count` at B2 to
                           * reflect that it is no longer boolean-print-only. */
    /* Collected .rodata string defs are emitted inline as encountered into a
     * deferred buffer is unnecessary: we emit .text and .rodata in separate
     * passes. To keep a single AST walk we instead emit strings to .rodata at
     * the point of use by buffering. For simplicity and determinism we do a
     * first pass to emit .rodata strings, then a second pass for .text. */
} Cg;

/* ---- fatal (Rule 2): internal AST contract violation, never user input ----
 * _Noreturn (C11): lets the compiler prove control never falls through here,
 * silencing "maybe uninitialized" false positives at call sites that declare
 * a local, branch to cg_ice() in the "should never happen" arm, and use the
 * local afterward (B6, beads initech-rug7, e.g. gen_field_base_designator). */
static _Noreturn void cg_ice(const char *what, const AstNode *n)
{
    fprintf(stderr,
            "initechc: codegen ICE: %s (kind=%d at %d:%d)\n",
            what, n ? (int)n->kind : -1, n ? n->line : 0, n ? n->col : 0);
    abort();
}

/* Emit a lower-cased ident as a label-safe suffix. The lexer guarantees idents
 * are [A-Za-z_][A-Za-z0-9_]*, so a straight lower-case is collision-free. */
static void emit_var_label(FILE *out, const char *name)
{
    for (const char *p = name; *p; p++)
        fputc((char)tolower((unsigned char)*p), out);
}

/* ---------------- B4 (beads initech-63ce): frame model helpers ------------ */

/* Case-fold a name into `dst` (NUL-terminated, truncated at cap-1). Mirrors
 * emit_var_label's lowering, but into a buffer for table lookups. */
static void cg_lower(char *dst, size_t cap, const char *src)
{
    size_t i = 0;
    for (; src[i] && i + 1 < cap; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

/* Parameter i (0-based, left-to-right) lives at [ebp + 8 + 4i] (cdecl,
 * right-to-left push -- see the file-header frame model). */
static int cg_param_offset(int i)
{
    return 8 + 4 * i;
}

/* Local/result slot k (0-based; slot 0 is the function result if any, then
 * locals in declaration order) lives at [ebp - 4*(k+1)]. */
static int cg_local_offset(int slot)
{
#ifdef SEED_MUT_CODEGEN_FRAME_OFF4
    /* MUTATION HOOK (Rule 6; beads initech-63ce, ADR-0007 DEC-07 B4 "frame
     * offsets" deep-bug locus): shift every local/result 4 bytes too shallow,
     * so slot 0 lands on the saved-ebp word [ebp+0] and a function writing
     * its result corrupts its own frame link. test-seed-func-mutant asserts
     * func.pas goes RED. */
    return -(4 * slot);
#else
    return -(4 * (slot + 1));
#endif
}

/*
 * B6 (beads initech-rug7; ADR-0007 DEC-02/DEC-04 "records ... deterministic
 * field layout"): field k of a record lives at byte offset cg_field_offset(k)
 * from the record's base address (the ONE choke point every field-address
 * computation in this file routes through -- gen_field_addr, gen_record_copy
 * -- so the SEED_MUT_CODEGEN_FIELD_OFF4 mutation hook perturbs EVERY field
 * access uniformly, mirroring cg_local_offset's own FRAME_OFF4 shape above).
 * Layout rule (binding, ast.h's B6 comment): fields occupy CONSECUTIVE
 * uniform 4-byte slots in DECLARATION ORDER -- field k is at 4*k.
 */
static int cg_field_offset(int field_index)
{
#ifdef SEED_MUT_CODEGEN_FIELD_OFF4
    /* MUTATION HOOK (Rule 6; beads initech-rug7, ADR-0007 DEC-07's
     * per-family mutation obligation for B6 -- the committee's own named
     * deep-bug locus: "every field offset shifted +4"). Compile with
     * -DSEED_MUT_CODEGEN_FIELD_OFF4 to shift EVERY field's byte offset 4
     * bytes too high, so a record's field 0 reads/writes field 1's slot
     * (and its LAST field reads/writes one dword past the record entirely).
     * test-seed-record-mutant asserts record.pas goes RED. */
    return 4 * field_index + 4;
#else
    return 4 * field_index;
#endif
}

/* A parameter's effective by-reference flag. The VARPARAM_COPY mutant forces
 * it to 0 everywhere (scope + call table), degrading `var` to a value copy
 * consistently. */
static int cg_param_is_var(int declared_is_var)
{
#ifdef SEED_MUT_CODEGEN_VARPARAM_COPY
    /* MUTATION HOOK (Rule 6; beads initech-63ce, ADR-0007 DEC-07 B4
     * "var-param aliasing" deep-bug locus). test-seed-func-mutant asserts
     * func.pas goes RED. */
    (void)declared_is_var;
    return 0;
#else
    return declared_is_var;
#endif
}

/* Resolve a name in the ACTIVE routine scope; NULL means "not a param/local/
 * result" -> a program global (v_<name> in .bss). Always NULL when emitting
 * pas_main (cg->scope == NULL). */
static const CgScopeEnt *cg_resolve(const Cg *cg, const char *name)
{
    if (!cg->scope)
        return NULL;
    char lc[CG_NAME_CAP];
    cg_lower(lc, sizeof(lc), name);
    for (int i = 0; i < cg->scope->n; i++)
        if (strcmp(cg->scope->ent[i].name, lc) == 0)
            return &cg->scope->ent[i];
    return NULL;
}

/* Emit an "[ebp<+/->N]" memory operand for a signed ebp displacement. */
static void cg_ebp(FILE *o, int off)
{
    if (off < 0)
        fprintf(o, "[ebp-%d]", -off);
    else
        fprintf(o, "[ebp+%d]", off);
}

/* Find a routine in the call table by name; NULL if absent (never happens
 * for a well-typed program -- typecheck rejects a call to an unknown name). */
static const CgProc *cg_proc_find(const Cg *cg, const char *name)
{
    char lc[CG_NAME_CAP];
    cg_lower(lc, sizeof(lc), name);
    for (int i = 0; i < cg->nproc; i++)
        if (strcmp(cg->proctab[i].name, lc) == 0)
            return &cg->proctab[i];
    return NULL;
}

/* B5 (beads initech-54uu): find a GLOBAL array in the array table by name;
 * NULL if absent (never happens for a well-typed program targeting a name
 * that cg_resolve already reported as "not in the active routine scope" --
 * typecheck guarantees an indexed reference resolves to SOME array). */
static const CgGlobalArr *cg_global_arr_find(const Cg *cg, const char *name)
{
    char lc[CG_NAME_CAP];
    cg_lower(lc, sizeof(lc), name);
    for (int i = 0; i < cg->ngarr; i++)
        if (strcmp(cg->garr[i].name, lc) == 0)
            return &cg->garr[i];
    return NULL;
}

/* B6 (beads initech-rug7): a record type's FIELD COUNT (= its size in
 * dwords, Rule 11's "sizeof(record) = 4 * field-count" layout rule -- see
 * ast.h's B6 comment), by name. Fails loud (should never happen for a
 * well-typed program -- typecheck.c already validated every rectype
 * reference against a real `type` declaration). */
static int cg_rectype_fields(const Cg *cg, const char *name)
{
    char lc[CG_NAME_CAP];
    cg_lower(lc, sizeof(lc), name);
    for (int i = 0; i < cg->nrectypes; i++)
        if (strcmp(cg->rectypes[i].name, lc) == 0)
            return cg->rectypes[i].nfields;
    cg_ice("unknown record type name in codegen (should be caught by "
           "typecheck)", NULL);
}

/* ----- .rodata string pass: assign ordinal labels to each string arg ----- */
/* We walk write/writeln args in source order and emit each string literal as a
 * labelled byte array. To map a string node to its label in the .text pass we
 * re-walk in the SAME order and re-derive the ordinal -- deterministic. */

static void rodata_walk(Cg *cg, const AstNode *n);

static void rodata_emit_string(Cg *cg, const AstNode *s)
{
    FILE *o = cg->out;
    int idx = cg->str_count++;
    fprintf(o, "str_%d:\n", idx);
    fprintf(o, "    db ");
    for (size_t i = 0; i < s->as.strlit.length; i++) {
        fprintf(o, "%u,", (unsigned char)s->as.strlit.text[i]);
    }
    fprintf(o, "0\n");
}

static void rodata_walk(Cg *cg, const AstNode *n)
{
    if (!n)
        return;
    switch (n->kind) {
    case AST_PROGRAM:
        /* B4 (beads initech-63ce): string literals can appear in ROUTINE
         * bodies too, so walk each defining routine's body in source order
         * FIRST, then the main block -- the SAME order the .text pass emits
         * them, so str_<n> ordinals match between the two passes. */
        for (size_t i = 0; i < n->as.program.decls.count; i++) {
            const AstNode *d = n->as.program.decls.items[i];
            if ((d->kind == AST_PROCDECL || d->kind == AST_FUNCDECL)
                && !d->as.procfunc.is_forward)
                rodata_walk(cg, d->as.procfunc.body);
        }
        rodata_walk(cg, n->as.program.block);
        break;
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count; i++)
            rodata_walk(cg, n->as.block.stmts.items[i]);
        break;
    case AST_WRITE:
    case AST_WRITELN:
        for (size_t i = 0; i < n->as.write.args.count; i++) {
            AstNode *arg = n->as.write.args.items[i];
            if (arg->kind == AST_STRLIT)
                rodata_emit_string(cg, arg);
        }
        break;
    case AST_ASSIGN:
        /* assignment value is an integer expr -- no string literals there */
        break;
    /* B2 (beads initech-80iw): if/while bodies can contain write/writeln
     * with string args (control.pas's IF1/IF2/DE tags do exactly this), so
     * this pass MUST recurse into them in the SAME order gen_stmt's .text
     * pass will -- else a string literal inside a branch/loop body would
     * never reach .rodata and codegen would emit a reference to a label
     * that was never defined. Conditions themselves are never walked here:
     * a condition is an expression context and this subset's expressions
     * never contain string literals (write/writeln args are the only place
     * a bare string literal is legal -- see parser.c's parse_write). */
    case AST_IF:
        rodata_walk(cg, n->as.ifstmt.then_stmt);
        rodata_walk(cg, n->as.ifstmt.else_stmt);
        break;
    case AST_WHILE:
        rodata_walk(cg, n->as.whilestmt.body);
        break;
    default:
        break;
    }
}

/* ----------------------------- .text pass ----------------------------- */

static void gen_expr(Cg *cg, const AstNode *e);
/* B4 (beads initech-63ce). */
static void gen_call(Cg *cg, const AstNode *call);
static void gen_addr_of(Cg *cg, const AstNode *arg);
/* B5 (beads initech-54uu). */
static void gen_elem_addr(Cg *cg, const char *name, const AstNode *idxexpr);
/* B6 (beads initech-rug7). */
static void gen_field_base_designator(const AstNode *base, const char **out_name,
                                      const AstNode **out_idx);
static void gen_field_addr(Cg *cg, const char *name, const AstNode *idxexpr,
                          int field_index);
static void gen_record_copy(Cg *cg, const char *dst_name,
                            const AstNode *dst_idx, const char *src_name,
                            const AstNode *src_idx, int rec_fields);

/*
 * B5 (beads initech-54uu; ADR-0007 DEC-02/DEC-04 "static arrays"; codegen:
 * base + (index - lo) * stride) -- element addressing.
 *
 * gen_elem_addr emits the ELEMENT ADDRESS of `name[idxexpr]` into EAX. It is
 * the ONE shared address-computation path for every array use: an r-value
 * read (gen_expr's AST_INDEX case, which then dereferences the address),
 * an l-value store (gen_stmt's AST_ASSIGN case, indexed target), and a
 * `var`-parameter call argument (gen_addr_of, which just needs the address,
 * no dereference) -- so the base+offset arithmetic is written exactly once.
 *
 * STRIDE (DECISION, report -- the bead's own "decide now, record" point):
 * EVERY element occupies a UNIFORM 4-byte slot, regardless of element type
 * (integer, boolean, OR char) -- the identical "one zero-extended dword per
 * scalar" convention this seed has used since B1 (booleans) and B3 (chars,
 * see codegen.c's file-header AST_CHARLIT note: "one uniform slot width for
 * every scalar in this subset"). A char ARRAY therefore does NOT pack one
 * byte per element here; it costs 4x the memory a byte-packed array would.
 * Rationale: (1) ONE stride constant (4) for every element type means one
 * address-computation path with no per-type dispatch -- simpler codegen,
 * and self-host-sufficient (the bead's own oracle framing -- symbol tables,
 * source buffers -- needs correctness, not density, to reach the K2==K3
 * fixed point); (2) it is the ALREADY-established seed convention, so
 * introducing a second (byte) stride HERE would be the actual divergence,
 * not the other way around. FORWARD-LOOKING NOTE (recorded now, per the
 * bead's explicit ask to "think one step ahead" toward B7 strings): B7's
 * fixed/ShortString type will almost certainly want a BYTE-PACKED
 * length-prefixed representation (a ShortString's whole point is compact,
 * Pascal-compatible storage) -- this seed's design intends for B7 to define
 * its OWN dedicated string representation and codegen path (a new AST kind
 * with its own layout), NOT to reuse "array of char" with a byte stride
 * bolted on after the fact. If a future bead instead chooses to model
 * ShortString as a byte-packed "array[0..N] of char" reusing THIS feature,
 * that is a considered amendment (a second, byte-addressed stride variant),
 * not an oversight -- flag it explicitly rather than silently special-
 * casing char here.
 *
 * ADDRESSING FORMULA: element address = base + (index - lo) * stride.
 *   GLOBAL array `name` (a .bss block labelled v_<name>, resd (hi-lo+1)):
 *     base = the label's address (a bare "mov eax, v_name", the same
 *     convention gen_addr_of already uses for a global's address-of).
 *   LOCAL array (frame-resident, cg_build_scope): base = [ebp + se->offset],
 *     where se->offset is the ebp displacement of ELEMENT 0 (index lo).
 *     Because cg_local_offset's slots grow MORE NEGATIVE as the slot number
 *     increases (cg_build_scope allocates element j at slot+j), the
 *     relationship is `offset(element j) = se->offset - 4*j`, so the local
 *     case SUBTRACTS the byte offset from the element-0 address rather than
 *     adding it (see the two branches below) -- this is the "[ebp-...]
 *     offset math" the bead flags as composing with B4's deep-bug locus:
 *     it reuses cg_local_offset's EXACT linear form (including under the
 *     SEED_MUT_CODEGEN_FRAME_OFF4 mutant, which shifts the whole family
 *     uniformly and therefore still corrupts an array local's addressing
 *     exactly as it corrupts a scalar local's -- no special-casing needed
 *     for the mutant to bite here too).
 */
/*
 * B6 (beads initech-rug7; ADR-0007 DEC-02 "arrays of records ... deterministic
 * field layout"): `elem_words` generalizes the B5 hardcoded 4-byte stride to
 * `4 * elem_words` -- 1 for a scalar array (the pre-B6 stride, unchanged), or
 * a record type's FIELD COUNT for an array-of-record (the DEEP-BUG
 * intersection the bead names: B5's "base + (index-lo)*stride" formula
 * composes UNCHANGED with B6's per-record size, just fed a bigger stride --
 * see ast.h's B6 layout-rule comment).
 */
static void gen_index_byteoff(Cg *cg, const AstNode *idxexpr, long lo,
                              int elem_words)
{
    FILE *o = cg->out;
    gen_expr(cg, idxexpr);           /* index value -> eax */
#ifdef SEED_MUT_CODEGEN_ARRAY_LO_SKIP
    /* MUTATION HOOK (Rule 6; beads initech-54uu, ADR-0007 DEC-07's
     * per-family mutation obligation for B5 -- the cheap second leg for the
     * lo-offset half of "base + (index - lo) * stride"). Compile with
     * -DSEED_MUT_CODEGEN_ARRAY_LO_SKIP to OMIT the "- lo" step entirely
     * (every array is addressed as if lo were always 0), so any array whose
     * declared lower bound is NON-ZERO is indexed off-by-`lo` elements --
     * array.pas's a[]/c[]/local_arr[]/letters[]/flags[] all have a non-zero
     * lo (only b's lo=0 is unaffected by this particular leg), so this
     * bites broadly. test-seed-array-mutant asserts array.pas goes RED
     * under this leg too. */
    (void)lo;
#else
    if (lo != 0)
        fprintf(o, "    sub eax, %ld\n", lo);  /* zero-based: (index - lo) */
#endif
#ifdef SEED_MUT_CODEGEN_STRIDE
    /* MUTATION HOOK (Rule 6; beads initech-54uu, ADR-0007 DEC-07's
     * per-family mutation obligation for B5). Compile with
     * -DSEED_MUT_CODEGEN_STRIDE to multiply by 2 instead of the correct
     * uniform 4-byte stride, so EVERY indexed read/write/var-param address
     * lands on the wrong element (or straddles two elements) for any index
     * other than 0. test-seed-array-mutant asserts array.pas goes RED. */
    (void)elem_words;
    fprintf(o, "    imul eax, 2\n");
#elif defined(SEED_MUT_CODEGEN_REC_STRIDE)
    /* MUTATION HOOK (Rule 6; beads initech-rug7, ADR-0007 DEC-07's
     * per-family mutation obligation for B6 -- the bead's own named "array-
     * of-record stride = field-count-1 slots" deep-bug leg). Compile with
     * -DSEED_MUT_CODEGEN_REC_STRIDE to under-count the stride by one FIELD
     * (4*(elem_words-1) instead of 4*elem_words). Only meaningfully wrong
     * for a RECORD element (elem_words > 1) -- record.pas's array-of-record
     * fixture (toks: array[...] of Token, 3 fields) is what this bites;
     * test-seed-record-mutant asserts it goes RED. */
    fprintf(o, "    imul eax, %d\n", 4 * (elem_words - 1));
#else
    fprintf(o, "    imul eax, %d\n", 4 * elem_words); /* stride: uniform
                                                        * 4-byte slot(s) */
#endif
}

/* B6 (beads initech-rug7): `field_words` (0 for a scalar array element, or
 * the record's field count for an array-of-record -- 0 is treated as "1
 * element word", the pre-B6 stride) is resolved HERE, from the local scope
 * / global array table, so every gen_elem_addr caller stays unchanged (its
 * signature is the SAME as before B6). */
static void gen_elem_addr(Cg *cg, const char *name, const AstNode *idxexpr)
{
    FILE *o = cg->out;
    const CgScopeEnt *se = cg_resolve(cg, name);
    if (se) {
        if (!se->is_array)
            cg_ice("indexed access to a non-array local", idxexpr);
        int elem_words = se->rec_fields > 0 ? se->rec_fields : 1;
        gen_index_byteoff(cg, idxexpr, se->lo, elem_words); /* eax = byte offset */
        fprintf(o, "    mov edx, eax\n");         /* edx = byte offset */
        fprintf(o, "    lea eax, ");
        cg_ebp(o, se->offset);                    /* eax = &element[lo] */
        fprintf(o, "\n    sub eax, edx\n");        /* local slots grow down */
    } else {
        const CgGlobalArr *ga = cg_global_arr_find(cg, name);
        if (!ga)
            cg_ice("indexed access to an unknown/non-array global", idxexpr);
        int elem_words = ga->rec_fields > 0 ? ga->rec_fields : 1;
        gen_index_byteoff(cg, idxexpr, ga->lo, elem_words); /* eax = byte offset */
        fprintf(o, "    mov edx, eax\n");         /* edx = byte offset */
        fprintf(o, "    mov eax, v_");
        emit_var_label(o, name);                  /* eax = &v_name[0] */
        fprintf(o, "\n    add eax, edx\n");
    }
}

/*
 * ============================================================================
 * B6 (beads initech-rug7; ADR-0007 DEC-02 "records (record ... end), field
 * access, arrays of records, with a deterministic field layout") -- record
 * field addressing.
 *
 * DETERMINISTIC LAYOUT RULE (Rule 11, ast.h's B6 comment has the full story):
 * fields sit in CONSECUTIVE UNIFORM 4-byte slots in DECLARATION ORDER; field
 * k is at byte offset cg_field_offset(k) = 4*k from the record's BASE
 * address. sizeof(record) = 4 * field-count. An array-of-record ELEMENT uses
 * the record's TOTAL SIZE as its STRIDE (gen_index_byteoff's `elem_words`
 * parameter, above) -- the bead's own "deep-bug intersection" of B5's
 * per-element addressing composing with B6's per-field addressing.
 *
 * gen_designator_base_addr computes the BASE address (= field 0's address)
 * of a plain designator: `name` (a scalar OR record variable) or
 * `name[idxexpr]` (one array element, scalar or record) -- reusing
 * gen_elem_addr for the indexed case (which ALREADY normalizes local-vs-
 * global addressing via its own sub/add split, so the element address it
 * leaves in eax behaves like a plain ascending base from here on: a further
 * field offset is ALWAYS a plain ADD on top of an indexed base, regardless of
 * whether the array itself is local or global).
 *
 * gen_field_addr computes the address of ONE FIELD of `name`/`name[idxexpr]`:
 * the base, then cg_field_offset(field_index) -- ADDED for a global or an
 * indexed base (gen_elem_addr already normalized those to ascending
 * addresses), SUBTRACTED for a bare LOCAL scalar-record designator (frame
 * slots grow to MORE NEGATIVE addresses as the field index increases, the
 * IDENTICAL "[ebp-...] offset math" gen_elem_addr's own LOCAL branch already
 * uses for array elements -- see cg_build_scope's record-local slot
 * allocation). A `var` parameter of record type is NOT "local" in this sense
 * -- its frame slot holds a POINTER whose VALUE is the record's own
 * (ascending) base address, so field access through it ADDS, exactly like a
 * global.
 *
 * gen_record_copy implements WHOLE-RECORD assignment (`r1 := r2` or
 * `arr[i] := r`) as a fully UNROLLED, compile-time member-wise word copy --
 * field count is always a compile-time constant here, so no runtime loop or
 * extra ordinal label is needed (deterministic, Rule 11; DECISION, report:
 * simpler and just as fast as a labelled loop for the small field counts
 * this subset's own records have).
 * ============================================================================
 */

/* Unpack a field-access BASE (always AST_VARREF or AST_INDEX -- fields are
 * scalar-only, so a base is never itself an AST_FIELD, ast.h's B6 note) into
 * a (name, optional index expression) pair -- the SAME shape AST_ASSIGN's own
 * `name`/`index` members already use, so gen_field_addr/gen_record_copy have
 * ONE designator representation regardless of whether it came from an
 * AST_FIELD's base or an AST_ASSIGN's target/value. */
static void gen_field_base_designator(const AstNode *base, const char **out_name,
                                      const AstNode **out_idx)
{
    if (base->kind == AST_VARREF) {
        *out_name = base->as.varref.name;
        *out_idx = NULL;
        return;
    }
    if (base->kind == AST_INDEX) {
        *out_name = base->as.arrayindex.name;
        *out_idx = base->as.arrayindex.index;
        return;
    }
    cg_ice("field access base is not a variable or array element", base);
}

/* Address of the designator `name` (idxexpr == NULL) or `name[idxexpr]`
 * (idxexpr non-NULL) -- field 0 / the record's own base address. Shared by
 * gen_field_addr and gen_record_copy. */
static void gen_designator_base_addr(Cg *cg, const char *name,
                                     const AstNode *idxexpr)
{
    if (idxexpr) {
        gen_elem_addr(cg, name, idxexpr);
        return;
    }
    FILE *o = cg->out;
    const CgScopeEnt *se = cg_resolve(cg, name);
    if (se) {
        if (se->kind == CG_VARPARAM) {
            /* The frame slot holds the caller's record's address already --
             * forward it unchanged (do NOT take its own address). */
            fprintf(o, "    mov eax, ");
            cg_ebp(o, se->offset);
            fprintf(o, "\n");
        } else {
            fprintf(o, "    lea eax, ");
            cg_ebp(o, se->offset);
            fprintf(o, "\n");
        }
    } else {
        fprintf(o, "    mov eax, v_");
        emit_var_label(o, name);
        fprintf(o, "\n");
    }
}

/* Is `name` (with NO index -- idxexpr NULL) a LOCAL frame-resident designator
 * whose fields SUBTRACT the field offset (frame slots grow down), as opposed
 * to a global label or a `var`-parameter's forwarded (ascending) pointer,
 * which ADD it? An INDEXED base is never "local" in this sense -- gen_elem_addr
 * already normalized it to an ascending address. */
static int gen_designator_is_local_frame(Cg *cg, const char *name,
                                         const AstNode *idxexpr)
{
    if (idxexpr)
        return 0;
    const CgScopeEnt *se = cg_resolve(cg, name);
    return se && se->kind != CG_VARPARAM;
}

/* Address of field `field_index` of designator `name`/`name[idxexpr]` -> eax. */
static void gen_field_addr(Cg *cg, const char *name, const AstNode *idxexpr,
                          int field_index)
{
    int local = gen_designator_is_local_frame(cg, name, idxexpr);
    gen_designator_base_addr(cg, name, idxexpr);
    int off = cg_field_offset(field_index);
    if (off == 0)
        return;
    if (local)
        fprintf(cg->out, "    sub eax, %d\n", off);
    else
        fprintf(cg->out, "    add eax, %d\n", off);
}

/* WHOLE-RECORD assignment: copy `rec_fields` fields, one at a time, from
 * src_name[src_idx?] to dst_name[dst_idx?]. Fully unrolled (field count is a
 * compile-time constant) -- deterministic, no runtime loop/label. Each
 * field's address is recomputed fresh for both sides (mirrors this file's
 * existing "never cache an address across a sub-computation that might
 * clobber eax/ecx/edx" discipline, e.g. gen_stmt's indexed-assignment spill)
 * rather than hoisting the base once, keeping the code simple and safe under
 * the FIELD_OFF4 mutant (which must perturb EVERY field, including field 0,
 * uniformly -- see cg_field_offset). */
static void gen_record_copy(Cg *cg, const char *dst_name,
                            const AstNode *dst_idx, const char *src_name,
                            const AstNode *src_idx, int rec_fields)
{
    FILE *o = cg->out;
    for (int k = 0; k < rec_fields; k++) {
        gen_field_addr(cg, src_name, src_idx, k);   /* &src.field[k] -> eax */
        fprintf(o, "    mov eax, [eax]\n");         /* load value -> eax */
        fprintf(o, "    push eax\n");               /* spill value */
        gen_field_addr(cg, dst_name, dst_idx, k);    /* &dst.field[k] -> eax */
        fprintf(o, "    pop ecx\n");
        fprintf(o, "    mov [eax], ecx\n");
    }
}

static void gen_binop(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    gen_expr(cg, e->as.binop.lhs);     /* lhs -> eax */
    fprintf(o, "    push eax\n");       /* spill lhs */
    gen_expr(cg, e->as.binop.rhs);     /* rhs -> eax */
    fprintf(o, "    mov ecx, eax\n");   /* rhs -> ecx */
    fprintf(o, "    pop eax\n");        /* lhs -> eax */
    switch (e->as.binop.op) {
    case OP_ADD:
        fprintf(o, "    add eax, ecx\n");
        break;
    case OP_SUB:
        fprintf(o, "    sub eax, ecx\n");
        break;
    case OP_MUL:
#ifdef SEED_MUT_CODEGEN_MUL_AS_ADD
        /* MUTATION HOOK (Rule 6; beads initech-tf3c, restoring the
         * initech-znb one-off manual "imul->add" perturbation as a
         * repeatable gate). Compile with -DSEED_MUT_CODEGEN_MUL_AS_ADD to
         * emit `add` where `*` should emit `imul`: `2 + 3 * 4` then computes
         * 2 + (3 + 4) = 9 instead of R=14, and `(2 + 3) * 4` computes
         * (2+3)+4 = 9 instead of R=20. test-seed-codegen-mutant asserts this
         * makes the arith corpus's exact-serial expectation go RED. */
        fprintf(o, "    add eax, ecx\n");
#else
        fprintf(o, "    imul eax, ecx\n");
#endif
        break;
    case OP_DIV:
        fprintf(o, "    cdq\n");          /* sign-extend eax into edx:eax */
        fprintf(o, "    idiv ecx\n");     /* quotient -> eax */
        break;
    case OP_MOD:
        fprintf(o, "    cdq\n");
        fprintf(o, "    idiv ecx\n");     /* remainder -> edx */
        fprintf(o, "    mov eax, edx\n");
        break;
    case OP_EQ:
        fprintf(o, "    cmp eax, ecx\n");
        fprintf(o, "    sete al\n");
        fprintf(o, "    movzx eax, al\n");
        break;
    case OP_NE:
        fprintf(o, "    cmp eax, ecx\n");
        fprintf(o, "    setne al\n");
        fprintf(o, "    movzx eax, al\n");
        break;
    case OP_LT:
        fprintf(o, "    cmp eax, ecx\n");
#ifdef SEED_MUT_CODEGEN_SETL_AS_SETG
        /* MUTATION HOOK (Rule 6; beads initech-f0uc, ADR-0007 DEC-03: "a
         * Rule-6 mutant that flips setl/setg in the relational codegen").
         * Compile with -DSEED_MUT_CODEGEN_SETL_AS_SETG to swap setl<->setg:
         * '<' wrongly emits setg and '>' wrongly emits setl, so e.g. "3 < 5"
         * evaluates to FALSE instead of TRUE. test-seed-bool-mutant asserts
         * this makes bool.pas's exact-serial golden go RED. */
        fprintf(o, "    setg al\n");
#else
        fprintf(o, "    setl al\n");
#endif
        fprintf(o, "    movzx eax, al\n");
        break;
    case OP_LE:
        fprintf(o, "    cmp eax, ecx\n");
        fprintf(o, "    setle al\n");
        fprintf(o, "    movzx eax, al\n");
        break;
    case OP_GT:
        fprintf(o, "    cmp eax, ecx\n");
#ifdef SEED_MUT_CODEGEN_SETL_AS_SETG
        fprintf(o, "    setl al\n");   /* mutant: see the OP_LT case above */
#else
        fprintf(o, "    setg al\n");
#endif
        fprintf(o, "    movzx eax, al\n");
        break;
    case OP_GE:
        fprintf(o, "    cmp eax, ecx\n");
        fprintf(o, "    setge al\n");
        fprintf(o, "    movzx eax, al\n");
        break;
    case OP_AND:
        /* Bitwise AND on two already-0/1 values -- both eax (lhs) and ecx
         * (rhs) were unconditionally evaluated above (COMPLETE evaluation,
         * ADR-0007 DEC-03; see the file-header comment). */
        fprintf(o, "    and eax, ecx\n");
        break;
    case OP_OR:
        fprintf(o, "    or eax, ecx\n");
        break;
    case OP_NEG:
        cg_ice("OP_NEG in binop position", e);
        break;
    case OP_NOT:
        cg_ice("OP_NOT in binop position", e);
        break;
    case OP_ORD:
        cg_ice("OP_ORD in binop position", e);
        break;
    case OP_CHR:
        cg_ice("OP_CHR in binop position", e);
        break;
    default:
        cg_ice("unknown binop", e);
        break;
    }
}

static void gen_expr(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    switch (e->kind) {
    case AST_INTLIT:
        /* long literal -> 32-bit immediate. The lexer/parser keep these in
         * range for the subset; emit as a decimal immediate. */
        fprintf(o, "    mov eax, %ld\n", e->as.intlit.value);
        break;
    case AST_BOOLLIT:
        /* B1 (beads initech-f0uc): true/false -> the 0/1 encoding every
         * relational/and/or/not result also uses. */
        fprintf(o, "    mov eax, %d\n", e->as.boollit.value ? 1 : 0);
        break;
    case AST_CHARLIT:
        /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "char"). DECISION: a char
         * is a BYTE, represented ZERO-EXTENDED in the 32-bit eax register
         * (and, for a char variable, in a full 4-byte .bss slot -- see
         * emit_bss) -- the same uniform stack-machine convention every
         * other scalar in this subset already uses (integer, boolean), so
         * there is no mixed-width memory access or partial-register-stall
         * special-casing anywhere in codegen. The literal's value is
         * already guaranteed 0..255 by the lexer/parser (a char literal is
         * one raw source byte), so no masking is needed here. */
        fprintf(o, "    mov eax, %d\n", e->as.charlit.value);
        break;
    case AST_VARREF: {
        /* B4 (beads initech-63ce): resolve in the active routine scope. A
         * global (or any name while emitting pas_main) reads its .bss slot; a
         * param/local/result reads its frame slot; a var parameter loads the
         * pointer and DEREFERENCES it (fresh every time -- no staleness). */
        const CgScopeEnt *se = cg_resolve(cg, e->as.varref.name);
        if (!se) {
            fprintf(o, "    mov eax, [v_");
            emit_var_label(o, e->as.varref.name);
            fprintf(o, "]\n");
        } else if (se->kind == CG_VARPARAM) {
            fprintf(o, "    mov eax, ");
            cg_ebp(o, se->offset);
            fprintf(o, "\n    mov eax, [eax]\n");
        } else {
            fprintf(o, "    mov eax, ");
            cg_ebp(o, se->offset);
            fprintf(o, "\n");
        }
        break;
    }
    case AST_CALL:
        /* B4: a function call used as an expression value (result -> eax). */
        gen_call(cg, e);
        break;
    /* B5 (beads initech-54uu): `name[index]` as an r-value -- compute the
     * element ADDRESS (gen_elem_addr, the ONE shared address path) then
     * dereference it. */
    case AST_INDEX:
        gen_elem_addr(cg, e->as.arrayindex.name, e->as.arrayindex.index);
        fprintf(o, "    mov eax, [eax]\n");
        break;
    /* B6 (beads initech-rug7): `base.field` as an r-value -- compute the
     * FIELD address (gen_field_addr, the ONE shared field-address path) then
     * dereference it, mirroring AST_INDEX just above. */
    case AST_FIELD: {
        const char *fname;
        const AstNode *fidx;
        gen_field_base_designator(e->as.field.base, &fname, &fidx);
        gen_field_addr(cg, fname, fidx, e->as.field.field_index);
        fprintf(o, "    mov eax, [eax]\n");
        break;
    }
    case AST_BINOP:
        gen_binop(cg, e);
        break;
    case AST_UNOP:
        gen_expr(cg, e->as.unop.operand);
        switch (e->as.unop.op) {
        case OP_NEG:
            fprintf(o, "    neg eax\n");
            break;
        case OP_NOT:
            /* B1: not a branch -- xor with 1 flips a 0/1 value in place,
             * consistent with the complete-evaluation / no-jump discipline
             * documented at the top of this file. */
            fprintf(o, "    xor eax, 1\n");
            break;
        /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "ord, chr"). */
        case OP_ORD:
            /* VALUE NO-OP (report per the bead): char, boolean, and integer
             * already share ONE zero-extended representation in eax (B1's
             * boolean 0/1; B3's char is likewise zero-extended, never
             * sign-extended -- see AST_CHARLIT above and the .bss slot note
             * in emit_bss). ord() therefore changes only the STATIC type
             * the rest of codegen/typecheck reasons about; it emits no
             * instruction of its own. (A "sign-extend the byte" mutant --
             * ORD_SIGNEXT, cited as an alternative Rule-6 hook in the bead
             * -- would be independently observable exactly because this
             * path emits nothing today: any instruction added here that
             * treated the byte as signed would be a real, detectable
             * divergence for a char whose value is >= 128.) */
            break;
        case OP_CHR:
            /* DECISION (report): chr(i) TRUNCATES the operand to 8 bits
             * when i is outside 0..255, rather than trapping. Borland
             * Turbo Pascal 7.0 Language Guide, "Chr": "If X is not in the
             * range 0..255, a runtime error occurs if range checking is on
             * ({$R+}); otherwise the low byte of X is used" -- and {$R-}
             * (range checking OFF) is Turbo Pascal's DEFAULT compiler
             * state. This subset implements no runtime range-check
             * machinery at all (there is no trap/exception mechanism in
             * this compiler yet), so truncation is not just the default
             * TP behaviour but the only sound, deterministic choice
             * available (Rule 11 -- a pure function of the input bits,
             * no runtime decision needed). */
            fprintf(o, "    and eax, 0xFF\n");
#ifdef SEED_MUT_CODEGEN_CHR_OFF1
            /* MUTATION HOOK (Rule 6; beads initech-7mo3, ADR-0007 DEC-07's
             * per-family mutation obligation for B3). Compile with
             * -DSEED_MUT_CODEGEN_CHR_OFF1 to make chr(i) compute
             * (i & 0xFF) + 1 instead of i & 0xFF -- e.g. chr(65) wrongly
             * yields 'B' instead of 'A'. test-seed-char-mutant asserts this
             * makes char.pas's exact-serial golden go RED. */
            fprintf(o, "    add eax, 1\n");
#endif
            break;
        default:
            cg_ice("unknown unop", e);
            break;
        }
        break;
    default:
        cg_ice("non-expression node in expression context", e);
        break;
    }
}

/* re-derive a string arg's label ordinal by counting strings emitted before it
 * in the SAME source-order walk used by the rodata pass. */
static void gen_write(Cg *cg, const AstNode *n, int *str_idx)
{
    FILE *o = cg->out;
    for (size_t i = 0; i < n->as.write.args.count; i++) {
        AstNode *arg = n->as.write.args.items[i];
        if (arg->kind == AST_STRLIT) {
            fprintf(o, "    mov eax, str_%d\n", (*str_idx)++);
            fprintf(o, "    call serial_puts\n");
        } else if (arg->type == AST_TY_BOOLEAN) {
            /* B1 (beads initech-f0uc): Turbo Pascal's writeln prints a
             * boolean as "TRUE"/"FALSE" (see the file-header / bool.pas
             * citation for the DECISION). arg->type is populated by
             * seed/typecheck.c, which the driver runs before codegen_emit
             * -- see codegen.h's updated contract note. */
            gen_expr(cg, arg);                 /* value -> eax (0 or 1) */
            int lbl = cg->lbl_count++;
            fprintf(o, "    test eax, eax\n");
            fprintf(o, "    jz .Lbfalse_%d\n", lbl);
            fprintf(o, "    mov eax, str_bool_true\n");
            fprintf(o, "    jmp .Lbdone_%d\n", lbl);
            fprintf(o, ".Lbfalse_%d:\n", lbl);
            fprintf(o, "    mov eax, str_bool_false\n");
            fprintf(o, ".Lbdone_%d:\n", lbl);
            fprintf(o, "    call serial_puts\n");
        } else if (arg->type == AST_TY_CHAR) {
            /* B3 (beads initech-7mo3; ADR-0007 DEC-02 "char"). write/
             * writeln of a char emits the RAW BYTE via serial_putc -- never
             * decimal formatting (that would be printing the char's ORD,
             * which is a different, explicit operation) and never the
             * TRUE/FALSE boolean path above. arg's value is already
             * zero-extended 0..255 in eax (see AST_CHARLIT / OP_CHR), so AL
             * already holds the exact byte serial_putc's ABI expects
             * ("In: AL = byte", seed/rt/start.asm). */
            gen_expr(cg, arg);                 /* value -> eax (al = byte) */
            fprintf(o, "    call serial_putc\n");
        } else {
            gen_expr(cg, arg);                 /* value -> eax */
            fprintf(o, "    call serial_put_int\n");
        }
    }
    if (n->as.write.is_newline) {
        fprintf(o, "    mov al, 10\n");          /* '\n' */
        fprintf(o, "    call serial_putc\n");
    }
}

/*
 * SEED_MUT_CODEGEN_BRANCH_INVERT (Rule 6; beads initech-80iw, ADR-0007
 * DEC-07's per-family mutation obligation). Compile with
 * -DSEED_MUT_CODEGEN_BRANCH_INVERT to flip every if/while guard's `jz` to
 * `jnz`: an if's then/else branches swap, and a while loop that should stop
 * keeps looping (and one that should keep looping stops immediately).
 * test-seed-control-mutant asserts this makes control.pas's exact-serial
 * golden go RED -- mirrors SEED_MUT_CODEGEN_SETL_AS_SETG's shape in
 * gen_binop above, scoped to control-flow guards instead of relational ops.
 */
static const char *guard_jump_mnemonic(void)
{
#ifdef SEED_MUT_CODEGEN_BRANCH_INVERT
    return "jnz";
#else
    return "jz";
#endif
}

static void gen_stmt(Cg *cg, const AstNode *n, int *str_idx)
{
    FILE *o = cg->out;
    switch (n->kind) {
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count; i++)
            gen_stmt(cg, n->as.block.stmts.items[i], str_idx);
        break;
    case AST_ASSIGN: {
        /* B6 (beads initech-rug7): a FIELD assignment -- `name.field := v`
         * or `name[index].field := v` -- computes the FIELD ADDRESS first
         * and spills it, exactly like the B5 indexed-scalar case just below
         * (a `call` inside the value or index expression is free to clobber
         * eax/ecx/edx, cdecl caller-saved). Checked FIRST since `field` and
         * `index` may BOTH be set. */
        if (n->as.assign.field) {
            gen_field_addr(cg, n->as.assign.name, n->as.assign.index,
                          n->as.assign.field_index);
            fprintf(o, "    push eax\n");        /* spill &field */
            gen_expr(cg, n->as.assign.value);    /* value -> eax */
            fprintf(o, "    pop edx\n");          /* edx = &field */
            fprintf(o, "    mov [edx], eax\n");
            break;
        }
        /* B6: a WHOLE-RECORD assignment -- `r1 := r2` (index NULL) or
         * `arr[i] := r` (index non-NULL, an array-of-record element) --
         * rec_fields > 0 (set by typecheck.c) marks it. The VALUE side is
         * always a plain designator too (AST_VARREF or AST_INDEX -- no
         * record-valued general expressions exist in this subset), unpacked
         * the same way an AST_FIELD's base is. */
        if (n->as.assign.rec_fields > 0) {
            const char *src_name;
            const AstNode *src_idx;
            gen_field_base_designator(n->as.assign.value, &src_name, &src_idx);
            gen_record_copy(cg, n->as.assign.name, n->as.assign.index,
                            src_name, src_idx, n->as.assign.rec_fields);
            break;
        }
        /* B5 (beads initech-54uu): an INDEXED target (`name[index] :=
         * value`) computes the ELEMENT ADDRESS FIRST and spills it to the
         * STACK (push) before evaluating the value expression -- the value
         * (or the index expression itself, inside gen_elem_addr) may contain
         * a call, which is free to clobber eax/ecx/edx (cdecl, caller-saved)
         * and would corrupt an address left in any of those registers. This
         * mirrors gen_binop's own lhs-spill discipline (push eax before
         * evaluating the rhs) applied to an address instead of a value. */
        if (n->as.assign.index) {
            gen_elem_addr(cg, n->as.assign.name, n->as.assign.index);
            fprintf(o, "    push eax\n");        /* spill &element */
            gen_expr(cg, n->as.assign.value);    /* value -> eax */
            fprintf(o, "    pop edx\n");          /* edx = &element */
            fprintf(o, "    mov [edx], eax\n");
            break;
        }
        /* B4 (beads initech-63ce): resolve the target in the active routine
         * scope. A global (or any name in pas_main) stores to its .bss slot;
         * a value param/local/result stores to its frame slot; a var
         * parameter stores THROUGH its pointer (loaded fresh -- no
         * staleness). The value is evaluated first (into eax); for the var-
         * parameter case edx then holds the pointer (edx is free after the
         * expression completes). */
        const CgScopeEnt *se = cg_resolve(cg, n->as.assign.name);
        gen_expr(cg, n->as.assign.value);       /* value -> eax */
        if (!se) {
            fprintf(o, "    mov [v_");
            emit_var_label(o, n->as.assign.name);
            fprintf(o, "], eax\n");
        } else if (se->kind == CG_VARPARAM) {
            fprintf(o, "    mov edx, ");
            cg_ebp(o, se->offset);
            fprintf(o, "\n    mov [edx], eax\n");
        } else {
            fprintf(o, "    mov ");
            cg_ebp(o, se->offset);
            fprintf(o, ", eax\n");
        }
        break;
    }
    case AST_CALL:
        /* B4: a procedure call used as a statement (result, if any, ignored
         * -- typecheck already forbids discarding a function result). */
        gen_call(cg, n);
        break;
    case AST_WRITE:
    case AST_WRITELN:
        gen_write(cg, n, str_idx);
        break;
    /* B2 (beads initech-80iw; ADR-0007 DEC-02/DEC-04). ONE shared ordinal
     * counter (cg->lbl_count) picks each node's label suffix here, in this
     * single emission walk -- see the file-header comment. Guards branch
     * off the B1 boolean the same way gen_write's boolean-print path does:
     * gen_expr leaves 0/1 in eax, then `test eax, eax` / a conditional jump
     * (guard_jump_mnemonic -- jz normally, jnz under the Rule-6 mutant). */
    case AST_IF: {
        int lbl = cg->lbl_count++;
        gen_expr(cg, n->as.ifstmt.cond);          /* cond -> eax */
        fprintf(o, "    test eax, eax\n");
        if (n->as.ifstmt.else_stmt) {
            fprintf(o, "    %s .Lelse_%d\n", guard_jump_mnemonic(), lbl);
            if (n->as.ifstmt.then_stmt)
                gen_stmt(cg, n->as.ifstmt.then_stmt, str_idx);
            fprintf(o, "    jmp .Lendif_%d\n", lbl);
            fprintf(o, ".Lelse_%d:\n", lbl);
            gen_stmt(cg, n->as.ifstmt.else_stmt, str_idx);
            fprintf(o, ".Lendif_%d:\n", lbl);
        } else {
            fprintf(o, "    %s .Lendif_%d\n", guard_jump_mnemonic(), lbl);
            if (n->as.ifstmt.then_stmt)
                gen_stmt(cg, n->as.ifstmt.then_stmt, str_idx);
            fprintf(o, ".Lendif_%d:\n", lbl);
        }
        break;
    }
    case AST_WHILE: {
        int lbl = cg->lbl_count++;
        fprintf(o, ".Lwhile_top_%d:\n", lbl);
        gen_expr(cg, n->as.whilestmt.cond);        /* cond -> eax */
        fprintf(o, "    test eax, eax\n");
        fprintf(o, "    %s .Lwhile_end_%d\n", guard_jump_mnemonic(), lbl);
        if (n->as.whilestmt.body)
            gen_stmt(cg, n->as.whilestmt.body, str_idx);
        fprintf(o, "    jmp .Lwhile_top_%d\n", lbl);
        fprintf(o, ".Lwhile_end_%d:\n", lbl);
        break;
    }
    default:
        cg_ice("unexpected statement node", n);
        break;
    }
}

/* ---------------- B4 (beads initech-63ce): calls + routine emission ------- */

/* Emit the ADDRESS of a variable (or, B5, an array ELEMENT; or, B6, a whole
 * RECORD or one FIELD) argument into eax (for a `var` parameter). A
 * plain-variable argument is an AST_VARREF (typecheck guaranteed it is a
 * plain variable, scalar OR record): a global -> its label address; a value
 * param/local/result -> lea of its frame slot; a var parameter -> its stored
 * pointer forwarded unchanged (gen_designator_base_addr, shared with the B6
 * field-addressing helpers above -- SAME address computation, no
 * duplication). B5 (beads initech-54uu): an AST_INDEX argument (`a[i]`,
 * typecheck guaranteed `a` is a real array) passes THAT ELEMENT's address --
 * gen_elem_addr is the exact same address computation an indexed l-value/
 * r-value uses, reused here. B6 (beads initech-rug7): an AST_FIELD argument
 * (`r.f`/`arr[i].f`) passes that FIELD's address -- gen_field_addr, the same
 * path an indexed/plain field l-value uses. */
static void gen_addr_of(Cg *cg, const AstNode *arg)
{
    if (arg->kind == AST_INDEX) {
        gen_elem_addr(cg, arg->as.arrayindex.name, arg->as.arrayindex.index);
        return;
    }
    if (arg->kind == AST_FIELD) {
        const char *fname;
        const AstNode *fidx;
        gen_field_base_designator(arg->as.field.base, &fname, &fidx);
        gen_field_addr(cg, fname, fidx, arg->as.field.field_index);
        return;
    }
    if (arg->kind != AST_VARREF)
        cg_ice("var-parameter argument is not a variable", arg);
    gen_designator_base_addr(cg, arg->as.varref.name, NULL);
}

/* Emit a call. Arguments are pushed RIGHT-TO-LEFT (cdecl); a value parameter
 * contributes its evaluated value, a var parameter contributes an address.
 * The caller cleans the stack (add esp, 4*nargs). The result (functions) is
 * left in eax. */
static void gen_call(Cg *cg, const AstNode *call)
{
    FILE *o = cg->out;
    const CgProc *pr = cg_proc_find(cg, call->as.call.name);
    if (!pr)
        cg_ice("call to unknown routine (should be caught by typecheck)", call);
    int n = (int)call->as.call.args.count;
    if (n != pr->nparams)
        cg_ice("call arity mismatch (should be caught by typecheck)", call);

    for (int i = n - 1; i >= 0; i--) {
        const AstNode *arg = call->as.call.args.items[i];
        if (pr->is_var[i])
            gen_addr_of(cg, arg);       /* address -> eax */
        else
            gen_expr(cg, arg);          /* value   -> eax */
        fprintf(o, "    push eax\n");
    }
    fprintf(o, "    call pf_");
    emit_var_label(o, call->as.call.name);
    fprintf(o, "\n");
    if (n > 0)
        fprintf(o, "    add esp, %d\n", 4 * n);
}

/* Count a routine's frame slots (result + local variables) and build its
 * scope table (params first, at +offsets; then result + locals, at
 * -offsets).
 *
 * B5 (beads initech-54uu): a LOCAL ARRAY of N = hi-lo+1 elements occupies N
 * CONTIGUOUS frame slots (the "extend the frame-slot allocator" the bead
 * names) -- its scope entry's `offset` is cg_local_offset(slot) for the
 * FIRST slot (element index `lo`); element j (0-based from lo) then lives at
 * cg_local_offset(slot + j) = offset - 4*j (the [ebp-...] offset math this
 * composes with: cg_local_offset's linear form, INCLUDING under the
 * FRAME_OFF4 mutant, keeps that relationship exact -- see gen_elem_addr).
 * `slot` advances by N instead of 1 for an array local.
 *
 * B6 (beads initech-rug7): a LOCAL RECORD (scalar, `rec_fields` slots) or a
 * LOCAL ARRAY-OF-RECORD (`rec_fields * (hi-lo+1)` slots -- the per-element
 * word count is `rec_fields`, composing directly with the array-of-N formula
 * above) likewise occupies CONTIGUOUS frame slots; `e->rec_fields` records
 * the field count (0 for a scalar, non-record entry) so gen_elem_addr's
 * array-of-record stride and gen_field_addr's field offset can both resolve
 * it later purely from the scope table, with no second AST walk. A `var`
 * parameter of record type needs NO extra frame slots (its ONE slot holds a
 * pointer, exactly like a scalar var parameter) -- only `rec_fields` is set,
 * for gen_field_addr's benefit (see `cg` now being passed in, needed to
 * resolve a record TYPE NAME to its field count via cg_rectype_fields). */
static void cg_build_scope(const Cg *cg, CgScope *sc, const AstNode *pf)
{
    sc->n = 0;
    sc->frame_slots = 0;

    for (size_t i = 0; i < pf->as.procfunc.params.count; i++) {
        const AstNode *pn = pf->as.procfunc.params.items[i];
        if (sc->n >= CG_MAX_SCOPE)
            cg_ice("routine scope overflow (params)", pn);
        CgScopeEnt *e = &sc->ent[sc->n++];
        cg_lower(e->name, sizeof(e->name), pn->as.param.name);
        e->kind = cg_param_is_var(pn->as.param.is_var) ? CG_VARPARAM
                                                        : CG_VALPARAM;
        e->offset = cg_param_offset((int)i);
        e->is_array = 0; /* array parameters are out of scope (ast.h B5) */
        e->lo = e->hi = 0;
        e->rec_fields = pn->as.param.rectype
                      ? cg_rectype_fields(cg, pn->as.param.rectype) : 0;
    }

    int slot = 0;
    if (pf->as.procfunc.has_result) {
        if (sc->n >= CG_MAX_SCOPE)
            cg_ice("routine scope overflow (result)", pf);
        CgScopeEnt *e = &sc->ent[sc->n++];
        cg_lower(e->name, sizeof(e->name), pf->as.procfunc.name);
        e->kind = CG_RESULT;
        e->offset = cg_local_offset(slot++);
        e->is_array = 0; /* a function result is always scalar */
        e->lo = e->hi = 0;
        e->rec_fields = 0; /* record function results are out of scope */
    }
    for (size_t i = 0; i < pf->as.procfunc.decls.count; i++) {
        const AstNode *vd = pf->as.procfunc.decls.items[i];
        if (vd->kind != AST_VARDECL)
            cg_ice("non-vardecl in routine locals", vd);
        int rec_fields = vd->as.vardecl.rectype
                        ? cg_rectype_fields(cg, vd->as.vardecl.rectype) : 0;
        int elem_words = rec_fields > 0 ? rec_fields : 1;
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (sc->n >= CG_MAX_SCOPE)
                cg_ice("routine scope overflow (locals)", vr);
            CgScopeEnt *e = &sc->ent[sc->n++];
            cg_lower(e->name, sizeof(e->name), vr->as.varref.name);
            e->kind = CG_LOCAL;
            e->rec_fields = rec_fields;
            if (vd->as.vardecl.is_array) {
                long lo = vd->as.vardecl.lo, hi = vd->as.vardecl.hi;
                long count = hi - lo + 1;
                e->is_array = 1;
                e->lo = lo;
                e->hi = hi;
                e->offset = cg_local_offset(slot); /* element 0 (index lo) */
                slot += (int)count * elem_words;
            } else {
                e->is_array = 0;
                e->lo = e->hi = 0;
                e->offset = cg_local_offset(slot); /* field 0 (scalar: itself) */
                slot += elem_words;
            }
        }
    }
    sc->frame_slots = slot;
}

/* Emit one procedure/function as `pf_<name>` with a cdecl prologue/epilogue.
 * A forward declaration has no body and emits nothing. */
static void emit_proc(Cg *cg, const AstNode *pf, int *str_idx)
{
    FILE *o = cg->out;
    if (pf->as.procfunc.is_forward)
        return;

    CgScope sc;
    cg_build_scope(cg, &sc, pf);

    fprintf(o, "pf_");
    emit_var_label(o, pf->as.procfunc.name);
    fprintf(o, ":\n");
    fprintf(o, "    push ebp\n");
    fprintf(o, "    mov ebp, esp\n");
    if (sc.frame_slots > 0)
        fprintf(o, "    sub esp, %d\n", 4 * sc.frame_slots);

    cg->scope = &sc;
    gen_stmt(cg, pf->as.procfunc.body, str_idx);
    cg->scope = NULL;

    if (pf->as.procfunc.has_result) {
        /* Load the result slot (slot 0) into eax for the return value. */
        fprintf(o, "    mov eax, ");
        cg_ebp(o, cg_local_offset(0));
        fprintf(o, "\n");
    }
    fprintf(o, "    leave\n");
    fprintf(o, "    ret\n");
}

/* Collect declared variable names (AST_VARREF leaves under vardecls). */
static void emit_bss(Cg *cg, const AstNode *program)
{
    FILE *o = cg->out;
    const AstList *decls = &program->as.program.decls;
    if (decls->count == 0)
        return;
    fprintf(o, "section .bss\n");
    fprintf(o, "align 4\n");
    for (size_t i = 0; i < decls->count; i++) {
        const AstNode *vd = decls->items[i];
        /* B3 (beads initech-7mo3): a const-decl gets NO .bss slot at all --
         * "front-end FOLD to literals at use sites (no runtime storage)"
         * per the bead. Every const use was already replaced by a literal
         * AST node at parse time (seed/parser.c's parse_factor), so an
         * AST_CONSTDECL reaching here carries no codegen obligation
         * whatsoever; skip it rather than treating it as the internal
         * contract violation a non-vardecl/non-constdecl node would be. */
        if (vd->kind == AST_CONSTDECL)
            continue;
        /* B4 (beads initech-63ce): procedure/function decls carry no .bss
         * slot -- their params/locals/result are frame-resident (emit_proc).
         * Skip them here exactly as const-decls are skipped. */
        if (vd->kind == AST_PROCDECL || vd->kind == AST_FUNCDECL)
            continue;
        /* B6 (beads initech-rug7): a `type` declaration carries no .bss slot
         * either -- skip it exactly as const/proc decls are skipped. */
        if (vd->kind == AST_TYPEDECL)
            continue;
        if (vd->kind != AST_VARDECL)
            cg_ice("non-vardecl in program decls", vd);
        /* B6 (beads initech-rug7): a RECORD-typed global's per-ELEMENT word
         * count is its record type's field count (1 for a scalar,
         * non-record global -- the pre-B6 case, unchanged). */
        int rec_fields = vd->as.vardecl.rectype
                        ? cg_rectype_fields(cg, vd->as.vardecl.rectype) : 0;
        int elem_words = rec_fields > 0 ? rec_fields : 1;
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (vr->kind != AST_VARREF)
                cg_ice("non-varref in vardecl names", vr);
            fprintf(o, "v_");
            emit_var_label(o, vr->as.varref.name);
            /* B5 (beads initech-54uu): a GLOBAL array is a SIZED .bss block
             * of (hi-lo+1) uniform 4-byte slots (resd N) instead of the
             * scalar resd 1 -- the array's element 0 (index lo) is the
             * label's address itself, exactly like a scalar's slot. B6
             * (beads initech-rug7): each element/the scalar itself now
             * occupies `elem_words` dwords instead of always 1. */
            if (vd->as.vardecl.is_array) {
                long count = vd->as.vardecl.hi - vd->as.vardecl.lo + 1;
                fprintf(o, ": resd %ld\n", count * elem_words);
            } else {
                fprintf(o, ": resd %d\n", elem_words);
            }
        }
    }
    fprintf(o, "\n");
}

int codegen_emit(const AstNode *program, FILE *out)
{
    if (!program || program->kind != AST_PROGRAM) {
        fprintf(stderr, "initechc: codegen: root is not a program\n");
        return 1;
    }

    Cg cg;
    cg.out = out;
    cg.str_count = 0;
    cg.lbl_count = 0;
    cg.nproc = 0;
    cg.ngarr = 0;
    cg.nrectypes = 0;
    cg.scope = NULL;

    /* B6 (beads initech-rug7): build the RECORD-TYPE table (name -> field
     * COUNT) from every top-level AST_TYPEDECL, BEFORE the global-array
     * table below (which may need a record type's field count for an
     * array-of-record's element word-count) and before emit_bss/.text
     * (which need it for frame-slot/.bss sizing and stride). A record
     * type's field count is the SUM of each field-GROUP's name count
     * (flattened declaration order -- ast.h's B6 layout-rule comment: "kind,
     * x: integer; ch: char;" is two groups but three fields). */
    for (size_t i = 0; i < program->as.program.decls.count; i++) {
        const AstNode *td = program->as.program.decls.items[i];
        if (td->kind != AST_TYPEDECL)
            continue;
        if (cg.nrectypes >= CG_MAX_RECTYPES) {
            fprintf(stderr, "initechc: codegen: too many record types\n");
            return 1;
        }
        CgRecordType *rt = &cg.rectypes[cg.nrectypes++];
        cg_lower(rt->name, sizeof(rt->name), td->as.typedecl.name);
        rt->nfields = 0;
        for (size_t g = 0; g < td->as.typedecl.fields.count; g++) {
            const AstNode *fg = td->as.typedecl.fields.items[g];
            rt->nfields += (int)fg->as.vardecl.names.count;
        }
    }

    /* B5 (beads initech-54uu): build the GLOBAL array table (name -> bounds)
     * from every top-level AST_VARDECL group with is_array set, BEFORE
     * emit_bss/.text so gen_elem_addr can resolve a global array's `lo`
     * bound the moment it is reached during either pass. Mirrors the
     * routine-table build just below (collected once, consulted
     * throughout). B6 (beads initech-rug7): also records the element's
     * record-type field count (0 for a scalar-element array, unchanged). */
    for (size_t i = 0; i < program->as.program.decls.count; i++) {
        const AstNode *vd = program->as.program.decls.items[i];
        if (vd->kind != AST_VARDECL || !vd->as.vardecl.is_array)
            continue;
        int rec_fields = vd->as.vardecl.rectype
                        ? cg_rectype_fields(&cg, vd->as.vardecl.rectype) : 0;
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (cg.ngarr >= CG_MAX_GLOBAL_ARRAYS) {
                fprintf(stderr, "initechc: codegen: too many global arrays\n");
                return 1;
            }
            CgGlobalArr *ga = &cg.garr[cg.ngarr++];
            cg_lower(ga->name, sizeof(ga->name), vr->as.varref.name);
            ga->lo = vd->as.vardecl.lo;
            ga->hi = vd->as.vardecl.hi;
            ga->rec_fields = rec_fields;
        }
    }

    /* B4 (beads initech-63ce): build the routine call table (name -> arity +
     * per-parameter by-reference flags + result-ness) from every proc/func
     * declaration. A forward declaration and its defining occurrence share
     * one identical signature, so dedup by name (the first seen wins; both
     * carry the same is_var flags). */
    for (size_t i = 0; i < program->as.program.decls.count; i++) {
        const AstNode *d = program->as.program.decls.items[i];
        if (d->kind != AST_PROCDECL && d->kind != AST_FUNCDECL)
            continue;
        if (cg_proc_find(&cg, d->as.procfunc.name))
            continue; /* already registered (forward + defining) */
        if (cg.nproc >= CG_MAX_PROCS) {
            fprintf(stderr, "initechc: codegen: too many routines\n");
            return 1;
        }
        CgProc *pr = &cg.proctab[cg.nproc++];
        cg_lower(pr->name, sizeof(pr->name), d->as.procfunc.name);
        pr->has_result = d->as.procfunc.has_result;
        pr->nparams = (int)d->as.procfunc.params.count;
        if (pr->nparams > CG_MAX_PARAMS) {
            fprintf(stderr, "initechc: codegen: too many parameters\n");
            return 1;
        }
        for (int k = 0; k < pr->nparams; k++) {
            const AstNode *pn = d->as.procfunc.params.items[k];
            pr->is_var[k] = cg_param_is_var(pn->as.param.is_var);
        }
    }

    /* File banner (deterministic; cites the PRD per Law 1). */
    fprintf(out,
        "; Generated by initechc (seed cross-compiler, beads initech-znb).\n"
        "; Ref: PRD Sec 6.7 (stack-machine codegen, fixed registers, no\n"
        ";      optimizer/allocator). DO NOT EDIT -- emit is deterministic.\n"
        "; ABI: pas_main is called by the runtime (seed/rt/start.asm).\n"
        ";      Expressions evaluate onto the x86 stack; eax = result.\n"
        "bits 32\n\n");

    /* externs: runtime serial helpers. */
    fprintf(out, "extern serial_putc\n");
    fprintf(out, "extern serial_puts\n");
    fprintf(out, "extern serial_put_int\n\n");

    /* .rodata: string literals, in source order, plus the two fixed boolean
     * print constants (B1, beads initech-f0uc). These are emitted
     * unconditionally (whether or not the program prints a boolean) -- a
     * few constant bytes, in exchange for a simpler, branch-free rodata
     * pass with no "does this program need them" bookkeeping. */
    fprintf(out, "section .rodata\n");
    fprintf(out, "str_bool_true: db 84,82,85,69,0\n");   /* "TRUE" */
    fprintf(out, "str_bool_false: db 70,65,76,83,69,0\n"); /* "FALSE" */
#ifdef SEED_MUT_NONDET
    /* MUTATION HOOK (Rule 6; bead initech-3yv, ADR-0007 FO-5): see the file
     * header comment. Dead data, never referenced -- perturbs .s/.o/.elf
     * bytes only, never pas_main's behavior. */
    {
        unsigned long mut_pid = (unsigned long)getpid();
        fprintf(out, "seed_mut_nondet_pid: db %lu,%lu,%lu,%lu,0\n",
                (mut_pid >> 24) & 0xFFUL, (mut_pid >> 16) & 0xFFUL,
                (mut_pid >> 8) & 0xFFUL, mut_pid & 0xFFUL);
    }
#endif
    rodata_walk(&cg, program);
    fprintf(out, "\n");

    /* .bss: variable slots. */
    emit_bss(&cg, program);

    /* .text: routines first (source order), then the program body as
     * pas_main. The ONE str_idx counter threads across routines THEN main in
     * the SAME order the .rodata pass walked them (see rodata_walk's
     * AST_PROGRAM case), so str_<n> ordinals line up. The ONE lbl_count
     * (cg.lbl_count) likewise continues across every routine and pas_main --
     * DEC-04's single threaded label counter, now spanning routines too. */
    fprintf(out, "section .text\n");

    int str_idx = 0;
    for (size_t i = 0; i < program->as.program.decls.count; i++) {
        const AstNode *d = program->as.program.decls.items[i];
        if (d->kind == AST_PROCDECL || d->kind == AST_FUNCDECL)
            emit_proc(&cg, d, &str_idx);
    }

    fprintf(out, "global pas_main\n");
    fprintf(out, "pas_main:\n");
    fprintf(out, "    push ebp\n");
    fprintf(out, "    mov ebp, esp\n");

    cg.scope = NULL; /* pas_main: globals only */
    gen_stmt(&cg, program->as.program.block, &str_idx);

    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");

    if (ferror(out)) {
        fprintf(stderr, "initechc: codegen: write error\n");
        return 1;
    }
    return 0;
}
