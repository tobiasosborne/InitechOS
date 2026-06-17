/*
 * os/samir/core/lex.c -- SAMIR xBase III+ expression lexer (S3.1).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib -fno-stack-protector -fno-pic -std=c11 -Wall -Wextra -Werror).
 * Only <stdint.h> and rt.h/eval.h are included. No libc, no PAL, no INT 21h.
 *
 * Target: dBASE III PLUS 1.1 ONLY.
 * (docs/plans/SAMIR-implementation-plan.md sec.2.C: III+1.1; == is a lex
 * error; IV features % != == are rejected.)
 *
 * -------------------------------------------------------------------------
 * TOKEN INVENTORY (all period-exact from HELP.DBS mined surface):
 *
 * String literals (data-types.md sec.2):
 *   'text'  "text"  [text]
 *   Single- and double-quote forms are verified.
 *   Bracket [text] form: documented as valid III+ syntax in data-types.md
 *   sec.2; GATED: nesting/escape behavior [oracle-resolves]. CONSERVATIVE
 *   CHOICE: accept [text] literally (no nesting support) -- flag GATED.
 *   Unterminated literal -> XBLE_UNTERM_STR (DBASE.MSG line 34).
 *
 * Numeric literals (data-types.md sec.3):
 *   Digits with optional single decimal point. A leading sign (- or +) is
 *   NOT part of the literal -- it is a separate unary operator token.
 *   [documented: data-types.md sec.3; consistent with dBASE source grammar
 *    where STR()/etc. use N expressions, not signed-literal tokens]
 *
 * Logical literals (data-types.md sec.5):
 *   .T. .F. -- verified.
 *   .Y. .N. -- listed in data-types.md quick reference (line 625) and
 *   section 5 as III+ logical literals; the dotted form in source is
 *   [oracle-resolves] (data-types.md Open question 5). CONSERVATIVE CHOICE:
 *   accept .Y. and .N. as aliases for .T. / .F. (GATED).
 *   Case-insensitive: .t. .and. etc. are equivalent to .T. .AND. etc.
 *   [documented: dBASE is case-insensitive for keywords/literals]
 *
 * Dotted logical operators:
 *   .AND. .OR. .NOT.
 *   The dots are mandatory; bare AND/OR/NOT are identifiers.
 *   [verified: HELP.DBS @1807-1809; expressions-and-operators.md sec.5.1]
 *   Case-insensitive.
 *
 * Disambiguation: a dot in source could start:
 *   (a) A dotted logical literal: .T. .F. .Y. .N.
 *   (b) A dotted operator: .AND. .OR. .NOT.
 *   (c) A numeric literal starting with '.': e.g. .5
 *       GATED: numeric literals with leading dot (.5 meaning 0.5) are
 *       documented as a dBASE PLUS convention; the III+ HELP surface does
 *       not document bare .N numeric forms -- period sources always show
 *       0.5 not .5 [data-types.md sec.3 examples: 98.25, 0.00, -44.00].
 *       CONSERVATIVE CHOICE: a bare dot followed by a digit is treated as
 *       a NUMERIC literal (.5 -> 0.5). This is strictly less dangerous than
 *       misidentifying it as an operator. Flagged GATED.
 *   Strategy: consume the dot, then:
 *     - If next is a digit -> numeric starting at the dot (GATED: .5 form)
 *     - If next matches T/F/Y/N (case-insensitive) and the char after that
 *       is dot -> logical literal
 *     - If next matches A/O/N... -> try .AND. .OR. .NOT. (case-insensitive)
 *     - Otherwise -> XBLE_UNKNOWN_CHAR (bare dot is not a valid III+ token)
 *
 * Identifiers:
 *   First char: ASCII letter (A-Z a-z) or underscore.
 *   Subsequent: ASCII letter, digit, or underscore.
 *   Max 10 chars [verified: HELP.DBS @FIELD NAME; data-types.md sec.9].
 *   CONSERVATIVE CHOICE: we do NOT enforce the 10-char limit at lex time --
 *   the identifier is returned with its full span; the evaluator/parser can
 *   enforce the limit or truncate. This keeps the lexer pure.
 *   Case-preserved in the token; callers fold for lookup.
 *
 * Operators -- two-char first, then single-char:
 *   **  (STARSTAR, exponentiation; synonym of ^)
 *   <=  (LE)
 *   >=  (GE)
 *   <>  (NEQ, not-equal)
 *   ^   (CARET, exponentiation; synonym of **)
 *   +   (PLUS)
 *   -   (MINUS, also unary)
 *   *   (STAR)
 *   /   (SLASH)
 *   <   (LT)
 *   >   (GT)
 *   =   (EQ)
 *   #   (HASH, not-equal; synonym of <>)
 *   $   (DOLLAR, substring containment)
 *   (   (LPAREN)
 *   )   (RPAREN)
 *   ,   (COMMA, function-argument separator -- S3.5)
 *
 * NOT III+ operators (lex errors):
 *   ==   -> XBLE_EQ_EQ    (dBASE IV; plan sec.2.C and sec.3.3)
 *   !=   -> XBLE_BANG_EQ  (not in III+; coercion.json not_in_iii_plus)
 *   %    -> XBLE_PERCENT  (dBASE IV; use MOD())
 *
 * Whitespace: ASCII space (0x20) and tab (0x09) are skipped between tokens.
 *
 * -------------------------------------------------------------------------
 * MUTATION HOOK (Rule 6):
 *
 * Compile with -DXB_MUTATE_LEX to enable a single perturbation: the lexer
 * ACCEPTS == (treats it as a valid EQ token) instead of rejecting it with
 * XBLE_EQ_EQ. The unit test's "== -> lex error" assertion then goes RED,
 * proving the test can catch the regression.
 *
 * -------------------------------------------------------------------------
 * Ref (Law 1):
 *   - docs/plans/SAMIR-implementation-plan.md S3.1, sec.2.C, sec.3.3
 *   - ../dbase3-decomp/specs/language/expressions-and-operators.md sec.2-6
 *   - ../dbase3-decomp/specs/language/coercion-table.md sec.2 (op inventory)
 *   - ../dbase3-decomp/specs/language/data-types.md sec.2,3,4,5
 *   - spec/samir/xbase_coercion.json not_in_iii_plus
 *   - re/mint-results-002.md (== not in III+ confirmed by omission)
 *
 * ASCII-clean (Rule 12). No timestamps / host paths (Rule 11).
 */

#include <stdint.h>
#include "samir/rt.h"
#include "samir/eval.h"

/* ---- internal helpers ---- */

/* ASCII case-fold: return uppercase if c is a lowercase letter, else c. */
static int upper_ascii(int c)
{
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

/* Attempt to parse a decimal double from src[i..]. Advance *pos past digits.
 * The function assumes the caller has already verified there is at least one
 * digit or a leading dot at src[*pos]. Returns the decoded value. */
static double parse_num(const char *src, uint32_t slen, uint32_t *pos)
{
    double whole   = 0.0;
    double frac    = 0.0;
    double fdiv    = 1.0;
    int    in_frac = 0;

    while (*pos < slen) {
        char c = src[*pos];
        if (c >= '0' && c <= '9') {
            if (!in_frac) {
                whole = whole * 10.0 + (double)(c - '0');
            } else {
                fdiv *= 10.0;
                frac += (double)(c - '0') / fdiv;
            }
            (*pos)++;
        } else if (c == '.' && !in_frac) {
            in_frac = 1;
            (*pos)++;
        } else {
            break;
        }
    }
    return whole + frac;
}

/* Match a keyword k (NUL-terminated, already upper-case) case-insensitively
 * at src[i]. Returns 1 if it matches and the char after the match is not
 * alphanumeric or underscore (so we don't match "ANDX" as ".AND."). */
static int match_kw(const char *src, uint32_t slen, uint32_t i,
                    const char *k, uint32_t klen)
{
    uint32_t j;
    if (i + klen > slen) return 0;
    for (j = 0; j < klen; j++) {
        if (upper_ascii((unsigned char)src[i + j]) != (unsigned char)k[j])
            return 0;
    }
    /* ensure next char (if any) is not alnum/underscore (word-break) */
    if (i + klen < slen) {
        char nx = src[i + klen];
        if ((nx >= 'A' && nx <= 'Z') || (nx >= 'a' && nx <= 'z') ||
            (nx >= '0' && nx <= '9') || nx == '_') {
            return 0;
        }
    }
    return 1;
}

/* ======================================================================== */
/* xb_lex                                                                     */
/* ======================================================================== */

int xb_lex(const char *src, uint32_t len,
           xb_token *out, uint32_t cap,
           int *err_code)
{
    uint32_t i   = 0;  /* current position in src */
    uint32_t cnt = 0;  /* tokens produced so far */

    *err_code = XBLE_OK;

#define EMIT(tok) \
    do { \
        if (cnt >= cap) { \
            *err_code = XBLE_BUF_OVERFLOW; \
            return -1; \
        } \
        out[cnt++] = (tok); \
    } while (0)

#define EMIT_ERR(ecode, off, tlen) \
    do { \
        xb_token _e; \
        _e.type = XBT_ERROR; \
        _e.offset = (off); \
        _e.len = (uint16_t)(tlen); \
        _e.u.l = 0; \
        *err_code = (ecode); \
        if (cnt < cap) out[cnt++] = _e; \
        return -1; \
    } while (0)

    while (i < len) {
        char c = src[i];

        /* --- skip whitespace --- */
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }

        /* ---------------------------------------------------------------- */
        /* String literals: 'text'  "text"  [text]                          */
        /* ---------------------------------------------------------------- */
        if (c == '\'' || c == '"' || c == '[') {
            char close = (c == '[') ? ']' : c;
            uint32_t start  = i;
            uint32_t cstart = i + 1; /* content start (after opening delim) */
            i++;
            while (i < len && src[i] != close) {
                i++;
            }
            if (i >= len) {
                /* Unterminated string (DBASE.MSG line 34) */
                EMIT_ERR(XBLE_UNTERM_STR, start, (uint16_t)(i - start));
            }
            /* src[i] == close; advance past it */
            i++;
            {
                xb_token t;
                t.type   = XBT_LIT_C;
                t.offset = (uint32_t)start;
                t.len    = (uint16_t)(i - start);
                t.u.c.p  = src + cstart;
                t.u.c.len = (uint16_t)((i - 1) - cstart);
                EMIT(t);
            }
            continue;
        }

        /* ---------------------------------------------------------------- */
        /* Numeric literals: digits[.digits]                                 */
        /* A leading sign is a separate token.                               */
        /* ---------------------------------------------------------------- */
        if (c >= '0' && c <= '9') {
            uint32_t start = i;
            double val = parse_num(src, len, &i);
            {
                xb_token t;
                t.type    = XBT_LIT_N;
                t.offset  = start;
                t.len     = (uint16_t)(i - start);
                t.u.n     = val;
                EMIT(t);
            }
            continue;
        }

        /* ---------------------------------------------------------------- */
        /* Dot -- could be: logical literal .T./.F./.Y./.N.,                 */
        /*                  operator .AND./.OR./.NOT.,                        */
        /*                  or (GATED) numeric .5 form.                      */
        /* ---------------------------------------------------------------- */
        if (c == '.') {
            uint32_t start = i;

            /* GATED: .5 numeric form: dot followed immediately by a digit.
             * CONSERVATIVE CHOICE: accept as numeric literal 0.N.
             * [data-types.md sec.3: no .5 form documented; oracle-resolves] */
            if (i + 1 < len && src[i + 1] >= '0' && src[i + 1] <= '9') {
                double val = parse_num(src, len, &i);
                xb_token t;
                t.type   = XBT_LIT_N;
                t.offset = start;
                t.len    = (uint16_t)(i - start);
                t.u.n    = val;
                EMIT(t);
                continue;
            }

            i++; /* consume the opening dot */

            /* Logical literals: .T. .F. .Y. .N. (case-insensitive) */
            if (i < len) {
                int u = upper_ascii((unsigned char)src[i]);

                /* .T. or .Y. -> true [.Y. GATED: oracle-resolves] */
                if ((u == 'T' || u == 'Y') &&
                    i + 1 < len && src[i + 1] == '.') {
                    xb_token t;
                    t.type    = XBT_LIT_L;
                    t.offset  = start;
                    t.len     = (uint16_t)(i + 2 - start);
                    t.u.l     = 1; /* true */
                    i += 2;
                    EMIT(t);
                    continue;
                }

                /* .F. or .N. -> false [.N. GATED: oracle-resolves] */
                if ((u == 'F' || u == 'N') &&
                    i + 1 < len && src[i + 1] == '.') {
                    xb_token t;
                    t.type    = XBT_LIT_L;
                    t.offset  = start;
                    t.len     = (uint16_t)(i + 2 - start);
                    t.u.l     = 0; /* false */
                    i += 2;
                    EMIT(t);
                    continue;
                }

                /* .AND. (4 letters + trailing dot = 5 chars after opening dot) */
                /* We consumed the opening dot; src[i..] is "AND." */
                if (match_kw(src, len, i, "AND.", 4)) {
                    xb_token t;
                    t.type    = XBT_AND;
                    t.offset  = start;
                    t.len     = 6; /* .AND. */
                    t.u.l     = 0;
                    i += 4; /* skip AND. */
                    EMIT(t);
                    continue;
                }

                /* .OR. (2 letters + trailing dot = 3 chars after opening dot) */
                if (match_kw(src, len, i, "OR.", 3)) {
                    xb_token t;
                    t.type    = XBT_OR;
                    t.offset  = start;
                    t.len     = 5; /* .OR. */
                    t.u.l     = 0;
                    i += 3;
                    EMIT(t);
                    continue;
                }

                /* .NOT. (3 letters + trailing dot = 4 chars after opening dot) */
                if (match_kw(src, len, i, "NOT.", 4)) {
                    xb_token t;
                    t.type    = XBT_NOT;
                    t.offset  = start;
                    t.len     = 6; /* .NOT. */
                    t.u.l     = 0;
                    i += 4;
                    EMIT(t);
                    continue;
                }
            }

            /* Bare dot with no recognized continuation */
            EMIT_ERR(XBLE_UNKNOWN_CHAR, start, 1);
        }

        /* ---------------------------------------------------------------- */
        /* Two-char and one-char operators                                   */
        /* Check two-char forms first.                                       */
        /* ---------------------------------------------------------------- */

        /* ** (exponentiation; must check before single *) */
        if (c == '*' && i + 1 < len && src[i + 1] == '*') {
            xb_token t;
            t.type = XBT_STARSTAR; t.offset = i; t.len = 2; t.u.l = 0;
            i += 2; EMIT(t); continue;
        }

        /* <= */
        if (c == '<' && i + 1 < len && src[i + 1] == '=') {
            xb_token t;
            t.type = XBT_LE; t.offset = i; t.len = 2; t.u.l = 0;
            i += 2; EMIT(t); continue;
        }

        /* >= */
        if (c == '>' && i + 1 < len && src[i + 1] == '=') {
            xb_token t;
            t.type = XBT_GE; t.offset = i; t.len = 2; t.u.l = 0;
            i += 2; EMIT(t); continue;
        }

        /* <> (not-equal) -- must check before < and > singles */
        if (c == '<' && i + 1 < len && src[i + 1] == '>') {
            xb_token t;
            t.type = XBT_NEQ; t.offset = i; t.len = 2; t.u.l = 0;
            i += 2; EMIT(t); continue;
        }

        /* == -- dBASE IV operator; III+ lex error (plan sec.2.C, sec.3.3) */
        if (c == '=' && i + 1 < len && src[i + 1] == '=') {
#ifdef XB_MUTATE_LEX
            /*
             * MUTATION HOOK (Rule 6): treat == as a valid EQ token instead
             * of rejecting it. This makes the "== -> lex error" test go RED,
             * proving the assertion can catch this regression.
             * Compile with -DXB_MUTATE_LEX to activate.
             */
            {
                xb_token t;
                t.type = XBT_EQ; t.offset = i; t.len = 2; t.u.l = 0;
                i += 2; EMIT(t); continue;
            }
#else
            EMIT_ERR(XBLE_EQ_EQ, i, 2);
#endif
        }

        /* != -- not a III+ operator (coercion.json not_in_iii_plus) */
        if (c == '!' && i + 1 < len && src[i + 1] == '=') {
            EMIT_ERR(XBLE_BANG_EQ, i, 2);
        }

        /* % -- dBASE IV modulo operator; use MOD() in III+ */
        if (c == '%') {
            EMIT_ERR(XBLE_PERCENT, i, 1);
        }

        /* Single-char operators */
        {
            xb_token_type tt;
            int matched = 1;
            switch ((unsigned char)c) {
                case '+': tt = XBT_PLUS;   break;
                case '-': tt = XBT_MINUS;  break;
                case '*': tt = XBT_STAR;   break;
                case '/': tt = XBT_SLASH;  break;
                case '^': tt = XBT_CARET;  break;
                case '<': tt = XBT_LT;     break;
                case '>': tt = XBT_GT;     break;
                case '=': tt = XBT_EQ;     break;
                case '#': tt = XBT_HASH;   break;
                case '$': tt = XBT_DOLLAR; break;
                case '(': tt = XBT_LPAREN; break;
                case ')': tt = XBT_RPAREN; break;
                case ',': tt = XBT_COMMA;  break;  /* fn arg separator (S3.5) */
                default:  matched = 0;     tt = XBT_ERROR; break;
            }
            if (matched) {
                xb_token t;
                t.type = tt; t.offset = i; t.len = 1; t.u.l = 0;
                i++; EMIT(t); continue;
            }
        }

        /* ---------------------------------------------------------------- */
        /* Identifiers: letter or underscore, then alnum or underscore       */
        /* ---------------------------------------------------------------- */
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
            uint32_t start = i;
            while (i < len) {
                char nc = src[i];
                if ((nc >= 'A' && nc <= 'Z') || (nc >= 'a' && nc <= 'z') ||
                    (nc >= '0' && nc <= '9') || nc == '_') {
                    i++;
                } else {
                    break;
                }
            }
            {
                xb_token t;
                t.type      = XBT_IDENT;
                t.offset    = start;
                t.len       = (uint16_t)(i - start);
                t.u.ident.p   = src + start;
                t.u.ident.len = (uint16_t)(i - start);
                EMIT(t);
            }
            continue;
        }

        /* ---------------------------------------------------------------- */
        /* Unknown character                                                  */
        /* ---------------------------------------------------------------- */
        EMIT_ERR(XBLE_UNKNOWN_CHAR, i, 1);
    }

    /* Emit EOI */
    {
        xb_token t;
        t.type   = XBT_EOI;
        t.offset = i;
        t.len    = 0;
        t.u.l    = 0;
        EMIT(t);
    }

    return (int)cnt;

#undef EMIT
#undef EMIT_ERR
}
