/* One python3 for all of a suite's independent-reader runs (factory C).
 * tests/oracles/serve.py runs each unchanged reader script as `python3
 * SCRIPT ARG...` would; this client replaces one interpreter start per run
 * (about 0.1 s each) with one per suite. Nothing here reads a reader's
 * output: callers parse and grade it exactly as before. Any transport
 * failure is a reader failure (status 2), never an empty success.
 * Paths are relative to the component directory, where tests/run.sh runs. */
#ifndef I123_ORACLE_SERVER_H
#define I123_ORACLE_SERVER_H
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static FILE *oracle_to, *oracle_from;
static pid_t oracle_pid;
static int oracle_dead;

/* LOTUS123_DECOMP is the readers' corpus; set before the first run. */
static int oracle_start(const char *corpus)
{
    int in[2], out[2];
    if (oracle_dead) return 0;
    if (oracle_to) return 1;
    if (setenv("LOTUS123_DECOMP", corpus, 1) || pipe(in) || pipe(out)) { oracle_dead = 1; return 0; }
    signal(SIGPIPE, SIG_IGN);
    fflush(NULL);
    oracle_pid = fork();
    if (oracle_pid < 0) { oracle_dead = 1; return 0; }
    if (!oracle_pid) {
        if (dup2(in[0], 0) < 0 || dup2(out[1], 1) < 0) _exit(2);
        close(in[0]); close(in[1]); close(out[0]); close(out[1]);
        execlp("python3", "python3", "tests/oracles/serve.py", "--serve", (char *)0);
        _exit(2);
    }
    close(in[0]); close(out[1]);
    oracle_to = fdopen(in[1], "w"); oracle_from = fdopen(out[0], "r");
    if (!oracle_to || !oracle_from) { oracle_dead = 1; return 0; }
    return 1;
}

/* Run SCRIPT with the NULL-terminated arguments; standard output goes to
 * the file OUT, or with OUT NULL into *inline_out (a rewound tmpfile the
 * caller closes). Returns the reader's exit status, 2 on transport failure. */
static int oracle_vrun(const char *corpus, const char *out, FILE **inline_out,
                       const char *script, va_list ap)
{
    unsigned status, size;
    const char *arg;
    if (inline_out) *inline_out = 0;
    if (!oracle_start(corpus)) return 2;
    fprintf(oracle_to, "%s\t%s", out ? out : "-", script);
    while ((arg = va_arg(ap, const char *)) != 0) fprintf(oracle_to, "\t%s", arg);
    fputc('\n', oracle_to);
    if (fflush(oracle_to) || fscanf(oracle_from, "%u %u", &status, &size) != 2 ||
        fgetc(oracle_from) != '\n') {
        oracle_dead = 1;
        return 2;
    }
    FILE *t = inline_out ? tmpfile() : 0;
    if (inline_out && !t) { oracle_dead = 1; return 2; }
    for (unsigned i = 0; i < size; i++) {
        int c = fgetc(oracle_from);
        if (c == EOF) { oracle_dead = 1; if (t) fclose(t); return 2; }
        if (t) fputc(c, t);
    }
    if (t) {
        if (fflush(t) || fseek(t, 0, SEEK_SET)) { fclose(t); return 2; }
        *inline_out = t;
    }
    return status > 255 ? 2 : (int)status;
}
static __attribute__((unused)) int oracle_file(const char *corpus, const char *out, const char *script, ...)
{
    va_list ap; va_start(ap, script);
    int rc = oracle_vrun(corpus, out, 0, script, ap);
    va_end(ap);
    return rc;
}
static __attribute__((unused)) int oracle_read(const char *corpus, FILE **result, const char *script, ...)
{
    va_list ap; va_start(ap, script);
    int rc = oracle_vrun(corpus, 0, result, script, ap);
    va_end(ap);
    return rc;
}
#endif
