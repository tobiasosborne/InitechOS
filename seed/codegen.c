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
/*
 * ============================================================================
 * B7 (beads initech-39k2; B7 committee 2026-07-14 (3 seats + chair synthesis),
 * bead initech-39k2; ADR-0007 DEC-02 "minimal fixed/ShortString-style strings
 * (length/index/compare/concat)", DEC-04 deterministic codegen, DEC-05 "the
 * RTL stays byte/block I/O only") -- fixed/ShortString strings + frame-resident
 * temporaries.
 *
 * REPRESENTATION (ast.h's B7 block has the full layout rule). A ShortString is
 * byte 0 = length, bytes 1..N = content, W = round4(N+1) bytes. A GLOBAL is a
 * .bss `resb W`; a LOCAL is W/4 contiguous frame dwords whose designator base
 * is the block's LOWEST address (byte i at base+i, ascending -- deliberately
 * UNLIKE B5's array-local element-0-at-highest convention, so every storage
 * class shares one flat ascending pointer). A `var string` parameter's frame
 * slot holds the caller's byte-0 POINTER. Capacity is ALWAYS a compile-time
 * immediate (assign.strcap for a target; 255 for the concat intermediate);
 * there is NO runtime cap field.
 *
 * INTRINSIC ABI (DEC-05: NOT the RTL -- start.asm is untouched; codegen emits a
 * fixed __str_* prelude in .text IFF program.uses_strings, so a stringless
 * program's .s is byte-identical to pre-B7). Fixed internal labels in the
 * start.asm .next/.done local-label style; the DEC-04 threaded ordinal counter
 * is NOT consumed by intrinsics. 386-safe only (movzx / branch-min / rep
 * movsb/cmpsb; NO cmov). Each helper GUARDS len=0 and clobbers only caller-
 * saved-equivalent registers (eax/ecx/edx/esi/edi); it preserves ebx/ebp/esp,
 * so a temp address computed via lea from ebp survives every call.
 *   __str_assign (edi=&dst, esi=&src, ecx=cap): n=min(len src, cap); copy n
 *      content bytes; dst[0]=n. Silent truncation (TP/fpc).
 *   __str_concat (edi=&dst, esi=&src): append src content to dst, clamping the
 *      dst length at 255 (the ShortString intermediate cap); never wraps.
 *   __str_cmp (esi=&a, edi=&b): eax = -1/0/+1. Unsigned bytewise over the
 *      min-length prefix; first differing byte decides; prefix-equal => the
 *      shorter compares LESS (length tiebreak).
 *   __str_write (esi=&s): read length, loop content bytes through the EXISTING
 *      serial_putc (never serial_puts -- a ShortString may contain 0x00).
 * length() is emitted INLINE (`movzx eax, byte [base]`), no intrinsic.
 *
 * TEMPORARIES (D4, the deep-bug locus + the chair's correction). Concat/
 * coercion/compare materialize into FRAME temporaries of CG_STR_TEMP_DWORDS (64
 * dwords = 256 bytes) each, allocated AFTER a routine's locals. A deterministic
 * per-routine source-order pre-walk (plan_string_temps) stamps each
 * materializing node's str_temp INDEX, RESET per statement, and sizes the
 * routine's temp region to the max-live count; pas_main gets its own temp
 * region (globals stay .bss). LEFT-DEEP a+b+c reuses ONE accumulator temp
 * (in-place accumulation); a RIGHT-NESTED a+(b+c) forces a SECOND
 * simultaneously-live temp. THE CHAIR'S CORRECTION (binding): no string temp is
 * ever live across a call in this subset, so the temp-lifetime bug is
 * TWO-LIVE-TEMPS-IN-ONE-STATEMENT (right-nested concat), NOT clobber-across-
 * recursion.
 *
 * WORKED SEQUENCE -- s := a + (b + c)  (a,b,c string designators; s cap Cs):
 *   ; RHS concat node C=(+ a (+ b c)); planner: C->str_temp=T0, (b+c)->str_temp=T1
 *   ; -- init accumulator T0 with the leftmost leaf a:
 *   mov eax, a_addr / mov esi, eax / lea edi, [T0] / mov ecx, 255 / call __str_assign
 *   ; -- append the RIGHT operand (b+c), materialized into T1 first:
 *   mov eax, b_addr / mov esi, eax / lea edi, [T1] / mov ecx, 255 / call __str_assign
 *   mov eax, c_addr / mov esi, eax / lea edi, [T1] / call __str_concat   ; T1 = b+c
 *   lea eax, [T1]   / mov esi, eax / lea edi, [T0] / call __str_concat   ; T0 += T1
 *   ; -- store the fully-evaluated RHS into s with truncation:
 *   lea eax, [T0]   / mov esi, eax / <dst base -> edi> / mov ecx, Cs / call __str_assign
 * Under SEED_MUT_CODEGEN_STR_TEMP_CLOBBER (every str_temp forced to 0) T1==T0,
 * so evaluating (b+c) OVERWRITES a's copy in T0 and the golden RNEST tag flips
 * -- the load-bearing bite (string.pas RNEST). Under
 * SEED_MUT_CODEGEN_STR_CMP_NOLEN, __str_cmp drops the length tiebreak so
 * 'ab' = 'abc' is wrongly TRUE (string.pas EQF).
 * ============================================================================
 */
#define CG_NAME_CAP    128
/* B7 (beads initech-39k2): one string TEMPORARY is 64 dwords (256 bytes) --
 * enough for a full string[255] ShortString intermediate (cap-255 accumulator).
 * A routine reserves max-live-temps of these AFTER its locals/result. */
#define CG_STR_TEMP_DWORDS 64
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
    /* B7 (beads initech-39k2): the ShortString capacity (1..255) for a string
     * LOCAL or a `var string` parameter (always 255 for the latter), else 0.
     * For a string local, `offset` is byte 0 (the block's LOWEST address);
     * for a `var string` parameter, the slot holds the caller's byte-0
     * pointer (CG_VARPARAM). strcap>0 marks a string entry. */
    int    strcap;
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
    int   str_count;      /* next .rodata write-literal (str_<n>) ordinal */
    /* B7 (beads initech-39k2): a SEPARATE ordinal for EXPRESSION-context string
     * literals (strlit_<n>, length-prefixed) -- assigned in the ONE rodata
     * walk and STORED on each AST_STRLIT node's lit_ord (the .text pass reads
     * it, never re-derives it, so there is no second divergent walk). The
     * write-literal str_count sequence is left byte-identical to pre-B7. */
    int   strlit_count;
    /* B7: program.uses_strings (typecheck's verdict) -- the __str_* intrinsic
     * prelude + string machinery are emitted only when set. */
    int   uses_strings;
    /* B7: the routine currently being emitted -- its frame_slots (0 for
     * pas_main), so string-temp addressing can place temps AFTER the locals.
     * Set in emit_proc / the pas_main emission, read by cg_str_temp_ebp. */
    int   cur_frame_slots;
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

/* ----- .rodata string pass: assign ordinal labels to each string literal --- */
/*
 * Two label families, each with its OWN ordinal counter, in ONE source-order
 * walk (no second re-walk -- DEC-04):
 *   - WRITE-argument bare literals -> `str_<n>` (NUL-terminated), str_count.
 *     UNCHANGED from pre-B7; the .text pass (gen_write) re-derives the ordinal
 *     by counting write-literals in the same order, so pre-B7 output stays
 *     byte-identical.
 *   - B7 (beads initech-39k2) EXPRESSION-context literals -> `strlit_<n>`
 *     (length-prefixed, `db len, bytes`), strlit_count, with the ordinal
 *     STORED on the node (lit_ord) so the .text pass reads it directly (no
 *     re-derivation, so no divergence). Reached via rodata_walk_expr, which
 *     descends assignment values, conditions, call args, and non-literal write
 *     args -- pre-B7 programs have no such literals, so this emits nothing and
 *     the str_<n> sequence is untouched.
 */

static void rodata_walk(Cg *cg, AstNode *n);
static void rodata_walk_expr(Cg *cg, AstNode *e);

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

/* B7 (beads initech-39k2): a length-prefixed .rodata blob for an expression-
 * context string literal, its ordinal stored on the node. */
static void rodata_emit_strlit(Cg *cg, AstNode *s)
{
    FILE *o = cg->out;
    int idx = cg->strlit_count++;
    s->as.strlit.lit_ord = idx;
    fprintf(o, "strlit_%d:\n", idx);
    fprintf(o, "    db %zu", s->as.strlit.length);   /* length prefix (0..255) */
    for (size_t i = 0; i < s->as.strlit.length; i++)
        fprintf(o, ",%u", (unsigned char)s->as.strlit.text[i]);
    fprintf(o, "\n");
}

/* Descend an EXPRESSION, emitting strlit_<n> for every AST_STRLIT reached
 * (B7). Pre-B7 expressions contain no AST_STRLIT, so this is inert for them. */
static void rodata_walk_expr(Cg *cg, AstNode *e)
{
    if (!e)
        return;
    switch (e->kind) {
    case AST_STRLIT:
        rodata_emit_strlit(cg, e);
        break;
    case AST_BINOP:
        rodata_walk_expr(cg, e->as.binop.lhs);
        rodata_walk_expr(cg, e->as.binop.rhs);
        break;
    case AST_UNOP:
        rodata_walk_expr(cg, e->as.unop.operand);
        break;
    case AST_INDEX:
        rodata_walk_expr(cg, e->as.arrayindex.index);
        break;
    case AST_FIELD:
        rodata_walk_expr(cg, e->as.field.base);
        break;
    case AST_CALL:
        for (size_t i = 0; i < e->as.call.args.count; i++)
            rodata_walk_expr(cg, e->as.call.args.items[i]);
        break;
    default:
        break; /* leaves (int/char/bool/varref) hold no string literal */
    }
}

static void rodata_walk(Cg *cg, AstNode *n)
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
            AstNode *d = n->as.program.decls.items[i];
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
                rodata_emit_string(cg, arg);   /* a bare write literal (str_) */
            else
                rodata_walk_expr(cg, arg);     /* B7: an expression arg */
        }
        break;
    case AST_ASSIGN:
        /* B7 (beads initech-39k2): an assignment value (and a string index)
         * may now contain string literals (`s := 'x' + s`, `s[i] := ...`). */
        rodata_walk_expr(cg, n->as.assign.index);
        rodata_walk_expr(cg, n->as.assign.value);
        break;
    /* B2 (beads initech-80iw): if/while bodies can contain write/writeln
     * with string args (control.pas's IF1/IF2/DE tags do exactly this), so
     * this pass MUST recurse into them in the SAME order gen_stmt's .text
     * pass will. B7 (beads initech-39k2): the CONDITION is now also walked as
     * an expression -- a string compare in a guard (`if s1 = s2 ...`) may
     * carry a coerced-char or literal operand needing a strlit_<n> blob. */
    case AST_IF:
        rodata_walk_expr(cg, n->as.ifstmt.cond);
        rodata_walk(cg, n->as.ifstmt.then_stmt);
        rodata_walk(cg, n->as.ifstmt.else_stmt);
        break;
    case AST_WHILE:
        rodata_walk_expr(cg, n->as.whilestmt.cond);
        rodata_walk(cg, n->as.whilestmt.body);
        break;
    case AST_CALL:
        /* B7 (beads initech-39k2): a procedure-call statement's args (a
         * stage-then-pass literal is a `var` arg -- a designator -- but a value
         * arg could embed a string literal via a compare/ord). */
        for (size_t i = 0; i < n->as.call.args.count; i++)
            rodata_walk_expr(cg, n->as.call.args.items[i]);
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
/* B7 (beads initech-39k2): string materialization + addressing. */
static void gen_str_value(Cg *cg, const AstNode *e);   /* byte-0 addr -> eax */
static void emit_concat_into(Cg *cg, const AstNode *e);/* concat -> e->str_temp */
static void gen_str_base_addr(Cg *cg, const char *name);/* designator base -> eax */
static void gen_str_compare(Cg *cg, const AstNode *e); /* string relop -> 0/1 eax */

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

/*
 * ============================================================================
 * B7 (beads initech-39k2) -- string materialization, addressing, temporaries.
 * See this file's B7 header block for the layout rule, the intrinsic ABI, the
 * temp model, and the worked s := a + (b + c) sequence.
 * ============================================================================
 */

/* Byte-0 ebp displacement of string TEMPORARY k in the routine currently being
 * emitted: the temps sit AFTER the routine's locals/result (cur_frame_slots
 * dwords), each CG_STR_TEMP_DWORDS wide, and the designator base is the block's
 * LOWEST address (= its highest slot number, so byte i ascends toward ebp). */
static int cg_str_temp_offset(const Cg *cg, int k)
{
    return cg_local_offset(cg->cur_frame_slots + (k + 1) * CG_STR_TEMP_DWORDS
                           - 1);
}

/* Emit "lea <reg>, [ebp-N]" for string temp k's byte 0. */
static void cg_emit_temp_lea(Cg *cg, const char *reg, int k)
{
    fprintf(cg->out, "    lea %s, ", reg);
    cg_ebp(cg->out, cg_str_temp_offset(cg, k));
    fprintf(cg->out, "\n");
}

/* Byte-0 address of a string DESIGNATOR `name` -> eax. A `var string`
 * parameter's frame slot holds the caller's byte-0 POINTER (forward it); a
 * string LOCAL's offset is byte 0 (its block's lowest address) so `lea`;
 * a global string is `v_<name>` (byte 0). */
static void gen_str_base_addr(Cg *cg, const char *name)
{
    FILE *o = cg->out;
    const CgScopeEnt *se = cg_resolve(cg, name);
    if (se) {
        if (se->kind == CG_VARPARAM) {
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

/*
 * gen_str_value: leave the byte-0 ADDRESS of expression `e`'s string value in
 * eax, MATERIALIZING into e->str_temp when needed. The four cases:
 *   - AST_STRLIT (expression literal): its .rodata label `strlit_<ord>`.
 *   - string designator (AST_VARREF): its own storage address.
 *   - concat (AST_BINOP OP_ADD, string): emit_concat_into e->str_temp, addr it.
 *   - char coercion (e->type == CHAR): materialize a len-1 string into
 *     e->str_temp (byte 0 = 1, byte 1 = the char). See ast.h's B7 coercion
 *     table (contexts 1-3).
 */
static void gen_str_value(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    if (e->kind == AST_STRLIT) {
        fprintf(o, "    mov eax, strlit_%d\n", e->as.strlit.lit_ord);
        return;
    }
    if (e->kind == AST_VARREF && e->type == AST_TY_STRING) {
        gen_str_base_addr(cg, e->as.varref.name);
        return;
    }
    if (e->kind == AST_BINOP && e->as.binop.op == OP_ADD
        && e->type == AST_TY_STRING) {
        emit_concat_into(cg, e);
        cg_emit_temp_lea(cg, "eax", e->str_temp);
        return;
    }
    if (e->type == AST_TY_CHAR) {
        gen_expr(cg, e);                       /* al = char byte */
        cg_emit_temp_lea(cg, "edx", e->str_temp);
        fprintf(o, "    mov byte [edx], 1\n");
        fprintf(o, "    mov [edx+1], al\n");
        fprintf(o, "    mov eax, edx\n");
        return;
    }
    cg_ice("non-string expression reached string-value materialization", e);
}

/*
 * emit_concat_into: materialize concat `e` into its accumulator temp
 * e->str_temp, in-place. The LEFT spine shares the accumulator (left-deep chains
 * cost ONE temp); the RIGHT operand is materialized (into ITS temp, if it is a
 * concat/char) then appended -- a right-nested a+(b+c) is thus the two-live-
 * temps case the STR_TEMP_CLOBBER mutant collapses. The leftmost leaf
 * INITIALIZES the accumulator (a char inline, a designator/literal via a cap-255
 * __str_assign); every subsequent operand is a __str_concat.
 */
static void emit_concat_into(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    int t = e->str_temp;
    const AstNode *lhs = e->as.binop.lhs;
    const AstNode *rhs = e->as.binop.rhs;

    if (lhs->kind == AST_BINOP && lhs->as.binop.op == OP_ADD
        && lhs->type == AST_TY_STRING) {
        emit_concat_into(cg, lhs);            /* lhs->str_temp == t (planner) */
    } else if (lhs->type == AST_TY_CHAR) {
        gen_expr(cg, lhs);                    /* al = char */
        cg_emit_temp_lea(cg, "edx", t);
        fprintf(o, "    mov byte [edx], 1\n");
        fprintf(o, "    mov [edx+1], al\n");
    } else {
        gen_str_value(cg, lhs);               /* eax = src addr (designator/lit) */
        fprintf(o, "    mov esi, eax\n");
        cg_emit_temp_lea(cg, "edi", t);
        fprintf(o, "    mov ecx, 255\n");
        fprintf(o, "    call __str_assign\n");
    }

    gen_str_value(cg, rhs);                   /* eax = rhs addr (materialized) */
    fprintf(o, "    mov esi, eax\n");
    cg_emit_temp_lea(cg, "edi", t);
    fprintf(o, "    call __str_concat\n");
}

/*
 * gen_str_compare: a string relop `lhs op rhs` (>= 1 string operand; the char
 * side coerces). Materialize lhs (spill its addr), materialize rhs into a
 * DISTINCT temp (the planner gives rhs a higher temp base so lhs survives),
 * call __str_cmp (eax = -1/0/+1), then the EXACT B1 relational tail
 * (`cmp eax, 0` + setcc + movzx). The existing SETL_AS_SETG mutant thereby also
 * perturbs string </> (incidental cross-coverage, D8); the string mutant
 * (STR_CMP_NOLEN) targets __str_cmp's BODY instead.
 */
static void gen_str_compare(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    gen_str_value(cg, e->as.binop.lhs);       /* eax = lhs addr */
    fprintf(o, "    push eax\n");
    gen_str_value(cg, e->as.binop.rhs);       /* eax = rhs addr */
    fprintf(o, "    mov edi, eax\n");
    fprintf(o, "    pop esi\n");
    fprintf(o, "    call __str_cmp\n");
    fprintf(o, "    cmp eax, 0\n");
    switch (e->as.binop.op) {
    case OP_EQ: fprintf(o, "    sete al\n"); break;
    case OP_NE: fprintf(o, "    setne al\n"); break;
    case OP_LE: fprintf(o, "    setle al\n"); break;
    case OP_GE: fprintf(o, "    setge al\n"); break;
    case OP_LT:
#ifdef SEED_MUT_CODEGEN_SETL_AS_SETG
        fprintf(o, "    setg al\n");
#else
        fprintf(o, "    setl al\n");
#endif
        break;
    case OP_GT:
#ifdef SEED_MUT_CODEGEN_SETL_AS_SETG
        fprintf(o, "    setl al\n");
#else
        fprintf(o, "    setg al\n");
#endif
        break;
    default:
        cg_ice("non-relational operator in gen_str_compare", e);
    }
    fprintf(o, "    movzx eax, al\n");
}

static void gen_binop(Cg *cg, const AstNode *e)
{
    FILE *o = cg->out;
    /* B7 (beads initech-39k2; D7/D8): a relop with a string operand is a
     * string compare (0/1 in eax), not the integer stack-machine path below.
     * A string CONCAT never reaches gen_binop -- string VALUES route through
     * gen_str_value, so a concat here is an internal contract break. */
    {
        AstOp op = e->as.binop.op;
        int lstr = (e->as.binop.lhs->type == AST_TY_STRING);
        int rstr = (e->as.binop.rhs->type == AST_TY_STRING);
        if ((lstr || rstr)
            && (op == OP_EQ || op == OP_NE || op == OP_LT || op == OP_LE
                || op == OP_GT || op == OP_GE)) {
            gen_str_compare(cg, e);
            return;
        }
        if (op == OP_ADD && (lstr || rstr))
            cg_ice("string concat reached gen_binop (should route through "
                   "gen_str_value)", e);
    }
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
        /* B7 (beads initech-39k2; D5): a STRING s[i] is a 1-based BYTE read (a
         * char r-value) -- address = base + i (byte 0 is the length prefix, so
         * no -1), movzx to zero-extend. The base is spilled before the index
         * expression evaluates (it may call). NOT an array dword load. */
        if (e->as.arrayindex.is_string) {
            gen_str_base_addr(cg, e->as.arrayindex.name);   /* eax = base */
            fprintf(o, "    push eax\n");
            gen_expr(cg, e->as.arrayindex.index);           /* eax = i */
            fprintf(o, "    pop edx\n");                     /* edx = base */
            fprintf(o, "    movzx eax, byte [edx+eax]\n");   /* base + i */
            break;
        }
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
        /* B7 (beads initech-39k2; D9): length(s) is emitted INLINE -- the
         * ShortString length prefix at byte 0, zero-extended. The operand is
         * a string VALUE (routed through gen_str_value, NOT gen_expr, since a
         * string is an address not an int), so this is handled BEFORE the
         * generic gen_expr(operand) below. */
        if (e->as.unop.op == OP_LENGTH) {
            gen_str_value(cg, e->as.unop.operand);   /* eax = string base addr */
            fprintf(o, "    movzx eax, byte [eax]\n");
            break;
        }
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
        } else if (arg->type == AST_TY_STRING) {
            /* B7 (beads initech-39k2): a STRING expression arg (a string var or
             * a concat -- never a bare write literal, which is AST_STRLIT and
             * took the serial_puts path above) is written via __str_write,
             * which loops its content bytes through serial_putc (a ShortString
             * may contain 0x00, so it must NEVER route through serial_puts). */
            gen_str_value(cg, arg);            /* eax = string base addr */
            fprintf(o, "    mov esi, eax\n");
            fprintf(o, "    call __str_write\n");
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
        /* B7 (beads initech-39k2; D5): a STRING target -- handled FIRST
         * (assign.strcap>0, stamped by typecheck). `index` non-NULL is an
         * s[i] BYTE write (`s[i] := <char>`, no length change); `index` NULL
         * is a WHOLE-string assign with silent truncation to strcap. A bare
         * char RHS uses the inline fast path (store len 1 + the byte, D4);
         * anything else materializes the RHS FULLY into a temp/addr, then
         * __str_assign copies it to dst (NEVER writing dst while it is a live
         * source -- the aliasing rule). */
        if (n->as.assign.strcap > 0) {
            if (n->as.assign.index) {
                gen_str_base_addr(cg, n->as.assign.name);  /* eax = base */
                fprintf(o, "    push eax\n");
                gen_expr(cg, n->as.assign.index);          /* eax = i */
                fprintf(o, "    pop edx\n");
                fprintf(o, "    add edx, eax\n");          /* edx = base + i */
                fprintf(o, "    push edx\n");
                gen_expr(cg, n->as.assign.value);          /* al = char */
                fprintf(o, "    pop edx\n");
                fprintf(o, "    mov [edx], al\n");         /* byte; no length */
                break;
            }
            if (n->as.assign.value->type == AST_TY_CHAR) {
                /* fast path: s := <char> is an inline len-1 store. */
                gen_str_base_addr(cg, n->as.assign.name);  /* eax = dst base */
                fprintf(o, "    push eax\n");
                gen_expr(cg, n->as.assign.value);          /* al = char */
                fprintf(o, "    pop edx\n");
                fprintf(o, "    mov byte [edx], 1\n");
                fprintf(o, "    mov [edx+1], al\n");
                break;
            }
            gen_str_value(cg, n->as.assign.value);         /* eax = src addr */
            fprintf(o, "    mov esi, eax\n");
            gen_str_base_addr(cg, n->as.assign.name);      /* eax = dst base */
            fprintf(o, "    mov edi, eax\n");
            fprintf(o, "    mov ecx, %d\n", n->as.assign.strcap);
            fprintf(o, "    call __str_assign\n");
            break;
        }
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
/*
 * ============================================================================
 * B7 (beads initech-39k2) -- the string-TEMPORARY pre-walk (D4). A pure
 * function of the AST (DEC-04 deterministic): per ROUTINE, walk statements in
 * source order and stamp each string-materializing node's str_temp INDEX,
 * RESET per statement; the routine reserves max-live-count temps. In-place
 * LEFT-DEEP accumulation shares one temp; a RIGHT-NESTED operand gets a fresh
 * simultaneously-live temp. The mutant SEED_MUT_CODEGEN_STR_TEMP_CLOBBER forces
 * EVERY index to 0 (collapse-to-one) at the single allocation choke point
 * below -- string.pas's right-nested RNEST clause then corrupts.
 * ============================================================================
 */

/* The single temp-index allocation choke point (the CLOBBER mutant's locus). */
static int str_temp_alloc(int requested)
{
#ifdef SEED_MUT_CODEGEN_STR_TEMP_CLOBBER
    /* MUTATION HOOK (Rule 6; beads initech-39k2, D10 (i)): collapse every
     * string temporary to index 0, so a right-nested concat a+(b+c) evaluates
     * (b+c) into the SAME temp holding a's copy. test-seed-string-mutant
     * asserts string.pas's RNEST tag goes RED. */
    (void)requested;
    return 0;
#else
    return requested;
#endif
}

/* Does `e`, used as a STRING value, need a temporary? A plain designator or a
 * .rodata literal does not; a concat or a coerced char does. */
static int str_materializes(const AstNode *e)
{
    if (e->kind == AST_STRLIT)
        return 0;
    if (e->kind == AST_VARREF && e->type == AST_TY_STRING)
        return 0;
    return 1;
}

static int plan_str(AstNode *e, int b);

/* Walk a NON-string-context expression `e` for EMBEDDED string materializations
 * (a string compare buried in a boolean/integer expression, e.g. an `if`
 * condition). Returns the highest temp index used, or b-1 if none. */
static int plan_val(AstNode *e, int b)
{
    if (!e)
        return b - 1;
    switch (e->kind) {
    case AST_INDEX:
        /* string s[i] or array a[i]: the index is an integer expression. */
        return plan_val(e->as.arrayindex.index, b);
    case AST_UNOP:
        return plan_val(e->as.unop.operand, b);
    case AST_CALL: {
        int hi = b - 1;
        for (size_t i = 0; i < e->as.call.args.count; i++) {
            int m = plan_val(e->as.call.args.items[i], b); /* args sequential */
            if (m > hi) hi = m;
        }
        return hi;
    }
    case AST_BINOP: {
        AstOp op = e->as.binop.op;
        AstNode *l = e->as.binop.lhs, *r = e->as.binop.rhs;
        if ((op == OP_EQ || op == OP_NE || op == OP_LT || op == OP_LE
             || op == OP_GT || op == OP_GE)
            && (l->type == AST_TY_STRING || r->type == AST_TY_STRING)) {
            /* a string compare: lhs and rhs are SIMULTANEOUSLY live for
             * __str_cmp, so rhs's temps sit ABOVE lhs's. */
            int hl = plan_str(l, b);
            int hr = plan_str(r, hl + 1);
            return hr;
        }
        int hl = plan_val(l, b);
        int hr = plan_val(r, b); /* sequential (lhs spilled to the data stack) */
        return (hl > hr) ? hl : hr;
    }
    default:
        return b - 1; /* leaves + AST_FIELD: no string temp */
    }
}

/* Plan a STRING-context expression `e`, materialized into temp `b` if it
 * materializes. Returns the highest temp index used, or b-1 if none. */
static int plan_str(AstNode *e, int b)
{
    if (!str_materializes(e))
        return b - 1;                         /* designator / .rodata literal */
    if (e->kind == AST_BINOP && e->as.binop.op == OP_ADD
        && e->type == AST_TY_STRING) {
        e->str_temp = str_temp_alloc(b);      /* accumulator */
        int hi = b;
        AstNode *lhs = e->as.binop.lhs;
        AstNode *rhs = e->as.binop.rhs;
        if (lhs->kind == AST_BINOP && lhs->as.binop.op == OP_ADD
            && lhs->type == AST_TY_STRING) {
            int m = plan_str(lhs, b);         /* left spine shares accumulator */
            if (m > hi) hi = m;
        }
        /* a leftmost char/designator/literal initializes the accumulator with
         * no extra temp. */
        if (str_materializes(rhs)) {
            int m = plan_str(rhs, b + 1);     /* fresh simultaneously-live temp */
            if (m > hi) hi = m;
        }
        return hi;
    }
    /* a coerced char: materialize a len-1 string into temp b. Its own value
     * computation (rare embedded compares) reuses temps from b (dead before
     * the coercion store). */
    e->str_temp = str_temp_alloc(b);
    int m = plan_val(e, b);
    return (m > b) ? m : b;
}

/* Track a statement's max simultaneously-live temp count. */
static void plan_track(int hi, int *maxtemps)
{
    if (hi + 1 > *maxtemps)
        *maxtemps = hi + 1;
}

/* Walk one statement, RESETTING the temp base to 0 for each top-level string-
 * context expression (they evaluate sequentially, so each may reuse temp 0). */
static void plan_stmt(AstNode *n, int *maxtemps)
{
    if (!n)
        return;
    switch (n->kind) {
    case AST_BLOCK:
        for (size_t i = 0; i < n->as.block.stmts.count; i++)
            plan_stmt(n->as.block.stmts.items[i], maxtemps);
        return;
    case AST_ASSIGN:
        if (n->as.assign.strcap > 0) {                     /* string target */
            if (n->as.assign.index) {
                plan_track(plan_val(n->as.assign.index, 0), maxtemps);
                plan_track(plan_val(n->as.assign.value, 0), maxtemps);
            } else if (n->as.assign.value->type == AST_TY_CHAR) {
                plan_track(plan_val(n->as.assign.value, 0), maxtemps);
            } else if (str_materializes(n->as.assign.value)) {
                plan_track(plan_str(n->as.assign.value, 0), maxtemps);
            }
            return;
        }
        if (n->as.assign.rec_fields > 0)
            return;                                        /* record copy */
        if (n->as.assign.index)
            plan_track(plan_val(n->as.assign.index, 0), maxtemps);
        plan_track(plan_val(n->as.assign.value, 0), maxtemps);
        return;
    case AST_IF:
        plan_track(plan_val(n->as.ifstmt.cond, 0), maxtemps);
        plan_stmt(n->as.ifstmt.then_stmt, maxtemps);
        plan_stmt(n->as.ifstmt.else_stmt, maxtemps);
        return;
    case AST_WHILE:
        plan_track(plan_val(n->as.whilestmt.cond, 0), maxtemps);
        plan_stmt(n->as.whilestmt.body, maxtemps);
        return;
    case AST_WRITE:
    case AST_WRITELN:
        for (size_t i = 0; i < n->as.write.args.count; i++) {
            AstNode *arg = n->as.write.args.items[i];
            if (arg->kind == AST_STRLIT)
                continue;                                  /* write literal */
            if (arg->type == AST_TY_STRING)
                plan_track(plan_str(arg, 0), maxtemps);
            else
                plan_track(plan_val(arg, 0), maxtemps);
        }
        return;
    case AST_CALL:
        for (size_t i = 0; i < n->as.call.args.count; i++)
            plan_track(plan_val(n->as.call.args.items[i], 0), maxtemps);
        return;
    default:
        return;
    }
}

/* Reserve max-live string temps for a routine body (0 if it uses none). */
static int plan_string_temps(AstNode *body)
{
    int maxtemps = 0;
    plan_stmt(body, &maxtemps);
    return maxtemps;
}

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
        /* B7 (beads initech-39k2): a `var string` parameter (always cap 255)
         * -- its ONE slot holds the caller's byte-0 pointer (CG_VARPARAM). */
        e->strcap = (pn->as.param.ptype == AST_TY_STRING)
                  ? pn->as.param.strcap : 0;
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
        e->strcap = 0;     /* string function results are out of scope (B7) */
    }
    for (size_t i = 0; i < pf->as.procfunc.decls.count; i++) {
        const AstNode *vd = pf->as.procfunc.decls.items[i];
        if (vd->kind != AST_VARDECL)
            cg_ice("non-vardecl in routine locals", vd);
        int rec_fields = vd->as.vardecl.rectype
                        ? cg_rectype_fields(cg, vd->as.vardecl.rectype) : 0;
        int elem_words = rec_fields > 0 ? rec_fields : 1;
        /* B7 (beads initech-39k2): a LOCAL string occupies W/4 contiguous frame
         * dwords (W = round4(cap+1)); its designator base is the block's LOWEST
         * address (highest slot number), so byte i ascends toward ebp -- see
         * ast.h's B7 layout note (deliberately unlike B5's array-local
         * convention). */
        int strcap = (vd->as.vardecl.vtype == AST_TY_STRING)
                   ? vd->as.vardecl.strcap : 0;
        int strwords = strcap > 0 ? (strcap + 4) / 4 : 0;
        for (size_t j = 0; j < vd->as.vardecl.names.count; j++) {
            const AstNode *vr = vd->as.vardecl.names.items[j];
            if (sc->n >= CG_MAX_SCOPE)
                cg_ice("routine scope overflow (locals)", vr);
            CgScopeEnt *e = &sc->ent[sc->n++];
            cg_lower(e->name, sizeof(e->name), vr->as.varref.name);
            e->kind = CG_LOCAL;
            e->rec_fields = rec_fields;
            e->strcap = strcap;
            if (strcap > 0) {
                e->is_array = 0;
                e->lo = e->hi = 0;
                /* byte 0 = the block's LOWEST address = its highest slot. */
                e->offset = cg_local_offset(slot + strwords - 1);
                slot += strwords;
            } else if (vd->as.vardecl.is_array) {
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

    /* B7 (beads initech-39k2): reserve max-live string temporaries AFTER this
     * routine's locals/result. The cast drops const on a genuinely-mutable
     * arena node so the pre-walk can stamp str_temp (like typecheck's own
     * annotations). A stringless program plans 0 temps, so `sub esp` and the
     * prologue below stay byte-identical to pre-B7. */
    int max_temps = cg->uses_strings
                  ? plan_string_temps((AstNode *)pf->as.procfunc.body) : 0;
    int total_slots = sc.frame_slots + max_temps * CG_STR_TEMP_DWORDS;
    cg->cur_frame_slots = sc.frame_slots;

    fprintf(o, "pf_");
    emit_var_label(o, pf->as.procfunc.name);
    fprintf(o, ":\n");
    fprintf(o, "    push ebp\n");
    fprintf(o, "    mov ebp, esp\n");
    if (total_slots > 0)
        fprintf(o, "    sub esp, %d\n", 4 * total_slots);

    /* B7 (beads initech-39k2): zero each LOCAL string's length byte so
     * length()/write of an unassigned local is a well-defined empty string
     * (Rule 2 spirit; a small differential-invisible TP divergence -- fixtures
     * always assign before reading). A `var string` parameter aliases the
     * caller's (already-initialized) string, so it is NOT zeroed. */
    for (int i = 0; i < sc.n; i++) {
        if (sc.ent[i].strcap > 0 && sc.ent[i].kind == CG_LOCAL) {
            fprintf(o, "    mov byte ");
            cg_ebp(o, sc.ent[i].offset);
            fprintf(o, ", 0\n");
        }
    }

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
        /* B7 (beads initech-39k2): a GLOBAL string is a `resb W` block, W =
         * round4(cap+1) (a dword multiple, so align 4 holds). Zero-init .bss
         * gives byte 0 = 0 = an empty string. */
        int strcap = (vd->as.vardecl.vtype == AST_TY_STRING)
                   ? vd->as.vardecl.strcap : 0;
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
            if (strcap > 0) {
                fprintf(o, ": resb %d\n", ((strcap + 4) / 4) * 4);
            } else if (vd->as.vardecl.is_array) {
                long count = vd->as.vardecl.hi - vd->as.vardecl.lo + 1;
                fprintf(o, ": resd %ld\n", count * elem_words);
            } else {
                fprintf(o, ": resd %d\n", elem_words);
            }
        }
    }
    fprintf(o, "\n");
}

/*
 * ============================================================================
 * B7 (beads initech-39k2) -- the fixed __str_* intrinsic prelude (DEC-05: NOT
 * the RTL; start.asm is untouched). Emitted ONCE into .text, ONLY when the
 * program uses strings, in a fixed order with fixed internal labels (the
 * start.asm .next/.done local-label style; each helper's `.xxx` labels attach
 * to its own non-local __str_* label, so they never collide with each other or
 * with pas_main's `.L*` labels). 386-safe only (movzx / branch-min / rep
 * movsb/cmpsb; NO cmov). Every helper GUARDS len=0, returns via `ret`, clobbers
 * only eax/ecx/edx/esi/edi, and preserves ebx/ebp/esp (so a temp address in a
 * caller's register -- always recomputed via lea from ebp anyway -- survives).
 * The ABI is documented in this file's B7 header block.
 * ============================================================================
 */
static void emit_str_intrinsics(FILE *o)
{
    /* __str_assign: edi=&dst, esi=&src, ecx=cap(dst). n=min(len src, cap);
     * copy n content bytes; dst[0]=n. Silent truncation. */
    fprintf(o,
        "__str_assign:\n"
        "    movzx eax, byte [esi]\n"      /* len(src) */
        "    cmp eax, ecx\n"
        "    jbe .len_ok\n"
        "    mov eax, ecx\n"               /* clamp to cap */
        ".len_ok:\n"
        "    mov [edi], al\n"              /* dst[0] = n */
        "    mov ecx, eax\n"              /* count = n */
        "    inc esi\n"
        "    inc edi\n"
        "    rep movsb\n"                  /* copy n bytes (guards n==0) */
        "    ret\n");

    /* __str_concat: edi=&dst, esi=&src. Append src content to dst, clamping the
     * new length at 255 (ShortString intermediate cap). */
    fprintf(o,
        "__str_concat:\n"
        "    movzx eax, byte [edi]\n"      /* dstlen */
        "    movzx ecx, byte [esi]\n"      /* srclen */
        "    mov edx, 255\n"
        "    sub edx, eax\n"               /* navail = 255 - dstlen */
        "    cmp ecx, edx\n"
        "    jbe .n_ok\n"
        "    mov ecx, edx\n"               /* n = min(srclen, navail) */
        ".n_ok:\n"
        "    lea edx, [edi+eax+1]\n"       /* dest = dst + 1 + dstlen */
        "    add eax, ecx\n"               /* new dstlen */
        "    mov [edi], al\n"              /* store new length */
        "    inc esi\n"                    /* src content */
        "    mov edi, edx\n"               /* dest start */
        "    rep movsb\n"                  /* copy n bytes (guards n==0) */
        "    ret\n");

    /* __str_cmp: esi=&a, edi=&b -> eax = -1/0/+1. Unsigned bytewise over the
     * min-length prefix; a prefix-equal pair decides on length (shorter < ). */
    fprintf(o,
        "__str_cmp:\n"
        "    movzx eax, byte [esi]\n"      /* lenA */
        "    movzx edx, byte [edi]\n"      /* lenB */
        "    mov ecx, eax\n"               /* ecx = min(lenA,lenB) */
        "    cmp edx, ecx\n"
        "    jae .have_min\n"
        "    mov ecx, edx\n"
        ".have_min:\n"
        "    push eax\n"                   /* save lenA */
        "    push edx\n"                   /* save lenB */
        "    inc esi\n"
        "    inc edi\n"
        ".cmp_loop:\n"
        "    test ecx, ecx\n"
        "    jz .prefix_equal\n"
        "    mov al, [esi]\n"
        "    mov dl, [edi]\n"
        "    cmp al, dl\n"
        "    jb .a_less\n"
        "    ja .a_greater\n"
        "    inc esi\n"
        "    inc edi\n"
        "    dec ecx\n"
        "    jmp .cmp_loop\n"
        ".a_less:\n"
        "    add esp, 8\n"                 /* discard saved lengths */
        "    mov eax, -1\n"
        "    ret\n"
        ".a_greater:\n"
        "    add esp, 8\n"
        "    mov eax, 1\n"
        "    ret\n"
        ".prefix_equal:\n"
        "    pop edx\n"                    /* lenB */
        "    pop eax\n");                  /* lenA */
#ifdef SEED_MUT_CODEGEN_STR_CMP_NOLEN
    /* MUTATION HOOK (Rule 6; beads initech-39k2, D10 (ii)): drop the length
     * tiebreak, so a prefix-equal pair is wrongly EQUAL ('ab' = 'abc' TRUE).
     * test-seed-string-mutant asserts string.pas's EQF tag goes RED. */
    fprintf(o,
        "    mov eax, 0\n"
        "    ret\n");
#else
    fprintf(o,
        "    cmp eax, edx\n"
        "    jb .len_less\n"
        "    ja .len_greater\n"
        "    mov eax, 0\n"
        "    ret\n"
        ".len_less:\n"
        "    mov eax, -1\n"
        "    ret\n"
        ".len_greater:\n"
        "    mov eax, 1\n"
        "    ret\n");
#endif

    /* __str_write: esi=&s. Loop the content bytes through serial_putc (never
     * serial_puts -- a ShortString may contain 0x00). serial_putc clobbers
     * only DX, so esi/ecx survive the call. */
    fprintf(o,
        "__str_write:\n"
        "    movzx ecx, byte [esi]\n"      /* len */
        "    inc esi\n"                    /* content */
        ".w_loop:\n"
        "    test ecx, ecx\n"
        "    jz .w_done\n"
        "    mov al, [esi]\n"
        "    call serial_putc\n"
        "    inc esi\n"
        "    dec ecx\n"
        "    jmp .w_loop\n"
        ".w_done:\n"
        "    ret\n");
}

int codegen_emit(AstNode *program, FILE *out)
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
    /* B7 (beads initech-39k2). */
    cg.strlit_count = 0;
    cg.uses_strings = program->as.program.uses_strings;
    cg.cur_frame_slots = 0;

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

    /* B7 (beads initech-39k2): the __str_* intrinsic prelude, emitted ONCE
     * before any routine, ONLY when the program uses strings -- a stringless
     * program's .text is byte-identical to pre-B7 (the repro byte-identity
     * guard). */
    if (cg.uses_strings)
        emit_str_intrinsics(out);

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

    /* B7 (beads initech-39k2): pas_main's main-body string TEMPORARIES live in
     * its own frame (globals stay in .bss; only temps are frame-resident).
     * frame_slots is 0 for pas_main, so the temp region starts right below
     * ebp. A stringless program plans 0 temps -> no `sub esp` -> byte-identical
     * to pre-B7. `leave` reclaims the region on return. */
    int main_temps = cg.uses_strings
                   ? plan_string_temps(program->as.program.block) : 0;
    cg.cur_frame_slots = 0;
    if (main_temps > 0)
        fprintf(out, "    sub esp, %d\n", 4 * main_temps * CG_STR_TEMP_DWORDS);

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
