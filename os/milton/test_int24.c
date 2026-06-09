/* test_int24.c -- host unit oracle for the INT 22/23/24 handlers, SETVECT/
 * GETVECT (AH=25h/35h), and PSP-vector save/restore (beads initech-509.8).
 * Factory test: libc OK, reuses seed/test_assert.h.
 *
 * THIS is the Law-2 centerpiece for initech-509.8's acceptance criteria:
 *   "A critical error presents MSG-DOS-0001 and processes Abort/Retry/Fail;
 *    vectors saved/restored across EXEC/EXIT."
 * It pins, HOST-SIDE through the REAL artifact int21.c / psp.c (the same TUs the
 * kernel runs), the logic the in-emulator keystone (make test-vect) exercises
 * end-to-end. The emu gate proves the WIRING (live IDT save/restore across a real
 * EXEC/EXIT); this gate proves the LOGIC is correct and -- crucially -- is the
 * mutation-proof (Rule 6): the emu restore path relies on psp_save_vectors /
 * psp_load_vectors being correct, which the PSP_MUTATE_VEC_OFFSET mutant here
 * bites (see make test-int24-mutant).
 *
 * Ref: docs/adr/ADR-0003 Sec 5.10 (DEC-10: INT 22h/23h/24h handlers; 24h presents
 *      MSG-DOS-0001 + processes A/R/F; PSP 0Ah-15h save/restore across EXEC/EXIT);
 *      App C (MSG-DOS-0001 = "Abort, Retry, Fail?"); spec/dos_messages.json;
 *      os/milton/int21.h (crit_error_action, int22/23/24_dispatch, the seams);
 *      os/milton/psp.h (psp_save_vectors / psp_load_vectors + PSP_SAVED_VEC*_OFF).
 *      CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *      (mutation-prove), Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact int21.c + psp.c (+ sft.c for the
 * handle layer int21.c references, + irq.c for the reentrancy guard). Strategy
 * mirrors test_int21.c / test_conin.c: bind a CAPTURING CON sink, a QUEUED MOCK
 * conin source, a recording EXIT hook, and a MOCK vector table -- then drive
 * int24_dispatch / int22_dispatch / int23_dispatch / int21_dispatch (25h/35h)
 * with synthesized frames and assert AL / CF / ES / the captured CON text / the
 * recorded set-vector tuple / the terminate observation.
 *
 * MUTATION (Rule 6), driven by make (test-int24-mutant) -- each MUST go RED:
 *   -DCRIT_MUTATE_AF_SWAP        : crit_error_action swaps Abort<->Fail
 *                                  -> sections [1] + [2] RED (mapping + AL).
 *   -DINT24_MUTATE_NO_REPROMPT   : int24 accepts the first (invalid) key, no loop
 *                                  -> section [2b] RED (no re-prompt; wrong AL).
 *   -DPSP_MUTATE_VEC_OFFSET      : psp_save_vectors writes the 24h slot at the
 *                                  wrong offset -> section [3] RED (round-trip).
 *   -DGETVECT_MUTATE_AX          : do_getvect returns the handler in EAX not EBX
 *                                  -> section [4] RED (wrong register).
 * A mutant that PASSES means the oracle is decoration.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "int21.h"
#include "psp.h"            /* psp_save_vectors / psp_load_vectors + the offsets */
#include "test_assert.h"
#include "dos_messages.h"   /* MSG_DOS_0001 (the SAME macro int21.c emits) */

TEST_HARNESS();

/* --- The capturing CON sink -------------------------------------------- */
static char   g_sink_buf[512];
static size_t g_sink_len;

static void sink_capture(char c)
{
    if (g_sink_len < sizeof(g_sink_buf) - 1) {
        g_sink_buf[g_sink_len++] = c;
    }
}

static void sink_reset(void)
{
    g_sink_len = 0;
    g_sink_buf[0] = '\0';
}

static const char *sink_str(void)
{
    g_sink_buf[g_sink_len] = '\0';
    return g_sink_buf;
}

/* Count non-overlapping occurrences of `needle` in the captured CON text -- used
 * to prove the int24 RE-PROMPT (MSG-DOS-0001 appears TWICE after one bad key). */
static int sink_count(const char *needle)
{
    int n = 0;
    const char *p = sink_str();
    size_t len = strlen(needle);
    if (len == 0) {
        return 0;
    }
    for (;;) {
        const char *hit = strstr(p, needle);
        if (!hit) {
            return n;
        }
        n++;
        p = hit + len;
    }
}

/* --- The MOCK conin source: a queued string ----------------------------- *
 * mock_get is BLOCKING -- it dequeues the next byte and ABORTS the test on
 * underflow (so int24's re-prompt loop, fed a bad key with no follow-up, can
 * never hang the oracle). int24 reads via the blocking path (conin_get_pb). */
static const uint8_t *g_q;
static size_t         g_q_len;
static size_t         g_q_pos;

static void queue_set(const char *s, size_t n)
{
    g_q     = (const uint8_t *)s;
    g_q_len = n;
    g_q_pos = 0;
}

static int mock_get(void)
{
    if (g_q_pos >= g_q_len) {
        fprintf(stderr,
                "  FATAL %s: mock_get underflow -- int24 read more keys than "
                "queued (a re-prompt loop with no valid key?)\n", __FILE__);
        exit(2);
    }
    return g_q[g_q_pos++];
}

static int mock_poll(void)
{
    if (g_q_pos >= g_q_len) {
        return -1;
    }
    return g_q[g_q_pos++];
}

/* --- The observable terminate hook (int22/int23 route here) ------------- */
static int     g_exit_called;
static uint8_t g_exit_code;

static void exit_observe(uint8_t code)
{
    g_exit_called = 1;
    g_exit_code   = code;
}

/* --- The MOCK vector table (SETVECT/GETVECT seam) ----------------------- *
 * do_setvect calls g_setvect(vec, handler); we record the tuple. do_getvect
 * calls g_getvect(vec); we return a per-vector canned handler. */
static int      g_setvect_called;
static uint8_t  g_setvect_vec;
static uint32_t g_setvect_handler;

static void mock_setvect(uint8_t vec, uint32_t handler)
{
    g_setvect_called  = 1;
    g_setvect_vec     = vec;
    g_setvect_handler = handler;
}

static uint32_t g_getvect_return;   /* what mock_getvect hands back */
static uint8_t  g_getvect_seen_vec;

static uint32_t mock_getvect(uint8_t vec)
{
    g_getvect_seen_vec = vec;
    return g_getvect_return;
}

/* CF: bit 0 of EFLAGS. Preload "wrong" so a dispatcher that forgets to write it
 * is caught. */
#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;   /* IF + reserved bit1; CF clear initially */
    return f;
}

/* ===================================================================== */
int main(void)
{
    int21_set_sink(sink_capture);
    int21_set_exit(exit_observe);
    int21_set_conin(mock_get, mock_poll);
    int21_set_vectortable(mock_setvect, mock_getvect);

    /* =================================================================
     * [1] crit_error_action: the PURE A/R/F decision mapping.
     *     'R'/'r'->0 Retry, 'A'/'a'->1 Abort, 'F'/'f'->2 Fail; anything else
     *     -> -1 (re-prompt). Exhaustive enough to pin the mapping (Law 2).
     * ================================================================= */
    printf("[1] crit_error_action mapping (A/R/F + invalid -> -1)\n");
    CHECK(crit_error_action('A') == 1, "crit_error_action('A') == 1 (Abort)");
    CHECK(crit_error_action('a') == 1, "crit_error_action('a') == 1 (Abort, lc)");
    CHECK(crit_error_action('R') == 0, "crit_error_action('R') == 0 (Retry)");
    CHECK(crit_error_action('r') == 0, "crit_error_action('r') == 0 (Retry, lc)");
    CHECK(crit_error_action('F') == 2, "crit_error_action('F') == 2 (Fail)");
    CHECK(crit_error_action('f') == 2, "crit_error_action('f') == 2 (Fail, lc)");
    CHECK(crit_error_action(' ')  == -1, "crit_error_action(' ')  -> -1 (re-prompt)");
    CHECK(crit_error_action('X')  == -1, "crit_error_action('X')  -> -1 (re-prompt)");
    CHECK(crit_error_action('I')  == -1, "crit_error_action('I')  -> -1 (Ignore deferred)");
    CHECK(crit_error_action(0)    == -1, "crit_error_action(NUL)  -> -1 (re-prompt)");
    CHECK(crit_error_action('Z')  == -1, "crit_error_action('Z')  -> -1 (re-prompt)");

    /* =================================================================
     * [2] int24_dispatch: present MSG-DOS-0001, read a key, return the A/R/F
     *     action in AL with CF clear. Re-prompt on an invalid key.
     * ================================================================= */
    printf("[2] int24_dispatch: MSG-DOS-0001 + A/R/F + re-prompt\n");

    /* [2a] A single 'A' -> AL=1, CF clear, MSG-DOS-0001 emitted ONCE. */
    {
        sink_reset();
        queue_set("A", 1);
        int_frame_t f = fresh_frame();
        f.eflags |= CF_BIT;        /* preload CF=1 so we prove it is CLEARED */
        int24_dispatch(&f);
        CHECK(strstr(sink_str(), MSG_DOS_0001) != NULL,
              "[2a] int24 emits MSG-DOS-0001 (\"Abort, Retry, Fail?\") to CON");
        CHECK(sink_count(MSG_DOS_0001) == 1,
              "[2a] a single valid key prompts EXACTLY once (no spurious re-prompt)");
        CHECK((uint8_t)(f.eax & 0xFFu) == 1u, "[2a] 'A' -> AL=1 (Abort)");
        CHECK(frame_cf(&f) == 0, "[2a] int24 clears CF (returns the action, not an error)");
    }

    /* [2b] An INVALID key then 'A' -> RE-PROMPT: MSG-DOS-0001 appears TWICE and
     *      AL=1. This is the load-bearing re-prompt loop (no silent default). */
    {
        sink_reset();
        queue_set("ZA", 2);        /* 'Z' invalid -> re-prompt; 'A' -> Abort */
        int_frame_t f = fresh_frame();
        f.eflags |= CF_BIT;
        int24_dispatch(&f);
        CHECK(sink_count(MSG_DOS_0001) == 2,
              "[2b] invalid key RE-PROMPTS: MSG-DOS-0001 appears TWICE");
        CHECK((uint8_t)(f.eax & 0xFFu) == 1u, "[2b] re-prompt then 'A' -> AL=1 (Abort)");
        CHECK(frame_cf(&f) == 0, "[2b] int24 clears CF after the re-prompt");
    }

    /* [2c] 'R' -> AL=0 (Retry). */
    {
        sink_reset();
        queue_set("R", 1);
        int_frame_t f = fresh_frame();
        f.eflags |= CF_BIT;
        int24_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "[2c] 'R' -> AL=0 (Retry)");
        CHECK(frame_cf(&f) == 0, "[2c] int24('R') clears CF");
    }

    /* [2d] 'F' -> AL=2 (Fail). */
    {
        sink_reset();
        queue_set("F", 1);
        int_frame_t f = fresh_frame();
        f.eflags |= CF_BIT;
        int24_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 2u, "[2d] 'F' -> AL=2 (Fail)");
        CHECK(frame_cf(&f) == 0, "[2d] int24('F') clears CF");
    }

    /* =================================================================
     * [3] psp_save_vectors / psp_load_vectors: the EXEC-save / EXIT-restore
     *     primitives. The 12 saved bytes must be EXACTLY the three flat
     *     handlers little-endian at offsets 0/4/8 (PSP 0x0A/0x0E/0x12);
     *     neighbouring PSP bytes must be untouched; the load round-trips.
     * ================================================================= */
    printf("[3] psp_save_vectors / psp_load_vectors round-trip (PSP 0x0A-0x15)\n");
    {
        /* Guard the PSP buffer with a poison fill so we can prove ONLY the 12
         * saved-vector bytes change. */
        psp_t psp;
        memset(&psp, 0xCCu, sizeof(psp));   /* poison every byte */

        const uint32_t V22 = 0x11223344u;
        const uint32_t V23 = 0x55667788u;
        const uint32_t V24 = 0x99AABBCCu;
        psp_save_vectors(&psp, V22, V23, V24);

        const uint8_t *sv = psp.saved_vectors;   /* the 12-byte field at PSP 0x0A */

        /* INT 22h @ +0 little-endian. */
        CHECK(sv[0] == 0x44u && sv[1] == 0x33u && sv[2] == 0x22u && sv[3] == 0x11u,
              "[3] v22 stored little-endian at saved_vectors[0..3] (PSP 0x0A)");
        /* INT 23h @ +4 little-endian. */
        CHECK(sv[4] == 0x88u && sv[5] == 0x77u && sv[6] == 0x66u && sv[7] == 0x55u,
              "[3] v23 stored little-endian at saved_vectors[4..7] (PSP 0x0E)");
        /* INT 24h @ +8 little-endian. */
        CHECK(sv[8] == 0xCCu && sv[9] == 0xBBu && sv[10] == 0xAAu && sv[11] == 0x99u,
              "[3] v24 stored little-endian at saved_vectors[8..11] (PSP 0x12)");

        /* The offset constants are exactly 0/4/8 within the field (pins the
         * 22h/23h/24h slot order the loader + the emu gate rely on). */
        CHECK(PSP_SAVED_VEC22_OFF == 0u && PSP_SAVED_VEC23_OFF == 4u &&
              PSP_SAVED_VEC24_OFF == 8u,
              "[3] saved-vector slot offsets are 22h@0 / 23h@4 / 24h@8");

        /* Neighbouring PSP bytes are UNTOUCHED: 0x09 (reserved_04's last byte,
         * just below the field) and 0x16 (parent_psp's low byte, just above) must
         * still hold the 0xCC poison -- psp_save_vectors writes ONLY 0x0A..0x15. */
        const uint8_t *raw = (const uint8_t *)&psp;
        CHECK(raw[0x09] == 0xCCu,
              "[3] PSP byte 0x09 (below saved_vectors) untouched by save");
        CHECK(raw[0x16] == 0xCCu,
              "[3] PSP byte 0x16 (above saved_vectors) untouched by save");

        /* Round-trip: psp_load_vectors reads back exactly what was saved. */
        uint32_t r22 = 0, r23 = 0, r24 = 0;
        psp_load_vectors(&psp, &r22, &r23, &r24);
        CHECK(r22 == V22, "[3] psp_load_vectors recovers v22 (0x11223344)");
        CHECK(r23 == V23, "[3] psp_load_vectors recovers v23 (0x55667788)");
        CHECK(r24 == V24, "[3] psp_load_vectors recovers v24 (0x99AABBCC)");
    }

    /* =================================================================
     * [4] do_setvect (AH=25h) / do_getvect (AH=35h) through int21_dispatch.
     *     25h: AL=vec, EDX=flat handler -> the seam records (vec, handler), CF
     *     clear. 35h: AL=vec -> returns the handler in EBX, ES=0, CF clear.
     * ================================================================= */
    printf("[4] AH=25h SETVECT / AH=35h GETVECT (the vector-table seam)\n");

    /* [4a] SETVECT 0x24 with handler 0xDEADBEEF. */
    {
        g_setvect_called = 0;
        g_setvect_vec = 0; g_setvect_handler = 0;
        int_frame_t f = fresh_frame();
        f.eax = 0x2524u;            /* AH=25h, AL=0x24 */
        f.edx = 0xDEADBEEFu;        /* flat handler address */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(g_setvect_called == 1, "[4a] AH=25h reaches the SETVECT seam");
        CHECK(g_setvect_vec == 0x24u, "[4a] AH=25h passes AL (0x24) as the vector");
        CHECK(g_setvect_handler == 0xDEADBEEFu,
              "[4a] AH=25h passes EDX (0xDEADBEEF) as the handler");
        CHECK(frame_cf(&f) == 0, "[4a] AH=25h clears CF (DOS 25h has no error path)");
    }

    /* [4b] GETVECT 0x24 -> EBX=0xCAFEF00D, ES=0, CF clear. */
    {
        g_getvect_return  = 0xCAFEF00Du;
        g_getvect_seen_vec = 0;
        int_frame_t f = fresh_frame();
        f.eax = 0x3524u;            /* AH=35h, AL=0x24 */
        f.ebx = 0x0u;
        f.es  = 0xFFFFu;            /* poison: do_getvect must zero ES */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(g_getvect_seen_vec == 0x24u, "[4b] AH=35h passes AL (0x24) as the vector");
        CHECK(f.ebx == 0xCAFEF00Du,
              "[4b] AH=35h returns the handler in EBX (0xCAFEF00D)");
        CHECK(f.es == 0u, "[4b] AH=35h sets ES=0 (flat model)");
        CHECK(frame_cf(&f) == 0, "[4b] AH=35h clears CF (DOS 35h has no error path)");
    }

    /* =================================================================
     * [5] int22_dispatch / int23_dispatch: the terminate + control-break
     *     handlers. 22h terminates (exit hook). 23h emits "INT23-BREAK" AND
     *     terminates. Both route through do_terminate -> the exit hook.
     * ================================================================= */
    printf("[5] int22_dispatch (terminate) / int23_dispatch (break + terminate)\n");

    /* [5a] INT 22h -> the exit hook fires (terminate), code 0. */
    {
        g_exit_called = 0; g_exit_code = 0xFFu;
        int_frame_t f = fresh_frame();
        int22_dispatch(&f);
        CHECK(g_exit_called == 1, "[5a] INT 22h calls the terminate hook");
        CHECK(g_exit_code == 0u, "[5a] INT 22h terminates with code 0");
    }

    /* [5b] INT 23h -> "INT23-BREAK" emitted AND the exit hook fires. */
    {
        sink_reset();
        g_exit_called = 0; g_exit_code = 0xFFu;
        int_frame_t f = fresh_frame();
        int23_dispatch(&f);
        CHECK(strstr(sink_str(), "INT23-BREAK") != NULL,
              "[5b] INT 23h emits the \"INT23-BREAK\" marker to CON");
        CHECK(g_exit_called == 1, "[5b] INT 23h calls the terminate hook (break-abort)");
        CHECK(g_exit_code == 0u, "[5b] INT 23h terminates with code 0");
    }

    return TEST_SUMMARY("test_int24");
}
