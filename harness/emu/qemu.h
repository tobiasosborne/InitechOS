/*
 * qemu.h -- InitechOS QEMU oracle harness (public API)
 *
 * beads: initech-f2s ("QEMU harness: boot, serial capture, gdb stub,
 *        QMP screendump, triple-fault detect")
 * Ref:   PRD Sec 8 "Oracle & Test Infrastructure" -- the QEMU signals:
 *          gdb stub (-s -S) + symbols; QMP `screendump`; `-serial` printf;
 *          `-d int,guest_errors,cpu_reset`; triple-fault detect.
 *        CLAUDE.md Law 2 ("the oracle is the truth") -- a false-green
 *          harness is the worst possible outcome, so the result struct
 *          surfaces every failure mode explicitly.
 *        CLAUDE.md Rule 2 ("fail fast, fail loud").
 *        CLAUDE.md Rule 12 (ASCII-clean source).
 *
 * The harness launches `qemu-system-i386` on a freestanding guest binary
 * and returns structured debug signals. It is FACTORY code -- C only
 * (CLAUDE.md Law 3). It never drives gdb itself; it only wires the flags.
 *
 * Boot mechanism (per the multiboot1 spec): with `-kernel <elf>` QEMU's
 * built-in loader lands a multiboot1 ELF in 32-bit protected mode, A20 on,
 * flat segments -- so the self-test fixtures do NOT need the real
 * MBR/A20/GDT boot (that is a separate task: the tracer boot, M1).
 * A raw disk image can be booted instead via `disk_path`, for when the
 * real OS image exists.
 *
 * Ownership: the caller fills a QemuConfig (all string fields point to
 * caller-owned storage that must outlive the qemu_run call). qemu_run
 * fills a QemuResult whose owned heap buffers (serial_text) must be freed
 * with qemu_result_free. Path fields in QemuResult point into the result's
 * own fixed-size buffers; they are valid until qemu_result_free.
 */

#ifndef INITECH_HARNESS_EMU_QEMU_H
#define INITECH_HARNESS_EMU_QEMU_H

#include <stdbool.h>
#include <stddef.h>

/* Sane fixed bound for a constructed filesystem path. */
#define QEMU_PATH_MAX 1024

/*
 * QemuConfig -- how to launch the guest. Caller owns all pointers.
 *
 * Exactly one of kernel_path / disk_path selects the boot mechanism:
 *   - kernel_path != NULL : `-kernel <elf>` multiboot1 boot (the self-test
 *     and seed smoke path use this).
 *   - disk_path   != NULL : `-drive format=raw,file=<img>` raw-disk boot
 *     (for the real OS image; mutually exclusive with kernel_path).
 * If both are set kernel_path wins; if neither, qemu_run fails loudly.
 */
typedef struct {
    const char *kernel_path;   /* multiboot1 ELF for `-kernel` (or NULL).   */
    const char *disk_path;     /* raw disk image for `-drive` (or NULL).    */
    const char *data_disk_path;/* FAT12 data volume: a SECOND -drive on the
                                  IDE primary SLAVE (if=ide,index=1). NULL if
                                  not attached. The boot disk_path stays the
                                  primary master (index=0). Used so the kernel
                                  can mount a real filesystem over ATA.       */

    const char *output_dir;    /* dir for serial/log/qmp/ppm files; if NULL
                                  defaults to "build". Must already exist.  */
    const char *name;          /* short label for output filenames; if NULL
                                  defaults to "qemu". ASCII, no slashes.    */

    int timeout_ms;            /* wall-clock kill deadline; <=0 => default
                                  (QEMU_DEFAULT_TIMEOUT_MS). Critical:
                                  freestanding guests can hang.             */

    bool enable_gdb;           /* add `-s -S`: gdb stub on :1234, CPU halted
                                  at reset. Harness does NOT drive gdb.     */
    bool enable_qmp_screendump;/* open a QMP socket, handshake, screendump
                                  to <output_dir>/<name>.ppm, then quit.    */

    const char *expect_marker; /* if non-NULL, a substring asserted present
                                  in the captured serial text (sets
                                  result->marker_found).                    */

    /* QMP keystroke injection (beads initech-43b). When keys_spec is non-NULL,
     * the harness -- after the guest boots -- sends QMP `send-key` events over
     * the QMP socket so a key typed on the host reaches the guest keyboard
     * (IRQ1). The spec is a comma-separated list of qcode tokens: single
     * letters a-z / digits 0-9 map to their qcode; the words "ret"/"spc" map to
     * Return/Space (see keys_to_qcode in qemu.c for the full set). Requires the
     * QMP socket, so it implies enable_qmp_screendump's QMP plumbing -- if
     * keys_spec is set the harness opens the QMP socket even without a
     * screendump. */
    const char *keys_spec;     /* comma-separated qcode tokens, or NULL.     */
    /* If non-NULL, the harness waits until this substring appears on the
     * captured serial BEFORE injecting keys (the robust trigger: the guest
     * tells us it is ready). If NULL, a fixed startup delay is used instead.  */
    const char *keys_after;    /* serial marker to wait for, or NULL.        */

    /* Pure-screendump synchronisation (beads initech-3pe). When a screendump is
     * requested (enable_qmp_screendump) with NO keys, the harness otherwise
     * dumps the framebuffer the instant QMP connects -- a wall-clock race: under
     * a loaded host the guest may not have PAINTED yet, so the screendump is
     * blank and the ppm gate goes RED non-deterministically. If this marker is
     * non-NULL, the harness WAITS for the substring on the serial capture
     * (the guest signals paint-complete) BEFORE screendumping, removing the race
     * for guests that DO paint, just slower under load. The wall-clock timeout
     * remains the hard backstop: if the marker never appears within budget the
     * harness screendumps anyway (best-effort), so a guest that truly never
     * painted still fails HONESTLY (Law 2 -- fail loud, never false-green nor
     * hang). NULL => legacy immediate-dump behaviour. */
    const char *screendump_after; /* serial marker to wait for, or NULL.     */
} QemuConfig;

/* Default wall-clock timeout if config->timeout_ms <= 0. */
#define QEMU_DEFAULT_TIMEOUT_MS 5000

/*
 * QemuResult -- structured signals captured from the run.
 *
 * `ok` is the overall verdict: QEMU launched and reaped, did NOT time out,
 * reported no triple fault, no guest errors, and (if expect_marker was set)
 * the marker was found. It is the single boolean a gate should consult,
 * but every component is exposed so a gate can be precise.
 */
typedef struct {
    bool launched;             /* fork/exec of qemu succeeded.              */
    bool timed_out;            /* wall-clock deadline hit; qemu was killed. */
    int  exit_code;            /* qemu exit status (WEXITSTATUS), or -1 if
                                  killed/uncollectable.                     */
    int  term_signal;          /* signal that killed qemu, or 0.           */

    char *serial_text;         /* heap-owned NUL-terminated serial capture
                                  (may be ""); free via qemu_result_free.   */
    size_t serial_len;         /* bytes captured (excludes the NUL).        */
    char serial_path[QEMU_PATH_MAX];   /* file qemu wrote serial to.        */

    bool triple_fault;         /* a triple fault / unhandled reset detected
                                  in the -d log.                            */
    int  cpu_reset_count;      /* count of cpu_reset events in the log.     */
    int  guest_errors;         /* count of guest_errors lines in the log.   */
    char log_path[QEMU_PATH_MAX];      /* the -D debug log file.            */

    bool screendump_taken;     /* QMP screendump issued and file exists.    */
    char screendump_path[QEMU_PATH_MAX]; /* the .ppm (if screendump_taken). */

    bool marker_found;         /* expect_marker present in serial_text
                                  (false if expect_marker was NULL).        */

    int  keys_sent;            /* count of QMP send-key events issued (beads
                                  initech-43b); 0 if keys_spec was NULL.     */

    bool ok;                   /* overall verdict (see above).              */
} QemuResult;

/*
 * qemu_run -- launch QEMU per cfg, capture signals into out.
 *
 * Returns 0 if the harness machinery itself ran to completion (regardless
 * of whether the GUEST passed -- consult out->ok for that). Returns
 * non-zero only on harness-level failure (bad config, fork/exec failure,
 * unwritable output dir), in which case out is still zero-initialized and
 * partially filled where meaningful.
 *
 * Never hangs: a hung guest is killed at the timeout and reported via
 * out->timed_out. The caller must call qemu_result_free(out) afterwards.
 */
int qemu_run(const QemuConfig *cfg, QemuResult *out);

/* Free heap buffers owned by a QemuResult. Safe on a zeroed/partial result
 * and idempotent. */
void qemu_result_free(QemuResult *out);

#endif /* INITECH_HARNESS_EMU_QEMU_H */
