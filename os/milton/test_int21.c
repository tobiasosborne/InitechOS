/* test_int21.c -- host unit oracle for the INT 21h dispatch logic
 * (beads initech-509.5). Factory test: libc OK, reuses seed/test_assert.h.
 *
 * Ref: docs/research/internals-int21h-ground-truth.md Sec 5 (flat calling
 *      convention: AH-dispatch, EDX flat ptr, ECX count, EBX handle, EAX
 *      return, CF in saved EFLAGS), Sec 6 (console subset);
 *      spec/int21h_calling_convention.json (LOCKED per-function contract);
 *      spec/int21h_register.json (controlled scope). CLAUDE.md Law 2 (oracle is
 *      truth), Rule 1 (RED->GREEN), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact int21.c (the same int21_dispatch the
 * kernel runs). Strategy: the dispatcher routes ALL "display" bytes through an
 * int21_sink_fn we override to capture into a buffer, and routes terminate
 * through an int21_exit_fn we override to record the code (instead of halting).
 * We build a fake int_frame_t, call int21_dispatch, and assert outputs + the CF
 * bit in frame.eflags.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DINT21_MUTATE_PUTS_EMIT_DOLLAR : 09h emits the '$' too -> the PUTS test goes RED.
 *   -DINT21_MUTATE_UNLISTED_NOOP    : unlisted-AH path becomes a silent no-op
 *                                     (no CF, no diagnostic) -> the controlled-
 *                                     scope test goes RED.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "sft.h"    /* JFT->SFT handle layer (initech-509.3): AH=40h/45h/46h */
#include "psp.h"    /* psp_build -> the standard JFT bound for the write tests */
#include "test_assert.h"

TEST_HARNESS();

/* The handle functions (40h WRITE, 45h DUP, 46h DUP2) resolve EBX/ECX through
 * the current process's JFT into the system SFT. Bind a standard process PSP
 * (psp_build lays jft[0..4] onto the device SFT slots) + sft_init (lays the
 * device entries) so handle 1/2 -> CON-write, handle >=5 closed, etc. */
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

/* ---- CLOCK mock (beads initech-yv9) ---------------------------------------
 * A fixed wall-clock the GET DATE/TIME functions read, and a settable sink the
 * SET DATE/TIME functions write -- so the dispatch + register-packing logic is
 * tested HOSTED with no RTC. Pinned to 2026-06-09 12:34:56 (a Tuesday, DOW=2),
 * the SAME instant the in-emulator test-datetime pins via qemu -rtc base. */
static uint16_t g_mock_year   = 2026u;
static uint8_t  g_mock_month  = 6u, g_mock_day = 9u;
static uint8_t  g_mock_hour   = 12u, g_mock_min = 34u, g_mock_sec = 56u;
static uint8_t  g_mock_dow    = 2u;  /* Tuesday */
static int      g_mock_set_calls = 0;
static uint8_t  g_mock_last_which = 0u;

static int mock_clock_get(uint16_t *y, uint8_t *mo, uint8_t *d,
                          uint8_t *h, uint8_t *mi, uint8_t *s, uint8_t *dow)
{
    *y = g_mock_year; *mo = g_mock_month; *d = g_mock_day;
    *h = g_mock_hour; *mi = g_mock_min;   *s = g_mock_sec; *dow = g_mock_dow;
    return 1;
}
static int mock_clock_set(uint16_t y, uint8_t mo, uint8_t d,
                          uint8_t h, uint8_t mi, uint8_t s, uint8_t which)
{
    /* Range-check exactly as the real RTC encode would (so SET DATE with an
     * invalid month returns AL=0xFF). */
    if (mo < 1u || mo > 12u || d < 1u || d > 31u ||
        h > 23u || mi > 59u || s > 59u) {
        return 0;
    }
    g_mock_set_calls++;
    g_mock_last_which = which;
    if (which & INT21_CLOCK_SET_DATE) {
        g_mock_year = y; g_mock_month = mo; g_mock_day = d;
        g_mock_dow = 0u;   /* the real path re-derives DOW on read */
    }
    if (which & INT21_CLOCK_SET_TIME) {
        g_mock_hour = h; g_mock_min = mi; g_mock_sec = s;
    }
    return 1;
}

/* ---- FREESPACE mock (AH=36h) ---------------------------------------------- */
static uint16_t mock_freespace(uint16_t *spc, uint16_t *bps,
                               uint16_t *total, uint16_t *freec)
{
    *spc = 1u; *bps = 512u; *total = 2847u; *freec = 2840u;
    return 0u;
}
static const int21_file_backend_t g_space_backend = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, mock_freespace,
    NULL,  /* resolve: AH=36h does not resolve a path (beads initech-mzxa) */
    NULL,  /* resolve_dir: AH=36h does not CHDIR a path (beads initech-u6wa) */
    NULL,  /* mkdir: AH=36h does not create a dir (beads initech-u6wa) */
    NULL   /* rmdir: AH=36h does not remove a dir (beads initech-u6wa) */
};

/* The dispatcher reads EDX as a FLAT 32-bit linear address (uint32_t). On a
 * 64-bit host a stack/heap pointer does NOT fit in uint32_t, so any buffer the
 * test hands via EDX must live in the low 4 GiB. Mirror test_console.c's
 * alloc_low (MAP_32BIT) so (uint32_t)(uintptr_t)p round-trips losslessly --
 * exactly the artifact's world where every pointer is < 4 GiB. */
static void *alloc_low(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED) {
        return NULL;
    }
    if ((uintptr_t)p > 0xFFFFFFFFu) {
        munmap(p, n);
        return NULL;
    }
    return p;
}

/* Copy a C string into a fresh low-4-GiB buffer and return its uint32_t flat
 * address (the value the test loads into EDX). Aborts the test (returns 0) if
 * low memory is unavailable -- never false-green on a truncated pointer. */
static uint32_t low_dup(const char *s)
{
    size_t n = strlen(s) + 1u;
    void *p = alloc_low(n);
    if (!p) {
        return 0u;
    }
    memcpy(p, s, n);
    return (uint32_t)(uintptr_t)p;
}

/* --- The capturing CON sink --------------------------------------------- */
static char g_sink_buf[256];
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

/* --- The observable terminate hook -------------------------------------- */
static int      g_exit_called;
static uint8_t  g_exit_code;

static void exit_observe(uint8_t code)
{
    g_exit_called = 1;
    g_exit_code = code;
}

/* CF helpers: bit 0 of EFLAGS. The dispatcher must SET it on error, CLEAR it on
 * success. We pre-load eflags with CF in the "wrong" state so a dispatcher that
 * forgets to write it is caught. */
#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

/* Build a fresh zeroed frame with a known-junk eflags pattern (incl. some high
 * bits set, so we can confirm the dispatcher touches ONLY bit 0). */
static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;  /* IF + reserved bit1; CF clear initially */
    return f;
}

int main(void)
{
    int21_set_sink(sink_capture);
    int21_set_exit(exit_observe);
    bind_standard_process();   /* standard JFT + SFT for the handle functions */

    /* --- AH=02h DISPLAY OUTPUT: DL='A' -> sink gets "A"; CF clear. -------- */
    {
        sink_reset();
        int_frame_t f = fresh_frame();
        f.eax = 0x0200u;          /* AH=02h, AL=0 */
        f.edx = (uint32_t)'A';    /* DL='A' */
        f.eflags |= CF_BIT;       /* preload CF=1 so we prove it is CLEARED */
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "A", "AH=02h should emit DL ('A')");
        CHECK(frame_cf(&f) == 0, "AH=02h must clear CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 'A', "AH=02h returns AL=DL");
    }

    /* --- AH=09h DISPLAY STRING: "HELLO$" -> "HELLO" (no '$'); CF clear. --- */
    {
        sink_reset();
        uint32_t edx = low_dup("HELLO$ and beyond");
        CHECK(edx != 0u, "low_dup string buffer in low 4 GiB");
        int_frame_t f = fresh_frame();
        f.eax = 0x0900u;                       /* AH=09h */
        f.edx = edx;                           /* flat ptr */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "HELLO", "AH=09h emits up to but NOT including '$'");
        CHECK(frame_cf(&f) == 0, "AH=09h must clear CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 0x24u, "AH=09h returns AL='$'");
    }

    /* --- AH=09h with a NULL pointer (EDX=0) must NOT dereference address 0
     *     (bcg.5). do_puts walked EDX unconditionally to the '$' terminator;
     *     a NULL or unterminated string let the kernel walk arbitrary memory
     *     (the DEC-14 fail-loud class for buffer-taking calls). Guard: NULL ->
     *     emit nothing, AL='$', CF clear. Pre-fix this dereferences NULL. */
    {
        sink_reset();
        int_frame_t f = fresh_frame();
        f.eax = 0x0900u;                       /* AH=09h */
        f.edx = 0u;                            /* NULL display string */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "", "AH=09h NULL pointer emits nothing (bcg.5)");
        CHECK(frame_cf(&f) == 0, "AH=09h NULL pointer clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 0x24u, "AH=09h NULL pointer returns AL='$'");
    }

    /* --- AH=40h WRITE handle=1 count=5 "WORLD" -> "WORLD", EAX=5, CF clear. */
    {
        sink_reset();
        uint32_t edx = low_dup("WORLDXYZ");    /* only first 5 should be written */
        CHECK(edx != 0u, "low_dup WORLD buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u;                       /* AH=40h */
        f.ebx = INT21_HANDLE_STDOUT;           /* handle 1 */
        f.ecx = 5u;                            /* count */
        f.edx = edx;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "WORLD", "AH=40h h=1 writes exactly ECX bytes");
        CHECK(f.eax == 5u, "AH=40h returns EAX=bytes written");
        CHECK(frame_cf(&f) == 0, "AH=40h success must clear CF");
    }

    /* AH=40h to stderr (handle 2) is also CON. */
    {
        sink_reset();
        uint32_t edx = low_dup("ERR");
        CHECK(edx != 0u, "low_dup ERR buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u;
        f.ebx = INT21_HANDLE_STDERR;
        f.ecx = 3u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "ERR", "AH=40h h=2 (stderr) also routes to CON");
        CHECK(f.eax == 3u, "AH=40h h=2 returns bytes written");
        CHECK(frame_cf(&f) == 0, "AH=40h h=2 success clears CF");
    }

    /* AH=40h bad handle (7) -> CF set, AX=6, nothing written. Handle 7 is closed
     * (jft[7]==0xFF) so sft_from_handle returns NULL = invalid handle. */
    {
        sink_reset();
        char buf[] = "NOPE";
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u;
        f.ebx = 7u;                            /* closed handle -> invalid */
        f.ecx = 4u;
        f.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "", "AH=40h bad handle writes nothing");
        CHECK(frame_cf(&f) == 1, "AH=40h bad handle sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "AH=40h bad handle returns AX=0x0006");
    }

    /* AH=45h DUP through the dispatcher: duplicating stdout (handle 1) returns
     * the lowest free handle (5) in AL, CF clear; the dup'd handle then writes
     * to CON exactly like stdout. Re-bind a clean process first so the JFT/SFT
     * are pristine (prior tests mutate the shared g_sft via the table API). */
    {
        bind_standard_process();
        int_frame_t f = fresh_frame();
        f.eax = 0x4500u;          /* AH=45h */
        f.ebx = INT21_HANDLE_STDOUT;
        f.eflags |= CF_BIT;       /* preload CF=1 so we prove it is CLEARED */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=45h DUP success clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 5u, "AH=45h DUP returns the new handle (5) in AL");

        /* The dup'd handle 5 writes to CON (it aliases the stdout SFT slot). */
        sink_reset();
        int_frame_t w = fresh_frame();
        uint32_t edx = low_dup("DUP");
        CHECK(edx != 0u, "low_dup DUP buffer");
        w.eax = 0x4000u; w.ebx = 5u; w.ecx = 3u; w.edx = edx;
        int21_dispatch(&w);
        CHECK_STR_EQ(sink_str(), "DUP", "the DUP'd handle 5 writes to CON like stdout");
    }

    /* AH=45h DUP of a closed handle -> CF set, AX=0x0006 (invalid handle). */
    {
        bind_standard_process();
        int_frame_t f = fresh_frame();
        f.eax = 0x4500u;
        f.ebx = 9u;               /* closed */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=45h DUP(closed) sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "AH=45h DUP(closed) returns AX=0x0006");
    }

    /* AH=46h DUP2 through the dispatcher: redirect stdout (handle 1) onto AUX
     * (handle 3). CF clear; afterward a WRITE to handle 1 lands on the AUX
     * device (no driver) -> access-denied and nothing reaches CON. This proves
     * the frame path drives the SFT redirect (handle 1's backing changed from
     * CON-write to AUX). AUX, not stdin, is used so the result is unambiguous:
     * do_write writes to ANY CON slot regardless of mode, but AUX has no write
     * backing, so the redirect is observable. */
    {
        bind_standard_process();
        int_frame_t f = fresh_frame();
        f.eax = 0x4600u;          /* AH=46h */
        f.ebx = 3u;               /* src = AUX */
        f.ecx = 1u;               /* dst = stdout */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=46h DUP2 success clears CF");

        sink_reset();
        int_frame_t w = fresh_frame();
        char buf[] = "X";
        w.eax = 0x4000u; w.ebx = 1u; w.ecx = 1u; w.edx = (uint32_t)(uintptr_t)buf;
        int21_dispatch(&w);
        CHECK(frame_cf(&w) == 1, "after DUP2(AUX->stdout) a WRITE to handle 1 is no longer CON");
        CHECK((uint16_t)(w.eax & 0xFFFFu) == INT21_ERR_ACCESS_DENIED,
              "redirected stdout (now AUX) returns access-denied");
        CHECK_STR_EQ(sink_str(), "", "redirected handle 1 writes nothing to CON");
    }

    /* AH=46h DUP2 with a closed source -> CF set, AX=0x0006. */
    {
        bind_standard_process();
        int_frame_t f = fresh_frame();
        f.eax = 0x4600u;
        f.ebx = 9u;               /* closed src */
        f.ecx = 1u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=46h DUP2(closed src) sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "AH=46h DUP2(closed src) returns AX=0x0006");
    }

    /* --- AH=30h GET VERSION: AL=3, AH=0x1E (30); CF clear. --------------- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x3000u;
        f.ebx = 0x0000FF00u;   /* caller BH=0xFF: must be zeroed (OEM=0) on return */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == INT21_VER_MAJOR, "GETVER AL=major(3)");
        CHECK((uint8_t)((f.eax >> 8) & 0xFFu) == INT21_VER_MINOR,
              "GETVER AH=minor(30=0x1E)");
        CHECK((uint8_t)((f.ebx >> 8) & 0xFFu) == 0u, "GETVER zeroes BH (OEM byte)");
        CHECK(frame_cf(&f) == 0, "GETVER clears CF");
    }

    /* --- AH=4Ch TERMINATE: hook observed with AL; CF irrelevant (no return
     *     to caller in the kernel, but host build just records). ----------- */
    {
        g_exit_called = 0; g_exit_code = 0xFF;
        int_frame_t f = fresh_frame();
        f.eax = 0x4C2Au;        /* AH=4Ch, AL=0x2A (42) */
        int21_dispatch(&f);
        CHECK(g_exit_called == 1, "AH=4Ch must call the terminate hook");
        CHECK(g_exit_code == 0x2Au, "AH=4Ch passes AL as the return code");
    }

    /* AH=00h TERMINATE alias -> rc 0. */
    {
        g_exit_called = 0; g_exit_code = 0xFF;
        int_frame_t f = fresh_frame();
        f.eax = 0x0000u;        /* AH=00h */
        int21_dispatch(&f);
        CHECK(g_exit_called == 1, "AH=00h aliases terminate");
        CHECK(g_exit_code == 0x00u, "AH=00h terminate rc=0");
    }

    /* --- UNLISTED AH (0xFE) -> CF set, AX=1, a diagnostic emitted (NOT a
     *     silent no-op). The sink captures the diagnostic text. ----------- */
    {
        sink_reset();
        int_frame_t f = fresh_frame();
        f.eax = 0xFE00u;        /* AH=0xFE -- not in the locked register */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "unlisted AH sets CF (controlled scope)");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_FUNCTION,
              "unlisted AH returns AX=0x0001 (invalid function)");
        CHECK(g_sink_len > 0, "unlisted AH must emit a diagnostic, NOT be silent");
        CHECK(strstr(sink_str(), "unknown") != NULL ||
              strstr(sink_str(), "AH=") != NULL,
              "unlisted AH diagnostic should mention the unknown AH");
    }

    /* --- LISTED-but-unimplemented AH (0Fh FCB OPEN -- the FCB range 0Fh-24h is
     *     in the register but deferred) -> recognized as a distinct 'not yet
     *     implemented' diagnostic, CF set, AX=1; NOT 'unknown function'.
     *     (CON input 01h/06h/0Ah are NOW real -- beads initech-n62 -- so this
     *     test moved to a still-deferred listed AH.) -------------------------- */
    {
        sink_reset();
        int_frame_t f = fresh_frame();
        f.eax = 0x0F00u;        /* AH=0Fh -- listed (FCB ops) but deferred */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "listed-but-deferred AH sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_FUNCTION,
              "listed-but-deferred AH returns AX=0x0001 for now");
        CHECK(g_sink_len > 0, "listed-but-deferred AH must emit a diagnostic");
        CHECK(strstr(sink_str(), "not-yet-impl") != NULL,
              "listed-but-deferred AH says not-yet-impl, distinct from unknown");
    }

    /* ======================================================================
     * Resident query functions (beads initech-yv9): SELDISK/GETDISK, DATE/TIME
     * (via the clock mock), GET DISK FREE SPACE, GET CWD, GET EXTENDED ERROR,
     * GET PSP. These exercise the dispatch + the 16-bit register packing.
     * ==================================================================== */
    int21_set_clock(mock_clock_get, mock_clock_set);

    /* AH=0Eh SELECT DISK: AL = number of logical drives = 1 (A: only). */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x0E00u;
        f.edx = 0u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=0Eh SELDISK clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 1u, "AH=0Eh returns AL=1 (drive count)");
    }

    /* AH=19h GET CURRENT DISK: AL = 0 (A:). */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x1900u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=19h GETDISK clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "AH=19h returns AL=0 (A:)");
    }

    /* AH=2Ah GET DATE: CX=year, DH=month, DL=day, AL=DOW (from the clock mock). */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x2A00u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=2Ah GET DATE clears CF");
        CHECK((uint16_t)(f.ecx & 0xFFFFu) == 2026u, "AH=2Ah CX=year 2026");
        CHECK((uint8_t)((f.edx >> 8) & 0xFFu) == 6u, "AH=2Ah DH=month 6");
        CHECK((uint8_t)(f.edx & 0xFFu) == 9u, "AH=2Ah DL=day 9");
        CHECK((uint8_t)(f.eax & 0xFFu) == 2u, "AH=2Ah AL=DOW 2 (Tuesday)");
    }

    /* AH=2Ch GET TIME: CH=hour, CL=min, DH=sec, DL=0 (1s resolution). */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x2C00u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=2Ch GET TIME clears CF");
        CHECK((uint8_t)((f.ecx >> 8) & 0xFFu) == 12u, "AH=2Ch CH=hour 12");
        CHECK((uint8_t)(f.ecx & 0xFFu) == 34u, "AH=2Ch CL=min 34");
        CHECK((uint8_t)((f.edx >> 8) & 0xFFu) == 56u, "AH=2Ch DH=sec 56");
        CHECK((uint8_t)(f.edx & 0xFFu) == 0u, "AH=2Ch DL=centisec 0 (1s resolution)");
    }

    /* AH=2Bh SET DATE: valid date -> AL=0; the mock records SET_DATE only. */
    {
        g_mock_set_calls = 0;
        int_frame_t f = fresh_frame();
        f.eax = 0x2B00u;
        f.ecx = 1999u;                          /* year */
        f.edx = ((uint32_t)12u << 8) | 25u;     /* DH=Dec, DL=25 */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "AH=2Bh valid SET DATE -> AL=0");
        CHECK(g_mock_set_calls == 1, "AH=2Bh calls the clock set once");
        CHECK(g_mock_last_which == INT21_CLOCK_SET_DATE,
              "AH=2Bh sets ONLY the date (time preserved)");
        CHECK(g_mock_year == 1999u && g_mock_month == 12u && g_mock_day == 25u,
              "AH=2Bh wrote the date through to the clock");
        CHECK(g_mock_hour == 12u && g_mock_min == 34u && g_mock_sec == 56u,
              "AH=2Bh did NOT disturb the time");
    }

    /* AH=2Bh SET DATE: invalid month -> AL=0xFF. */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x2B00u;
        f.ecx = 2026u;
        f.edx = ((uint32_t)13u << 8) | 1u;      /* month 13 = invalid */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0xFFu, "AH=2Bh invalid month -> AL=0xFF");
    }

    /* AH=2Dh SET TIME: valid -> AL=0; SET_TIME only (date preserved). */
    {
        g_mock_set_calls = 0;
        int_frame_t f = fresh_frame();
        f.eax = 0x2D00u;
        f.ecx = ((uint32_t)8u << 8) | 15u;      /* CH=8, CL=15 */
        f.edx = ((uint32_t)30u << 8) | 0u;      /* DH=30 sec */
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "AH=2Dh valid SET TIME -> AL=0");
        CHECK(g_mock_last_which == INT21_CLOCK_SET_TIME,
              "AH=2Dh sets ONLY the time (date preserved)");
        CHECK(g_mock_hour == 8u && g_mock_min == 15u && g_mock_sec == 30u,
              "AH=2Dh wrote the time through to the clock");
        CHECK(g_mock_year == 1999u && g_mock_month == 12u && g_mock_day == 25u,
              "AH=2Dh did NOT disturb the date");
    }

    /* AH=36h GET DISK FREE SPACE: AX=spc, BX=free, CX=bps, DX=total. */
    {
        int21_set_file_backend(&g_space_backend);
        int_frame_t f = fresh_frame();
        f.eax = 0x3600u;
        f.edx = 1u;                             /* DL=1 -> A: */
        int21_dispatch(&f);
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 1u, "AH=36h AX=sectors/cluster 1");
        CHECK((uint16_t)(f.ebx & 0xFFFFu) == 2840u, "AH=36h BX=free clusters 2840");
        CHECK((uint16_t)(f.ecx & 0xFFFFu) == 512u, "AH=36h CX=bytes/sector 512");
        CHECK((uint16_t)(f.edx & 0xFFFFu) == 2847u, "AH=36h DX=total clusters 2847");
    }

    /* AH=36h invalid drive -> AX=0xFFFF. */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x3600u;
        f.edx = 9u;                             /* DL=9 -> no such drive */
        int21_dispatch(&f);
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0xFFFFu,
              "AH=36h bad drive -> AX=0xFFFF");
    }

    /* AH=47h GET CWD: fills the DS:SI (ESI) buffer with the root path (empty). */
    {
        char *buf = (char *)alloc_low(64);
        CHECK(buf != NULL, "AH=47h CWD buffer in low 4 GiB");
        if (buf) {
            buf[0] = 'X';   /* poison so we can prove it was written */
            int_frame_t f = fresh_frame();
            f.eax = 0x4700u;
            f.edx = 1u;                         /* DL=1 -> A: */
            f.esi = (uint32_t)(uintptr_t)buf;
            int21_dispatch(&f);
            CHECK(frame_cf(&f) == 0, "AH=47h GET CWD clears CF");
            CHECK(buf[0] == '\0', "AH=47h root CWD is an empty string");
        }
    }

    /* AH=47h invalid drive -> CF set, AX=0x000F. */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x4700u;
        f.edx = 7u;
        f.esi = 0u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=47h bad drive sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0x000Fu, "AH=47h bad drive AX=0x000F");
    }

    /* AH=59h GET EXTENDED ERROR: seed a known error, query the triple. */
    {
        int21_note_error(INT21_ERR_FILE_NOT_FOUND);
        int_frame_t f = fresh_frame();
        f.eax = 0x5900u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=59h clears CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "AH=59h AX=last error (file-not-found)");
        CHECK((uint8_t)((f.ebx >> 8) & 0xFFu) == 0x08u, "AH=59h BH=class NOT_FOUND");
        CHECK((uint8_t)(f.ebx & 0xFFu) != 0u, "AH=59h BL=action nonzero");
        CHECK((uint8_t)((f.ecx >> 8) & 0xFFu) != 0u, "AH=59h CH=locus nonzero");
    }
    {
        /* No-error path: clear it and confirm AX=0, class=0. */
        int21_note_error(0u);
        int_frame_t f = fresh_frame();
        f.eax = 0x5900u;
        int21_dispatch(&f);
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0u, "AH=59h AX=0 when no error");
        CHECK((uint8_t)((f.ebx >> 8) & 0xFFu) == 0u, "AH=59h BH=0 when no error");
    }

    /* AH=62h GET PSP: BX = (PSP flat addr >> 4), nonzero with a bound PSP. */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x6200u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "AH=62h clears CF");
        uint16_t want = (uint16_t)(((uintptr_t)&g_test_psp >> 4) & 0xFFFFu);
        CHECK((uint16_t)(f.ebx & 0xFFFFu) == want,
              "AH=62h BX = current PSP paragraph");
    }

    /* --- CF propagation, explicit: success path must CLEAR a preset CF and
     *     touch ONLY bit 0 (leave the other eflags bits intact). ---------- */
    {
        int_frame_t f = fresh_frame();
        uint32_t before = f.eflags | CF_BIT;   /* preset CF */
        f.eflags = before;
        f.eax = 0x0200u;        /* PUTCHAR -- a success path */
        f.edx = (uint32_t)'Z';
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "success clears CF");
        CHECK((f.eflags & ~CF_BIT) == (before & ~CF_BIT),
              "dispatcher must touch ONLY CF (bit 0) of eflags");
    }

    return TEST_SUMMARY("test_int21");
}
