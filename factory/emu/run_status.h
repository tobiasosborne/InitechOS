/* Exit status of the emulator harnesses (factory/README.md, Emulator exit
 * status). CLAUDE.md, The oracle is the truth: a guest run that never
 * completed proves nothing about the product, so it must not look like a
 * failed verdict (exit 1) to a test or to the mutation runner.
 *
 *   0  the run completed and its verdict passed
 *   1  the run completed and its verdict failed (product evidence)
 *   2  the harness itself failed (arguments, files, sockets)
 *   3  the guest run never completed: not launched, timed out, the
 *      emulator ended without a byte of serial output and no triple fault,
 *      or it ended abnormally (a signal the harness did not send, an exit
 *      status that is no guest verdict, or it could not be reaped) without
 *      a triple fault, however much serial it had printed (review F2)
 *
 * When INITECH_INFRA_LOG names a file, exits 2 and 3 append one line to it;
 * factory/testlib/run-mutants.sh reads that file and reports the mutant as an
 * error, never a kill. */
#ifndef INITECH_EMU_RUN_STATUS_H
#define INITECH_EMU_RUN_STATUS_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum { EMU_EXIT_OK = 0, EMU_EXIT_VERDICT = 1, EMU_EXIT_HARNESS = 2, EMU_EXIT_INCOMPLETE = 3 };

static inline int emu_run_status(int ok, int launched, int timed_out,
                                 size_t serial_len, int triple_fault,
                                 int abnormal_end)
{
    if (!launched || timed_out) return EMU_EXIT_INCOMPLETE;
    /* A triple fault is the guest's own doing, whatever became of the
     * emulator process afterwards. */
    if (!triple_fault && (serial_len == 0 || abnormal_end))
        return EMU_EXIT_INCOMPLETE;
    return ok ? EMU_EXIT_OK : EMU_EXIT_VERDICT;
}

/* The emulator process ended in a way no guest causes: it could not be
 * reaped, a signal the harness did not send killed it (SIGKILL from the
 * OOM killer, SIGSEGV), or it exited with a status that is not a guest
 * verdict (bad_exit, decided by each harness). */
static inline int emu_ended_abnormally(int reap_failed, int term_signal,
                                       int harness_stopped, int bad_exit)
{
    if (reap_failed) return 1;
    if (term_signal != 0) return !harness_stopped;
    return bad_exit;
}

static inline void emu_record_infra(const char *tool, int status,
                                    int launched, int timed_out, size_t serial_len,
                                    int abnormal_end)
{
    const char *path = getenv("INITECH_INFRA_LOG");
    FILE *f;
    if (status != EMU_EXIT_HARNESS && status != EMU_EXIT_INCOMPLETE) return;
    if (path == NULL || path[0] == '\0') return;
    f = fopen(path, "a");
    if (f == NULL) return;
    fprintf(f, "%s exit %d (launched=%d timed_out=%d serial_len=%zu abnormal_end=%d)\n",
            tool, status, launched, timed_out, serial_len, abnormal_end);
    fclose(f);
}
#endif
