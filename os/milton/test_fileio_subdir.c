/* os/milton/test_fileio_subdir.c -- INT 21h subdir OPEN/READ over the REAL FAT12
 * backend (beads initech-mzxa; ti8 Layer 2 INTEGRATION oracle).
 *
 * THE STRONGEST mzxa oracle (CLAUDE.md Law 2): unlike test_fileio.c -- which
 * drives the int21 dispatch over a MOCK file backend -- this test binds the
 * REAL kernel FAT12 backend (os/milton/fileio_fat.c) over the minted nested
 * image (build/fat12_nested.img) via the host file-backed blockdev, and drives
 * the WHOLE stack: int21_dispatch (AH=3Dh OPEN / AH=3Fh READ) ->
 * resolve_dir_path (int21.c) -> fat_resolve (fileio_fat.c) -> fat12_resolve_path
 * / fat12_read_dir (fat12.c, Layer 1). It proves the DOS-API->fileio_fat->
 * fat12_resolve_path path end to end: a '\SUB\NESTED.TXT' and a '\SUB\DEEP\
 * DEEP.TXT' OPEN+READ return the EXACT fixture bytes.
 *
 * FACTORY-HOSTED, but the code under test (int21.c + fileio_fat.c + fat12.c) is
 * the SAME artifact source the kernel compiles. The host blockdev + libc test
 * scaffolding are factory-only (Law 3). Image + fixture-dir paths are argv
 * (Makefile) so no host path is baked in (Rule 11). ASCII-clean (Rule 12).
 *
 * Ref (Law 1): brief initech-mzxa oracle ("STRONGLY PREFERRED ... integration
 *   test that drives int21 OPEN/READ of '\SUB\NESTED.TXT' and '\SUB\DEEP\
 *   DEEP.TXT' through the real fat12 resolve and asserts the bytes");
 *   docs/research/fs-mount-sft-ground-truth.md Sec 4; os/milton/fileio_fat.h.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "int21.h"
#include "fileio_fat.h"
#include "fat12.h"
#include "sft.h"
#include "psp.h"
#include "find_data.h"       /* find_data_t (43-byte DTA block) */
#include "blockdev_file.h"   /* host file-backed blockdev (factory) */
#include "test_assert.h"

TEST_HARNESS();

#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;
    return f;
}

/* A capturing CON sink so any stray diagnostic is harmless (never NULL-faults). */
static void sink_capture(char c) { (void)c; }

/* --- low-4-GiB buffers so EDX/ESI (uint32_t flat ptrs) round-trip ---------- */
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

static long read_whole_file(const char *path, uint8_t *buf, long cap)
{
    FILE *fp = fopen(path, "rb");
    long  n;
    if (fp == NULL) {
        return -1;
    }
    n = (long)fread(buf, 1u, (size_t)cap, fp);
    if (!feof(fp) && n == cap) {
        fclose(fp);
        return -1;          /* file larger than cap -- fixture sizing bug */
    }
    fclose(fp);
    return n;
}

/* ---- subdir WRITE-side driver (beads initech-zs24) ------------------------
 * Drive INT 21h CREATE / WRITE / LSEEK+WRITE / UNLINK of a file INSIDE a
 * SUBDIRECTORY through the REAL FAT12 backend over a READ-WRITE image, so the
 * Makefile can diff the on-disk result with mtools (mcopy/mdir) + the python3
 * reference (fat12_ref.py --cat-path). This is the load-bearing zs24 oracle:
 * the file must land in \SUB (not the root), with the EXACT bytes, addressable
 * via the subdir cluster chain. */

/* AH=3Ch CREAT `path`; AH=40h WRITE `data`; AH=3Eh CLOSE. Returns the handle's
 * write count (or -1 on any failure). */
static long creat_write_close(const char *path, const uint8_t *data, uint32_t n)
{
    uint32_t edx = low_dup(path);
    if (edx == 0u) {
        return -1;
    }
    int_frame_t c = fresh_frame();
    c.eax = 0x3C00u; c.ecx = 0u; c.edx = edx; c.eflags |= CF_BIT;
    int21_dispatch(&c);
    if (frame_cf(&c) != 0) {
        return -1;   /* CREAT failed (e.g. a kept root-only guard mutant) */
    }
    uint16_t handle = (uint16_t)(c.eax & 0xFFFFu);

    uint8_t *wb = (uint8_t *)(uintptr_t)alloc_low(n ? n : 1u);
    if (wb == NULL) {
        return -1;
    }
    memcpy(wb, data, n);
    int_frame_t w = fresh_frame();
    w.eax = 0x4000u; w.ebx = handle; w.ecx = n;
    w.edx = (uint32_t)(uintptr_t)wb;
    int21_dispatch(&w);
    long wrote = (frame_cf(&w) == 0) ? (long)(w.eax & 0xFFFFFFFFu) : -1;

    int_frame_t cl = fresh_frame();
    cl.eax = 0x3E00u; cl.ebx = handle;
    int21_dispatch(&cl);
    return wrote;
}

/* AH=3Dh OPEN `path` RDWR; AH=42h LSEEK to `pos`; AH=40h WRITE `data`; CLOSE.
 * Returns the write count (or -1 on failure). The positioned write into an
 * existing subdir file. */
static long open_seek_write_close(const char *path, uint32_t pos,
                                  const uint8_t *data, uint32_t n)
{
    uint32_t edx = low_dup(path);
    if (edx == 0u) {
        return -1;
    }
    int_frame_t o = fresh_frame();
    o.eax = 0x3D02u; o.edx = edx; o.eflags |= CF_BIT;   /* AL=2 RDWR */
    int21_dispatch(&o);
    if (frame_cf(&o) != 0) {
        return -1;
    }
    uint16_t handle = (uint16_t)(o.eax & 0xFFFFu);

    int_frame_t s = fresh_frame();
    s.eax = 0x4200u; s.ebx = handle;              /* AL=0 from start */
    s.ecx = 0u; s.edx = pos;
    int21_dispatch(&s);
    if (frame_cf(&s) != 0) {
        return -1;
    }

    uint8_t *wb = (uint8_t *)(uintptr_t)alloc_low(n ? n : 1u);
    if (wb == NULL) {
        return -1;
    }
    memcpy(wb, data, n);
    int_frame_t w = fresh_frame();
    w.eax = 0x4000u; w.ebx = handle; w.ecx = n;
    w.edx = (uint32_t)(uintptr_t)wb;
    int21_dispatch(&w);
    long wrote = (frame_cf(&w) == 0) ? (long)(w.eax & 0xFFFFFFFFu) : -1;

    int_frame_t cl = fresh_frame();
    cl.eax = 0x3E00u; cl.ebx = handle;
    int21_dispatch(&cl);
    return wrote;
}

/* AH=3Dh OPEN `path` RDWR; AH=57h AL=01h SET the packed mtime/mdate (VERBATIM)
 * via the handle; CLOSE. Drives the REAL int21 -> fileio_fat -> fat12 set-time
 * stack over a read-write image so the Makefile re-reads the on-disk packed
 * fields and diffs them (beads initech-qekc). Returns 0 on success, -1 on any CF
 * (OPEN / SET / the SET intentionally returns CF on a read-only backend). */
static int open_settime_close(const char *path, uint16_t mtime, uint16_t mdate)
{
    uint32_t edx = low_dup(path);
    if (edx == 0u) {
        return -1;
    }
    int_frame_t o = fresh_frame();
    o.eax = 0x3D02u; o.edx = edx; o.eflags |= CF_BIT;   /* AL=2 RDWR */
    int21_dispatch(&o);
    if (frame_cf(&o) != 0) {
        return -1;
    }
    uint16_t handle = (uint16_t)(o.eax & 0xFFFFu);

    int_frame_t s = fresh_frame();
    s.eax = 0x5701u; s.ebx = handle; s.ecx = mtime; s.edx = mdate;
    s.eflags |= CF_BIT;
    int21_dispatch(&s);
    int set_ok = (frame_cf(&s) == 0) ? 0 : -1;

    int_frame_t cl = fresh_frame();
    cl.eax = 0x3E00u; cl.ebx = handle;
    int21_dispatch(&cl);
    return set_ok;
}

/* AH=43h CHMOD `path`: GET (set==0) returns the attribute byte via *out_attr;
 * SET (set==1) writes `attr` as the new attribute byte. Returns 0 on success
 * (CF clear), -1 on CF. Drives the full int21 -> fileio_fat -> fat12 stack. */
static int chmod_path(const char *path, int set, uint8_t attr, uint8_t *out_attr)
{
    uint32_t edx = low_dup(path);
    if (edx == 0u) {
        return -1;
    }
    int_frame_t c = fresh_frame();
    c.eax = set ? 0x4301u : 0x4300u;
    c.edx = edx;
    c.ecx = set ? (uint32_t)attr : 0u;
    c.eflags |= CF_BIT;
    int21_dispatch(&c);
    if (frame_cf(&c) != 0) {
        return -1;
    }
    if (!set && out_attr) {
        *out_attr = (uint8_t)(c.ecx & 0xFFu);
    }
    return 0;
}

/* AH=41h UNLINK `path`. Returns 0 on success, -1 on CF. */
static int unlink_path(const char *path)
{
    uint32_t edx = low_dup(path);
    if (edx == 0u) {
        return -1;
    }
    int_frame_t u = fresh_frame();
    u.eax = 0x4100u; u.edx = edx; u.eflags |= CF_BIT;
    int21_dispatch(&u);
    return (frame_cf(&u) == 0) ? 0 : -1;
}

/* Bind a standard process so the handle functions have a valid JFT/SFT. */
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

/* OPEN `path` through the dispatcher, READ its whole content, and assert it
 * matches the committed fixture byte-for-byte. Drives the real resolve stack. */
static void check_open_read(const char *path, const char *golden_path)
{
    uint8_t golden[8192];
    long    glen = read_whole_file(golden_path, golden, (long)sizeof(golden));
    char    msg[200];

    snprintf(msg, sizeof(msg), "read golden fixture %s", golden_path);
    CHECK(glen >= 0, msg);
    if (glen < 0) {
        return;
    }

    /* AH=3Dh OPEN (mode 0 = read). */
    uint32_t edx = low_dup(path);
    CHECK(edx != 0u, "OPEN path buffer in low 4 GiB");
    int_frame_t o = fresh_frame();
    o.eax = 0x3D00u; o.edx = edx; o.eflags |= CF_BIT;
    int21_dispatch(&o);
    snprintf(msg, sizeof(msg), "OPEN '%s' through the real FAT12 backend succeeds", path);
    CHECK(frame_cf(&o) == 0, msg);
    if (frame_cf(&o) != 0) {
        return;
    }
    uint16_t handle = (uint16_t)(o.eax & 0xFFFFu);

    /* AH=3Fh READ glen+1 bytes (one past EOF) into a poisoned buffer; expect
     * exactly glen bytes (a clean EOF, no padding). */
    uint8_t *rb = (uint8_t *)(uintptr_t)alloc_low((size_t)glen + 16);
    CHECK(rb != NULL, "READ buffer in low 4 GiB");
    memset(rb, 0x7E, (size_t)glen + 16);
    int_frame_t r = fresh_frame();
    r.eax = 0x3F00u; r.ebx = handle; r.ecx = (uint32_t)glen + 1u;
    r.edx = (uint32_t)(uintptr_t)rb;
    int21_dispatch(&r);
    snprintf(msg, sizeof(msg), "READ '%s' returns exactly the file size (%ld bytes)", path, glen);
    CHECK(r.eax == (uint32_t)glen, msg);
    snprintf(msg, sizeof(msg), "READ '%s' bytes match the fixture byte-for-byte", path);
    CHECK(memcmp(rb, golden, (size_t)glen) == 0, msg);

    /* AH=3Eh CLOSE so the next OPEN reuses the slot. */
    int_frame_t c = fresh_frame();
    c.eax = 0x3E00u; c.ebx = handle;
    int21_dispatch(&c);
    snprintf(msg, sizeof(msg), "CLOSE '%s' handle", path);
    CHECK(frame_cf(&c) == 0, msg);
}

int main(int argc, char **argv)
{
    const char     *img;
    const char     *fixdir;
    blockdev_file_t bf;
    static fat12_volume_t vol;   /* static: outlives fileio_fat_bind (Rule 2) */
    uint8_t         sector_buf[512];
    char            nested_path[512];
    char            deep_path[512];
    char            hello_path[512];
    int             rc;

    if (argc < 3) {
        fprintf(stderr,
                "usage: %s <fat12-nested-image> <fixture-dir>\n"
                "       %s <rw-image> <fixture-dir> --write <op>\n",
                argv[0], argv[0]);
        return 2;
    }
    img    = argv[1];
    fixdir = argv[2];
    snprintf(nested_path, sizeof(nested_path), "%s/nested.txt", fixdir);
    snprintf(deep_path,   sizeof(deep_path),   "%s/deep.txt",   fixdir);
    snprintf(hello_path,  sizeof(hello_path),  "%s/hello.txt",  fixdir);

    /* ---- WRITE mode (beads initech-zs24): drive a subdir CREATE/WRITE/UNLINK
     * over a READ-WRITE image, then exit so the Makefile diffs the on-disk
     * result with mtools + python3. The op selects which side-effect to apply;
     * the harness here only PERFORMS the INT 21h calls + sanity-checks the
     * register returns (the byte-exact ground truth is the external diff). */
    if (argc >= 5 && strcmp(argv[3], "--write") == 0) {
        const char *op = argv[4];
        static blockdev_file_t   wbf;
        static fat12_volume_t    wvol;
        uint8_t                  wsec[512];

        int21_set_sink(sink_capture);
        bind_standard_process();

        rc = blockdev_file_open_rw(&wbf, img);
        CHECK(rc == 0, "blockdev_file_open_rw should succeed on the rw image");
        if (rc != 0) {
            return TEST_SUMMARY("test_fileio_subdir");
        }
        rc = fat12_mount(&wvol, &wbf.dev, wsec);
        CHECK(rc == FAT12_OK, "fat12_mount (rw) should succeed");
        if (rc != FAT12_OK) {
            blockdev_file_close(&wbf);
            return TEST_SUMMARY("test_fileio_subdir");
        }
        rc = fileio_fat_bind(&wvol);
        CHECK(rc == 0, "fileio_fat_bind (rw) should bind");
        if (rc != 0) {
            blockdev_file_close(&wbf);
            return TEST_SUMMARY("test_fileio_subdir");
        }

        if (strcmp(op, "create-write") == 0) {
            /* CREATE+WRITE '\SUB\NEW.TXT' with a content that spans into a 2nd
             * cluster (> 512 bytes) so the subdir-file CHAIN is exercised, not
             * just a single cluster. */
            static uint8_t payload[700];
            for (uint32_t i = 0u; i < sizeof(payload); i++) {
                payload[i] = (uint8_t)('A' + (i % 26u));
            }
            long w = creat_write_close("\\SUB\\NEW.TXT", payload,
                                       (uint32_t)sizeof(payload));
            CHECK(w == (long)sizeof(payload),
                  "zs24: CREATE+WRITE '\\SUB\\NEW.TXT' writes all bytes");
        } else if (strcmp(op, "seek-write") == 0) {
            /* POSITIONED write: OPEN the just-created '\SUB\NEW.TXT', LSEEK to
             * byte 600, overwrite 50 bytes (spanning the cluster the seek lands
             * in), CLOSE. The Makefile diffs the whole file content after. */
            static uint8_t patch[50];
            for (uint32_t i = 0u; i < sizeof(patch); i++) {
                patch[i] = (uint8_t)('0' + (i % 10u));
            }
            long w = open_seek_write_close("\\SUB\\NEW.TXT", 600u, patch,
                                           (uint32_t)sizeof(patch));
            CHECK(w == (long)sizeof(patch),
                  "zs24: LSEEK+WRITE into '\\SUB\\NEW.TXT' writes all bytes");
        } else if (strcmp(op, "grow") == 0) {
            /* DIRECTORY GROW (beads initech-zs24; Fix 1): \SUB starts with
             * '.'/'..'/NESTED.TXT/DEEP = 4 entries (slots 0..3). On the floppy
             * geometry (spc==1 => 16 entries/cluster) slots 4..15 fill the FIRST
             * cluster, so creating GROW00..GROW11 (12 files) lands in slots 4..15
             * and GROW12 (the 13th) crosses into slot 16 -- the FIRST slot of a
             * SECOND cluster, which does not exist yet, so fat12_create must call
             * fat12_grow_dir to APPEND a cluster. Each file gets a DISTINCT,
             * deterministic payload so the Makefile can read GROW12 back byte-exact
             * from the GROWN cluster (proving the FAT relink + slot mapping +
             * zero-fill). 13 files, GROW12 is the boundary-crossing file. */
            int i;
            int ok = 1;
            char path[32];
            uint8_t payload[40];
            for (i = 0; i <= 12; i++) {
                uint32_t j;
                snprintf(path, sizeof(path), "\\SUB\\GROW%02d.TXT", i);
                /* Distinct content: every byte is ('A' + i) so file i is a
                 * constant run unique to its index; GROW12 reads back as 40 'M's
                 * ('A'+12). */
                for (j = 0u; j < sizeof(payload); j++) {
                    payload[j] = (uint8_t)('A' + i);
                }
                long w = creat_write_close(path, payload, (uint32_t)sizeof(payload));
                if (w != (long)sizeof(payload)) {
                    ok = 0;
                }
            }
            CHECK(ok == 1,
                  "zs24 GROW: CREATE+WRITE GROW00..GROW12 in '\\SUB' all write "
                  "their bytes (GROW12 forces a directory grow into a 2nd cluster)");
        } else if (strcmp(op, "unlink") == 0) {
            int u = unlink_path("\\SUB\\NEW.TXT");
            CHECK(u == 0, "zs24: UNLINK '\\SUB\\NEW.TXT' clears CF");
        } else if (strcmp(op, "filetime-set") == 0) {
            /* AH=57h SET FILE DATE/TIME (beads initech-qekc): CREATE a fresh
             * subdir file '\SUB\TSTAMP.TXT' (which lands with the deterministic
             * FAT12_FIXED_MTIME==0 baseline, NOT the host-clock stamp mtools bakes
             * into mcopy'd fixtures), then SET its packed mtime/mdate to a
             * distinctive value and FLUSH. The Makefile re-reads the on-disk
             * 0x16/0x18 words (python --stat-path-time + mtools mdir) and asserts
             * they EXACTLY equal what SET wrote -- the INVERSE of every other FAT
             * oracle (which normalizes timestamps away). The fresh-create baseline
             * makes the BEFORE state deterministic (0/0) and proves the SET (a
             * skipped flush would leave 0/0, biting the diff). The constants are
             * plausible packed DOS values:
             *   mtime 0x4A6B = 09:19:22  (hh=9, mm=19, ss/2=11)
             *   mdate 0x2C8D = 2002-04-13 (yr-1980=22, mon=4, day=13). */
            static uint8_t tsp[64];
            for (uint32_t i = 0u; i < sizeof(tsp); i++) {
                tsp[i] = (uint8_t)('a' + (i % 26u));
            }
            long w = creat_write_close("\\SUB\\TSTAMP.TXT", tsp,
                                       (uint32_t)sizeof(tsp));
            CHECK(w == (long)sizeof(tsp),
                  "qekc: CREATE+WRITE '\\SUB\\TSTAMP.TXT' (fresh 0/0 baseline)");
            int rc57 = open_settime_close("\\SUB\\TSTAMP.TXT", 0x4A6Bu, 0x2C8Du);
            CHECK(rc57 == 0,
                  "qekc: AH=57h SET on '\\SUB\\TSTAMP.TXT' clears CF");
        } else if (strcmp(op, "filetime-persist") == 0) {
            /* PERSISTENCE leg (beads initech-qekc): after a FILETIME-set, a later
             * positioned WRITE must NOT clobber the on-disk mtime/mdate
             * (fat12_write_partial patches only size/start_cluster). OPEN the
             * already-stamped '\SUB\TSTAMP.TXT' RDWR, LSEEK to 0, overwrite the
             * first 16 bytes, CLOSE. The Makefile re-reads 0x16/0x18 after and
             * asserts the stamp survived. */
            static uint8_t patch[16];
            for (uint32_t i = 0u; i < sizeof(patch); i++) {
                patch[i] = (uint8_t)('Z');
            }
            long w = open_seek_write_close("\\SUB\\TSTAMP.TXT", 0u, patch,
                                           (uint32_t)sizeof(patch));
            CHECK(w == (long)sizeof(patch),
                  "qekc: a later WRITE into the stamped '\\SUB\\TSTAMP.TXT' writes all bytes");
        } else if (strcmp(op, "attr-set") == 0) {
            /* AH=43h CHMOD (beads initech-b53d): CREATE a fresh subdir file
             * '\SUB\ATTR.TXT' (which lands with the deterministic DIR_ATTR_ARCHIVE
             * 0x20 baseline), then AH=43h AL=01 SET its attribute to RO|HIDDEN
             * (0x03 -- note: deliberately NO Archive bit, so the on-disk byte is
             * an exact 0x03 the python/mtools references can pin) and FLUSH. Prove
             * the patch THREE ways: (1) AH=43h GET back through the FULL dispatch
             * stack returns 0x03; (2) fat12_get_attr DIRECTLY off the cached FAT
             * returns 0x03 (the on-disk byte, independent of the SFT); the Makefile
             * then adds (3) python --attr + (4) mtools mattrib. A re-typing reject
             * is the host oracle's job (test_fileio.c); here we prove persistence.
             *   attr 0x03 = DIR_ATTR_READONLY | DIR_ATTR_HIDDEN. */
            static uint8_t ap[48];
            for (uint32_t i = 0u; i < sizeof(ap); i++) {
                ap[i] = (uint8_t)('A' + (i % 26u));
            }
            long w = creat_write_close("\\SUB\\ATTR.TXT", ap,
                                       (uint32_t)sizeof(ap));
            CHECK(w == (long)sizeof(ap),
                  "b53d: CREATE+WRITE '\\SUB\\ATTR.TXT' (fresh 0x20 baseline)");
            uint8_t new_attr = (uint8_t)(DIR_ATTR_READONLY | DIR_ATTR_HIDDEN);
            int rcset = chmod_path("\\SUB\\ATTR.TXT", 1, new_attr, NULL);
            CHECK(rcset == 0,
                  "b53d: AH=43h SET RO|HIDDEN on '\\SUB\\ATTR.TXT' clears CF");
            /* (1) AH=43h GET through the dispatcher reads it back. */
            uint8_t got = 0xFFu;
            int rcget = chmod_path("\\SUB\\ATTR.TXT", 0, 0u, &got);
            CHECK(rcget == 0 && got == new_attr,
                  "b53d: AH=43h GET '\\SUB\\ATTR.TXT' -> 0x03 (RO|HIDDEN, dispatch)");
            /* (2) fat12_get_attr DIRECTLY off the cached FAT (the on-disk byte,
             * independent of the SFT in-memory copy). Resolve '\SUB\ATTR.TXT'
             * with fat12_resolve_path (an INDEPENDENT decode -- it does NOT call
             * fat12_get_attr): out_dir.start_cluster is SUB's first cluster, and
             * out_entry.attribute is the raw on-disk byte. Then re-read the SAME
             * byte through fat12_get_attr to prove the primitive agrees. */
            {
                uint32_t    flen   = 0u;
                void       *fatbuf = fileio_fat_fat_buffer(&flen);
                fat12_dir_t containing;
                dir_entry_t fent;
                int rr = fat12_resolve_path(&wvol, wsec, fatbuf, flen,
                                            "\\SUB\\ATTR.TXT", &containing, &fent);
                CHECK(rr == FAT12_OK,
                      "b53d: fat12_resolve_path '\\SUB\\ATTR.TXT' for read-back");
                CHECK(rr == FAT12_OK && fent.attribute == new_attr,
                      "b53d: resolved on-disk dir entry attribute == 0x03");
                uint16_t sub_start = containing.is_root ? 0u
                                                        : containing.start_cluster;
                uint8_t disk_attr = 0xFFu;
                int gr = fat12_get_attr(&wvol, fatbuf, flen, "ATTR.TXT",
                                        sub_start, wsec, &disk_attr);
                CHECK(gr == FAT12_OK && disk_attr == new_attr,
                      "b53d: fat12_get_attr off cached FAT -> 0x03 (on-disk byte)");
            }
            /* DIRECTORY-TARGET REJECT (the DOS-faithful SET reject set, fail loud):
             * AH=43h SET on '\SUB\DEEP' (a directory) MUST be access-denied. CX is
             * a plain file attr (RO) so it clears the dispatch CX-reject; the dir
             * TARGET reject is the fat12 primitive's guard. A mutant that drops the
             * fat12 reject makes THIS leg go RED (the SET would succeed/CF=0). */
            {
                uint8_t dummy = 0u;
                int rcd = chmod_path("\\SUB\\DEEP", 1,
                                     (uint8_t)DIR_ATTR_READONLY, &dummy);
                CHECK(rcd == -1,
                      "b53d: AH=43h SET on the DIRECTORY '\\SUB\\DEEP' is rejected (CF=1)");
            }
            /* DIRECTORY GET SUCCEEDS with the dir attribute (0x10) -- GET is a pure
             * read (RBIL AX=4300h has NO directory exclusion; the canonical "does
             * this directory exist / stat a path" idiom). Prove the corrected
             * behaviour TWO in-process ways over the REAL stack (the Makefile adds
             * the python --attr + mtools mattrib legs for the 3-way differential):
             *   (1) AH=43h GET '\SUB\DEEP' through the FULL dispatch stack -> CF=0,
             *       CX=0x10;  (2) fat12_get_attr DIRECTLY off the cached FAT for the
             * DEEP entry inside SUB -> 0x10. A mutant that RE-INTRODUCES the GET
             * reject (-DFAT12_MUTATE_GETATTR_DIR_REJECT) makes BOTH legs go RED. */
            {
                uint8_t dattr = 0xFFu;
                int rcgd = chmod_path("\\SUB\\DEEP", 0, 0u, &dattr);
                CHECK(rcgd == 0 && dattr == (uint8_t)DIR_ATTR_DIRECTORY,
                      "b53d: AH=43h GET '\\SUB\\DEEP' -> CF=0, CX=0x10 (dir attr, RBIL 4300h)");
                uint32_t    flen2   = 0u;
                void       *fatbuf2 = fileio_fat_fat_buffer(&flen2);
                fat12_dir_t cont2;
                dir_entry_t dent;
                int rr2 = fat12_resolve_path(&wvol, wsec, fatbuf2, flen2,
                                             "\\SUB\\DEEP", &cont2, &dent);
                CHECK(rr2 == FAT12_OK,
                      "b53d: fat12_resolve_path '\\SUB\\DEEP' (containing dir = SUB)");
                uint16_t sub_start2 = cont2.is_root ? 0u : cont2.start_cluster;
                uint8_t disk_dattr = 0xFFu;
                int gd = fat12_get_attr(&wvol, fatbuf2, flen2, "DEEP",
                                        sub_start2, wsec, &disk_dattr);
                CHECK(gd == FAT12_OK && disk_dattr == (uint8_t)DIR_ATTR_DIRECTORY,
                      "b53d: fat12_get_attr off cached FAT for DEEP -> 0x10 (on-disk dir byte)");
            }
            /* MISSING-FILE error code (through the REAL fileio_fat backend): a GET
             * of '\SUB\NOPE.TXT' MUST report CF=1, AX=0x0002 (file not found), NOT
             * 0x0005. A mutant that maps fat12 NOT_FOUND to ACCESS_DENIED makes
             * THIS leg go RED (the AX is 0x0005, not 0x0002). */
            {
                int_frame_t mg = fresh_frame();
                mg.eax = 0x4300u; mg.edx = low_dup("\\SUB\\NOPE.TXT");
                mg.eflags |= CF_BIT;
                int21_dispatch(&mg);
                CHECK(frame_cf(&mg) == 1 &&
                      (uint16_t)(mg.eax & 0xFFFFu) == 0x0002u,
                      "b53d: AH=43h GET missing '\\SUB\\NOPE.TXT' -> CF=1, AX=0x0002");
            }
        } else if (strcmp(op, "root-regress") == 0) {
            /* A ROOT CREATE+WRITE+UNLINK still works (dir_start==0 path stays
             * functional under the generalized primitives). */
            static uint8_t rp[300];
            for (uint32_t i = 0u; i < sizeof(rp); i++) {
                rp[i] = (uint8_t)('a' + (i % 26u));
            }
            long w = creat_write_close("\\ROOTNEW.TXT", rp, (uint32_t)sizeof(rp));
            CHECK(w == (long)sizeof(rp),
                  "zs24: ROOT CREATE+WRITE '\\ROOTNEW.TXT' still works");
        } else {
            fprintf(stderr, "test_fileio_subdir --write: unknown op '%s'\n", op);
            blockdev_file_close(&wbf);
            return 2;
        }

        blockdev_file_close(&wbf);
        return TEST_SUMMARY("test_fileio_subdir");
    }

    int21_set_sink(sink_capture);
    bind_standard_process();

    rc = blockdev_file_open(&bf, img);
    CHECK(rc == 0, "blockdev_file_open should succeed on the nested image");
    if (rc != 0) {
        return TEST_SUMMARY("test_fileio_subdir");
    }

    rc = fat12_mount(&vol, &bf.dev, sector_buf);
    CHECK(rc == FAT12_OK, "fat12_mount should succeed on the nested image");
    if (rc != FAT12_OK) {
        blockdev_file_close(&bf);
        return TEST_SUMMARY("test_fileio_subdir");
    }

    /* Bind the REAL kernel FAT12 backend (caches the FAT, installs the vtable). */
    rc = fileio_fat_bind(&vol);
    CHECK(rc == 0, "fileio_fat_bind should bind the FAT12 backend");
    if (rc != 0) {
        blockdev_file_close(&bf);
        return TEST_SUMMARY("test_fileio_subdir");
    }

    /* (1) A ROOT file still opens + reads correctly (root==start_cluster 0 path
     * is byte-identical to the pre-mzxa behavior). */
    check_open_read("\\HELLO.TXT", hello_path);
    check_open_read("HELLO.TXT", hello_path);     /* a bare name == the root */

    /* (2) A SUBDIRECTORY file: '\SUB\NESTED.TXT' -- the load-bearing assertion.
     * Resolves SUB -> its cluster, opens NESTED.TXT inside it, reads the bytes. */
    check_open_read("\\SUB\\NESTED.TXT", nested_path);

    /* (3) A DEEPER file: '\SUB\DEEP\DEEP.TXT' -- two levels of resolution. */
    check_open_read("\\SUB\\DEEP\\DEEP.TXT", deep_path);

    /* (4) A leading 'A:' drive prefix is stripped and resolves identically.
     * (The NODRIVE mutant is RED-proven by the MOCK unit oracle test_fileio.c,
     * where the mock resolve does NOT defensively re-strip; the real fat_resolve
     * here strips the drive a second time as belt-and-suspenders robustness, so
     * this case is a positive integration check, not the NODRIVE mutant gate.) */
    check_open_read("A:\\SUB\\NESTED.TXT", nested_path);

    /* (5) Error legs through the real backend:
     *   - '\SUB\MISSING.TXT' -> SUB exists, the file is absent -> 0x0002.
     *   - '\NODIR\X.TXT'     -> NODIR is not a directory -> 0x0003 (path).
     *   - '\SUB\NESTED.TXT\X'-> a file mid-path is not traversable -> 0x0003. */
    {
        uint32_t edx = low_dup("\\SUB\\MISSING.TXT");
        int_frame_t o = fresh_frame();
        o.eax = 0x3D00u; o.edx = edx;
        int21_dispatch(&o);
        CHECK(frame_cf(&o) == 1 &&
              (uint16_t)(o.eax & 0xFFFFu) == 0x0002u,
              "OPEN '\\SUB\\MISSING.TXT' -> AX=0x0002 (dir exists, file absent)");
    }
    {
        uint32_t edx = low_dup("\\NODIR\\X.TXT");
        int_frame_t o = fresh_frame();
        o.eax = 0x3D00u; o.edx = edx;
        int21_dispatch(&o);
        CHECK(frame_cf(&o) == 1 &&
              (uint16_t)(o.eax & 0xFFFFu) == 0x0003u,
              "OPEN '\\NODIR\\X.TXT' -> AX=0x0003 (missing directory component)");
    }
    {
        uint32_t edx = low_dup("\\SUB\\NESTED.TXT\\X");
        int_frame_t o = fresh_frame();
        o.eax = 0x3D00u; o.edx = edx;
        int21_dispatch(&o);
        CHECK(frame_cf(&o) == 1 &&
              (uint16_t)(o.eax & 0xFFFFu) == 0x0003u,
              "OPEN '\\SUB\\NESTED.TXT\\X' -> AX=0x0003 (non-dir mid-path)");
    }

    /* (6) FINDFIRST in '\SUB' enumerates the subdir (real dir_entry walk). */
    {
        find_data_t *fd = (find_data_t *)(uintptr_t)alloc_low(sizeof(find_data_t));
        CHECK(fd != NULL, "find_data DTA in low 4 GiB");
        memset(fd, 0, sizeof(find_data_t));
        int_frame_t s = fresh_frame();
        s.eax = 0x1A00u; s.edx = (uint32_t)(uintptr_t)fd;
        int21_dispatch(&s);

        uint32_t edx = low_dup("\\SUB\\*.*");
        int_frame_t ff = fresh_frame();
        ff.eax = 0x4E00u; ff.ecx = 0u; ff.edx = edx; ff.eflags |= CF_BIT;
        int21_dispatch(&ff);
        CHECK(frame_cf(&ff) == 0, "FINDFIRST '\\SUB\\*.*' finds a file in SUB");
        /* SUB contains NESTED.TXT (+ '.'/'..' dirs which an attr-0 search skips). */
        CHECK(strcmp(fd->fname, "NESTED.TXT") == 0,
              "FINDFIRST '\\SUB\\*.*' yields NESTED.TXT (the subdir's regular file)");
    }

    /* (7) AH=47h GET CURRENT DIR reports the root (no CHDIR writer yet). */
    {
        char *cwdbuf = (char *)(uintptr_t)alloc_low(64);
        memset(cwdbuf, 'Z', 64);
        int_frame_t g = fresh_frame();
        g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwdbuf;
        int21_dispatch(&g);
        CHECK(frame_cf(&g) == 0 && cwdbuf[0] == '\0',
              "AH=47h GETCWD reports the ROOT (empty path) over the real backend");
    }

    /* (8) AH=3Bh CHDIR over the REAL FAT12 backend (beads initech-u6wa). The
     * canon text is derived from the filesystem structure (a reverse '..' walk),
     * so a RELATIVE path canonicalizes identically to its absolute form -- the
     * relative leg ('CD DEEP' from CWD '\SUB') is the load-bearing assertion for
     * the cwd_start descend-seed fix (the m5 mutant reverts it). */
    {
        char *cwd = (char *)(uintptr_t)alloc_low(64);
        CHECK(cwd != NULL, "CHDIR: GETCWD buffer in low 4 GiB");

        int21_cwd_reset();   /* start at the root */

        /* (8a) CD '\SUB' (absolute) -> GETCWD reports "SUB". */
        {
            uint32_t edx = low_dup("\\SUB");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "real CD '\\SUB' clears CF");
            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(strcmp(cwd, "SUB") == 0,
                  "real backend: after CD '\\SUB', GETCWD reports 'SUB'");
        }

        /* (8b) RELATIVE CD 'DEEP' from CWD '\SUB' -> resolves SUB\DEEP from the
         *      cwd_start seed. THIS is the m5 cwd_start oracle: with the seed
         *      reverted to root, 'DEEP' is looked up in the root and fails. */
        {
            uint32_t edx = low_dup("DEEP");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0,
                  "real backend: relative CD 'DEEP' from '\\SUB' clears CF (cwd_start seed)");
            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(strcmp(cwd, "SUB\\DEEP") == 0,
                  "real backend: after relative CD 'DEEP', GETCWD reports 'SUB\\DEEP'");
        }

        /* (8c) CD '..' from SUB\DEEP -> SUB (reverse '..' canon). */
        {
            uint32_t edx = low_dup("..");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "real backend: CD '..' from SUB\\DEEP clears CF");
            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(strcmp(cwd, "SUB") == 0,
                  "real backend: after CD '..', GETCWD reports 'SUB'");
        }

        /* (8d) CD '..' from SUB -> the root (empty canon). */
        {
            uint32_t edx = low_dup("..");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx; cd.eflags |= CF_BIT;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 0, "real backend: CD '..' from SUB clears CF");
            memset(cwd, 'Z', 64);
            int_frame_t g = fresh_frame();
            g.eax = 0x4700u; g.edx = 0u; g.esi = (uint32_t)(uintptr_t)cwd;
            int21_dispatch(&g);
            CHECK(cwd[0] == '\0',
                  "real backend: after CD '..' at SUB, GETCWD reports the root (empty)");
        }

        /* (8e) CD into a FILE (not a directory) -> 0x0003. THIS is the m1 oracle:
         *      the FILEIO_MUTATE_CHDIR_NOATTR mutant skips the DIR_ATTR gate so
         *      CHDIR into a file wrongly succeeds and this leg goes RED. */
        {
            int21_cwd_reset();
            uint32_t edx = low_dup("\\SUB\\NESTED.TXT");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 1 &&
                  (uint16_t)(cd.eax & 0xFFFFu) == 0x0003u,
                  "real backend: CD '\\SUB\\NESTED.TXT' (a file) -> AX=0x0003");
        }

        /* (8f) CD to a missing directory -> 0x0003. */
        {
            uint32_t edx = low_dup("\\NODIR");
            int_frame_t cd = fresh_frame();
            cd.eax = 0x3B00u; cd.edx = edx;
            int21_dispatch(&cd);
            CHECK(frame_cf(&cd) == 1 &&
                  (uint16_t)(cd.eax & 0xFFFFu) == 0x0003u,
                  "real backend: CD '\\NODIR' -> AX=0x0003 (missing directory)");
        }

        int21_cwd_reset();   /* leave the CWD at the root for any later code */
    }

    blockdev_file_close(&bf);
    return TEST_SUMMARY("test_fileio_subdir");
}
