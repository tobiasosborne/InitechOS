/* test_int21_edge.c -- host unit oracle HARDENING the INT 21h error paths and
 * the implemented-but-untested resident functions (beads initech-xrd,
 * initech-1zk; double-close part of initech-00x). Factory test: libc OK, reuses
 * seed/test_assert.h. Drives the REAL artifact int21_dispatch with a MOCK file
 * backend, exactly as os/milton/test_fileio.c does.
 *
 * These are CONTRACT / CHARACTERIZATION tests: they assert the REAL current
 * behavior of the handlers (against the code + spec/int21h_calling_convention.json
 * error codes 0x0002/0x0003/0x0005/0x0006/0x0009 + the GETVER lock), NOT idealized
 * DOS. Where a handler's behavior differs from textbook DOS (e.g. SELDISK/GETDISK
 * single-drive, GETCWD always-root, double-close returns invalid-handle) the test
 * pins what THIS kernel does so a future change to that contract is caught.
 *
 * Ref: os/milton/int21.c (do_read 0x3F / do_write 0x40 / do_close 0x3E /
 *      do_lseek 0x42 / do_seldisk 0x0E / do_getdisk 0x19 / do_getcwd 0x47 /
 *      do_geterr 0x59 / do_getpsp 0x62), os/milton/sft.c (sft_from_handle,
 *      sft.c:195 ref_count>0 guard), int21.c:984 do_close ref_count>0 guard;
 *      spec/int21h_calling_convention.json; CLAUDE.md Law 2 (oracle is truth),
 *      Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DINT21_MUTATE_CLOSE_NO_REFGUARD : do_close drops the int21.c:984 ref_count>0
 *       guard, so a CLOSE of a slot whose ref_count is already 0 UNDERFLOWS the
 *       uint16 to 0xFFFF (the corruption beads initech-00x flagged). The
 *       ref_count-no-underflow oracle goes RED.
 *
 * NULL/!valid EDX policy (CLAUDE.md Rule 2 / beads initech-tzq): we feed NULL EDX
 * ONLY to handlers that ACTUALLY guard it (do_open guards `path==0`, do_setdta
 * never dereferences EDX). do_read / do_write do NOT guard a NULL/garbage data
 * pointer once the handle is valid -- feeding them NULL with a VALID handle and a
 * non-zero count would memcpy through NULL and fault the test, so we DO NOT. The
 * invalid-handle leg returns BEFORE EDX is touched, so the NULL-EDX-on-bad-handle
 * tests below are safe and exercise the real early-out. The latent NULL-deref gap
 * is recorded in StructuredOutput.findings for initech-tzq.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "sft.h"
#include "psp.h"
#include "dos_structs.h"
#include "find_data.h"
#include "test_assert.h"

TEST_HARNESS();

/* --- standard process binding (mirrors test_fileio.c) ------------------- */
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

/* --- low-4-GiB buffers so EDX (uint32_t flat ptr) round-trips ----------- */
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

/* --- capturing CON sink (the file functions do not display, but the sink must
 * be bound so any stray diagnostic is harmless rather than NULL-faulting) --- */
static char g_sink_buf[512];
static size_t g_sink_len;
static void sink_capture(char c) { if (g_sink_len < sizeof(g_sink_buf) - 1) g_sink_buf[g_sink_len++] = c; }

#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;
    return f;
}

/* --- the MOCK file backend (a positioned, stateless in-memory directory),
 * trimmed from test_fileio.c to exactly the hooks the edge oracle drives:
 * open() LOCATES a file (returns its dir entry + a root-dir slot); read_at()
 * is a positioned read of the per-file byte buffer at `offset`; write_at()
 * overwrites/extends; create()/close()/unlink()/dir_entry are not exercised
 * here but are bound (NULL) so the dispatcher's NULL-hook guards are visible. */
#define MOCK_CAP 1024u
typedef struct mock_file {
    const char *name8;
    const char *ext3;
    uint8_t     attr;
    uint8_t     data[MOCK_CAP];
    uint32_t    size;
    int         present;
} mock_file_t;

static const char *HELLO_DATA = "Hello from InitechOS test file.\n";
static mock_file_t g_mock[2];

static void mock_reset_dir(void)
{
    memset(g_mock, 0, sizeof(g_mock));
    g_mock[0].name8 = "HELLO   "; g_mock[0].ext3 = "TXT"; g_mock[0].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[0].data, HELLO_DATA, strlen(HELLO_DATA));
    g_mock[0].size = (uint32_t)strlen(HELLO_DATA); g_mock[0].present = 1;

    g_mock[1].name8 = "README  "; g_mock[1].ext3 = "   "; g_mock[1].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[1].data, "read me", 7);
    g_mock[1].size = 7; g_mock[1].present = 1;
}
#define MOCK_N 2u

static void fill_dir_entry(const mock_file_t *m, dir_entry_t *de)
{
    memset(de, 0, sizeof(*de));
    for (int i = 0; i < 8; i++) de->filename[i]  = (uint8_t)(m->name8 ? m->name8[i] : ' ');
    for (int i = 0; i < 3; i++) de->extension[i] = (uint8_t)(m->ext3 ? m->ext3[i] : ' ');
    de->attribute  = m->attr;
    de->mtime      = 0x1234u;
    de->mdate      = 0x5678u;
    de->file_size  = m->size;
}

static int mock_find_by_name(const char *name83)
{
    for (uint32_t i = 0; i < MOCK_N; i++) {
        if (!g_mock[i].present || g_mock[i].name8 == NULL) continue;
        char fmt[13];
        int n = 0;
        for (int k = 0; k < 8 && g_mock[i].name8[k] != ' '; k++) fmt[n++] = g_mock[i].name8[k];
        if (g_mock[i].ext3[0] != ' ') {
            fmt[n++] = '.';
            for (int k = 0; k < 3 && g_mock[i].ext3[k] != ' '; k++) fmt[n++] = g_mock[i].ext3[k];
        }
        fmt[n] = '\0';
        const char *a = fmt, *b = name83;
        int eq = 1;
        while (*a && *b) {
            char ca = *a, cb = *b;
            if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 32);
            if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 32);
            if (ca != cb) { eq = 0; break; }
            a++; b++;
        }
        if (eq && *a == '\0' && *b == '\0') return (int)i;
    }
    return -1;
}

static uint16_t mock_open(const char *name83, dir_entry_t *out_entry,
                          uint32_t *out_slot)
{
    int idx = mock_find_by_name(name83);
    if (idx < 0) return 0x0002u;                  /* not found */
    fill_dir_entry(&g_mock[idx], out_entry);
    *out_slot = (uint32_t)idx;
    return 0u;
}

static uint16_t mock_read_at(const dir_entry_t *e, uint32_t offset,
                             uint8_t *buf, uint32_t len, uint32_t *out_read)
{
    char name[13];
    int n = 0;
    for (int k = 0; k < 8 && e->filename[k] != ' '; k++) name[n++] = (char)e->filename[k];
    if (e->extension[0] != ' ') {
        name[n++] = '.';
        for (int k = 0; k < 3 && e->extension[k] != ' '; k++) name[n++] = (char)e->extension[k];
    }
    name[n] = '\0';
    int idx = mock_find_by_name(name);
    uint32_t fsize = (idx >= 0) ? g_mock[idx].size : e->file_size;
    const uint8_t *data = (idx >= 0) ? g_mock[idx].data : NULL;

    uint32_t avail = (offset < fsize) ? (fsize - offset) : 0u;
    uint32_t take  = (len < avail) ? len : avail;
    if (take && data) memcpy(buf, data + offset, take);
    *out_read = take;
    return 0u;
}

static const int21_file_backend_t g_mock_backend = {
    mock_open, mock_read_at, NULL,
    NULL, NULL, NULL, NULL,
    NULL
};

/* Open HELLO.TXT through the dispatcher and return its handle (lowest free = 5
 * for a freshly-bound process). Asserts the open succeeded. */
static uint32_t open_hello(void)
{
    uint32_t edx = low_dup("HELLO.TXT");
    int_frame_t f = fresh_frame();
    f.eax = 0x3D00u; f.edx = edx;
    f.eflags |= CF_BIT;
    int21_dispatch(&f);
    CHECK(frame_cf(&f) == 0, "open_hello: OPEN HELLO.TXT clears CF");
    return (uint16_t)(f.eax & 0xFFFFu);
}

int main(void)
{
    mock_reset_dir();
    int21_set_sink(sink_capture);
    bind_standard_process();
    int21_set_file_backend(&g_mock_backend);

    /* =====================================================================
     * 1. ERROR PATHS (beads initech-xrd)
     * ===================================================================== */

    /* --- READ on an unopened handle (5) -> CF=1, AX=0x0006 --------------- */
    {
        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(16); memset(dst, 0xAA, 16);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = 5u; f.ecx = 8u; f.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "READ on unopened handle 5 sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "READ on unopened handle 5 AX=0x0006");
        CHECK(dst[0] == 0xAA, "READ on bad handle wrote nothing into the buffer");
    }

    /* --- READ on an out-of-JFT-range handle (255) -> CF=1, AX=0x0006 ----- */
    {
        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(16); memset(dst, 0xBB, 16);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = 255u; f.ecx = 8u; f.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "READ on handle 255 sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "READ on handle 255 AX=0x0006");
    }

    /* --- WRITE on an unopened handle (5) -> CF=1, AX=0x0006, nothing ----- */
    {
        uint32_t wbuf = low_dup("XYZ");
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u; f.ebx = 5u; f.ecx = 3u; f.edx = wbuf;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "WRITE on unopened handle 5 sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "WRITE on unopened handle 5 AX=0x0006");
    }

    /* --- WRITE on handle 255 (out of JFT range) -> CF=1, AX=0x0006 ------- */
    {
        uint32_t wbuf = low_dup("XYZ");
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u; f.ebx = 255u; f.ecx = 3u; f.edx = wbuf;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "WRITE on handle 255 sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "WRITE on handle 255 AX=0x0006");
    }

    /* --- ALREADY-CLOSED handle: OPEN -> CLOSE -> READ -> CF=1, AX=0x0006 - */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        uint32_t h = open_hello();
        CHECK(h == 5u, "open->close->read: fresh OPEN gets handle 5");

        int_frame_t c = fresh_frame();
        c.eax = 0x3E00u; c.ebx = h;
        int21_dispatch(&c);
        CHECK(frame_cf(&c) == 0, "open->close->read: CLOSE clears CF");

        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(16); memset(dst, 0xCC, 16);
        int_frame_t r = fresh_frame();
        r.eax = 0x3F00u; r.ebx = h; r.ecx = 8u; r.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&r);
        CHECK(frame_cf(&r) == 1, "READ on an already-closed handle sets CF");
        CHECK((uint16_t)(r.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "READ on an already-closed handle AX=0x0006");
    }

    /* --- DOUBLE CLOSE: OPEN -> CLOSE -> CLOSE. The 2nd close returns
     *     invalid-handle (the handle's JFT slot is already JFT_CLOSED, so
     *     sft_from_handle returns NULL); the SFT slot's ref_count NEVER
     *     underflows (int21.c:984 guard + the slot freed at ref 0). --------- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        uint32_t h = open_hello();
        uint8_t sft_idx = g_test_psp.jft[h];
        CHECK(sft_idx < (uint8_t)SFT_MAX_ENTRIES, "double-close: handle maps to a real SFT slot");
        CHECK(g_sft[sft_idx].kind == SFT_KIND_FILE, "double-close: slot is a FILE before close");
        CHECK(g_sft[sft_idx].ref_count == 1u, "double-close: ref_count is 1 on a fresh OPEN");

        /* first close: frees the slot (ref 1 -> 0), JFT[h] = closed */
        int_frame_t c1 = fresh_frame();
        c1.eax = 0x3E00u; c1.ebx = h;
        int21_dispatch(&c1);
        CHECK(frame_cf(&c1) == 0, "double-close: 1st CLOSE clears CF");
        CHECK(g_sft[sft_idx].kind == SFT_KIND_FREE,
              "double-close: slot is FREE after the 1st close (ref hit 0)");
        CHECK(g_sft[sft_idx].ref_count == 0u,
              "double-close: ref_count is 0 after the slot is freed");

        /* second close of the SAME handle: invalid-handle, no underflow */
        int_frame_t c2 = fresh_frame();
        c2.eax = 0x3E00u; c2.ebx = h;
        int21_dispatch(&c2);
        CHECK(frame_cf(&c2) == 1, "double-close: 2nd CLOSE of the same handle sets CF");
        CHECK((uint16_t)(c2.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "double-close: 2nd CLOSE AX=0x0006 (handle already closed)");
        CHECK(g_sft[sft_idx].ref_count != 0xFFFFu,
              "double-close: ref_count did NOT underflow to 0xFFFF on the 2nd close");
        CHECK(g_sft[sft_idx].ref_count == 0u,
              "double-close: ref_count remains 0 (slot stays free, no wrap)");
    }

    /* --- DIRECT ref_count>0 GUARD (int21.c:984): manufacture a reachable
     *     FILE slot whose ref_count is already 0, point a JFT handle at it,
     *     then CLOSE. sft_from_handle does NOT fail loud (kind != FREE), so
     *     do_close reaches the decrement. The guard keeps ref_count at 0 (no
     *     wrap); the mutant (no guard) underflows it to 0xFFFF and -- because
     *     the post-decrement value is then != 0 -- ALSO fails to free the
     *     slot. This is the exact double-close underflow beads initech-00x
     *     described, isolated so the int21.c:984 guard is mutation-proven. -- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);

        uint8_t idx = (uint8_t)SFT_FIRST_FILE;   /* lowest file-eligible slot */
        memset(&g_sft[idx], 0, sizeof(g_sft[idx]));
        g_sft[idx].kind      = SFT_KIND_FILE;
        g_sft[idx].open_mode = SFT_MODE_READ;
        g_sft[idx].ref_count = 0u;               /* the degenerate input */
        uint8_t h = 5u;
        g_test_psp.jft[h] = idx;                 /* a live handle onto the slot */

        int_frame_t c = fresh_frame();
        c.eax = 0x3E00u; c.ebx = h;
        int21_dispatch(&c);
        CHECK(frame_cf(&c) == 0, "ref-guard: CLOSE of a ref_count==0 slot clears CF");
        CHECK(g_sft[idx].ref_count == 0u,
              "ref-guard: int21.c:984 holds ref_count at 0 -- NO underflow to 0xFFFF");
        CHECK(g_sft[idx].kind == SFT_KIND_FREE,
              "ref-guard: the ref_count==0 slot is freed (guarded decrement keeps it 0 -> freed)");
        CHECK(g_test_psp.jft[h] == JFT_CLOSED,
              "ref-guard: the handle's JFT entry is closed after CLOSE");
    }

    /* --- CX=0 READ: 0 bytes, CF clear (no error) ------------------------- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        uint32_t h = open_hello();
        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(16); memset(dst, 0xDD, 16);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = h; f.ecx = 0u; f.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "CX=0 READ clears CF");
        CHECK(f.eax == 0u, "CX=0 READ returns 0 bytes");
        CHECK(dst[0] == 0xDD, "CX=0 READ touched no buffer bytes");

        /* a following normal read still works (offset was not disturbed) */
        int_frame_t r = fresh_frame();
        r.eax = 0x3F00u; r.ebx = h; r.ecx = 5u; r.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&r);
        CHECK(r.eax == 5u && memcmp(dst, "Hello", 5) == 0,
              "CX=0 READ did not advance the file offset (next read starts at 0)");
    }

    /* --- CX=0 WRITE to a CON device handle (1): 0 bytes, CF clear -------- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        g_sink_len = 0;
        uint32_t wbuf = low_dup("ZZZ");
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u; f.ebx = 1u; f.ecx = 0u; f.edx = wbuf;   /* handle 1 = stdout */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "CX=0 WRITE clears CF");
        CHECK(f.eax == 0u, "CX=0 WRITE returns 0 bytes");
        CHECK(g_sink_len == 0u, "CX=0 WRITE emits nothing to CON");
    }

    /* --- LARGE CX READ past EOF: short read (bytes < CX), CF clear ------- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        uint32_t h = open_hello();
        uint32_t hsize = (uint32_t)strlen(HELLO_DATA);
        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(256); memset(dst, 0, 256);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = h; f.ecx = 200u; f.edx = (uint32_t)(uintptr_t)dst; /* > file size */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "large-CX READ past EOF clears CF");
        CHECK(f.eax == hsize, "large-CX READ returns a SHORT count (file size, < CX)");
        CHECK(f.eax < 200u, "large-CX READ count is strictly less than the requested CX");
        CHECK(memcmp(dst, HELLO_DATA, hsize) == 0, "large-CX READ delivered the whole file");

        /* a second large-CX read now at EOF -> 0 bytes, CF clear */
        int_frame_t g = fresh_frame();
        g.eax = 0x3F00u; g.ebx = h; g.ecx = 200u; g.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "large-CX READ at EOF clears CF");
        CHECK(g.eax == 0u, "large-CX READ at EOF returns 0 bytes (clean EOF)");
    }

    /* --- LSEEK on a bad handle -> CF=1, AX=0x0006 (error-path coverage) -- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        int_frame_t f = fresh_frame();
        f.eax = 0x4200u; f.ebx = 9u; f.ecx = 0u; f.edx = 0u;   /* handle 9 closed */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "LSEEK on a closed handle sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "LSEEK on a closed handle AX=0x0006");
    }

    /* =====================================================================
     * 2. NULL / !valid EDX -- ONLY for handlers that GUARD it (Rule 2).
     * ===================================================================== */

    /* --- do_open GUARDS NULL EDX: OPEN with EDX=0 -> CF=1, AX=0x0002 ----- *
     * int21.c do_open: `if (path == 0 || *path == '\0')` -> file-not-found. */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = 0u;   /* NULL path pointer */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "OPEN with NULL EDX sets CF (do_open guards NULL)");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "OPEN with NULL EDX AX=0x0002 (guarded, not a fault)");
    }

    /* --- do_setdta does NOT dereference EDX: SETDTA NULL just stores it
     *     (no fault), and GETDTA reads it back. Characterization: a NULL DTA
     *     is recorded verbatim, not rejected. -------------------------------- */
    {
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = 0u;   /* SET DTA = NULL (no deref in do_setdta) */
        int21_dispatch(&s);
        CHECK(frame_cf(&s) == 0, "SETDTA NULL clears CF (no dereference, no fault)");

        /* GETDTA: with g_dta==0 the handler returns the PSP:0x80 default, NOT 0
         * (do_getdta falls back to &g_cur_psp->cmd_tail[0]). Characterize that. */
        int_frame_t g = fresh_frame();
        g.eax = 0x2F00u;
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "GETDTA after SETDTA NULL clears CF");
        CHECK(g.ebx == (uint32_t)(uintptr_t)&g_test_psp.cmd_tail[0],
              "GETDTA with a NULL-set DTA returns the PSP:0x80 default (not 0)");
    }

    /* =====================================================================
     * 3. IMPLEMENTED-BUT-UNTESTED resident functions (beads initech-1zk).
     *    Assert what the CODE returns (characterization), not idealized DOS.
     * ===================================================================== */

    /* --- AH=0Eh SELECT DISK: AL = number of logical drives = 1 (A: only).
     *     do_seldisk IGNORES the requested DL (only A: exists). Pin both: a
     *     valid DL and a wild DL return AL=1, CF clear. ----------------------- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x0E00u; f.edx = 0u;       /* DL=0 -> A: */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "SELDISK clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 1u, "SELDISK returns AL=1 (one logical drive)");

        int_frame_t g = fresh_frame();
        g.eax = 0x0E00u; g.edx = 9u;       /* wild DL -> still AL=1 (DL ignored) */
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "SELDISK with a wild DL still clears CF (DL ignored)");
        CHECK((uint8_t)(g.eax & 0xFFu) == 1u,
              "SELDISK with a wild DL still returns AL=1 (only A: exists)");
    }

    /* --- AH=19h GET CURRENT DISK: AL = 0 (A:). No error path. ------------ */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x1900u;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "GETDISK clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u, "GETDISK returns AL=0 (current drive A:)");
    }

    /* --- AH=47h GET CWD: root -> empty string in DS:SI (ESI). Bad drive
     *     (>1) -> CF=1, AX=0x000F, and seeds the extended-error code. -------- */
    {
        char *buf = (char *)alloc_low(64);
        CHECK(buf != NULL, "GETCWD buffer in low 4 GiB");
        if (buf) {
            buf[0] = 'X';  /* poison so we prove the handler wrote the NUL */
            int_frame_t f = fresh_frame();
            f.eax = 0x4700u; f.edx = 1u;   /* DL=1 -> A: */
            f.esi = (uint32_t)(uintptr_t)buf;
            f.eflags |= CF_BIT;
            int21_dispatch(&f);
            CHECK(frame_cf(&f) == 0, "GETCWD A: clears CF");
            CHECK(buf[0] == '\0', "GETCWD root CWD is an empty string (no subdirs yet)");
        }

        int_frame_t b = fresh_frame();
        b.eax = 0x4700u; b.edx = 7u;       /* DL=7 -> no such drive */
        b.esi = 0u;
        int21_dispatch(&b);
        CHECK(frame_cf(&b) == 1, "GETCWD bad drive sets CF");
        CHECK((uint16_t)(b.eax & 0xFFFFu) == 0x000Fu, "GETCWD bad drive AX=0x000F");
    }

    /* --- AH=59h GET EXTENDED ERROR: do_getcwd(bad drive) above noted 0x000F,
     *     so 59h now reports it -- the (class, action, locus) triple the code
     *     derives. Then clear and confirm the no-error triple. ---------------- */
    {
        /* The prior GETCWD bad-drive call set g_last_error = 0x000F. 0x000F is
         * NOT one of the codes do_geterr maps explicitly (2/3/5/6/4), so it
         * lands in the UNKNOWN bucket: class 0x0D, action 0x05, locus 0x01. */
        int_frame_t f = fresh_frame();
        f.eax = 0x5900u;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "GETERR clears CF (59h never fails)");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == 0x000Fu,
              "GETERR AX = the last error code (0x000F from the bad-drive GETCWD)");
        CHECK((uint8_t)((f.ebx >> 8) & 0xFFu) == 0x0Du,
              "GETERR BH=class 0x0D (unknown) for the unmapped 0x000F code");
        CHECK((uint8_t)(f.ebx & 0xFFu) == 0x05u, "GETERR BL=action 0x05 (ignore)");
        CHECK((uint8_t)((f.ecx >> 8) & 0xFFu) == 0x01u, "GETERR CH=locus 0x01 (unknown)");
        CHECK((uint8_t)(f.ecx & 0xFFu) == 0u, "GETERR CL=0 (DOS zeroes it)");
    }
    {
        /* Seed a KNOWN, mapped code (file-not-found) and characterize its
         * triple; then clear and confirm the no-error report. */
        int21_note_error(INT21_ERR_FILE_NOT_FOUND);
        int_frame_t f = fresh_frame();
        f.eax = 0x5900u;
        int21_dispatch(&f);
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "GETERR AX = file-not-found after seeding it");
        CHECK((uint8_t)((f.ebx >> 8) & 0xFFu) == 0x08u,
              "GETERR BH=class 0x08 (NOT FOUND) for file-not-found");
        CHECK((uint8_t)(f.ebx & 0xFFu) == 0x03u, "GETERR BL=action 0x03 (user/retry)");
        CHECK((uint8_t)((f.ecx >> 8) & 0xFFu) == 0x02u,
              "GETERR CH=locus 0x02 (block device) for file-not-found");

        int21_note_error(0u);
        int_frame_t g = fresh_frame();
        g.eax = 0x5900u;
        int21_dispatch(&g);
        CHECK((uint16_t)(g.eax & 0xFFFFu) == 0u, "GETERR AX=0 when there is no error");
        CHECK((uint8_t)((g.ebx >> 8) & 0xFFu) == 0u, "GETERR BH=0 when there is no error");
        CHECK((uint8_t)(g.ebx & 0xFFu) == 0u, "GETERR BL=0 when there is no error");
        CHECK((uint16_t)(g.ecx & 0xFFFFu) == 0u, "GETERR CX=0 when there is no error");
    }

    /* --- AH=62h GET PSP: BX = (current PSP flat addr >> 4), with a bound PSP.
     *     Characterize the flat-model paragraph encoding (psp.h Option B). ---- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x6200u;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "GETPSP clears CF");
        uint16_t want = (uint16_t)(((uintptr_t)&g_test_psp >> 4) & 0xFFFFu);
        CHECK((uint16_t)(f.ebx & 0xFFFFu) == want,
              "GETPSP BX = (current PSP flat address >> 4) -- the flat-model paragraph");
    }

    /* --- AH=30h GET VERSION lock (spec/int21h_calling_convention.json): the
     *     GETVER contract is pinned -- AL=major, AH=minor, BH zeroed -- so a
     *     drift in the version constants is caught here too. ------------------ */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x3000u;
        f.ebx = 0x0000FF00u;   /* caller BH=0xFF: must be zeroed on return */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "GETVER clears CF");
        CHECK((uint8_t)(f.eax & 0xFFu) == INT21_VER_MAJOR, "GETVER AL=major (locked)");
        CHECK((uint8_t)((f.eax >> 8) & 0xFFu) == INT21_VER_MINOR, "GETVER AH=minor (locked)");
        CHECK((uint8_t)((f.ebx >> 8) & 0xFFu) == 0u, "GETVER zeroes BH (OEM byte)");
    }

    /* --- ADR-0003 DEC-14 / initech-tzq: INT 21h user-pointer guards. -------- *
     * do_read / do_write / the FINDFIRST DTA write validate [EDX,EDX+CX) before
     * dereferencing: NULL or a 32-bit-wrapping range -> CF=1, AX=0x0009 instead
     * of faulting/scribbling; a zero count always succeeds. The
     * INT21_MUTATE_NO_PTR_GUARD mutant disables the guard, so case (a) -- a NULL
     * read of the non-empty HELLO.TXT -- SIGSEGVs (the exact fault the guard
     * prevents), which the mutant oracle reads as RED. */
    {
        /* (a) READ into a NULL buffer on a VALID handle, count>0 -> 0x0009. */
        uint32_t h = open_hello();
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = h; f.ecx = 8u; f.edx = 0u;   /* NULL dst */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "READ NULL buffer (valid handle) sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_MEMORY,
              "READ NULL buffer -> AX=0x0009 (invalid memory, DEC-14)");
    }
    {
        /* (b) WRITE from a NULL buffer to CON (handle 1), count>0 -> 0x0009. */
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u; f.ebx = 1u; f.ecx = 4u; f.edx = 0u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "WRITE NULL buffer to CON sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_MEMORY,
              "WRITE NULL buffer -> AX=0x0009 (DEC-14)");
    }
    {
        /* (c) WRITE with a count that WRAPS the 32-bit space -> 0x0009. */
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u; f.ebx = 1u; f.ecx = 0x20u; f.edx = 0xFFFFFFF0u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "WRITE wrapping [EDX,EDX+CX) sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_MEMORY,
              "WRITE 32-bit-wrap buffer -> AX=0x0009 (DEC-14)");
    }
    {
        /* (d) CX=0 with a NULL buffer is a no-op success (no memory access). */
        int_frame_t f = fresh_frame();
        f.eax = 0x4000u; f.ebx = 1u; f.ecx = 0u; f.edx = 0u;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "WRITE CX=0 NULL buffer clears CF (no access)");
        CHECK((f.eax & 0xFFFFFFFFu) == 0u, "WRITE CX=0 returns 0 bytes");
    }
    {
        /* (e) A VALID low buffer is NOT rejected (the guard's negative space). */
        uint32_t h = open_hello();
        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(16); memset(dst, 0, 16);
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = h; f.ecx = 8u; f.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "READ into a valid low buffer clears CF (guard allows)");
        CHECK((f.eax & 0xFFFFFFFFu) == 8u, "READ valid buffer returns 8 bytes");
    }
    /* NOTE: the FINDFIRST DTA-write guard (emit_find_data validates the DTA
     * against sizeof(find_data_t) before writing) is implemented and uses the
     * same user_buf_ok helper proven above; it is not exercised here because
     * this suite's mock backend has no dir_entry hook (FINDFIRST returns
     * no-more-files before the write). The guard is defensive + ADR-documented
     * (DEC-14); a dedicated FINDFIRST-DTA test belongs in test_fileio. */

    return TEST_SUMMARY("test_int21_edge");
}
