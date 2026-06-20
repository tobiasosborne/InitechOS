/* ansi.h -- InitechDOS ANSI.SYS escape-sequence interpreter: pure FSM.
 *
 * beads: initech-x3mh (ANSI.SYS escape-sequence interpreter -- LOGIC half;
 *        wiring into the CON output path is a deferred follow-up bead).
 * Ref:   Microsoft MS-DOS 3.3 Technical Reference, Chapter 4 "ANSI.SYS"
 *        (the canonical list of sequences supported by MS-DOS 3.3 ANSI.SYS);
 *        Ralf Brown's Interrupt List -- ANSI.SYS section (RBIL table AH=4Bh,
 *        the INT 29h / CON output path, the CSI sequence catalogue);
 *        ECMA-48 (5th ed., 1991) for the formal parameter grammar / terminator
 *        table (DOS ANSI.SYS implements a strict, forward-compatible subset);
 *        IBM PC Technical Reference (1984) "CGA/Text-Mode Attribute Byte" for
 *        the colour-nibble encoding: fg = bits 0..3, bg = bits 4..6, blink =
 *        bit 7 (bright bg in DOS) -- this is what ANSI.SYS maps SGR codes onto.
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * No libc: hand-rolled helpers (same discipline as batch.c / env.c).
 *
 * HOST-TESTABILITY SEAM: all ansi_* functions are PURE (no asm, no I/O), so
 * the SAME translation unit compiles HOSTED for os/milton/test_ansi.c.  The
 * test file #includes ansi.c directly (the same TU trick as test_batch.c /
 * test_env.c / test_mz.c).
 *
 * DESIGN -- callback/command model:
 *   ansi_feed(st, byte, cb, ctx) parses one byte and calls cb(action, ctx) for
 *   each resulting action.  The FSM itself has no I/O; the caller provides the
 *   sink.  This keeps the module testable and decoupled from the console driver.
 *
 * ANSI-COLOR <-> CGA-ATTRIBUTE MAPPING (Law 1; cite: IBM PC Technical Reference
 * Appendix B "Display Adapter" attribute byte + RBIL ANSI.SYS colour table):
 *
 *   ANSI SGR code  Colour name  CGA fg nibble  CGA bg nibble
 *   30 / 40        Black        0x0            0x0
 *   31 / 41        Red          0x4            0x4   <-- NOTE: ANSI 31 = red,
 *   32 / 42        Green        0x2            0x2       CGA red = attribute 4
 *   33 / 43        Yellow       0x6            0x6       (NOT attribute 1 which
 *   34 / 44        Blue         0x1            0x1       is CGA dark blue).
 *   35 / 45        Magenta      0x5            0x5   The mapping is NOT a simple
 *   36 / 46        Cyan         0x3            0x3   identity: ANSI uses the
 *   37 / 47        White        0x7            0x7   RGB-additive colour cube
 *                                                    (red=1,green=2,blue=4) but
 *   SGR 1 (bold)   -> bright bit (fg bit 3):         CGA red=4, green=2, blue=1
 *   bright fg = fg | 0x8 (e.g. bright red = 0xC)    (CGA uses RCB order, not
 *   SGR 5 (blink)  -> blink bit (attribute bit 7):   RGB). See ANSI_TO_CGA[]
 *   blink/bright-bg bit in DOS text mode             in ansi.c for the table.
 *
 * The six-element swap in the colour table (ANSI-red=1 vs CGA-red=4 and
 * ANSI-blue=4 vs CGA-blue=1) is the classic DOS ANSI.SYS footgun -- always
 * cite the IBM PC Technical Reference attribute byte, not intuition.
 *
 * MUTATION hooks (CLAUDE.md Rule 6):
 *   ANSI_MUTATE_PARAM_ACCUM  -- the multi-digit accumulator multiplies by 1
 *                               instead of 10; "10;5H" reads params as {1,5}
 *                               so the cursor-position test goes RED.
 *   ANSI_MUTATE_SGR_COLOR    -- the CGA colour nibble comes straight from the
 *                               raw ANSI code (0..7) without the ANSI_TO_CGA[]
 *                               lookup swap; the bright-red SGR attribute test
 *                               and the multi-param SGR test go RED.
 */
#ifndef INITECH_ANSI_H
#define INITECH_ANSI_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Limits
 * -------------------------------------------------------------------------*/

/* Maximum number of numeric parameters in a single CSI sequence.
 * ECMA-48 does not define a cap; DOS ANSI.SYS processes at most a handful
 * (SGR can take up to ~4 in practice).  16 is ample and bounded (Rule 2). */
#define ANSI_PARAM_MAX   16

/* The default parameter value when a numeric field is omitted.  Most DOS
 * ANSI.SYS sequences treat an absent parameter as 1 (cursor movement) or 0
 * (SGR / erase).  Callers use ANSI_PARAM_DEFAULT to distinguish "not set". */
#define ANSI_PARAM_DEFAULT  (-1)

/* Default CGA text attribute: light grey on black = 0x07.
 * Ref: IBM PC Technical Reference attribute byte: fg=7 (white), bg=0 (black),
 * blink=0. The DOS command prompt default.  Ref: RBIL ANSI.SYS init state. */
#define ANSI_ATTR_DEFAULT   0x07u

/* ---------------------------------------------------------------------------
 * FSM states
 * -------------------------------------------------------------------------*/

/* Ref: ECMA-48 Sec 5.4 "Control sequences" -- the three parser states that
 * DOS ANSI.SYS needs: ground (no escape seen), ESC_SEEN (got 0x1B, waiting
 * for '['), CSI_PARAM (inside CSI, accumulating digits and semicolons). */
typedef enum {
    ANSI_ST_GROUND    = 0,  /* Normal output; pass bytes through       */
    ANSI_ST_ESC_SEEN  = 1,  /* Got 0x1B; waiting for '[' (CSI intro)  */
    ANSI_ST_CSI_PARAM = 2   /* Inside CSI; reading digits/';'/final    */
} ansi_fsm_state_t;

/* ---------------------------------------------------------------------------
 * Action types -- the FSM's output vocabulary
 * -------------------------------------------------------------------------*/

/* An action is one discrete side-effect the FSM requests from the caller.
 * The caller (console driver) handles each action; the FSM never touches I/O.
 * Ref: the DOS ANSI.SYS effect table (RBIL ANSI.SYS section). */
typedef enum {
    ANSI_ACT_PUT_CHAR,      /* Emit one character at cursor, then advance     */
    ANSI_ACT_MOVE_CURSOR,   /* Set absolute cursor position (row,col)         */
    ANSI_ACT_CURSOR_REL,    /* Move cursor relative (delta_row, delta_col)    */
    ANSI_ACT_SAVE_CURSOR,   /* Save current cursor position                   */
    ANSI_ACT_RESTORE_CURSOR,/* Restore cursor position from saved             */
    ANSI_ACT_ERASE_DISPLAY, /* Erase display (mode: 0=to-end,1=to-start,2=all)*/
    ANSI_ACT_ERASE_LINE,    /* Erase line    (mode: 0=to-end,1=to-start,2=all)*/
    ANSI_ACT_SET_ATTR,      /* Set SGR attribute byte (CGA format)            */
    ANSI_ACT_DEVICE_STATUS, /* ESC[6n Device Status Report -- note only       */
    ANSI_ACT_BELL,          /* BEL (0x07) -- ring the bell                   */
    ANSI_ACT_KEY_REMAP,     /* ESC[...p keyboard reassignment -- note only    */
    ANSI_ACT_NONE           /* No-op (sequence consumed, no visible effect)   */
} ansi_action_kind_t;

/* Payload for a single action.  Only the fields relevant to the kind are
 * meaningful; unused fields are zero-initialised by the FSM. */
typedef struct {
    ansi_action_kind_t kind;

    /* ANSI_ACT_PUT_CHAR: the character to emit and the attribute to use.
     * The caller draws this glyph at the current cursor position, THEN
     * advances the cursor (wrap at right edge; scroll at bottom). */
    uint8_t  ch;        /* glyph codepoint (0x00..0xFF)                       */
    uint8_t  attr;      /* CGA attribute byte at time of emission              */

    /* ANSI_ACT_MOVE_CURSOR: absolute 0-based (row, col).
     * ANSI sequences use 1-based coords; the FSM converts to 0-based before
     * issuing this action (document the off-by-one here so callers do NOT
     * re-adjust).  Ref: ECMA-48 Sec 4.2 "parameter default values". */
    int      row;       /* 0-based destination row                             */
    int      col;       /* 0-based destination column                         */

    /* ANSI_ACT_CURSOR_REL: signed (delta_row, delta_col).
     * Positive delta_row = down; positive delta_col = right (A/B/C/D). */
    int      delta_row; /* signed row delta                                   */
    int      delta_col; /* signed column delta                                */

    /* ANSI_ACT_ERASE_DISPLAY / ANSI_ACT_ERASE_LINE: the 'J'/'K' mode param.
     * 0 = cursor to end, 1 = start to cursor, 2 = whole display/line. */
    int      erase_mode;

} ansi_action_t;

/* ---------------------------------------------------------------------------
 * Callback type
 * -------------------------------------------------------------------------*/

/* The caller supplies a function of this type.  The FSM calls it once per
 * action generated.  `ctx` is forwarded unchanged (opaque caller context).
 * The callback MUST NOT modify `act` (it is const).
 * The callback MUST NOT call ansi_feed() recursively. */
typedef void (*ansi_action_cb_t)(const ansi_action_t *act, void *ctx);

/* ---------------------------------------------------------------------------
 * ANSI parser state struct
 * -------------------------------------------------------------------------*/

typedef struct {
    ansi_fsm_state_t fsm;                    /* current parser state         */

    int  params[ANSI_PARAM_MAX];             /* accumulated numeric params   */
    int  param_count;                        /* how many params so far        */
    int  param_current;                      /* digit accumulator, -1 = empty */

    int  row;                                /* 0-based cursor row            */
    int  col;                                /* 0-based cursor col            */
    int  saved_row;                          /* ESC[s saved row               */
    int  saved_col;                          /* ESC[s saved col               */

    int  rows;                               /* screen height (initialised)   */
    int  cols_wide;                          /* screen width  (initialised)   */

    uint8_t attr;                            /* current CGA attribute byte    */
} ansi_state_t;

/* ---------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/

/* Reset the parser: FSM -> GROUND, cursor -> (0,0), attr -> ANSI_ATTR_DEFAULT.
 * `rows` and `cols` are the screen dimensions (used for clamping; pass
 * CONSOLE_ROWS / CONSOLE_COLS from console.h when wiring into the kernel). */
void ansi_init(ansi_state_t *st, int rows, int cols);

/* Feed one byte into the parser.  Zero, one, or more actions are dispatched
 * to `cb(action, ctx)` synchronously before ansi_feed() returns.  `cb` must
 * not be NULL.  `ctx` is forwarded to every cb call.
 *
 * Thread safety / reentrancy: none required (DOS is single-threaded).
 * Deterministic (Rule 11): same byte sequence -> same action stream. */
void ansi_feed(ansi_state_t *st, uint8_t byte,
               ansi_action_cb_t cb, void *ctx);

#endif /* INITECH_ANSI_H */
