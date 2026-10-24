/* tbx_save.h -- the tenant runtime's atomic document save (bead initech-mcep).
 *
 * WHY. TBX_FILE_CREATE is DOS AH=3Ch: it truncates an existing file before
 * the first byte of the new one is written (spec/toolbox_gate.h Sec 10). A
 * save that then runs out of disk left a zero-byte file where the user's last
 * saved version had been -- audits 2026-10-23 iword L001/L002 (twice
 * reproduced), das L003 (InitechText), i123 L001 (Initech 123).
 *
 * THE SEQUENCE (authored; the period precedent is DOS 3.3 EDLIN, which writes
 * the edited file and only then renames the original -- MS-DOS 3.3 User's
 * Reference p. 164, "How to End Edlin and Save Your Changes"):
 *   1. CREATE_NEW (AH=5Bh, never truncates) <name>.$$$ in the SAME
 *      directory -- or, when that name is taken (a user's file, a remnant of
 *      an interrupted save), the first free of <name>.$$0 .. <name>.$$9 --
 *      and WRITE every byte; CLOSE. Any failure here DELETEs only the file
 *      this call created: the old file, any file already under a temporary
 *      name, and the directory are exactly as they were.
 *   2. DELETE <name> (DOS error 2, no such file, is a new save, not a
 *      failure). A refused DELETE (a read-only file: DOS error 5) DELETEs the
 *      temporary file and returns the error -- the old file is untouched.
 *   3. RENAME <name>.$$$ -> <name> (AH=56h, spec/toolbox_gate.h Sec 10b).
 * DOS 3.3 has no rename-over-an-existing-file (Technical Reference p. 6-206;
 * os/milton/int21.c do_rename refuses an existing target with AX=5), hence
 * delete-THEN-rename. THE WINDOW: between steps 2 and 3 only <name>.$$$ holds
 * the document; a machine reset exactly there leaves it under that name (the
 * EDLIN-style recoverable remnant). Gate calls are synchronous and the
 * scheduler is cooperative, so no other tenant runs inside the window.
 * THE COST: the new file is written while the old one still occupies its
 * clusters, so a replacing save needs free space for the whole new file.
 * A candidate equal to the target's own name is skipped; eleven taken names
 * refuse the save with TBX_ERR_DOS(0x50) (file exists) before any write.
 *
 * RESULT: `n` on success; 0..n-1 is the short count of a WRITE that ran out
 * of disk (the caller's "Disk Full"); negative is the TBX_ERR_* /
 * TBX_ERR_DOS(code) of the CREATE, CLOSE, DELETE or RENAME that refused.
 * `path` and `buf` must be static module/BSS storage (tbx.h Sec 10 rule); the
 * temporary name is built in this header's own BSS buffer.
 *
 * Freestanding, ASCII-only, deterministic (Laws 1-3, Rules 11/12). Included
 * by each saving tenant (and compiled verbatim by its host oracle); unused
 * in a tenant that never saves. */
#ifndef INITECH_TBX_SAVE_H
#define INITECH_TBX_SAVE_H
#include "tbx.h"

static char g_tbx_save_tmp[TBX_FILE_PATH_MAX];
static unsigned char g_tbx_save_chunk[1024];

/* initech-7aou: the SAME transaction also consumes a file handle. The input
 * is closed before committing; short reads/writes and either failed close
 * discard only our temporary. This avoids a document-sized copy limit. */
static int32_t tbx_file_save_feed(const char *path, const void *buf, uint32_t n, int32_t input)
{
    const unsigned char *b = (const unsigned char *)buf;
    uint32_t i = 0, dot = 0, pos = 0, k;
    int32_t h = TBX_ERR_DOS(0x50), rc = 0, cl;
#ifdef TBX_MUT_SAVE_IN_PLACE
    /* MUTANT (Rule 6; test-flair-fullsave-mutant): the pre-mcep save --
     * CREATE truncates the target, then the WRITE runs out of disk. */
    h = tbx_file_create(path);
    if (h < 0) return h;
    while (pos < n && (rc = tbx_file_write(h, b + pos, n - pos)) > 0 && (uint32_t)rc <= n - pos)
        pos += (uint32_t)rc;
    cl = tbx_file_close(h);
    return cl < 0 ? cl : (int32_t)pos;
#endif
    /* <name>.$$$: the leaf's extension (after the last '.' of the last
     * component) replaced, or appended when there is none. */
    for (; path[i] != '\0'; i++) {
        if (path[i] == '\\' || path[i] == ':') dot = 0;
        else if (path[i] == '.') dot = i;
    }
    if (dot == 0) dot = i;
    if (dot + 5u > TBX_FILE_PATH_MAX) {
        if (input >= 0) (void)tbx_file_close(input);
        return TBX_ERR_BADARG;
    }
    for (i = 0; i < dot; i++) g_tbx_save_tmp[i] = path[i];
    g_tbx_save_tmp[dot] = '.'; g_tbx_save_tmp[dot+1] = '$';
    g_tbx_save_tmp[dot+2] = '$'; g_tbx_save_tmp[dot+4] = '\0';
    for (k = 0; k < 11u && h == TBX_ERR_DOS(0x50); k++) {
        g_tbx_save_tmp[dot+3] = k == 0 ? '$' : (char)('0' + k - 1u);
        if (path[dot] == '.' && path[dot+1] == '$' && path[dot+2] == '$' &&
            path[dot+3] == g_tbx_save_tmp[dot+3] && path[dot+4] == '\0')
            continue;                            /* never the target itself */
#ifndef TBX_MUT_SAVE_TEMP_CREATE
        h = tbx_file_create_new(g_tbx_save_tmp); /* proven unused, or 0x50 */
#else
        /* MUTANT (Rule 6; test-flair-fullsave-mutant COLLIDE): the temporary
         * is a plain CREATE -- a file already under that name is truncated
         * and then deleted on failure. NEVER real. */
        h = tbx_file_create(g_tbx_save_tmp);
#endif
    }
    if (h < 0) {
        if (input >= 0) (void)tbx_file_close(input);
        return h;
    }
    while (pos < n) {
        const unsigned char *part = b ? b + pos : g_tbx_save_chunk;
        uint32_t count = n - pos, done = 0;
        if (input >= 0) {
            if (count > sizeof g_tbx_save_chunk) count = sizeof g_tbx_save_chunk;
            rc = tbx_file_read(input, g_tbx_save_chunk, count);
            if (rc <= 0 || (uint32_t)rc > count) { if (rc == 0) rc = TBX_ERR_DOS(5); break; }
            count = (uint32_t)rc;
        }
        while (done < count) {
            rc = tbx_file_write(h, part + done, count - done);
            if (rc <= 0 || (uint32_t)rc > count - done) break;
            done += (uint32_t)rc;
#ifdef TBX_MUT_SAVE_ONE_WRITE
            break;
#endif
        }
        pos += done;
        if (done != count) break;
#ifdef TBX_MUT_SAVE_ONE_WRITE
        break;   /* MUTANT (iword IWORD_MUT_SETUP_SHORT): one write only */
#endif
    }
    cl = tbx_file_close(h);
    if (input >= 0) {
        int32_t in_close = tbx_file_close(input);
        if (in_close < 0) cl = in_close;
    }
    if (pos != n || cl < 0) {
        (void)tbx_file_delete(g_tbx_save_tmp);   /* the old file never moved */
        return cl < 0 ? cl : rc < 0 ? rc : (int32_t)pos;
    }
    rc = tbx_file_delete(path);
    if (rc < 0 && rc != TBX_ERR_DOS(2)) {
        (void)tbx_file_delete(g_tbx_save_tmp);   /* e.g. read-only: kept */
        return rc;
    }
    rc = tbx_file_rename(g_tbx_save_tmp, path);
    return rc < 0 ? rc : (int32_t)n;
}

static __attribute__((unused)) int32_t tbx_file_save(const char *path, const void *buf, uint32_t n)
{
    return tbx_file_save_feed(path, buf, n, -1);
}

/* Copy through the mcep transaction, never CREATE over a destination.
 * Both paths and the transfer buffer live in module/BSS storage. Returns
 * TBX_OK only when every source byte has committed; DOS 39 is disk full. */
static __attribute__((unused)) int32_t tbx_file_copy_save(const char *src, const char *dst)
{
    int32_t h = tbx_file_open(src, TBX_FILE_READ_ONLY), n, rc;
    if (h < 0) return h;
    n = tbx_file_seek(h, 0, TBX_FILE_FROM_END);
    rc = n < 0 ? n : tbx_file_seek(h, 0, TBX_FILE_FROM_START);
    if (rc < 0) { (void)tbx_file_close(h); return rc; }
    rc = tbx_file_save_feed(dst, 0, (uint32_t)n, h);
    return rc < 0 ? rc : rc == n ? TBX_OK : TBX_ERR_DOS(39);
}
#endif
