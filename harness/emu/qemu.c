/*
 * qemu.c -- InitechOS QEMU oracle harness (implementation)
 *
 * beads: initech-f2s ("QEMU harness: boot, serial capture, gdb stub,
 *        QMP screendump, triple-fault detect")
 * Ref:   PRD Sec 8 "Oracle & Test Infrastructure": gdb stub (-s -S);
 *          QMP `screendump`; `-serial` printf; `-d int,guest_errors,
 *          cpu_reset`; triple-fault detect. `-no-reboot` makes a triple
 *          fault exit QEMU instead of looping (CLAUDE.md hallucination
 *          callout: "A triple-fault in QEMU silently reboots").
 *        CLAUDE.md Law 2 (oracle is truth; never false-green),
 *          Rule 2 (fail fast/loud), Rule 12 (ASCII-clean).
 *
 * FACTORY code, C11. POSIX.1-2008 used for fork/exec/waitpid, AF_UNIX
 * sockets, nanosleep, clock_gettime.
 *
 * Design notes / decisions:
 *  - Timeout mechanism: we fork/exec qemu, then poll waitpid(WNOHANG) in a
 *    short nanosleep loop against a CLOCK_MONOTONIC deadline. On expiry we
 *    SIGTERM, give a brief grace window, then SIGKILL, and always reap.
 *    This is simpler and more portable than SIGALRM/timer_create and never
 *    leaves a zombie or a runaway qemu. (The harness must never itself
 *    hang -- a hung guest is the common case for freestanding code.)
 *  - QMP handshake: connect AF_UNIX, read the greeting, send
 *    {"execute":"qmp_capabilities"}, then {"execute":"screendump",...},
 *    then {"execute":"quit"}. We do not parse JSON structurally; we drive
 *    the (line-oriented) protocol and check for "error" / file existence.
 *    QMP server is started with server=on,wait=off so qemu does not block
 *    waiting for us if we never connect (e.g. it triple-faults first).
 */

#define _POSIX_C_SOURCE 200809L

#include "qemu.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* small helpers                                                       */
/* ------------------------------------------------------------------ */

#define QEMU_BIN "qemu-system-i386"
#define MAX_ARGV 48

/* Compose <dir>/<name><suffix> into dst (bounded). Returns 0 on success. */
static int join_path(char *dst, size_t cap, const char *dir,
                     const char *name, const char *suffix)
{
    int n = snprintf(dst, cap, "%s/%s%s", dir, name, suffix);
    if (n < 0 || (size_t)n >= cap) {
        return -1;
    }
    return 0;
}

/* Monotonic milliseconds since an arbitrary epoch. */
static long long mono_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Sleep for ms milliseconds (best-effort, restart-safe). */
static void sleep_ms(int ms)
{
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&req, &req) != 0 && errno == EINTR) {
        /* finish the remaining interval */
    }
}

/* Read an entire file into a freshly malloc'd NUL-terminated buffer.
 * Returns the buffer (caller frees) and sets *len, or NULL on error. */
static char *read_file(const char *path, size_t *len)
{
    *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    size_t cap = 4096, used = 0;
    char *buf = malloc(cap);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    for (;;) {
        if (used + 4096 + 1 > cap) {
            size_t ncap = cap * 2;
            char *nb = realloc(buf, ncap);
            if (!nb) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = nb;
            cap = ncap;
        }
        size_t got = fread(buf + used, 1, 4096, f);
        used += got;
        if (got < 4096) {
            if (ferror(f)) {
                free(buf);
                fclose(f);
                return NULL;
            }
            break;
        }
    }
    fclose(f);
    buf[used] = '\0';
    *len = used;
    return buf;
}

/* Count non-overlapping occurrences of needle in haystack (which may
 * contain embedded NULs is irrelevant here -- we treat it as a C string). */
static int count_substr(const char *haystack, const char *needle)
{
    int n = 0;
    size_t nl = strlen(needle);
    if (nl == 0) {
        return 0;
    }
    const char *p = haystack;
    while ((p = strstr(p, needle)) != NULL) {
        n++;
        p += nl;
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* QMP screendump driver                                               */
/* ------------------------------------------------------------------ */

/* Send a NUL-terminated command over fd, appending nothing (caller
 * includes the trailing newline). Returns 0 on success. */
static int qmp_send(int fd, const char *cmd)
{
    size_t left = strlen(cmd);
    const char *p = cmd;
    long long deadline = mono_ms() + 1000;
    while (left > 0) {
        if (mono_ms() >= deadline) {
            return -1; /* bounded: never block forever on a wedged peer */
        }
        ssize_t w = write(fd, p, left);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd = {fd, POLLOUT, 0};
                poll(&pfd, 1, 100);
                continue;
            }
            return -1;
        }
        p += w;
        left -= (size_t)w;
    }
    return 0;
}

/* Drain whatever QMP has sent so far into a small scratch buffer, using a
 * poll-bounded wait so it can NEVER block indefinitely (the original sin
 * that wedged the harness). fd must be non-blocking. We do not parse JSON
 * structurally; we just keep the socket flowing and let qemu act on the
 * commands. Returns when budget_ms elapses or the peer closes. */
static void qmp_drain(int fd, int budget_ms)
{
    long long deadline = mono_ms() + budget_ms;
    char scratch[2048];
    for (;;) {
        long long now = mono_ms();
        int remaining = (int)(deadline - now);
        if (remaining <= 0) {
            return;
        }
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, remaining);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (pr == 0) {
            return; /* budget elapsed with no more data */
        }
        ssize_t r = read(fd, scratch, sizeof(scratch));
        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return;
        }
        if (r == 0) {
            return; /* peer closed */
        }
        /* keep reading until poll reports no more within budget */
    }
}

/* ------------------------------------------------------------------ */
/* QMP keystroke injection (beads initech-43b)                         */
/* ------------------------------------------------------------------ */

/*
 * Map ONE key token to its QMP qcode name (the QMP "key value qcode"
 * vocabulary). Returns the qcode string for a recognized token, or NULL.
 * Supported (sufficient to type letters + digits + Return + Space, per the
 * bead): single chars a-z (lowercased A-Z too) and 0-9 -> the same character;
 * the words "ret"/"enter"/"return" -> "ret"; "spc"/"space" -> "spc". This is
 * the small qcode set the bead asks for; extend here as gates need more keys.
 * Ref: QMP `send-key` command + the QKeyCode enum (qapi/ui.json: a..z, 0..9,
 * ret, spc are all valid qcode names).
 */
static const char *token_to_qcode(const char *tok)
{
    /* Single-character tokens: a-z, A-Z (folded to lower), 0-9. The qcode name
     * for a letter/digit IS that character, so we return a per-call static
     * 2-byte buffer. Not re-entrant, but the caller uses it immediately. */
    if (tok[0] != '\0' && tok[1] == '\0') {
        char c = tok[0];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            static char one[2];
            one[0] = c;
            one[1] = '\0';
            return one;
        }
        return NULL;
    }
    if (strcmp(tok, "ret") == 0 || strcmp(tok, "enter") == 0 ||
        strcmp(tok, "return") == 0) {
        return "ret";
    }
    if (strcmp(tok, "spc") == 0 || strcmp(tok, "space") == 0) {
        return "spc";
    }
    return NULL;
}

/*
 * Send one QMP `send-key` event for a single qcode (press+release of one key).
 * Format: {"execute":"send-key","arguments":{"keys":[{"type":"qcode",
 * "data":"<qcode>"}]}}. Returns 0 on a successful write.
 */
static int qmp_send_key(int fd, const char *qcode)
{
    char cmd[160];
    int n = snprintf(cmd, sizeof(cmd),
                     "{\"execute\":\"send-key\",\"arguments\":{\"keys\":"
                     "[{\"type\":\"qcode\",\"data\":\"%s\"}]}}\n", qcode);
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        return -1;
    }
    return qmp_send(fd, cmd);
}

/*
 * Inject every key in keys_spec (comma-separated tokens) over an already-
 * handshaked QMP fd. Unknown tokens are skipped with a stderr note (Law 2: be
 * loud about a silently-ignored key). Returns the number of keys sent; *bad is
 * set to the count of unrecognized tokens.
 */
static int qmp_inject_keys(int fd, const char *keys_spec, int *bad)
{
    int sent = 0;
    int unknown = 0;
    char buf[256];
    /* Copy so we can tokenize in place. Oversized specs are truncated loudly. */
    size_t L = strlen(keys_spec);
    if (L >= sizeof(buf)) {
        fprintf(stderr, "[harness] --keys spec too long (max %zu); truncating\n",
                sizeof(buf) - 1);
        L = sizeof(buf) - 1;
    }
    memcpy(buf, keys_spec, L);
    buf[L] = '\0';

    char *save = NULL;
    for (char *tok = strtok_r(buf, ",", &save); tok != NULL;
         tok = strtok_r(NULL, ",", &save)) {
        /* trim surrounding spaces */
        while (*tok == ' ') {
            tok++;
        }
        size_t tl = strlen(tok);
        while (tl > 0 && tok[tl - 1] == ' ') {
            tok[--tl] = '\0';
        }
        if (tok[0] == '\0') {
            continue;
        }
        const char *qc = token_to_qcode(tok);
        if (!qc) {
            fprintf(stderr, "[harness] --keys: unknown token '%s' (skipped)\n",
                    tok);
            unknown++;
            continue;
        }
        if (qmp_send_key(fd, qc) == 0) {
            sent++;
            /* Drain any QMP reply + give the guest a beat to take the IRQ1
             * before the next key (keeps the injection deterministic). */
            qmp_drain(fd, 30);
        }
    }
    if (bad) {
        *bad = unknown;
    }
    return sent;
}

/*
 * Wait until `marker` appears in the file at serial_path, or budget_ms
 * elapses. Re-reads the (growing) serial capture file in a short poll loop.
 * Returns 1 if the marker was seen, 0 on timeout. Used as the robust "guest is
 * ready" trigger for key injection (beads initech-43b: --keys-after).
 */
static int wait_for_serial_marker(const char *serial_path, const char *marker,
                                  int budget_ms)
{
    long long deadline = mono_ms() + budget_ms;
    for (;;) {
        size_t len = 0;
        char *txt = read_file(serial_path, &len);
        if (txt) {
            int hit = strstr(txt, marker) != NULL;
            free(txt);
            if (hit) {
                return 1;
            }
        }
        if (mono_ms() >= deadline) {
            return 0;
        }
        sleep_ms(25);
    }
}

/*
 * Connect to the QMP unix socket, do the capabilities handshake, request a
 * screendump to ppm_path, then quit. Retries the connect briefly because
 * qemu may not have created the socket the instant we return from fork.
 * Returns 0 if the session completed (screendump-file existence + the
 * keys-sent count are checked by / reported through the caller).
 *
 * Generalized for beads initech-43b: it now also injects keystrokes. After the
 * capabilities handshake it optionally (1) waits for keys_after on the serial
 * capture (else a fixed delay), (2) injects keys_spec via QMP send-key, then
 * (3) screendumps if ppm_path != NULL, then quits. ppm_path may be NULL (keys
 * only); keys_spec may be NULL (screendump only) -- at least one is set by the
 * caller. *keys_sent (if non-NULL) receives the count of keys injected.
 */
static int qmp_session(const char *sock_path, const char *ppm_path,
                       const char *keys_spec, const char *keys_after,
                       const char *serial_path, int *keys_sent)
{
    if (keys_sent) {
        *keys_sent = 0;
    }
    int fd = -1;
    long long deadline = mono_ms() + 3000;
    while (mono_ms() < deadline) {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            return -1;
        }
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        if (strlen(sock_path) >= sizeof(addr.sun_path)) {
            close(fd);
            return -1;
        }
        strcpy(addr.sun_path, sock_path);
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            break;
        }
        close(fd);
        fd = -1;
        sleep_ms(50);
    }
    if (fd < 0) {
        return -1;
    }

    /* Make the socket non-blocking so qmp_drain's poll loop fully bounds
     * every read -- a blocking read here is what previously hung the whole
     * harness (Law 2: never let the harness itself hang). */
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl >= 0) {
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    }

    /* 1. read greeting */
    qmp_drain(fd, 300);
    /* 2. enter command mode */
    if (qmp_send(fd, "{\"execute\":\"qmp_capabilities\"}\n") != 0) {
        close(fd);
        return -1;
    }
    qmp_drain(fd, 300);

    /* 3. keystroke injection (beads initech-43b). Trigger timing: if keys_after
     * is set, WAIT for that substring on the serial capture (the guest tells us
     * it is ready -- the robust, deterministic choice); else fall back to a
     * fixed 500 ms startup delay. Then inject keys_spec via QMP send-key. */
    if (keys_spec && keys_spec[0] != '\0') {
        if (keys_after && keys_after[0] != '\0') {
            if (!wait_for_serial_marker(serial_path, keys_after, 4000)) {
                fprintf(stderr,
                        "[harness] --keys-after marker '%s' not seen before "
                        "inject deadline; injecting anyway\n", keys_after);
            }
        } else {
            sleep_ms(500);
        }
        int bad = 0;
        int sent = qmp_inject_keys(fd, keys_spec, &bad);
        if (keys_sent) {
            *keys_sent = sent;
        }
        /* Let the last keystroke's IRQ1 + the guest's echo flush to serial
         * before we (optionally) screendump and quit. */
        qmp_drain(fd, 100);
    }

    /* 4. screendump (PPM), if requested. filename must be JSON-escaped; our
     * paths are plain ASCII under build/ with no quotes/backslashes, so a
     * direct embed is safe and reproducible. */
    if (ppm_path) {
        char cmd[QEMU_PATH_MAX + 64];
        int n = snprintf(cmd, sizeof(cmd),
                         "{\"execute\":\"screendump\",\"arguments\":"
                         "{\"filename\":\"%s\"}}\n", ppm_path);
        if (n < 0 || (size_t)n >= sizeof(cmd)) {
            close(fd);
            return -1;
        }
        if (qmp_send(fd, cmd) != 0) {
            close(fd);
            return -1;
        }
        qmp_drain(fd, 500);
    }

    /* 5. quit cleanly so the guest does not have to be killed. */
    qmp_send(fd, "{\"execute\":\"quit\"}\n");
    qmp_drain(fd, 300);
    close(fd);
    return 0;
}

/* ------------------------------------------------------------------ */
/* argv construction                                                   */
/* ------------------------------------------------------------------ */

/*
 * Build the qemu argv. Returns the argument count, or -1 on overflow.
 * All strings either are literals or point into caller-owned / *_path
 * buffers that outlive the exec. The exact command line emitted is:
 *
 *   qemu-system-i386 -display none -no-reboot
 *     -device isa-debug-exit,iobase=0xF4,iosize=0x04
 *     -serial file:<serial>
 *     -d int,guest_errors,cpu_reset -D <log>
 *     [-kernel <elf> | -drive format=raw,file=<img>]
 *     [-s -S]                              (if enable_gdb)
 *     [-qmp unix:<sock>,server=on,wait=off]  (if enable_qmp_screendump)
 */
static int build_argv(const QemuConfig *cfg, char **argv,
                      const char *serial_path, const char *log_path,
                      const char *sock_path)
{
    int i = 0;
#define PUSH(s)                          \
    do {                                 \
        if (i >= MAX_ARGV - 1) {         \
            return -1;                   \
        }                                \
        argv[i++] = (char *)(s);         \
    } while (0)

    PUSH(QEMU_BIN);
    /* Headless. NOTE: deliberately NOT `-nographic` -- that muxes the VGA
     * firmware/monitor banner onto the same stream as serial and pollutes
     * the capture. `-display none` keeps `-serial file:` clean (verified:
     * serial_hello.serial is exactly "HARNESS-OK\n"). */
    PUSH("-display");
    PUSH("none");
    /* -no-reboot: a triple fault EXITS qemu (code 0) instead of looping
     * (PRD Sec 8: triple-fault detect). The reset is still recorded in the
     * -d log, which is our detector. NOTE: we deliberately do NOT pass
     * -no-shutdown -- it keeps the VM paused after the reset request, so
     * qemu hangs on the fault path and only the wall-clock timeout reaps
     * it. With -no-reboot alone the bad fixture exits promptly AND the log
     * carries "Triple fault" (verified). The timeout still guards genuine
     * hangs (see the `hang` fixture). */
    PUSH("-no-reboot");

    /* isa-debug-exit: lets a guest request a clean QEMU exit by writing to
     * I/O port 0xF4 (exit status = (value<<1)|1). Fixtures use this so the
     * GOOD path stops promptly instead of relying on the wall-clock timeout
     * (a timed-out run is by definition not `ok`). Harmless to a guest that
     * never writes the port. */
    PUSH("-device");
    PUSH("isa-debug-exit,iobase=0xF4,iosize=0x04");

    /* Serial capture to a file we read after reaping. */
    PUSH("-serial");
    static char serbuf[QEMU_PATH_MAX + 8];
    snprintf(serbuf, sizeof(serbuf), "file:%s", serial_path);
    PUSH(serbuf);

    /* Debug log: interrupts, guest errors, cpu resets -> the log we parse. */
    PUSH("-d");
    PUSH("int,guest_errors,cpu_reset");
    PUSH("-D");
    PUSH(log_path);

    /* Boot mechanism. */
    if (cfg->kernel_path) {
        PUSH("-kernel");
        PUSH(cfg->kernel_path);
    } else {
        /* disk_path validated non-NULL by caller. */
        static char drvbuf[QEMU_PATH_MAX + 24];
        snprintf(drvbuf, sizeof(drvbuf), "format=raw,file=%s",
                 cfg->disk_path);
        PUSH("-drive");
        PUSH(drvbuf);
    }

    /* Optional FAT12 data volume on the IDE primary SLAVE (if=ide,index=1).
     * The boot drive above defaults to if=ide,index=0 (primary master), so
     * index=1 reuses the same channel port base (0x1F0) with the slave
     * drive-select bit -- exactly what os/milton/ata.c addresses. The boot
     * disk is unaffected; the kernel mounts this second disk over ATA.
     * (brief Sec 1.2: index 1 = primary slave.) Pushed for both -kernel and
     * -drive boot modes, but only meaningful for the raw-disk OS boot. */
    if (cfg->data_disk_path) {
        static char drvbuf2[QEMU_PATH_MAX + 48];
        snprintf(drvbuf2, sizeof(drvbuf2),
                 "file=%s,format=raw,if=ide,index=1", cfg->data_disk_path);
        PUSH("-drive");
        PUSH(drvbuf2);
    }

    /* gdb stub: stub on tcp::1234, CPU stopped at reset. We only wire the
     * flags; driving gdb is out of scope (PRD Sec 8). Usage:
     *   gdb -ex 'target remote :1234' build/<guest>.elf */
    if (cfg->enable_gdb) {
        PUSH("-s");
        PUSH("-S");
    }

    /* QMP for screendump AND/OR keystroke injection (beads initech-43b: --keys
     * needs the same QMP socket). server=on,wait=off => qemu does not block on
     * us. */
    if (cfg->enable_qmp_screendump ||
        (cfg->keys_spec && cfg->keys_spec[0] != '\0')) {
        static char qmpbuf[QEMU_PATH_MAX + 32];
        snprintf(qmpbuf, sizeof(qmpbuf), "unix:%s,server=on,wait=off",
                 sock_path);
        PUSH("-qmp");
        PUSH(qmpbuf);
        /* -display none is already pushed unconditionally above. */
    }

    argv[i] = NULL;
    return i;
#undef PUSH
}

/* ------------------------------------------------------------------ */
/* public entry points                                                 */
/* ------------------------------------------------------------------ */

void qemu_result_free(QemuResult *out)
{
    if (!out) {
        return;
    }
    free(out->serial_text);
    out->serial_text = NULL;
    out->serial_len = 0;
}

int qemu_run(const QemuConfig *cfg, QemuResult *out)
{
    if (!cfg || !out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->exit_code = -1;

    /* Never let a closed QMP/serial pipe kill the harness with SIGPIPE --
     * the harness must always reach its reap+report (Law 2). */
    signal(SIGPIPE, SIG_IGN);

    if (!cfg->kernel_path && !cfg->disk_path) {
        fprintf(stderr,
                "qemu_run: config has neither kernel_path nor disk_path\n");
        return -1;
    }

    const char *dir = cfg->output_dir ? cfg->output_dir : "build";
    const char *name = cfg->name ? cfg->name : "qemu";
    int timeout_ms = cfg->timeout_ms > 0 ? cfg->timeout_ms
                                         : QEMU_DEFAULT_TIMEOUT_MS;

    char sock_path[QEMU_PATH_MAX];
    if (join_path(out->serial_path, sizeof(out->serial_path), dir, name,
                  ".serial") != 0 ||
        join_path(out->log_path, sizeof(out->log_path), dir, name,
                  ".qemu.log") != 0 ||
        join_path(out->screendump_path, sizeof(out->screendump_path), dir,
                  name, ".ppm") != 0 ||
        join_path(sock_path, sizeof(sock_path), dir, name, ".qmp.sock")
            != 0) {
        fprintf(stderr, "qemu_run: output path too long\n");
        return -1;
    }

    /* Fresh outputs: remove stale files so we never read a previous run's
     * serial/log/ppm (a classic false-green source -- Law 2). */
    unlink(out->serial_path);
    unlink(out->log_path);
    unlink(out->screendump_path);
    unlink(sock_path);

    char *argv[MAX_ARGV];
    if (build_argv(cfg, argv, out->serial_path, out->log_path, sock_path)
        < 0) {
        fprintf(stderr, "qemu_run: too many qemu arguments\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "qemu_run: fork failed: %s\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        /* child: silence qemu's stdout/stderr noise onto the log dir's
         * null so -nographic monitor banner does not pollute our streams.
         * Serial goes to a file already; QMP banner goes to the socket. */
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) {
                close(devnull);
            }
        }
        execvp(QEMU_BIN, argv);
        /* exec failed */
        _exit(127);
    }

    out->launched = true;

    /* If we are doing a screendump and/or keystroke injection, drive QMP from
     * the parent while qemu runs: qmp_session retries the connect, optionally
     * waits for the keys-after marker + injects keys, optionally screendumps,
     * then quits (beads initech-f2s + initech-43b). */
    bool want_keys = cfg->keys_spec && cfg->keys_spec[0] != '\0';
    if (cfg->enable_qmp_screendump || want_keys) {
        const char *ppm = cfg->enable_qmp_screendump ? out->screendump_path
                                                      : NULL;
        int sent = 0;
        if (qmp_session(sock_path, ppm,
                        want_keys ? cfg->keys_spec : NULL,
                        want_keys ? cfg->keys_after : NULL,
                        out->serial_path, &sent) == 0) {
            out->keys_sent = sent;
            if (cfg->enable_qmp_screendump) {
                struct stat st;
                if (stat(out->screendump_path, &st) == 0 && st.st_size > 0) {
                    out->screendump_taken = true;
                }
            }
        }
    }

    /* Wall-clock timeout loop: poll waitpid, escalate SIGTERM -> SIGKILL. */
    long long deadline = mono_ms() + timeout_ms;
    int status = 0;
    bool reaped = false;
    bool sent_term = false;
    long long term_at = 0;
    for (;;) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) {
            reaped = true;
            break;
        }
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            break; /* unexpected; treat as unreapable */
        }
        long long now = mono_ms();
        if (!sent_term && now >= deadline) {
            out->timed_out = true;
            kill(pid, SIGTERM);
            sent_term = true;
            term_at = now;
        } else if (sent_term && now >= term_at + 500) {
            /* grace expired -> hard kill */
            kill(pid, SIGKILL);
        }
        sleep_ms(20);
    }

    if (reaped) {
        if (WIFEXITED(status)) {
            out->exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            out->term_signal = WTERMSIG(status);
            out->exit_code = -1;
        }
    }

    /* Clean up the socket file regardless. */
    unlink(sock_path);

    /* Capture serial. Missing file => empty capture (not an error). */
    {
        size_t len = 0;
        char *txt = read_file(out->serial_path, &len);
        if (txt) {
            out->serial_text = txt;
            out->serial_len = len;
        } else {
            out->serial_text = calloc(1, 1);
            out->serial_len = 0;
        }
    }

    /* Parse the -d log for triple-fault / reset / guest_error indicators.
     * QEMU emits a "Triple fault" line and "CPU Reset" entries; guest
     * errors appear under the guest_errors class. We count rather than
     * just boolean so a gate can be precise. */
    {
        size_t llen = 0;
        char *log = read_file(out->log_path, &llen);
        if (log) {
            out->cpu_reset_count = count_substr(log, "CPU Reset");
            out->guest_errors = count_substr(log, "guest_error");
            /* Triple-fault detection. IMPORTANT: cpu_reset_count is NOT a
             * fault signal -- SeaBIOS issues ~2 normal "CPU Reset" events
             * during boot in EVERY run (verified against both fixtures), so
             * keying off it would false-positive on the GOOD guest, the
             * worst outcome (Law 2). The reliable signals, emitted only on
             * an actual triple fault:
             *   - QEMU's explicit "Triple fault" log line; and
             *   - the double-fault cascade "check_exception old: 0x8"
             *     (a #DF that itself faults -> triple fault).
             * We report the count for diagnostics only. */
            if (strstr(log, "Triple fault") != NULL ||
                strstr(log, "triple fault") != NULL ||
                strstr(log, "check_exception old: 0x8") != NULL) {
                out->triple_fault = true;
            }
            free(log);
        }
    }

    /* Marker assertion. */
    if (cfg->expect_marker && out->serial_text) {
        out->marker_found =
            strstr(out->serial_text, cfg->expect_marker) != NULL;
    }

    /* Overall verdict. */
    out->ok = out->launched && !out->timed_out && !out->triple_fault &&
              out->guest_errors == 0 &&
              (cfg->expect_marker == NULL || out->marker_found);

    return 0;
}
