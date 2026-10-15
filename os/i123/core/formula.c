/*
 * os/i123/core/formula.c -- the formula COMPILER (typed text -> the .WK1
 * RPN bytecode 123.EXE writes), DECOMPILER (bytecode -> the text the control
 * panel shows) and the structural CHECK of a code stream (THE ARTIFACT).
 *
 * bead: initech-94ah (I123 P2). See i123.h for the contract and
 * spec/i123/wk1_formula.h for the token table and its citations.
 *
 * Ref (Law 1):
 *   ../lotus123-decomp/specs/formulas/wk1-formula-bytecode.md (the token
 *     widths, the reference words, the call convention, PAREN, UPLUS);
 *   ../lotus123-decomp/mint/{mint01,MINT03,MINT04,MINT05}.steps with
 *     goldens/minted/NAME.WK1 (typed text and the bytes 123.EXE compiled it
 *     to -- graded by test-i123-formula part A);
 *   R2.2 Reference Manual (OCR, PDF pages): p. 1-29 (PDF 45) formula entry
 *     rules (begins with a number or + - @ . ( $ #; a cell address first
 *     needs + - ( or $; at most 240 characters; no spaces outside literal
 *     strings); p. 1-30 (PDF 46) precedence; p. 1-24 (PDF 40) a range is
 *     two corners separated by one or two periods; p. 1-31..1-33 (PDF 47-
 *     49) $ absolute / mixed references; p. 1-41 (PDF 57) linking formulas
 *     +<<FILE>>ref.
 *
 * COMPILATION RULES and where each comes from:
 *   - Postfix in source order: operands, then the operator; arguments left
 *     to right, then the function token ([C] Sec 5).
 *   - Every typed parenthesised group is followed by PAREN (token 4): 253
 *     uses in the goldens, each after a closed group ([C] Sec 4). A
 *     function's own argument parentheses are not groups.
 *   - The FIRST character of a formula, when it is '+', is the entry
 *     marker the manual requires before a cell address and compiles to
 *     nothing: MINT01 "+B1*2" -> VAR INT(2) * ; MINT04 "+A1+1" -> VAR INT +.
 *     A '+' anywhere else in operand position is UPLUS (token 23): the
 *     37 shipped uses are all inside arguments or groups, e.g. SAMP7X
 *     "(+$D$20)" -> VAR UPLUS PAREN.
 *   - A '-' in operand position is NEG (token 8): MINT03 "@ROUND(-2.5,0)"
 *     -> CONST(2.5) NEG INT(0) @ROUND. Literals are unsigned.
 *   - An integral literal 0..32767 compiles to INT (token 5), any other to
 *     CONST (token 0): MINT01/03/05 (INT(2), INT(100), INT(53); CONST 2.5,
 *     0.1, 0.2, 1e+99). Over the 44 goldens every INT is 0..20000 and
 *     every CONST non-integral or >= 1e99. The rule for a typed "2.0" or
 *     "1E2" is [inferred] (value-based).
 *   - A lone cell in a RANGE-parameter position (spec rmask) compiles to
 *     RANGE(a..a) ([C] Sec 2.1, @S in SAMP6X); in a list function (@SUM ..)
 *     it stays VAR [inferred: the list takes values or ranges].
 *   - Relative reference words are offsets from the formula's own cell,
 *     written as 14-bit two's complement under bit 15 (MINT04 col -1 =
 *     0xBFFF, row +0 = 0x8000). The goldens also hold column offsets
 *     written modulo 256 (CAMBRIDG 0x80FC = +252 for -4, 298 cases, [C]
 *     Sec 2.1): the compiler writes the in-range form; the decompiler reads
 *     both.
 *
 * Freestanding; ASCII-clean (Rule 12); deterministic (Rule 11).
 */
#include "i123.h"
#include "i123/wk1_format.h"
#include "i123/wk1_formula.h"

const wkf_token_t i123_tokens[WKF_NTOKENS] = WKF_TOKENS_INIT;

/* ---- small helpers -------------------------------------------------------- */
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static int up(int ch) { return (ch >= 'a' && ch <= 'z') ? ch - 'a' + 'A' : ch; }
static int is_alpha(int ch) { ch = up(ch); return ch >= 'A' && ch <= 'Z'; }
static int is_digit(int ch) { return ch >= '0' && ch <= '9'; }

/* Bytes a token occupies at code[i] (token included), 0 when malformed. */
uint32_t i123_token_size(const uint8_t *code, uint32_t len, uint32_t i)
{
    uint8_t t;
    if (i >= len) return 0;
    t = code[i];
    if (t >= WKF_NTOKENS || i123_tokens[t].cls == WKF_K_NONE) return 0;
    switch (t) {
    case WKF_CONST:  return (i + 9u <= len) ? 9u : 0u;
    case WKF_VAR:    return (i + 5u <= len) ? 5u : 0u;
    case WKF_RANGE:  return (i + 9u <= len) ? 9u : 0u;
    case WKF_INT:    return (i + 3u <= len) ? 3u : 0u;
    case WKF_STRING: {
        uint32_t k = i + 1u;
        while (k < len && code[k] != 0u) k++;
        return (k < len) ? k + 1u - i : 0u;
    }
    default:
        if (i123_tokens[t].cls == WKF_K_COUNTED) return (i + 2u <= len) ? 2u : 0u;
        return 1u;
    }
}

/* ---- reference words -------------------------------------------------------- */

/* Resolve one reference word against the formula cell coordinate `home`.
 * -1 is the ERR reference: a word with bit 14 set, which real 1-2-3 R2.2
 * writes as 0xFFFF in both words of a corner /Move overwrote (MOVE01B.WK1
 * E1 01 ffff ffff; wk1_formula.h WKF_REF_ERR, bead initech-nn46). A
 * relative column wraps modulo 256 ([C] Sec 2.1; the 14-bit ffbf and the
 * 8-bit 80ff spell the same -1) and a relative row modulo 8192 (WRAP01C.WK1
 * B1 ffbf ffbf at row 0 is +A8192+1, value 1, not ERR). An absolute row
 * outside the sheet is ERR [inferred: never in a golden]. */
int32_t i123_ref_resolve(uint16_t word, uint16_t home, int is_col)
{
    int32_t v = (int32_t)(word & WKF_REF_MASK);
#ifndef I123_MUT_ERR_LOAD
    /* MUTANT (Rule 6; test-i123-copy-move-corpus-mutant): without this a
     * loaded 0xFFFF reads as relative -1,-1. NEVER in a real build. */
    if (word & WKF_REF_ERRBIT) return -1;
#endif
    if (word & WKF_REF_REL) {
        if (v & (int32_t)WKF_REF_SIGN) v -= (int32_t)(WKF_REF_MASK + 1u);
        v += (int32_t)home;
#ifdef I123_MUT_ROW_ERR
        /* MUTANT (Rule 6; test-i123-copy-move-corpus-mutant): the manual-
         * derived "row off the sheet is ERR" rule WRAP01C refutes. */
        if (!is_col && (v < 0 || v >= (int32_t)I123_MAX_ROWS)) return -1;
#endif
    } else if (!is_col && v >= (int32_t)I123_MAX_ROWS) {
        return -1;
    }
    return (int32_t)((uint32_t)v & (is_col ? 0xFFu : I123_MAX_ROWS - 1u));
}

static uint16_t ref_word(int32_t target, uint16_t home, int absolute)
{
    if (absolute) return (uint16_t)target;
    return (uint16_t)(WKF_REF_REL | ((uint32_t)(target - (int32_t)home) & WKF_REF_MASK));
}

/* ---- the structural check ---------------------------------------------------- */

/* A code stream is well formed when every token is known and complete, the
 * stack never underflows, a function's arity matches its table row, the
 * stream ends in RETURN at exactly `len`, and one value is left. This is the
 * stack simulation [C] Sec 5 ran over 1090/1090 formulas. RANGE values may
 * only be consumed by a function (never by an operator), and none may be
 * the final value [inferred: 0 counterexamples in the goldens]. */
int i123_formula_check(const uint8_t *code, uint32_t len)
{
    uint8_t isr[256];          /* per stack slot: 1 = a RANGE value */
    uint32_t sp = 0, i = 0;
    while (i < len) {
        uint32_t n = i123_token_size(code, len, i);
        uint8_t t = code[i];
        const wkf_token_t *tk;
        if (n == 0u) return I123_E_FORMAT;
        tk = &i123_tokens[t];
        if (t == WKF_RETURN) {
            if (i + 1u != len || sp != 1u || isr[0]) return I123_E_FORMAT;
            return I123_OK;
        }
        switch (tk->cls) {
        case WKF_K_OPERAND:
            if (sp >= sizeof isr) return I123_E_FORMAT;
            isr[sp++] = (uint8_t)(t == WKF_RANGE);
            break;
        case WKF_K_CTRL:      /* PAREN */
            if (sp == 0u) return I123_E_FORMAT;
            break;
        case WKF_K_UNARY:
            if (sp < 1u || isr[sp - 1u]) return I123_E_FORMAT;
            break;
        case WKF_K_BINARY:
            if (sp < 2u || isr[sp - 1u] || isr[sp - 2u]) return I123_E_FORMAT;
            sp--;
            break;
        case WKF_K_FIXED:
        case WKF_K_COUNTED: {
            uint32_t a = (tk->cls == WKF_K_COUNTED) ? code[i + 1u] : tk->arity;
            if (tk->cls == WKF_K_COUNTED && a < tk->arity) return I123_E_FORMAT;
            if (sp < a) return I123_E_FORMAT;
            sp -= a;
            if (sp >= sizeof isr) return I123_E_FORMAT;
            isr[sp++] = 0;
            break;
        }
        default:
            return I123_E_FORMAT;
        }
        i += n;
    }
    return I123_E_FORMAT;   /* no RETURN */
}

/* ---- the compiler -------------------------------------------------------------- */
typedef struct {
    const char *s;
    const i123_sheet_t *sh;
    uint32_t    n, p;
    uint16_t    col, row;
    uint8_t    *out;
    uint32_t    cap, len;
    int         err;
    int         depth;
} cc_t;

#define CC_DEPTH_MAX 48   /* nesting guard: the tenant runs on a shared stack */

static void emit(cc_t *c, uint8_t b)
{
    if (c->len >= c->cap) { c->err = I123_E_SPACE; return; }
    c->out[c->len++] = b;
}

static void emit16(cc_t *c, uint16_t v)
{
    emit(c, (uint8_t)(v & 0xFFu));
    emit(c, (uint8_t)(v >> 8));
}

static int peek(const cc_t *c, uint32_t k)
{
    return (c->p + k < c->n) ? (int)(unsigned char)c->s[c->p + k] : 0;
}

/* Parse one cell address at c->p: [$]letters[$]digits. On success fills the
 * coordinates and absolute flags and advances; else leaves p alone. */
static int parse_cell(cc_t *c, int32_t *col, int32_t *row, int *acol, int *arow)
{
    uint32_t p = c->p;
    int32_t cv = 0, rv = 0;
    int nl = 0, nd = 0;
    *acol = *arow = 0;
    if (p < c->n && c->s[p] == '$') { *acol = 1; p++; }
    while (p < c->n && is_alpha(c->s[p]) && nl < 3) {
        cv = cv * 26 + (up(c->s[p]) - 'A' + 1);
        p++; nl++;
    }
    if (nl == 0 || nl > 2) return 0;
    if (p < c->n && c->s[p] == '$') { *arow = 1; p++; }
    while (p < c->n && is_digit(c->s[p])) {
        if (rv < 100000) rv = rv * 10 + (c->s[p] - '0');
        p++; nd++;
    }
    if (nd == 0) return 0;
    if (p < c->n && (is_alpha(c->s[p]) || c->s[p] == '_')) return 0;   /* a name, not an address */
    cv -= 1;
    if (cv < 0 || cv >= (int32_t)I123_MAX_COLS || rv < 1 || rv > (int32_t)I123_MAX_ROWS)
        return 0;
    *col = cv;
    *row = rv - 1;
    c->p = p;
    return 1;
}

/* A cell or range reference at c->p. Returns 0 none, 1 cell, 2 range. */
static int parse_ref(cc_t *c, int32_t v[4], int a[4])
{
    uint32_t save = c->p;
    if (!parse_cell(c, &v[0], &v[1], &a[0], &a[1])) {
        /* Manual p. 3-3 (PDF 71): NAME is relative, $NAME absolute;
         * mixed references cannot be used with names. WK1 stores the
         * resolved coordinates, not a name token (corpus bytecode Sec 2). */
        uint32_t p = save;
        int absolute = 0;
        i123_rect_t rg;
        if (peek(c, 0) == '$') { absolute = 1; p++; }
        uint32_t st = p;
        while (p < c->n && (is_alpha(c->s[p]) || is_digit(c->s[p]) ||
                            c->s[p] == '_' || c->s[p] == '.')) p++;
        if (!c->sh || p == st || i123_name_find(c->sh, c->s+st, p-st, &rg)) return 0;
        c->p = p;
        v[0] = rg.c0; v[1] = rg.r0; v[2] = rg.c1; v[3] = rg.r1;
        for (int i = 0; i < 4; i++) a[i] = absolute;
        return rg.c0 == rg.c1 && rg.r0 == rg.r1 ? 1 : 2;
    }
    if (peek(c, 0) == '.') {
        uint32_t s2 = c->p;
        c->p++;
        if (peek(c, 0) == '.') c->p++;
        if (parse_cell(c, &v[2], &v[3], &a[2], &a[3])) return 2;
        c->p = s2;
        (void)save;
        return 0;   /* "A1." followed by no address: not a reference */
    }
    return 1;
}

static void emit_ref(cc_t *c, int kind, const int32_t v[4], const int a[4])
{
    emit(c, kind == 2 ? (uint8_t)WKF_RANGE : (uint8_t)WKF_VAR);
    emit16(c, ref_word(v[0], c->col, a[0]));
    emit16(c, ref_word(v[1], c->row, a[1]));
    if (kind == 2) {
        emit16(c, ref_word(v[2], c->col, a[2]));
        emit16(c, ref_word(v[3], c->row, a[3]));
    }
}

static void parse_expr(cc_t *c, uint32_t maxp);

/* A function argument. A bare reference (followed by ',' or ')') is VAR or
 * RANGE as written, and RANGE(a..a) in a RANGE-parameter slot. */
static void parse_arg(cc_t *c, int range_slot)
{
    uint32_t save = c->p;
    int32_t v[4];
    int a[4];
    int k = parse_ref(c, v, a);
    if (k != 0 && (peek(c, 0) == ',' || peek(c, 0) == ';' || peek(c, 0) == ')')) {
        if (k == 1 && range_slot) {
            v[2] = v[0]; v[3] = v[1]; a[2] = a[0]; a[3] = a[1];
            k = 2;
        }
        emit_ref(c, k, v, a);
        return;
    }
    c->p = save;
    if (range_slot) { c->err = I123_E_SYNTAX; return; }   /* needs a range */
    parse_expr(c, WKF_PREC_MAX);
}

static int fn_lookup(const char *s, uint32_t n)
{
    for (uint32_t t = WKF_FIRST_FN; t <= WKF_LAST_FN; t++) {
        const char *nm = i123_tokens[t].name;
        uint32_t k = 0;
        if (nm == (const char *)0) continue;
        while (k < n && nm[k] != '\0' && up(s[k]) == nm[k]) k++;
        if (k == n && nm[k] == '\0') return (int)t;
    }
    return -1;
}

static void parse_function(cc_t *c)
{
    uint32_t st = c->p, n;
    int t;
    const wkf_token_t *tk;
    uint32_t argc = 0;

    c->p++;                                  /* '@' */
    if (peek(c, 0) == '@') c->p++;           /* @@ */
    while (is_alpha(peek(c, 0)) || is_digit(peek(c, 0))) c->p++;
    n = c->p - st;
    t = fn_lookup(c->s + st, n);
    if (t < 0) { c->err = I123_E_SYNTAX; return; }   /* "@LEN(" is refused (GAPS L-1) */
    tk = &i123_tokens[t];
    if (peek(c, 0) == '(') {
        c->p++;
        for (;;) {
            if (argc >= 255u) { c->err = I123_E_SYNTAX; return; }
            parse_arg(c, argc < 8u && ((tk->rmask >> argc) & 1u));
            if (c->err) return;
            argc++;
            /* commas and semicolons separate arguments (manual p. 13-3,
             * PDF 291: "By default, commas and semicolons are the valid
             * argument separators") */
            if (peek(c, 0) == ',' || peek(c, 0) == ';') { c->p++; continue; }
            if (peek(c, 0) == ')') { c->p++; break; }
            c->err = I123_E_SYNTAX;
            return;
        }
    }
    if (tk->cls == WKF_K_FIXED && argc != tk->arity) { c->err = I123_E_SYNTAX; return; }
    if (tk->cls == WKF_K_COUNTED && argc < tk->arity) { c->err = I123_E_SYNTAX; return; }
    emit(c, (uint8_t)t);
    if (tk->cls == WKF_K_COUNTED) emit(c, (uint8_t)argc);
}

/* A numeric literal: digits [. digits] [E [+-] digits], or . digits. */
static void parse_number(cc_t *c)
{
    uint32_t st = c->p;
    double x;
    while (is_digit(peek(c, 0))) c->p++;
    if (peek(c, 0) == '.') { c->p++; while (is_digit(peek(c, 0))) c->p++; }
    if (up(peek(c, 0)) == 'E' &&
        (is_digit(peek(c, 1)) || ((peek(c, 1) == '+' || peek(c, 1) == '-') && is_digit(peek(c, 2))))) {
        c->p += 2;
        while (is_digit(peek(c, 0))) c->p++;
    }
    if (peek(c, 0) == '%') c->p++;   /* 15.5% (manual p. 13-8): CONST 0.155 [inferred bytes] */
    if (i123_parse_value(c->s + st, c->p - st, &x) != I123_OK) { c->err = I123_E_SYNTAX; return; }
    if (x >= 0.0 && x <= 32767.0 && (double)(int32_t)x == x) {
        emit(c, (uint8_t)WKF_INT);
        emit16(c, (uint16_t)(int32_t)x);
    } else {
        union { double d; uint8_t b[8]; } u;
        u.d = x;
        emit(c, (uint8_t)WKF_CONST);
        for (int i = 0; i < 8; i++) emit(c, u.b[i]);
    }
}

/* A string literal "...": no escape for '"' exists [M p. 1-29]. */
static void parse_string(cc_t *c)
{
    c->p++;
    emit(c, (uint8_t)WKF_STRING);
    while (c->p < c->n && c->s[c->p] != '"') {
        if (c->s[c->p] == '\0') { c->err = I123_E_SYNTAX; return; }
        emit(c, (uint8_t)c->s[c->p]);
        c->p++;
    }
    if (c->p >= c->n) { c->err = I123_E_SYNTAX; return; }
    c->p++;
    emit(c, 0u);
}

/* A link "<<FILE>>ref": the STRING of its text then the @@ token (all 101
 * link formulas in the goldens; manual p. 1-41). */
static void parse_link(cc_t *c)
{
    uint32_t st = c->p, k = c->p + 2u;
    while (k + 1u < c->n && !(c->s[k] == '>' && c->s[k + 1u] == '>')) k++;
    if (k + 1u >= c->n) { c->err = I123_E_SYNTAX; return; }
    k += 2u;
    while (k < c->n && (is_alpha(c->s[k]) || is_digit(c->s[k]) || c->s[k] == '_' ||
                        c->s[k] == '$' || c->s[k] == '.'))
        k++;
    if (k == st) { c->err = I123_E_SYNTAX; return; }
    emit(c, (uint8_t)WKF_STRING);
    for (uint32_t i = st; i < k; i++) emit(c, (uint8_t)c->s[i]);
    emit(c, 0u);
    emit(c, (uint8_t)WKF_LINK);
    c->p = k;
}

static void parse_operand(cc_t *c)
{
    int ch = peek(c, 0);
    int32_t v[4];
    int a[4];

    if (c->err) return;
    if (++c->depth > CC_DEPTH_MAX) { c->err = I123_E_SYNTAX; return; }
    if (ch == '-' || ch == '+') {
        int drop = (ch == '+' && c->p == 0u);   /* the entry marker (MINT01) */
        c->p++;
        parse_expr(c, WKF_PREC_SIGN - 1u);
        if (!drop) emit(c, ch == '-' ? (uint8_t)WKF_NEG : (uint8_t)WKF_UPLUS);
    } else if (ch == '#') {
        static const char NOT[] = "#NOT#";
        for (int i = 0; i < 5; i++)
            if (up(peek(c, (uint32_t)i)) != NOT[i]) { c->err = I123_E_SYNTAX; c->depth--; return; }
        c->p += 5u;
        parse_expr(c, WKF_PREC_NOT - 1u);
        emit(c, (uint8_t)WKF_NOT);
    } else if (ch == '(') {
        c->p++;
        parse_expr(c, WKF_PREC_MAX);
        if (!c->err && peek(c, 0) != ')') c->err = I123_E_SYNTAX;
        c->p++;
        emit(c, (uint8_t)WKF_PAREN);
    } else if (ch == '@') {
        parse_function(c);
    } else if (ch == '"') {
        parse_string(c);
    } else if (ch == '<' && peek(c, 1) == '<') {
        parse_link(c);
    } else if (is_digit(ch) || ch == '.') {
        parse_number(c);
    } else {
        int k = parse_ref(c, v, a);
        if (k != 1) c->err = I123_E_SYNTAX;   /* a range is not a value */
        else emit_ref(c, 1, v, a);
    }
    c->depth--;
}

/* The binary operator at c->p: its token and text length, or 0. */
static int binop(const cc_t *c, uint32_t *len)
{
    int ch = peek(c, 0), n1 = peek(c, 1);
    *len = 1;
    switch (ch) {
    case '+': return WKF_ADD;
    case '-': return WKF_SUB;
    case '*': return WKF_MUL;
    case '/': return WKF_DIV;
    case '^': return WKF_POW;
    case '&': return WKF_CAT;
    case '=': return WKF_EQ;
    case '<':
        if (n1 == '>') { *len = 2; return WKF_NE; }
        if (n1 == '=') { *len = 2; return WKF_LE; }
        return WKF_LT;
    case '>':
        if (n1 == '=') { *len = 2; return WKF_GE; }
        return WKF_GT;
    case '#': {
        static const char *const W[2] = { "#AND#", "#OR#" };
        for (int w = 0; w < 2; w++) {
            uint32_t k = 0;
            while (W[w][k] && up(peek(c, k)) == W[w][k]) k++;
            if (W[w][k] == '\0') { *len = k; return w == 0 ? WKF_AND : WKF_OR; }
        }
        return 0;
    }
    default:
        return 0;
    }
}

/* Precedence climbing over the manual's table: an operator joins when its
 * precedence number is <= maxp; equal numbers associate left to right (the
 * right operand admits only strictly tighter operators). */
static void parse_expr(cc_t *c, uint32_t maxp)
{
    parse_operand(c);
    while (!c->err) {
        uint32_t len;
        int t = binop(c, &len);
        uint32_t p;
        if (t == 0) return;
        p = i123_tokens[t].prec;
#ifdef I123_MUT_PREC_FLAT
        /* MUTANT (Rule 6; test-i123-formula-mutant): every binary operator
         * has the same precedence -- "+A1+B1*2" compiles as (A1+B1)*2, so
         * the typed MINT/TEXT legs and the round trip go RED. NEVER real. */
        p = WKF_PREC_ADD;
#endif
        if (p > maxp) return;
        c->p += len;
        parse_expr(c, p - 1u);
        emit(c, (uint8_t)t);
    }
}

int i123_formula_compile_named(const i123_sheet_t *sh, const char *text, uint32_t len,
                               uint16_t col, uint16_t row, uint8_t *code,
                               uint32_t cap, uint32_t *code_len)
{
    cc_t c;
    if (text == (const char *)0 || code == (uint8_t *)0 || code_len == (uint32_t *)0)
        return I123_E_ARG;
    if (len == 0u || len > WKF_TEXT_MAX) return I123_E_SYNTAX;
    for (uint32_t i = 0, q = 0; i < len; i++) {        /* no spaces outside "..." */
        if (text[i] == '"') q = !q;
        else if (text[i] == ' ' && !q) return I123_E_SYNTAX;
    }
    c.s = text; c.sh = sh; c.n = len; c.p = 0; c.col = col; c.row = row;
    c.out = code; c.cap = cap; c.len = 0; c.err = 0; c.depth = 0;
    parse_expr(&c, WKF_PREC_MAX);
    if (!c.err && c.p != c.n) c.err = I123_E_SYNTAX;
    emit(&c, (uint8_t)WKF_RETURN);
    if (c.err) return c.err;
    if (i123_formula_check(code, c.len) != I123_OK) return I123_E_SYNTAX;
    *code_len = c.len;
    return I123_OK;
}

int i123_formula_compile(const char *text, uint32_t len, uint16_t col, uint16_t row,
                         uint8_t *code, uint32_t cap, uint32_t *code_len)
{
    return i123_formula_compile_named((const i123_sheet_t *)0, text, len, col, row,
                                      code, cap, code_len);
}

/* ---- the decompiler ------------------------------------------------------------ */

/* The expression tree of a code stream: one node per value-producing token
 * (PAREN wraps its operand), children as first/next lists. */
#define DN_MAX 160u
typedef struct {
    uint32_t at;          /* code offset of the token */
    int16_t  kid, next;   /* first child, next sibling (-1 none) */
} dnode_t;

typedef struct {
    const uint8_t *code;
    uint32_t       len;
    uint16_t       col, row;
    dnode_t        nd[DN_MAX];
    uint32_t       nn;
    char          *out;
    uint32_t       cap, n;
    int            err, depth;
} dc_t;

static void put(dc_t *d, char ch)
{
    if (d->n + 1u >= d->cap) { d->err = I123_E_SPACE; return; }
    d->out[d->n++] = ch;
}

static void puts_(dc_t *d, const char *s) { while (*s) put(d, *s++); }

static void put_uint(dc_t *d, uint32_t v)
{
    char t[12];
    int k = 0;
    do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v != 0u);
    while (k > 0) put(d, t[--k]);
}

/* A reference operand as text: a VAR, or a RANGE's two corners joined by
 * "..". The ERR reference (any word with bit 14 set; real 1-2-3 writes
 * ffff ffff) shows as ERR, and a RANGE with an ERR corner as ONE ERR
 * (bead initech-nn46; MOVE01B.WK1 E1 01 ffff ffff "+ERR+1", E2 "+ERR", H1
 * 02 ffff ffff fcbf 0180 "@SUM(ERR)": MOVE01.shots/04_show_E1/E2/H1). */
static void put_ref(dc_t *d, const uint8_t *p, int range)
{
    uint32_t n = range ? 8u : 4u;
    for (uint32_t i = 0; i < n; i += 2u)
        if (rd16(p + i) & WKF_REF_ERRBIT) { puts_(d, "ERR"); return; }
    for (uint32_t i = 0; i < n; i += 4u) {
        char nm[4];
        uint16_t cw = rd16(p + i), rw = rd16(p + i + 2u);
        int32_t c = i123_ref_resolve(cw, d->col, 1);
        int32_t r = i123_ref_resolve(rw, d->row, 0);
        uint32_t k;
        if (r < 0) { d->err = I123_E_FORMAT; return; }   /* absolute row off the sheet */
        if (i) { put(d, '.'); put(d, '.'); }
        if (!(cw & WKF_REF_REL)) put(d, '$');
        k = i123_col_name((uint16_t)c, nm);
        for (uint32_t q = 0; q < k; q++) put(d, nm[q]);
        if (!(rw & WKF_REF_REL)) put(d, '$');
        put_uint(d, (uint32_t)r + 1u);
    }
}

/* The shortest decimal that reads back (through i123_parse_value, the same
 * reader the compiler uses) as exactly x >= 0. Never a timestamp-dependent
 * or libc path (Rule 11). */
void i123_number_text(double x, char *out, uint32_t cap)
{
    char best[40];
    best[0] = '\0';
    if (cap == 0u) return;
    for (uint32_t p = 1; p <= 17u; p++) {
        char s[40];
        double back;
        uint32_t n = i123_sig_text(x, p, s, sizeof s);
        if (n == 0u) break;
        if (i123_parse_value(s, n, &back) == I123_OK && back == x) {
            for (uint32_t i = 0; i <= n; i++) best[i] = s[i];
            break;
        }
    }
    for (uint32_t i = 0; i + 1u < cap; i++) { out[i] = best[i]; if (!best[i]) return; }
    out[cap - 1u] = '\0';
}

static int node_prec(const dc_t *d, int k)
{
    uint8_t t = d->code[d->nd[k].at];
    const wkf_token_t *tk = &i123_tokens[t];
    if (tk->cls == WKF_K_UNARY || tk->cls == WKF_K_BINARY) return tk->prec;
    return 0;   /* operands, functions, groups bind tightest */
}

static void print_node(dc_t *d, int k);

static void print_child(dc_t *d, int k, int need_le, int need_lt)
{
    int p = node_prec(d, k);
    int paren = (need_le && p > need_le) || (need_lt && p >= need_lt);
    if (paren) put(d, '(');
    print_node(d, k);
    if (paren) put(d, ')');
}

static void print_node(dc_t *d, int k)
{
    uint32_t at = d->nd[k].at;
    uint8_t t = d->code[at];
    const wkf_token_t *tk = &i123_tokens[t];
    int kid = d->nd[k].kid;

    if (d->err) return;
    if (++d->depth > 64) { d->err = I123_E_FORMAT; return; }
    switch (t) {
    case WKF_CONST: {
        union { double v; uint8_t b[8]; } u;
        char s[40];
        for (int i = 0; i < 8; i++) u.b[i] = d->code[at + 1u + (uint32_t)i];
        if (!(u.v >= 0.0) || u.v != u.v) { d->err = I123_E_FORMAT; break; }
        i123_number_text(u.v, s, sizeof s);
        puts_(d, s);
        break;
    }
    case WKF_INT: {
        int16_t v = (int16_t)rd16(d->code + at + 1u);
        if (v < 0) { put(d, '-'); put_uint(d, (uint32_t)(-(int32_t)v)); }
        else put_uint(d, (uint32_t)v);
        break;
    }
    case WKF_STRING: {
        const uint8_t *s = d->code + at + 1u;
        put(d, '"');
        while (*s) put(d, (char)*s++);
        put(d, '"');
        break;
    }
    case WKF_VAR:
    case WKF_RANGE:
        put_ref(d, d->code + at + 1u, t == WKF_RANGE);
        break;
    case WKF_PAREN:
        put(d, '(');
        print_node(d, kid);
        put(d, ')');
        break;
    default:
        if (tk->cls == WKF_K_UNARY) {
            puts_(d, tk->name);
            /* a prefix operand admits only tighter operators */
            print_child(d, kid, 0, tk->prec);
        } else if (tk->cls == WKF_K_BINARY) {
            int rk = d->nd[kid].next;
            print_child(d, kid, tk->prec, 0);
            puts_(d, tk->name);
            /* a sign on the right needs no group of its own ("2^-3",
             * "A1*-B1"); #NOT# does when its operand would swallow what
             * follows (its operand admits + - * / and comparisons) */
            uint8_t rt = d->code[d->nd[rk].at];
            if (rt == WKF_NEG || rt == WKF_UPLUS ||
                (rt == WKF_NOT && tk->prec >= WKF_PREC_NOT)) print_node(d, rk);
            else print_child(d, rk, 0, tk->prec);
        } else if (t == WKF_LINK && kid >= 0 && d->code[d->nd[kid].at] == WKF_STRING &&
                   d->code[d->nd[kid].at + 1u] == '<' && d->code[d->nd[kid].at + 2u] == '<') {
            const uint8_t *s = d->code + d->nd[kid].at + 1u;   /* +<<FILE>>ref */
            while (*s) put(d, (char)*s++);
        } else {
            puts_(d, tk->name);
            if (kid >= 0) {
                put(d, '(');
                for (int a = kid; a >= 0; a = d->nd[a].next) {
                    print_node(d, a);
                    if (d->nd[a].next >= 0) put(d, ',');
                }
                put(d, ')');
            }
        }
        break;
    }
    d->depth--;
}

int i123_formula_text(const uint8_t *code, uint32_t len, uint16_t col, uint16_t row,
                      char *out, uint32_t cap)
{
    dc_t d;
    int16_t st[DN_MAX];
    uint32_t sp = 0, i = 0;
    int root;

    if (code == (const uint8_t *)0 || out == (char *)0 || cap < 2u) return I123_E_ARG;
    if (i123_formula_check(code, len) != I123_OK) return I123_E_FORMAT;
    d.code = code; d.len = len; d.col = col; d.row = row; d.nn = 0;
    d.out = out; d.cap = cap; d.n = 0; d.err = 0; d.depth = 0;

    while (i < len && code[i] != WKF_RETURN) {
        uint8_t t = code[i];
        const wkf_token_t *tk = &i123_tokens[t];
        uint32_t take = 0;
        int16_t me;
        if (d.nn >= DN_MAX) return I123_E_SPACE;
        if (tk->cls == WKF_K_UNARY || t == WKF_PAREN) take = 1;
        else if (tk->cls == WKF_K_BINARY) take = 2;
        else if (tk->cls == WKF_K_FIXED) take = tk->arity;
        else if (tk->cls == WKF_K_COUNTED) take = code[i + 1u];
        me = (int16_t)d.nn++;
        d.nd[me].at = i;
        d.nd[me].kid = -1;
        d.nd[me].next = -1;
        /* pop `take` children, link them in source order */
        for (uint32_t a = 0; a < take; a++) {
            int16_t ch = st[sp - 1u - a];
            d.nd[ch].next = d.nd[me].kid;
            d.nd[me].kid = ch;
        }
        sp -= take;
        st[sp++] = me;
        i += i123_token_size(code, len, i);
    }
    root = st[0];
    {
        uint8_t t0 = code[d.nd[root].at];
        (void)t0;
    }
    print_node(&d, root);
    if (d.err) return d.err;
    out[d.n] = '\0';
    /* The entry rules (manual p. 1-29): a formula whose text would start
     * like a label (a cell address, a literal string, a link) or with the
     * '+' the compiler drops, is shown with a leading '+'. */
    {
        char f = out[0];
        if (is_alpha(f) || f == '"' || f == '<' || f == '+') {
            if (d.n + 2u > cap) return I123_E_SPACE;
            for (uint32_t k = d.n + 1u; k > 0u; k--) out[k] = out[k - 1u];
            out[0] = '+';
        }
    }
    return I123_OK;
}
