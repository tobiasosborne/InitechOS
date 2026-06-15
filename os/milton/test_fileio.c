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
    uint16_t    dir;       /* containing dir cluster: 0 = root, SUB_CLUSTER = SUB */
    uint16_t    start;     /* this entry's own start_cluster (SUB's == SUB_CLUSTER) */
    int         present;   /* 0 once unlinked / before create                 */
} mock_file_t;

/* Tiny NESTED namespace (beads initech-mzxa; ti8 Layer 2 unit oracle). The ROOT
 * directory (dir==0) holds HELLO.TXT, README, DISK (vollabel), the CREAT spare
 * (slot 3), and a SUB DIRECTORY entry (slot 4, attr DIR, start_cluster ==
 * SUB_CLUSTER). The SUB subdirectory (dir==SUB_CLUSTER) holds NESTED.TXT
 * (slot 5). The mock resolve() walks '\SUB\FILE' to SUB_CLUSTER + leaf; the mock
 * open/dir_entry honor dir_start_cluster so OPEN/FINDFIRST in '\SUB' see only
 * SUB's contents. Slots 0..2 remain the stable ROOT enumeration the existing
 * root FINDFIRST tests assert on (SUB is a directory -> skipped by an attr-0
 * '*.*' search, so it does not perturb them). */
#define SUB_CLUSTER  2u
#define DEEP_CLUSTER 3u   /* the DEEP subdirectory inside SUB (beads initech-u6wa) */
static const char *HELLO_DATA  = "Hello from InitechOS test file.\n";
static const char *NESTED_DATA = "Nested file inside SUB.\n";
/* Slots 0..6 are the stable NESTED namespace below (MOCK_N == 7). Slots 7..8 are
 * spares the MKDIR mock (beads initech-u6wa) claims for a freshly-created
 * directory + the new directory's own '.'/'..' cluster placeholder. */
static mock_file_t g_mock[9];

/* A cluster id the MKDIR mock assigns to a newly-created root directory (any
 * value distinct from 0/SUB_CLUSTER/DEEP_CLUSTER; the do_rmdir CWD guard
 * compares the target start_cluster against g_cwd_start_cluster). */
#define NEWDIR_CLUSTER 7u

static void mock_reset_dir(void)
{
    memset(g_mock, 0, sizeof(g_mock));
    g_mock[0].name8 = "HELLO   "; g_mock[0].ext3 = "TXT"; g_mock[0].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[0].data, HELLO_DATA, strlen(HELLO_DATA));
    g_mock[0].size = (uint32_t)strlen(HELLO_DATA); g_mock[0].dir = 0u; g_mock[0].present = 1;

    g_mock[1].name8 = "README  "; g_mock[1].ext3 = "   "; g_mock[1].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[1].data, "read me", 7);
    g_mock[1].size = 7; g_mock[1].dir = 0u; g_mock[1].present = 1;

    g_mock[2].name8 = "DISK    "; g_mock[2].ext3 = "   "; g_mock[2].attr = DIR_ATTR_VOLLABEL;
    g_mock[2].size = 0; g_mock[2].dir = 0u; g_mock[2].present = 1;

    g_mock[3].present = 0;   /* spare slot for CREAT (in the root) */

    /* SUB directory entry in the ROOT (attr DIR, start_cluster == SUB_CLUSTER). */
    g_mock[4].name8 = "SUB     "; g_mock[4].ext3 = "   "; g_mock[4].attr = DIR_ATTR_DIRECTORY;
    g_mock[4].size = 0; g_mock[4].dir = 0u; g_mock[4].start = SUB_CLUSTER; g_mock[4].present = 1;

    /* NESTED.TXT INSIDE the SUB subdirectory (dir == SUB_CLUSTER). */
    g_mock[5].name8 = "NESTED  "; g_mock[5].ext3 = "TXT"; g_mock[5].attr = DIR_ATTR_ARCHIVE;
    memcpy(g_mock[5].data, NESTED_DATA, strlen(NESTED_DATA));
    g_mock[5].size = (uint32_t)strlen(NESTED_DATA); g_mock[5].dir = SUB_CLUSTER; g_mock[5].present = 1;

    /* DEEP directory entry INSIDE SUB (attr DIR, start == DEEP_CLUSTER). It gives
     * CHDIR a two-level target ('\SUB\DEEP') AND a RELATIVE target ('DEEP' from
     * CWD '\SUB') so the resolve_dir oracle exercises the non-root-cwd seed
     * (beads initech-u6wa). */
    g_mock[6].name8 = "DEEP    "; g_mock[6].ext3 = "   "; g_mock[6].attr = DIR_ATTR_DIRECTORY;
    g_mock[6].size = 0; g_mock[6].dir = SUB_CLUSTER; g_mock[6].start = DEEP_CLUSTER; g_mock[6].present = 1;
}
#define MOCK_N 7u
/* The full slot count INCLUDING the MKDIR spares (7..8). The stable root/subdir
 * ENUMERATION (mock_dir_entry) stays bounded by MOCK_N so the FINDFIRST tests are
 * byte-identical; the NAME LOOKUP (mock_find_in_dir) + the MKDIR/RMDIR mocks scan
 * all MOCK_SLOTS so a freshly-created directory is visible to the resolve seam
 * and the rmdir leaf lookup (beads initech-u6wa). */
#define MOCK_SLOTS 9u

static void fill_dir_entry(const mock_file_t *m, dir_entry_t *de)
{
    memset(de, 0, sizeof(*de));
    for (int i = 0; i < 8; i++) de->filename[i]  = (uint8_t)(m->name8 ? m->name8[i] : ' ');
    for (int i = 0; i < 3; i++) de->extension[i] = (uint8_t)(m->ext3 ? m->ext3[i] : ' ');
    de->attribute     = m->attr;
    de->mtime         = 0x1234u;
    de->mdate         = 0x5678u;
    de->file_size     = m->size;
    de->start_cluster = m->start;
}

/* Map a "HELLO.TXT" name to a comparable formatted name and find the slot WITHIN
 * the directory `dir` (0 = root, SUB_CLUSTER = SUB). Directory scoping makes
 * OPEN/UNLINK in '\SUB' see only SUB's files (beads initech-mzxa). */
static int mock_find_in_dir(const char *name83, uint16_t dir)
{
    for (uint32_t i = 0; i < MOCK_SLOTS; i++) {
        if (!g_mock[i].present || g_mock[i].name8 == NULL) continue;
        if (g_mock[i].dir != dir) continue;
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

/* Root-directory name lookup (the read_at mock keys on the dir-entry name copy,
 * which has no directory context; files in the root and SUB never share a name
 * in this fixture, so a root-or-SUB search is unambiguous). */
static int mock_find_by_name(const char *name83)
{
    int idx = mock_find_in_dir(name83, 0u);
    if (idx < 0) idx = mock_find_in_dir(name83, SUB_CLUSTER);
    return idx;
}

/* OPEN: LOCATE only -- return the dir entry + the table index as the slot. No
 * whole-file read; no single-buffer limit (N files open concurrently). */
static uint16_t mock_open(const char *name83, uint16_t dir_start_cluster,
                          dir_entry_t *out_entry, uint32_t *out_slot)
{
    int idx = mock_find_in_dir(name83, dir_start_cluster);
    if (idx < 0) return 0x0002u;                  /* not found in that directory */
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

static uint16_t mock_create(const char *name83, uint16_t dir_start_cluster,
                            dir_entry_t *out_entry, uint32_t *out_slot)
{
    /* This READ-side milestone only creates in the ROOT (subdir write is a
     * follow-up bead; the real backend returns 0x0003 for a non-root dir). */
    if (dir_start_cluster != 0u) return 0x0003u;  /* path not found (no subdir write) */
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

static uint16_t mock_write_at(uint16_t dir_start, uint32_t slot, uint32_t offset,
                              const uint8_t *data, uint32_t len,
                              uint32_t *out_written, dir_entry_t *out_entry)
{
    (void)dir_start;   /* mock files key off the table slot, not the directory */
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

static uint16_t mock_unlink(const char *name83, uint16_t dir_start_cluster)
{
    /* Root (dir==0): "DELETE.ME" exists; anything else not found (the existing
     * unlink oracle). Subdir write is out of READ-side scope -> 0x0003. */
    if (dir_start_cluster != 0u) return 0x0003u;
    if (strcmp(name83, "DELETE.ME") != 0) return 0x0002u;
    g_mock_unlinked = 1;
    int i = 0;
    for (; name83[i] && i < (int)sizeof(g_mock_unlinked_name) - 1; i++)
        g_mock_unlinked_name[i] = name83[i];
    g_mock_unlinked_name[i] = '\0';
    return 0u;
}

static uint16_t mock_dir_entry(uint32_t index, uint16_t dir_start_cluster,
                               dir_entry_t *out_entry, int *out_found)
{
    *out_found = 0;
    if (dir_start_cluster == 0u) {
        /* ROOT: enumerate only the PRESENT seed files (slots 0..2) so FINDFIRST/
         * NEXT see a stable directory regardless of CREAT having touched slot 3
         * (the existing root-enum contract). The SUB directory (slot 4) is a
         * directory entry -- an attr-0 '*.*' search skips it, so the existing
         * root tests are byte-identical. */
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

    /* SUBDIRECTORY (dir_start_cluster != 0): enumerate the files whose `dir`
     * matches (beads initech-mzxa). FINDFIRST in '\SUB' sees only SUB's files. */
    uint32_t seen = 0;
    for (uint32_t i = 0; i < MOCK_N; i++) {
        if (!g_mock[i].present || g_mock[i].dir != dir_start_cluster) continue;
        if (seen == index) {
            fill_dir_entry(&g_mock[i], out_entry);
            *out_found = 1;
            return 0u;
        }
        seen++;
    }
    return 0u;
}

/* RESOLVE (beads initech-mzxa): walk '\SUB\FILE' to (containing-dir cluster,
 * leaf). The mock does NOT strip a leading 'X:' drive (int21.c strips it; the
 * NODRIVE mutant leaves it so an 'A:'-prefixed path reaches here intact and must
 * FAIL to resolve). Supported parents: the root (a bare name or a leading '\')
 * and "\SUB" (-> SUB_CLUSTER). A missing/non-directory non-final component ->
 * 0x0003 (path not found). The leaf points INTO `path`. */
static uint16_t mock_resolve(const char *path, uint16_t cwd_start,
                             const char **out_leaf, uint16_t *out_dir_start)
{
    (void)cwd_start;   /* CWD is always root this milestone */

    /* A leading 'X:' here is the NODRIVE-mutant failure mode (int21.c did NOT
     * strip it): it is not a valid component -> path not found. */
    if (path[0] != '\0' && path[1] == ':') {
        return 0x0003u;
    }

    const char *p = path;
    if (*p == '\\') p++;               /* absolute: skip the leading root '\' */

    /* Find a backslash separating a subdir component from the leaf. */
    const char *sep = NULL;
    for (const char *q = p; *q; q++) {
        if (*q == '\\') { sep = q; break; }
    }

    if (sep == NULL) {
        /* Bare name in the root. */
        *out_leaf      = p;
        *out_dir_start = 0u;
        return 0u;
    }

    /* The first component is a subdirectory; this fixture knows only "SUB". */
    size_t complen = (size_t)(sep - p);
    if (complen == 3u && (p[0]=='S'||p[0]=='s') && (p[1]=='U'||p[1]=='u') &&
        (p[2]=='B'||p[2]=='b')) {
        const char *rest = sep + 1;    /* after "SUB\" */
        /* A further '\' means a deeper component -> the next component must be a
         * directory; SUB holds only the file NESTED.TXT (not traversable), so
         * any "\SUB\X\..." is path-not-found (the non-dir mid-path case). */
        for (const char *q = rest; *q; q++) {
            if (*q == '\\') return 0x0003u;
        }
        *out_leaf      = rest;
        *out_dir_start = SUB_CLUSTER;
        return 0u;
    }

    /* An unknown intermediate directory component -> path not found. */
    return 0x0003u;
}

/* RESOLVE_DIR (beads initech-u6wa; AH=3Bh CHDIR): resolve a FULL `path` to a
 * DIRECTORY's start cluster + its canonical root-relative text. Models the same
 * tiny tree (root -> SUB -> DEEP) the resolve() seam knows, plus '.'/'..' and a
 * RELATIVE descent from `cwd_start` so the do_chdir oracle exercises the
 * non-root CWD seed WITHOUT the real FAT12 backend (the real backend + its m5
 * cwd_start mutant are proven in test_fileio_subdir.c). A leading 'X:' or '\'
 * makes the path absolute (descend from the root); otherwise descend from
 * cwd_start. Returns 0x0003 for a missing dir OR a CHDIR into a FILE (a regular
 * file has no directory child). The m1 DIR-attr gate lives in the REAL backend
 * (fileio_fat.c FILEIO_MUTATE_CHDIR_NOATTR); the mock independently rejects a
 * file by name so the unit oracle's "CD into a FILE -> 0x0003" still bites. */
static uint16_t mock_resolve_dir(const char *path, uint16_t cwd_start,
                                 uint16_t *out_dir_start, char *out_canon,
                                 uint32_t canon_max)
{
    /* The NODRIVE-mutant failure mode: int21.c stripped the drive, so a residual
     * 'X:' here is not a valid component -> path not found. */
    if (path[0] != '\0' && path[1] == ':') {
        return 0x0003u;
    }

    /* Seed: absolute ('\'-prefixed) -> root; else -> the CWD cluster. */
    const char *p = path;
    uint16_t    cur;
    if (*p == '\\') { cur = 0u; p++; }
    else            { cur = cwd_start; }

    /* Walk '\'-delimited components, updating `cur`. */
    while (*p != '\0') {
        char comp[16];
        int  n = 0;
        while (*p != '\0' && *p != '\\') {
            if (n < (int)sizeof(comp) - 1) comp[n++] = *p;
            p++;
        }
        comp[n] = '\0';
        if (*p == '\\') p++;          /* consume the separator */
        if (n == 0) continue;          /* doubled/trailing '\' */

        if (comp[0] == '.' && comp[1] == '\0') {
            /* '.' -> stay */
        } else if (comp[0] == '.' && comp[1] == '.' && comp[2] == '\0') {
            if (cur == DEEP_CLUSTER)      cur = SUB_CLUSTER;
            else if (cur == SUB_CLUSTER)  cur = 0u;
            else                          cur = 0u;   /* '..' at root stays root */
        } else {
            /* A named component: which dir does it select from `cur`? */
            int up = (comp[0]>='a'&&comp[0]<='z') ? comp[0]-32 : comp[0];
            (void)up;
            if (cur == 0u && n == 3 &&
                (comp[0]=='S'||comp[0]=='s') && (comp[1]=='U'||comp[1]=='u') &&
                (comp[2]=='B'||comp[2]=='b')) {
                cur = SUB_CLUSTER;
            } else if (cur == SUB_CLUSTER && n == 4 &&
                       (comp[0]=='D'||comp[0]=='d') && (comp[1]=='E'||comp[1]=='e') &&
                       (comp[2]=='E'||comp[2]=='e') && (comp[3]=='P'||comp[3]=='p')) {
                cur = DEEP_CLUSTER;
            } else if (mock_find_in_dir(comp, cur) >= 0) {
                int fi = mock_find_in_dir(comp, cur);
                if ((g_mock[fi].attr & DIR_ATTR_DIRECTORY) != 0) {
                    /* A real DIRECTORY entry in `cur` (e.g. a MKDIR-created
                     * NEWDIR; beads initech-u6wa): descend into its own cluster
                     * so RMDIR's CWD/root guard can resolve the target. */
                    cur = g_mock[fi].start;
                } else {
                    /* The component IS a real entry in `cur` but NOT a directory (a
                     * regular file like NESTED.TXT) -> the DIR_ATTR_DIRECTORY gate:
                     * CHDIR into a file is path-not-found. The m1 mutant skips this
                     * gate so 'CD \SUB\NESTED.TXT' wrongly SUCCEEDS and the
                     * "CD into a file -> 0x0003" assertion goes RED (Rule 6). */
#ifndef INT21_MUTATE_CHDIR_NOATTR
                    return 0x0003u;
#else
                    /* MUTANT m1: treat the file as a no-op success (stay). */
#endif
                }
            } else {
                /* A missing name -> path not found. */
                return 0x0003u;
            }
        }
    }

    /* Build the canonical root-relative text from the resolved cluster (the same
     * structure-derived canon the real backend emits). */
    if (out_canon != 0 && canon_max > 0u) {
        const char *canon =
            (cur == DEEP_CLUSTER) ? "SUB\\DEEP" :
            (cur == SUB_CLUSTER)  ? "SUB"       : "";
        uint32_t i = 0u;
        for (; canon[i] != '\0' && i + 1u < canon_max; i++) out_canon[i] = canon[i];
        out_canon[i] = '\0';
    }
    *out_dir_start = cur;
    return 0u;
}

/* ---- mock MKDIR / RMDIR backend (beads initech-u6wa; AH=39h/3Ah) ---------- *
 * Root-parent only (dir_start_cluster != 0 -> 0x0003, mirroring the real
 * backend's deferral to initech-zs24). MKDIR claims spare slot 7 for the new
 * DIRECTORY entry in the root (attr DIR, start_cluster == NEWDIR_CLUSTER);
 * RMDIR removes a named EMPTY root directory (an empty dir has no g_mock entry
 * whose dir == its start). The created dir is visible to the resolve seam (for
 * the do_rmdir CWD/root guard) and to FINDFIRST via mock_find_in_dir. */
static char g_mkdir_nm8[9];
static char g_mkdir_ex3[4];

static uint16_t mock_mkdir(const char *name83, uint16_t dir_start_cluster)
{
    if (dir_start_cluster != 0u) return 0x0003u;  /* non-root parent (zs24) */
    /* Duplicate name in the root -> DOS MKDIR-exists (0x0005). */
    if (mock_find_in_dir(name83, 0u) >= 0) return 0x0005u;
    /* Parse name83 into the new slot's 8.3 fields. */
    memset(g_mkdir_nm8, ' ', 8); g_mkdir_nm8[8] = '\0';
    memset(g_mkdir_ex3, ' ', 3); g_mkdir_ex3[3] = '\0';
    int i = 0, j = 0;
    for (; name83[i] && name83[i] != '.' && j < 8; i++, j++) {
        char c = name83[i]; if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        g_mkdir_nm8[j] = c;
    }
    while (name83[i] && name83[i] != '.') i++;
    if (name83[i] == '.') {
        i++; j = 0;
        for (; name83[i] && j < 3; i++, j++) {
            char c = name83[i]; if (c >= 'a' && c <= 'z') c = (char)(c - 32);
            g_mkdir_ex3[j] = c;
        }
    }
    int idx = 7;                       /* the MKDIR spare slot */
    g_mock[idx].name8   = g_mkdir_nm8;
    g_mock[idx].ext3    = g_mkdir_ex3;
    g_mock[idx].attr    = DIR_ATTR_DIRECTORY;
    g_mock[idx].size    = 0;
    g_mock[idx].dir     = 0u;           /* lives in the root */
    g_mock[idx].start   = NEWDIR_CLUSTER;
    g_mock[idx].present = 1;
    return 0u;
}

static uint16_t mock_rmdir(const char *name83, uint16_t dir_start_cluster)
{
    if (dir_start_cluster != 0u) return 0x0003u;  /* non-root parent (zs24) */
    int idx = mock_find_in_dir(name83, 0u);
    if (idx < 0) return 0x0003u;                  /* missing dir -> path not found */
    if ((g_mock[idx].attr & DIR_ATTR_DIRECTORY) == 0) return 0x0003u; /* not a dir */
    /* VERIFY-EMPTY: any present entry whose containing dir == this dir's start
     * cluster makes it non-empty (the real backend enumerates the cluster; the
     * mock checks the directory-scope field). */
    for (uint32_t k = 0; k < MOCK_SLOTS; k++) {
        if (!g_mock[k].present) continue;
        if (g_mock[k].dir == g_mock[idx].start) {
            return 0x0005u;            /* non-empty -> DOS RMDIR-non-empty */
        }
    }
    g_mock[idx].present = 0;            /* removed */
    return 0u;
}

static const int21_file_backend_t g_mock_backend = {
    mock_open, mock_read_at, mock_dir_entry,
    mock_create, mock_write_at, mock_close, mock_unlink,
    NULL,  /* freespace: not exercised by the read/write file oracle (AH=36h is
              covered end-to-end on the emulator, make test-datetime) */
    mock_resolve,     /* the path->containing-directory seam (beads initech-mzxa) */
    mock_resolve_dir, /* the path->DIRECTORY seam for CHDIR (beads initech-u6wa) */
    mock_mkdir,       /* AH=39h MKDIR (beads initech-u6wa) */
    mock_rmdir        /* AH=3Ah RMDIR (beads initech-u6wa) */
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

    /* --- OPEN a path through a MISSING directory -> CF=1, AX=0x0003 (path not
     *     found). NODIR is not a directory in the root, so the non-final
     *     component cannot be traversed (beads initech-mzxa). --------------- */
    {
        uint32_t edx = low_dup("\\NODIR\\HELLO.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "OPEN through a missing directory sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "OPEN through a missing directory AX=0x0003 (path not found)");
    }

    /* --- SUBDIRECTORY resolution (beads initech-mzxa; ti8 Layer 2) -------- *
     * The mock root holds a SUB directory containing NESTED.TXT. Drive these
     * THROUGH the real int21_dispatch (the resolve seam + the dir_start-aware
     * backend).
     *   - OPEN '\SUB\NESTED.TXT'      -> resolves + succeeds (the file is read).
     *   - OPEN '\SUB\MISSING.TXT'     -> 0x0002 (SUB exists; the FILE is absent
     *                                   -- DOS-correct file-not-found, NOT path).
     *   - OPEN '\SUB\NESTED.TXT\X'    -> 0x0003 (a file is not traversable: the
     *                                   non-dir mid-path case).
     *   - OPEN 'A:\SUB\NESTED.TXT'    -> the leading 'A:' is stripped + succeeds.
     */
    {
        /* '\SUB\NESTED.TXT' resolves and opens; read its bytes back. */
        uint32_t edx = low_dup("\\SUB\\NESTED.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx; f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "OPEN '\\SUB\\NESTED.TXT' resolves + clears CF");
        uint32_t h = (uint16_t)(f.eax & 0xFFFFu);

        uint8_t *nb = (uint8_t *)(uintptr_t)alloc_low(64); memset(nb, 0, 64);
        int_frame_t r = fresh_frame();
        r.eax = 0x3F00u; r.ebx = h; r.ecx = (uint32_t)strlen(NESTED_DATA);
        r.edx = (uint32_t)(uintptr_t)nb;
        int21_dispatch(&r);
        CHECK(r.eax == (uint32_t)strlen(NESTED_DATA) &&
              memcmp(nb, NESTED_DATA, strlen(NESTED_DATA)) == 0,
              "READ of '\\SUB\\NESTED.TXT' returns the nested file's bytes");

        int_frame_t cl = fresh_frame();
        cl.eax = 0x3E00u; cl.ebx = h;
        int21_dispatch(&cl);
        CHECK(frame_cf(&cl) == 0, "CLOSE the nested handle");
    }
    {
        /* '\SUB\MISSING.TXT': SUB exists, the file does not -> 0x0002. */
        uint32_t edx = low_dup("\\SUB\\MISSING.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "OPEN '\\SUB\\MISSING.TXT' sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "OPEN '\\SUB\\MISSING.TXT' AX=0x0002 (dir exists, file absent)");
    }
    {
        /* '\SUB\NESTED.TXT\X': a file mid-path is not traversable -> 0x0003. */
        uint32_t edx = low_dup("\\SUB\\NESTED.TXT\\X");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "OPEN '\\SUB\\NESTED.TXT\\X' sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "OPEN '\\SUB\\NESTED.TXT\\X' AX=0x0003 (non-dir mid-path)");
    }
    {
        /* A leading 'A:' is stripped and the rest resolves identically. */
        uint32_t edx = low_dup("A:\\SUB\\NESTED.TXT");
        int_frame_t f = fresh_frame();
        f.eax = 0x3D00u; f.edx = edx; f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 0, "OPEN 'A:\\SUB\\NESTED.TXT' (drive stripped) succeeds");
        if (frame_cf(&f) == 0) {
            int_frame_t cl = fresh_frame();
            cl.eax = 0x3E00u; cl.ebx = (uint16_t)(f.eax & 0xFFFFu);
            int21_dispatch(&cl);
        }
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

    /* --- FINDFIRST in a SUBDIRECTORY (beads initech-mzxa; ti8 Layer 2) ------ *
     * '\SUB\*.*' resolves the directory portion to SUB and the leaf '*.*' to the
     * search template, so the enumeration sees ONLY SUB's files (NESTED.TXT) --
     * NOT the root files. Proves dir_start is threaded into dir_entry(). */
    {
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);

        find_data_t *fd = (find_data_t *)(uintptr_t)alloc_low(FIND_DATA_SIZE);
        CHECK(fd != NULL, "find_data DTA in low 4 GiB (subdir find)");
        memset(fd, 0, FIND_DATA_SIZE);
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = (uint32_t)(uintptr_t)fd;
        int21_dispatch(&s);

        uint32_t edx = low_dup("\\SUB\\*.*");
        int_frame_t ff = fresh_frame();
        ff.eax = 0x4E00u; ff.ecx = 0u; ff.edx = edx;   /* attr 0 = files only */
        ff.eflags |= CF_BIT;
        int21_dispatch(&ff);
        CHECK(frame_cf(&ff) == 0, "FINDFIRST '\\SUB\\*.*' finds a file in SUB");
        CHECK(strcmp(fd->fname, "NESTED.TXT") == 0,
              "FINDFIRST '\\SUB\\*.*' yields NESTED.TXT (SUB's file, not a root file)");

        int_frame_t fn = fresh_frame();
        fn.eax = 0x4F00u;
        int21_dispatch(&fn);
        CHECK(frame_cf(&fn) == 1, "FINDNEXT in SUB past its only file is exhausted");
        CHECK((uint16_t)(fn.eax & 0xFFFFu) == INT21_ERR_NO_MORE_FILES,
              "FINDNEXT in SUB AX=0x0012 (SUB holds exactly one file)");
    }

    /* --- FINDFIRST through a MISSING directory -> 0x0003 (path not found) --- */
    {
        find_data_t *fd = (find_data_t *)(uintptr_t)alloc_low(FIND_DATA_SIZE);
        memset(fd, 0, FIND_DATA_SIZE);
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = (uint32_t)(uintptr_t)fd;
        int21_dispatch(&s);

        uint32_t edx = low_dup("\\NODIR\\*.*");
        int_frame_t ff = fresh_frame();
        ff.eax = 0x4E00u; ff.ecx = 0u; ff.edx = edx;
        int21_dispatch(&ff);
        CHECK(frame_cf(&ff) == 1, "FINDFIRST through a missing directory sets CF");
        CHECK((uint16_t)(ff.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "FINDFIRST '\\NODIR\\*.*' AX=0x0003 (path not found)");
    }

    /* --- AH=47h GET CURRENT DIR reports the ROOT (buf[0]==0) until u6wa's
     *     CHDIR writer lands (beads initech-mzxa; ti8 Layer 2 read-side). ---- */
    {
        char *cwdbuf = (char *)(uintptr_t)alloc_low(64);
        CHECK(cwdbuf != NULL, "GETCWD buffer in low 4 GiB");
        memset(cwdbuf, 'Z', 64);    /* poison so a no-write would be visible */
        int_frame_t g = fresh_frame();
        g.eax = 0x4700u; g.edx = 0u;                   /* DL = 0 (default drive) */
        g.esi = (uint32_t)(uintptr_t)cwdbuf;
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0, "AH=47h GETCWD clears CF for the default drive");
        CHECK(cwdbuf[0] == '\0',
              "AH=47h GETCWD reports the ROOT (empty root-relative path, no leading '\\')");
    }

    /* --- AH=3Bh CHDIR (beads initech-u6wa): change directory through the mock
     *     namespace (root -> SUB -> DEEP). Each leg drives the REAL int21_dispatch
     *     -> do_chdir -> g_file->resolve_dir seam and cross-checks the new CWD via
     *     AH=47h GETCWD. The relative leg (CD DEEP from CWD '\SUB') exercises the
     *     non-root cwd_start seed (the real-backend m5 mutant proves it bites in
     *     test_fileio_subdir.c). CWD is reset to the root at the end so later
     *     tests see a root CWD. ------------------------------------------------ */
    {
        char *cwd = (char *)(uintptr_t)alloc_low(64);
        CHECK(cwd != NULL, "CHDIR: GETCWD buffer in low 4 GiB");

        /* (a) CD '\SUB' (absolute) -> success; GETCWD reports "SUB". */
        {
            uint32_t edx = low_dup("\\SUB");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "CD '\\SUB' clears CF (the directory exists)");

            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(strcmp(cwd, "SUB") == 0,
                  "after CD '\\SUB', GETCWD reports 'SUB' (canon, no leading '\\')");
        }

        /* (b) CD 'DEEP' RELATIVE from CWD '\SUB' -> resolves SUB\DEEP from the
         *     cwd_start seed; GETCWD reports "SUB\DEEP". */
        {
            uint32_t edx = low_dup("DEEP");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0,
                  "relative CD 'DEEP' from CWD '\\SUB' clears CF (resolves from the CWD)");

            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(strcmp(cwd, "SUB\\DEEP") == 0,
                  "after relative CD 'DEEP', GETCWD reports 'SUB\\DEEP'");
        }

        /* (c) CD '..' from SUB\DEEP -> back up to SUB. */
        {
            uint32_t edx = low_dup("..");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "CD '..' from SUB\\DEEP clears CF");

            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(strcmp(cwd, "SUB") == 0, "after CD '..', GETCWD reports 'SUB' again");
        }

        /* (d) CD '..' from SUB -> the root (empty canon). */
        {
            uint32_t edx = low_dup("..");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "CD '..' from SUB clears CF (clamps at root)");

            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(cwd[0] == '\0', "after CD '..' at SUB, GETCWD reports the root (empty)");
        }

        /* (e) CD to a MISSING directory -> CF=1, AX=0x0003 (path not found). */
        {
            uint32_t edx = low_dup("\\NODIR");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 1, "CD to a missing directory sets CF");
            CHECK((uint16_t)(cd.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
                  "CD '\\NODIR' -> AX=0x0003 (path not found)");
        }

        /* (f) CD into a FILE (not a directory) -> CF=1, AX=0x0003. This is the
         *     m1 case (the DIR-attr gate): the real backend's
         *     FILEIO_MUTATE_CHDIR_NOATTR mutant makes this leg go RED. */
        {
            uint32_t edx = low_dup("\\SUB\\NESTED.TXT");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 1, "CD into a FILE sets CF (a file is not a directory)");
            CHECK((uint16_t)(cd.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
                  "CD '\\SUB\\NESTED.TXT' -> AX=0x0003 (CHDIR into a file)");
        }

        /* (g) A FAILED CD must NOT move the CWD: GETCWD still reports the root. */
        {
            int_frame_t g = fresh_frame();
            memset(cwd, 'Z', 64);
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(cwd[0] == '\0', "a failed CD leaves the CWD at the root (no partial move)");
        }
    }

    /* --- AH=39h MKDIR + AH=3Ah RMDIR (beads initech-u6wa): create/remove a
     *     directory through the REAL int21_dispatch -> do_mkdir/do_rmdir ->
     *     the backend mkdir/rmdir seam, with the dispatcher-level RMDIR guards
     *     (root-reject + current-dir). The CWD is at the root from the CHDIR
     *     block above. Reset the mock namespace so the MKDIR spare (slot 7) is
     *     clean. -------------------------------------------------------------- */
    {
        mock_reset_dir();
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);

        /* (a) MKDIR '\NEWDIR' in the root -> success, CF clear. */
        {
            uint32_t edx = low_dup("\\NEWDIR");
            int_frame_t m = fresh_frame();
            m.eax = 0x3900u; m.edx = edx; m.eflags |= CF_BIT;
            int21_dispatch(&m);
            CHECK(frame_cf(&m) == 0, "MKDIR '\\NEWDIR' clears CF (created in the root)");
        }

        /* (b) After MKDIR, the directory is findable in the root namespace. */
        CHECK(mock_find_in_dir("NEWDIR", 0u) >= 0,
              "after MKDIR, NEWDIR is present in the root (round-trip create leg)");

        /* (c) MKDIR an EXISTING name -> CF=1, AX=0x0005 (DOS MKDIR-exists, NOT
         *     0x0003). 'SUB' already exists in the root. */
        {
            uint32_t edx = low_dup("\\SUB");
            int_frame_t m = fresh_frame();
            m.eax = 0x3900u; m.edx = edx;
            int21_dispatch(&m);
            CHECK(frame_cf(&m) == 1, "MKDIR of an existing name sets CF");
            CHECK((uint16_t)(m.eax & 0xFFFFu) == INT21_ERR_ACCESS_DENIED,
                  "MKDIR '\\SUB' (exists) -> AX=0x0005 (access denied, not 0x0003)");
        }

        /* (d) MKDIR with a NON-ROOT parent ('\SUB\NEW') -> the backend's
         *     dir_start!=0 guard returns 0x0003 (nested MD deferred to zs24). */
        {
            uint32_t edx = low_dup("\\SUB\\NEW");
            int_frame_t m = fresh_frame();
            m.eax = 0x3900u; m.edx = edx;
            int21_dispatch(&m);
            CHECK(frame_cf(&m) == 1, "MKDIR in a subdir sets CF (out of scope)");
            CHECK((uint16_t)(m.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
                  "MKDIR '\\SUB\\NEW' (non-root parent) -> AX=0x0003");
        }

        /* (e) RMDIR of the ROOT '\' -> CF=1, AX=0x0010 (cannot remove root). */
        {
            uint32_t edx = low_dup("\\");
            int_frame_t r = fresh_frame();
            r.eax = 0x3A00u; r.edx = edx;
            int21_dispatch(&r);
            CHECK(frame_cf(&r) == 1, "RMDIR '\\' (root) sets CF");
            CHECK((uint16_t)(r.eax & 0xFFFFu) == INT21_ERR_CURRENT_DIR,
                  "RMDIR '\\' (root) -> AX=0x0010 (cannot remove the root)");
        }

        /* (f) RMDIR of a NON-EMPTY directory ('\SUB' holds NESTED.TXT) -> CF=1,
         *     AX=0x0005 (DOS RMDIR-non-empty, NOT 0x0010). */
        {
            uint32_t edx = low_dup("\\SUB");
            int_frame_t r = fresh_frame();
            r.eax = 0x3A00u; r.edx = edx;
            int21_dispatch(&r);
            CHECK(frame_cf(&r) == 1, "RMDIR of a non-empty dir sets CF");
            CHECK((uint16_t)(r.eax & 0xFFFFu) == INT21_ERR_ACCESS_DENIED,
                  "RMDIR '\\SUB' (non-empty) -> AX=0x0005 (access denied, not 0x0010)");
        }

        /* (g) RMDIR of a MISSING directory -> CF=1, AX=0x0003 (path not found). */
        {
            uint32_t edx = low_dup("\\NODIR");
            int_frame_t r = fresh_frame();
            r.eax = 0x3A00u; r.edx = edx;
            int21_dispatch(&r);
            CHECK(frame_cf(&r) == 1, "RMDIR of a missing dir sets CF");
            CHECK((uint16_t)(r.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
                  "RMDIR '\\NODIR' (missing) -> AX=0x0003");
        }

        /* (h) RMDIR of the CURRENT directory -> CF=1, AX=0x0010. CD into NEWDIR
         *     first (it is empty), then RMDIR it while it IS the CWD. */
        {
            uint32_t cdx = low_dup("\\NEWDIR");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = cdx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "CD '\\NEWDIR' clears CF (so it becomes the CWD)");

            uint32_t edx = low_dup("\\NEWDIR");
            int_frame_t r = fresh_frame();
            r.eax = 0x3A00u; r.edx = edx;
            int21_dispatch(&r);
            CHECK(frame_cf(&r) == 1, "RMDIR of the current directory sets CF");
            CHECK((uint16_t)(r.eax & 0xFFFFu) == INT21_ERR_CURRENT_DIR,
                  "RMDIR '\\NEWDIR' while it is the CWD -> AX=0x0010");

            /* CD back to the root so the empty-RMDIR leg is not the CWD. CD '..'
             * from '\NEWDIR' (whose parent is the root) lands on the root; a bare
             * CD '\' is a DOS no-op (stay), so '..' is the explicit way up. */
            uint32_t backx = low_dup("..");
            int_frame_t back = fresh_frame();
            back.eax = 0x3B00u; back.edx = backx; back.eflags |= CF_BIT;
            int21_dispatch(&back);
            CHECK(frame_cf(&back) == 0, "CD '..' back to the root clears CF");
        }

        /* (i) RMDIR of the now-EMPTY '\NEWDIR' from the root -> success, CF clear;
         *     and afterwards it is GONE (round-trip remove leg). */
        {
            uint32_t edx = low_dup("\\NEWDIR");
            int_frame_t r = fresh_frame();
            r.eax = 0x3A00u; r.edx = edx; r.eflags |= CF_BIT;
            int21_dispatch(&r);
            CHECK(frame_cf(&r) == 0, "RMDIR '\\NEWDIR' (empty, not the CWD) clears CF");
            CHECK(mock_find_in_dir("NEWDIR", 0u) < 0,
                  "after RMDIR, NEWDIR is GONE from the root (round-trip remove leg)");
        }

        /* (j) RMDIR with NO write backend -> CF=1, AX=0x0005 (access denied). */
        {
            static const int21_file_backend_t ro2 = {
                mock_open, mock_read_at, mock_dir_entry,
                NULL, NULL, NULL, NULL, NULL,
                mock_resolve, mock_resolve_dir, NULL, NULL };
            mock_reset_dir();
            bind_standard_process();
            int21_set_file_backend(&ro2);
            uint32_t edx = low_dup("\\SUB");
            int_frame_t r = fresh_frame();
            r.eax = 0x3A00u; r.edx = edx;
            int21_dispatch(&r);
            CHECK(frame_cf(&r) == 1, "RMDIR with no rmdir backend sets CF");
            CHECK((uint16_t)(r.eax & 0xFFFFu) == INT21_ERR_ACCESS_DENIED,
                  "RMDIR with no rmdir backend -> AX=0x0005 (access denied)");
            int21_set_file_backend(&g_mock_backend);
        }

        /* Reset the CWD + namespace for the subsequent WRITE-path block. */
        int21_cwd_reset();
        mock_reset_dir();
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

    /* --- AH=5Bh CREATNEW (beads initech-kji0): like CREAT, but FAIL with
     *     AX=0x0050 (ERROR_FILE_EXISTS) if the target already exists; otherwise
     *     behave exactly like CREAT (zero-length file, AX=handle, CF clear).
     *     Drives the REAL int21_dispatch through the mock backend whose open()
     *     is the existence probe. Ref: DOS 3.3 PRM AH=5Bh; ADR-0003 Appendix A. */
    {
        mock_reset_dir();
        bind_standard_process();
        int21_set_file_backend(&g_mock_backend);

        /* (a) CREATNEW a FRESH name -> CF clear, EAX = a real handle. */
        uint32_t nedx = low_dup("BRAND.NEW");
        int_frame_t cn = fresh_frame();
        cn.eax = 0x5B00u; cn.ecx = 0u; cn.edx = nedx;
        cn.eflags |= CF_BIT;
        int21_dispatch(&cn);
        CHECK(frame_cf(&cn) == 0, "CREATNEW fresh name BRAND.NEW clears CF");
        uint32_t nh = (uint16_t)(cn.eax & 0xFFFFu);
        CHECK(nh == 5u, "CREATNEW fresh name returns the lowest free handle (5)");

        /* (b) CREATNEW an EXISTING name (HELLO.TXT lives in the mock root) ->
         *     CF=1, AX=0x0050 (ERROR_FILE_EXISTS). Commit nothing. */
        uint32_t eedx = low_dup("HELLO.TXT");
        int_frame_t ce = fresh_frame();
        ce.eax = 0x5B00u; ce.ecx = 0u; ce.edx = eedx;
        int21_dispatch(&ce);
        CHECK(frame_cf(&ce) == 1, "CREATNEW on existing HELLO.TXT sets CF");
        CHECK((uint16_t)(ce.eax & 0xFFFFu) == INT21_ERR_FILE_EXISTS,
              "CREATNEW on existing file AX=0x0050 (ERROR_FILE_EXISTS)");

        /* (c) AH=59h GETERR right after the (b) failure must report 0x0050 ->
         *     proves the dispatch-wrapper auto-note carried the CREATNEW error. */
        int_frame_t ge = fresh_frame();
        ge.eax = 0x5900u;
        int21_dispatch(&ge);
        CHECK((uint16_t)(ge.eax & 0xFFFFu) == INT21_ERR_FILE_EXISTS,
              "AH=59h GETERR after CREATNEW-exists reports 0x0050 (auto-note carried it)");

        /* (d) shared resolve seam: CREATNEW an EXISTING subdir file
         *     '\SUB\NESTED.TXT' -> CF=1, AX=0x0050 (proves the resolve_dir_path
         *     seam threads the subdir existence probe, beads initech-mzxa/zs24). */
        uint32_t sedx = low_dup("\\SUB\\NESTED.TXT");
        int_frame_t cs = fresh_frame();
        cs.eax = 0x5B00u; cs.ecx = 0u; cs.edx = sedx;
        int21_dispatch(&cs);
        CHECK(frame_cf(&cs) == 1, "CREATNEW on existing subdir file \\SUB\\NESTED.TXT sets CF");
        CHECK((uint16_t)(cs.eax & 0xFFFFu) == INT21_ERR_FILE_EXISTS,
              "CREATNEW on existing subdir file AX=0x0050 (shared resolve seam)");

        /* (e) CREATNEW a FRESH name in '\SUB' that does NOT exist there: the
         *     existence probe MUST clear (open()==0x0002), so do_creatnew falls
         *     THROUGH the guard to the CREATE path. This mock's create() only
         *     writes the ROOT (subdir CREATE is the FAT12 zs24 harness, not this
         *     read-side mock), so it returns 0x0003 -- proving the rejection came
         *     from the backend create(), NOT from the CREATNEW exists-guard
         *     (which would be 0x0050). This pins that the probe did NOT false-hit
         *     a non-existent subdir leaf. */
        uint32_t sfedx = low_dup("\\SUB\\FRESH.TXT");
        int_frame_t csf = fresh_frame();
        csf.eax = 0x5B00u; csf.ecx = 0u; csf.edx = sfedx;
        int21_dispatch(&csf);
        CHECK(frame_cf(&csf) == 1, "CREATNEW fresh subdir file sets CF (mock create() is root-only)");
        CHECK((uint16_t)(csf.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "CREATNEW fresh subdir leaf -> 0x0003 from backend create() (NOT 0x0050; probe passed cleanly)");
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
                                                  NULL, NULL, NULL, NULL, NULL,
                                                  mock_resolve, mock_resolve_dir,
                                                  NULL, NULL /* mkdir/rmdir: read-only */ };
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

        /* UNLINK a subdir path: the path RESOLVES to SUB (dir_start=SUB_CLUSTER,
         * leaf=X.TXT; beads initech-mzxa), but subdir WRITE (delete) is out of
         * the READ-side scope so the backend returns 0x0003. The NOTROOT mutant
         * forces dir_start=0 -> the backend looks in the root -> 0x0002, which
         * makes THIS assertion bite (proving dir_start is threaded into unlink). */
        uint32_t edx3 = low_dup("\\SUB\\X.TXT");
        int_frame_t u3 = fresh_frame();
        u3.eax = 0x4100u; u3.edx = edx3;
        int21_dispatch(&u3);
        CHECK(frame_cf(&u3) == 1 &&
              (uint16_t)(u3.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "UNLINK subdir path AX=0x0003");
    }

    return TEST_SUMMARY("test_fileio");
}
