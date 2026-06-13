/* test_fileio.c -- host unit oracle for the INT 21h file-handle functions
 * (beads initech-509.5 read-side). Factory test: libc OK, reuses
 * seed/test_assert.h. Drives 3Dh OPEN / 3Fh READ / 3Eh CLOSE / 42h LSEEK /
 * 4Eh FINDFIRST / 4Fh FINDNEXT / 1Ah SETDTA / 2Fh GETDTA through the REAL
 * artifact int21_dispatch, with a MOCK file backend (an in-memory directory)
 * standing in for the FAT12 volume (brief Sec 6 Step 4: "wire a mock blockdev").
 *
 * Ref: docs/research/fs-mount-sft-ground-truth.md Sec 4 (the exact spec), Sec
 *      4.5 (the 43-byte DTA find-data layout); spec/find_data.h (LOCKED);
 *      spec/dos_structs.h (dir_entry_t); spec/int21h_register.json. CLAUDE.md
 *      Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6 (mutation-prove),
 *      Rule 12 (ASCII).
 *
 * Strategy: int21.c routes FAT-specific work through an int21_file_backend_t we
 * bind to a mock (a fixed array of {name, attr, data, size} files). The
 * dispatcher's CON sink + terminate hook are abstracted as in test_int21.c. We
 * build a fake int_frame_t, call int21_dispatch, and assert outputs + the CF
 * bit + the DTA find-data bytes.
 *
 * MULTI-TENANT (beads initech-0qh; epic initech-6qy): the backend is now
 * POSITIONED + STATELESS -- open() merely LOCATES a file (returns its dir entry
 * + a root-dir slot), read_at()/write_at() are positioned over a per-file byte
 * array indexed by slot, and each SFT slot carries its own position. The mock
 * mirrors that: a fixed table of mock files, each its own byte buffer; open
 * returns the table index as the "slot"; read_at copies from offset; write_at
 * overwrites/extends at offset. So this oracle now PROVES two files open
 * concurrently with independent positions (the old single-buffer-busy case is
 * replaced by a true-concurrency assertion).
 *
 * MUTATION (Rule 6), driven by make:
 *   -DINT21_MUTATE_READ_IGNORE_OFFSET : READ does not advance file_offset, so a
 *       chunked read re-reads the start -> the two-chunk READ test goes RED.
 *   -DINT21_MUTATE_LSEEK_WHENCE       : LSEEK whence 2 (from end) uses base 0,
 *       so SEEK_END lands at the wrong offset -> the LSEEK test goes RED.
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

/* --- standard process binding (mirrors test_int21.c) -------------------- */
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

/* --- the MOCK file backend (an in-memory root directory, positioned) ------- *
 * Each mock file owns a mutable byte buffer (so write_at can overwrite/extend)
 * and a live size. open()/create() return the table index as the "root slot";
 * read_at()/write_at() index back into the table by slot. This makes the mock
 * behave exactly like the positioned FAT backend: STATELESS w.r.t. position
 * (the SFT slot holds it) and capable of N concurrent open files. */
#define MOCK_CAP 1024u
typedef struct mock_file {
    const char *name8;     /* 8-char field (space-padded conceptual; we pad)  */
    const char *ext3;      /* 3-char ext                                       */
    uint8_t     attr;      /* DIR_ATTR_* */
    uint8_t     data[MOCK_CAP];
    uint32_t    size;
    int         present;   /* 0 once unlinked / before create                 */
} mock_file_t;

/* HELLO.TXT mirrors the FAT fixture intent; README is a second file; DISK is a
 * volume label that the *.* enumeration must SKIP unless requested. Slots 0..2
 * are the seed directory; slot 3 is a spare for CREAT (OUT.TXT). */
static const char *HELLO_DATA = "Hello from InitechOS test file.\n";
static mock_file_t g_mock[4];

static void mock_reset_dir(void)
{
    memset(g_mock, 0, sizeof(g_mock));
    g_mock[0].name8 = "HELLO   "; g_mock[0].ext3 = "TXT"; g_mock[0].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[0].data, HELLO_DATA, strlen(HELLO_DATA));
    g_mock[0].size = (uint32_t)strlen(HELLO_DATA); g_mock[0].present = 1;

    g_mock[1].name8 = "README  "; g_mock[1].ext3 = "   "; g_mock[1].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[1].data, "read me", 7);
    g_mock[1].size = 7; g_mock[1].present = 1;

    g_mock[2].name8 = "DISK    "; g_mock[2].ext3 = "   "; g_mock[2].attr = DIR_ATTR_VOLLABEL;
    g_mock[2].size = 0; g_mock[2].present = 1;

    g_mock[3].present = 0;   /* spare slot for CREAT */
}
#define MOCK_N 4u

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

/* Map a "HELLO.TXT" name to a comparable formatted name and find the slot. */
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

/* OPEN: LOCATE only -- return the dir entry + the table index as the slot. No
 * whole-file read; no single-buffer limit (N files open concurrently). */
static uint16_t mock_open(const char *name83, dir_entry_t *out_entry,
                          uint32_t *out_slot)
{
    int idx = mock_find_by_name(name83);
    if (idx < 0) return 0x0002u;                  /* not found */
    fill_dir_entry(&g_mock[idx], out_entry);
    *out_slot = (uint32_t)idx;
    return 0u;
}

/* READ_AT: positioned read from the per-file buffer at `offset`. EOF -> 0. */
static uint16_t mock_read_at(const dir_entry_t *e, uint32_t offset,
                             uint8_t *buf, uint32_t len, uint32_t *out_read)
{
    /* Resolve the file by start_cluster? The mock keys on the dir entry's name
     * fields (the dispatcher passes the SFT's dir_entry copy). Build the name
     * and find the slot so reads are served from the live buffer. */
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

/* ---- mock WRITE backend (beads initech-0qh, positioned) ------------------- *
 * create() claims the spare slot (index 3) and records the name; write_at()
 * overwrites/extends the slot's buffer at `offset` and returns the updated dir
 * entry; close() is a no-op; unlink() forgets a named file. The committed bytes
 * are simply the slot's buffer (no separate flush -- per-call commit). */
static int      g_mock_unlinked  = 0;
static char     g_mock_unlinked_name[16];

static uint16_t mock_create(const char *name83, dir_entry_t *out_entry,
                            uint32_t *out_slot)
{
    /* Use the spare slot (index 3) for the created file; truncate to size 0. */
    int idx = 3;
    /* Parse name83 into 8.3 fields for the slot's name. */
    static char nm8[9], ex3[4];
    memset(nm8, ' ', 8); nm8[8] = '\0';
    memset(ex3, ' ', 3); ex3[3] = '\0';
    int i = 0, j = 0;
    for (; name83[i] && name83[i] != '.' && j < 8; i++, j++) {
        char c = name83[i]; if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        nm8[j] = c;
    }
    while (name83[i] && name83[i] != '.') i++;
    if (name83[i] == '.') {
        i++; j = 0;
        for (; name83[i] && j < 3; i++, j++) {
            char c = name83[i]; if (c >= 'a' && c <= 'z') c = (char)(c - 32);
            ex3[j] = c;
        }
    }
    g_mock[idx].name8   = nm8;
    g_mock[idx].ext3    = ex3;
    g_mock[idx].attr    = DIR_ATTR_ARCHIVE;
    g_mock[idx].size    = 0;
    g_mock[idx].present = 1;
    memset(g_mock[idx].data, 0, MOCK_CAP);
    fill_dir_entry(&g_mock[idx], out_entry);
    *out_slot = (uint32_t)idx;
    return 0u;
}

static uint16_t mock_write_at(uint32_t slot, uint32_t offset, const uint8_t *data,
                              uint32_t len, uint32_t *out_written,
                              dir_entry_t *out_entry)
{
    if (slot >= MOCK_N || !g_mock[slot].present) { *out_written = 0; return 0x0005u; }
    if (offset + len > MOCK_CAP) { *out_written = 0; return 0x0005u; }
    /* Zero-fill any hole between current size and offset (positioned semantics). */
    if (offset > g_mock[slot].size) {
        memset(g_mock[slot].data + g_mock[slot].size, 0, offset - g_mock[slot].size);
    }
    memcpy(g_mock[slot].data + offset, data, len);
    if (offset + len > g_mock[slot].size) g_mock[slot].size = offset + len;
    *out_written = len;
    fill_dir_entry(&g_mock[slot], out_entry);   /* updated size */
    return 0u;
}

static void mock_close(uint32_t slot) { (void)slot; }

static uint16_t mock_unlink(const char *name83)
{
    /* "DELETE.ME" exists; anything else not found (mirrors a small dir). */
    if (strcmp(name83, "DELETE.ME") != 0) return 0x0002u;
    g_mock_unlinked = 1;
    int i = 0;
    for (; name83[i] && i < (int)sizeof(g_mock_unlinked_name) - 1; i++)
        g_mock_unlinked_name[i] = name83[i];
    g_mock_unlinked_name[i] = '\0';
    return 0u;
}

static uint16_t mock_dir_entry(uint32_t index, dir_entry_t *out_entry, int *out_found)
{
    *out_found = 0;
    /* Enumerate only the PRESENT seed files (slots 0..2) so FINDFIRST/NEXT see a
     * stable directory regardless of CREAT having touched slot 3. */
    uint32_t seen = 0;
    for (uint32_t i = 0; i < 3u; i++) {
        if (!g_mock[i].present) continue;
        if (seen == index) {
            fill_dir_entry(&g_mock[i], out_entry);
            *out_found = 1;
            return 0u;
        }
        seen++;
    }
    return 0u;
}

static const int21_file_backend_t g_mock_backend = {
    mock_open, mock_read_at, mock_dir_entry,
    mock_create, mock_write_at, mock_close, mock_unlink,
    NULL   /* freespace: not exercised by the read/write file oracle (AH=36h is
              covered end-to-end on the emulator, make test-datetime) */
};

int main(void)
{
    mock_reset_dir();

    int21_set_sink(sink_capture);
    bind_standard_process();
    int21_set_file_backend(&g_mock_backend);

    /* --- OPEN a missing file -> CF=1, AX=0x0002 --------------------------- */
    {
        uint32_t edx = low_dup("NOPE.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "OPEN missing file sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "OPEN missing file AX=0x0002");
    }

    /* --- OPEN a path with a subdir separator -> CF=1, AX=0x0003 ---------- */
    {
        uint32_t edx = low_dup("SUB\\HELLO.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "OPEN subdir path sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "OPEN subdir path AX=0x0003");
    }

    uint32_t hello_handle = 0;
    /* --- OPEN HELLO.TXT -> CF clear, EAX = handle (lowest free = 5) ------- */
    {
        uint32_t edx = low_dup("HELLO.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "OPEN HELLO.TXT clears CF");
        hello_handle = (uint16_t)(f.eax & 0xFFFFu);
        CHECK(hello_handle == 5u, "OPEN returns lowest free handle (5)");
    }

    /* --- a SECOND concurrent OPEN now SUCCEEDS (multi-tenant, beads
     *     initech-0qh): two files open at once, each its own handle + position.
     *     Prove independence by reading from EACH and confirming no cross-talk,
     *     then close the second so the rest of the HELLO tests are unaffected. */
    {
        uint32_t edx = low_dup("README");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "second concurrent OPEN succeeds (multi-tenant, no single-buffer limit)");
        uint32_t readme_handle = (uint16_t)(f.eax & 0xFFFFu);
        CHECK(readme_handle == 6u, "second concurrent OPEN gets the next handle (6)");

        /* Read 3 bytes from README (handle 6) -- "rea". */
        uint8_t *rb = (uint8_t *)(uintptr_t)alloc_low(16); memset(rb, 0, 16);
        int_frame_t r = fresh_frame();
        r.eax = 0x3F00u; r.ebx = readme_handle; r.ecx = 3u; r.edx = (uint32_t)(uintptr_t)rb;
        int21_dispatch(&r);
        CHECK(r.eax == 3u && memcmp(rb, "rea", 3) == 0,
              "READ on the 2nd handle reads README from ITS own offset (no cross-talk)");

        /* HELLO (handle 5) position is still 0 -- read 5 bytes -> "Hello". */
        uint8_t *hb = (uint8_t *)(uintptr_t)alloc_low(16); memset(hb, 0, 16);
        int_frame_t r2 = fresh_frame();
        r2.eax = 0x3F00u; r2.ebx = hello_handle; r2.ecx = 5u; r2.edx = (uint32_t)(uintptr_t)hb;
        int21_dispatch(&r2);
        CHECK(r2.eax == 5u && memcmp(hb, "Hello", 5) == 0,
              "the 1st handle's position is INDEPENDENT of the 2nd handle's read");

        /* Rewind HELLO so the two-chunk READ test below sees offset 0. */
        int_frame_t s = fresh_frame();
        s.eax = 0x4200u; s.ebx = hello_handle; s.ecx = 0u; s.edx = 0u;
        int21_dispatch(&s);
        CHECK(s.eax == 0u, "rewind HELLO to offset 0 for the following tests");

        /* Close the README handle (concurrency proven). */
        int_frame_t c = fresh_frame();
        c.eax = 0x3E00u; c.ebx = readme_handle;
        int21_dispatch(&c);
        CHECK(frame_cf(&c) == 0, "CLOSE the 2nd concurrent handle clears CF");
    }

    uint32_t hsize = (uint32_t)strlen(HELLO_DATA);
    /* --- READ the whole file in TWO chunks; concatenation must equal data. */
    {
        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(256);
        CHECK(dst != NULL, "read dst buffer in low 4 GiB");
        memset(dst, 0, 256);

        /* chunk 1: 10 bytes */
        int_frame_t f = fresh_frame();
        f.eax = 0x3F00u; f.ebx = hello_handle; f.ecx = 10u;
        f.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "READ chunk1 clears CF");
        CHECK(f.eax == 10u, "READ chunk1 returns 10 bytes");

        /* chunk 2: the rest (ask for more than remains -> capped) */
        int_frame_t g = fresh_frame();
        g.eax = 0x3F00u; g.ebx = hello_handle; g.ecx = 200u;
        g.edx = (uint32_t)(uintptr_t)(dst + 10);
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "READ chunk2 clears CF");
        CHECK(g.eax == (hsize - 10u), "READ chunk2 returns the remaining bytes");

        CHECK(memcmp(dst, HELLO_DATA, hsize) == 0,
              "two-chunk READ concatenates to the exact file contents (offset advanced)");

        /* chunk 3: at EOF -> 0 bytes, CF clear */
        int_frame_t h = fresh_frame();
        h.eax = 0x3F00u; h.ebx = hello_handle; h.ecx = 16u;
        h.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&h);
        CHECK(frame_cf(&h) == 0, "READ at EOF clears CF");
        CHECK(h.eax == 0u, "READ at EOF returns 0 bytes");
    }

    /* --- LSEEK SEEK_SET to 6, then read "from " ... actually read 5 bytes - */
    {
        int_frame_t s = fresh_frame();
        s.eax = 0x4200u;          /* AH=42h, AL=0 (SEEK_SET) */
        s.ebx = hello_handle;
        s.ecx = 0u; s.edx = 6u;   /* offset 6 */
        s.eflags |= CF_BIT;
        int21_dispatch(&s);
        CHECK(frame_cf(&s) == 0, "LSEEK SEEK_SET clears CF");
        CHECK(s.eax == 6u, "LSEEK SEEK_SET returns new offset 6");

        uint8_t *dst = (uint8_t *)(uintptr_t)alloc_low(16);
        memset(dst, 0, 16);
        int_frame_t r = fresh_frame();
        r.eax = 0x3F00u; r.ebx = hello_handle; r.ecx = 4u;
        r.edx = (uint32_t)(uintptr_t)dst;
        int21_dispatch(&r);
        /* HELLO_DATA[6..9] = "from" */
        CHECK(r.eax == 4u, "READ after SEEK_SET returns 4 bytes");
        CHECK(memcmp(dst, HELLO_DATA + 6, 4) == 0,
              "READ after SEEK_SET reads from the sought offset");
    }

    /* --- LSEEK SEEK_END (whence 2) lands at file_size --------------------- */
    {
        int_frame_t s = fresh_frame();
        s.eax = 0x4202u;          /* AH=42h, AL=2 (SEEK_END) */
        s.ebx = hello_handle;
        s.ecx = 0u; s.edx = 0u;   /* offset 0 from end */
        int21_dispatch(&s);
        CHECK(frame_cf(&s) == 0, "LSEEK SEEK_END clears CF");
        CHECK(s.eax == hsize, "LSEEK SEEK_END(0) returns file_size");

        /* a READ here returns 0 (at EOF) */
        uint8_t b[4];
        int_frame_t r = fresh_frame();
        r.eax = 0x3F00u; r.ebx = hello_handle; r.ecx = 4u;
        r.edx = (uint32_t)(uintptr_t)b;
        int21_dispatch(&r);
        CHECK(r.eax == 0u, "READ at SEEK_END offset returns 0");
    }

    /* --- LSEEK bad whence -> CF=1, AX=0x0001 ------------------------------ */
    {
        int_frame_t s = fresh_frame();
        s.eax = 0x4203u;          /* whence 3 is invalid */
        s.ebx = hello_handle;
        int21_dispatch(&s);
        CHECK(frame_cf(&s) == 1, "LSEEK bad whence sets CF");
        CHECK((uint16_t)(s.eax & 0xFFFFu) == INT21_ERR_INVALID_FUNCTION,
              "LSEEK bad whence AX=0x0001");
    }

    /* --- CLOSE the file; buffer freed so a new OPEN succeeds -------------- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x3E00u; f.ebx = hello_handle;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "CLOSE clears CF");

        /* the SFT slot must be free again; reopen succeeds */
        uint32_t edx = low_dup("README");
        int_frame_t g = fresh_frame();
        g.eax = 0x3D00u; g.edx = edx;
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "OPEN after CLOSE succeeds (buffer freed)");
        /* close it back */
        int_frame_t h = fresh_frame();
        h.eax = 0x3E00u; h.ebx = (uint16_t)(g.eax & 0xFFFFu);
        int21_dispatch(&h);
        CHECK(frame_cf(&h) == 0, "CLOSE reopened file clears CF");
    }

    /* --- CLOSE a predefined handle (1) is a no-op success ----------------- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x3E00u; f.ebx = 1u;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "CLOSE predefined handle 1 is a no-op success");
    }

    /* --- CLOSE a bad handle -> CF=1, AX=0x0006 ---------------------------- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x3E00u; f.ebx = 9u;   /* closed */
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "CLOSE bad handle sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_HANDLE,
              "CLOSE bad handle AX=0x0006");
    }

    /* --- SETDTA / GETDTA round-trip --------------------------------------- */
    {
        uint8_t *dta = (uint8_t *)(uintptr_t)alloc_low(64);
        CHECK(dta != NULL, "DTA buffer in low 4 GiB");
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = (uint32_t)(uintptr_t)dta;
        int21_dispatch(&s);
        CHECK(frame_cf(&s) == 0, "SETDTA clears CF");

        int_frame_t g = fresh_frame();
        g.eax = 0x2F00u;
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "GETDTA clears CF");
        CHECK(g.ebx == (uint32_t)(uintptr_t)dta, "GETDTA returns the DTA set by SETDTA in EBX");
    }

    /* --- FINDFIRST "*.*" then FINDNEXT: enumerate regular files, skip the
     *     volume label. The DTA holds the 43-byte find_data_t. ------------- */
    {
        /* Point the DTA at a fresh 43-byte block we can inspect. */
        find_data_t *fd = (find_data_t *)(uintptr_t)alloc_low(FIND_DATA_SIZE);
        CHECK(fd != NULL, "find_data DTA in low 4 GiB");
        memset(fd, 0, FIND_DATA_SIZE);
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = (uint32_t)(uintptr_t)fd;
        int21_dispatch(&s);

        uint32_t edx = low_dup("*.*");
        int_frame_t ff = fresh_frame();
        ff.eax = 0x4E00u; ff.ecx = 0u; ff.edx = edx;  /* attr mask 0 = files only */
        ff.eflags |= CF_BIT;
        int21_dispatch(&ff);
        CHECK(frame_cf(&ff) == 0, "FINDFIRST *.* finds the first regular file");
        CHECK(strcmp(fd->fname, "HELLO.TXT") == 0, "FINDFIRST yields HELLO.TXT");
        CHECK(fd->fsize == hsize, "FINDFIRST find_data fsize == file size");
        CHECK(fd->ftime == 0x1234u, "FINDFIRST find_data ftime copied through");
        CHECK(fd->fdate == 0x5678u, "FINDFIRST find_data fdate copied through");

        /* dww byte-golden: the found-entry fields must land at the REAL-DOS DTA
         * offsets (attr@0x15, time@0x16, date@0x18, size@0x1A, name@0x1E). A
         * real-DOS program / differential test reads these FIXED bytes, not the
         * struct -- so inspect raw bytes. This guards the writer (emit_find_data);
         * the per-field offsetof _Static_asserts in spec/find_data.h guard the
         * struct. Mutation-proof: drop a writer field or move a struct field and
         * one of these goes RED. */
        {
            const uint8_t *raw = (const uint8_t *)fd;
            CHECK(raw[0x15] == fd->attr,                       "DTA byte 0x15 = found attribute (real DOS)");
            CHECK(*(const uint16_t *)(raw + 0x16) == 0x1234u,  "DTA byte 0x16 = file time (real DOS)");
            CHECK(*(const uint16_t *)(raw + 0x18) == 0x5678u,  "DTA byte 0x18 = file date (real DOS)");
            CHECK(*(const uint32_t *)(raw + 0x1A) == hsize,    "DTA byte 0x1A = file size (real DOS)");
            CHECK(memcmp(raw + 0x1E, "HELLO.TXT", 10) == 0,    "DTA byte 0x1E = file name (real DOS)");
        }

        int_frame_t fn = fresh_frame();
        fn.eax = 0x4F00u;
        int21_dispatch(&fn);
        CHECK(frame_cf(&fn) == 0, "FINDNEXT finds the second regular file");
        CHECK(strcmp(fd->fname, "README") == 0, "FINDNEXT yields README (skips no files)");

        /* the next FINDNEXT must SKIP the volume label and report no more */
        int_frame_t fn2 = fresh_frame();
        fn2.eax = 0x4F00u;
        int21_dispatch(&fn2);
        CHECK(frame_cf(&fn2) == 1, "FINDNEXT past the last regular file sets CF");
        CHECK((uint16_t)(fn2.eax & 0xFFFFu) == INT21_ERR_NO_MORE_FILES,
              "FINDNEXT exhausted AX=0x0012 (volume label was skipped)");
    }

    /* --- exact-match FINDFIRST "README" finds exactly that file ----------- */
    {
        find_data_t *fd = (find_data_t *)(uintptr_t)alloc_low(FIND_DATA_SIZE);
        memset(fd, 0, FIND_DATA_SIZE);
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = (uint32_t)(uintptr_t)fd;
        int21_dispatch(&s);

        uint32_t edx = low_dup("README");
        int_frame_t ff = fresh_frame();
        ff.eax = 0x4E00u; ff.ecx = 0u; ff.edx = edx;
        int21_dispatch(&ff);
        CHECK(frame_cf(&ff) == 0, "FINDFIRST exact name finds the file");
        CHECK(strcmp(fd->fname, "README") == 0, "FINDFIRST exact name yields README");

        int_frame_t fn = fresh_frame();
        fn.eax = 0x4F00u;
        int21_dispatch(&fn);
        CHECK(frame_cf(&fn) == 1, "FINDNEXT after an exact 1-file match is exhausted");
    }

    /* --- FINDNEXT with no active search -> CF=1, AX=0x0012 ---------------- */
    {
        /* re-bind a clean process to clear find state */
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        int_frame_t fn = fresh_frame();
        fn.eax = 0x4F00u;
        int21_dispatch(&fn);
        CHECK(frame_cf(&fn) == 1, "FINDNEXT with no FINDFIRST sets CF");
        CHECK((uint16_t)(fn.eax & 0xFFFFu) == INT21_ERR_NO_MORE_FILES,
              "FINDNEXT with no active search AX=0x0012");
    }

    /* --- WRITE path (beads initech-0qh, positioned): CREAT + WRITE(s) + CLOSE,
     *     then re-OPEN + READ BACK through the dispatcher (proves the positioned
     *     write committed). Plus a positioned OVERWRITE via LSEEK to prove
     *     write_at honours the per-handle offset. */
    {
        mock_reset_dir();
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);

        /* CREAT "OUT.TXT", CX=0 (normal attr) -> handle, CF clear. */
        uint32_t edx = low_dup("OUT.TXT");
        int_frame_t c = fresh_frame();
        c.eax = 0x3C00u; c.ecx = 0u; c.edx = edx;
        c.eflags |= CF_BIT;
        int21_dispatch(&c);
        CHECK(frame_cf(&c) == 0, "CREAT OUT.TXT clears CF");
        uint32_t wh = (uint16_t)(c.eax & 0xFFFFu);
        CHECK(wh == 5u, "CREAT returns the lowest free handle (5)");

        /* WRITE "hello\r\n" at offset 0 -> EAX = bytes written, CF clear. */
        const char *payload = "hello\r\n";
        uint32_t plen = (uint32_t)strlen(payload);
        uint32_t wbuf = low_dup(payload);
        int_frame_t w = fresh_frame();
        w.eax = 0x4000u; w.ebx = wh; w.ecx = plen; w.edx = wbuf;
        int21_dispatch(&w);
        CHECK(frame_cf(&w) == 0, "WRITE to file handle clears CF");
        CHECK(w.eax == plen, "WRITE returns the byte count written");

        /* a SECOND WRITE appends at the advanced position. */
        const char *more = "world";
        uint32_t mlen = (uint32_t)strlen(more);
        uint32_t mbuf = low_dup(more);
        int_frame_t w2 = fresh_frame();
        w2.eax = 0x4000u; w2.ebx = wh; w2.ecx = mlen; w2.edx = mbuf;
        int21_dispatch(&w2);
        CHECK(frame_cf(&w2) == 0 && w2.eax == mlen, "second WRITE appends at the advanced offset");

        /* CLOSE the write handle. */
        int_frame_t cl = fresh_frame();
        cl.eax = 0x3E00u; cl.ebx = wh;
        cl.eflags |= CF_BIT;
        int21_dispatch(&cl);
        CHECK(frame_cf(&cl) == 0, "CLOSE write handle clears CF");

        /* Re-OPEN + READ BACK through the dispatcher: the committed file is the
         * exact concatenation we wrote (proves positioned write reached the
         * backing store, no flush hack). */
        uint32_t oedx = low_dup("OUT.TXT");
        int_frame_t o = fresh_frame();
        o.eax = 0x3D00u; o.edx = oedx;
        int21_dispatch(&o);
        CHECK(frame_cf(&o) == 0, "re-OPEN OUT.TXT after write clears CF");
        uint32_t rh = (uint16_t)(o.eax & 0xFFFFu);
        uint8_t *rb = (uint8_t *)(uintptr_t)alloc_low(64); memset(rb, 0, 64);
        int_frame_t rd = fresh_frame();
        rd.eax = 0x3F00u; rd.ebx = rh; rd.ecx = 64u; rd.edx = (uint32_t)(uintptr_t)rb;
        int21_dispatch(&rd);
        CHECK(rd.eax == plen + mlen, "READ BACK returns the total bytes written");
        CHECK(memcmp(rb, "hello\r\nworld", plen + mlen) == 0,
              "READ BACK == the exact concatenation written (positioned write committed)");

        /* POSITIONED OVERWRITE: LSEEK rh to offset 0, WRITE "HELLO" over the
         * first 5 bytes, then re-read offset 0..4 and confirm the overwrite. */
        int_frame_t sk = fresh_frame();
        sk.eax = 0x4200u; sk.ebx = rh; sk.ecx = 0u; sk.edx = 0u;
        int21_dispatch(&sk);
        CHECK(sk.eax == 0u, "LSEEK read handle back to 0 for overwrite-via-write handle setup");
        /* Open a fresh WRITE handle, seek to 0, overwrite 5 bytes. */
        uint32_t oedx2 = low_dup("OUT.TXT");
        int_frame_t o2 = fresh_frame();
        o2.eax = 0x3D01u; o2.edx = oedx2;          /* AL=1 (write mode) */
        int21_dispatch(&o2);
        CHECK(frame_cf(&o2) == 0, "OPEN OUT.TXT in write mode clears CF");
        uint32_t wh2 = (uint16_t)(o2.eax & 0xFFFFu);
        uint32_t obuf = low_dup("HELLO");
        int_frame_t ow = fresh_frame();
        ow.eax = 0x4000u; ow.ebx = wh2; ow.ecx = 5u; ow.edx = obuf;
        int21_dispatch(&ow);
        CHECK(ow.eax == 5u && frame_cf(&ow) == 0, "positioned WRITE overwrites 5 bytes at offset 0");
        /* re-read offset 0..11 via rh (seek to 0 first). */
        int_frame_t sk2 = fresh_frame();
        sk2.eax = 0x4200u; sk2.ebx = rh; sk2.ecx = 0u; sk2.edx = 0u;
        int21_dispatch(&sk2);
        memset(rb, 0, 64);
        int_frame_t rd2 = fresh_frame();
        rd2.eax = 0x3F00u; rd2.ebx = rh; rd2.ecx = 64u; rd2.edx = (uint32_t)(uintptr_t)rb;
        int21_dispatch(&rd2);
        CHECK(memcmp(rb, "HELLO\r\nworld", plen + mlen) == 0,
              "positioned overwrite-in-place: first 5 bytes are now HELLO, tail intact");
    }

    /* --- RDWR (AL=2) round-trip (bcg.1): AH=3Dh AL=2 is read/write per DOS 3.3
     *     PRM; a mode-2 handle MUST permit WRITE. The bug: do_write gated on
     *     open_mode==SFT_MODE_WRITE only, so a mode-2 handle was denied --
     *     breaking in-place record updates (InitechBase .dbf). */
    {
        mock_reset_dir();
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);

        /* Make the file exist (CREAT + CLOSE), then OPEN it AL=2. */
        uint32_t cedx = low_dup("RW.TXT");
        int_frame_t cc = fresh_frame();
        cc.eax = 0x3C00u; cc.ecx = 0u; cc.edx = cedx;
        int21_dispatch(&cc);
        CHECK(frame_cf(&cc) == 0, "CREAT RW.TXT clears CF");
        int_frame_t cc_cl = fresh_frame();
        cc_cl.eax = 0x3E00u; cc_cl.ebx = (uint16_t)(cc.eax & 0xFFFFu);
        int21_dispatch(&cc_cl);

        uint32_t oedx = low_dup("RW.TXT");
        int_frame_t o = fresh_frame();
        o.eax = 0x3D02u; o.edx = oedx;             /* AL=2 (read/write mode) */
        int21_dispatch(&o);
        CHECK(frame_cf(&o) == 0, "OPEN RW.TXT in read/write mode clears CF");
        uint32_t h = (uint16_t)(o.eax & 0xFFFFu);

        /* WRITE through the RDWR handle MUST succeed (the bcg.1 regression). */
        uint32_t wbuf = low_dup("DATA!");
        int_frame_t w = fresh_frame();
        w.eax = 0x4000u; w.ebx = h; w.ecx = 5u; w.edx = wbuf;
        w.eflags |= CF_BIT;
        int21_dispatch(&w);
        CHECK(frame_cf(&w) == 0, "WRITE through AL=2 RDWR handle clears CF (bcg.1)");
        CHECK(w.eax == 5u, "WRITE through RDWR handle returns 5 bytes");

        /* READ BACK through the same handle (LSEEK 0 then READ): mode 2 reads too. */
        int_frame_t sk = fresh_frame();
        sk.eax = 0x4200u; sk.ebx = h; sk.ecx = 0u; sk.edx = 0u;
        int21_dispatch(&sk);
        CHECK(sk.eax == 0u, "LSEEK RDWR handle back to 0");
        uint8_t *rb2 = (uint8_t *)(uintptr_t)alloc_low(16); memset(rb2, 0, 16);
        int_frame_t rd = fresh_frame();
        rd.eax = 0x3F00u; rd.ebx = h; rd.ecx = 16u; rd.edx = (uint32_t)(uintptr_t)rb2;
        int21_dispatch(&rd);
        CHECK(rd.eax == 5u, "READ BACK through RDWR handle returns 5 bytes");
        CHECK(memcmp(rb2, "DATA!", 5) == 0, "RDWR round-trip: read back == written (bcg.1)");

        /* NEGATIVE (bcg.1): a READ-mode (AL=0) handle must still be DENIED a
         * WRITE -- this is what keeps the gate from being loosened to
         * "always writable". */
        uint32_t roedx = low_dup("RW.TXT");
        int_frame_t ro_o = fresh_frame();
        ro_o.eax = 0x3D00u; ro_o.edx = roedx;      /* AL=0 (read mode) */
        int21_dispatch(&ro_o);
        CHECK(frame_cf(&ro_o) == 0, "OPEN RW.TXT read-only clears CF");
        uint32_t roh = (uint16_t)(ro_o.eax & 0xFFFFu);
        uint32_t rowbuf = low_dup("NOPE!");
        int_frame_t row = fresh_frame();
        row.eax = 0x4000u; row.ebx = roh; row.ecx = 5u; row.edx = rowbuf;
        int21_dispatch(&row);
        CHECK(frame_cf(&row) == 1, "WRITE through AL=0 read handle is DENIED (bcg.1 negative)");
        CHECK((uint16_t)(row.eax & 0xFFFFu) == INT21_ERR_ACCESS_DENIED,
              "AL=0 read handle WRITE -> access denied 0x0005");
    }

    /* --- CREAT with NO write backend -> access denied --------------------- */
    {
        /* Bind a read-only backend (create/write_at=NULL) by reusing g_mock but
         * clearing the write hooks via a local read-only vtable. */
        static const int21_file_backend_t ro = { mock_open, mock_read_at, mock_dir_entry,
                                                  NULL, NULL, NULL, NULL, NULL };
        mock_reset_dir();
        bind_standard_process();
        int21_set_file_backend(&ro);
        uint32_t edx = low_dup("NOWRITE.TXT");
        int_frame_t c = fresh_frame();
        c.eax = 0x3C00u; c.ecx = 0u; c.edx = edx;
        int21_dispatch(&c);
        CHECK(frame_cf(&c) == 1, "CREAT with no write backend sets CF");
        CHECK((uint16_t)(c.eax & 0xFFFFu) == INT21_ERR_ACCESS_DENIED,
              "CREAT with no write backend AX=0x0005 (access denied)");
        int21_set_file_backend(&g_mock_backend);
    }

    /* --- AH=41h UNLINK: delete an existing file + a missing one ----------- */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);
        g_mock_unlinked = 0;

        uint32_t edx = low_dup("DELETE.ME");
        int_frame_t u = fresh_frame();
        u.eax = 0x4100u; u.edx = edx;
        u.eflags |= CF_BIT;
        int21_dispatch(&u);
        CHECK(frame_cf(&u) == 0, "UNLINK existing file clears CF");
        CHECK(g_mock_unlinked == 1 && strcmp(g_mock_unlinked_name, "DELETE.ME") == 0,
              "UNLINK forwarded the name to the backend");

        uint32_t edx2 = low_dup("GONE.XYZ");
        int_frame_t u2 = fresh_frame();
        u2.eax = 0x4100u; u2.edx = edx2;
        int21_dispatch(&u2);
        CHECK(frame_cf(&u2) == 1, "UNLINK missing file sets CF");
        CHECK((uint16_t)(u2.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "UNLINK missing file AX=0x0002");

        /* UNLINK a subdir path -> path not found. */
        uint32_t edx3 = low_dup("SUB\\X.TXT");
        int_frame_t u3 = fresh_frame();
        u3.eax = 0x4100u; u3.edx = edx3;
        int21_dispatch(&u3);
        CHECK(frame_cf(&u3) == 1 &&
              (uint16_t)(u3.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "UNLINK subdir path AX=0x0003");
    }

    return TEST_SUMMARY("test_fileio");
}
