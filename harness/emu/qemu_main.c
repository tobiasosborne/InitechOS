/*
 * qemu_main.c -- CLI wrapper around the QEMU oracle harness
 *
 * beads: initech-f2s ("QEMU harness ...").
 * Ref:   PRD Sec 8 (the QEMU signals); CLAUDE.md Law 2 (oracle is truth),
 *        Rule 2 (fail fast/loud), Rule 12 (ASCII-clean).
 *
 * Manual driver for the harness. Exit status mirrors the oracle verdict:
 *   0  -> result.ok (launched, no timeout, no triple fault, no guest
 *         errors, and the expected marker -- if any -- was found)
 *   1  -> the guest run failed an expectation (a real RED signal)
 *   2  -> harness-level / usage error
 *
 * This makes the wrapper usable directly as a gate from make/shell.
 *
 * Usage:
 *   qemu_harness --kernel <elf> [--expect "MARKER"] [--screendump]
 *                [--gdb] [--timeout-ms N] [--name LABEL] [--out DIR]
 *   qemu_harness --disk <img.raw> [...]
 */

#include "qemu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s (--kernel ELF | --disk IMG) [options]\n"
        "  --expect MARKER    assert MARKER substring present on serial\n"
        "  --screendump       QMP screendump to <out>/<name>.ppm\n"
        "  --gdb              add -s -S (gdb stub on :1234, halted)\n"
        "  --timeout-ms N     wall-clock kill deadline (default %d)\n"
        "  --name LABEL       output filename label (default \"qemu\")\n"
        "  --out DIR          output dir (default \"build\")\n"
        "  --serial-stdout    echo captured serial text to stdout\n",
        argv0, QEMU_DEFAULT_TIMEOUT_MS);
}

int main(int argc, char **argv)
{
    QemuConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    int serial_stdout = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
#define NEED_ARG()                                            \
    do {                                                      \
        if (i + 1 >= argc) {                                  \
            fprintf(stderr, "%s: %s needs an argument\n",     \
                    argv[0], a);                              \
            return 2;                                         \
        }                                                     \
    } while (0)
        if (strcmp(a, "--kernel") == 0) {
            NEED_ARG();
            cfg.kernel_path = argv[++i];
        } else if (strcmp(a, "--disk") == 0) {
            NEED_ARG();
            cfg.disk_path = argv[++i];
        } else if (strcmp(a, "--expect") == 0) {
            NEED_ARG();
            cfg.expect_marker = argv[++i];
        } else if (strcmp(a, "--screendump") == 0) {
            cfg.enable_qmp_screendump = true;
        } else if (strcmp(a, "--gdb") == 0) {
            cfg.enable_gdb = true;
        } else if (strcmp(a, "--timeout-ms") == 0) {
            NEED_ARG();
            cfg.timeout_ms = atoi(argv[++i]);
        } else if (strcmp(a, "--name") == 0) {
            NEED_ARG();
            cfg.name = argv[++i];
        } else if (strcmp(a, "--out") == 0) {
            NEED_ARG();
            cfg.output_dir = argv[++i];
        } else if (strcmp(a, "--serial-stdout") == 0) {
            serial_stdout = 1;
        } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage(argv[0]);
            return 2;
        } else {
            fprintf(stderr, "%s: unknown argument: %s\n", argv[0], a);
            usage(argv[0]);
            return 2;
        }
#undef NEED_ARG
    }

    if (!cfg.kernel_path && !cfg.disk_path) {
        fprintf(stderr, "%s: need --kernel or --disk\n", argv[0]);
        usage(argv[0]);
        return 2;
    }

    QemuResult res;
    if (qemu_run(&cfg, &res) != 0) {
        fprintf(stderr, "%s: harness failure\n", argv[0]);
        qemu_result_free(&res);
        return 2;
    }

    if (serial_stdout) {
        fputs(res.serial_text, stdout);
        if (res.serial_len > 0 && res.serial_text[res.serial_len - 1] != '\n') {
            fputc('\n', stdout);
        }
    }

    /* Report card to stderr so --serial-stdout stays clean. */
    fprintf(stderr,
        "[harness] launched=%d timed_out=%d exit=%d signal=%d\n"
        "[harness] serial_len=%zu marker_found=%d (expect=%s)\n"
        "[harness] triple_fault=%d cpu_reset=%d guest_errors=%d\n"
        "[harness] screendump_taken=%d path=%s\n"
        "[harness] OK=%d\n",
        res.launched, res.timed_out, res.exit_code, res.term_signal,
        res.serial_len, res.marker_found,
        cfg.expect_marker ? cfg.expect_marker : "(none)",
        res.triple_fault, res.cpu_reset_count, res.guest_errors,
        res.screendump_taken,
        res.screendump_taken ? res.screendump_path : "(none)",
        res.ok);

    int rc = res.ok ? 0 : 1;
    qemu_result_free(&res);
    return rc;
}
