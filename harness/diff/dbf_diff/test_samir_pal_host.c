/*
 * harness/diff/dbf_diff/test_samir_pal_host.c -- pal_host binding oracle (S0.2).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Reuses the seed test_assert.h
 * idiom (TEST_HARNESS / CHECK / TEST_SUMMARY) -- a non-zero process exit on any
 * failed check (CLAUDE.md Law 2: the oracle is the truth, never false-green).
 *
 * This is the S0.2 gate: verifies pal_host.c, the libc+injectable-clock PAL
 * binding, covers ALL contract requirements enumerated in the bead design note
 * (initech-586.5.3) and SAMIR-implementation-plan.md S0.2.
 *
 * Tests:
 *   1. CREATE + WRITE a temp file; CLOSE; re-OPEN read; READ back; assert bytes equal.
 *   2. seek(fd, 0, PAL_SEEK_END) == file size (ADR-0008 DEC-02 seek=filesize idiom).
 *   3. open() on a missing file -> -(PAL_ENOENT) (errno mapping; never raw magnitude).
 *   4. today() returns the INJECTED date (deterministic; Rule 11; NOT wall clock).
 *   5. Arena alloc + reset round-trip: alloc pointer is non-NULL; reset(NULL) unwinds;
 *      alloc after reset returns the same base address.
 *   6. Cleanup: remove the temp file via remove().
 *
 * Ref (Law 1):
 *   - SAMIR-implementation-plan.md S0.2 + bead initech-586.5.3 design note.
 *   - os/samir/include/samir/pal.h (contract; open/close/read/write/seek/remove).
 *   - ADR-0008 DEC-02 (seek=filesize; conin_char/gotoxy/set_attr are stubs here).
 *
 * ASCII-clean (Rule 12). No timestamps / host paths in artifacts (Rule 11).
 * The temp file name is relative (no absolute host path baked in).
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_assert.h"   /* seed/, on -Iseed                         */
#include "samir/pal.h"     /* os/samir/include/, on -Ios/samir/include  */

TEST_HARNESS();

/* Forward-declare the constructor/destructor from pal_host.c. */
struct pal_host_cfg {
    uint8_t  date_yy;
    uint8_t  date_mm;
    uint8_t  date_dd;
    uint32_t heap_size;
};
samir_pal_t *pal_host_make(struct pal_host_cfg cfg);
void         pal_host_free(samir_pal_t *p);

/* Temp file used for I/O tests -- relative, no host path baked in (Rule 11). */
#define TMPFILE  "test_samir_pal_host_tmp.bin"

/* Known payload to write and read back. */
static const uint8_t PAYLOAD[] = {
    0x03, 0x5A, 0x4D, 0x49, 0x52, 0x00, 0xFF, 0x01,
    0xDE, 0xAD, 0xBE, 0xEF, 0x42, 0x00, 0x7F, 0x20
};
#define PAYLOAD_LEN  ((uint32_t)sizeof(PAYLOAD))

int main(void)
{
    struct pal_host_cfg cfg;
    samir_pal_t *p;
    pal_fd fd;
    int32_t rc;
    uint8_t rbuf[32];

    /* ---- construct the host PAL with an injected date ---- */
    memset(&cfg, 0, sizeof cfg);
    cfg.date_yy  = 93;   /* 1993 */
    cfg.date_mm  = 7;
    cfg.date_dd  = 19;
    cfg.heap_size = 4096;

    p = pal_host_make(cfg);
    CHECK(p != NULL, "pal_host_make() must succeed");
    if (p == NULL) {
        /* Can't continue without the PAL; bail with the current fail count. */
        return TEST_SUMMARY("test_samir_pal_host");
    }

    /* Ensure the temp file does not exist from a prior failed run. */
    p->remove(p, TMPFILE);   /* ignore error */

    /* ================================================================
     * Test 1: CREATE + WRITE, then re-OPEN READ + READ BACK.
     * ================================================================ */
    fd = p->open(p, TMPFILE, PAL_WR | PAL_CREATE | PAL_TRUNC);
    CHECK(fd >= 0, "open(TMPFILE, WR|CREATE|TRUNC) must succeed");

    if (fd >= 0) {
        rc = p->write(p, fd, PAYLOAD, PAYLOAD_LEN);
        CHECK(rc == (int32_t)PAYLOAD_LEN,
              "write() must return the exact byte count written");

        CHECK(p->close(p, fd) == 0, "close() after write must succeed");
    }

    /* Re-open for reading. */
    fd = p->open(p, TMPFILE, PAL_RD);
    CHECK(fd >= 0, "re-open(TMPFILE, RD) must succeed");

    if (fd >= 0) {
        memset(rbuf, 0xCC, sizeof rbuf);
        rc = p->read(p, fd, rbuf, PAYLOAD_LEN);
        CHECK(rc == (int32_t)PAYLOAD_LEN,
              "read() must return the exact byte count read");
        CHECK(memcmp(rbuf, PAYLOAD, PAYLOAD_LEN) == 0,
              "read-back bytes must match written bytes (round-trip)");

        /* ============================================================
         * Test 2: seek(fd, 0, PAL_SEEK_END) == file size.
         * ============================================================ */
        rc = p->seek(p, fd, 0, PAL_SEEK_END);
        CHECK(rc == (int32_t)PAYLOAD_LEN,
              "seek(fd,0,PAL_SEEK_END) must equal file size (ADR-0008 DEC-02)");

        /* seek to start, read again -- confirms seek really repositioned. */
        rc = p->seek(p, fd, 0, PAL_SEEK_SET);
        CHECK(rc == 0, "seek(fd,0,PAL_SEEK_SET) must return new position 0");

        memset(rbuf, 0xCC, sizeof rbuf);
        rc = p->read(p, fd, rbuf, PAYLOAD_LEN);
        CHECK(rc == (int32_t)PAYLOAD_LEN,
              "second read after seek-to-start must return full count");
        CHECK(memcmp(rbuf, PAYLOAD, PAYLOAD_LEN) == 0,
              "second read after seek must also round-trip");

        CHECK(p->close(p, fd) == 0, "close() after read must succeed");
    }

    /* ================================================================
     * Test 3: open() on a missing file -> -(PAL_ENOENT).
     * The returned value must equal -(PAL_ENOENT) exactly -- symbolic
     * compare only; the raw errno magnitude must never leak (S0.1-FOLLOWUP).
     * ================================================================ */
    fd = p->open(p, "__no_such_file_samir__.dbf", PAL_RD);
    CHECK(fd == -(pal_fd)PAL_ENOENT,
          "open() on missing file must return -(PAL_ENOENT), never raw errno");

    /* ================================================================
     * Test 4: today() returns the injected date.
     * Deterministic -- NEVER reads the wall clock (Rule 11).
     * ================================================================ */
    {
        uint8_t yy = 0, mm = 0, dd = 0;
        p->today(p, &yy, &mm, &dd);
        CHECK(yy == 93, "today() must return the injected yy=93");
        CHECK(mm == 7,  "today() must return the injected mm=7");
        CHECK(dd == 19, "today() must return the injected dd=19");
    }

    /* ================================================================
     * Test 5: Arena alloc + reset round-trip.
     * ================================================================ */
    {
        void *a, *b, *mark, *a2;

        a = p->alloc(p, 64u);
        CHECK(a != NULL, "alloc(64) must return a non-NULL pointer");

        mark = p->alloc(p, 0u);  /* zero-size alloc captures current mark */
        CHECK(mark != NULL, "alloc(0) for mark capture must return non-NULL");

        b = p->alloc(p, 128u);
        CHECK(b != NULL, "alloc(128) after mark must return non-NULL");

        /* b is after mark, so b != mark. */
        CHECK((uint8_t *)b >= (uint8_t *)mark,
              "second alloc must not overlap the mark");

        /* Unwind to mark: next alloc should be at the same address as b. */
        p->reset(p, mark);
        a2 = p->alloc(p, 128u);
        CHECK(a2 == b,
              "alloc after reset(mark) must return the same address as before");

        /* Reset to base: next alloc should return the base address. */
        p->reset(p, NULL);
        a2 = p->alloc(p, 1u);
        CHECK(a2 == a, "alloc after reset(NULL) must return the arena base");

        /* Verify exhaustion: alloc more than the arena (cfg.heap_size=4096). */
        p->reset(p, NULL);
        a2 = p->alloc(p, 4096u + 1u);
        CHECK(a2 == NULL, "alloc beyond arena capacity must return NULL");
    }

    /* ================================================================
     * Test 6: Cleanup -- remove the temp file.
     * ================================================================ */
    {
        int rem = p->remove(p, TMPFILE);
        CHECK(rem == 0, "remove(TMPFILE) must succeed (clean up)");

        /* After removal, re-open should fail with PAL_ENOENT. */
        fd = p->open(p, TMPFILE, PAL_RD);
        CHECK(fd == -(pal_fd)PAL_ENOENT,
              "open() on removed file must return -(PAL_ENOENT)");
    }

    pal_host_free(p);

    return TEST_SUMMARY("test_samir_pal_host");
}
