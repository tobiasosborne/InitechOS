/* test_devwire.c -- host unit oracle for the INT 21h <-> character-device-chain
 *                    wiring (beads initech-6zd9). Drives the REAL int21_dispatch
 *                    over the REAL devices.c chain so OPEN-by-name + device
 *                    READ/WRITE routing are proven end-to-end through the syscall
 *                    spine -- not in isolation.
 *
 * Ref (Law 1):
 *   - Ralf Brown's Interrupt List INT 21/AH=3Dh: DOS OPEN checks for a character
 *     device of the given NAME (CON/NUL/PRN/AUX/CLOCK$) BEFORE the directory; a
 *     match yields a device handle, never a file.
 *   - MS-DOS 3.3 Technical Reference Ch. 4: the resident device names; NUL is the
 *     bit-bucket (WRITE consumes all, READ is EOF); CLOCK$ READ returns the
 *     6-byte date/time record; PRN is write-only.
 *   - ADR-0003 DEC-09 (the device chain); os/milton/devices.h (the chain API).
 *   - CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6 (mutation-
 *     proven), Rule 12 (ASCII).
 *
 * Factory test: libc OK, reuses seed/test_assert.h. Links the REAL artifact TUs
 * int21.c + sft.c + psp.c + mcb.c + irq.c + devices.c (devices.c is a SEPARATE
 * TU here -- int21_dispatch calls devices_find()/devices_request() as external
 * symbols; this is NOT the #include trick test_devices.c uses).
 *
 * The CON + CLOCK$ legs of the device chain are self-wired by int21.c to its OWN
 * sink/conin/clock seams (so a device-chain CON write is the SAME byte stream as
 * a handle-1 write -- the CON output path is preserved exactly). So this oracle
 * binds those seams (int21_set_sink / int21_set_conin / int21_set_clock) and the
 * PRN sink through the device io bundle (int21_set_devices).
 *
 * MUTATION (Rule 6), driven by make test-devwire-mutant:
 *   -DINT21_MUTATE_OPEN_NO_DEVICE : do_open's device lookup always returns NULL,
 *       so OPEN "NUL"/"CON"/"PRN"/"CLOCK$" falls through to the FAT path -> with
 *       no such FILE it returns file-not-found and the device-OPEN assertions go
 *       RED. A one-branch RUNTIME perturbation that still compiles under -Werror.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "sft.h"
#include "psp.h"
#include "devices.h"
#include "test_assert.h"

TEST_HARNESS();

/* --- standard process binding (mirrors test_int21.c / test_fileio.c) ------- */
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

/* --- low-4-GiB buffers so EDX (uint32_t flat ptr) round-trips -------------- */
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

/* --- capturing CON sink (the CON device + the legacy handle-1 path both fan
 * bytes here -- so the no-regression assertion can check ordinary CON output) - */
static char g_sink_buf[512];
static size_t g_sink_len;
static void sink_capture(char c) { if (g_sink_len < sizeof(g_sink_buf) - 1) g_sink_buf[g_sink_len++] = c; }
static void sink_reset(void) { g_sink_len = 0; g_sink_buf[0] = '\0'; }
static const char *sink_str(void) { g_sink_buf[g_sink_len] = '\0'; return g_sink_buf; }

/* --- mock CON input (the OPEN-by-name CON read pulls raw bytes through here) - */
static const char *g_conin_src = "";
static size_t      g_conin_pos = 0;
/* On exhaustion return CR (0x0D): the OPEN-by-name CON device read takes ONE byte
 * per call (no cooked editing), and the legacy cooked-line editor (only reached
 * if a test ever drove a predefined CON handle through 3Fh) terminates on CR --
 * so a stray read can NEVER hang the host oracle (the mutant deliberately mis-
 * routes a failed-OPEN error code as a handle; CR keeps that path bounded). */
static int mock_conin_get(void)
{
    if (g_conin_src[g_conin_pos] == '\0') return '\r';
    return (unsigned char)g_conin_src[g_conin_pos++];
}
static int mock_conin_poll(void)
{
    if (g_conin_src[g_conin_pos] == '\0') return -1;
    return (unsigned char)g_conin_src[g_conin_pos++];
}

/* --- mock CLOCK seam (CLOCK$ READ converts this to the 6-byte record) ------ *
 * Pinned to 1980-01-02 03:04:05 so days-since-1980 == 1 (a clean, asserted
 * value) and the time bytes are distinct. */
static int mock_clock_get(uint16_t *y, uint8_t *mo, uint8_t *d,
                          uint8_t *h, uint8_t *mi, uint8_t *s, uint8_t *dow)
{
    *y = 1980u; *mo = 1u; *d = 2u; *h = 3u; *mi = 4u; *s = 5u; *dow = 3u;
    return 1;
}

/* --- mock PRN sink (the device io bundle's prn_write target) --------------- */
static char g_prn_buf[256];
static int  g_prn_len = 0;
static void prn_reset(void) { g_prn_len = 0; g_prn_buf[0] = '\0'; }
static int mock_prn_write(const uint8_t *b, int n, void *ctx)
{
    (void)ctx;
    for (int i = 0; i < n && g_prn_len < (int)sizeof(g_prn_buf) - 1; i++) {
        g_prn_buf[g_prn_len++] = (char)b[i];
    }
    g_prn_buf[g_prn_len] = '\0';
    return n;
}

#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;   /* IF + reserved; CF clear initially */
    return f;
}

/* OPEN path -> handle (AX), CF in *cf. */
static uint16_t do_open_name(const char *path, int *cf)
{
    uint32_t edx = low_dup(path);
    int_frame_t f = fresh_frame();
    f.eax = 0x3D00u;            /* AH=3Dh, AL=0 (read mode) */
    f.edx = edx;
    f.eflags |= CF_BIT;         /* preload CF=1 so success is proven to clear it */
    int21_dispatch(&f);
    *cf = frame_cf(&f);
    return (uint16_t)(f.eax & 0xFFFFu);
}

/* WRITE `s` (len bytes) to handle h. Returns EAX, CF in *cf. */
static uint32_t do_write_h(uint16_t h, const char *s, uint32_t len, int *cf)
{
    uint32_t edx = low_dup(s);
    int_frame_t f = fresh_frame();
    f.eax = 0x4000u;
    f.ebx = h;
    f.ecx = len;
    f.edx = edx;
    f.eflags |= CF_BIT;
    int21_dispatch(&f);
    *cf = frame_cf(&f);
    return f.eax;
}

/* READ up to len bytes from handle h into a low buffer. Returns EAX (bytes),
 * CF in *cf, and copies the bytes into out (caller-sized). */
static uint32_t do_read_h(uint16_t h, uint32_t len, uint8_t *out, int *cf)
{
    void *p = alloc_low(len ? len : 1u);
    memset(p, 0xAB, len ? len : 1u);   /* poison so a 0-byte read is observable */
    int_frame_t f = fresh_frame();
    f.eax = 0x3F00u;
    f.ebx = h;
    f.ecx = len;
    f.edx = (uint32_t)(uintptr_t)p;
    f.eflags |= CF_BIT;
    int21_dispatch(&f);
    *cf = frame_cf(&f);
    uint32_t got = f.eax;
    if (out && got <= len) memcpy(out, p, got);
    return got;
}

/* CLOSE handle h. */
static void do_close_h(uint16_t h)
{
    int_frame_t f = fresh_frame();
    f.eax = 0x3E00u;
    f.ebx = h;
    int21_dispatch(&f);
}

int main(void)
{
    int21_set_sink(sink_capture);
    int21_set_conin(mock_conin_get, mock_conin_poll);
    int21_set_clock(mock_clock_get, NULL);
    bind_standard_process();

    /* Build the REAL device chain + bind it into INT 21h, with PRN routed to the
     * mock sink (CON/CLOCK$ are self-wired by int21.c to the seams above). */
    devices_init();
    {
        devices_io_t io;
        memset(&io, 0, sizeof(io));
        io.prn_write = mock_prn_write;
        int21_set_devices(devices_head(), &io);
    }

    /* ===================================================================== *
     * 1. OPEN "NUL" -> a valid handle; WRITE consumes all + succeeds; READ EOF.
     * ===================================================================== */
    {
        int cf = 9;
        uint16_t h = do_open_name("NUL", &cf);
        CHECK(cf == 0, "OPEN NUL clears CF (it is a device, not a file)");
        CHECK(h >= SFT_FIRST_FILE, "OPEN NUL returns a real handle (>= first file slot)");

        int wcf = 9;
        uint32_t wrote = do_write_h(h, "DISCARD ME", 10u, &wcf);
        CHECK(wcf == 0, "WRITE to NUL clears CF");
        CHECK(wrote == 10u, "WRITE to NUL reports all bytes consumed (bit-bucket)");
        CHECK_STR_EQ(sink_str(), "", "WRITE to NUL reaches NO console output");

        uint8_t rb[16]; int rcf = 9;
        uint32_t got = do_read_h(h, 8u, rb, &rcf);
        CHECK(rcf == 0, "READ from NUL clears CF");
        CHECK(got == 0u, "READ from NUL returns 0 bytes (EOF) -- the NUL contract");

        do_close_h(h);
    }

    /* ===================================================================== *
     * 2. OPEN "CON" -> a handle; WRITE reaches the (test-stub) console sink.
     *    This is the OPEN-by-name CON path, distinct from the predefined slot 1.
     * ===================================================================== */
    {
        sink_reset();
        int cf = 9;
        uint16_t h = do_open_name("CON", &cf);
        CHECK(cf == 0, "OPEN CON clears CF");
        CHECK(h >= SFT_FIRST_FILE, "OPEN CON returns a real device handle");

        int wcf = 9;
        uint32_t wrote = do_write_h(h, "HELLO-CON", 9u, &wcf);
        CHECK(wcf == 0, "WRITE to OPEN-by-name CON clears CF");
        CHECK(wrote == 9u, "WRITE to CON reports all bytes written");
        CHECK_STR_EQ(sink_str(), "HELLO-CON",
                     "WRITE to OPEN-by-name CON reaches the SAME console sink");
        do_close_h(h);
    }

    /* ===================================================================== *
     * 3. OPEN "CLOCK$" -> READ returns the injected date/time (6-byte record).
     *    days-since-1980 for 1980-01-02 == 1 (LE word 0x0001), then min/hr/cs/sec.
     * ===================================================================== */
    {
        int cf = 9;
        uint16_t h = do_open_name("CLOCK$", &cf);
        CHECK(cf == 0, "OPEN CLOCK$ clears CF");
        CHECK(h >= SFT_FIRST_FILE, "OPEN CLOCK$ returns a real device handle");

        uint8_t rec[8]; int rcf = 9;
        uint32_t got = do_read_h(h, 6u, rec, &rcf);
        CHECK(rcf == 0, "READ CLOCK$ clears CF");
        CHECK(got == 6u, "READ CLOCK$ returns exactly the 6-byte record");
        /* offset 0..1: days-since-1980 (LE) == 1; 2: minutes; 3: hours;
         * 4: hundredths; 5: seconds. (mock clock 1980-01-02 03:04:05) */
        uint16_t days = (uint16_t)(rec[0] | (rec[1] << 8));
        CHECK(days == 1u, "CLOCK$ days-since-1980 == 1 for 1980-01-02");
        CHECK(rec[2] == 4u, "CLOCK$ minutes byte == 4");
        CHECK(rec[3] == 3u, "CLOCK$ hours byte == 3");
        CHECK(rec[5] == 5u, "CLOCK$ seconds byte == 5");
        do_close_h(h);
    }

    /* ===================================================================== *
     * 4. OPEN "PRN" -> WRITE reaches the prn sink callback; READ is an error.
     * ===================================================================== */
    {
        prn_reset();
        sink_reset();
        int cf = 9;
        uint16_t h = do_open_name("PRN", &cf);
        CHECK(cf == 0, "OPEN PRN clears CF");
        CHECK(h >= SFT_FIRST_FILE, "OPEN PRN returns a real device handle");

        int wcf = 9;
        uint32_t wrote = do_write_h(h, "PRINTME", 7u, &wcf);
        CHECK(wcf == 0, "WRITE to PRN clears CF");
        CHECK(wrote == 7u, "WRITE to PRN reports all bytes written");
        CHECK_STR_EQ(g_prn_buf, "PRINTME", "WRITE to PRN reaches the prn sink callback");
        CHECK_STR_EQ(sink_str(), "", "WRITE to PRN does NOT leak to the console sink");

        /* PRN read is an error (the printer is output-only). */
        uint8_t rb[8]; int rcf = 9;
        (void)do_read_h(h, 4u, rb, &rcf);
        CHECK(rcf == 1, "READ from PRN sets CF (printer is write-only)");
        do_close_h(h);
    }

    /* ===================================================================== *
     * 5. OPEN "A:\NUL" (drive + path prefix) still resolves to the NUL device.
     * ===================================================================== */
    {
        int cf = 9;
        uint16_t h = do_open_name("A:\\NUL", &cf);
        CHECK(cf == 0, "OPEN 'A:\\NUL' clears CF (drive+path prefix stripped)");
        CHECK(h >= SFT_FIRST_FILE, "OPEN 'A:\\NUL' returns a NUL device handle");
        int wcf = 9;
        uint32_t wrote = do_write_h(h, "x", 1u, &wcf);
        CHECK(wcf == 0 && wrote == 1u, "WRITE to 'A:\\NUL' handle consumes the byte");
        do_close_h(h);
    }

    /* OPEN "NUL.TXT" (an extension) still names the NUL device (extension ignored). */
    {
        int cf = 9;
        uint16_t h = do_open_name("NUL.TXT", &cf);
        CHECK(cf == 0, "OPEN 'NUL.TXT' clears CF (extension ignored on device match)");
        CHECK(h >= SFT_FIRST_FILE, "OPEN 'NUL.TXT' returns the NUL device handle");
        do_close_h(h);
    }

    /* ===================================================================== *
     * 6. NO-REGRESSION: the EXISTING CON output paths still reach the console.
     *    (a) AH=09h DISPLAY STRING; (b) AH=40h WRITE to predefined handle 1.
     *    These are the load-bearing shell-prompt / diagnostic paths -- they must
     *    be untouched by the device wiring (they take the legacy CON fast path,
     *    device == NULL, dev_id == SFT_DEV_CON).
     * ===================================================================== */
    {
        bind_standard_process();   /* pristine standard handles */
        sink_reset();
        uint32_t edx = low_dup("PROMPT$");
        int_frame_t f = fresh_frame();
        f.eax = 0x0900u;           /* AH=09h DISPLAY STRING */
        f.edx = edx;
        int21_dispatch(&f);
        CHECK_STR_EQ(sink_str(), "PROMPT",
                     "AH=09h still emits to CON (no-regression, '$' not emitted)");
    }
    {
        sink_reset();
        int wcf = 9;
        uint32_t wrote = do_write_h((uint16_t)INT21_HANDLE_STDOUT, "A:\\>", 4u, &wcf);
        CHECK(wcf == 0, "AH=40h handle 1 (predefined CON) still clears CF");
        CHECK(wrote == 4u, "AH=40h handle 1 reports bytes written");
        CHECK_STR_EQ(sink_str(), "A:\\>",
                     "AH=40h handle 1 still reaches the console (CON output preserved)");
    }
    {
        /* handle 2 (stderr) also CON -- the second leg the shell depends on. */
        sink_reset();
        int wcf = 9;
        uint32_t wrote = do_write_h((uint16_t)INT21_HANDLE_STDERR, "ERR", 3u, &wcf);
        CHECK(wcf == 0 && wrote == 3u, "AH=40h handle 2 (stderr->CON) still clears CF");
        CHECK_STR_EQ(sink_str(), "ERR", "AH=40h handle 2 still reaches the console");
    }

    /* ===================================================================== *
     * 7. A non-device OPEN with no FAT backend bound falls through to the file
     *    path and returns file-not-found (the device check did NOT swallow it).
     * ===================================================================== */
    {
        bind_standard_process();
        int21_set_file_backend(NULL);     /* no volume */
        int cf = 9;
        uint16_t ax = do_open_name("REALFILE.TXT", &cf);
        CHECK(cf == 1, "OPEN of a non-device name (no backend) sets CF");
        CHECK(ax == INT21_ERR_FILE_NOT_FOUND,
              "OPEN of a non-device name returns file-not-found (fell through to FAT path)");
    }

    return TEST_SUMMARY("test_devwire");
}
