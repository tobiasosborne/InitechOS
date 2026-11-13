/* Canonical host assertion API. Ref: CLAUDE.md, The constitution / oracle. */
#ifndef SEED_TEST_ASSERT_H
#define SEED_TEST_ASSERT_H

#include <stdio.h>
#include <string.h>

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
     printf("INITECH_CHECKS %d failures=%d\n", g_checks, g_fails), \
     (g_fails == 0) ? 0 : 1)

#endif /* SEED_TEST_ASSERT_H */
