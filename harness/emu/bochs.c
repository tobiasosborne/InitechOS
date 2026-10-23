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
 *    times out unless a completion marker was requested; the verdict is
 *    markers + fault scan (PRD Sec 8; initech-cqej reboot-and-wait).
 *  - Bochs locks the disk image (<disk>.lock); we remove a stale lock before
 *    launch and after reaping so a killed run does not poison the next.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700   /* realpath(3) (XSI; glibc gates it on XOPEN_EXTENDED) */

#include "bochs.h"
#include "bochs_trace.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/file.h>
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

/* A live capture may end mid-line. Count only COMPLETE exact lines, allowing
 * the UART's CRLF. Ref: bochs.h expect_marker contract, spec/flair_power_
 * traces.mk: one boot marker cannot certify a completed restart (Rule 5). */
static int serial_marker_count(const char *text, const char *marker)
{
    int count = 0;
    size_t want = strlen(marker);
    const char *line = text;
    const char *nl;
    while ((nl = strchr(line, '\n')) != NULL) {
        size_t n = (size_t)(nl - line);
        if (n && line[n - 1] == '\r') n--;
#ifdef BOCHS_MUTATE_MARKER_SUBSTRING
        const char *hit = strstr(line, marker);
        if (hit && hit + want <= nl) count++;
#else
        if (n == want && memcmp(line, marker, n) == 0) count++;
#endif
        line = nl + 1;
    }
#ifdef BOCHS_MUTATE_PARTIAL_LINE
    if (strcmp(line, marker) == 0) count++;
#endif
    return count;
}

static bool serial_completion(const char *text, const char *marker, int count)
{
#ifdef BOCHS_MUTATE_NO_REBOOT_WAIT
    count = 1;                /* Rule 6: falsely finish on the first boot. */
#endif
    return serial_marker_count(text, marker) >= (count > 0 ? count : 1);
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
                         const char *serial_path, long cylinders,
                         long data_cylinders)
{
    const char *bios = cfg->bios_path    ? cfg->bios_path    : BOCHS_DEFAULT_BIOS;
    const char *vga  = cfg->vgabios_path ? cfg->vgabios_path : BOCHS_DEFAULT_VGABIOS;
    int port = (cfg->rfb_port > 0) ? cfg->rfb_port : BOCHS_DEFAULT_RFB_PORT;

    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    /* The WORKING combo (bd memory bochs-boot-solved-initech-6pj). vbe is
     * the default extension: the LGPL vgabios offers 0101h 640x480x8 with an
     * LFB, which IO.SYS takes since initech-2gva (WL-0170 corrected the old
     * "ENOMODE" claim). display_library rfb is the only headless option. */
    fprintf(f,
        "romimage: file=%s\n"
        "vgaromimage: file=%s\n"
        "megs: 32\n"
        "vga: extension=%s\n"
        "cpu: model=pentium, ips=50000000\n"
        "ata0-master: type=disk, path=\"%s\", mode=flat, cylinders=%ld, heads=%d, spt=%d\n",
        bios, vga, cfg->vga_extension ? cfg->vga_extension : "vbe",
        cfg->disk_path, cylinders, BOCHS_HEADS, BOCHS_SPT);
    if (cfg->data_disk_path) {
        fprintf(f,
        "ata0-slave: type=disk, path=\"%s\", mode=flat, cylinders=%ld, heads=%d, spt=%d\n",
        cfg->data_disk_path, data_cylinders, BOCHS_HEADS, BOCHS_SPT);
    }
    fprintf(f,
        "boot: disk\n"
        "com1: enabled=1, mode=file, dev=%s\n",
        serial_path);
    if (cfg->rtc_time0 > 0)
        fprintf(f, "clock: sync=none, time0=%ld\n", cfg->rtc_time0);
    else
        fputs("clock: sync=none\n", f);
    fputs("display_library: rfb\n", f);
    if (cfg->keys_script && strchr(cfg->keys_script, ':'))
        fputs("mouse: type=ps2, enabled=1\n", f);
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
        if (fd < 0) {
            fprintf(stderr, "bochs_run: RFB socket: %s\n", strerror(errno));
            return -1;
        }
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

/* ---- RFB server-message shadow (initech-u9gf) ---------------------------- *
 * The handshake above leaves the server's ServerInit and every later message
 * on the socket. Instead of discarding them, parse them (RFC 6143 Sec 7.3.2,
 * 7.6) into a shadow of the server framebuffer. Bochs 2.7 pushes raw
 * FramebufferUpdates on its own (rfb.cc flush/SendUpdate) and serves the
 * fixed 8-bit BGR233 pixel format (rfb.cc BGR233Format, ServerInit); we never
 * send SetPixelFormat/SetEncodings, so any other format or encoding is a
 * protocol surprise and fails loud (Rule 2) rather than producing pixels. */
typedef struct {
    int fd;
    unsigned char *rx;
    size_t len, cap;
    bool have_init;
    unsigned w, h;
    unsigned char *fb;           /* w*h BGR233 codes                          */
    unsigned long full_frames;   /* raw rects covering the whole window       */
    bool closed;
    char err[160];
} RfbShadow;

static unsigned be16(const unsigned char *p) { return ((unsigned)p[0] << 8) | p[1]; }
static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | p[3];
}

static void rfb_fail(RfbShadow *r, const char *why)
{
    if (!r->err[0]) snprintf(r->err, sizeof(r->err), "%s", why);
}

/* Parse ONE complete message at p[0..n); returns bytes consumed, 0 if more
 * bytes are needed, -1 on a protocol violation (r->err set). */
static long rfb_parse_one(RfbShadow *r, const unsigned char *p, size_t n)
{
    if (!r->have_init) {
        if (n < 24) return 0;
        size_t need = 24 + (size_t)be32(p + 20);
        if (n < need) return 0;
        r->w = be16(p);
        r->h = be16(p + 2);
        /* bpp depth bigEndian trueColour rMax gMax bMax rSh gSh bSh */
        if (p[4] != 8 || p[5] != 8 || p[7] != 1 || be16(p + 8) != 7 ||
            be16(p + 10) != 7 || be16(p + 12) != 3 || p[14] != 0 ||
            p[15] != 3 || p[16] != 6) {
            rfb_fail(r, "ServerInit pixel format is not BGR233");
            return -1;
        }
        if (r->w == 0 || r->h == 0 || r->w > 4096 || r->h > 4096) {
            rfb_fail(r, "ServerInit geometry out of range");
            return -1;
        }
        r->fb = calloc((size_t)r->w * r->h, 1);
        if (!r->fb) { rfb_fail(r, "out of memory"); return -1; }
        r->have_init = true;
        return (long)need;
    }
    if (n < 1) return 0;
    switch (p[0]) {
    case 0: {                                   /* FramebufferUpdate */
        if (n < 4) return 0;
        unsigned rects = be16(p + 2);
        size_t off = 4;
        for (unsigned i = 0; i < rects; i++) {   /* size pass: all present? */
            if (n < off + 12) return 0;
            unsigned long enc = be32(p + off + 8);
            if (enc != 0) { rfb_fail(r, "non-raw rectangle encoding"); return -1; }
            off += 12 + (size_t)be16(p + off + 4) * be16(p + off + 6);
            if (n < off) return 0;
        }
        off = 4;
        for (unsigned i = 0; i < rects; i++) {   /* apply pass */
            unsigned x = be16(p + off), y = be16(p + off + 2);
            unsigned w = be16(p + off + 4), h = be16(p + off + 6);
            off += 12;
            if (x + w > r->w || y + h > r->h) {
                rfb_fail(r, "rectangle outside the ServerInit window");
                return -1;
            }
            for (unsigned row = 0; row < h; row++)
                memcpy(r->fb + (size_t)(y + row) * r->w + x, p + off + (size_t)row * w, w);
            off += (size_t)w * h;
            if (x == 0 && y == 0 && w == r->w && h == r->h) r->full_frames++;
        }
        return (long)off;
    }
    case 1:                                     /* SetColourMapEntries */
        if (n < 6) return 0;
        if (n < 6 + 6 * (size_t)be16(p + 4)) return 0;
        return (long)(6 + 6 * (size_t)be16(p + 4));
    case 2:                                     /* Bell */
        return 1;
    case 3:                                     /* ServerCutText */
        if (n < 8) return 0;
        if (n < 8 + (size_t)be32(p + 4)) return 0;
        return (long)(8 + (size_t)be32(p + 4));
    default:
        rfb_fail(r, "unknown server message type");
        return -1;
    }
}

/* Read everything available (non-blocking) and apply complete messages.
 * Also the drain that keeps Bochs from blocking on a full send buffer. */
static void rfb_pump(RfbShadow *r)
{
    if (r->fd < 0 || r->closed) return;
    for (;;) {
        if (r->cap - r->len < 65536) {
            size_t ncap = r->cap ? r->cap * 2 : 1u << 20;
            unsigned char *nb = realloc(r->rx, ncap);
            if (!nb) { rfb_fail(r, "out of memory"); r->closed = true; return; }
            r->rx = nb;
            r->cap = ncap;
        }
        ssize_t got = recv(r->fd, r->rx + r->len, r->cap - r->len, 0);
        if (got > 0) { r->len += (size_t)got; continue; }
        if (got == 0) r->closed = true;
        break;   /* EWOULDBLOCK/EINTR/error: stop for now */
    }
    if (r->err[0]) { r->len = 0; return; }       /* stream unparseable: drain */
    size_t used = 0;
    for (;;) {
        long c = rfb_parse_one(r, r->rx + used, r->len - used);
        if (c < 0) { r->len = 0; return; }
        if (c == 0) break;
        used += (size_t)c;
    }
    memmove(r->rx, r->rx + used, r->len - used);
    r->len -= used;
}

/* FramebufferUpdateRequest, non-incremental, the whole window: Bochs then
 * marks the whole window dirty (rfb.cc rfbFramebufferUpdateRequest) and its
 * next flush() sends it as ONE raw rectangle. */
static bool rfb_request_full(RfbShadow *r)
{
    if (r->fd < 0 || !r->have_init) return false;
    unsigned char m[10] = { 3, 0, 0, 0, 0, 0,
                            (unsigned char)(r->w >> 8), (unsigned char)r->w,
                            (unsigned char)(r->h >> 8), (unsigned char)r->h };
    size_t sent = 0;
    long long until = mono_ms() + 2000;
    while (sent < sizeof(m) && mono_ms() < until) {
        ssize_t k = send(r->fd, m + sent, sizeof(m) - sent, 0);
        if (k > 0) { sent += (size_t)k; continue; }
        if (k < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) break;
        rfb_pump(r);
        sleep_ms(5);
    }
    return sent == sizeof(m);
}

/* Ref: docs/research/samir-bochs-rfb.md, RFC 6143 Sec 7.5.4.
 * Keep the accepted trace vocabulary explicit; an unknown token is never
 * silently dropped. This is factory input, not an OS keyboard shortcut. */
static unsigned rfb_key(const char *s, size_t n)
{
    BochsTraceStep step;
    return bochs_trace_parse(s, n, &step) && step.kind == BX_KEY ? step.key : 0;
}

static bool rfb_key_event(RfbShadow *r, unsigned key, bool down)
{
    unsigned char m[8] = { 4, down ? 1 : 0, 0, 0,
        (unsigned char)(key >> 24), (unsigned char)(key >> 16),
        (unsigned char)(key >> 8), (unsigned char)key };
    size_t sent = 0;
    long long until = mono_ms() + 2000;
    while (sent < sizeof m && mono_ms() < until) {
        ssize_t k = send(r->fd, m + sent, sizeof m - sent, MSG_NOSIGNAL);
        if (k > 0) { sent += (size_t)k; continue; }
        if (k < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) break;
        rfb_pump(r);
        sleep_ms(5);
    }
    if (sent != sizeof m) rfb_fail(r, "cannot send KeyEvent");
    return sent == sizeof m;
}

/* RFC 6143 Sec 7.5.5. As with KeyEvent, interrupted/partial sends must
 * finish the packet or fail loud; never leave a truncated RFB message. */
static bool rfb_pointer_event(RfbShadow *r, unsigned x, unsigned y, unsigned buttons)
{
    unsigned char m[6] = {5, (unsigned char)buttons,
        (unsigned char)(x >> 8), (unsigned char)x,
        (unsigned char)(y >> 8), (unsigned char)y};
    size_t sent = 0;
    long long until = mono_ms() + 2000;
    while (sent < sizeof m && mono_ms() < until) {
        ssize_t k = send(r->fd, m + sent, sizeof m - sent, MSG_NOSIGNAL);
        if (k > 0) { sent += (size_t)k; continue; }
        if (k < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) break;
        rfb_pump(r);
        sleep_ms(5);
    }
    if (sent != sizeof m) rfb_fail(r, "cannot send PointerEvent");
    return sent == sizeof m;
}

/* <name>.rfb.pgm: the server's BGR233 code per pixel, unchanged (the graded
 * artifact). <name>.rfb.ppm: each 3/3/2-bit field widened to 0..255 for the
 * Law-4 eyeball only. */
static bool rfb_write_capture(const unsigned char *fb, unsigned w, unsigned h,
                              const char *pgm, const char *ppm)
{
    FILE *f = fopen(pgm, "wb");
    if (!f) return false;
    fprintf(f, "P5\n%u %u\n255\n", w, h);
    bool ok = fwrite(fb, 1, (size_t)w * h, f) == (size_t)w * h;
    ok = (fclose(f) == 0) && ok;
    f = fopen(ppm, "wb");
    if (!f) return false;
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (size_t i = 0; ok && i < (size_t)w * h; i++) {
        unsigned char c = fb[i];
        unsigned char rgb[3] = { (unsigned char)((c & 7u) * 255u / 7u),
                                 (unsigned char)(((c >> 3) & 7u) * 255u / 7u),
                                 (unsigned char)(((c >> 6) & 3u) * 255u / 3u) };
        ok = fwrite(rgb, 1, 3, f) == 3;
    }
    ok = (fclose(f) == 0) && ok;
    return ok;
}

/* PointerEvent (RFC 6143 Sec 7.5.5) press+release on Bochs's header bar.
 * Bochs 2.7 dispatches a header-bar press by gui.cc headerbar_click(x), whose
 * right-gravity origin is guest_xres - xorigin (NOT the 720-wide window the
 * rfb.cc show_headerbar draws in). The right-hand buttons are 32 px wide and
 * created right-to-left power, save/restore, reset, config, SNAPSHOT
 * (gui.cc init_signal_handlers order), so snapshot is xorigin 160: for the
 * 640-wide guest it spans x in [480,512). y=16 is inside the 32-px bar. */
#define BOCHS_SNAPSHOT_XORIGIN 160
static bool rfb_click_header(RfbShadow *r, unsigned guest_xres)
{
    unsigned x = guest_xres - BOCHS_SNAPSHOT_XORIGIN + 16, y = 16;
    unsigned char m[12] = { 5, 1, (unsigned char)(x >> 8), (unsigned char)x,
                            (unsigned char)(y >> 8), (unsigned char)y,
                            5, 0, (unsigned char)(x >> 8), (unsigned char)x,
                            (unsigned char)(y >> 8), (unsigned char)y };
    size_t sent = 0;
    long long until = mono_ms() + 2000;
    while (sent < sizeof(m) && mono_ms() < until) {
        ssize_t k = send(r->fd, m + sent, sizeof(m) - sent, 0);
        if (k > 0) { sent += (size_t)k; continue; }
        if (k < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) break;
        rfb_pump(r);
        sleep_ms(5);
    }
    return sent == sizeof(m);
}

static unsigned long le32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

/* Is the BMP at path complete (st_size == bfSize, which gui.cc writes
 * first)? 1 complete, 0 not yet, -1 malformed. */
static int snapshot_complete(const char *path)
{
    size_t len = 0;
    char *b = read_file(path, &len);
    int rc = 0;
    if (!b) return 0;
    if (len >= 54) {
        const unsigned char *u = (const unsigned char *)b;
        if (u[0] != 'B' || u[1] != 'M') rc = -1;
        else if (len == le32(u + 2)) rc = 1;
        else if (len > le32(u + 2)) rc = -1;
    }
    free(b);
    return rc;
}

/* Convert gui.cc's 8bpp bottom-up BMP (54-byte header, 256 BGRx palette,
 * 4-byte-aligned rows) to a top-down exact-RGB P6. *blank is set when every
 * index is 0 (vga.cc update() skips a refresh that lands in its emulated
 * vertical retrace, leaving the zeroed snapshot buffer: retry, never grade). */
static bool snapshot_to_ppm(const char *bmp, const char *ppm, bool *blank)
{
    size_t len = 0;
    unsigned char *u = (unsigned char *)read_file(bmp, &len);
    bool ok = false;
    *blank = false;
    if (!u || len < 54 + 1024) goto out;
    unsigned long off = le32(u + 10), w = le32(u + 18), h = le32(u + 22);
    unsigned bpp = (unsigned)u[28] | ((unsigned)u[29] << 8);
    if (bpp != 8 || w == 0 || h == 0 || w > 4096 || h > 4096 || off != 54 + 1024)
        goto out;
    unsigned long rlen = (w + 3) & ~3ul;
    if (len < off + rlen * h) goto out;
    *blank = true;
    for (unsigned long i = 0; i < rlen * h; i++)
        if (u[off + i]) { *blank = false; break; }
    FILE *f = fopen(ppm, "wb");
    if (!f) goto out;
    fprintf(f, "P6\n%lu %lu\n255\n", w, h);
    ok = true;
    for (unsigned long y = 0; ok && y < h; y++) {
        const unsigned char *row = u + off + (h - 1 - y) * rlen;
        for (unsigned long x = 0; ok && x < w; x++) {
            const unsigned char *pal = u + 54 + 4u * row[x];
            unsigned char rgb[3] = { pal[2], pal[1], pal[0] };
            ok = fwrite(rgb, 1, 3, f) == 3;
        }
    }
    ok = (fclose(f) == 0) && ok;
out:
    free(u);
    return ok;
}

/* ---- the run ------------------------------------------------------------- */

void bochs_result_free(BochsResult *out)
{
    if (!out) return;
    free(out->serial_text);
    out->serial_text = NULL;
}

/* One Bochs at a time per host (bead initech-d9tt). The rfb display plugin
 * listens on 5900 and Bochs 2.7 has no bochsrc knob for the port: a second
 * concurrent Bochs silently takes 5901, nobody unblocks it, and BOTH gates go
 * red ("RFB unblock failed" / "did not complete the smoke") -- reproduced
 * 2026-10-05 with test-boot-bochs run in two checkouts at once. So every run
 * holds an exclusive flock(2) on a host-wide lock file from before the fork
 * until Bochs is reaped. The path is the one `flock(1)` wrappers already use,
 * so a wrapped old harness and this one exclude each other. The wait is
 * bounded and fails loud (Rule 2): a harness run inside such a wrapper would
 * otherwise wait on its own parent for ever. Returns the lock fd or -1. */
#define BOCHS_HOST_LOCK_PATH    "/tmp/initech-bochs.lock"
#define BOCHS_HOST_LOCK_WAIT_MS (20 * 60 * 1000)

static int bochs_host_lock(void)
{
    int fd = open(BOCHS_HOST_LOCK_PATH, O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if (fd < 0) {
        fprintf(stderr, "bochs_run: cannot open %s: %s\n",
                BOCHS_HOST_LOCK_PATH, strerror(errno));
        return -1;
    }
    long long deadline = mono_ms() + BOCHS_HOST_LOCK_WAIT_MS;
    bool told = false;
    for (;;) {
#ifndef BOCHS_MUTATE_NO_HOST_LOCK
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) return fd;
#else
        return fd;                 /* mutant: run without the host lock */
#endif
        if (errno != EWOULDBLOCK && errno != EINTR) {
            fprintf(stderr, "bochs_run: flock %s: %s\n",
                    BOCHS_HOST_LOCK_PATH, strerror(errno));
            close(fd);
            return -1;
        }
        if (!told) {
            fprintf(stderr, "bochs_run: another Bochs run holds %s; waiting\n",
                    BOCHS_HOST_LOCK_PATH);
            told = true;
        }
        if (mono_ms() >= deadline) {
            fprintf(stderr, "bochs_run: %s still held after %d s -- another "
                    "Bochs run (or an outer flock around this one) never "
                    "released it\n", BOCHS_HOST_LOCK_PATH,
                    BOCHS_HOST_LOCK_WAIT_MS / 1000);
            close(fd);
            return -1;
        }
        sleep_ms(250);
    }
}

int bochs_run(const BochsConfig *cfg, BochsResult *out)
{
    if (!cfg || !out) return -1;
    memset(out, 0, sizeof(*out));
    if (!cfg->disk_path) {
        fprintf(stderr, "bochs_run: disk_path is required\n");
        return -1;
    }
    if (cfg->quit_after_count > 1 && (!cfg->quit_after || !cfg->allow_reboot)) {
        fprintf(stderr, "bochs_run: a repeated boot wait needs --quit-after and --allow-reboot\n");
        return -1;
    }
    if (!!cfg->keys_script != !!cfg->keys_after ||
        (cfg->keys_script && !*cfg->keys_script)) {
        fprintf(stderr, "bochs_run: nonempty --keys requires --keys-after (and conversely)\n");
        return -1;
    }
    for (const char *p = cfg->keys_script; p && *p;) {
        const char *end = strchr(p, ',');
        BochsTraceStep step;
        if (!bochs_trace_parse(p, end ? (size_t)(end - p) : strlen(p), &step) ||
            (end && !end[1])) {
            fprintf(stderr, "bochs_run: unsupported key script token\n");
            return -1;
        }
        p = end ? end + 1 : NULL;
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
    long data_cyl = -1;
    if (cfg->data_disk_path) {
        data_cyl = disk_cylinders(cfg->data_disk_path);
        if (data_cyl < 0) {
            fprintf(stderr, "bochs_run: %s size is not a whole 2x32 geometry\n",
                    cfg->data_disk_path);
            return -1;
        }
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
    snprintf(fn, sizeof(fn), "%s.rfb.pgm", name);
    if (join_path(out->rfb_pgm_path, sizeof(out->rfb_pgm_path), dir, fn) != 0) return -1;
    snprintf(fn, sizeof(fn), "%s.rfb.ppm", name);
    if (join_path(out->rfb_ppm_path, sizeof(out->rfb_ppm_path), dir, fn) != 0) return -1;
    snprintf(fn, sizeof(fn), "%s.snap.bmp", name);
    if (join_path(out->snap_bmp_path, sizeof(out->snap_bmp_path), dir, fn) != 0) return -1;
    snprintf(fn, sizeof(fn), "%s.snap.ppm", name);
    if (join_path(out->snap_ppm_path, sizeof(out->snap_ppm_path), dir, fn) != 0) return -1;
    char bochs_snap[BOCHS_PATH_MAX];     /* where gui.cc writes, cwd=dir */
    if (join_path(bochs_snap, sizeof(bochs_snap), dir, "snapshot.bmp") != 0) return -1;

    /* Screendump runs Bochs with cwd=dir: every path Bochs sees is absolute
     * (the report keeps the caller's relative spellings). */
    BochsConfig rcfg = *cfg;
    char *abs_dir = NULL, *abs_disk = NULL, *abs_data = NULL, *abs_bios = NULL,
         *abs_vga = NULL;
    char abs_serial[BOCHS_PATH_MAX], abs_bochsrc[BOCHS_PATH_MAX], abs_rcfile[BOCHS_PATH_MAX];
    const char *child_serial = out->serial_path, *child_rc = rc_path,
               *child_bochsrc = out->bochsrc_path;
    if (cfg->screendump_after) {
        abs_dir = realpath(dir, NULL);
        abs_disk = realpath(cfg->disk_path, NULL);
        abs_data = cfg->data_disk_path ? realpath(cfg->data_disk_path, NULL) : NULL;
        abs_bios = realpath(cfg->bios_path ? cfg->bios_path : BOCHS_DEFAULT_BIOS, NULL);
        abs_vga = realpath(cfg->vgabios_path ? cfg->vgabios_path : BOCHS_DEFAULT_VGABIOS, NULL);
        bool okp = abs_dir && abs_disk && abs_bios && abs_vga &&
                   (!cfg->data_disk_path || abs_data);
        snprintf(fn, sizeof(fn), "%s.serial", name);
        okp = okp && join_path(abs_serial, sizeof(abs_serial), abs_dir, fn) == 0;
        snprintf(fn, sizeof(fn), "%s.rc", name);
        okp = okp && join_path(abs_rcfile, sizeof(abs_rcfile), abs_dir, fn) == 0;
        snprintf(fn, sizeof(fn), "%s.bochsrc", name);
        okp = okp && join_path(abs_bochsrc, sizeof(abs_bochsrc), abs_dir, fn) == 0;
        if (!okp) {
            fprintf(stderr, "bochs_run: cannot resolve absolute paths for the screendump run\n");
            free(abs_dir); free(abs_disk); free(abs_data); free(abs_bios); free(abs_vga);
            return -1;
        }
        rcfg.disk_path = abs_disk;
        rcfg.data_disk_path = abs_data;
        rcfg.bios_path = abs_bios;
        rcfg.vgabios_path = abs_vga;
        child_serial = abs_serial;
        child_rc = abs_rcfile;
        child_bochsrc = abs_bochsrc;
    }

    /* Remove a stale serial (so a launch failure cannot show old content) and
     * a stale disk lock (a previous killed run leaves <disk>.lock). */
    remove(out->serial_path);
    remove(out->rfb_pgm_path);   /* a failed capture must not leave stale pixels */
    remove(out->rfb_ppm_path);
    remove(out->snap_bmp_path);
    remove(out->snap_ppm_path);
    remove(bochs_snap);
    {
        char lock[BOCHS_PATH_MAX];
        if (snprintf(lock, sizeof(lock), "%s.lock", cfg->disk_path) < (int)sizeof(lock))
            remove(lock);
        if (cfg->data_disk_path &&
            snprintf(lock, sizeof(lock), "%s.lock", cfg->data_disk_path) < (int)sizeof(lock))
            remove(lock);
    }

    /* rc file: auto-continue the debugger build. */
    {
        FILE *f = fopen(rc_path, "wb");
        if (!f) { fprintf(stderr, "bochs_run: cannot write %s\n", rc_path); return -1; }
        fputs("c\n", f);
        fclose(f);
    }
    int wrc = write_bochsrc(out->bochsrc_path, &rcfg, child_serial, cyl, data_cyl);
    free(abs_disk); free(abs_data); free(abs_bios); free(abs_vga);
    if (wrc != 0) {
        free(abs_dir);
        fprintf(stderr, "bochs_run: cannot write %s\n", out->bochsrc_path);
        return -1;
    }

    /* SIGPIPE off: a dropped RFB socket must not kill the harness. */
    signal(SIGPIPE, SIG_IGN);

    int host_lock = bochs_host_lock();
    if (host_lock < 0) { free(abs_dir); return -1; }

    pid_t pid = fork();
    if (pid < 0) {
        close(host_lock);
        free(abs_dir);
        fprintf(stderr, "bochs_run: fork failed: %s\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        /* Child: stdout+stderr -> log, stdin <- /dev/null. */
        int logfd = open(out->log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        int nullfd = open("/dev/null", O_RDONLY);
        if (logfd >= 0) { dup2(logfd, 1); dup2(logfd, 2); }
        if (nullfd >= 0) dup2(nullfd, 0);
        /* A numeric time0 is localtime()d by Bochs 2.7 cmos.cc: pin UTC so
         * the guest RTC matches qemu -rtc base=<ISO> (UTC). */
        if (cfg->rtc_time0 > 0) setenv("TZ", "UTC", 1);
        if (abs_dir && chdir(abs_dir) != 0) _exit(126);
        char *argv[] = {
            (char *)BOCHS_BIN, (char *)"-q",
            (char *)"-f", (char *)child_bochsrc,
            (char *)"-rc", (char *)child_rc,
            NULL
        };
        execvp(BOCHS_BIN, argv);
        _exit(127);   /* exec failed */
    }

    free(abs_dir);               /* the child has its own copy */
    out->launched = true;
    long long start = mono_ms();
    long long deadline = start + timeout_ms;

    /* Unblock the RFB wait so Bochs proceeds (give it part of the budget to
     * open the port). */
    int handshook = 0;
    long long rfb_deadline = start + (timeout_ms / 2 < 10000 ? 10000 : timeout_ms / 2);
    int rfb_fd = rfb_unblock(port, rfb_deadline, &handshook);
    out->rfb_unblocked = (handshook != 0);
    RfbShadow rfb;
    memset(&rfb, 0, sizeof(rfb));
    rfb.fd = rfb_fd;

    /* Poll the SAME capture across guest RESET; never restart Bochs or clear
     * serial at reset. Ref: qemu.c wait_for_serial_marker_live and the locked
     * power traces. Missing completion is RED, even if expect_marker arrived.
     * Without quit_after the original deadline-driven boot legs are unchanged.
     *
     * Screendump (initech-u9gf): after the marker, request a full frame; once
     * one arrives (and a settle interval of guest run time has passed), keep
     * requesting full frames until two consecutive ones are byte-identical.
     * Only that stable frame is written: a frame torn by a paint in flight
     * cannot be graded (Law 2). */
    enum { SD_WAIT, SD_FIRST, SD_STABLE, SD_SNAP, SD_DONE } sd = SD_WAIT;
    unsigned long want_full = 0;
    long long sd_t = 0;
    unsigned char *snap = NULL;
    int status = 0;
    bool reaped = false;
    const char *key_next = cfg->keys_script;
    bool keys_started = false, key_down = false;
    long long key_at = 0;
    int pointer_x = 320, pointer_y = 240;
    unsigned pointer_buttons = 0;
    for (;;) {
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) { reaped = true; break; }
        if (w < 0 && errno != EINTR) break;
        rfb_pump(&rfb);
        if (key_next && !keys_started) {
            char *txt = read_file(out->serial_path, NULL);
            keys_started = txt && serial_marker_count(txt, cfg->keys_after) > 0;
            free(txt);
        }
        if (key_next && keys_started && rfb.have_init && mono_ms() >= key_at) {
            const char *end = strchr(key_next, ',');
            BochsTraceStep step;
            if (!bochs_trace_parse(key_next, end ? (size_t)(end - key_next) : strlen(key_next), &step)) break;
            int delay = 80;
            if (step.kind == BX_KEY) {
                if (step.ctrl && !key_down && !rfb_key_event(&rfb, 0xffe3, true)) break;
                if (!rfb_key_event(&rfb, rfb_key(key_next, end ? (size_t)(end - key_next) : strlen(key_next)), !key_down)) break;
                key_down = !key_down;
                if (step.ctrl && !key_down && !rfb_key_event(&rfb, 0xffe3, false)) break;
                delay = 30;
            } else if (step.kind == BX_WAIT) delay = step.delay;
            else {
                if (step.kind == BX_MOVE) {
                    pointer_x += step.x; pointer_y += step.y;
                    if (pointer_x < 0) pointer_x = 0;
                    if (pointer_x > 639) pointer_x = 639;
                    if (pointer_y < 0) pointer_y = 0;
                    if (pointer_y > 479) pointer_y = 479;
                } else pointer_buttons = (unsigned)step.x;
                /* RFB's 32-pixel Bochs header is outside guest coordinates. */
                unsigned y = (unsigned)pointer_y + 32u, x = (unsigned)pointer_x;
                if (!rfb_pointer_event(&rfb, x, y, pointer_buttons)) break;
            }
            if (!key_down) key_next = end ? end + 1 : NULL;
            key_at = mono_ms() + delay;
        }
        if (cfg->quit_after || (cfg->screendump_after && sd == SD_WAIT)) {
            char *txt = read_file(out->serial_path, NULL);
            bool complete = cfg->quit_after && txt &&
                            serial_completion(txt, cfg->quit_after,
                                              cfg->quit_after_count);
            if (cfg->screendump_after && sd == SD_WAIT && !key_next && txt &&
                serial_marker_count(txt, cfg->screendump_after) > 0) {
                out->screendump_marker_found = true;
                if (rfb_request_full(&rfb)) {
                    want_full = rfb.full_frames + 1;
                    sd_t = mono_ms() + 1000;
                    sd = SD_FIRST;
                } else {
                    rfb_fail(&rfb, "cannot send FramebufferUpdateRequest");
                }
            }
            free(txt);
            if (complete && !cfg->screendump_after && !key_next) break;
        }
        if (sd == SD_FIRST && rfb.full_frames >= want_full && mono_ms() >= sd_t) {
            free(snap);
            snap = malloc((size_t)rfb.w * rfb.h);
            if (!snap) { rfb_fail(&rfb, "out of memory"); break; }
            memcpy(snap, rfb.fb, (size_t)rfb.w * rfb.h);
            if (!rfb_request_full(&rfb)) break;
            want_full = rfb.full_frames + 1;
            sd_t = mono_ms() + 500;
            sd = SD_STABLE;
        } else if (sd == SD_STABLE && rfb.full_frames >= want_full && mono_ms() >= sd_t) {
            if (memcmp(snap, rfb.fb, (size_t)rfb.w * rfb.h) == 0) {
                out->screendump_ok =
                    rfb_write_capture(rfb.fb, rfb.w, rfb.h,
                                      out->rfb_pgm_path, out->rfb_ppm_path);
                if (!out->screendump_ok) break;
                /* The lossless twin, taken while the frame is stable. The
                 * guest width is the RFB window minus nothing Bochs reports;
                 * the only VBE geometry the guest may set here is 640. */
                if (!rfb_click_header(&rfb, 640)) break;
                out->snapshot_tries = 1;
                sd_t = mono_ms() + 15000;
                sd = SD_SNAP;
                continue;
            }
            memcpy(snap, rfb.fb, (size_t)rfb.w * rfb.h);
            if (!rfb_request_full(&rfb)) break;
            want_full = rfb.full_frames + 1;
            sd_t = mono_ms() + 500;
        }
        if (sd == SD_SNAP) {
            int c = snapshot_complete(bochs_snap);
            if (c < 0) break;                    /* malformed: RED, not graded */
            if (c > 0) {
                bool blank = false;
                if (rename(bochs_snap, out->snap_bmp_path) != 0) break;
                out->snapshot_ok = snapshot_to_ppm(out->snap_bmp_path,
                                                   out->snap_ppm_path, &blank);
                if (out->snapshot_ok && blank && out->snapshot_tries < 3) {
                    out->snapshot_ok = false;    /* retrace-skipped refresh */
                    remove(out->snap_ppm_path);
                    if (!rfb_click_header(&rfb, 640)) break;
                    out->snapshot_tries++;
                    sd_t = mono_ms() + 15000;
                } else {
                    sd = SD_DONE;
                    break;
                }
            } else if (mono_ms() >= sd_t) {
                break;
            }
        }
        if (rfb.err[0] && cfg->screendump_after) break;
        if (mono_ms() >= deadline) {
            out->timed_out = true;
            break;
        }
        sleep_ms(sd == SD_WAIT && !key_next ? 100 : 20);
    }
    free(snap);
    out->rfb_width = rfb.w;
    out->rfb_height = rfb.h;
    out->rfb_full_frames = rfb.full_frames;
    snprintf(out->rfb_error, sizeof(out->rfb_error), "%s", rfb.err);
    free(rfb.rx);
    free(rfb.fb);

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
    close(host_lock);              /* Bochs is reaped: the port is free again */

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
        if (cfg->data_disk_path &&
            snprintf(lock, sizeof(lock), "%s.lock", cfg->data_disk_path) < (int)sizeof(lock))
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

    if (cfg->quit_after && out->serial_text) {
        out->quit_markers_seen = serial_marker_count(out->serial_text, cfg->quit_after);
        out->quit_marker_found = serial_completion(out->serial_text, cfg->quit_after,
                                                   cfg->quit_after_count);
    }

    out->ok = out->launched && out->rfb_unblocked && !out->triple_fault &&
              (cfg->keys_script ? (!key_next && !rfb.err[0]) : true) &&
              (cfg->expect_marker ? out->marker_found : true) &&
              (cfg->quit_after ? out->quit_marker_found : true) &&
              (cfg->screendump_after ? (out->screendump_ok && out->snapshot_ok) : true);
    return 0;
}
