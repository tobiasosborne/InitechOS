/*
 * test_assert.h -- tiny zero-dependency assert harness for the seed front end.
 *
 * beads: initech-znb ("Step A of the InitechOS seed cross-compiler")
 * Ref:   CLAUDE.md Rule 1 (Red->Green->Refactor), Law 2 (the oracle is the
 *        truth), Rule 2 (fail fast, fail loud). A test binary must exit
 *        NON-ZERO if any check fails, so `make test-seed` can never false-green.
 *
 * No external framework (CLAUDE.md "no extra runtimes", Law 3). Each test
 * file defines a `static int g_checks` / `static int g_fails` pair via the
 * TEST_HARNESS() macro and calls CHECK(cond, msg). main() returns
 * test_summary(name) which prints a count line and yields a process exit code.
 *
 * ASCII-clean (CLAUDE.md Rule 12). No timestamps (Rule 11).
 */
#ifndef SEED_TEST_ASSERT_H
#define SEED_TEST_ASSERT_H

#include <stdio.h>

/* Place once at file scope in each test_*.c. */
#define TEST_HARNESS() \
    static int g_checks = 0; \
    static int g_fails = 0

/* CHECK(cond, msg): count a check; on failure print a located line. */
#define CHECK(cond, msg)                                                   \
    do {                                                                   \
        g_checks++;                                                        \
        if (!(cond)) {                                                     \
            g_fails++;                                                     \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__,      \
                    (msg));                                                \
        }                                                                  \
    } while (0)

/* CHECK_STR_EQ: convenience for string equality with both values shown. */
#define CHECK_STR_EQ(got, want, msg)                                       \
    do {                                                                   \
        g_checks++;                                                        \
        if (strcmp((got), (want)) != 0) {                                  \
            g_fails++;                                                     \
            fprintf(stderr, "  FAIL %s:%d: %s\n        got : %s\n"          \
                    "        want: %s\n", __FILE__, __LINE__, (msg),        \
                    (got), (want));                                        \
        }                                                                  \
    } while (0)

/* Call from main(): print a summary, return 0 if all green else 1. */
#define TEST_SUMMARY(name)                                                 \
    (printf("%s: %d checks, %d failures\n", (name), g_checks, g_fails),    \
     (g_fails == 0) ? 0 : 1)

#endif /* SEED_TEST_ASSERT_H */
