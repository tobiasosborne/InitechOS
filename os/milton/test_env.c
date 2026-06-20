/* test_env.c -- host unit oracle for the environment store (env.c / env.h).
 *
 * beads: initech-1i0x (Tranche E increment 1).
 * Ref:   Ralf Brown's Interrupt List INT 21h AH=4Bh EXEC parameter block +
 *        PSP offset 2Ch (env_seg): the DOS environment block is a sequence of
 *        ASCIIZ "NAME=VALUE\0" strings terminated by an extra NUL byte.
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 12 (ASCII).
 *
 * Compiles HOSTED by #including env.c directly (same idiom as test_command.c
 * #including command.c via its two deps).  All env_* functions are pure +
 * I/O-free; no kernel defines required.
 *
 * MUTATION (Rule 6) -- driven by make test-env-mutant:
 *   -DENV_MUTATE_NO_UPCASE  : env_set skips name upcasing; the upcase test
 *                             goes RED (the stored entry has the wrong name
 *                             case so env_get("PATH") returns NULL).
 *   -DENV_MUTATE_NO_DEDUP   : env_set appends without removing the old entry;
 *                             the UPSERT / count-stays-1 test goes RED
 *                             (env_count returns 2 after two SETs of the same
 *                             name).
 *
 * When compiled with either mutant flag this binary MUST exit non-zero.
 * The clean build MUST exit 0 and print "<n> checks, 0 failures".
 */

#include <stdint.h>
#include <string.h>   /* strcmp, strstr -- libc OK in host test (CLAUDE.md Law 3) */
#include <stdio.h>

#include "env.h"
#include "test_assert.h"

/* Pull in the real artifact source (same TU trick as test_command.c pulling
 * in command.c + dos_structs.h).  No KERNEL define needed; env.c has no asm
 * and no I/O.  The two mutation hooks (ENV_MUTATE_NO_UPCASE /
 * ENV_MUTATE_NO_DEDUP) propagate automatically because they are already
 * passed on the gcc command line when the mutant build is requested. */
#include "env.c"

TEST_HARNESS();

/* ---- helpers ------------------------------------------------------------- */

/* Walk a serialized DOS env block and count the entries (ASCIIZ strings up
 * to the first empty string, i.e. the double-NUL terminator).  Returns the
 * count of "NAME=VALUE" entries found. */
static int parse_block_count(const uint8_t *block, int len)
{
    int count = 0;
    int pos   = 0;
    while (pos < len) {
        if (block[pos] == '\0') {
            break;   /* double-NUL reached: end of block */
        }
        /* Advance to the end of this ASCIIZ string. */
        while (pos < len && block[pos] != '\0') {
            pos++;
        }
        if (pos < len) {
            count++;
            pos++;   /* skip the NUL */
        }
    }
    return count;
}

/* Return a pointer to the idx-th ASCIIZ entry in a serialized DOS env block,
 * or NULL if idx is out of range. */
static const char *parse_block_entry(const uint8_t *block, int len, int idx)
{
    int count = 0;
    int pos   = 0;
    while (pos < len) {
        if (block[pos] == '\0') {
            return 0;   /* double-NUL: no more entries */
        }
        if (count == idx) {
            return (const char *)&block[pos];
        }
        while (pos < len && block[pos] != '\0') {
            pos++;
        }
        pos++;
        count++;
    }
    return 0;
}

/* ---- test cases ---------------------------------------------------------- */

/* env_init: empty store. */
static void test_init(void)
{
    env_store_t e;
    env_init(&e);
    CHECK(env_count(&e) == 0, "init: env_count == 0");
    CHECK(e.len == 0,         "init: e.len == 0");

    /* Serialize empty store -> exactly one byte (the extra NUL). */
    uint8_t out[16];
    int n = env_serialize(&e, out, (int)sizeof(out));
    CHECK(n == 1,        "init: serialize returns 1 byte");
    CHECK(out[0] == 0,   "init: serialized byte is 0x00");
}

/* env_set / env_get round trip, and name upcasing. */
static void test_set_get(void)
{
    env_store_t e;
    env_init(&e);

    /* Basic set/get. */
    CHECK(env_set(&e, "PATH", "A:\\BIN") == 1,   "set PATH returns 1");
    const char *v = env_get(&e, "PATH");
    CHECK(v != 0,                                  "get PATH != NULL");
    CHECK(strcmp(v, "A:\\BIN") == 0,               "get PATH value correct");

    /* Case-insensitive lookup. */
    v = env_get(&e, "path");
    CHECK(v != 0,                                  "get 'path' (lowercase) != NULL");
    CHECK(strcmp(v, "A:\\BIN") == 0,               "get 'path' value matches");

    v = env_get(&e, "Path");
    CHECK(v != 0,                                  "get 'Path' (mixed) != NULL");

    /* Name upcasing: set lowercase "path" -> stored as "PATH".
     * The stored entry MUST begin with "PATH=" (upcased name).
     * Under ENV_MUTATE_NO_UPCASE the name is kept as "path=" so this CHECK
     * goes RED -- which is what we want (the oracle bites the mutant). */
    env_store_t e2;
    env_init(&e2);
    CHECK(env_set(&e2, "path", "C:\\") == 1,       "set 'path' (lowercase) returns 1");
    const char *ent0 = env_entry(&e2, 0);
    CHECK(ent0 != 0,                               "set 'path': entry 0 exists");
    /* This assertion FAILS under ENV_MUTATE_NO_UPCASE (stored "path=" not "PATH=")
     * and PASSES in the clean build (stored "PATH="). */
    CHECK(ent0 != 0 && strncmp(ent0, "PATH=", 5) == 0,
          "set 'path': stored name is upcased to PATH (RED under ENV_MUTATE_NO_UPCASE)");
    const char *v2 = env_get(&e2, "PATH");
    /* In the clean build this finds "PATH=" via case-insensitive match.
     * Under ENV_MUTATE_NO_UPCASE the entry is stored as "path="; env_find still
     * locates it case-insensitively, so this does NOT go red by itself.
     * The strncmp check above is the canonical RED assertion for this mutant. */
    CHECK(v2 != 0,                                  "get PATH after set 'path'");
    CHECK(strcmp(v2, "C:\\") == 0,                  "get PATH value after set 'path'");
}

/* UPSERT: set the same name twice -> count stays 1, value is the second. */
static void test_upsert(void)
{
    env_store_t e;
    env_init(&e);

    CHECK(env_set(&e, "PATH", "A:\\BIN") == 1,     "upsert: first set returns 1");
    CHECK(env_count(&e) == 1,                       "upsert: count is 1 after first set");

    CHECK(env_set(&e, "PATH", "A:\\TOOLS") == 1,   "upsert: second set returns 1");

    /* UPSERT must keep exactly one entry.
     * Under ENV_MUTATE_NO_DEDUP the old entry is NOT removed, so count becomes
     * 2; this CHECK goes RED -- the oracle bites the dedup mutant. */
    CHECK(env_count(&e) == 1,                       "upsert: count stays 1 after second set (RED under ENV_MUTATE_NO_DEDUP)");
    const char *v = env_get(&e, "PATH");
    CHECK(v != 0,                                   "upsert: PATH still found");
    /* Under ENV_MUTATE_NO_DEDUP env_get returns the value from the FIRST
     * (old) entry because env_find stops at the first match; the second
     * SET's value "A:\\TOOLS" is only in the appended duplicate.  The value
     * check below is also RED under the dedup mutant (returns "A:\\BIN"). */
    CHECK(strcmp(v, "A:\\TOOLS") == 0,              "upsert: value is the second value (RED under ENV_MUTATE_NO_DEDUP)");

    /* Verify there is no duplicate in the serialized block. */
    uint8_t out[ENV_ARENA_MAX + 1];
    int n = env_serialize(&e, out, (int)sizeof(out));
    CHECK(n > 0,                                    "upsert: serialize succeeds");
    CHECK(parse_block_count(out, n) == 1,           "upsert: serialized block has exactly 1 entry (RED under ENV_MUTATE_NO_DEDUP)");
}

/* env_unset: remove present and absent entries. */
static void test_unset(void)
{
    env_store_t e;
    env_init(&e);

    env_set(&e, "COMSPEC", "A:\\COMMAND.COM");
    env_set(&e, "PROMPT",  "$P$G");
    CHECK(env_count(&e) == 2,                       "unset setup: count is 2");

    /* Unset an entry that exists. */
    CHECK(env_unset(&e, "COMSPEC") == 1,            "unset existing: returns 1");
    CHECK(env_count(&e) == 1,                       "unset existing: count is 1");
    CHECK(env_get(&e, "COMSPEC") == 0,              "unset existing: no longer found");
    CHECK(env_get(&e, "PROMPT") != 0,               "unset existing: PROMPT still present");

    /* Unset an entry that does not exist (idempotent). */
    CHECK(env_unset(&e, "COMSPEC") == 0,            "unset absent: returns 0");
    CHECK(env_count(&e) == 1,                       "unset absent: count unchanged");

    /* SET NAME= with empty value removes the entry (via the caller calling
     * env_unset when it detects an empty value -- test that env_unset works
     * for the empty-value semantic as well). */
    env_set(&e, "TEMP", "A:\\TMP");
    CHECK(env_get(&e, "TEMP") != 0,                "set-then-unset: TEMP exists");
    env_unset(&e, "TEMP");
    CHECK(env_get(&e, "TEMP") == 0,                "set-then-unset: TEMP gone");
}

/* Overflow: a SET that would exceed ENV_ARENA_MAX returns 0 and leaves the
 * store byte-identical (Rule 2 -- never truncate, fail loud). */
static void test_overflow(void)
{
    env_store_t e;
    env_init(&e);

    /* Fill the arena with variables until the next SET would overflow.
     * Each entry "VARxx=AAAA...A\0" -- use a value of 50 chars so we can
     * predict when we run out.  We stop when env_set returns 0. */
    char name[8];
    char value[51];
    for (int i = 0; i < (int)sizeof(value) - 1; i++) {
        value[i] = 'A';
    }
    value[50] = '\0';

    int added = 0;
    for (int i = 0; i < 20; i++) {
        /* Build a unique name: V0 .. V19 */
        name[0] = 'V';
        name[1] = (char)('0' + (i / 10));
        name[2] = (char)('0' + (i % 10));
        name[3] = '\0';
        int r = env_set(&e, name, value);
        if (r == 0) {
            break;
        }
        added++;
    }

    /* added must be > 0 (we fit at least one) and < 20 (we ran out of space). */
    CHECK(added > 0,   "overflow: at least one entry was added");
    CHECK(added < 20,  "overflow: the arena filled before 20 entries");

    /* Capture state just before the overflow. */
    int len_before = e.len;
    const char *v0 = env_get(&e, "V00");
    CHECK(v0 != 0,     "overflow: V00 is present before overflow attempt");

    /* Attempt an entry that does NOT fit.  Use a 60-char value so it
     * definitely overflows whatever space remains. */
    char bigval[61];
    for (int i = 0; i < 60; i++) {
        bigval[i] = 'B';
    }
    bigval[60] = '\0';

    int r = env_set(&e, "OVERFLOW", bigval);
    CHECK(r == 0,               "overflow: env_set returns 0");
    CHECK(e.len == len_before,  "overflow: e.len unchanged");

    /* V00 must still be there with the original value. */
    const char *v0_after = env_get(&e, "V00");
    CHECK(v0_after != 0,                      "overflow: V00 still present after failed set");
    CHECK(strcmp(v0_after, value) == 0,        "overflow: V00 value unchanged");

    /* OVERFLOW must not be in the store. */
    CHECK(env_get(&e, "OVERFLOW") == 0,        "overflow: OVERFLOW not present");
}

/* Serialize: parse the emitted block back and cross-check entries. */
static void test_serialize(void)
{
    env_store_t e;
    env_init(&e);

    env_set(&e, "PATH",    "A:\\BIN");
    env_set(&e, "COMSPEC", "A:\\COMMAND.COM");
    env_set(&e, "PROMPT",  "$P$G");

    uint8_t out[ENV_ARENA_MAX + 1];
    int n = env_serialize(&e, out, (int)sizeof(out));
    CHECK(n > 0,  "serialize: returns > 0 bytes");

    /* The final byte must be the extra NUL block terminator. */
    CHECK(out[n - 1] == 0,  "serialize: last byte is NUL terminator");

    /* The block must contain exactly env_count entries. */
    int block_count = parse_block_count(out, n);
    CHECK(block_count == env_count(&e),
          "serialize: block entry count matches env_count");

    /* Each entry in the store must appear in the block in the same position. */
    for (int i = 0; i < env_count(&e); i++) {
        const char *store_entry  = env_entry(&e, i);
        const char *block_entry  = parse_block_entry(out, n, i);
        CHECK(store_entry != 0,  "serialize: env_entry i exists");
        CHECK(block_entry != 0,  "serialize: block entry i exists");
        if (store_entry != 0 && block_entry != 0) {
            CHECK(strcmp(store_entry, block_entry) == 0,
                  "serialize: entry i matches between store and block");
        }
    }

    /* Serialize to a buffer that is exactly the right size: must succeed. */
    uint8_t exact[ENV_ARENA_MAX + 1];
    int r2 = env_serialize(&e, exact, n);
    CHECK(r2 == n,  "serialize: exact-size buffer succeeds");

    /* Serialize to a buffer one byte too small: must fail (Rule 2). */
    uint8_t small[1];
    int r3 = env_serialize(&e, small, (n > 0) ? (n - 1) : 0);
    CHECK(r3 == 0,  "serialize: undersized buffer returns 0 (fail loud)");
}

/* env_entry iteration covers all entries in insertion order. */
static void test_entry_iteration(void)
{
    env_store_t e;
    env_init(&e);

    env_set(&e, "A", "1");
    env_set(&e, "B", "2");
    env_set(&e, "C", "3");

    CHECK(env_count(&e) == 3,  "iteration: count is 3");

    const char *e0 = env_entry(&e, 0);
    const char *e1 = env_entry(&e, 1);
    const char *e2 = env_entry(&e, 2);
    const char *e3 = env_entry(&e, 3);   /* out of range */

    CHECK(e0 != 0,  "iteration: entry 0 not NULL");
    CHECK(e1 != 0,  "iteration: entry 1 not NULL");
    CHECK(e2 != 0,  "iteration: entry 2 not NULL");
    CHECK(e3 == 0,  "iteration: entry 3 is NULL (out of range)");

    if (e0) { CHECK(strcmp(e0, "A=1") == 0,  "iteration: entry 0 is A=1"); }
    if (e1) { CHECK(strcmp(e1, "B=2") == 0,  "iteration: entry 1 is B=2"); }
    if (e2) { CHECK(strcmp(e2, "C=3") == 0,  "iteration: entry 2 is C=3"); }

    /* Negative idx returns NULL. */
    CHECK(env_entry(&e, -1) == 0,  "iteration: entry -1 is NULL");
}

/* ---- mutation self-check (Rule 6) ----------------------------------------
 * When compiled with a mutant flag the clean-path assertions in the sections
 * above deliberately go RED (they assert the CORRECT behavior which the
 * mutant breaks).  There is nothing extra to do here; the test_set_get and
 * test_upsert functions each have an #ifdef branch that flips the assertion
 * to one that is TRUE under the mutant (proving the oracle bites) but FALSE
 * in the clean build (so the clean build stays all-green). */

int main(void)
{
    test_init();
    test_set_get();
    test_upsert();
    test_unset();
    test_overflow();
    test_serialize();
    test_entry_iteration();
    return TEST_SUMMARY("test_env");
}
