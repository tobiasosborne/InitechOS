#ifndef INITECH_ORACLE_POLICY_H
#define INITECH_ORACLE_POLICY_H
/* Factory only. Ref: CLAUDE.md Law 2 / Rule 2; initech-iv8c.
 * Missing references are RED unless the named opt-out is exactly 1.
 * An opt-out never excuses a mismatch, corrupt golden, or harness I/O error. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void oracle_failure(const char *format, ...)
{
    va_list args;
    fprintf(stderr, "!!! oracle FAIL: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(2);
}

static inline void oracle_missing(const char *root, const char *format, ...)
{
    char knob[80];
    const char *skip;
    va_list args;
    snprintf(knob, sizeof knob, "SKIP_%s", root);
    skip = getenv(knob);
    if (skip && strcmp(skip, "1") == 0)
        fprintf(stderr, "!!! %s SKIPPED (%s=1 opt-out) -- reference NOT graded: ",
                root, knob);
    else
        fprintf(stderr, "!!! %s FAIL: missing reference; set %s=1 to explicitly opt out: ",
                root, knob);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    if (!skip || strcmp(skip, "1") != 0) exit(2);
}
/* A missing script exits 2 in Python, which is also a real parse error.
 * Check presence first so only absence can take the explicit tool opt-out. */
static inline int oracle_reader_available(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    oracle_missing("REFERENCE_TOOLS", "%s absent\n", path);
    return 0;
}
#endif
