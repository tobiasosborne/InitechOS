/* test_exec.c -- host unit oracle for INT 21h AH=4Bh EXEC + AH=4Dh GET-RETURN-
 * CODE dispatch logic (beads initech-saw + initech-509.5). Factory test: libc
 * OK, reuses seed/test_assert.h.
 *
 * Ref: DOS 3.3 Programmer's Reference Manual AH=4Bh (EXEC, AL=00h load+execute,
 *      EDX -> ASCIIZ path) / AH=4Dh (GET RETURN CODE, AL=child exit code, AH=
 *      termination type); os/milton/int21.c do_exec / do_get_return_code;
 *      os/milton/int21.h int21_exec_fn (the EXEC backend seam); ADR-0003 DEC-08
 *      (flat .COM). CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN),
 *      Rule 2 (fail loud), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Compiles HOSTED against the REAL artifact int21.c (the same int21_dispatch the
 * kernel runs). The actual FAT-sourced load+run (loader.c) is KERNEL-ONLY, so
 * the dispatcher reaches it through the int21_exec_fn seam, which we override
 * with a MOCK that records the requested name + returns a scripted result. This
 * is exactly the file-backend/sink/conin pattern: it lets us test the 4Bh/4Dh
 * register decode + path validation + not-found/nested mapping WITHOUT linking
 * loader.c. The in-emulator make test-exec proves the real load FROM the FAT
 * volume; this proves the dispatch logic.
 *
 * MUTATION (Rule 6), driven by make:
 *   -DINT21_MUTATE_RETCODE_ZERO : AH=4Dh always reports rc=0 -> the GET-RETURN-
 *                                 CODE test (EXEC of a program that exits rc=7)
 *                                 goes RED. A mutant that PASSES means the 4Dh
 *                                 oracle is decoration.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>

#include "int21.h"
#include "test_assert.h"

TEST_HARNESS();

/* EDX is a FLAT 32-bit linear address; on a 64-bit host any path buffer must
 * live in the low 4 GiB so (uint32_t)(uintptr_t)p round-trips (mirrors
 * test_int21.c alloc_low). */
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

/* --- The mock EXEC backend ----------------------------------------------- */
static int      g_exec_called;
static char     g_exec_name[64];
static uint16_t g_exec_result;   /* the DOS error the mock returns (0 = success) */
static uint8_t  g_exec_child_rc; /* the child rc the mock reports on success     */

static uint16_t exec_mock(const char *name83, uint8_t *out_rc)
{
    g_exec_called = 1;
    g_exec_name[0] = '\0';
    if (name83) {
        size_t i = 0;
        for (; name83[i] && i < sizeof(g_exec_name) - 1; i++) {
            g_exec_name[i] = name83[i];
        }
        g_exec_name[i] = '\0';
    }
    if (g_exec_result == 0u && out_rc) {
        *out_rc = g_exec_child_rc;
    }
    return g_exec_result;
}

static void exec_reset(uint16_t result, uint8_t child_rc)
{
    g_exec_called   = 0;
    g_exec_name[0]  = '\0';
    g_exec_result   = result;
    g_exec_child_rc = child_rc;
}

#define CF_BIT 0x1u
static int frame_cf(const int_frame_t *f) { return (f->eflags & CF_BIT) ? 1 : 0; }

static int_frame_t fresh_frame(void)
{
    int_frame_t f;
    memset(&f, 0, sizeof(f));
    f.eflags = 0x00000202u;  /* IF + reserved bit1; CF clear initially */
    return f;
}

int main(void)
{
    int21_set_exec_backend(exec_mock);

    /* --- AH=4Bh AL=00h, valid path "GREET.COM", child rc=7 -> backend invoked
     *     with the name; CF clear (the child ran + returned). ----------------- */
    {
        exec_reset(0u, 7u);
        uint32_t edx = low_dup("GREET.COM");
        CHECK(edx != 0u, "low_dup GREET.COM buffer in low 4 GiB");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;          /* AH=4Bh, AL=00h (load+execute) */
        f.edx = edx;
        f.eflags |= CF_BIT;       /* preload CF=1 so we prove it is CLEARED */
        int21_dispatch(&f);
        CHECK(g_exec_called == 1, "AH=4Bh AL=0 must reach the EXEC backend");
        CHECK_STR_EQ(g_exec_name, "GREET.COM",
                     "AH=4Bh passes the EDX path to the EXEC backend");
        CHECK(frame_cf(&f) == 0, "AH=4Bh success (child ran+returned) clears CF");
    }

    /* --- AH=4Dh GET RETURN CODE after that EXEC -> AL = child rc (7), AH = 0
     *     (normal), CF clear. THE mutation point. ----------------------------- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x4D00u;          /* AH=4Dh */
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 7u,
              "AH=4Dh returns the last child's exit code in AL (7)");
        CHECK((uint8_t)((f.eax >> 8) & 0xFFu) == 0u,
              "AH=4Dh AH = 0 (normal termination)");
        CHECK(frame_cf(&f) == 0, "AH=4Dh clears CF");
    }

    /* --- AH=4Dh a SECOND time -> rc consumed (DOS clears after read): AL=0. -- */
    {
        int_frame_t f = fresh_frame();
        f.eax = 0x4D00u;
        int21_dispatch(&f);
        CHECK((uint8_t)(f.eax & 0xFFu) == 0u,
              "AH=4Dh is consuming: a second read returns 0");
    }

    /* --- AH=4Bh with a subdir '\' in the path -> CF=1, AX=0x0003 (path not
     *     found); the backend is NOT called (validation rejects first). ------- */
    {
        exec_reset(0u, 0u);
        uint32_t edx = low_dup("SUB\\GREET.COM");
        CHECK(edx != 0u, "low_dup subdir-path buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh with '\\' sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "AH=4Bh with '\\' returns AX=0x0003 (path not found)");
        CHECK(g_exec_called == 0,
              "AH=4Bh rejects a subdir path BEFORE calling the backend");
    }

    /* --- AH=4Bh with a drive ':' in the path -> CF=1, AX=0x0003. ------------ */
    {
        exec_reset(0u, 0u);
        uint32_t edx = low_dup("C:GREET.COM");
        CHECK(edx != 0u, "low_dup drive-path buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh with ':' sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "AH=4Bh with ':' returns AX=0x0003");
        CHECK(g_exec_called == 0, "AH=4Bh rejects a drive path before the backend");
    }

    /* --- AH=4Bh empty path -> CF=1, AX=0x0002 (file not found). ------------- */
    {
        exec_reset(0u, 0u);
        uint32_t edx = low_dup("");
        CHECK(edx != 0u, "low_dup empty-path buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh empty path sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "AH=4Bh empty path returns AX=0x0002");
        CHECK(g_exec_called == 0, "AH=4Bh empty path does not call the backend");
    }

    /* --- AH=4Bh missing file (backend returns 0x0002) -> CF=1, AX=0x0002. The
     *     backend WAS called; the failure is mapped, not a false success. ----- */
    {
        exec_reset(INT21_ERR_FILE_NOT_FOUND, 0u);
        uint32_t edx = low_dup("NOPE.COM");
        CHECK(edx != 0u, "low_dup NOPE.COM buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        f.eflags |= CF_BIT;       /* preload; the failure path must KEEP CF set */
        int21_dispatch(&f);
        CHECK(g_exec_called == 1, "AH=4Bh of a missing file still calls the backend");
        CHECK(frame_cf(&f) == 1, "AH=4Bh backend failure sets CF (fail loud)");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "AH=4Bh missing file returns AX=0x0002");
    }

    /* --- AH=4Bh nested (backend returns 0x0008 -- a load is already active) ->
     *     CF=1, AX=0x0008 (insufficient memory). The reentrancy guard surfaced
     *     by the loader maps here. -------------------------------------------- */
    {
        exec_reset(INT21_ERR_INSUFFICIENT_MEM, 0u);
        uint32_t edx = low_dup("GREET.COM");
        CHECK(edx != 0u, "low_dup nested-case buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh nested (load active) sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INSUFFICIENT_MEM,
              "AH=4Bh nested EXEC returns AX=0x0008 (deferred; guarded)");
    }

    /* --- AH=4Bh AL=03h (load overlay) + other subfunctions -> CF=1, AX=0x0001
     *     (invalid function); out of scope this milestone. -------------------- */
    {
        exec_reset(0u, 0u);
        uint32_t edx = low_dup("GREET.COM");
        CHECK(edx != 0u, "low_dup overlay-case buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B03u;          /* AH=4Bh, AL=03h (load overlay) */
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh AL=03h sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_INVALID_FUNCTION,
              "AH=4Bh AL=03h (overlay) returns AX=0x0001 (out of scope)");
        CHECK(g_exec_called == 0, "AH=4Bh overlay does not call the load backend");
    }

    /* --- No backend bound (NULL) -> AH=4Bh returns file-not-found (Rule 2:
     *     never a silent success when nothing could have run). ---------------- */
    {
        int21_set_exec_backend(NULL);
        uint32_t edx = low_dup("GREET.COM");
        CHECK(edx != 0u, "low_dup no-backend buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh with no EXEC backend bound sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_FILE_NOT_FOUND,
              "AH=4Bh with no backend returns AX=0x0002 (not a silent success)");
        int21_set_exec_backend(exec_mock);   /* restore for any later cases */
    }

    return TEST_SUMMARY("test_exec");
}
