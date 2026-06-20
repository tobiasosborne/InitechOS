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
#include "dos_structs.h"   /* exec_param_block_t (AH=4Bh EBX block; initech-456) */
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
static uint16_t g_exec_dir_start;/* the containing-dir cluster do_exec resolved   */
static uint16_t g_exec_result;   /* the DOS error the mock returns (0 = success) */
static uint8_t  g_exec_child_rc; /* the child rc the mock reports on success     */
static char     g_exec_tail[160];/* the command tail TEXT the backend received   */
static uint32_t g_exec_tail_len; /* its length (initech-456)                     */
static uint32_t g_exec_env_block;/* the env_block the backend received (inc 3)    */

static uint16_t exec_mock(const char *name83, uint16_t dir_start,
                          const char *cmd_tail, uint32_t cmd_tail_len,
                          uint32_t env_block, uint8_t *out_rc)
{
    g_exec_called = 1;
    g_exec_dir_start = dir_start;
    g_exec_env_block = env_block;   /* capture the threaded env block (inc 3) */
    g_exec_name[0] = '\0';
    if (name83) {
        size_t i = 0;
        for (; name83[i] && i < sizeof(g_exec_name) - 1; i++) {
            g_exec_name[i] = name83[i];
        }
        g_exec_name[i] = '\0';
    }
    /* Capture the command tail the dispatcher extracted from the EBX param block
     * (initech-456). cmd_tail is the TEXT (no count byte / CR), length cmd_tail_len. */
    g_exec_tail_len = cmd_tail_len;
    {
        uint32_t i = 0;
        if (cmd_tail) {
            for (; i < cmd_tail_len && i < sizeof(g_exec_tail) - 1; i++) {
                g_exec_tail[i] = cmd_tail[i];
            }
        }
        g_exec_tail[i] = '\0';
    }
    if (g_exec_result == 0u && out_rc) {
        *out_rc = g_exec_child_rc;
    }
    return g_exec_result;
}

static void exec_reset(uint16_t result, uint8_t child_rc)
{
    g_exec_called    = 0;
    g_exec_name[0]   = '\0';
    g_exec_dir_start = 0xFFFFu;   /* sentinel: prove do_exec wrote the real value */
    g_exec_result    = result;
    g_exec_child_rc  = child_rc;
    g_exec_tail[0]   = '\0';
    g_exec_tail_len  = 0;
    g_exec_env_block = 0xFFFFFFFFu;   /* sentinel: prove do_exec wrote the real value */
}

/* --- A MOCK resolve backend (beads initech-zs24) -------------------------- *
 * The dispatch logic for SUBDIR EXEC lives in do_exec -> resolve_dir_path ->
 * g_file->resolve (the file backend seam). With NO file backend bound (the
 * default below), resolve_dir_path falls back to root-only (a bare name resolves
 * to dir_start 0; any '\'/':' -> 0x0003) -- the pre-zs24 behavior the existing
 * cases assert. To prove do_exec THREADS a resolved subdir cluster to the EXEC
 * backend the SAME way OPEN does, this mock resolve maps "\SUB\GREET.COM" to
 * (dir_start=7, leaf="GREET.COM"), a bare name to (dir_start=cwd_start,
 * leaf=name), and a missing dir to 0x0003 -- a tiny stand-in for fat12_resolve_path
 * (the REAL backend is exercised end-to-end by the in-emulator test-zs24-exec
 * gate + the kernel test_fileio_subdir integration test). */
#define MOCK_SUB_CLUSTER  7u
static uint16_t resolve_mock(const char *path, uint16_t cwd_start,
                             const char **out_leaf, uint16_t *out_dir_start)
{
    /* "\SUB\GREET.COM" -> dir_start=7, leaf points at "GREET.COM" within path. */
    if (strncmp(path, "\\SUB\\", 5) == 0) {
        const char *leaf = path + 5;
        if (strchr(leaf, '\\') != NULL) {
            return INT21_ERR_PATH_NOT_FOUND;   /* deeper than one level: unknown */
        }
        *out_leaf      = leaf;
        *out_dir_start = MOCK_SUB_CLUSTER;
        return 0u;
    }
    /* "\BADDIR\..." -> a missing directory component (DOS path-not-found). */
    if (strncmp(path, "\\BADDIR\\", 8) == 0) {
        return INT21_ERR_PATH_NOT_FOUND;
    }
    /* A bare root-relative name resolves to the CWD (root-relative, cwd_start). */
    if (strchr(path, '\\') == NULL) {
        *out_leaf      = path;
        *out_dir_start = cwd_start;
        return 0u;
    }
    return INT21_ERR_PATH_NOT_FOUND;
}

static const int21_file_backend_t g_resolve_backend = { .resolve = resolve_mock };

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
        CHECK(g_exec_dir_start == 0u,
              "AH=4Bh of a bare root name resolves dir_start=0 (root)");
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

    /* --- AH=4Bh with a subdir '\' in the path + NO resolve backend bound ->
     *     CF=1, AX=0x0003 (path not found); the backend is NOT called. With no
     *     file backend, resolve_dir_path falls back to root-only (the pre-zs24
     *     behavior): a '\' cannot resolve -> 0x0003. (The subdir-RESOLVE path is
     *     proven below once a mock resolve backend is bound.) ------------------ */
    {
        exec_reset(0u, 0u);
        uint32_t edx = low_dup("SUB\\GREET.COM");
        CHECK(edx != 0u, "low_dup subdir-path buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh with '\\' + no resolve backend sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "AH=4Bh with '\\' + no resolve backend returns AX=0x0003");
        CHECK(g_exec_called == 0,
              "AH=4Bh root-only fallback rejects a subdir path BEFORE the backend");
    }

    /* --- AH=4Bh with a leading drive 'C:' -> the drive prefix is STRIPPED and the
     *     remaining bare name resolves to the root, IDENTICALLY to do_open (beads
     *     initech-zs24 -- EXEC path resolution must MATCH OPEN path resolution).
     *     The backend IS called with leaf="GREET.COM", dir_start=0. Earlier EXEC
     *     rejected any ':'; now it honors a drive letter the way OPEN does. ----- */
    {
        exec_reset(0u, 0u);
        uint32_t edx = low_dup("C:GREET.COM");
        CHECK(edx != 0u, "low_dup drive-path buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        f.eflags |= CF_BIT;       /* preload CF=1 so we prove it is CLEARED */
        int21_dispatch(&f);
        CHECK(g_exec_called == 1,
              "AH=4Bh strips the 'C:' drive and reaches the backend (matches OPEN)");
        CHECK_STR_EQ(g_exec_name, "GREET.COM",
                     "AH=4Bh passes the post-drive-strip leaf to the backend");
        CHECK(g_exec_dir_start == 0u,
              "AH=4Bh of 'C:GREET.COM' resolves dir_start=0 (root)");
        CHECK(frame_cf(&f) == 0, "AH=4Bh 'C:GREET.COM' clears CF (drive stripped, ran)");
    }

    /* --- AH=4Bh with an OVERLENGTH path -> CF=1, AX=0x0003; runaway-guarded. *
     * A possibly-unterminated/oversized ASCIIZ pointer must not be scanned off
     * into memory (Rule 2). resolve_dir_path's path_overlength bounds the scan at
     * INT21_PATH_SCAN_MAX (128) and rejects anything longer BEFORE the backend.
     * 199 'A's (no '\' or ':') exceeds the bound -> rejected. The
     * INT21_MUTATE_PATHSCAN_NOBOUND mutant removes the bound -> the path passes
     * validation, the backend is called, and this goes RED. */
    {
        exec_reset(0u, 0u);
        char longname[200];
        memset(longname, 'A', sizeof(longname) - 1u);
        longname[sizeof(longname) - 1u] = '\0';   /* 199 'A's, NUL-terminated */
        uint32_t edx = low_dup(longname);
        CHECK(edx != 0u, "low_dup overlength-path buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        int21_dispatch(&f);
        CHECK(frame_cf(&f) == 1, "AH=4Bh overlength path sets CF");
        CHECK((uint16_t)(f.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "AH=4Bh overlength path -> AX=0x0003 (runaway-guarded reject)");
        CHECK(g_exec_called == 0,
              "AH=4Bh rejects an overlength path BEFORE the backend (bounded scan)");
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

    /* --- AH=4Bh command-tail args (initech-456): EBX -> exec_param_block_t whose
     *     cmd_tail points at a DOS-format {count, text, CR}; the dispatcher must
     *     extract the TEXT + len and hand them to the backend (-> child PSP:80h).
     *     Also assert the no-EBX path degrades to a no-argument launch. ------- */
    {
        const char *tailtext = " /S FILE.TXT";   /* leading separator + args */
        uint8_t n = (uint8_t)strlen(tailtext);

        /* The DOS-format tail {count, text, CR} in low memory. */
        uint8_t *tailblk = (uint8_t *)alloc_low((size_t)n + 2u);
        CHECK(tailblk != NULL, "alloc_low tail block in low 4 GiB");
        tailblk[0] = n;
        memcpy(tailblk + 1, tailtext, n);
        tailblk[1 + n] = 0x0Du;

        /* The EXEC parameter block in low memory; EBX -> it. */
        exec_param_block_t *pb = (exec_param_block_t *)alloc_low(sizeof(*pb));
        CHECK(pb != NULL, "alloc_low param block in low 4 GiB");
        pb->env_block = 0u;
        pb->cmd_tail  = (uint32_t)(uintptr_t)tailblk;
        pb->fcb1 = 0u; pb->fcb2 = 0u;

        exec_reset(0u, 0u);
        uint32_t edx = low_dup("GREET.COM");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        f.ebx = (uint32_t)(uintptr_t)pb;
        int21_dispatch(&f);
        CHECK(g_exec_called == 1, "AH=4Bh with args reaches the backend");
        CHECK(frame_cf(&f) == 0, "AH=4Bh with args clears CF on a clean run");
        CHECK(g_exec_tail_len == (uint32_t)n,
              "AH=4Bh passes the command-tail length to the backend");
        CHECK_STR_EQ(g_exec_tail, " /S FILE.TXT",
                     "AH=4Bh passes the command-tail TEXT (-> PSP:80h) to the backend");
        CHECK(g_exec_env_block == 0u,
              "AH=4Bh with env_block=0 threads 0 (inherit-empty) to the backend");

        /* No EBX param block (fresh_frame zeroes EBX) -> a no-argument launch
         * (count=0), never a fault on a legacy caller's stale EBX (Rule 2). The
         * env_block also degrades to 0 (inherit-empty), never a fault. */
        exec_reset(0u, 0u);
        uint32_t edx2 = low_dup("GREET.COM");
        int_frame_t g = fresh_frame();
        g.eax = 0x4B00u;
        g.edx = edx2;
        int21_dispatch(&g);
        CHECK(g_exec_called == 1, "AH=4Bh with no param block still runs");
        CHECK(g_exec_tail_len == 0u,
              "AH=4Bh with no EBX block -> empty tail (no-argument launch)");
        CHECK(g_exec_env_block == 0u,
              "AH=4Bh with no EBX block -> env_block=0 (inherit-empty, no fault)");
    }

    /* --- AH=4Bh env inheritance threading (beads initech-1i0x Tranche E inc 3):
     *     the EBX param block's env_block field (offset 0, a FLAT linear ptr) must
     *     reach the EXEC backend UNCHANGED, so the kernel loader can point the child
     *     PSP env_seg at the inherited block. We assert a populated env_block (a
     *     non-zero flat addr standing in for ENV_BLOCK) threads through verbatim. -- */
    {
        const uint32_t kEnvAddr = 0x0005F000u;   /* ENV_BLOCK; do_exec is address-agnostic */

        exec_param_block_t *pb = (exec_param_block_t *)alloc_low(sizeof(*pb));
        CHECK(pb != NULL, "alloc_low env-param block in low 4 GiB");
        pb->env_block = kEnvAddr;     /* the shell's populated env block            */
        pb->cmd_tail  = 0u;           /* no tail (no-argument launch)               */
        pb->fcb1 = 0u; pb->fcb2 = 0u;

        exec_reset(0u, 0u);
        uint32_t edx = low_dup("GREET.COM");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        f.ebx = (uint32_t)(uintptr_t)pb;
        int21_dispatch(&f);
        CHECK(g_exec_called == 1, "AH=4Bh with a populated env_block reaches the backend");
        CHECK(g_exec_env_block == kEnvAddr,
              "AH=4Bh threads the param block's env_block (ENV_BLOCK) to the backend");
        CHECK(g_exec_tail_len == 0u,
              "AH=4Bh env-only block -> empty tail (no-argument launch)");
    }

    /* --- SUBDIR EXEC dispatch (beads initech-zs24, Landing 2) ---------------- *
     * Bind a mock resolve backend so do_exec resolves a '\SUB\FILE' path EXACTLY
     * the way do_open does (resolve_dir_path -> g_file->resolve). The dispatcher
     * must hand the EXEC backend the BARE LEAF + the resolved containing-directory
     * cluster (NOT the whole path), so the loader can locate the .COM in that
     * subdir. The REAL fat12_resolve_path stack + the actual subdir load are
     * exercised end-to-end by the in-emulator test-zs24-exec gate; this proves the
     * int21 dispatch WIRING (leaf + dir_start) deterministically and hosted. */
    {
        int21_set_file_backend(&g_resolve_backend);

        /* (a) Absolute "\SUB\GREET.COM" -> backend gets leaf="GREET.COM",
         *     dir_start=7 (the mock's SUB cluster). This is the load-bearing
         *     assertion: a subdir EXEC threads the resolved cluster, not 0. */
        exec_reset(0u, 9u);
        uint32_t edx = low_dup("\\SUB\\GREET.COM");
        CHECK(edx != 0u, "low_dup '\\SUB\\GREET.COM' buffer");
        int_frame_t f = fresh_frame();
        f.eax = 0x4B00u;
        f.edx = edx;
        f.eflags |= CF_BIT;
        int21_dispatch(&f);
        CHECK(g_exec_called == 1,
              "AH=4Bh '\\SUB\\GREET.COM' resolves + reaches the EXEC backend");
        CHECK_STR_EQ(g_exec_name, "GREET.COM",
                     "AH=4Bh hands the backend the BARE LEAF (not the whole path)");
        CHECK(g_exec_dir_start == MOCK_SUB_CLUSTER,
              "AH=4Bh threads the RESOLVED subdir cluster (=7) to the EXEC backend");
        CHECK(frame_cf(&f) == 0, "AH=4Bh subdir EXEC clears CF on a clean run");

        /* (b) A bare name with the CWD at root resolves dir_start=0 (cwd_start). */
        exec_reset(0u, 0u);
        uint32_t edx2 = low_dup("GREET.COM");
        int_frame_t g = fresh_frame();
        g.eax = 0x4B00u;
        g.edx = edx2;
        int21_dispatch(&g);
        CHECK(g_exec_called == 1, "AH=4Bh bare name + resolve backend reaches backend");
        CHECK(g_exec_dir_start == 0u,
              "AH=4Bh bare name at root CWD resolves dir_start=0 (cwd_start)");

        /* (c) A '\SUB\FILE' whose non-final component is MISSING -> 0x0003
         *     (path not found), the backend is NOT called (resolve rejects). */
        exec_reset(0u, 0u);
        uint32_t edx3 = low_dup("\\BADDIR\\GREET.COM");
        int_frame_t h = fresh_frame();
        h.eax = 0x4B00u;
        h.edx = edx3;
        int21_dispatch(&h);
        CHECK(frame_cf(&h) == 1, "AH=4Bh '\\BADDIR\\..' (missing dir) sets CF");
        CHECK((uint16_t)(h.eax & 0xFFFFu) == INT21_ERR_PATH_NOT_FOUND,
              "AH=4Bh of a missing-dir subdir path returns AX=0x0003");
        CHECK(g_exec_called == 0,
              "AH=4Bh does not call the EXEC backend when the resolve fails");

        int21_set_file_backend(NULL);   /* restore: leave no resolve backend bound */
    }

    return TEST_SUMMARY("test_exec");
}
