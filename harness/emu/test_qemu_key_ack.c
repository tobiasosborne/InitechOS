/* initech-ptzy; PRD Sec 6.3/8; spec/event_model.h Sec 5; Rule 6.
 * A scripted guest consumes each release after 175 virtual ms. QMP replies
 * and the old serial echo are already available: neither permits another key.
 * Exercise BOTH real injection loops without a scheduler-dependent sleep. */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <unistd.h>
static int ack_clock(clockid_t, struct timespec *);
static int ack_sleep(const struct timespec *, struct timespec *);
static int ack_poll(struct pollfd *, nfds_t, int);
static ssize_t ack_write(int, const void *, size_t);
#define clock_gettime ack_clock
#define nanosleep ack_sleep
#define poll ack_poll
#define write ack_write
#include "qemu.c"
#undef clock_gettime
#undef nanosleep
#undef poll
#undef write

static FILE *serial;
static long long now_ms, due;
static int writes, early, echo_enabled, lag_ms, implicit_holds;
static int qmp_reply_fd;
static const char echo[] = "FLAIR-EVT what=4 where=211,122 msg=00004D00\n";

static void advance(int ms)
{
    now_ms += ms;
    if (due && echo_enabled && now_ms >= due) {
        fputs(echo, serial);
        fflush(serial);
        due = 0;
    }
}
static int ack_clock(clockid_t id, struct timespec *ts)
{
    (void)id;
    ts->tv_sec = now_ms / 1000;
    ts->tv_nsec = now_ms % 1000 * 1000000;
    return 0;
}
static int ack_sleep(const struct timespec *ts, struct timespec *rem)
{
    (void)rem;
    advance((int)(ts->tv_sec * 1000 + ts->tv_nsec / 1000000));
    return 0;
}
static int ack_poll(struct pollfd *fds, nfds_t n, int ms)
{
    int ready = poll(fds, n, 0);
    if (ready) return ready;
    advance(ms);
    return 0;
}
static ssize_t ack_write(int fd, const void *data, size_t len)
{
    (void)fd;
    char cmd[1024];
    if (len >= sizeof cmd) return -1;
    memcpy(cmd, data, len); cmd[len] = 0;
    if (strstr(cmd, "send-key") || strstr(cmd, "\"type\":\"key\"")) {
        if (strstr(cmd, "send-key") || strstr(cmd, "\"down\":true")) {
            if (due) early++;
            writes++;
        }
        if (strstr(cmd, "send-key")) implicit_holds++;
        if (strstr(cmd, "send-key") || strstr(cmd, "\"down\":false"))
            due = now_ms + lag_ms;
    }
    /* QMP accepts the command at once; guest consumption is still delayed. */
    static const char reply[] = "{\"return\":{}}\n";
    if (write(qmp_reply_fd, reply, sizeof reply - 1) < 0) return -1;
    return (ssize_t)len;
}

static int run(const char *path, int fd, const char *label, int mouse,
               int paced, int echoes, int lag, int expected)
{
    rewind(serial);
    if (ftruncate(fileno(serial), 0)) return 1;
    fputs(echo, serial); fflush(serial); /* old echo MUST NOT satisfy key 1 */
    now_ms = due = 0;
    writes = early = implicit_holds = 0;
    echo_enabled = echoes;
    lag_ms = lag;
    KeyAckCtx ack = { path, paced ? "FLAIR-EVT what=4 " : NULL,
                      1000, 120000, NULL };
    int bad = 0;
    int sent = mouse == 1
        ? qmp_inject_mouse(fd, "kright,kright,kminus", NULL, NULL, path, &ack)
        : mouse == 2
        ? qmp_inject_mouse(fd, "Kright:1,Kright:0,Kright:1,Kright:0",
                           NULL, NULL, path, &ack)
        : qmp_inject_keys(fd, "right,right,minus", &bad, NULL, NULL, &ack);
    int fail = expected < 0 ? sent >= 0 || writes != 1
        : sent != expected || (paced && (early || due)) || (!paced && !early);
    if (paced && implicit_holds) {
        fprintf(stderr, "RED qemu-key-ack: paced stroke still uses QMP's implicit hold timer\n");
        fail = 1;
    }
    if (fail) fprintf(stderr, "RED qemu-key-ack: %s (sent=%d writes=%d early=%d at=%lld)\n",
                      label, sent, writes, early, now_ms);
    return fail;
}

int main(void)
{
    char path[] = "build/qemu-key-ack-XXXXXX";
    int file = mkstemp(path), pair[2];
    if (file < 0 || socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) return 2;
    serial = fdopen(file, "w+");
    if (!serial || fcntl(pair[0], F_SETFL, O_NONBLOCK) < 0) return 2;
    qmp_reply_fd = pair[1];
    int bad = 0;
    bad |= run(path, pair[0], "--keys must wait for each new guest release", 0, 1, 1, 175, 3);
    bad |= run(path, pair[0], "--mouse k must wait for each new guest release", 1, 1, 1, 175, 3);
    bad |= run(path, pair[0], "explicit K release must wait too", 2, 1, 1, 175, 4);
    bad |= run(path, pair[0], "fast echoes need no fixed input beat", 1, 1, 1, 10, 3);
    bad |= run(path, pair[0], "missing echo stops before key 2", 1, 1, 0, 175, -1);
    bad |= run(path, pair[0], "opt-out retains legacy injection", 1, 0, 0, 175, 3);
    close(pair[0]); close(pair[1]); fclose(serial); unlink(path);
    if (bad) return 1;
    puts(">>> test-harness-key-ack: green (both streams wait for new release echoes; explicit K, fast echo, missing echo and opt-out)");
    return 0;
}
