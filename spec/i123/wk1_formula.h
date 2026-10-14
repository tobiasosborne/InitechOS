/*
 * spec/i123/wk1_formula.h -- the Lotus 1-2-3 Release 2.2 formula bytecode:
 * the RPN token set a .WK1 FORMULA record carries, the operator precedence
 * the typed text compiles by, and the @function table (LOCKED SPEC-DATA,
 * Rule 8).
 *
 * bead: initech-94ah (I123 P2: formula engine, @function core, recalc).
 *       Plan: docs/plans/INITECH-123-plan.md Sec 4 (@function inventory),
 *       Sec 5 (oracle strategy), Sec 6 P2.
 *
 * WHY HERE (spec/, not os/i123/core): the token numbers ARE file-format
 * bytes -- a FORMULA record's code (wk1_format.h WK1_OP_FORMULA) is a
 * stream of them -- so they are locked format data beside wk1_format.h,
 * changed only by a deliberate, issue-backed edit. The SEMANTICS of each
 * token (what @MOD returns) live in code, os/i123/core/eval.c, each rule
 * citing its source.
 *
 * Ref (Law 1):
 *   [C] ../lotus123-decomp/specs/formulas/wk1-formula-bytecode.md Sec 1-5:
 *       token widths, arities derived by stack simulation over the 1090
 *       FORMULA records of the 44 goldens, the counted-arity call
 *       convention, which tokens the minted goldens pin by TYPED TEXT;
 *   [M] Lotus 1-2-3 Release 2.2 Reference Manual (1989; scanned, OCR'd by
 *       this lane with tesseract into build/man/ocr/pNNN.txt -- PDF page
 *       numbers below): p. 1-30 (PDF 47) the order-of-precedence table;
 *       p. 1-29 (PDF 46) "A formula can contain up to 240 characters ...
 *       cannot contain spaces, except within literal strings"; Chapter 13
 *       (@functions) for the R2.2 NAME and ARGUMENT COUNT of every
 *       function, cited per row by its manual page.
 *
 * INDEPENDENCE BARRIER (ADR-0008 DEC-05): nothing here is copied from
 * ../lotus123-decomp/tools/wk1_ref.py. The token NUMBERS are the bytes in
 * the goldens; the NAMES are the R2.2 manual's (they differ from the
 * reader's older literature names in five places: @COUNT/@CNT,
 * @DCOUNT/@DCNT, @CHAR/@CHR, @CODE/@ASCII, @SYD/@SOY, and token 72 is
 * @STRING by typed evidence, GAPS L-1 / G-4). The oracle that grades the
 * compiler (test-i123-formula) checks the bytes 123.EXE wrote and the values
 * it cached, never this table against the reader's.
 *
 * EVIDENCE per row (WKF_EV_*):
 *   TYPED   the minted goldens hold the typed source text AND the bytes
 *           (mint/NAME.steps + goldens/minted/NAME.WK1): the name<->token map is
 *           verified.
 *   RECOMP  the token occurs in the shipped sheets; its name comes from the
 *           public token table and the manual; test-i123-formula recomputes
 *           the real cached results with this engine's semantics.
 *   DOC     never seen in any golden: the token number is the public
 *           literature's, the name/arity the manual's. Implemented to the
 *           manual; NOT oracle-verified [oracle-resolves].
 *
 * Freestanding: <stdint.h> only. ASCII-clean (Rule 12).
 */
#ifndef INITECH_SPEC_I123_WK1_FORMULA_H
#define INITECH_SPEC_I123_WK1_FORMULA_H

#include <stdint.h>

/* --- 1. Operand / control tokens ([C] Sec 2) ---------------------------- */
#define WKF_CONST      0u   /* + 8 bytes IEEE binary64 LE                    */
#define WKF_VAR        1u   /* + u16 colWord, u16 rowWord (Sec 2.1)           */
#define WKF_RANGE      2u   /* + two 4-byte refs                              */
#define WKF_RETURN     3u   /* ends the stream (1090/1090)                    */
#define WKF_PAREN      4u   /* a typed (...) group: no stack effect (Sec 4)   */
#define WKF_INT        5u   /* + i16 LE                                       */
#define WKF_STRING     6u   /* + NUL-terminated bytes                         */

/* --- 2. Operators ([C] Sec 3) -------------------------------------------- */
#define WKF_NEG        8u
#define WKF_ADD        9u
#define WKF_SUB       10u
#define WKF_MUL       11u
#define WKF_DIV       12u
#define WKF_POW       13u
#define WKF_EQ        14u
#define WKF_NE        15u
#define WKF_LE        16u
#define WKF_GE        17u
#define WKF_LT        18u
#define WKF_GT        19u
#define WKF_AND       20u
#define WKF_OR        21u   /* DOC: never in a golden (GAPS G-12)            */
#define WKF_NOT       22u   /* DOC: never in a golden (GAPS G-12)            */
#define WKF_UPLUS     23u
#define WKF_CAT       24u   /* '&': 81 uses, string results (GAPS F-1)       */

#define WKF_FIRST_FN  31u
#define WKF_LINK     115u   /* the @@ token; 101/101 uses in the goldens are
                             * a STRING "<<FILE>>ref" then 115: a FILE LINK
                             * (manual p. 1-41..1-43, PDF 57-59, "+<<FILE>>A1") */
#define WKF_LAST_FN  121u
#define WKF_NTOKENS  122u

/* Reference words (Sec 2.1): bit 15 = relative; the low 14 bits are then a
 * signed offset (bit 13 sign); relative COLUMN offsets wrap modulo 256. */
#define WKF_REF_REL      0x8000u
#define WKF_REF_MASK     0x3FFFu
#define WKF_REF_SIGN     0x2000u

/* --- 3. Token classes ---------------------------------------------------- */
#define WKF_K_NONE     0u   /* unassigned (7, 25..30) -- refused             */
#define WKF_K_OPERAND  1u   /* CONST VAR RANGE INT STRING                     */
#define WKF_K_CTRL     2u   /* RETURN PAREN                                   */
#define WKF_K_UNARY    3u
#define WKF_K_BINARY   4u
#define WKF_K_FIXED    5u   /* fixed arity, NO count byte (@IF: [C] Sec 5)    */
#define WKF_K_COUNTED  6u   /* one count byte follows the token               */

#define WKF_EV_NONE    0u
#define WKF_EV_TYPED   1u
#define WKF_EV_RECOMP  2u
#define WKF_EV_DOC     3u

/* Operator precedence, manual p. 1-30 (PDF 47) "Order of Precedence": the
 * LOWER number is performed FIRST; equal numbers left to right.
 *   ^ 1;  unary - + 2;  * / 3;  + - 4;  = <> < > <= >= 5;  #NOT# 6;
 *   #AND# #OR# 7;  & 7. */
#define WKF_PREC_POW   1u
#define WKF_PREC_SIGN  2u
#define WKF_PREC_MUL   3u
#define WKF_PREC_ADD   4u
#define WKF_PREC_CMP   5u
#define WKF_PREC_NOT   6u
#define WKF_PREC_LOG   7u
#define WKF_PREC_MAX   7u

/* Formula text limits: manual p. 1-29 (PDF 46). */
#define WKF_TEXT_MAX   240u

typedef struct wkf_token {
    const char *name;   /* the typed text: "+", "#AND#", "@SUM" ...        */
    uint8_t cls;        /* WKF_K_*                                          */
    uint8_t arity;      /* operators / fixed functions; counted: minimum    */
    uint8_t prec;       /* operators only (WKF_PREC_*)                      */
    uint8_t rmask;      /* bit i: argument i is a RANGE parameter -- a lone
                         * cell there compiles to RANGE(a..a) ([C] Sec 2.1:
                         * "@S code RANGE(..a..a) @S", SAMP6X)              */
    uint8_t ev;         /* WKF_EV_*                                         */
} wkf_token_t;

/* --- 4. THE TABLE, indexed by token number. -----------------------------
 * Function rows cite the manual page of their Chapter 13 entry (PDF page).
 * Argument counts of DOC rows are the manual's; of TYPED/RECOMP rows they
 * are ALSO the stack-simulation arity of [C] Sec 5 (they agree). */
#define WKF_TOKENS_INIT { \
    /*   0 */ { "CONST",       WKF_K_OPERAND, 0, 0, 0, WKF_EV_TYPED  }, \
    /*   1 */ { "VAR",         WKF_K_OPERAND, 0, 0, 0, WKF_EV_TYPED  }, \
    /*   2 */ { "RANGE",       WKF_K_OPERAND, 0, 0, 0, WKF_EV_TYPED  }, \
    /*   3 */ { "RETURN",      WKF_K_CTRL,    0, 0, 0, WKF_EV_TYPED  }, \
    /*   4 */ { "PAREN",       WKF_K_CTRL,    0, 0, 0, WKF_EV_RECOMP }, \
    /*   5 */ { "INT",         WKF_K_OPERAND, 0, 0, 0, WKF_EV_TYPED  }, \
    /*   6 */ { "STRING",      WKF_K_OPERAND, 0, 0, 0, WKF_EV_TYPED  }, \
    /*   7 */ { 0,             WKF_K_NONE,    0, 0, 0, WKF_EV_NONE   }, \
    /*   8 */ { "-",           WKF_K_UNARY,   1, WKF_PREC_SIGN, 0, WKF_EV_TYPED  }, \
    /*   9 */ { "+",           WKF_K_BINARY,  2, WKF_PREC_ADD,  0, WKF_EV_TYPED  }, \
    /*  10 */ { "-",           WKF_K_BINARY,  2, WKF_PREC_ADD,  0, WKF_EV_RECOMP }, \
    /*  11 */ { "*",           WKF_K_BINARY,  2, WKF_PREC_MUL,  0, WKF_EV_TYPED  }, \
    /*  12 */ { "/",           WKF_K_BINARY,  2, WKF_PREC_MUL,  0, WKF_EV_TYPED  }, \
    /*  13 */ { "^",           WKF_K_BINARY,  2, WKF_PREC_POW,  0, WKF_EV_TYPED  }, \
    /*  14 */ { "=",           WKF_K_BINARY,  2, WKF_PREC_CMP,  0, WKF_EV_RECOMP }, \
    /*  15 */ { "<>",          WKF_K_BINARY,  2, WKF_PREC_CMP,  0, WKF_EV_RECOMP }, \
    /*  16 */ { "<=",          WKF_K_BINARY,  2, WKF_PREC_CMP,  0, WKF_EV_RECOMP }, \
    /*  17 */ { ">=",          WKF_K_BINARY,  2, WKF_PREC_CMP,  0, WKF_EV_RECOMP }, \
    /*  18 */ { "<",           WKF_K_BINARY,  2, WKF_PREC_CMP,  0, WKF_EV_RECOMP }, \
    /*  19 */ { ">",           WKF_K_BINARY,  2, WKF_PREC_CMP,  0, WKF_EV_TYPED  }, \
    /*  20 */ { "#AND#",       WKF_K_BINARY,  2, WKF_PREC_LOG,  0, WKF_EV_RECOMP }, \
    /*  21 */ { "#OR#",        WKF_K_BINARY,  2, WKF_PREC_LOG,  0, WKF_EV_DOC    }, \
    /*  22 */ { "#NOT#",       WKF_K_UNARY,   1, WKF_PREC_NOT,  0, WKF_EV_DOC    }, \
    /*  23 */ { "+",           WKF_K_UNARY,   1, WKF_PREC_SIGN, 0, WKF_EV_RECOMP }, \
    /*  24 */ { "&",           WKF_K_BINARY,  2, WKF_PREC_LOG,  0, WKF_EV_RECOMP }, \
    /*  25 */ { 0, WKF_K_NONE, 0, 0, 0, WKF_EV_NONE }, \
    /*  26 */ { 0, WKF_K_NONE, 0, 0, 0, WKF_EV_NONE }, \
    /*  27 */ { 0, WKF_K_NONE, 0, 0, 0, WKF_EV_NONE }, \
    /*  28 */ { 0, WKF_K_NONE, 0, 0, 0, WKF_EV_NONE }, \
    /*  29 */ { 0, WKF_K_NONE, 0, 0, 0, WKF_EV_NONE }, \
    /*  30 */ { 0, WKF_K_NONE, 0, 0, 0, WKF_EV_NONE }, \
    /*  31 */ { "@NA",         WKF_K_FIXED,   0, 0, 0,    WKF_EV_TYPED  }, \
    /*  32 */ { "@ERR",        WKF_K_FIXED,   0, 0, 0,    WKF_EV_TYPED  }, \
    /*  33 */ { "@ABS",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  34 */ { "@INT",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  35 */ { "@SQRT",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  36 */ { "@LOG",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  37 */ { "@LN",         WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  38 */ { "@PI",         WKF_K_FIXED,   0, 0, 0,    WKF_EV_TYPED  }, \
    /*  39 */ { "@SIN",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  40 */ { "@COS",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  41 */ { "@TAN",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  42 */ { "@ATAN2",      WKF_K_FIXED,   2, 0, 0,    WKF_EV_DOC    }, \
    /*  43 */ { "@ATAN",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  44 */ { "@ASIN",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  45 */ { "@ACOS",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  46 */ { "@EXP",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  47 */ { "@MOD",        WKF_K_FIXED,   2, 0, 0,    WKF_EV_TYPED  }, \
    /*  48 */ { "@CHOOSE",     WKF_K_COUNTED, 2, 0, 0,    WKF_EV_TYPED  }, \
    /*  49 */ { "@ISNA",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  50 */ { "@ISERR",      WKF_K_FIXED,   1, 0, 0,    WKF_EV_RECOMP }, \
    /*  51 */ { "@FALSE",      WKF_K_FIXED,   0, 0, 0,    WKF_EV_DOC    }, \
    /*  52 */ { "@TRUE",       WKF_K_FIXED,   0, 0, 0,    WKF_EV_TYPED  }, \
    /*  53 */ { "@RAND",       WKF_K_FIXED,   0, 0, 0,    WKF_EV_DOC    }, \
    /*  54 */ { "@DATE",       WKF_K_FIXED,   3, 0, 0,    WKF_EV_TYPED  }, \
    /*  55 */ { "@NOW",        WKF_K_FIXED,   0, 0, 0,    WKF_EV_DOC    }, \
    /*  56 */ { "@PMT",        WKF_K_FIXED,   3, 0, 0,    WKF_EV_RECOMP }, \
    /*  57 */ { "@PV",         WKF_K_FIXED,   3, 0, 0,    WKF_EV_RECOMP }, \
    /*  58 */ { "@FV",         WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /*  59 */ { "@IF",         WKF_K_FIXED,   3, 0, 0,    WKF_EV_TYPED  }, \
    /*  60 */ { "@DAY",        WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  61 */ { "@MONTH",      WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  62 */ { "@YEAR",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  63 */ { "@ROUND",      WKF_K_FIXED,   2, 0, 0,    WKF_EV_TYPED  }, \
    /*  64 */ { "@TIME",       WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /*  65 */ { "@HOUR",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  66 */ { "@MINUTE",     WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  67 */ { "@SECOND",     WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  68 */ { "@ISNUMBER",   WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  69 */ { "@ISSTRING",   WKF_K_FIXED,   1, 0, 0,    WKF_EV_RECOMP }, \
    /*  70 */ { "@LENGTH",     WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  71 */ { "@VALUE",      WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /*  72 */ { "@STRING",     WKF_K_FIXED,   2, 0, 0,    WKF_EV_TYPED  }, \
    /*  73 */ { "@MID",        WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /*  74 */ { "@CHAR",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  75 */ { "@CODE",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  76 */ { "@FIND",       WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /*  77 */ { "@DATEVALUE",  WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  78 */ { "@TIMEVALUE",  WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  79 */ { "@CELLPOINTER",WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /*  80 */ { "@SUM",        WKF_K_COUNTED, 1, 0, 0,    WKF_EV_TYPED  }, \
    /*  81 */ { "@AVG",        WKF_K_COUNTED, 1, 0, 0,    WKF_EV_TYPED  }, \
    /*  82 */ { "@COUNT",      WKF_K_COUNTED, 1, 0, 0,    WKF_EV_DOC    }, \
    /*  83 */ { "@MIN",        WKF_K_COUNTED, 1, 0, 0,    WKF_EV_DOC    }, \
    /*  84 */ { "@MAX",        WKF_K_COUNTED, 1, 0, 0,    WKF_EV_TYPED  }, \
    /*  85 */ { "@VLOOKUP",    WKF_K_FIXED,   3, 0, 0x02, WKF_EV_RECOMP }, \
    /*  86 */ { "@NPV",        WKF_K_FIXED,   2, 0, 0x02, WKF_EV_DOC    }, \
    /*  87 */ { "@VAR",        WKF_K_COUNTED, 1, 0, 0,    WKF_EV_DOC    }, \
    /*  88 */ { "@STD",        WKF_K_COUNTED, 1, 0, 0,    WKF_EV_DOC    }, \
    /*  89 */ { "@IRR",        WKF_K_FIXED,   2, 0, 0x02, WKF_EV_DOC    }, \
    /*  90 */ { "@HLOOKUP",    WKF_K_FIXED,   3, 0, 0x02, WKF_EV_RECOMP }, \
    /*  91 */ { "@DSUM",       WKF_K_FIXED,   3, 0, 0x05, WKF_EV_DOC    }, \
    /*  92 */ { "@DAVG",       WKF_K_FIXED,   3, 0, 0x05, WKF_EV_DOC    }, \
    /*  93 */ { "@DCOUNT",     WKF_K_FIXED,   3, 0, 0x05, WKF_EV_RECOMP }, \
    /*  94 */ { "@DMIN",       WKF_K_FIXED,   3, 0, 0x05, WKF_EV_DOC    }, \
    /*  95 */ { "@DMAX",       WKF_K_FIXED,   3, 0, 0x05, WKF_EV_DOC    }, \
    /*  96 */ { "@DVAR",       WKF_K_FIXED,   3, 0, 0x05, WKF_EV_DOC    }, \
    /*  97 */ { "@DSTD",       WKF_K_FIXED,   3, 0, 0x05, WKF_EV_DOC    }, \
    /*  98 */ { "@INDEX",      WKF_K_FIXED,   3, 0, 0x01, WKF_EV_RECOMP }, \
    /*  99 */ { "@COLS",       WKF_K_FIXED,   1, 0, 0x01, WKF_EV_DOC    }, \
    /* 100 */ { "@ROWS",       WKF_K_FIXED,   1, 0, 0x01, WKF_EV_RECOMP }, \
    /* 101 */ { "@REPEAT",     WKF_K_FIXED,   2, 0, 0,    WKF_EV_DOC    }, \
    /* 102 */ { "@UPPER",      WKF_K_FIXED,   1, 0, 0,    WKF_EV_TYPED  }, \
    /* 103 */ { "@LOWER",      WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /* 104 */ { "@LEFT",       WKF_K_FIXED,   2, 0, 0,    WKF_EV_DOC    }, \
    /* 105 */ { "@RIGHT",      WKF_K_FIXED,   2, 0, 0,    WKF_EV_DOC    }, \
    /* 106 */ { "@REPLACE",    WKF_K_FIXED,   4, 0, 0,    WKF_EV_DOC    }, \
    /* 107 */ { "@PROPER",     WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /* 108 */ { "@CELL",       WKF_K_FIXED,   2, 0, 0x02, WKF_EV_DOC    }, \
    /* 109 */ { "@TRIM",       WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /* 110 */ { "@CLEAN",      WKF_K_FIXED,   1, 0, 0,    WKF_EV_DOC    }, \
    /* 111 */ { "@S",          WKF_K_FIXED,   1, 0, 0x01, WKF_EV_RECOMP }, \
    /* 112 */ { "@N",          WKF_K_FIXED,   1, 0, 0x01, WKF_EV_DOC    }, \
    /* 113 */ { "@EXACT",      WKF_K_FIXED,   2, 0, 0,    WKF_EV_DOC    }, \
    /* 114 */ { 0,             WKF_K_NONE,    0, 0, 0,    WKF_EV_NONE   }, \
    /* 115 */ { "@@",          WKF_K_FIXED,   1, 0, 0,    WKF_EV_RECOMP }, \
    /* 116 */ { "@RATE",       WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /* 117 */ { "@TERM",       WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /* 118 */ { "@CTERM",      WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /* 119 */ { "@SLN",        WKF_K_FIXED,   3, 0, 0,    WKF_EV_DOC    }, \
    /* 120 */ { "@SYD",        WKF_K_FIXED,   4, 0, 0,    WKF_EV_DOC    }, \
    /* 121 */ { "@DDB",        WKF_K_FIXED,   4, 0, 0,    WKF_EV_DOC    }  \
}

#endif /* INITECH_SPEC_I123_WK1_FORMULA_H */
