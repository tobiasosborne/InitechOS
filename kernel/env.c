/* env.c -- InitechDOS COMMAND.COM master environment store.
 *
 * beads: initech-1i0x (Tranche E increment 1).
 * Ref:   Ralf Brown's Interrupt List INT 21h AH=4Bh EXEC parameter block +
 *        PSP offset 2Ch (env_seg).  DOS environment block format: ASCIIZ
 *        "NAME=VALUE\0" entries, terminated by an extra NUL.
 *        CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Law 3
 *        (artifact = C), Rule 2 (fail loud), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 * No libc: hand-rolled upcase, strlen-alike, strcmp-alike below (mirroring
 * the freestanding discipline of command.c which does the same for is_space
 * and the classify table's hand-rolled strcmp).
 */

#include "env.h"

/* ---- freestanding helpers (no libc; same discipline as command.c) -------- */

/* Upper-case one ASCII byte (a-z -> A-Z); other bytes pass through.
 * DOS upcases environment variable names (same rule as command + filename). */
static char env_upcase_char(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

/* Return the byte-length of an ASCIIZ string (no NUL included). */
static int env_strlen(const char *s)
{
    int n = 0;
    if (s == 0) {
        return 0;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

/* ---- internal: walk the packed buffer ------------------------------------ */

/* Find the start of the entry whose name matches `name` (case-insensitive).
 * Returns the byte offset of the matching "NAME=VALUE\0" entry within e->buf,
 * or -1 if not found.
 *
 * Walk strategy: step entry-by-entry through the packed ASCIIZ strings.
 * Each entry is "NAME=VALUE\0"; the name portion is everything before '='.
 * We compare that name (case-insensitively) to the caller-supplied `name`. */
static int env_find(const env_store_t *e, const char *name)
{
    int pos = 0;

    while (pos < e->len) {
        /* Find '=' in this entry to delimit the name. */
        int name_len = 0;
        int scan = pos;
        while (scan < e->len && e->buf[scan] != '=' && e->buf[scan] != '\0') {
            name_len++;
            scan++;
        }
        if (scan >= e->len || e->buf[scan] != '=') {
            /* Malformed entry (no '=') or ran off the end -- stop. */
            break;
        }

        /* Compare the name portion (e->buf[pos..pos+name_len-1]) with `name`
         * using case-insensitive matching (DOS env var names). */
        int match = 1;
        if (env_strlen(name) == name_len) {
            for (int i = 0; i < name_len; i++) {
                if (env_upcase_char((char)e->buf[pos + i]) !=
                    env_upcase_char(name[i])) {
                    match = 0;
                    break;
                }
            }
        } else {
            match = 0;
        }

        if (match) {
            return pos;
        }

        /* Advance past this entry's NUL terminator to the next entry. */
        while (pos < e->len && e->buf[pos] != '\0') {
            pos++;
        }
        pos++;   /* skip the NUL itself */
    }
    return -1;
}

/* Return the byte-length of the entry starting at `pos` (including its NUL
 * terminator).  Treats a run of non-NUL bytes followed by NUL as one entry. */
static int env_entry_len(const env_store_t *e, int pos)
{
    int n = 0;
    while (pos + n < e->len && e->buf[pos + n] != '\0') {
        n++;
    }
    return n + 1;   /* +1 for the NUL terminator */
}

/* ---- public API ---------------------------------------------------------- */

void env_init(env_store_t *e)
{
    e->len = 0;
    /* Zero the buffer for determinism (Rule 11) -- no stale bytes. */
    for (int i = 0; i < ENV_ARENA_MAX; i++) {
        e->buf[i] = 0;
    }
}

int env_set(env_store_t *e, const char *name, const char *value)
{
    if (e == 0 || name == 0 || value == 0) {
        return 0;
    }

    /* Upper-case the name into a local scratch buffer.  Names must be purely
     * ASCII; an excessively long name simply will not fit in ENV_ARENA_MAX
     * and will be caught by the overflow check below. */
    char uname[ENV_ARENA_MAX + 1];
    int  nlen = 0;
    for (const char *p = name; *p != '\0' && nlen < ENV_ARENA_MAX; p++) {
#ifdef ENV_MUTATE_NO_UPCASE
        /* MUTANT (Rule 6; make test-env-mutant only): skip upcasing so the
         * stored name keeps its original case.  A "path" SET leaves the entry
         * as "path=..." instead of "PATH=..."; the upcase assertion goes RED.
         * NEVER in a real build. */
        uname[nlen++] = *p;
#else
        uname[nlen++] = env_upcase_char(*p);
#endif
    }
    uname[nlen] = '\0';

    int  vlen      = env_strlen(value);
    /* New entry wire size: "UNAME=VALUE\0" = nlen + 1 ('=') + vlen + 1 ('\0'). */
    int  new_entry_len = nlen + 1 + vlen + 1;

    /* Locate an existing binding so we can remove it (UPSERT; no duplicates). */
    int  old_pos   = env_find(e, uname);
    int  old_len   = (old_pos >= 0) ? env_entry_len(e, old_pos) : 0;

    /* Overflow guard (Rule 2): the new store size must not exceed ENV_ARENA_MAX.
     * Compute BEFORE making any change so the store stays byte-identical on
     * failure (the caller can re-check e->len and a prior env_get). */
    int new_total = e->len - old_len + new_entry_len;
    if (new_total > ENV_ARENA_MAX) {
        return 0;   /* would overflow; store is UNCHANGED */
    }

#ifndef ENV_MUTATE_NO_DEDUP
    /* Remove the existing entry (if any) by compacting the buffer.
     * MUTANT ENV_MUTATE_NO_DEDUP: skip this removal so env_set always
     * appends, leaving duplicates; the UPSERT / count-stays-1 test goes RED.
     * NEVER in a real build when ENV_MUTATE_NO_DEDUP is not defined. */
    if (old_pos >= 0) {
        /* Shift the bytes after the old entry down over it. */
        int tail_start = old_pos + old_len;
        int tail_len   = e->len - tail_start;
        for (int i = 0; i < tail_len; i++) {
            e->buf[old_pos + i] = e->buf[tail_start + i];
        }
        /* Zero the vacated tail for determinism (Rule 11). */
        for (int i = old_pos + tail_len; i < e->len; i++) {
            e->buf[i] = 0;
        }
        e->len -= old_len;
    }
#endif

    /* Append the new "UNAME=VALUE\0" entry at e->len. */
    int pos = e->len;
    for (int i = 0; i < nlen; i++) {
        e->buf[pos++] = (uint8_t)uname[i];
    }
    e->buf[pos++] = (uint8_t)'=';
    for (int i = 0; i < vlen; i++) {
        e->buf[pos++] = (uint8_t)value[i];
    }
    e->buf[pos++] = 0;   /* NUL terminator for this entry */
    e->len = pos;

    return 1;
}

const char *env_get(const env_store_t *e, const char *name)
{
    if (e == 0 || name == 0) {
        return 0;
    }
    int pos = env_find(e, name);
    if (pos < 0) {
        return 0;
    }
    /* Walk forward past the name portion to the '=' and return what follows. */
    while (pos < e->len && e->buf[pos] != '=' && e->buf[pos] != '\0') {
        pos++;
    }
    if (pos >= e->len || e->buf[pos] != '=') {
        return 0;   /* malformed -- should not happen */
    }
    return (const char *)&e->buf[pos + 1];   /* skip '=' -> value start */
}

int env_unset(env_store_t *e, const char *name)
{
    if (e == 0 || name == 0) {
        return 0;
    }
    int pos = env_find(e, name);
    if (pos < 0) {
        return 0;   /* absent; idempotent */
    }
    int elen       = env_entry_len(e, pos);
    int tail_start = pos + elen;
    int tail_len   = e->len - tail_start;

    /* Compact: shift tail down. */
    for (int i = 0; i < tail_len; i++) {
        e->buf[pos + i] = e->buf[tail_start + i];
    }
    /* Zero vacated bytes for determinism (Rule 11). */
    for (int i = pos + tail_len; i < e->len; i++) {
        e->buf[i] = 0;
    }
    e->len -= elen;
    return 1;
}

int env_count(const env_store_t *e)
{
    int count = 0;
    int pos   = 0;

    if (e == 0) {
        return 0;
    }
    while (pos < e->len) {
        /* Skip to the NUL terminator of this entry. */
        while (pos < e->len && e->buf[pos] != '\0') {
            pos++;
        }
        if (pos < e->len) {
            count++;
            pos++;   /* skip the NUL */
        }
    }
    return count;
}

const char *env_entry(const env_store_t *e, int idx)
{
    int pos   = 0;
    int count = 0;

    if (e == 0 || idx < 0) {
        return 0;
    }
    while (pos < e->len) {
        if (count == idx) {
            return (const char *)&e->buf[pos];
        }
        /* Advance past this entry. */
        while (pos < e->len && e->buf[pos] != '\0') {
            pos++;
        }
        pos++;   /* skip the NUL */
        count++;
    }
    return 0;   /* idx out of range */
}

int env_serialize(const env_store_t *e, uint8_t *out, int cap)
{
    if (e == 0 || out == 0 || cap <= 0) {
        return 0;
    }

    /* Required output size: e->len bytes (packed entries) + 1 (extra NUL). */
    int need = e->len + 1;
    if (need > cap) {
        return 0;   /* fail loud (Rule 2): would not fit */
    }

    /* Copy the packed entries verbatim. */
    for (int i = 0; i < e->len; i++) {
        out[i] = e->buf[i];
    }
    /* Append the mandatory extra NUL block terminator. */
    out[e->len] = 0;

    return need;
}
