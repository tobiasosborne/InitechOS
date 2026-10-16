/* Rule 2 probes: only SIGABRT counts, never a null dereference or a hang.
 * Ref: test_region.c's forked invariant probes; factory only. */
#ifndef INITECH_EXPECT_ABORT_H
#define INITECH_EXPECT_ABORT_H
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <string.h>
#define EXPECT_SUCCESS(why, ...) do { \
    (void)fflush(NULL); \
    int probe_prior = g_fails; \
    pid_t probe_pid = fork(); \
    if (probe_pid == 0) { \
        struct rlimit probe_limit = { 0, 0 }; \
        (void)setrlimit(RLIMIT_CORE, &probe_limit); \
        alarm(10); \
        __VA_ARGS__; \
        (void)fflush(stdout); \
        _exit(g_fails == probe_prior ? 0 : 1); \
    } \
    int probe_status = 0; \
    if (probe_pid > 0) waitpid(probe_pid, &probe_status, 0); \
    CHECK(probe_pid > 0 && WIFEXITED(probe_status) && WEXITSTATUS(probe_status) == 0, why); \
} while (0)
#define EXPECT_MARKER(statement, marker, why) do { \
    int probe_pipe[2]; \
    if (pipe(probe_pipe) != 0) { CHECK(0, "marker probe pipe"); break; } \
    pid_t probe_pid = fork(); \
    if (probe_pid == 0) { \
        close(probe_pipe[0]); \
        dup2(probe_pipe[1], STDERR_FILENO); close(probe_pipe[1]); \
        statement; \
        _exit(0); \
    } \
    close(probe_pipe[1]); \
    char probe_buf[256]; size_t probe_n = 0; ssize_t probe_got; \
    while (probe_n < sizeof probe_buf - 1 && \
           (probe_got = read(probe_pipe[0], probe_buf + probe_n, sizeof probe_buf - 1 - probe_n)) > 0) \
        probe_n += (size_t)probe_got; \
    probe_buf[probe_n] = 0; close(probe_pipe[0]); \
    int probe_status = 0; \
    if (probe_pid > 0) waitpid(probe_pid, &probe_status, 0); \
    CHECK(probe_pid > 0 && WIFEXITED(probe_status) && WEXITSTATUS(probe_status) == 0 && \
          strcmp(probe_buf, marker) == 0, why); \
} while (0)
#define EXPECT_ABORT(statement, why) do { \
    pid_t probe_pid = fork(); \
    if (probe_pid == 0) { \
        struct rlimit probe_limit = { 0, 0 }; \
        (void)setrlimit(RLIMIT_CORE, &probe_limit); \
        alarm(3); \
        statement; \
        _exit(0); \
    } \
    int probe_status = 0; \
    CHECK(probe_pid > 0, "panic probe fork"); \
    if (probe_pid > 0) { \
        (void)waitpid(probe_pid, &probe_status, 0); \
        CHECK(WIFSIGNALED(probe_status) && WTERMSIG(probe_status) == SIGABRT, why); \
    } \
} while (0)
#endif
