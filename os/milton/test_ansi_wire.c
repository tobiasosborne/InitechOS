/* test_ansi_wire.c -- host oracle for WIRING the ANSI.SYS FSM into the CON
 *                     output path (beads initech-p96i; CLOSES initech-x3mh).
 *
 * beads: initech-p96i (wire ansi.c into con_putc, gated by DEVICE=ANSI.SYS).
 * Ref:   MS-DOS 3.3 Technical Reference Chapter 4 "ANSI.SYS" (CUP/ED/SGR/SCP/RCP
 *        effects); Ralf Brown's Interrupt List -- ANSI.SYS section; IBM PC
 *        Technical Reference (1984) Appendix B "Display Adapter" (CGA attribute
 *        byte). CLAUDE.md Law 2 (the oracle is truth), Law 4 (no-ANSI rendering
 *        is byte-identical), Rule 1 (RED->GREEN), Rule 6 (mutation-proven),
 *        Rule 12 (ASCII).
 *
 * WHAT THIS TESTS (the WIRING, not the pure FSM -- that is test_ansi.c):
 *   The REAL shipped chain con_putc -> ansi_feed -> ansi_apply -> console-ops.
 *   We #include the REAL int21.c TU (the same trick test_int21.c-adjacent tests
 *   use), bind a MOCK console-ops table + a MOCK gate + a MOCK sink through the
 *   PUBLIC seams (int21_set_ansi_console / int21_set_ansi_gate / int21_set_sink),
 *   and drive AH=02h DISPLAY OUTPUT through int21_dispatch so con_putc runs for
 *   real. The mock console is a 25x80 grid capturing (glyph, cga_attr) per cell
 *   plus a cursor -- a faithful stand-in for console.c's cur_row/cur_col +
 *   cur_fg/cur_bg, so the assertions are about the real wiring's behaviour.
 *
 * ASSERTS:
 *   - ANSI ON, plain "HI"        -> two glyphs at the cursor with the DEFAULT
 *                                   attribute, cursor advanced (== no-ANSI).
 *   - ANSI ON, ESC[2J            -> grid cleared.
 *   - ANSI ON, ESC[5;10H         -> cursor at row 4, col 9 (0-based).
 *   - ANSI ON, ESC[31;1m + 'X'   -> 'X' drawn with bright-red attribute (0x0C).
 *   - ANSI ON, ESC[s..move..ESC[u-> cursor restored to the saved position.
 *   - ANSI OFF, the SAME escape bytes -> each byte rendered LITERALLY through the
 *                                   raw sink (proving the gate works).
 *
 * MUTATION (Rule 6) -- driven by make test-ansi-wire-mutant:
 *   -DANSIWIRE_MUTATE_IGNORE_GATE : con_ansi_active() ignores the gate and always
 *                                   feeds the FSM. The "ANSI OFF renders the
 *                                   escape bytes literally" assertion goes RED
 *                                   (the FSM swallows them instead). A one-branch
 *                                   RUNTIME perturbation that COMPILES under
 *                                   -Werror (it lives inside con_ansi_active()).
 *
 * Compiles HOSTED, libc OK (Law 3: factory/host test). #includes int21.c (which
 * #includes ansi.c) directly; links the SAME deps test_int21 links (sft/psp/mcb/
 * devices/irq/config_sys) since int21_dispatch resolves handles through them.
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "int21.h"
#include "sft.h"
#include "psp.h"
#include "test_assert.h"

/* Pull in the REAL artifact dispatcher TU (which #includes ansi.c). The mutation
 * flag propagates from the gcc command line into int21.c's con_ansi_active(). */
#include "int21.c"

TEST_HARNESS();

/* The ANSI default CGA attribute (light grey on black), 0x07. ansi.c starts the
 * FSM here and ESC[0m resets to it. console.c maps it back to the console's own
 * default ink/paper; in this mock it is simply the sentinel "default" attr. */
#define ATTR_DEFAULT 0x07u
/* Bright red = ANSI 31 (fg red) + ANSI 1 (bold) -> CGA red nibble 4 | bright 8
 * = 0x0C. Ref: ansi.c ANSI_TO_CGA[1]=4 + SGR 1 sets bit 3. */
#define ATTR_BRIGHT_RED 0x0Cu

/* ===========================================================================
 * The MOCK console -- a faithful stand-in for the live console.c grid.
 * =========================================================================*/
#define MOCK_ROWS 25
#define MOCK_COLS 80

typedef struct {
    uint8_t glyph[MOCK_ROWS][MOCK_COLS];
    uint8_t attr [MOCK_ROWS][MOCK_COLS];
    int     row;          /* cursor row (0-based)            */
    int     col;          /* cursor col (0-based)            */
} mock_console_t;

static mock_console_t g_mock;

static void mock_reset(void)
{
    memset(&g_mock, 0, sizeof(g_mock));
    g_mock.row = 0;
    g_mock.col = 0;
}

/* console_putc semantics: draw glyph at cursor with `attr`, advance with wrap +
 * scroll. (No real scroll needed at the small inputs here; we wrap to next row
 * and clamp at the last row, matching console.c advance_line.) */
static void mock_put_char(void *ctx, uint8_t ch, uint8_t cga_attr)
{
    mock_console_t *m = (mock_console_t *)ctx;
    if (ch == '\n') { m->col = 0; if (m->row + 1 < MOCK_ROWS) m->row++; return; }
    if (ch == '\r') { m->col = 0; return; }
    if (ch == '\b') { if (m->col > 0) m->col--; return; }
    if (m->row < 0 || m->row >= MOCK_ROWS || m->col < 0 || m->col >= MOCK_COLS) {
        return;
    }
    m->glyph[m->row][m->col] = ch;
    m->attr [m->row][m->col] = cga_attr;
    m->col++;
    if (m->col >= MOCK_COLS) {
        m->col = 0;
        if (m->row + 1 < MOCK_ROWS) m->row++;
    }
}

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void mock_set_cursor(void *ctx, int row, int col)
{
    mock_console_t *m = (mock_console_t *)ctx;
    m->row = clamp_int(row, 0, MOCK_ROWS - 1);
    m->col = clamp_int(col, 0, MOCK_COLS - 1);
}

static void mock_cursor_rel(void *ctx, int drow, int dcol)
{
    mock_console_t *m = (mock_console_t *)ctx;
    m->row = clamp_int(m->row + drow, 0, MOCK_ROWS - 1);
    m->col = clamp_int(m->col + dcol, 0, MOCK_COLS - 1);
}

/* The "current attribute" the mock's set_attr stashes; mock_put_char draws each
 * glyph with the attr the FSM passes in the PUT_CHAR action, so we do not need a
 * separate current-attr field -- the action carries it. set_attr is asserted via
 * the glyph attr that follows it. */
static void mock_set_attr(void *ctx, uint8_t cga_attr)
{
    (void)ctx; (void)cga_attr;   /* effect observed via the next glyph's attr */
}

static void mock_erase_display(void *ctx, int mode)
{
    mock_console_t *m = (mock_console_t *)ctx;
    if (mode == 2) {
        memset(m->glyph, 0, sizeof(m->glyph));
        memset(m->attr,  0, sizeof(m->attr));
    } else if (mode == 0) {
        /* cursor..end: clear the rest of the current row + all rows below. */
        for (int c = m->col; c < MOCK_COLS; c++) { m->glyph[m->row][c] = 0; m->attr[m->row][c] = 0; }
        for (int r = m->row + 1; r < MOCK_ROWS; r++)
            for (int c = 0; c < MOCK_COLS; c++) { m->glyph[r][c] = 0; m->attr[r][c] = 0; }
    } else if (mode == 1) {
        for (int r = 0; r < m->row; r++)
            for (int c = 0; c < MOCK_COLS; c++) { m->glyph[r][c] = 0; m->attr[r][c] = 0; }
        for (int c = 0; c <= m->col && c < MOCK_COLS; c++) { m->glyph[m->row][c] = 0; m->attr[m->row][c] = 0; }
    }
}

static void mock_erase_line(void *ctx, int mode)
{
    mock_console_t *m = (mock_console_t *)ctx;
    if (mode == 2) {
        for (int c = 0; c < MOCK_COLS; c++) { m->glyph[m->row][c] = 0; m->attr[m->row][c] = 0; }
    } else if (mode == 1) {
        for (int c = 0; c <= m->col && c < MOCK_COLS; c++) { m->glyph[m->row][c] = 0; m->attr[m->row][c] = 0; }
    } else {
        for (int c = m->col; c < MOCK_COLS; c++) { m->glyph[m->row][c] = 0; m->attr[m->row][c] = 0; }
    }
}

static void mock_get_cursor(void *ctx, int *row, int *col)
{
    mock_console_t *m = (mock_console_t *)ctx;
    if (row) *row = m->row;
    if (col) *col = m->col;
}

static int21_ansi_console_t make_mock_ops(void)
{
    int21_ansi_console_t ops;
    ops.put_char      = mock_put_char;
    ops.set_cursor    = mock_set_cursor;
    ops.cursor_rel    = mock_cursor_rel;
    ops.erase_display = mock_erase_display;
    ops.erase_line    = mock_erase_line;
    ops.set_attr      = mock_set_attr;
    ops.get_cursor    = mock_get_cursor;
    ops.ctx           = &g_mock;
    return ops;
}

/* ===========================================================================
 * The MOCK gate -- flips ANSI on/off so we can prove con_ansi_active() routes.
 * =========================================================================*/
static int g_gate_on = 0;
static int mock_gate(void) { return g_gate_on; }

/* ===========================================================================
 * The capturing raw sink -- the OFF path (and the put_char fallback) write here.
 * =========================================================================*/
static char   g_sink_buf[512];
static size_t g_sink_len;
static void   sink_capture(char c) { if (g_sink_len < sizeof(g_sink_buf) - 1) g_sink_buf[g_sink_len++] = c; }
static void   sink_reset(void)     { g_sink_len = 0; g_sink_buf[0] = '\0'; }
static const char *sink_str(void)  { g_sink_buf[g_sink_len] = '\0'; return g_sink_buf; }

/* ===========================================================================
 * Drive the REAL con_putc via AH=02h DISPLAY OUTPUT through int21_dispatch.
 * =========================================================================*/
static psp_t g_test_psp;
static void bind_standard_process(void)
{
    psp_params_t params;
    params.alloc_end_linear  = 0x00070000u;
    params.env_linear        = 0u;
    params.parent_psp_linear = 0u;
    params.cmd_tail          = (const char *)0;
    params.cmd_tail_len      = 0u;
    (void)psp_build(&g_test_psp, &params);
    sft_init();
    int21_set_psp(&g_test_psp);
}

/* Emit one byte through the REAL dispatcher (AH=02h, DL=byte) -> con_putc. */
static void emit(uint8_t byte)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;
    f.eax    = 0x0200u;             /* AH=02h DISPLAY OUTPUT */
    f.edx    = (uint32_t)byte;      /* DL = byte */
    int21_dispatch(&f);
}

static void emit_str(const char *s) { while (*s) emit((uint8_t)*s++); }

/* The CSI introducer 0x1B '[' as a writable buffer so the ASCII grep stays
 * clean (the ESC byte is a numeric literal, not a source byte). */
static void emit_esc(void) { emit(0x1Bu); }

int main(void)
{
    int21_set_sink(sink_capture);
    int21_set_ansi_gate(mock_gate);
    {
        int21_ansi_console_t ops = make_mock_ops();
        int21_set_ansi_console(&ops);
    }
    bind_standard_process();

    /* ===================================================================
     * 1. ANSI ON: plain "HI" -> two glyphs at the cursor, DEFAULT attr,
     *    cursor advanced. This is the byte-identical-to-no-ANSI claim.
     * =================================================================*/
    g_gate_on = 1;
    g_ansi_inited = 0;          /* force a fresh ansi_init at first con_putc */
    mock_reset();
    sink_reset();
    emit_str("HI");
    CHECK(g_mock.glyph[0][0] == 'H' && g_mock.glyph[0][1] == 'I',
          "ANSI ON: plain 'HI' lands as two glyphs at the cursor");
    CHECK(g_mock.attr[0][0] == ATTR_DEFAULT && g_mock.attr[0][1] == ATTR_DEFAULT,
          "ANSI ON: plain text uses the DEFAULT attribute (== no-ANSI rendering)");
    CHECK(g_mock.col == 2 && g_mock.row == 0,
          "ANSI ON: cursor advanced by two columns");
    CHECK(g_sink_len == 0,
          "ANSI ON: plain text did NOT leak to the raw sink (it went to put_char)");

    /* ===================================================================
     * 2. ANSI ON: ESC[2J clears the grid.
     * =================================================================*/
    mock_reset();
    g_mock.glyph[3][4] = 'Z';    /* pre-existing content to be cleared */
    emit_esc(); emit_str("[2J");
    CHECK(g_mock.glyph[3][4] == 0, "ANSI ON: ESC[2J clears the grid");

    /* ===================================================================
     * 3. ANSI ON: ESC[5;10H -> cursor row 4 col 9 (1-based -> 0-based).
     * =================================================================*/
    mock_reset();
    emit_esc(); emit_str("[5;10H");
    CHECK(g_mock.row == 4 && g_mock.col == 9,
          "ANSI ON: ESC[5;10H moves cursor to row 4, col 9 (0-based)");

    /* ===================================================================
     * 4. ANSI ON: ESC[31;1m sets bright red; the next glyph carries it.
     * =================================================================*/
    mock_reset();
    emit_esc(); emit_str("[31;1m");
    emit((uint8_t)'X');
    CHECK(g_mock.glyph[0][0] == 'X',
          "ANSI ON: ESC[31;1m then 'X' draws the glyph");
    CHECK(g_mock.attr[0][0] == ATTR_BRIGHT_RED,
          "ANSI ON: ESC[31;1m gives the glyph a bright-red CGA attribute (0x0C)");

    /* ===================================================================
     * 5. ANSI ON: ESC[s .. move .. ESC[u restores the saved cursor.
     *    Save at (0,0); move to (4,9); restore -> back to (0,0).
     * =================================================================*/
    mock_reset();
    emit_esc(); emit_str("[s");           /* save (0,0) -- reads live cursor */
    emit_esc(); emit_str("[5;10H");       /* move to (4,9) */
    CHECK(g_mock.row == 4 && g_mock.col == 9, "ANSI ON: moved away from the saved spot");
    emit_esc(); emit_str("[u");           /* restore -> (0,0) */
    CHECK(g_mock.row == 0 && g_mock.col == 0,
          "ANSI ON: ESC[s then ESC[u restores the saved cursor position");

    /* Cursor authority: ESC[s must save the LIVE console position, not the FSM's
     * stale one. Print 3 chars (advancing the console to col 3), THEN save, move,
     * restore -> the restore must land at col 3 (the live position), not col 0. */
    mock_reset();
    emit_str("abc");                       /* console cursor now (0,3) */
    emit_esc(); emit_str("[s");            /* save the LIVE (0,3) */
    emit_esc(); emit_str("[5;10H");        /* move away */
    emit_esc(); emit_str("[u");            /* restore */
    CHECK(g_mock.row == 0 && g_mock.col == 3,
          "ANSI ON: ESC[s saves the LIVE cursor (after plain text), not a stale FSM cursor");

    /* ===================================================================
     * 6. ANSI OFF: the SAME escape bytes render LITERALLY through the raw
     *    sink (proving the gate works -- the FSM is bypassed entirely).
     *    The MUTANT (ANSIWIRE_MUTATE_IGNORE_GATE) makes con_ansi_active()
     *    ignore the gate, so the FSM swallows these bytes -> this is RED.
     * =================================================================*/
    g_gate_on = 0;
    mock_reset();
    sink_reset();
    emit_esc(); emit_str("[2J");          /* ESC [ 2 J */
    /* With ANSI OFF, EVERY byte (incl. ESC, '[', '2', 'J') reaches the raw sink
     * literally; the mock console is untouched. */
    CHECK(g_sink_len == 4,
          "ANSI OFF: all 4 escape bytes reached the raw sink (gate bypassed the FSM)");
    {
        const char *s = sink_str();
        CHECK((uint8_t)s[0] == 0x1Bu && s[1] == '[' && s[2] == '2' && s[3] == 'J',
              "ANSI OFF: the escape bytes are emitted LITERALLY (ESC '[' '2' 'J')");
    }
    CHECK(g_mock.glyph[0][0] == 0,
          "ANSI OFF: the mock console was NOT touched (no FSM interpretation)");

    /* A plain char with ANSI OFF still reaches the sink (the original path). */
    sink_reset();
    emit((uint8_t)'Q');
    CHECK_STR_EQ(sink_str(), "Q", "ANSI OFF: a plain char passes straight to the sink");

    return TEST_SUMMARY("test_ansi_wire");
}
