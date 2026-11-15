/* An emulator run that never completed must not exit 1 (initech-kkqbw). */
#include "../emu/run_status.h"
#include <stdio.h>
#include <string.h>

static int fails;
static void want(int got, int expected, const char *what)
{
    if (got != expected) { printf("FAIL emu exit: %s gave %d, want %d\n", what, got, expected); fails++; }
}

int main(int argc, char **argv)
{
    want(emu_run_status(1, 1, 0, 100, 0, 0), 0, "completed passing run");
    want(emu_run_status(0, 1, 0, 100, 0, 0), 1, "completed failing run");
    want(emu_run_status(0, 1, 0, 100, 1, 0), 1, "triple fault after output");
    want(emu_run_status(0, 1, 0, 0, 1, 0), 1, "triple fault before output");
    want(emu_run_status(0, 0, 0, 0, 0, 0), 3, "emulator never launched");
    want(emu_run_status(0, 1, 1, 100, 0, 0), 3, "deadline expired after output");
    want(emu_run_status(1, 1, 1, 100, 0, 0), 3, "timed out but marked ok");
    want(emu_run_status(0, 1, 0, 0, 0, 0), 3, "emulator ended with no serial output");
    /* Review F2: an emulator that died after partial output is no verdict. */
    want(emu_run_status(0, 1, 0, 100, 0, 1), 3, "emulator ended abnormally after output");
    want(emu_run_status(1, 1, 0, 100, 0, 1), 3, "abnormal end but marked ok");
    want(emu_run_status(0, 1, 0, 100, 1, 1), 1, "triple fault, then the emulator died");
    want(emu_ended_abnormally(0, 9, 0, 0), 1, "SIGKILL the harness did not send");
    want(emu_ended_abnormally(0, 11, 0, 0), 1, "SIGSEGV");
    want(emu_ended_abnormally(0, 15, 1, 0), 0, "the harness's own stop");
    want(emu_ended_abnormally(1, 0, 0, 0), 1, "could not be reaped");
    want(emu_ended_abnormally(0, 0, 0, 1), 1, "exit status that is no guest verdict");
    want(emu_ended_abnormally(0, 0, 0, 0), 0, "ordinary exit");
    if (argc == 2) {
        FILE *f;
        char line[200] = "";
        setenv("INITECH_INFRA_LOG", argv[1], 1);
        emu_record_infra("qemu_harness", 1, 1, 0, 5, 0);
        emu_record_infra("qemu_harness", 3, 1, 1, 5, 0);
        f = fopen(argv[1], "r");
        if (f == NULL || fgets(line, sizeof line, f) == NULL ||
            strcmp(line, "qemu_harness exit 3 (launched=1 timed_out=1 serial_len=5 abnormal_end=0)\n") != 0 ||
            fgets(line, sizeof line, f) != NULL) {
            printf("FAIL emu exit: infrastructure log holds exactly the incomplete run\n"); fails++;
        }
        if (f) fclose(f);
    }
    if (fails) return 1;
    printf("ok emu exit: incomplete runs exit 3 and are recorded; verdicts exit 0/1\n");
    return 0;
}
