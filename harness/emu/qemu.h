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
