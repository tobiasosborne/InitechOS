/* env.h -- InitechDOS COMMAND.COM master environment store.
 *
 * beads: initech-1i0x (Tranche E increment 1).
 * Ref:   Ralf Brown's Interrupt List INT 21h AH=4Bh EXEC parameter block
 *        (env_block field = segment of the environment block passed to the
 *        child process; 0 = inherit parent); PSP offset 2Ch (env_seg word,
 *        the segment address of the process environment block).
 *        DOS environment block format: a sequence of ASCIIZ "NAME=VALUE\0"
 *        strings terminated by an additional NUL byte (a single 0x00 for the
 *        empty environment).  The DOS 3.0+ trailing WORD (count=1) and
 *        program pathname string are OUT OF SCOPE for this increment (a later
 *        increment will append them during EXEC).
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * HOST-TESTABILITY SEAM: all env_* functions are PURE (no asm, no I/O), so
 * the SAME TU compiles HOSTED for os/initechdos/test_env.c.  The test file
 * #includes env.c directly (matching the test_command.c idiom) and drives
 * every function.
 *
 * MUTATION hooks (CLAUDE.md Rule 6):
 *   ENV_MUTATE_NO_UPCASE  -- env_set skips name upcasing, so "path" stays
 *                            "path" instead of "PATH"; the upcase test goes
 *                            RED.
 *   ENV_MUTATE_NO_DEDUP   -- env_set appends without removing the old entry,
 *                            so a double SET leaves two entries; the UPSERT /
 *                            duplicate test goes RED.
 */
#ifndef INITECH_ENV_H
#define INITECH_ENV_H

#include <stdint.h>
#include "entry.h"

/* Maximum size of the packed environment arena (all "NAME=VALUE\0" entries
 * concatenated, excluding the final extra terminator) for a fixed store
 * initialized without a SHELL environment size. */
#define ENV_ARENA_MAX 512

/* Bytes a store can hold. A secondary COMMAND /E:nnnnn takes up to 32768
 * (MS-DOS 3.3 Reference p.47); the shell can give a child what fits the
 * 4096-byte EXEC environment block with its count word and a full program
 * path, so a larger /E is honoured up to this size. */
#define ENV_STORE_MAX COMMAND_ENV_MAX_BYTES

/* The environment store: a fixed-capacity packed buffer of ASCIIZ entries of
 * the form "NAME=VALUE\0".  `len` is the number of bytes currently occupied
 * (does NOT include the final extra NUL terminator; that is appended only by
 * env_serialize). An empty store has len == 0. The active capacity defaults
 * to ENV_ARENA_MAX. A primary processor may grow up to growth_limit.
 *
 * Internal invariant: no duplicate names (env_set enforces UPSERT); names are
 * always stored upper-cased (DOS upcases environment variable names). */
typedef struct {
    uint8_t buf[ENV_STORE_MAX];
    int     len;
    int     capacity;
    int     growth_limit;
} env_store_t;

/* ---- API ----------------------------------------------------------------- */

/* Initialize the store to empty (len = 0).  Must be called before any other
 * operation.  Safe to call more than once (idempotent reset). */
void env_init(env_store_t *e);

/* Initialize the primary store to its CONFIG.SYS size in whole paragraphs,
 * including the final terminator. It can grow up to the EXEC storage limit. */
void env_init_primary(env_store_t *e, uint16_t bytes);

/* UPSERT a variable.  `name` is upper-cased before storage (DOS semantics);
 * `value` is stored verbatim.  If a variable with the same name (case-
 * insensitive) already exists, the old entry is removed and the new
 * "NAME=VALUE\0" entry is appended, so there is never more than one binding
 * for a given name.
 *
 * Returns 1 on success.  Returns 0 -- and leaves the store BYTE-IDENTICAL --
 * if the new entry would not fit in the active capacity (Rule 2: never
 * truncate; fail loud by returning 0). */
int env_set(env_store_t *e, const char *name, const char *value);

/* Look up a variable by name (case-insensitive).  Returns a pointer to the
 * value portion (the char immediately after '=') of the matching entry inside
 * e->buf, or NULL if the name is not present.  The pointer is valid until the
 * next mutating call (env_set / env_unset / env_init). */
const char *env_get(const env_store_t *e, const char *name);

/* Remove the entry for `name` (case-insensitive).  Returns 1 if the entry
 * was found and removed, 0 if it was absent (idempotent). */
int env_unset(env_store_t *e, const char *name);

/* Return the number of entries currently in the store. */
int env_count(const env_store_t *e);

/* Return a pointer to the idx-th raw "NAME=VALUE" ASCIIZ string (0-based),
 * for use by the SET built-in when listing all variables.  Returns NULL if
 * idx is out of range (>= env_count(e)).  The pointer is valid until the
 * next mutating call. */
const char *env_entry(const env_store_t *e, int idx);

/* Serialize the store into the DOS environment block format: each entry as
 * "NAME=VALUE\0", followed by one additional NUL byte (the block terminator).
 * An empty store emits exactly one byte (0x00).
 *
 * Writes at most `cap` bytes to `out`.  Returns the number of bytes written
 * on success, or 0 if the output would not fit in `cap` bytes (Rule 2: fail
 * loud, leave `out` in an unspecified state; the caller must re-check).
 * The minimum useful `cap` is ENV_ARENA_MAX + 1 (arena + terminator). */
int env_serialize(const env_store_t *e, uint8_t *out, int cap);

#endif /* INITECH_ENV_H */
