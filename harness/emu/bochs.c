/*
 * bochs.c -- InitechOS Bochs oracle harness (the Bochs leg of the
 * tri-emulator gate). beads: initech-564. See bochs.h for the contract.
 *
 * FACTORY code, C11 + POSIX.1-2008 (fork/exec/waitpid, AF_INET socket,
 * nanosleep, clock_gettime). C-only (Law 3): the RFB unblock that the Python
 * diagnosis helper (rfb_unblock.py) prototyped is implemented here in C.
 *
 * Why each piece (Law 1, grounded in this session's bd memories):
 *  - WORKING config = legacy 64K BIOS + the LGPL vgabios + cpu pentium; the
 *    latest BIOS triple-faults (CMOV / IDT.limit=0) on this Bochs 2.7 build.
 *  - This Bochs is a DEBUGGER build: `-rc <file>` with `c` auto-continues.
 *  - The only headless display plugin is rfb, which blocks ~30s for a VNC
 *    client; we connect + do the RFB 3.3 handshake (sec type None) and hold
 *    the socket (draining it so Bochs never blocks on a full send buffer).
 *  - Serial is captured via `com1: mode=file`. The guest hlt-loops, so the run
 *    ALWAYS times out -- expected; the verdict is markers + fault scan.
 *  - Bochs locks the disk image (<disk>.lock); we remove a stale lock before
 *    launch and after reaping so a killed run does not poison the next.
 */
#define _POSIX_C_SOURCE 200809L

#include "bochs.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BOCHS_BIN "bochs"
#define SECTOR_BYTES 512
/* Fixed CHS used for the raw tracer image: heads*spt = 64 sectors/cylinder.
 * The tracer is 128 sectors -> exactly 2 cylinders (bd memory). */
#define BOCHS_HEADS 2
#define BOCHS_SPT   32

/* ---- small self-contained helpers (mirrors qemu.c; kept local to this TU) -- */

static long long mono_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void sleep_ms(int ms)
{
    struct timespec req;
    req.tv_sec  = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&req, &req) != 0 && errno == EINTR) {
        /* resume the remaining interval */
    }
}

static int join_path(char *dst, size_t cap, const char *dir, const char *file)
{
    int n = snprintf(dst, cap, "%s/%s", dir, file);
    return (n < 0 || (size_t)n >= cap) ? -1 : 0;
}

/* Read a whole file into a heap NUL-terminated buffer; *len = bytes (no NUL).
 * Returns NULL if the file is absent/unreadable (caller treats as empty). */
static char *read_file(const char *path, size_t *len)
{
    if (len) *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';
    if (len) *len = got;
    return buf;
}

/* Strip every '\r' in place (Bochs serial may emit CRLF; our markers are
 * line-exact LF, matching the QEMU capture). Adjusts *len. */
static void strip_cr(char *s, size_t *len)
{
    size_t w = 0;
    for (size_t r = 0; s[r]; r++) {
        if (s[r] != '\r') s[w++] = s[r];
    }
    s[w] = '\0';
    if (len) *len = w;
}

/* ---- disk geometry ------------------------------------------------------- */

/* Compute the cylinder count for the fixed 2/32 geometry. Returns -1 if the
 * image size is not a whole number of cylinders (Rule 2 -- fail loud rather
 * than hand Bochs a geometry that does not match the image). */
static long disk_cylinders(const char *disk_path)
{
    struct stat st;
    if (stat(disk_path, &st) != 0) return -1;
    long sectors = (long)(st.st_size / SECTOR_BYTES);
    long per_cyl = (long)BOCHS_HEADS * BOCHS_SPT;
    if (sectors <= 0 || (sectors % per_cyl) != 0) return -1;
    return sectors / per_cyl;
}

/* ---- bochsrc generation -------------------------------------------------- */

static int write_bochsrc(const char *path, const BochsConfig *cfg,
                         const char *serial_path, long cylinders)
{
    const char *bios = cfg->bios_path    ? cfg->bios_path    : BOCHS_DEFAULT_BIOS;
    const char *vga  = cfg->vgabios_path ? cfg->vgabios_path : BOCHS_DEFAULT_VGABIOS;
    int port = (cfg->rfb_port > 0) ? cfg->rfb_port : BOCHS_DEFAULT_RFB_PORT;

    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    /* The WORKING combo (bd memory bochs-boot-solved-initech-6pj). vbe is
     * enabled so stage2's VBE probe runs (it then ENOMODEs and falls back to
     * mode 0x13); display_library rfb is the only headless option. */
    fprintf(f,
        "romimage: file=%s\n"
        "vgaromimage: file=%s\n"
        "megs: 32\n"
        "vga: extension=vbe\n"
        "cpu: model=pentium, ips=50000000\n"
        "ata0-master: type=disk, path=\"%s\", mode=flat, cylinders=%ld, heads=%d, spt=%d\n"
        "boot: disk\n"
        "com1: enabled=1, mode=file, dev=%s\n"
        "clock: sync=none\n"
        "display_library: rfb\n",
        bios, vga, cfg->disk_path, cylinders, BOCHS_HEADS, BOCHS_SPT, serial_path);
    /* The RFB port is selected by the BX_RFB display via the 5900+display#
     * default; this build uses 5900. (Bochs 2.7 has no bochsrc knob for it.) */
    (void)port;
    fclose(f);
    return 0;
}

/* ---- RFB 3.3 unblock (the headless key) ---------------------------------- */

static int recvn(int fd, void *buf, size_t n)
{
    unsigned char *p = buf;
    size_t got = 0;
    while (got < n) {
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r == 0) return -1;                /* peer closed */
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += (size_t)r;
    }
    return 0;
}

/* Connect to the Bochs RFB server (retrying until it opens the port or the
 * deadline passes), complete the RFB 3.3 handshake (security type None), and
 * return the connected socket fd (caller holds + drains it). Returns -1 on
 * failure. *out_handshook set to 1 if the handshake completed. */
static int rfb_unblock(int port, long long deadline_ms, int *out_handshook)
{
    *out_handshook = 0;
    int fd = -1;
    while (mono_ms() < deadline_ms) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons((unsigned short)port);
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == 0) break;
        close(fd);
        fd = -1;
        sleep_ms(200);
    }
    if (fd < 0) return -1;

    /* Handshake: recv 12-byte server ProtocolVersion, send our 3.3, recv the
     * 4-byte security type (1 == None), send the 1-byte shared ClientInit. */
    char ver[12];
    if (recvn(fd, ver, 12) != 0) { close(fd); return -1; }
    if (send(fd, "RFB 003.003\n", 12, 0) != 12) { close(fd); return -1; }
    unsigned char sec[4];
    if (recvn(fd, sec, 4) != 0) { close(fd); return -1; }
    unsigned long sectype = ((unsigned long)sec[0] << 24) | ((unsigned long)sec[1] << 16) |
                            ((unsigned long)sec[2] << 8) | sec[3];
    if (sectype == 0) { close(fd); return -1; }   /* server refused */
    unsigned char shared = 1;
    if (send(fd, &shared, 1, 0) != 1) { close(fd); return -1; }
    *out_handshook = 1;

    /* Non-blocking from here so the drain in the poll loop never stalls. */
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl >= 0) fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    return fd;
}

static void rfb_drain(int fd)
{
    if (fd < 0) return;
    char buf[4096];
    for (;;) {
        ssize_t r = recv(fd, buf, sizeof(buf), 0);
        if (r > 0) continue;
        break;   /* 0 (closed) or <0 (EWOULDBLOCK/EINTR/error): stop for now */
    }
}

/* ---- the run ------------------------------------------------------------- */

void bochs_result_free(BochsResult *out)
{
    if (!out) return;
    free(out->serial_text);
    out->serial_text = NULL;
}

int bochs_run(const BochsConfig *cfg, BochsResult *out)
{
    if (!cfg || !out) return -1;
    memset(out, 0, sizeof(*out));
    if (!cfg->disk_path) {
        fprintf(stderr, "bochs_run: disk_path is required\n");
        return -1;
    }

    const char *dir  = cfg->output_dir ? cfg->output_dir : "build";
    const char *name = cfg->name       ? cfg->name       : "bochs";
    int timeout_ms = (cfg->timeout_ms > 0) ? cfg->timeout_ms : BOCHS_DEFAULT_TIMEOUT_MS;
    int port = (cfg->rfb_port > 0) ? cfg->rfb_port : BOCHS_DEFAULT_RFB_PORT;

    long cyl = disk_cylinders(cfg->disk_path);
    if (cyl < 0) {
        fprintf(stderr, "bochs_run: %s size is not a whole 2x32 geometry\n",
                cfg->disk_path);
        return -1;
    }

    char rc_path[BOCHS_PATH_MAX], fn[256];
    snprintf(fn, sizeof(fn), "%s.rc", name);
    if (join_path(rc_path, sizeof(rc_path), dir, fn) != 0) return -1;
    snprintf(fn, sizeof(fn), "%s.bochsrc", name);
    if (join_path(out->bochsrc_path, sizeof(out->bochsrc_path), dir, fn) != 0) return -1;
    snprintf(fn, sizeof(fn), "%s.serial", name);
    if (join_path(out->serial_path, sizeof(out->serial_path), dir, fn) != 0) return -1;
    snprintf(fn, sizeof(fn), "%s.bochs.log", name);
    if (join_path(out->log_path, sizeof(out->log_path), dir, fn) != 0) return -1;

    /* Remove a stale serial (so a launch failure cannot show old content) and
     * a stale disk lock (a previous killed run leaves <disk>.lock). */
    remove(out->serial_path);
    {
        char lock[BOCHS_PATH_MAX];
        if (snprintf(lock, sizeof(lock), "%s.lock", cfg->disk_path) < (int)sizeof(lock))
            remove(lock);
    }

    /* rc file: auto-continue the debugger build. */
    {
        FILE *f = fopen(rc_path, "wb");
        if (!f) { fprintf(stderr, "bochs_run: cannot write %s\n", rc_path); return -1; }
        fputs("c\n", f);
        fclose(f);
    }
    if (write_bochsrc(out->bochsrc_path, cfg, out->serial_path, cyl) != 0) {
        fprintf(stderr, "bochs_run: cannot write %s\n", out->bochsrc_path);
        return -1;
    }

    /* SIGPIPE off: a dropped RFB socket must not kill the harness. */
    signal(SIGPIPE, SIG_IGN);

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "bochs_run: fork failed: %s\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        /* Child: stdout+stderr -> log, stdin <- /dev/null. */
        int logfd = open(out->log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        int nullfd = open("/dev/null", O_RDONLY);
        if (logfd >= 0) { dup2(logfd, 1); dup2(logfd, 2); }
        if (nullfd >= 0) dup2(nullfd, 0);
        char *argv[] = {
            (char *)BOCHS_BIN, (char *)"-q",
            (char *)"-f", out->bochsrc_path,
            (char *)"-rc", rc_path,
            NULL
        };
        execvp(BOCHS_BIN, argv);
        _exit(127);   /* exec failed */
    }

    out->launched = true;
    long long start = mono_ms();
    long long deadline = start + timeout_ms;

    /* Unblock the RFB wait so Bochs proceeds (give it part of the budget to
     * open the port). */
    int handshook = 0;
    long long rfb_deadline = start + (timeout_ms / 2 < 10000 ? 10000 : timeout_ms / 2);
    int rfb_fd = rfb_unblock(port, rfb_deadline, &handshook);
    out->rfb_unblocked = (handshook != 0);

    /* Poll waitpid against the deadline, draining the RFB socket so Bochs never
     * blocks on a full send buffer. The guest hlt-loops -> we run to deadline. */
    int status = 0;
    bool reaped = false;
    for (;;) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) { reaped = true; break; }
        if (w < 0 && errno != EINTR) break;
        if (mono_ms() >= deadline) {
            out->timed_out = true;
            break;
        }
        rfb_drain(rfb_fd);
        sleep_ms(100);
    }

    if (!reaped) {
        kill(pid, SIGTERM);
        long long grace = mono_ms() + 1500;
        while (mono_ms() < grace) {
            if (waitpid(pid, &status, WNOHANG) == pid) { reaped = true; break; }
            sleep_ms(50);
        }
        if (!reaped) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
        }
    }
    if (rfb_fd >= 0) close(rfb_fd);

    if (reaped && WIFEXITED(status)) {
        out->exit_code = WEXITSTATUS(status);
        out->term_signal = 0;
    } else if (WIFSIGNALED(status)) {
        out->exit_code = -1;
        out->term_signal = WTERMSIG(status);
    } else {
        out->exit_code = -1;
    }

    /* Drop the disk lock for the next run. */
    {
        char lock[BOCHS_PATH_MAX];
        if (snprintf(lock, sizeof(lock), "%s.lock", cfg->disk_path) < (int)sizeof(lock))
            remove(lock);
    }

    /* Serial capture. */
    {
        size_t len = 0;
        char *txt = read_file(out->serial_path, &len);
        if (txt) { strip_cr(txt, &len); out->serial_text = txt; out->serial_len = len; }
        else { out->serial_text = calloc(1, 1); out->serial_len = 0; }
    }

    /* Fault scan: a triple fault on this Bochs shows as the CPU "3rd (13)
     * exception" line and/or an interrupt with "IDT.limit = 0x0" in the log. */
    {
        size_t llen = 0;
        char *log = read_file(out->log_path, &llen);
        if (log) {
            if (strstr(log, "3rd (13) exception") != NULL ||
                strstr(log, "IDT.limit = 0x0") != NULL) {
                out->triple_fault = true;
            }
            free(log);
        }
    }

    /* Marker: line-exact (the marker on its own line), matching the Makefile
     * grep "^MARKER$" semantics, so a substring of a longer line never
     * false-passes. */
    if (cfg->expect_marker && out->serial_text) {
        size_t mlen = strlen(cfg->expect_marker);
        const char *s = out->serial_text;
        while (*s) {
            const char *nl = strchr(s, '\n');
            size_t linelen = nl ? (size_t)(nl - s) : strlen(s);
            if (linelen == mlen && memcmp(s, cfg->expect_marker, mlen) == 0) {
                out->marker_found = true;
                break;
            }
            if (!nl) break;
            s = nl + 1;
        }
    }

    out->ok = out->launched && out->rfb_unblocked && !out->triple_fault &&
              (cfg->expect_marker ? out->marker_found : true);
    return 0;
}
