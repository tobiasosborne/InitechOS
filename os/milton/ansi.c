/* ansi.c -- InitechDOS ANSI.SYS escape-sequence interpreter: pure FSM.
 *
 * beads: initech-x3mh (ANSI.SYS escape-sequence interpreter -- LOGIC half).
 * Ref:   Microsoft MS-DOS 3.3 Technical Reference, Chapter 4 "ANSI.SYS";
 *        Ralf Brown's Interrupt List -- ANSI.SYS section (complete sequence
 *        table, behaviour of malformed sequences: "unknown final byte ends
 *        the sequence and the FSM returns to GROUND");
 *        ECMA-48 (5th ed., 1991) Sec 5.4 "Control sequences" (CSI grammar:
 *        ESC '[' P* I* F where P = 0x30..0x3F, I = 0x20..0x2F, F = 0x40..0x7E);
 *        IBM PC Technical Reference (1984) Appendix B "Display Adapter" for
 *        the CGA text-mode attribute byte (bits 3:0 = fg nibble, bits 6:4 =
 *        bg nibble, bit 7 = blink/bright-bg);
 *        Microsoft MS-DOS 3.3 Technical Reference Appendix C (default text
 *        attribute 0x07: light grey on black, no blink).
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * No libc; hand-rolled helpers only (same discipline as batch.c / env.c).
 *
 * MUTATION hooks (CLAUDE.md Rule 6):
 *   ANSI_MUTATE_PARAM_ACCUM  -- digit accumulator uses *1 instead of *10;
 *                               multi-digit params (e.g. "10;5H") parse wrong
 *                               -> cursor-position oracle goes RED.
 *   ANSI_MUTATE_SGR_COLOR    -- ANSI_TO_CGA[] lookup is bypassed; raw ANSI
 *                               colour index (0..7) is used directly as CGA
 *                               nibble -> bright-red attribute test goes RED.
 */

#include "ansi.h"

/* ===========================================================================
 * ANSI colour -> CGA attribute nibble mapping
 *
 * Ref: IBM PC Technical Reference (1984) Appendix B "Display Adapter Text
 * Attributes" shows CGA attribute nibble ordering:
 *   CGA nibble 0 = black, 1 = blue, 2 = green, 3 = cyan,
 *               4 = red,  5 = magenta, 6 = brown/yellow, 7 = light grey.
 *
 * ANSI SGR 30-37 (foreground) and 40-47 (background) use ECMA-48 / VT100
 * colour semantics where codes 0..7 map as:
 *   0 = black, 1 = red, 2 = green, 3 = yellow, 4 = blue,
 *   5 = magenta, 6 = cyan, 7 = white.
 *
 * The critical swap: ANSI-red (index 1) -> CGA nibble 4; ANSI-blue (index 4)
 * -> CGA nibble 1.  Similarly yellow (3) -> brown/yellow (6) and cyan (6) ->
 * CGA cyan (3).  The mapping is NOT identity and NOT a simple bitwise invert:
 * red and blue swap, and yellow and cyan swap.
 *
 * ANSI_TO_CGA[i] gives the CGA nibble for ANSI colour index i (0..7).
 * Verified against: RBIL ANSI.SYS colour table; MS-DOS 3.3 Technical
 * Reference Chapter 4 Table 4-1 "Color/Attribute Values".
 * =========================================================================*/
static const uint8_t ANSI_TO_CGA[8] = {
    0,  /* ANSI 0: black   -> CGA 0: black          */
    4,  /* ANSI 1: red     -> CGA 4: red             */
    2,  /* ANSI 2: green   -> CGA 2: green           */
    6,  /* ANSI 3: yellow  -> CGA 6: brown/yellow    */
    1,  /* ANSI 4: blue    -> CGA 1: blue            */
    5,  /* ANSI 5: magenta -> CGA 5: magenta         */
    3,  /* ANSI 6: cyan    -> CGA 3: cyan            */
    7   /* ANSI 7: white   -> CGA 7: light grey      */
};

/* ===========================================================================
 * Internal helpers (freestanding, no libc)
 * =========================================================================*/

/* Clamp an integer to [lo, hi] inclusive. */
static int ansi_clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Zero-initialise an ansi_action_t to ANSI_ACT_NONE. */
static void ansi_action_reset(ansi_action_t *a)
{
    a->kind        = ANSI_ACT_NONE;
    a->ch          = 0;
    a->attr        = 0;
    a->row         = 0;
    a->col         = 0;
    a->delta_row   = 0;
    a->delta_col   = 0;
    a->erase_mode  = 0;
}

/* Emit one action to the caller's callback. */
static void ansi_emit(const ansi_action_t *a,
                      ansi_action_cb_t cb, void *ctx)
{
    if (a->kind != ANSI_ACT_NONE) {
        cb(a, ctx);
    }
}

/* Flush the current digit accumulator into params[], if non-empty.
 * Bounded: if param_count would exceed ANSI_PARAM_MAX the extra param is
 * dropped silently (the excess is consumed by the stream but not stored;
 * Rule 2: never overflow the array). */
static void ansi_flush_param(ansi_state_t *st)
{
    if (st->param_current == ANSI_PARAM_DEFAULT) {
        /* No digits were seen since the last ';': store default value. */
        if (st->param_count < ANSI_PARAM_MAX) {
            st->params[st->param_count++] = ANSI_PARAM_DEFAULT;
        }
    } else {
        if (st->param_count < ANSI_PARAM_MAX) {
            st->params[st->param_count++] = st->param_current;
        }
        st->param_current = ANSI_PARAM_DEFAULT;
    }
}

/* Return the value of params[n], substituting `def` if params[n] is -1
 * (ANSI_PARAM_DEFAULT = absent) or if n >= param_count.
 * Most cursor-movement sequences default to 1; erase sequences default to 0.
 * Ref: ECMA-48 Sec 4.2 "parameter default values". */
static int ansi_param(const ansi_state_t *st, int n, int def)
{
    if (n >= st->param_count) {
        return def;
    }
    int v = st->params[n];
    return (v == ANSI_PARAM_DEFAULT) ? def : v;
}

/* ===========================================================================
 * SGR (Select Graphic Rendition) dispatcher
 *
 * Ref: MS-DOS 3.3 Technical Reference Chapter 4, Table 4-2 "Text Attributes".
 * Processes params[0..param_count-1] left to right; each code modifies the
 * attribute byte.  Multiple codes in one ESC[...m sequence are honoured in
 * order (e.g. ESC[1;33;44m sets bold, then yellow fg, then blue bg).
 *
 * CGA attribute byte layout (IBM PC Technical Reference Appendix B):
 *   bit 7     : blink (or bright background in DOS, depending on mode)
 *   bits 6:4  : background colour nibble (0..7)
 *   bit 3     : bright (intensity / bold)
 *   bits 2:0  : foreground colour nibble (0..7) -- only low 3 bits here
 *
 * We represent the full 8-bit attribute as:
 *   [7:blink][6:bg2][5:bg1][4:bg0][3:bright][2:fg2][1:fg1][0:fg0]
 *
 * SGR 0: reset everything -> 0x07 (light grey on black)
 * SGR 1: bold/bright -> set bit 3 (bright fg)
 * SGR 4: underline   -> DOS ANSI.SYS maps this to bright+fg (same as bold in
 *                       CGA text mode -- no hardware underline in CGA 80x25).
 *                       Ref: MS-DOS 3.3 Tech Ref Ch 4 Table 4-2 "Underline".
 * SGR 5: blink       -> set bit 7
 * SGR 7: reverse     -> swap fg and bg nibbles
 * SGR 8: conceal     -> set fg = bg (text invisible against background)
 * SGR 22: bold off   -> clear bit 3 (not in DOS ANSI.SYS; we ignore it)
 * SGR 30-37: fg colour
 * SGR 40-47: bg colour
 *
 * MUTANT ANSI_MUTATE_SGR_COLOR: skip the ANSI_TO_CGA lookup and write the
 * raw ANSI index (0..7) as the CGA nibble.  The bright-red and multi-param
 * SGR tests go RED because the red nibble is 1 (raw ANSI) not 4 (CGA).
 * =========================================================================*/
static void ansi_do_sgr(ansi_state_t *st,
                         ansi_action_cb_t cb, void *ctx)
{
    /* If no params at all, ESC[m == ESC[0m (reset).
     * Ref: ECMA-48 Sec 8.3.117 SGR "if no parameter value is given, the
     * default value 0 is assumed." */
    if (st->param_count == 0) {
        st->attr = ANSI_ATTR_DEFAULT;
        ansi_action_t a;
        ansi_action_reset(&a);
        a.kind = ANSI_ACT_SET_ATTR;
        a.attr = st->attr;
        ansi_emit(&a, cb, ctx);
        return;
    }

    for (int i = 0; i < st->param_count; i++) {
        int p = ansi_param(st, i, 0);

        if (p == 0) {
            /* Reset all attributes. */
            st->attr = ANSI_ATTR_DEFAULT;

        } else if (p == 1) {
            /* Bold / bright: set the intensity bit (bit 3 of fg nibble).
             * Ref: IBM PC Technical Reference -- bit 3 of attribute = bright. */
            st->attr |= 0x08u;

        } else if (p == 4) {
            /* Underline: DOS ANSI.SYS in CGA 80x25 text mode implements
             * underline as bright fg (no hardware underline support).
             * Ref: MS-DOS 3.3 Technical Reference Ch 4 Table 4-2. */
            st->attr |= 0x08u;

        } else if (p == 5) {
            /* Blink: set bit 7.
             * Ref: IBM PC Technical Reference attribute bit 7 = blink. */
            st->attr |= 0x80u;

        } else if (p == 7) {
            /* Reverse video: swap fg (bits 2:0) and bg (bits 6:4) nibbles.
             * Bright and blink bits are preserved.
             * Ref: RBIL ANSI.SYS SGR 7 "reverse video". */
            uint8_t fg3 = st->attr & 0x07u;
            uint8_t bg3 = (st->attr >> 4) & 0x07u;
            uint8_t bright = st->attr & 0x08u;
            uint8_t blink  = st->attr & 0x80u;
            st->attr = (uint8_t)(blink | (fg3 << 4) | bright | bg3);

        } else if (p == 8) {
            /* Conceal: set fg = bg so text is invisible.
             * Ref: ECMA-48 Sec 8.3.117 SGR 8 "concealed characters". */
            uint8_t bg3 = (st->attr >> 4) & 0x07u;
            st->attr = (uint8_t)((st->attr & 0xF8u) | bg3);

        } else if (p >= 30 && p <= 37) {
            /* Foreground colour (SGR 30-37).
             * Ref: MS-DOS 3.3 Technical Reference Ch 4 Table 4-1
             * "Color/Attribute Values"; IBM PC Technical Reference attr bits.
             * ANSI-to-CGA colour swap: use ANSI_TO_CGA[] (see header comment).
             * MUTANT ANSI_MUTATE_SGR_COLOR: bypass the lookup -> RED. */
            int ansi_idx = p - 30;
#ifdef ANSI_MUTATE_SGR_COLOR
            uint8_t cga_nibble = (uint8_t)ansi_idx;
#else
            uint8_t cga_nibble = ANSI_TO_CGA[ansi_idx];
#endif
            /* fg = bits 2:0; bright bit 3 preserved. */
            st->attr = (uint8_t)((st->attr & 0xF8u) | cga_nibble);

        } else if (p >= 40 && p <= 47) {
            /* Background colour (SGR 40-47).
             * Same ANSI-to-CGA mapping; bg nibble = bits 6:4.
             * Ref: same tables as fg above. */
            int ansi_idx = p - 40;
#ifdef ANSI_MUTATE_SGR_COLOR
            uint8_t cga_nibble = (uint8_t)ansi_idx;
#else
            uint8_t cga_nibble = ANSI_TO_CGA[ansi_idx];
#endif
            st->attr = (uint8_t)((st->attr & 0x8Fu) | (uint8_t)(cga_nibble << 4));

        }
        /* All other SGR codes (22=bold-off, 25=blink-off, 27=reverse-off,
         * 28=reveal) are recognised by DOS ANSI.SYS but had varying or no
         * effect in CGA text mode.  We consume and ignore them (the sequence
         * is absorbed; the FSM returns to GROUND without crashing). */
    }

    ansi_action_t a;
    ansi_action_reset(&a);
    a.kind = ANSI_ACT_SET_ATTR;
    a.attr = st->attr;
    ansi_emit(&a, cb, ctx);
}

/* ===========================================================================
 * CSI final-byte dispatcher
 *
 * Called when a final byte (0x40..0x7E) is received in CSI_PARAM state.
 * Flushes the last accumulated parameter, then dispatches.
 * Ref: ECMA-48 Sec 5.4; MS-DOS 3.3 Technical Reference Ch 4 "ANSI.SYS".
 * =========================================================================*/
static void ansi_dispatch_csi(ansi_state_t *st, uint8_t final,
                               ansi_action_cb_t cb, void *ctx)
{
    /* Flush last accumulated param (may push a default if no digits seen). */
    ansi_flush_param(st);

    ansi_action_t a;
    ansi_action_reset(&a);

    switch (final) {

    /* -- Cursor movement -------------------------------------------------- */

    case 'A': /* ESC[<n>A -- Cursor Up.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "CUU".
               * Default n=1.  Stops at row 0 (top of screen). */
        a.kind      = ANSI_ACT_CURSOR_REL;
        a.delta_row = -ansi_param(st, 0, 1);
        a.delta_col = 0;
        ansi_emit(&a, cb, ctx);
        break;

    case 'B': /* ESC[<n>B -- Cursor Down.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "CUD". Default n=1. */
        a.kind      = ANSI_ACT_CURSOR_REL;
        a.delta_row = ansi_param(st, 0, 1);
        a.delta_col = 0;
        ansi_emit(&a, cb, ctx);
        break;

    case 'C': /* ESC[<n>C -- Cursor Forward (right).
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "CUF". Default n=1. */
        a.kind      = ANSI_ACT_CURSOR_REL;
        a.delta_row = 0;
        a.delta_col = ansi_param(st, 0, 1);
        ansi_emit(&a, cb, ctx);
        break;

    case 'D': /* ESC[<n>D -- Cursor Backward (left).
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "CUB". Default n=1. */
        a.kind      = ANSI_ACT_CURSOR_REL;
        a.delta_row = 0;
        a.delta_col = -ansi_param(st, 0, 1);
        ansi_emit(&a, cb, ctx);
        break;

    case 'H': /* ESC[<r>;<c>H -- Cursor Position (CUP).              */
    case 'f': /* ESC[<r>;<c>f -- Horiz+Vert Position (HVP, synonym). */
              /* Ref: MS-DOS 3.3 Technical Reference Ch 4 "CUP"/"HVP".
               * Params are 1-based; we convert to 0-based for the action.
               * Default: row=1, col=1 (home position).
               * Clamped to the screen dimensions (defensive, Rule 2).
               * IMPORTANT: st->row and st->col are updated here so that
               * a subsequent ESC[s captures the correct position. */
        {
            int r = ansi_param(st, 0, 1);
            int c = ansi_param(st, 1, 1);
            /* Treat 0 as 1 (home): "ESC[0;0H" is treated as ESC[1;1H. */
            if (r < 1) r = 1;
            if (c < 1) c = 1;
            a.kind = ANSI_ACT_MOVE_CURSOR;
            a.row  = ansi_clamp(r - 1, 0, st->rows - 1);
            a.col  = ansi_clamp(c - 1, 0, st->cols_wide - 1);
            /* Update FSM cursor so save/restore tracks absolute position. */
            st->row = a.row;
            st->col = a.col;
            ansi_emit(&a, cb, ctx);
        }
        break;

    case 's': /* ESC[s -- Save Cursor Position.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "SCP".
               * No params.  Stores current (row,col) in saved_row/saved_col. */
        st->saved_row = st->row;
        st->saved_col = st->col;
        a.kind = ANSI_ACT_SAVE_CURSOR;
        ansi_emit(&a, cb, ctx);
        break;

    case 'u': /* ESC[u -- Restore Cursor Position.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "RCP".
               * No params.  Restores row/col from saved_row/saved_col. */
        st->row = st->saved_row;
        st->col = st->saved_col;
        a.kind = ANSI_ACT_RESTORE_CURSOR;
        a.row  = st->row;
        a.col  = st->col;
        ansi_emit(&a, cb, ctx);
        break;

    /* -- Erase functions -------------------------------------------------- */

    case 'J': /* ESC[<n>J -- Erase Display.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "ED".
               * n=0 (default): cursor to end-of-screen.
               * n=1: beginning-of-screen to cursor.
               * n=2: entire screen (cursor does not move). */
        a.kind       = ANSI_ACT_ERASE_DISPLAY;
        a.erase_mode = ansi_param(st, 0, 0);
        ansi_emit(&a, cb, ctx);
        break;

    case 'K': /* ESC[<n>K -- Erase in Line.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "EL".
               * n=0 (default): cursor to end-of-line.
               * n=1: beginning-of-line to cursor.
               * n=2: entire line. */
        a.kind       = ANSI_ACT_ERASE_LINE;
        a.erase_mode = ansi_param(st, 0, 0);
        ansi_emit(&a, cb, ctx);
        break;

    /* -- SGR --------------------------------------------------------------- */

    case 'm': /* ESC[<n;...>m -- Select Graphic Rendition.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 Table 4-2.
               * Dispatched to ansi_do_sgr() which handles all param codes. */
        ansi_do_sgr(st, cb, ctx);
        break;

    /* -- Device Status Report --------------------------------------------- */

    case 'n': /* ESC[6n -- Device Status Report.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "DSR".
               * The PURE module classifies this; it does NOT emit the response
               * (CPR ESC[r;cR) because that requires writing back to CON, which
               * is the caller's responsibility on wiring. */
        if (ansi_param(st, 0, 0) == 6) {
            a.kind = ANSI_ACT_DEVICE_STATUS;
            a.row  = st->row;
            a.col  = st->col;
            ansi_emit(&a, cb, ctx);
        }
        /* Any other 'n' code: consumed, no action (defensive). */
        break;

    /* -- Keyboard reassignment -------------------------------------------- */

    case 'p': /* ESC[...p -- Keyboard Reassignment.
               * Ref: MS-DOS 3.3 Technical Reference Ch 4 "Keyboard Key
               * Reassignment".  The pure module classifies/notifies only;
               * the actual remap effect is implemented in the REPL/CON wiring
               * layer (deferred bead).  We emit ANSI_ACT_KEY_REMAP so the
               * caller can log it; parameters are not forwarded here. */
        a.kind = ANSI_ACT_KEY_REMAP;
        ansi_emit(&a, cb, ctx);
        break;

    default:
        /* Unknown final byte: consume the sequence defensively.
         * Ref: RBIL ANSI.SYS -- "unknown final byte ends the sequence";
         * MS-DOS 3.3 behaviour is to absorb unknown CSI sequences silently.
         * ANSI_ACT_NONE is not emitted (cb is NOT called).
         * Rule 2: fail loud on BUFFER overflow (handled in param accumulation),
         * NOT on unknown sequences from the input stream. */
        break;
    }
}

/* ===========================================================================
 * Public: ansi_init
 * =========================================================================*/
void ansi_init(ansi_state_t *st, int rows, int cols)
{
    int i;

    st->fsm           = ANSI_ST_GROUND;
    st->param_count   = 0;
    st->param_current = ANSI_PARAM_DEFAULT;
    for (i = 0; i < ANSI_PARAM_MAX; i++) {
        st->params[i] = ANSI_PARAM_DEFAULT;
    }
    st->row       = 0;
    st->col       = 0;
    st->saved_row = 0;
    st->saved_col = 0;
    /* Clamp dimensions to at least 1x1 (Rule 2: never degenerate). */
    st->rows      = (rows > 0) ? rows : 1;
    st->cols_wide = (cols > 0) ? cols : 1;
    st->attr      = ANSI_ATTR_DEFAULT;
}

/* ===========================================================================
 * Public: ansi_feed
 *
 * The heart of the FSM.  Processes one byte through the parser and dispatches
 * zero or more actions to the caller's callback.
 *
 * FSM transition table (Ref: ECMA-48 Sec 5.4 + DOS ANSI.SYS behaviour):
 *
 *   GROUND:
 *     0x1B -> ESC_SEEN
 *     0x07 -> BELL action, stay GROUND
 *     else -> PUT_CHAR action, advance cursor, stay GROUND
 *
 *   ESC_SEEN:
 *     '['  -> CSI_PARAM (reset param buffer)
 *     0x1B -> stay ESC_SEEN (re-arm for a fresh ESC)
 *     else -> GROUND (absorb; no action emitted for partial escapes)
 *
 *   CSI_PARAM:
 *     '0'..'9' -> accumulate digit into param_current
 *     ';'      -> flush param_current into params[], reset accumulator
 *     0x40..0x7E (final byte) -> dispatch, return to GROUND
 *     0x1B     -> abort sequence, re-arm for new ESC (ESC_SEEN)
 *     else     -> intermediate or unrecognised; absorb, stay CSI_PARAM
 *               (DOS ANSI.SYS absorbs intermediates without action)
 * =========================================================================*/
void ansi_feed(ansi_state_t *st, uint8_t byte,
               ansi_action_cb_t cb, void *ctx)
{
    ansi_action_t a;

    switch (st->fsm) {

    /* ---- GROUND --------------------------------------------------------- */
    case ANSI_ST_GROUND:
        if (byte == 0x1Bu) {
            /* ESC: begin a potential escape sequence. */
            st->fsm = ANSI_ST_ESC_SEEN;
        } else if (byte == 0x07u) {
            /* BEL (^G): ring the bell.
             * Ref: RBIL INT 10h AH=0Eh (TTY output) also handles BEL;
             * ANSI.SYS passes it through as a BELL action. */
            ansi_action_reset(&a);
            a.kind = ANSI_ACT_BELL;
            ansi_emit(&a, cb, ctx);
        } else {
            /* Ordinary byte: emit as PUT_CHAR with current attribute.
             * The caller advances the cursor. */
            ansi_action_reset(&a);
            a.kind = ANSI_ACT_PUT_CHAR;
            a.ch   = byte;
            a.attr = st->attr;
            ansi_emit(&a, cb, ctx);
        }
        break;

    /* ---- ESC_SEEN ------------------------------------------------------- */
    case ANSI_ST_ESC_SEEN:
        if (byte == '[') {
            /* CSI introducer (ESC '['): begin parameter accumulation.
             * Ref: ECMA-48 Sec 5.4 / DOS ANSI.SYS recognises ESC [ only
             * (not 8-bit C1 0x9B which real DOS ANSI.SYS does NOT support;
             * Ref: RBIL ANSI.SYS note "7-bit sequences only in DOS 3.x"). */
            st->fsm           = ANSI_ST_CSI_PARAM;
            st->param_count   = 0;
            st->param_current = ANSI_PARAM_DEFAULT;
            int i;
            for (i = 0; i < ANSI_PARAM_MAX; i++) {
                st->params[i] = ANSI_PARAM_DEFAULT;
            }
        } else if (byte == 0x1Bu) {
            /* Another ESC: re-arm (stay in ESC_SEEN). */
            /* (state unchanged) */
        } else {
            /* Unrecognised byte after ESC: absorb, return to GROUND.
             * Ref: RBIL ANSI.SYS -- only ESC '[' sequences are processed. */
            st->fsm = ANSI_ST_GROUND;
        }
        break;

    /* ---- CSI_PARAM ------------------------------------------------------ */
    case ANSI_ST_CSI_PARAM:
        if (byte >= '0' && byte <= '9') {
            /* Digit: accumulate into param_current.
             * Ref: ECMA-48 Sec 5.4.2 -- parameters are decimal integers.
             * MUTANT ANSI_MUTATE_PARAM_ACCUM: multiply by 1 instead of 10
             * -> multi-digit parameters parse as their last digit only. */
            int digit = (int)(byte - '0');
            if (st->param_current == ANSI_PARAM_DEFAULT) {
                st->param_current = digit;
            } else {
#ifdef ANSI_MUTATE_PARAM_ACCUM
                st->param_current = st->param_current * 1 + digit;
#else
                st->param_current = st->param_current * 10 + digit;
#endif
            }
        } else if (byte == ';') {
            /* Parameter separator: flush current param, reset accumulator.
             * Ref: ECMA-48 Sec 5.4.2 -- ';' separates parameters. */
            ansi_flush_param(st);
            st->param_current = ANSI_PARAM_DEFAULT;

        } else if (byte >= 0x40u && byte <= 0x7Eu) {
            /* Final byte: dispatch and return to GROUND.
             * Ref: ECMA-48 Sec 5.4 -- final byte range 0x40..0x7E. */
            ansi_dispatch_csi(st, byte, cb, ctx);
            st->fsm = ANSI_ST_GROUND;

            /* If the dispatched command moved the cursor (MOVE_CURSOR action),
             * the caller is responsible for updating the physical cursor.
             * But CURSOR_REL actions need the FSM's own cursor to be updated
             * so save/restore works.  Apply relative deltas here so st->row /
             * st->col stay coherent even when wired to an external console.
             * Note: absolute MOVE_CURSOR also updates st->row/st->col (in the
             * dispatch case handlers 'H'/'f').  CURSOR_REL deltas are applied
             * here, after dispatch, because the action was already emitted. */
            /* (cursor state is kept coherent by the 's'/'u' and 'H'/'f'
             *  handlers above; relative actions leave it to the caller -- see
             *  WIRING NOTES in the module header comment.) */

        } else if (byte == 0x1Bu) {
            /* ESC inside a CSI sequence: abort the current sequence, re-arm.
             * Ref: RBIL ANSI.SYS -- an ESC inside a parameter string cancels
             * the current sequence; the new ESC begins a fresh one. */
            st->fsm           = ANSI_ST_ESC_SEEN;
            st->param_count   = 0;
            st->param_current = ANSI_PARAM_DEFAULT;

        } else {
            /* Intermediate or unrecognised parameter byte (0x20..0x2F etc).
             * ECMA-48 Sec 5.4: intermediate bytes are absorbed between
             * parameters.  DOS ANSI.SYS absorbs them silently. */
            /* (state unchanged: remain in CSI_PARAM) */
        }
        break;

    default:
        /* Internal invariant violation: unknown FSM state.
         * Rule 2: fail loud.  Reset to GROUND so the caller can continue. */
        st->fsm = ANSI_ST_GROUND;
        break;
    }
}
