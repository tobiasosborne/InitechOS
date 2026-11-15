/* Host-only seam for factory/tests/emu-cli.sh (review F2): the real
 * qemu_main.c / bochs_main.c linked to a scripted emulator result. No
 * emulator is launched. EMU_STUB selects how the scripted run ended. */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifdef EMU_CLI_BOCHS
#include "bochs.h"
#define RESULT BochsResult
#else
#include "qemu.h"
#define RESULT QemuResult
#endif

static void script(RESULT *out)
{
    const char *k = getenv("EMU_STUB");
    if (!k) k = "";
    memset(out, 0, sizeof *out);
    out->launched = true;
#ifdef EMU_CLI_BOCHS
    out->reaped = true;
    out->rfb_unblocked = true;
#endif
    out->serial_text = strdup(strcmp(k, "complete") == 0 ? "BOOTING\nREADY\n" : "BOOTING\n");
    out->serial_len = strlen(out->serial_text);
    if (!strcmp(k, "complete")) {
        out->ok = out->marker_found = true;
#ifdef EMU_CLI_BOCHS
        out->quit_marker_found = out->harness_stopped = out->shutdown_requested = true;
        out->exit_code = -1; out->term_signal = SIGTERM;
#else
        out->quit_marker_found = true;
#endif
    } else if (!strcmp(k, "verdict")) {          /* ran to the end, no READY */
#ifdef EMU_CLI_BOCHS
        out->exit_code = 0;
#else
        out->exit_code = 1;                       /* isa-debug-exit 0 */
#endif
    } else if (!strcmp(k, "sigkill") || !strcmp(k, "sigsegv")) {
        out->exit_code = -1;
        out->term_signal = !strcmp(k, "sigkill") ? SIGKILL : SIGSEGV;
    } else if (!strcmp(k, "triple")) {
        out->exit_code = -1; out->term_signal = SIGABRT; out->triple_fault = true;
    } else if (!strcmp(k, "badexit")) {
#ifdef EMU_CLI_BOCHS
        out->exit_code = 1;                       /* a Bochs panic */
#else
        out->exit_code = 2;                       /* no guest verdict is even */
#endif
    } else if (!strcmp(k, "stopped")) {           /* our own stop, no READY */
        out->harness_stopped = true; out->exit_code = -1; out->term_signal = SIGTERM;
    }
#ifndef EMU_CLI_BOCHS
    else if (!strcmp(k, "qmpfail")) {     /* QMP channel died; we stopped it */
        out->harness_stopped = true; out->exit_code = -1; out->term_signal = SIGTERM;
        out->qmp_infra = true;
    }
#endif
}

#ifdef EMU_CLI_BOCHS
int bochs_run(const BochsConfig *cfg, BochsResult *out) { (void)cfg; script(out); return 0; }
void bochs_result_free(BochsResult *out) { free(out->serial_text); out->serial_text = NULL; }
#else
int qemu_run(const QemuConfig *cfg, QemuResult *out) { (void)cfg; script(out); return 0; }
void qemu_result_free(QemuResult *out) { free(out->serial_text); out->serial_text = NULL; }
#endif
