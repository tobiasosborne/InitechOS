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

/* --- the MOCK file backend (an in-memory root directory) ---------------- */
typedef struct mock_file {
    const char *name8;     /* 8-char field (space-padded conceptual; we pad)  */
    const char *ext3;      /* 3-char ext                                       */
    uint8_t     attr;      /* DIR_ATTR_* */
    const char *data;
    uint32_t    size;
} mock_file_t;

/* HELLO.TXT mirrors the FAT fixture intent; ZZ.DAT is a second file; VOL is a
 * volume label that the *.* enumeration must SKIP unless requested. */
static const char *HELLO_DATA = "Hello from InitechOS test file.\n";
static mock_file_t g_mock[] = {
    { "HELLO   ", "TXT", DIR_ATTR_ARCHIVE,  NULL, 0 }, /* data/size set in main */
    { "README  ", "   ", DIR_ATTR_ARCHIVE,  "read me", 7 },
    { "DISK    ", "   ", DIR_ATTR_VOLLABEL, "", 0 },   /* volume label */
};
#define MOCK_N ((uint32_t)(sizeof(g_mock) / sizeof(g_mock[0])))

static int      g_mock_buf_in_use = 0;
static uint8_t  g_mock_filebuf[4096];

static void fill_dir_entry(const mock_file_t *m, dir_entry_t *de)
{
    memset(de, 0, sizeof(*de));
    for (int i = 0; i < 8; i++) de->filename[i]  = (uint8_t)m->name8[i];
    for (int i = 0; i < 3; i++) de->extension[i] = (uint8_t)m->ext3[i];
    de->attribute  = m->attr;
    de->mtime      = 0x1234u;
    de->mdate      = 0x5678u;
    de->file_size  = m->size;
}

static int mock_find_by_name(const char *name83)
{
    /* name83 is "HELLO.TXT" style. Build a comparable formatted name per entry. */
    for (uint32_t i = 0; i < MOCK_N; i++) {
        char fmt[13];
        int n = 0;
        for (int k = 0; k < 8 && g_mock[i].name8[k] != ' '; k++) fmt[n++] = g_mock[i].name8[k];
        if (g_mock[i].ext3[0] != ' ') {
            fmt[n++] = '.';
            for (int k = 0; k < 3 && g_mock[i].ext3[k] != ' '; k++) fmt[n++] = g_mock[i].ext3[k];
        }
        fmt[n] = '\0';
        /* case-insensitive compare */
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
                          const uint8_t **out_data, uint32_t *out_size)
{
    if (g_mock_buf_in_use) return 0x0004u;        /* single-buffer limit */
    int idx = mock_find_by_name(name83);
    if (idx < 0) return 0x0002u;                  /* not found */
    const mock_file_t *m = &g_mock[idx];
    if (m->size > sizeof(g_mock_filebuf)) return 0x0004u;
    memcpy(g_mock_filebuf, m->data, m->size);
    g_mock_buf_in_use = 1;
    fill_dir_entry(m, out_entry);
    *out_data = g_mock_filebuf;
    *out_size = m->size;
    return 0u;
}

static void mock_close(void) { g_mock_buf_in_use = 0; }

static uint16_t mock_dir_entry(uint32_t index, dir_entry_t *out_entry, int *out_found)
{
    *out_found = 0;
    if (index >= MOCK_N) return 0u;
    fill_dir_entry(&g_mock[index], out_entry);
    *out_found = 1;
    return 0u;
}

static const int21_file_backend_t g_mock_backend = {
    mock_open, mock_close, mock_dir_entry
};

int main(void)
{
    g_mock[0].data = HELLO_DATA;
    g_mock[0].size = (uint32_t)strlen(HELLO_DATA);

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

    /* --- a SECOND concurrent OPEN -> single-buffer limit AX=0x0004 ------- */
    {
        uint32_t edx = low_dup("README");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "second concurrent OPEN fails loud (single buffer)");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_TOO_MANY_OPEN,
              "second OPEN AX=0x0004 (buffer busy)");
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

    return TEST_SUMMARY("test_fileio");
}
