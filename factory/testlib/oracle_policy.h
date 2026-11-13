#ifndef INITECH_ORACLE_POLICY_H
#define INITECH_ORACLE_POLICY_H
/* Missing references fail, as required by CLAUDE.md, The constitution. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
static inline void oracle_failure(const char *format, ...)
{
    va_list args;
    fprintf(stderr, "!!! oracle FAIL: ");
    va_start(args, format); vfprintf(stderr, format, args); va_end(args);
    exit(2);
}
static inline void oracle_missing(const char *root, const char *format, ...)
{
    va_list args;
    fprintf(stderr, "!!! missing reference; set %s: ", root);
    va_start(args, format); vfprintf(stderr, format, args); va_end(args);
    exit(2);
}
static inline int oracle_reader_available(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    oracle_missing("REFERENCE_TOOLS", "%s absent\n", path);
    return 0;
}
#endif
