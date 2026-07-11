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

typedef struct {
    FILE *out;
    int   str_count;      /* next .rodata string label ordinal */
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

/* ---- fatal (Rule 2): internal AST contract violation, never user input ---- */
static void cg_ice(const char *what, const AstNode *n)
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
    case AST_VARREF:
        fprintf(o, "    mov eax, [v_");
        emit_var_label(o, e->as.varref.name);
        fprintf(o, "]\n");
        break;
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
    case AST_ASSIGN:
        gen_expr(cg, n->as.assign.value);       /* value -> eax */
        fprintf(o, "    mov [v_");
        emit_var_label(o, n->as.assign.name);
        fprintf(o, "], eax\n");
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
        if (vd->kind != AST_VARDECL)
            cg_ice("non-vardecl in program decls", vd);
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (vr->kind != AST_VARREF)
                cg_ice("non-varref in vardecl names", vr);
            fprintf(o, "v_");
            emit_var_label(o, vr->as.varref.name);
            fprintf(o, ": resd 1\n");
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

    /* .text: the program body as pas_main. */
    fprintf(out, "section .text\n");
    fprintf(out, "global pas_main\n");
    fprintf(out, "pas_main:\n");
    fprintf(out, "    push ebp\n");
    fprintf(out, "    mov ebp, esp\n");

    int str_idx = 0;
    gen_stmt(&cg, program->as.program.block, &str_idx);

    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");

    if (ferror(out)) {
        fprintf(stderr, "initechc: codegen: write error\n");
        return 1;
    }
    return 0;
}
