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
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "test_assert.h"

TEST_HARNESS();

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

    /* --- AH=0Ah BUFFERED INPUT terminated by LF (0x0A). ----------------- *
     * InitechOS's PS/2 driver decodes Enter to LF, so AH=0Ah must accept LF as
     * the line terminator and NORMALIZE the stored byte to CR (0x0D). Queue
     * "ok\n" -> buf[1]=2, buf[2..3]="ok", buf[4]=0x0D (normalized, NOT 0x0A). */
    {
        sink_reset();
        uint8_t *buf = alloc_low_buf(16);
        memset(buf, 0xCC, 16);
        buf[0] = 8u;
        queue_set("ok\n", 3);
        int_frame_t f = fresh_frame();
        f.eax = 0x0A00u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK(buf[1] == 2u, "AH=0Ah LF-terminated: count = 2 (LF not counted)");
        CHECK(buf[2] == 'o' && buf[3] == 'k', "AH=0Ah LF-terminated: buffer = 'ok'");
        CHECK(buf[4] == 0x0Du,
              "AH=0Ah normalizes the LF terminator to CR (0x0D) in the buffer");
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
