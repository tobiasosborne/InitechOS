/* Exit status of the emulator harnesses (factory/README.md, Emulator exit
 * status). CLAUDE.md, The oracle is the truth: a guest run that never
 * completed proves nothing about the product, so it must not look like a
 * failed verdict (exit 1) to a test or to the mutation runner.
 *
 *   0  the run completed and its verdict passed
 *   1  the run completed and its verdict failed (product evidence)
 *   2  the harness itself failed (arguments, files, sockets)
 *   3  the guest run never completed: not launched, timed out, or the
 *      emulator ended without a byte of serial output and no triple fault
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
                                 size_t serial_len, int triple_fault)
{
    if (!launched || timed_out || (serial_len == 0 && !triple_fault))
        return EMU_EXIT_INCOMPLETE;
    return ok ? EMU_EXIT_OK : EMU_EXIT_VERDICT;
}

static inline void emu_record_infra(const char *tool, int status,
                                    int launched, int timed_out, size_t serial_len)
{
    const char *path = getenv("INITECH_INFRA_LOG");
    FILE *f;
    if (status != EMU_EXIT_HARNESS && status != EMU_EXIT_INCOMPLETE) return;
    if (path == NULL || path[0] == '\0') return;
    f = fopen(path, "a");
    if (f == NULL) return;
    fprintf(f, "%s exit %d (launched=%d timed_out=%d serial_len=%zu)\n",
            tool, status, launched, timed_out, serial_len);
    fclose(f);
}

/* Progress-renewed deadline (factory/README.md, Emulator exit status): on
 * guest progress the deadline moves to now + renew_ms, never past ceiling and
 * never earlier than it was; without progress it does not move. */
static inline long long emu_deadline_renew(long long deadline, long long ceiling,
                                           long long now, long long renew_ms,
                                           int progressed)
{
    long long want = now + renew_ms;
    if (!progressed || renew_ms <= 0) return deadline;
    if (want > ceiling) want = ceiling;
    return want > deadline ? want : deadline;
}
#endif
