/* initech-7r2; PRD Sec 8 serial milestones; CLAUDE.md Rules 1, 2, 6.
 * Exercise the real marker wait with a scripted serial producer and virtual
 * monotonic time. Host scheduling cannot make this regression test flaky. */
#define _POSIX_C_SOURCE 200809L
#include <time.h>
static int progress_clock(clockid_t, struct timespec *);
static int progress_sleep(const struct timespec *, struct timespec *);
#define clock_gettime progress_clock
#define nanosleep progress_sleep
#include "qemu.c"
#undef clock_gettime
#undef nanosleep

static long long now_ms;
static long long next_echo;
static long long marker_at;
static const char *marker_text;
static FILE *serial;
static const clockid_t guest_clock = (clockid_t)12345;
static int cpu_divisor = 1;

static int progress_clock(clockid_t id, struct timespec *ts)
{
    long long time_ms = id == guest_clock ? now_ms / cpu_divisor : now_ms;
    ts->tv_sec = time_ms / 1000;
    ts->tv_nsec = (time_ms % 1000) * 1000000L;
    return 0;
}

static int progress_sleep(const struct timespec *req, struct timespec *rem)
{
    (void)rem;
    now_ms += req->tv_sec * 1000 + req->tv_nsec / 1000000;
    if (next_echo && now_ms >= next_echo) {
        fputc('k', serial);
        fflush(serial);
        next_echo += 75;
    }
    if (marker_at && now_ms >= marker_at) {
        fputs(marker_text, serial);
        fflush(serial);
        marker_at = 0;
    }
    return 0;
}

static int check(const char *path, int fd, const char *label,
                 int echoes, long long at, const char *text, int count,
                 int expected, long long latest)
{
    rewind(serial);
    if (ftruncate(fileno(serial), 0)) return 1;
    now_ms = 0;
    next_echo = echoes ? 75 : 0;
    marker_at = at;
    marker_text = text;
    int hit = wait_for_serial_marker_live(fd, path, "SHELL-DONE", 100, count,
                                          120000, &guest_clock);
    if (hit != expected || now_ms > latest || (!expected && now_ms < latest)) {
        fprintf(stderr, "RED qemu-progress: %s (seen=%d at %lld ms)\n",
                label, hit, now_ms);
        return 1;
    }
    return 0;
}

int main(void)
{
    char path[] = "build/qemu-progress-XXXXXX";
    int file = mkstemp(path), pair[2];
    if (file < 0 || socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) return 2;
    serial = fdopen(file, "w+");
    if (!serial || fcntl(pair[0], F_SETFL, O_NONBLOCK) < 0) return 2;
    int bad = 0;
    bad |= check(path, pair[0], "echoes must renew the completion deadline",
                 1, 375, "SHELL-DONE", 1, 1, 375);
    bad |= check(path, pair[0], "missing final key must expire without a marker",
                 0, 0, "", 1, 0, 100);
    bad |= check(path, pair[0], "a partial marker must not satisfy completion",
                 0, 75, "SHELL-DON", 1, 0, 175);
    bad |= check(path, pair[0], "the requested marker count is still required",
                 0, 75, "SHELL-DONE", 2, 0, 175);
    bad |= check(path, pair[0], "serial chatter must not defeat the 120 s ceiling",
                 1, 0, "", 1, 0, 120000);
    cpu_divisor = 20;
    bad |= check(path, pair[0], "host starvation must not spend the guest CPU budget",
                 0, 1500, "SHELL-DONE", 1, 1, 1500);
    bad |= check(path, pair[0], "a CPU-starved hang must still expire",
                 0, 0, "", 1, 0, 2000);
    cpu_divisor = 1000000;
    bad |= check(path, pair[0], "a halted guest must still hit the wall ceiling",
                 0, 0, "", 1, 0, 120000);
    cpu_divisor = 1;
    close(pair[1]);
    bad |= check(path, pair[0], "a dead QMP peer must fail immediately",
                 0, 0, "", 1, 0, 0);
    fclose(serial);
    close(pair[0]);
    unlink(path);
    if (bad) return 1;
    puts(">>> test-harness-progress: green (serial progress renews the deadline; missing/partial/count/ceiling/exit stay bounded)");
    return 0;
}
