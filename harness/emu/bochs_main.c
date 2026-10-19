/*
 * bochs_main.c -- CLI wrapper around the Bochs oracle harness (initech-564).
 *
 * Exit status mirrors the oracle verdict (usable directly as a make gate):
 *   0  -> result.ok (launched, RFB unblocked, no triple fault, and the
 *         expected marker -- if any -- was found on serial)
 *   1  -> the guest run failed an expectation (a real RED signal)
 *   2  -> harness-level / usage error
 *
 * Ref: CLAUDE.md Law 2 (oracle is truth), Rule 2, Rule 12. Parallel to
 * qemu_main.c.
 *
 * Usage:
 *   bochs_harness --disk IMG [--disk2 IMG] [--expect MARKER] [--bios ROM] [--vgabios ROM]
 *                 [--timeout-ms N] [--name LABEL] [--out DIR] [--serial-stdout]
 *                 [--screendump-after MARKER] [--rtc-time0 N] [--vga-ext EXT]
 */
#include "bochs.h"

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *argv0)
{
    fprintf(stderr,
        "usage: %s --disk IMG [options]\n"
        "  --disk2 IMG        attach a raw disk as IDE primary slave\n"
        "  --expect MARKER    assert MARKER present (line-exact) on serial\n"
        "  --quit-after MARK  stop after this complete exact serial line\n"
        "  --quit-after-count N  wait for N occurrences (default 1)\n"
        "  --allow-reboot     opt into waiting across hardware RESET; required\n"
        "                     for count >1; triple faults always fail\n"
        "  --bios ROM         system BIOS (default %s)\n"
        "  --vgabios ROM      VGA BIOS  (default %s)\n"
        "  --timeout-ms N     wall-clock kill deadline (default %d)\n"
        "  --rfb-port N       Bochs RFB port (default %d)\n"
        "  --name LABEL       output filename label (default \"bochs\")\n"
        "  --out DIR          output dir (default \"build\")\n"
        "  --serial-stdout    echo captured serial to stdout\n"
        "  --screendump-after MARK  after this complete exact serial line,\n"
        "                     capture a stable full RFB frame to\n"
        "                     <out>/<name>.rfb.pgm (BGR233) + .rfb.ppm, then\n"
        "                     click Bochs's snapshot button for the lossless\n"
        "                     <name>.snap.bmp -> <name>.snap.ppm, then stop\n"
        "  --rtc-time0 N      pin the guest RTC to Unix time N (UTC)\n"
        "  --vga-ext EXT      bochsrc vga: extension= (default vbe)\n",
        argv0, BOCHS_DEFAULT_BIOS, BOCHS_DEFAULT_VGABIOS,
        BOCHS_DEFAULT_TIMEOUT_MS, BOCHS_DEFAULT_RFB_PORT);
}

int main(int argc, char **argv)
{
    BochsConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    int serial_stdout = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
#define NEED_ARG()                                            \
    do {                                                      \
        if (i + 1 >= argc) {                                  \
            fprintf(stderr, "%s: %s needs an argument\n", argv[0], a); \
            return 2;                                         \
        }                                                     \
    } while (0)
        if (strcmp(a, "--disk") == 0)            { NEED_ARG(); cfg.disk_path = argv[++i]; }
        else if (strcmp(a, "--disk2") == 0)      { NEED_ARG(); cfg.data_disk_path = argv[++i]; }
        else if (strcmp(a, "--expect") == 0)     { NEED_ARG(); cfg.expect_marker = argv[++i]; }
        else if (strcmp(a, "--quit-after") == 0) { NEED_ARG(); cfg.quit_after = argv[++i]; }
        else if (strcmp(a, "--allow-reboot") == 0) { cfg.allow_reboot = true; }
        else if (strcmp(a, "--quit-after-count") == 0) {
            NEED_ARG();
            char *end;
            errno = 0;
            long n = strtol(argv[++i], &end, 10);
            if (errno || *end || n < 1 || n > INT_MAX) {
                fprintf(stderr, "%s: --quit-after-count needs a positive integer\n", argv[0]);
                return 2;
            }
            cfg.quit_after_count = (int)n;
        }
        else if (strcmp(a, "--bios") == 0)       { NEED_ARG(); cfg.bios_path = argv[++i]; }
        else if (strcmp(a, "--vgabios") == 0)    { NEED_ARG(); cfg.vgabios_path = argv[++i]; }
        else if (strcmp(a, "--timeout-ms") == 0) { NEED_ARG(); cfg.timeout_ms = atoi(argv[++i]); }
        else if (strcmp(a, "--rfb-port") == 0)   { NEED_ARG(); cfg.rfb_port = atoi(argv[++i]); }
        else if (strcmp(a, "--name") == 0)       { NEED_ARG(); cfg.name = argv[++i]; }
        else if (strcmp(a, "--out") == 0)        { NEED_ARG(); cfg.output_dir = argv[++i]; }
        else if (strcmp(a, "--serial-stdout") == 0) { serial_stdout = 1; }
        else if (strcmp(a, "--screendump-after") == 0) { NEED_ARG(); cfg.screendump_after = argv[++i]; }
        else if (strcmp(a, "--vga-ext") == 0)    { NEED_ARG(); cfg.vga_extension = argv[++i]; }
        else if (strcmp(a, "--rtc-time0") == 0) {
            NEED_ARG();
            char *end;
            errno = 0;
            long t = strtol(argv[++i], &end, 10);
            if (errno || *end || t < 1) {
                fprintf(stderr, "%s: --rtc-time0 needs a positive Unix time\n", argv[0]);
                return 2;
            }
            cfg.rtc_time0 = t;
        }
        else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) { usage(argv[0]); return 2; }
        else { fprintf(stderr, "%s: unknown argument: %s\n", argv[0], a); usage(argv[0]); return 2; }
#undef NEED_ARG
    }

    if (!cfg.disk_path) {
        fprintf(stderr, "%s: need --disk\n", argv[0]);
        usage(argv[0]);
        return 2;
    }
    if ((cfg.quit_after_count && !cfg.quit_after) ||
        (cfg.quit_after_count > 1 && !cfg.allow_reboot)) {
        fprintf(stderr, "%s: --quit-after-count needs --quit-after (and --allow-reboot for count >1)\n", argv[0]);
        return 2;
    }

    BochsResult res;
    if (bochs_run(&cfg, &res) != 0) {
        fprintf(stderr, "%s: harness failure\n", argv[0]);
        bochs_result_free(&res);
        return 2;
    }

    if (serial_stdout) {
        fputs(res.serial_text, stdout);
        if (res.serial_len > 0 && res.serial_text[res.serial_len - 1] != '\n')
            fputc('\n', stdout);
    }

    fprintf(stderr,
        "[bochs] launched=%d rfb_unblocked=%d timed_out=%d exit=%d signal=%d\n"
        "[bochs] serial_len=%zu marker_found=%d (expect=%s)\n"
        "[bochs] triple_fault=%d\n"
        "[bochs] quit_marker_found=%d quit_markers_seen=%d (want=%d, quit_after=%s)\n"
        "[bochs] serial=%s\n"
        "[bochs] screendump_after=%s marker_found=%d screendump_ok=%d rfb=%ux%u full_frames=%lu rfb_error=%s\n"
        "[bochs] rfb_pgm=%s\n"
        "[bochs] snapshot_ok=%d snapshot_tries=%d snap_ppm=%s\n"
        "[bochs] OK=%d\n",
        res.launched, res.rfb_unblocked, res.timed_out, res.exit_code, res.term_signal,
        res.serial_len, res.marker_found,
        cfg.expect_marker ? cfg.expect_marker : "(none)",
        res.triple_fault, res.quit_marker_found, res.quit_markers_seen,
        cfg.quit_after_count > 0 ? cfg.quit_after_count : 1,
        cfg.quit_after ? cfg.quit_after : "(none)", res.serial_path,
        cfg.screendump_after ? cfg.screendump_after : "(none)",
        res.screendump_marker_found, res.screendump_ok, res.rfb_width,
        res.rfb_height, res.rfb_full_frames,
        res.rfb_error[0] ? res.rfb_error : "(none)",
        res.screendump_ok ? res.rfb_pgm_path : "(none)",
        res.snapshot_ok, res.snapshot_tries,
        res.snapshot_ok ? res.snap_ppm_path : "(none)", res.ok);

    int rc = res.ok ? 0 : 1;
    bochs_result_free(&res);
    return rc;
}
