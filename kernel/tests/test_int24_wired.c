/* test_int24_wired.c -- host oracle for the INT 24h critical-error WIRING
 * (beads initech-mvg). Factory test: libc OK, reuses seed/test_assert.h.
 *
 * THE GAP THIS CLOSES (Law 2): bead initech-509.8 built the INT 24h handler
 * (int24_dispatch: present MSG-DOS-0001, read the operator A/R/F key, return the
 * action in AL) AND a host oracle (test_int24.c) that pins the handler's LOGIC.
 * But NO kernel code path RAISED it -- a real disk error just returned an error
 * code up the syscall, the operator never saw "Abort, Retry, Fail?". This bead
 * wires the disk layer to RAISE INT 24h on a hard sector-I/O failure and HONOR
 * the answer, through TWO new seams exercised end-to-end here:
 *
 *   1. crit_blockdev (ata.c): a wrapper blockdev_t that sits between fat12.c and
 *      the real sector backend; on a negative inner return it raises the bound
 *      critical-error hook and, on a Retry decision, RE-ISSUES (bounded). This is
 *      the authentic DOS model -- INT 24h is raised by the disk DRIVER, not the
 *      FAT logic -- so fat12.c needs no change and every ata.c error source
 *      (floating-bus / BSY-DRQ timeout / status error / write-protect, all
 *      surfacing as a negative return) is covered uniformly.
 *   2. int21_run_critical_error (int21.c): the choke point the wrapper's hook is
 *      bound to. It RAISES INT 24h by running the REAL int24_dispatch over a
 *      synthesized frame (the SAME MSG-DOS-0001 + crit_error_action path
 *      test_int24.c pins), then maps the returned AL: Retry -> re-issue, Fail ->
 *      propagate the error, Abort -> terminate through the REAL do_terminate path
 *      (exit hook, code 0x23).
 *
 * We drive the wrapper at the SECTOR level over the lpf3 host file-backed backend
 * (blockdev_file, extended here with a READ-fault arm) -- arming a write/read
 * fault is EXACTLY a real ata I/O error -- and assert, per A/R/F answer:
 *   [A] Fail  -> INT 24h raised (MSG-DOS-0001 to CON), the op returns the inner
 *                error, the fault actually fired.
 *   [B] Retry -> INT 24h raised, the wrapper RE-ISSUES, and (the fault fires once)
 *                the re-issue SUCCEEDS -> the op returns 0.
 *   [C] Abort -> INT 24h raised, the REAL terminate hook fires (code 0x23), and
 *                the op still returns the error (Rule 2 -- no fake success).
 *   [D] Read fault + Fail -> the READ path raises INT 24h + propagates (proves a
 *                read source, not just write, is wired).
 *   [E] Bounded retry -> a PERMANENTLY-failing device + an always-Retry operator
 *                still TERMINATES after CRIT_BLOCKDEV_MAX_RETRIES (never hangs;
 *                Rule 2). Pins the retry BOUND is load-bearing.
 *   [F] Transparent default -> with NO hook bound the wrapper returns the inner
 *                error verbatim and NEVER prompts (no behavior change until wired).
 *
 * MUTATION (Rule 6), driven by make (test-int24-wired-mutant) -- each MUST go RED:
 *   -DMVG_MUTATE_NO_RAISE   : int21_run_critical_error returns FAIL WITHOUT
 *                             running int24_dispatch -> [A]/[B]/[C]/[D] RED (no
 *                             MSG-DOS-0001 raised; Retry never re-issues).
 *   -DMVG_MUTATE_RETRY_UNBOUNDED : crit_blockdev drops the retry bound -> [E]
 *                             hangs/never terminates (the test's bounded harness
 *                             catches it) -- proves the bound is load-bearing.
 * A mutant that PASSES means the oracle is decoration.
 *
 * Ref (Law 1): RBIL INT 24h (AL 0=Ignore/1=Retry/2=Abort/3=Fail varies by DOS
 *   rev; we use the crit_error_action A/R/F mapping pinned in int21.h); MS-DOS 3.3
 *   Tech Ref "Critical Error Handler"; ADR-0003 DEC-10; os/milton/ata.h
 *   (crit_blockdev), os/milton/int21.h (int21_run_critical_error, INT21_CRIT_*);
 *   blockdev.h (0 ok / negative on error). Image path is argv (the Makefile mints
 *   a blank floppy) -> no host path baked in (Rule 11). ASCII (Rule 12).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "int21.h"
#include "ata.h"             /* crit_blockdev wrapper (os/milton/) */
#include "blockdev.h"        /* blockdev_t contract                */
#include "blockdev_file.h"   /* lpf3 host file backend (harness)   */
#include "test_assert.h"
#include "dos_messages.h"    /* MSG_DOS_0001 (the SAME macro int21.c emits) */

TEST_HARNESS();

/* --- capturing CON sink (proves MSG-DOS-0001 was raised) ---------------- */
static char   g_sink_buf[1024];
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

/* --- the MOCK conin source: a queued A/R/F response string -------------- *
 * int24_dispatch reads the operator key through the blocking conin seam; we
 * drive it from a queued string. mock_get ABORTS the test on underflow so a
 * runaway re-prompt / unbounded retry can never silently hang the oracle. */
static const char *g_q;
static size_t      g_q_len;
static size_t      g_q_pos;

static void queue_set(const char *s)
{
    g_q     = s;
    g_q_len = strlen(s);
    g_q_pos = 0;
}

static int mock_get(void)
{
    if (g_q_pos >= g_q_len) {
        fprintf(stderr,
                "  FATAL %s: mock_get underflow -- int24 read more keys than "
                "queued (a runaway re-prompt or unbounded retry loop?)\n",
                __FILE__);
        exit(2);
    }
    return (unsigned char)g_q[g_q_pos++];
}

static int mock_poll(void)
{
    if (g_q_pos >= g_q_len) {
        return -1;
    }
    return (unsigned char)g_q[g_q_pos++];
}

/* --- the observable terminate hook (Abort routes here via do_terminate) - */
static int     g_exit_called;
static uint8_t g_exit_code;

static void exit_observe(uint8_t code)
{
    g_exit_called = 1;
    g_exit_code   = code;
}

/* --- a PERMANENTLY-failing inner blockdev (scenario [E]) ---------------- *
 * Every read/write returns an error, so an always-Retry operator can only be
 * stopped by the wrapper's retry BOUND. */
static int deaddev_read(void *ctx, uint32_t lba, uint32_t count, void *buf)
{
    (void)ctx; (void)lba; (void)count; (void)buf;
    return -4;   /* ATA_ERR_DEVICE-like: always fails */
}
static int deaddev_write(void *ctx, uint32_t lba, uint32_t count, const void *buf)
{
    (void)ctx; (void)lba; (void)count; (void)buf;
    return -4;
}

/* ===================================================================== */
int main(int argc, char **argv)
{
    const char      *img;
    blockdev_file_t  bf;
    crit_blockdev_t  cb;
    int              rc;
    uint8_t          sec[BLOCKDEV_SECTOR_SIZE];

    if (argc < 2) {
        fprintf(stderr, "usage: %s <blank-fat12-image>\n", argv[0]);
        return 2;
    }
    img = argv[1];

    /* Bind the int21 seams int24_dispatch + the choke point use: the capturing
     * sink (so we SEE MSG-DOS-0001), the queued conin (the operator response),
     * and the recording exit hook (so an Abort's do_terminate is observable). */
    int21_set_sink(sink_capture);
    int21_set_conin(mock_get, mock_poll);
    int21_set_exit(exit_observe);

    /* WIRE the disk layer to the critical-error choke point (the bead's wiring).
     * The kernel does exactly this at boot (crit_blockdev_set_hook(
     * int21_run_critical_error)); here we prove the SAME wiring host-side. */
    crit_blockdev_set_hook(int21_run_critical_error);

    rc = blockdev_file_open_rw(&bf, img);
    CHECK(rc == 0, "open the blank FAT12 image read-write");
    if (rc != 0) return TEST_SUMMARY("test_int24_wired");

    /* Wrap the host file backend with the critical-error wrapper -- the SAME
     * crit_blockdev_init the kernel calls around the ata device. From here we
     * drive cb.dev (the WRAPPED device) at the sector level. */
    crit_blockdev_init(&cb, &bf.dev);
    CHECK(cb.dev.read_sectors != NULL && cb.dev.write_sectors != NULL,
          "crit_blockdev_init wires read+write (inner backend is read-write)");

    /* =================================================================
     * [A] WRITE fault + 'F' (Fail): INT 24h raised, the op returns the inner
     *     error, the fault fired. (LBA 0 is harmless to clobber on the scratch
     *     image -- the wrapper FAILS the write so nothing lands anyway.)
     * ================================================================= */
    printf("[A] write fault + Fail -> INT 24h raised, error propagated\n");
    {
        sink_reset();
        queue_set("F");
        blockdev_file_arm_write_fault(&bf, 1u);   /* fail the 1st inner write */
        rc = cb.dev.write_sectors(cb.dev.ctx, 0u, 1u, sec);
        blockdev_file_arm_write_fault(&bf, 0u);   /* disarm */

        CHECK(strstr(sink_str(), MSG_DOS_0001) != NULL,
              "[A] the write failure RAISES INT 24h (MSG-DOS-0001 on CON)");
        CHECK(blockdev_file_write_faulted(&bf),
              "[A] the injected write fault ACTUALLY fired (else proves nothing)");
        CHECK(rc != 0,
              "[A] Fail PROPAGATES the inner error up (CF=1 / error code)");
    }

    /* =================================================================
     * [B] WRITE fault + 'R' (Retry): INT 24h raised, the wrapper RE-ISSUES; the
     *     fault fires ONCE (ordinal 1), so the re-issued write (ordinal 2)
     *     SUCCEEDS -> the wrapped op returns 0. THE load-bearing Retry proof.
     * ================================================================= */
    printf("[B] write fault + Retry -> re-issue succeeds\n");
    {
        sink_reset();
        queue_set("R");
        blockdev_file_arm_write_fault(&bf, 1u);   /* only the 1st write faults */
        rc = cb.dev.write_sectors(cb.dev.ctx, 1u, 1u, sec);
        blockdev_file_arm_write_fault(&bf, 0u);

        CHECK(strstr(sink_str(), MSG_DOS_0001) != NULL,
              "[B] the write failure RAISES INT 24h before the Retry");
        CHECK(blockdev_file_write_faulted(&bf),
              "[B] the injected write fault fired (the first attempt failed)");
        CHECK(rc == 0,
              "[B] Retry RE-ISSUES and the re-issued write SUCCEEDS (op returns 0)");
    }

    /* =================================================================
     * [C] WRITE fault + 'A' (Abort): INT 24h raised, the REAL terminate hook
     *     fires (do_terminate, code 0x23), and the op still returns the error.
     * ================================================================= */
    printf("[C] write fault + Abort -> terminate (do_terminate), error returned\n");
    {
        sink_reset();
        g_exit_called = 0; g_exit_code = 0xFFu;
        queue_set("A");
        blockdev_file_arm_write_fault(&bf, 1u);
        rc = cb.dev.write_sectors(cb.dev.ctx, 2u, 1u, sec);
        blockdev_file_arm_write_fault(&bf, 0u);

        CHECK(strstr(sink_str(), MSG_DOS_0001) != NULL,
              "[C] the write failure RAISES INT 24h before the Abort");
        CHECK(g_exit_called == 1,
              "[C] Abort routes through the REAL do_terminate -> the exit hook fires");
        CHECK(g_exit_code == 0x23u,
              "[C] Abort terminates with the DOS 'terminated by INT 24h' code 0x23");
        CHECK(rc != 0,
              "[C] Abort still returns the error (no fake success after terminate)");
    }

    /* =================================================================
     * [D] READ fault + 'F' (Fail): a READ source raises INT 24h + propagates
     *     (proves the read path is wired, not just the write path).
     * ================================================================= */
    printf("[D] read fault + Fail -> read path raises INT 24h, error propagated\n");
    {
        sink_reset();
        queue_set("F");
        blockdev_file_arm_read_fault(&bf, 1u);    /* fail the 1st inner read */
        rc = cb.dev.read_sectors(cb.dev.ctx, 0u, 1u, sec);
        blockdev_file_arm_read_fault(&bf, 0u);

        CHECK(strstr(sink_str(), MSG_DOS_0001) != NULL,
              "[D] the read failure RAISES INT 24h (read path wired, not just write)");
        CHECK(blockdev_file_read_faulted(&bf),
              "[D] the injected read fault ACTUALLY fired");
        CHECK(rc != 0, "[D] Fail PROPAGATES the read error up");
    }

    /* =================================================================
     * [E] PERMANENTLY-failing device + always-Retry: the wrapper must STOP
     *     after CRIT_BLOCKDEV_MAX_RETRIES (never hang). We queue exactly
     *     MAX_RETRIES+1 'R' keys -- the wrapper consumes at most MAX_RETRIES of
     *     them (one per re-issue decision) then gives up; a queue UNDERFLOW
     *     (mock_get exit(2)) would mean the bound was dropped (an unbounded loop
     *     would read past MAX_RETRIES). The op returns the inner error.
     * ================================================================= */
    printf("[E] permanently-failing device + always-Retry -> bounded, terminates\n");
    {
        blockdev_t dead;
        crit_blockdev_t cbd;
        char retries[CRIT_BLOCKDEV_MAX_RETRIES + 2u];
        unsigned i;

        dead.ctx           = NULL;
        dead.read_sectors  = deaddev_read;
        dead.write_sectors = deaddev_write;
        crit_blockdev_init(&cbd, &dead);

        /* MAX_RETRIES+1 'R' (one spare): a correctly-bounded wrapper reads at
         * most MAX_RETRIES of them (the first failure + MAX_RETRIES re-issue
         * decisions all answered Retry, then the bound stops it). */
        for (i = 0u; i < (unsigned)(CRIT_BLOCKDEV_MAX_RETRIES + 1u); i++) {
            retries[i] = 'R';
        }
        retries[CRIT_BLOCKDEV_MAX_RETRIES + 1u] = '\0';

        sink_reset();
        queue_set(retries);
        rc = cbd.dev.write_sectors(cbd.dev.ctx, 0u, 1u, sec);

        CHECK(rc != 0,
              "[E] a permanently-dead device returns the error (Retry cannot fix it)");
        CHECK(g_q_pos <= (size_t)CRIT_BLOCKDEV_MAX_RETRIES,
              "[E] the Retry loop is BOUNDED (<= MAX_RETRIES re-issues; did not run away)");
    }

    /* =================================================================
     * [F] NO hook bound -> the wrapper is TRANSPARENT: it returns the inner
     *     error verbatim and NEVER prompts (no behavior change until wired).
     *     Restores the hook afterwards for hygiene.
     * ================================================================= */
    printf("[F] no hook bound -> transparent (no prompt, inner error returned)\n");
    {
        sink_reset();
        crit_blockdev_set_hook(NULL);             /* unwire */
        blockdev_file_arm_write_fault(&bf, 1u);
        rc = cb.dev.write_sectors(cb.dev.ctx, 3u, 1u, sec);
        blockdev_file_arm_write_fault(&bf, 0u);
        crit_blockdev_set_hook(int21_run_critical_error);  /* re-wire */

        CHECK(rc != 0,
              "[F] no hook -> the inner write error is returned verbatim (Fail)");
        CHECK(strstr(sink_str(), MSG_DOS_0001) == NULL,
              "[F] no hook -> INT 24h is NOT raised (no MSG-DOS-0001 -- transparent)");
    }

    blockdev_file_close(&bf);
    return TEST_SUMMARY("test_int24_wired");
}
