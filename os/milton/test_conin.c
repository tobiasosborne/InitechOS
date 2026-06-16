/* test_conin.c -- host unit oracle for the INT 21h CON INPUT functions
 * (beads initech-n62). Factory test: libc OK, reuses seed/test_assert.h.
 *
 * Ref: DOS 3.3 Programmer's Reference Manual AH=01h (char in w/ echo), AH=06h
 *      (direct console I/O, DL=FF input / else output), AH=07h (direct char in,
 *      no echo, no ^C), AH=08h (char in, no echo), AH=0Ah (buffered input),
 *      AH=0Bh (get input status), AH=0Ch (flush + invoke input);
 *      os/milton/int21.h int21_set_conin (the INPUT SOURCE seam). CLAUDE.md
 *      Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6 (mutation-prove),
 *      Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact int21.c (the same int21_dispatch the
 * kernel runs). Strategy mirrors test_int21.c: the dispatcher routes ALL echo
 * bytes through an int21_sink_fn we override to capture, and reads input through
 * an int21_conin source we bind to a QUEUED MOCK STRING -- the blocking `get`
 * dequeues the next char and ABORTS the test on underflow (never hangs), the
 * non-blocking `poll` returns -1 at end-of-queue. We build a fake int_frame_t,
 * call int21_dispatch, and assert AL / the buffered-input buffer + count byte /
 * the echo capture / the ZF bit (06h-input, via EFLAGS bit 6) / CF.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DINT21_MUTATE_CONIN_NO_ECHO     : 01h drops the echo -> the 01h echo test goes RED.
 *   -DINT21_MUTATE_BUFINPUT_COUNT_CR : 0Ah counts the CR in buf[1] -> the 0Ah
 *                                      count test goes RED.
 *   -DINT21_MUTATE_CONHANDLE_NOCOOKED: AH=3Fh on a CON handle reverts to the old
 *                                      EOF return (0 bytes) -> the x8fs handle-0
 *                                      cooked-read cases go RED (beads initech-x8fs).
 *   -DINT21_MUTATE_NO_CTRLC_CHECK    : the CON-input ^C check-point is dropped, so
 *                                      0x03 is delivered as a raw char and INT 23h
 *                                      is NEVER invoked -> the [4tw.*] ^C cases go
 *                                      RED (beads initech-4tw; the M5 mutant of
 *                                      DEC-16 Sec 7.3).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "sft.h"
#include "psp.h"
#include "test_assert.h"

TEST_HARNESS();

/* AH=3Fh on a CON handle resolves EBX through the current process's JFT into the
 * system SFT (do_read -> sft_from_handle), so the handle-0 cooked-read cases
 * (beads initech-x8fs) need a bound process: psp_build lays jft[0..4] onto the
 * device SFT slots (jft[0] -> SFT_SLOT_CON_IN, a CON read device) and sft_init
 * lays the device entries. Mirrors test_int21.c's bind_standard_process(). */
static psp_t g_x8fs_psp;

static void bind_standard_process(void)
{
    psp_params_t params;
    params.alloc_end_linear  = 0x00070000u;
    params.env_linear        = 0u;
    params.parent_psp_linear = 0u;
    params.cmd_tail          = (const char *)0;
    params.cmd_tail_len      = 0u;
    (void)psp_build(&g_x8fs_psp, &params);
    sft_init();
    int21_set_psp(&g_x8fs_psp);
}

/* AH=0Ah reads EDX as a FLAT 32-bit linear address (uint32_t). On a 64-bit host
 * a stack buffer does NOT fit in uint32_t, so the 0Ah buffer must live in the
 * low 4 GiB. Mirror test_int21.c's alloc_low (MAP_32BIT) so (uint32_t)(uintptr_t)
 * p round-trips losslessly -- exactly the artifact's <4 GiB world. */
static uint8_t *alloc_low_buf(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED || (uintptr_t)p > 0xFFFFFFFFu) {
        fprintf(stderr, "  FATAL %s: low-4GiB buffer unavailable\n", __FILE__);
        exit(2);
    }
    return (uint8_t *)p;
}

/* --- The capturing CON sink (echo) -------------------------------------- */
static char   g_sink_buf[256];
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

/* True if the CON sink captured the grep-able INT 23h break marker. int23's
 * default handler (int23_dispatch_body) emits "INT23-BREAK\n" to the CON sink
 * BEFORE the terminate, so a CON-input ^C check-point that invoked INT 23h
 * leaves this marker in the capture -- the observable signal that 0x03 routed
 * to the break handler rather than being delivered as an ordinary char.
 * Ref: os/milton/int21.c int23_dispatch_body; DEC-16 Sec 7.2 (4tw ^C oracle). */
static int sink_saw_int23(void)
{
    g_sink_buf[g_sink_len] = '\0';
    return strstr(g_sink_buf, "INT23-BREAK") != NULL;
}

/* The second, independent int23 witness: int23 routes to do_terminate, which
 * calls the bound exit hook. We observe BOTH (sink marker + exit-hook fire) so a
 * single mutated observable cannot mask a regression. */
static int g_exit_called;
static void exit_observe(uint8_t code) { (void)code; g_exit_called = 1; }

/* --- The MOCK input source: a queued string ----------------------------- *
 * mock_get is the BLOCKING source -- it dequeues the next byte and ABORTS the
 * test (via TEST_FATAL) on underflow, so a wrong blocking call can never hang
 * the oracle. mock_poll is NON-blocking: it returns the next byte, or -1 at the
 * end of the queue. */
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
        /* Underflow: the dispatcher asked for a char the test did not queue.
         * Fail loud (non-zero exit) rather than hang or return garbage -- the
         * brief's stop condition for the blocking path. */
        fprintf(stderr,
                "  FATAL %s: mock_get underflow -- a blocking read asked for "
                "more input than queued\n", __FILE__);
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

/* CF / ZF: bit 0 / bit 6 of EFLAGS. Preload "wrong" so a dispatcher that
 * forgets to write the bit is caught. */
#define CF_BIT 0x1u
#define ZF_BIT 0x40u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }
static int frame_zf(const int_frame_t *f) { return (f->eflags & ZF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;  /* IF + reserved bit1; CF and ZF clear initially */
    return f;
}

int main(void)
{
    int21_set_sink(sink_capture);
    int21_set_conin(mock_get, mock_poll);
    /* Bind the terminate hook so int23's default handler (do_terminate -> g_exit)
     * is observable in the [4tw.*] ^C cases without actually exiting the test
     * process. With no hook bound do_terminate just returns (the host default);
     * with it bound we get a second witness alongside the CON "INT23-BREAK"
     * marker. Ref: os/milton/int21.c int23_dispatch_body / do_terminate. */
    int21_set_exit(exit_observe);
    /* Bind a process so AH=3Fh on handle 0 resolves to the CON read device. */
    bind_standard_process();

    /* --- AH=01h CHAR INPUT WITH ECHO: queue 'A' -> AL='A', echo 'A'. ----- */
    {
        sink_reset();
        queue_set("A", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0100u;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 'A', "AH=01h returns AL = the input char");
        CHECK_STR_EQ(sink_str(), "A", "AH=01h echoes the char to CON");
        CHECK(frame_cf(&f) == 0, "AH=01h clears CF");
    }

    /* --- AH=07h DIRECT CHAR INPUT (no echo): queue 'Q' -> AL='Q', NO echo. */
    {
        sink_reset();
        queue_set("Q", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0700u;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 'Q', "AH=07h returns AL = the input char");
        CHECK_STR_EQ(sink_str(), "", "AH=07h does NOT echo");
        CHECK(frame_cf(&f) == 0, "AH=07h clears CF");
    }

    /* --- AH=08h CHAR INPUT (no echo): queue 'Z' -> AL='Z', NO echo. ------ */
    {
        sink_reset();
        queue_set("Z", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0800u;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 'Z', "AH=08h returns AL = the input char");
        CHECK_STR_EQ(sink_str(), "", "AH=08h does NOT echo");
    }

    /* --- AH=06h DL=FF INPUT, char ready: ZF=0, AL=char, NO echo. -------- */
    {
        sink_reset();
        queue_set("k", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0600u;
        f.edx = 0xFFu;            /* DL=FF -> input direction */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 'k', "AH=06h DL=FF returns the ready char in AL");
        CHECK(frame_zf(&f) == 0, "AH=06h DL=FF with a char clears ZF");
        CHECK_STR_EQ(sink_str(), "", "AH=06h input does NOT echo");
        CHECK(frame_cf(&f) == 0, "AH=06h clears CF");
    }

    /* --- AH=06h DL=FF INPUT, NO char: ZF=1, AL=0, no wait. -------------- */
    {
        sink_reset();
        queue_set("", 0);         /* empty queue -> poll returns -1 */
        int_frame_t f = fresh_frame();
        f.eax = 0x0600u;
        f.edx = 0xFFu;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "AH=06h DL=FF no char -> AL=0");
        CHECK(frame_zf(&f) == 1, "AH=06h DL=FF no char SETS ZF (no character available)");
    }

    /* --- AH=06h DL!=FF OUTPUT: emit DL to CON, AL=DL. ------------------- */
    {
        sink_reset();
        queue_set("", 0);
        int_frame_t f = fresh_frame();
        f.eax = 0x0600u;
        f.edx = (uint32_t)'!';    /* DL='!' -> output direction */
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "!", "AH=06h DL!=FF outputs DL to CON");
        CHECK((uint8_t)(f.eax & 0xFFu) == '!', "AH=06h output returns AL=DL");
    }

    /* --- AH=0Bh GET INPUT STATUS: char available -> AL=0xFF, NOT consumed. */
    {
        sink_reset();
        queue_set("x", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0B00u;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0xFFu, "AH=0Bh available -> AL=0xFF");

        /* The char must NOT have been consumed: a following AH=07h still gets it. */
        int_frame_t g = fresh_frame();
        g.eax = 0x0700u;
        int21_dispatch(&g);
        CHECK((uint8_t)(g.eax & 0xFFu) == 'x', "AH=0Bh did NOT consume the char (07h still reads 'x')");
    }

    /* --- AH=0Bh GET INPUT STATUS: NO char -> AL=0x00. ------------------- */
    {
        queue_set("", 0);
        int_frame_t f = fresh_frame();
        f.eax = 0x0B00u;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0x00u, "AH=0Bh no char -> AL=0x00");
    }

    /* --- AH=0Ah BUFFERED INPUT, simple line "dir" + CR. ----------------- *
     * buf[0]=max=8 (incl. CR). Queue "dir\r". Expect: buf[1]=3 (NOT counting
     * CR), buf[2..4]="dir", buf[5]=CR, echo "dir\r\n". */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 8u;
        queue_set("dir\r", 4);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(buf[1] == 3u, "AH=0Ah count byte = 3 (chars read, NOT counting CR)");
        CHECK(buf[2] == 'd' && buf[3] == 'i' && buf[4] == 'r',
              "AH=0Ah stored the chars at buf[2..]");
        CHECK(buf[5] == 0x0Du, "AH=0Ah stored the CR after the chars (not counted)");
        CHECK_STR_EQ(sink_str(), "dir\r\n", "AH=0Ah echoes the line + CRLF");
        CHECK(frame_cf(&f) == 0, "AH=0Ah clears CF");
    }

    /* --- AH=0Ah BUFFERED INPUT: only CR terminates; a stray LF is ordinary. -- *
     * The kbd now decodes Enter to CR (initech-62m), so the old "accept LF as a
     * terminator and normalize to CR" bandaid is retired. This locks the retired
     * behaviour: a lone LF (0x0A) is stored as an ORDINARY char and does NOT end
     * the line -- only CR does. Queue "a\nb\r" -> buf[1]=3, buf[2..4]='a',0x0A,
     * 'b', buf[5]=0x0D. (Re-introducing `|| c == 0x0Au` would flip buf[1] to 1
     * and drop the LF/'b', so this CHECK bites the bandaid's return.) */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 8u;
        queue_set("a\nb\r", 4);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(buf[1] == 3u, "AH=0Ah: LF is not a terminator -> count = 3 ('a',LF,'b')");
        CHECK(buf[2] == 'a' && buf[3] == 0x0Au && buf[4] == 'b',
              "AH=0Ah: a stray LF is stored as an ordinary char");
        CHECK(buf[5] == 0x0Du, "AH=0Ah: only CR terminates and is stored at the end");
    }

    /* --- AH=0Ah BUFFERED INPUT with a BACKSPACE edit. ------------------- *
     * Queue "du\bir\r": type "du", backspace (erase 'u'), type "ir", CR ->
     * "dir". buf[1]=3, buf[2..4]="dir". Echo includes the "\b \b" erase. */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 8u;
        queue_set("du\bir\r", 6);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(buf[1] == 3u, "AH=0Ah after BACKSPACE: count = 3");
        CHECK(buf[2] == 'd' && buf[3] == 'i' && buf[4] == 'r',
              "AH=0Ah after BACKSPACE: buffer = 'dir'");
        CHECK(buf[5] == 0x0Du, "AH=0Ah after BACKSPACE: CR stored after 'dir'");
        /* Echo: 'd','u', then erase ('\b',' ','\b'), then 'i','r', then CRLF. */
        CHECK_STR_EQ(sink_str(), "du\b \bir\r\n",
                     "AH=0Ah BACKSPACE echoes the visual erase '\\b \\b'");
    }

    /* --- AH=0Ah BUFFERED INPUT: BACKSPACE on an EMPTY buffer is ignored. */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 8u;
        queue_set("\bX\r", 3);    /* backspace with nothing buffered, then "X" */
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(buf[1] == 1u, "AH=0Ah BACKSPACE-when-empty ignored: count = 1");
        CHECK(buf[2] == 'X', "AH=0Ah BACKSPACE-when-empty: buffer = 'X'");
        CHECK_STR_EQ(sink_str(), "X\r\n",
                     "AH=0Ah BACKSPACE-when-empty emits nothing for the BS");
    }

    /* --- AH=0Ah BUFFERED INPUT: MAX-LENGTH CLAMP (the shell case). ------- *
     * buf[0]=4 (max incl. CR) -> room for 3 chars + CR. Queue "abcdef\r":
     * "abc" fit, "def" are ignored (BEL each), then CR. buf[1]=3, buf[2..4]=
     * "abc", buf[5]=CR. No overflow past buf[5] (Rule 2). */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 4u;              /* 3 chars + CR */
        queue_set("abcdef\r", 7);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(buf[1] == 3u, "AH=0Ah clamp: count = max-1 = 3");
        CHECK(buf[2] == 'a' && buf[3] == 'b' && buf[4] == 'c',
              "AH=0Ah clamp: stored exactly the first 3 chars");
        CHECK(buf[5] == 0x0Du, "AH=0Ah clamp: CR stored at buf[2+count]");
        CHECK(buf[6] == 0xCCu, "AH=0Ah clamp: did NOT overflow past the CR (Rule 2)");
        /* Echo: "abc" then 3 BELs (the dropped d,e,f) then CRLF. */
        CHECK_STR_EQ(sink_str(), "abc\a\a\a\r\n",
                     "AH=0Ah clamp echoes 'abc', a BEL per dropped char, then CRLF");
    }

    /* --- AH=0Ah BUFFERED INPUT: FULL-SIZE buffer buf[0]=255. ------------- *
     * Regression for the CR-store guard. The old form
     *   (uint8_t)(2u+count) < (uint8_t)(2u+max)
     * WRAPPED for max>=254 (2u+max overflows uint8_t -> 0/1) and silently
     * dropped the terminating CR on a legal max-size request. With the fix
     * (count < max) the CR is stored. Queue "hi\r": count=2, CR at buf[4]. The
     * INT21_MUTATE_BUFINPUT_CR_WRAP mutant restores the bug -> this goes RED. */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(260);
        memset(buf, 0xCC, 260);
        buf[0] = 255u;            /* max incl. CR -- the uint8_t-wrap trigger */
        queue_set("hi\r", 3);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(buf[1] == 2u, "AH=0Ah max=255: count = 2 (CR not counted)");
        CHECK(buf[2] == 'h' && buf[3] == 'i', "AH=0Ah max=255: buffer = 'hi'");
        CHECK(buf[4] == 0x0Du,
              "AH=0Ah max=255: CR stored at buf[2+count] (no uint8_t wrap)");
        CHECK_STR_EQ(sink_str(), "hi\r\n", "AH=0Ah max=255: echoes 'hi' + CRLF");
    }

    /* --- AH=0Ch FLUSH + INVOKE. The mock is a single synchronous queue, so we
     * test the two observable halves of 0Ch without needing "input that arrives
     * after the flush" (which a flat queue cannot model). ----------------- */
    {
        /* Sub-case A: AL is NOT a chainable function (0x99) -> flush-only,
         * CF clear, and the pending queue is drained. */
        queue_set("junk", 4);
        int_frame_t f = fresh_frame();
        f.eax = 0x0C00u | 0x99u;  /* AH=0Ch, AL=0x99 (not 01/06/07/08/0A) */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=0Ch flush-only (AL not chainable) clears CF");
        CHECK(mock_poll() < 0, "AH=0Ch flushed all pending input");
    }
    {
        /* Sub-case B: AL=06h chains DIRECT CONSOLE I/O. 0Ch first FLUSHES the
         * stale queue, so the chained 06h-input (DL=FF) finds nothing -> ZF=1,
         * AL=0. This proves BOTH the flush ran (the stale "stale" did not
         * satisfy the read) AND the chain dispatched to 06h. */
        sink_reset();
        queue_set("stale", 5);
        int_frame_t f = fresh_frame();
        f.eax = 0x0C00u | 0x06u;  /* AH=0Ch, AL=06h -> flush then 06h */
        f.edx = 0xFFu;            /* DL=FF -> 06h input direction */
        int21_dispatch(&f);
        CHECK(frame_zf(&f) == 1,
              "AH=0Ch->06h: flush drained the stale input, chained poll reports ZF=1");
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "AH=0Ch->06h: no char -> AL=0");
    }

    /* === AH=3Fh on CON (handle 0): COOKED line read (beads initech-x8fs) ====
     * Real DOS reads a character device handle in COOKED mode through the same
     * line editor as AH=0Ah, but the handle-read CONTRACT differs: the data
     * returned is the line FOLLOWED BY CR (0x0D) AND LF (0x0A), and the byte
     * count in EAX INCLUDES both the CR and the LF. (AH=0Ah, by contrast, stores
     * only the CR and excludes it from buf[1].) Concretely: type "abc"+Enter with
     * a large count -> buffer "abc\r\n", EAX = 5.
     * Ground truth: Microsoft KB Q113058 "Using Interrupt 21h, Function 3Fh to
     * Read the Keyboard" -- buffer receives "abc\r\n" (5 bytes) and AX returns 5;
     * the line is terminated by CR and the CR/LF are returned to the caller as
     * data. Echo + BACKSPACE mirror the cooked AH=0Ah editor. */

    /* --- AH=3Fh handle 0: simple line "abc" + CR -> "abc\r\n", EAX=5. ------- */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(32);
        memset(buf, 0xCC, 32);
        queue_set("abc\r", 4);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u;
        f.ebx = 0u;                          /* handle 0 = stdin (CON read) */
        f.ecx = 16u;                         /* plenty of room */
        f.edx = (uint32_t)(uintptr_t)buf;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK((f.eax & 0xFFFFu) == 5u,
              "AH=3Fh CON: EAX = 5 (3 chars + CR + LF, inclusive count)");
        CHECK(buf[0] == 'a' && buf[1] == 'b' && buf[2] == 'c',
              "AH=3Fh CON: line chars at buf[0..2]");
        CHECK(buf[3] == 0x0Du, "AH=3Fh CON: CR (0x0D) stored after the chars");
        CHECK(buf[4] == 0x0Au, "AH=3Fh CON: LF (0x0A) stored after the CR");
        CHECK(buf[5] == 0xCCu, "AH=3Fh CON: no write past the LF (Rule 2)");
        CHECK_STR_EQ(sink_str(), "abc\r\n",
                     "AH=3Fh CON: echoes the line + CRLF (cooked)");
        CHECK(frame_cf(&f) == 0, "AH=3Fh CON: clears CF on success");
    }

    /* --- AH=3Fh handle 0: BACKSPACE edit -- "du\bir\r" -> "dir\r\n", EAX=5. -- */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(32);
        memset(buf, 0xCC, 32);
        queue_set("du\bir\r", 6);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u;
        f.ebx = 0u;
        f.ecx = 16u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK((f.eax & 0xFFFFu) == 5u, "AH=3Fh CON BACKSPACE: EAX = 5 ('dir'+CR+LF)");
        CHECK(buf[0] == 'd' && buf[1] == 'i' && buf[2] == 'r',
              "AH=3Fh CON BACKSPACE: buffer = 'dir'");
        CHECK(buf[3] == 0x0Du && buf[4] == 0x0Au,
              "AH=3Fh CON BACKSPACE: CR+LF after 'dir'");
        CHECK_STR_EQ(sink_str(), "du\b \bir\r\n",
                     "AH=3Fh CON BACKSPACE: echoes the visual erase '\\b \\b'");
    }

    /* --- AH=3Fh handle 0: empty line (just Enter) -> "\r\n", EAX=2. -------- */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(32);
        memset(buf, 0xCC, 32);
        queue_set("\r", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u;
        f.ebx = 0u;
        f.ecx = 16u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK((f.eax & 0xFFFFu) == 2u, "AH=3Fh CON empty line: EAX = 2 (CR + LF only)");
        CHECK(buf[0] == 0x0Du && buf[1] == 0x0Au,
              "AH=3Fh CON empty line: buffer = CR,LF");
        CHECK(buf[2] == 0xCCu, "AH=3Fh CON empty line: no write past the LF");
        CHECK_STR_EQ(sink_str(), "\r\n", "AH=3Fh CON empty line: echoes CRLF");
    }

    /* ===== [4tw.*] CON-input ^C (0x03) -> INT 23h check-point ================
     * beads initech-4tw; ADR-0003 Amendment DEC-16 (OEA-ADR-0003-A4, RATIFIED
     * 2026-06-15), Sec 3.3 Fork A + Sec 7.2.
     *
     * GROUND TRUTH (DEC-16 Sec 3.3 + DOS 3.3 PRM): under the ratified Fork A the
     * CON character-input family is ALWAYS a ^C check-point -- whether the BREAK
     * flag is ON or OFF (OFF = CON-I/O check-point ONLY; ON additionally widens to
     * every INT 21h call, the C-6 forward obligation NOT in 4tw's scope). So when
     * 0x03 is read by AH=01h / AH=08h / AH=0Ah it must route to the INT 23h vector
     * (observable here: the "INT23-BREAK" CON marker + the terminate hook fires),
     * NOT be delivered as an ordinary 0x03 char. AH=07h (DIRECT input, NO ^C) and
     * AH=06h (DIRECT console I/O) are the documented EXCEPTIONS -- they deliver
     * 0x03 raw (DOS 3.3 PRM AH=07h "no Ctrl-C"; int21.c:595).
     *
     * The INT21_MUTATE_NO_CTRLC_CHECK mutant drops the check (delivers 0x03 raw,
     * never invokes INT 23h) -> every [4tw.*] check below goes RED (the M5 mutant
     * of DEC-16 Sec 7.3). */

    /* [4tw.1] AH=01h reads 0x03 -> INT 23h (NOT delivered as the char). ------ */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(1u);       /* BREAK ON (CON is a check-point) */
        queue_set("\x03", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0100u;                /* AH=01h CHAR INPUT WITH ECHO */
        int21_dispatch(&f);
        CHECK(sink_saw_int23(),
              "AH=01h ^C: 0x03 invokes the INT 23h break handler (INT23-BREAK marker)");
        CHECK(g_exit_called == 1,
              "AH=01h ^C: INT 23h default action ran the terminate hook");
    }

    /* [4tw.2] AH=08h reads 0x03 -> INT 23h. --------------------------------- */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(1u);
        queue_set("\x03", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0800u;                /* AH=08h CHAR INPUT, no echo */
        int21_dispatch(&f);
        CHECK(sink_saw_int23(),
              "AH=08h ^C: 0x03 invokes the INT 23h break handler");
        CHECK(g_exit_called == 1, "AH=08h ^C: terminate hook ran");
    }

    /* [4tw.3] AH=0Ah (buffered, shared cooked editor) reads 0x03 -> INT 23h. - */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(1u);
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 8u;
        queue_set("\x03", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;                /* AH=0Ah BUFFERED INPUT */
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(sink_saw_int23(),
              "AH=0Ah ^C: 0x03 in the cooked line editor invokes the INT 23h handler");
        CHECK(g_exit_called == 1, "AH=0Ah ^C: terminate hook ran");
    }

    /* [4tw.4] FORK A: the CON ^C check-point fires with BREAK OFF too -- under
     * Fork A the CON family is ALWAYS a check-point (OFF = CON-I/O check-point
     * ONLY, still active). This pins the ratified semantics: 4tw's CON ^C is NOT
     * gated OFF by BREAK=OFF. (Mutating the check off makes this RED as well.) -- */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(0u);       /* BREAK OFF */
        queue_set("\x03", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0800u;                /* AH=08h */
        int21_dispatch(&f);
        CHECK(sink_saw_int23(),
              "AH=08h ^C with BREAK OFF: CON is ALWAYS a check-point (Fork A) -> INT 23h");
        CHECK(g_exit_called == 1, "AH=08h ^C BREAK OFF: terminate hook ran");
        int21_set_break_flag(1u);       /* restore boot default ON */
    }

    /* [4tw.5] AH=07h is the documented EXCEPTION (DIRECT input, NO ^C): 0x03 is
     * delivered RAW in AL, INT 23h is NOT invoked. This is the negative pin that
     * proves the check is scoped to the ^C-checking CON calls, not blanket. ---- */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(1u);       /* even ON, 07h never checks ^C */
        queue_set("\x03", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0700u;                /* AH=07h DIRECT CHAR INPUT, no echo, no ^C */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0x03u,
              "AH=07h ^C: 0x03 delivered RAW in AL (07h is the no-Ctrl-C call)");
        CHECK(!sink_saw_int23(), "AH=07h ^C: INT 23h NOT invoked (no marker)");
        CHECK(g_exit_called == 0, "AH=07h ^C: terminate hook did NOT run");
    }

    /* [4tw.6] AH=06h DIRECT CONSOLE I/O input is likewise NOT a ^C check-point:
     * 0x03 arrives raw in AL with ZF=0. (DOS 3.3 PRM AH=06h direct console I/O.) */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(1u);
        queue_set("\x03", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0600u;
        f.edx = 0xFFu;                  /* DL=FF -> 06h input direction */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0x03u,
              "AH=06h ^C: 0x03 delivered RAW in AL (06h direct console I/O, no ^C)");
        CHECK(!sink_saw_int23(), "AH=06h ^C: INT 23h NOT invoked");
    }

    /* [4tw.7] A NON-^C char on a ^C-checking call is unaffected: AH=01h reads a
     * plain 'A' -> AL='A', echoed, no INT 23h. Guards against a check that fires
     * on the wrong byte. -------------------------------------------------------- */
    {
        sink_reset();
        g_exit_called = 0;
        int21_set_break_flag(1u);
        queue_set("A", 1);
        int_frame_t f = fresh_frame();
        f.eax = 0x0100u;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 'A', "AH=01h non-^C: 'A' delivered normally");
        CHECK(!sink_saw_int23(), "AH=01h non-^C: INT 23h NOT invoked for an ordinary char");
        CHECK(g_exit_called == 0, "AH=01h non-^C: terminate hook did NOT run");
    }

    /* --- NULL source: a blocking read returns 0 (EOF), never hangs/faults. */
    {
        int21_set_conin(0, 0);    /* unbind the source */
        int_frame_t f = fresh_frame();
        f.eax = 0x0700u;          /* 07h blocking get */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "NULL source: blocking get yields 0 (no hang)");

        int_frame_t g = fresh_frame();
        g.eax = 0x0600u;
        g.edx = 0xFFu;            /* 06h poll */
        int21_dispatch(&g);
        CHECK(frame_zf(&g) == 1, "NULL source: 06h poll reports ZF=1 (no char)");
        int21_set_conin(mock_get, mock_poll);  /* restore */
    }

    return TEST_SUMMARY("test_conin");
}
