/*
 * bochs.h -- InitechOS Bochs oracle harness (public API)
 *
 * beads: initech-564 ("harness/emu/bochs.c driver + wire test-boot to
 *        dual-emulator differential"). The Bochs leg of the tri-emulator gate
 *        (CLAUDE.md Rule 5; PRD Sec 8). Parallel to qemu.h.
 * Ref:   bd memory bochs-boot-solved-initech-6pj (the WORKING bochsrc:
 *          legacy BIOS + pentium + the LGPL vgabios) and
 *          bochs-rfb-display-does-not-render-vga-mode (so this gate asserts on
 *          SERIAL markers + no-triple-fault, NOT a screendump).
 *        CLAUDE.md Law 2 (oracle is truth -- surface every failure mode),
 *          Law 3 (factory is C-only: the RFB unblock is implemented in C here,
 *          not the Python diagnosis helper), Rule 2 (fail loud), Rule 12.
 *
 * Mechanism (see bochs.c for detail): this Bochs is a DEBUGGER build, so we
 * pass `-rc <file>` containing `c` to auto-continue, and the only headless
 * display plugin (rfb) blocks ~30s for a VNC client -- so the harness opens a
 * TCP connection to the Bochs RFB server, completes the trivial RFB 3.3
 * handshake (security type None) IN C, and holds it (draining) for the run so
 * Bochs proceeds. Serial is captured via `com1: mode=file`. The guest
 * hlt-loops, so without quit_after the run times out -- expected (like the QEMU
 * tracer-boot gate); the verdict is driven by serial markers + fault scan, not
 * the exit code.
 *
 * Ownership mirrors qemu.h: caller fills BochsConfig (caller-owned strings that
 * must outlive bochs_run); bochs_run fills BochsResult whose heap buffer
 * (serial_text) is freed with bochs_result_free.
 */
#ifndef INITECH_HARNESS_EMU_BOCHS_H
#define INITECH_HARNESS_EMU_BOCHS_H

#include <stdbool.h>
#include <stddef.h>

#define BOCHS_PATH_MAX 1024

/* Default BIOS/vgabios for the WORKING Bochs config (bd memory
 * bochs-boot-solved-initech-6pj). These are the Debian/Ubuntu Bochs package
 * paths; a different distro may place them elsewhere -- override via the config
 * (Law 1: documented assumption, not a silent hardcode). The legacy 64K BIOS
 * is REQUIRED: BIOS-bochs-latest triple-faults on this Bochs 2.7 build. */
#define BOCHS_DEFAULT_BIOS    "/usr/share/bochs/BIOS-bochs-legacy"
#define BOCHS_DEFAULT_VGABIOS "/usr/share/bochs/VGABIOS-lgpl-latest"
#define BOCHS_DEFAULT_RFB_PORT 5900
/* The debugger build is SLOW; a full tracer boot to the last marker takes tens
 * of seconds of wall clock. Generous default (the guest hlt-loops so we always
 * run to the deadline). */
#define BOCHS_DEFAULT_TIMEOUT_MS 45000

typedef struct {
    const char *disk_path;     /* raw boot disk image (REQUIRED).            */
    const char *data_disk_path;/* optional IDE primary-slave raw data image. */
    const char *bios_path;     /* system BIOS ROM; NULL => BOCHS_DEFAULT_BIOS */
    const char *vgabios_path;  /* VGA BIOS ROM; NULL => BOCHS_DEFAULT_VGABIOS */

    const char *output_dir;    /* dir for bochsrc/serial/log files; NULL =>
                                  "build". Must already exist.              */
    const char *name;          /* label for output filenames; NULL => "bochs".
                                  ASCII, no slashes.                         */

    int timeout_ms;            /* wall-clock kill deadline; <=0 => default.  */
    int rfb_port;              /* Bochs RFB server port; <=0 => default.     */

    const char *expect_marker; /* if non-NULL, asserted present (line-exact,
                                  see bochs.c) in the serial capture.        */

    /* Completion-gated capture (initech-cqej; PRD Sec 8 and the independent
     * restart contract in spec/flair_power_traces.mk). Bochs already honors
     * hardware RESET without a QEMU-style -no-reboot switch. allow_reboot
     * explicitly opts into a multi-boot wait; it NEVER permits a triple
     * fault. quit_after is a COMPLETE exact serial line; count <=1 means 1.
     * No marker => original deadline-driven boot-leg behavior. */
    bool allow_reboot;
    const char *quit_after;
    int quit_after_count;

    /* RFB framebuffer capture (initech-u9gf; RFC 6143 Sec 7.6.1). When the
     * COMPLETE exact serial line screendump_after arrives, the harness asks
     * the Bochs RFB server for full-window FramebufferUpdates until two
     * consecutive full frames agree, writes <name>.rfb.pgm (the raw server
     * pixels: Bochs serves BGR233, rfb.cc BGR233Format) and <name>.rfb.ppm
     * (the same codes widened to RGB for the Law-4 eyeball), then clicks
     * Bochs's header-bar snapshot button for the LOSSLESS twin
     * (<name>.snap.bmp -> <name>.snap.ppm, exact guest RGB) and stops the
     * run. Bochs runs in output_dir for this (snapshot.bmp is cwd-relative),
     * so every bochsrc path is made absolute. NULL => no capture (the
     * original boot-leg behavior). */
    const char *screendump_after;

    /* Serial-gated RFB key replay for ADR-0009 DEC-08 (initech-hdlb).
     * Comma-separated lowercase letters/digits, ret, spc, dot; both press
     * and release are sent. Capture waits for the last release. Ref:
     * docs/research/samir-bochs-rfb.md (RFC 6143 Sec 7.5.4). */
    const char *keys_script;
    const char *keys_after;

    /* Pinned guest RTC (Rule 11; parallel to qemu.h rtc_base): >0 writes
     * bochsrc `clock: time0=N` and runs Bochs with TZ=UTC, because Bochs
     * 2.7 cmos.cc localtime()s a numeric time0. 0 => Bochs default (host
     * local time), the original behavior. */
    long rtc_time0;

    /* `vga: extension=` value; NULL => "vbe" (the LGPL vgabios VBE path).
     * "cirrus" with no PCI slot is the ISA CL-GD54xx (Bochs 2.7 svga_cirrus). */
    const char *vga_extension;
} BochsConfig;

typedef struct {
    bool launched;             /* fork/exec of bochs succeeded.              */
    bool rfb_unblocked;        /* RFB handshake completed (Bochs proceeded). */
    bool timed_out;            /* deadline hit (EXPECTED: guest hlt-loops).  */
    int  exit_code;            /* bochs WEXITSTATUS, or -1 if killed.        */
    int  term_signal;          /* signal that killed bochs, or 0.            */

    char *serial_text;         /* heap-owned NUL-terminated serial capture.  */
    size_t serial_len;
    char serial_path[BOCHS_PATH_MAX];
    char log_path[BOCHS_PATH_MAX];     /* bochs stdout/stderr log.           */
    char bochsrc_path[BOCHS_PATH_MAX]; /* the generated bochsrc.             */

    bool triple_fault;         /* "3rd (13) exception" or "IDT.limit = 0x0"
                                  seen in the bochs log (a triple fault).    */
    bool marker_found;         /* expect_marker present (false if NULL).     */
    bool quit_marker_found;    /* requested completion count reached.        */
    int quit_markers_seen;     /* complete exact quit_after lines captured.  */

    bool screendump_marker_found; /* screendump_after line seen.            */
    bool screendump_ok;        /* a stable full RFB frame was written.       */
    unsigned rfb_width, rfb_height;    /* ServerInit window geometry.        */
    unsigned long rfb_full_frames;     /* full-window raw rects received.     */
    char rfb_error[160];       /* first RFB protocol violation, or "".       */
    char rfb_pgm_path[BOCHS_PATH_MAX];
    char rfb_ppm_path[BOCHS_PATH_MAX];
    /* Lossless twin of the RFB frame: Bochs's own header-bar "snapshot"
     * button (gui.cc snapshot_handler) writes the guest's raw indices plus
     * the emulated DAC palette as an 8bpp BMP; converted to exact RGB P6. */
    bool snapshot_ok;
    int snapshot_tries;
    char snap_bmp_path[BOCHS_PATH_MAX];
    char snap_ppm_path[BOCHS_PATH_MAX];

    bool ok;                   /* launched && rfb_unblocked && !triple_fault
                                  && (expect_marker ? marker_found : true)
                                  && (quit_after ? quit_marker_found : true)
                                  && (screendump_after ? screendump_ok &&
                                                         snapshot_ok : true).
                                  NOT gated on timed_out (hlt-loop).         */
} BochsResult;

/* Launch Bochs per cfg, capture signals into out. Returns 0 if the harness ran
 * to completion (consult out->ok for the guest verdict); non-zero only on
 * harness-level failure (bad config, unwritable dir, fork/exec failure). Never
 * hangs: a hung/looping guest is killed at the timeout. Caller must
 * bochs_result_free(out) afterwards. */
int bochs_run(const BochsConfig *cfg, BochsResult *out);

/* Free heap buffers owned by a BochsResult. Safe on a zeroed result, idempotent. */
void bochs_result_free(BochsResult *out);

#endif /* INITECH_HARNESS_EMU_BOCHS_H */
