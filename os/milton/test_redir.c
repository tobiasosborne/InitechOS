/* test_redir.c -- host unit oracle for STDOUT-handle redirection of the
 * console-output builtins (beads initech-k36g). Factory test: libc OK, reuses
 * seed/test_assert.h.
 *
 * THE CLAIM UNDER TEST. Real DOS routes AH=02h DISPLAY OUTPUT / AH=09h DISPLAY
 * STRING / AH=06h direct-console-output to STDOUT (handle 1), so they ARE
 * redirectable: `echo HELLO > file` (DUP2(file_handle, 1)) catches the builtin's
 * bytes. Before initech-k36g these three handlers called con_putc DIRECTLY,
 * bypassing the JFT, so a DUP2(file,1) redirect did NOT catch them. This oracle
 * pins the post-fix contract:
 *
 *   (a) DEFAULT JFT (handle 1 -> the predefined CON-write slot): AH=09h/02h/06h
 *       output reaches the CON sink (the g_sink capture) -- byte-for-byte the
 *       un-redirected console path is preserved.
 *   (b) AFTER binding JFT handle 1 to a FILE SFT slot (the DUP2(file,1) redirect):
 *       AH=09h/02h/06h output reaches the FILE backend (the mock write_at captures
 *       the bytes) and does NOT reach CON. THIS is the load-bearing assertion --
 *       the 'echo HELLO > file' case the hsct redirection feature needs.
 *   (c) NO current PSP (handle 1 unresolvable): AH=09h output STILL reaches CON
 *       via the fallback -- the EARLY-banner-survival case (the banner prints via
 *       AH=09h before the kernel JFT is bound; Rule 2 -- never drop a byte).
 *
 * Ref (Law 1): DOS 3.3 Programmer's Reference Manual AH=02h/06h/09h (routed to
 *   STDOUT handle 1) + AH=46h DUP2 (the redirection primitive);
 *   fs-mount-sft-ground-truth.md Sec 3.2-3.4 (JFT->SFT handle layer);
 *   CLAUDE.md Law 2 (the oracle is truth), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact int21.c (the same int21_dispatch the
 * kernel runs), the same idiom as test_int21.c: the CON sink is captured via
 * int21_set_sink, and a mock FILE backend's write_at captures the redirected
 * bytes. We build a fake int_frame_t, call int21_dispatch, and assert WHERE the
 * output byte landed.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DK36G_MUTATE_NO_REDIR : stdout_emit always con_putc (never the resolved
 *                            handle-1 SFT) -> AH=02h/09h/06h are no longer
 *                            redirectable -> assertion (b) goes RED.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "sft.h"    /* JFT->SFT handle layer: the FILE slot we bind handle 1 to */
#include "psp.h"    /* psp_build -> the standard JFT (handle 1 -> CON-write)    */
#include "test_assert.h"

TEST_HARNESS();

/* The standard process PSP whose JFT we (re)bind. psp_build lays jft[0..4] onto
 * the device SFT slots so handle 1 -> CON-write; sft_init lays the device
 * entries. */
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

/* --- The capturing CON sink (same idiom as test_int21.c) ----------------- */
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

/* --- The mock FILE backend: write_at captures the redirected bytes ------- *
 * THIS is the file `echo HELLO > file` redirects to. The redirect binds JFT
 * handle 1 to a FILE SFT slot; do_write/sft_write then routes handle-1 output
 * through g_file->write_at, which lands here. We capture the bytes (positioned
 * by `offset`, so per-byte AH=09h writes accumulate in order) + return the
 * updated dir entry with the grown size, matching the real write_at contract. */
static char     g_file_buf[256];
static uint32_t g_file_len;     /* high-water mark = file size */

static void file_reset(void)
{
    g_file_len = 0u;
    memset(g_file_buf, 0, sizeof(g_file_buf));
}

static uint16_t mock_write_at(uint16_t dir_start, uint32_t slot, uint32_t offset,
                              const uint8_t *data, uint32_t len,
                              uint32_t *out_written, struct dir_entry *out_entry)
{
    (void)dir_start; (void)slot;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t pos = offset + i;
        if (pos < sizeof(g_file_buf) - 1u) {
            g_file_buf[pos] = (char)data[i];
            if (pos + 1u > g_file_len) {
                g_file_len = pos + 1u;   /* high-water mark = file size */
            }
        }
    }
    *out_written = len;                  /* the file accepts every byte */
    if (out_entry) {
        out_entry->file_size = g_file_len;   /* refresh size (real write_at does) */
    }
    return 0u;
}
static const char *file_str(void)
{
    g_file_buf[g_file_len < sizeof(g_file_buf) ? g_file_len : sizeof(g_file_buf) - 1] = '\0';
    return g_file_buf;
}

/* Only write_at is exercised by the redirect; the rest of the vtable is NULL
 * (a designated initializer leaves the unmentioned members zero). */
static const int21_file_backend_t g_redir_backend = {
    .write_at = mock_write_at,
};

/* Bind JFT handle 1 to a fresh FILE SFT slot -- the in-memory equivalent of
 * DUP2(file_handle, 1): stdout now points at a writable FILE entry, so the
 * console-output builtins' bytes go to g_file->write_at, not CON. Returns the
 * SFT slot index. */
static uint8_t redirect_stdout_to_file(void)
{
    uint8_t sft_idx = sft_alloc();
    g_sft[sft_idx].kind       = SFT_KIND_FILE;
    g_sft[sft_idx].open_mode  = SFT_MODE_WRITE;   /* writable (CREAT for '>') */
    g_sft[sft_idx].ref_count  = 1u;
    g_sft[sft_idx].file_offset = 0u;
    g_sft[sft_idx].dir_start   = 0u;
    g_sft[sft_idx].root_slot   = 0u;
    g_test_psp.jft[INT21_HANDLE_STDOUT] = sft_idx;   /* the DUP2(file,1) repoint */
    return sft_idx;
}

/* Low-4-GiB allocator so a flat (uint32_t)EDX round-trips (test_int21.c idiom). */
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

/* CF helpers: bit 0 of EFLAGS. */
#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;  /* IF + reserved bit1; CF clear */
    return f;
}

/* Emit "HI$" via AH=09h, 'Q' via AH=02h, 'Z' via AH=06h output leg -- the three
 * console-output builtins under test, exercised back-to-back. */
static void emit_all_three(void)
{
    /* AH=09h DISPLAY STRING "HI$" -> "HI" */
    uint32_t edx = low_dup("HI$");
    CHECK(edx != 0u, "low_dup HI$ buffer in low 4 GiB");
    int_frame_t s = fresh_frame();
    s.eax = 0x0900u;
    s.edx = edx;
    int21_dispatch(&s);
    CHECK((uint8_t)(s.eax & 0xFFu) == 0x24u, "AH=09h returns AL='$'");
    CHECK(frame_cf(&s) == 0, "AH=09h clears CF");

    /* AH=02h DISPLAY OUTPUT 'Q' */
    int_frame_t c = fresh_frame();
    c.eax = 0x0200u;
    c.edx = (uint32_t)'Q';
    int21_dispatch(&c);
    CHECK((uint8_t)(c.eax & 0xFFu) == 'Q', "AH=02h returns AL=DL ('Q')");
    CHECK(frame_cf(&c) == 0, "AH=02h clears CF");

    /* AH=06h DIRECT CONSOLE I/O, output leg (DL != 0xFF), 'Z' */
    int_frame_t d = fresh_frame();
    d.eax = 0x0600u;
    d.edx = (uint32_t)'Z';   /* DL='Z' (!= 0xFF -> output) */
    int21_dispatch(&d);
    CHECK((uint8_t)(d.eax & 0xFFu) == 'Z', "AH=06h output returns AL=DL ('Z')");
    CHECK(frame_cf(&d) == 0, "AH=06h output clears CF");
}

int main(void)
{
    int21_set_sink(sink_capture);
    bind_standard_process();

    /* ====================================================================
     * (a) DEFAULT JFT: handle 1 -> CON-write. The three builtins reach the
     *     CON sink; the redirect file stays empty. This is the un-redirected
     *     console path -- it MUST be byte-for-byte the legacy con_putc output.
     * ==================================================================== */
    {
        bind_standard_process();
        int21_set_file_backend(&g_redir_backend);  /* bound but stdout is CON */
        sink_reset();
        file_reset();

        emit_all_three();

        CHECK_STR_EQ(sink_str(), "HIQZ",
                     "(a) default stdout: AH=09h/02h/06h reach the CON sink");
        CHECK_STR_EQ(file_str(), "",
                     "(a) default stdout: nothing leaks to the FILE backend");
    }

    /* ====================================================================
     * (b) THE LOAD-BEARING CASE -- 'echo HELLO > file'. Bind JFT handle 1 to a
     *     FILE SFT slot (the DUP2(file,1) redirect). The three builtins now
     *     reach the FILE backend (write_at captures them) and NOTHING reaches
     *     CON. Before initech-k36g these con_putc'd directly -> the file stayed
     *     empty and CON got "HIQZ"; the mutant (-DK36G_MUTATE_NO_REDIR) restores
     *     exactly that, so this assertion bites.
     * ==================================================================== */
    {
        bind_standard_process();
        int21_set_file_backend(&g_redir_backend);
        (void)redirect_stdout_to_file();           /* DUP2(file,1) */
        sink_reset();
        file_reset();

        emit_all_three();

        CHECK_STR_EQ(file_str(), "HIQZ",
                     "(b) redirected stdout: AH=09h/02h/06h reach the FILE backend "
                     "(the 'echo HELLO > file' case)");
        CHECK_STR_EQ(sink_str(), "",
                     "(b) redirected stdout: NOTHING reaches CON (the redirect is caught)");
    }

    /* A second redirect pass with a longer string proves the positioned per-byte
     * AH=09h writes accumulate in order (offset advances), not just a single
     * byte -- the whole string lands in the file, in sequence. */
    {
        bind_standard_process();
        int21_set_file_backend(&g_redir_backend);
        (void)redirect_stdout_to_file();
        sink_reset();
        file_reset();

        uint32_t edx = low_dup("REDIRECTED-OUTPUT-LINE$");
        CHECK(edx != 0u, "low_dup long redirect string");
        int_frame_t s = fresh_frame();
        s.eax = 0x0900u; s.edx = edx;
        int21_dispatch(&s);

        CHECK_STR_EQ(file_str(), "REDIRECTED-OUTPUT-LINE",
                     "(b) per-byte AH=09h writes accumulate in order in the FILE");
        CHECK_STR_EQ(sink_str(), "",
                     "(b) long redirected string: nothing to CON");
    }

    /* ====================================================================
     * (c) NO current PSP -> handle 1 is unresolvable. AH=09h output STILL
     *     reaches CON via the fallback -- the banner-survival case (the banner
     *     prints via AH=09h before the kernel JFT is bound; Rule 2 never drops a
     *     byte). The mutant ALSO con_putc's here, so (c) is green under the
     *     mutant too (the fallback is exactly what the mutant always does) --
     *     (c) is a SAFETY assertion, (b) is the one that distinguishes the fix.
     * ==================================================================== */
    {
        int21_set_psp((struct psp *)0);   /* no process -> handle 1 unresolvable */
        sink_reset();
        file_reset();

        uint32_t edx = low_dup("BANNER$");
        CHECK(edx != 0u, "low_dup BANNER$ buffer");
        int_frame_t s = fresh_frame();
        s.eax = 0x0900u; s.edx = edx;
        int21_dispatch(&s);

        CHECK_STR_EQ(sink_str(), "BANNER",
                     "(c) no-PSP fallback: AH=09h still reaches CON (banner survives)");
        CHECK_STR_EQ(file_str(), "",
                     "(c) no-PSP: nothing to the FILE backend (there is no handle)");

        /* AH=02h + AH=06h fall back identically. */
        sink_reset();
        int_frame_t c = fresh_frame();
        c.eax = 0x0200u; c.edx = (uint32_t)'!';
        int21_dispatch(&c);
        int_frame_t d = fresh_frame();
        d.eax = 0x0600u; d.edx = (uint32_t)'?';
        int21_dispatch(&d);
        CHECK_STR_EQ(sink_str(), "!?",
                     "(c) no-PSP fallback: AH=02h + AH=06h also reach CON");

        /* Restore a PSP so a later add to this oracle does not run PSP-less. */
        int21_set_psp(&g_test_psp);
    }

    return TEST_SUMMARY("test_redir");
}
