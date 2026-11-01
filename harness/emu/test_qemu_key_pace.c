/* initech-1x6x2.1; PRD Sec 8; Rule 6. key_pace_wait against real processes:
 * a sleeping child stands in for a guest in hlt (the next stroke goes at once)
 * and a spinning child for a computing guest, sharing one CPU with a competing
 * spinner so that CPU time is NOT wall time (the next stroke waits for CPU the
 * "guest" actually received; a wall-clock pacer would send it after half). */
#define _GNU_SOURCE
#include "qemu.c"
#include <sched.h>

static void pin_last_cpu(void)
{
    cpu_set_t all, one;
    int cpu = -1;
    CPU_ZERO(&all);
    if (sched_getaffinity(0, sizeof(all), &all) != 0) { perror("affinity"); exit(2); }
    for (int i = 0; i < CPU_SETSIZE; i++) if (CPU_ISSET(i, &all)) cpu = i;
    CPU_ZERO(&one);
    CPU_SET(cpu, &one);
    if (sched_setaffinity(0, sizeof(one), &one) != 0) { perror("affinity"); exit(2); }
}

static pid_t child(int spin)
{
    pid_t p = fork();
    if (p < 0) { perror("fork"); exit(2); }
    if (p == 0) {
        pin_last_cpu();
        for (;;) { if (!spin) pause(); }
    }
    return p;
}

int main(void)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) { perror("socketpair"); return 2; }
    fcntl(sv[0], F_SETFL, O_NONBLOCK);
    long long far = mono_ms() + 30000;

    pid_t idle = child(0);
    clockid_t idle_clock;
    sleep_ms(50); /* let it reach pause() */
    if (clock_getcpuclockid(idle, &idle_clock) != 0) { perror("clock"); return 2; }
    KeyAckCtx a = { "", "", 0, far, &idle_clock, idle };
    long long t0 = mono_ms();
    key_pace_wait(sv[0], &a, guest_ms(&idle_clock));
    long long idle_wait = mono_ms() - t0;
    kill(idle, SIGKILL);
    waitpid(idle, NULL, 0);
    if (idle_wait > 1000) {
        printf("RED qemu-key-pace: an idle QEMU must not delay the next stroke (%lld ms)\n", idle_wait);
        return 1;
    }

    pid_t rival = child(1), busy = child(1);
    clockid_t busy_clock;
    sleep_ms(50); /* both pinned before the budget starts */
    if (clock_getcpuclockid(busy, &busy_clock) != 0) { perror("clock"); return 2; }
    KeyAckCtx b = { "", "", 0, far, &busy_clock, busy };
    long long cpu0 = guest_ms(&busy_clock);
    key_pace_wait(sv[0], &b, cpu0);
    long long got = guest_ms(&busy_clock) - cpu0;
    kill(busy, SIGKILL);
    kill(rival, SIGKILL);
    waitpid(busy, NULL, 0);
    waitpid(rival, NULL, 0);
    if (got < KEY_PACE_CPU_MS) {
        printf("RED qemu-key-pace: a busy QEMU must receive the CPU budget before the next stroke (%lld < %d ms)\n",
               got, KEY_PACE_CPU_MS);
        return 1;
    }
    printf("GREEN qemu-key-pace: idle QEMU releases in %lld ms; busy, starved QEMU got %lld ms CPU first\n",
           idle_wait, got);
    return 0;
}
