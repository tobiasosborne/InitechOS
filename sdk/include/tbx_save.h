/* tbx_save.h -- the tenant runtime's staged document save (bead initech-vtl9).
 *
 * WHY. TBX_FILE_CREATE is DOS AH=3Ch: it truncates an existing file before
 * the first byte of the new one is written (sdk/include/toolbox_gate.h Sec 10). A
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
 *      this call created; the previous file never moved.
 *   2. Check destination write access; find an absent <name>.~00 .. .~99.
 *      RENAME the previous file there, retaining its complete stream.
 *   3. RENAME the stage to the destination. If that fails, rename the old
 *      stream back before reporting the failure; never delete that stream.
 *      If rollback itself fails, retain both streams (see rollback_error).
 *   4. After commit, remove the backup. Failed cleanup retains an extra copy
 *      and is not reported as a failed save.
 * DOS 3.3 has no rename-over-an-existing-file (Technical Reference p. 6-206;
 * kernel/int21.c do_rename refuses an existing target with AX=5), hence
 * checked backup/rename/rollback. This is not crash atomicity. Gate calls are
 * synchronous and the scheduler is cooperative; no tenant runs in the window.
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
 * Freestanding, ASCII-only, deterministic (CLAUDE.md, The constitution). Included
 * by each saving tenant (and compiled verbatim by its host oracle); unused
 * in a tenant that never saves. */
#ifndef INITECH_TBX_SAVE_H
#define INITECH_TBX_SAVE_H
#include "tbx.h"

static char g_tbx_save_tmp[TBX_FILE_PATH_MAX];
static char g_tbx_save_backup[TBX_FILE_PATH_MAX];
/* Nonzero means rollback itself failed. Both complete streams are retained;
 * the storage owner must resolve that I/O failure before reusing this helper. */
static int32_t g_tbx_save_rollback_error;
static char g_tbx_save_notice[3u * TBX_ALERT_LINE_MAX] TBX_IN_IMAGE;
static unsigned char g_tbx_save_chunk[1024];
static unsigned char g_tbx_save_verify[1024];

/* Existing ALERT geometry: three lines, 285 pixels. At most 20 characters
 * per path line fits even the widest Chicago glyph (14 px, NFNT 5478's
 * locked advances in flair/include/assets/chicago12.h). Never truncate a name.
 * A save whose recovery name cannot fit refuses before creating a stage. */
static void tbx_save_recovery_notice(void)
{
    static const char heading[]="Previous version saved as:\r";
    uint32_t n=0,j=0;
    while(g_tbx_save_backup[n]) n++;
    if(n<=40) for(uint32_t i=0;heading[i];i++) g_tbx_save_notice[j++]=heading[i];
    for(uint32_t i=0;i<n;i++) {
        if(i && i%20u==0) g_tbx_save_notice[j++]='\r';
        g_tbx_save_notice[j++]=g_tbx_save_backup[i];
    }
    g_tbx_save_notice[j]=0;
    /* Queue this before the caller's ordinary error alert. The gate owns a
     * copy and refuses that later alert as BUSY, preserving the recovery name. */
    (void)tbx_alert(g_tbx_save_notice,"OK",0,0);
}

/* initech-7aou: the SAME transaction also consumes a file handle. The input
 * is closed before committing; short reads/writes and either failed close
 * discard only our temporary. This avoids a document-sized copy limit. */
static int32_t tbx_file_save_feed(const char *path, const void *buf, uint32_t n, int32_t input)
{
    const unsigned char *b = (const unsigned char *)buf;
    uint32_t i = 0, dot = 0, pos = 0, k;
    int32_t h = TBX_ERR_DOS(0x50), rc = 0, cl;
    int old_moved = 0;
    if(g_tbx_save_rollback_error<0) {
        if(input>=0) (void)tbx_file_close(input);
        tbx_save_recovery_notice();
        return g_tbx_save_rollback_error;
    }
    g_tbx_save_rollback_error = 0;

    /* <name>.$$$: the leaf's extension (after the last '.' of the last
     * component) replaced, or appended when there is none. */
    for (; path[i] != '\0'; i++) {
        if (path[i] == '\\' || path[i] == ':') dot = 0;
        else if (path[i] == '.') dot = i;
    }
    if (dot == 0) dot = i;
    if (dot + 5u > TBX_FILE_PATH_MAX || dot + 4u > 60u) {
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

        h = tbx_file_create_new(g_tbx_save_tmp); /* proven unused, or 0x50 */

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

        }
        pos += done;
        if (done != count) break;

    }
    cl = tbx_file_close(h);
    if (cl < 0) (void)tbx_file_close(h);
    /* Verify the closed staged stream against the caller's bytes, or the
     * rewound input stream, before moving the previous version at all. */
    if(pos==n && cl>=0) {
        h=tbx_file_open(g_tbx_save_tmp,TBX_FILE_READ_ONLY);
        if(h<0) cl=h;
        else {
            uint32_t checked=0;
            if(input>=0) cl=tbx_file_seek(input,0,TBX_FILE_FROM_START);
            while(cl>=0 && checked<n) {
                uint32_t want=n-checked;
                if(want>sizeof g_tbx_save_verify) want=sizeof g_tbx_save_verify;
                rc=tbx_file_read(h,g_tbx_save_verify,want);
                if(rc<=0 || (uint32_t)rc>want) { cl=rc<0?rc:TBX_ERR_DOS(0x1d); break; }
                uint32_t got=(uint32_t)rc;
                const unsigned char *expected=b?b+checked:g_tbx_save_chunk;
                if(input>=0) {
                    uint32_t read=0;
                    while(read<got) {
                        rc=tbx_file_read(input,g_tbx_save_chunk+read,got-read);
                        if(rc<=0 || (uint32_t)rc>got-read) { cl=rc<0?rc:TBX_ERR_DOS(0x1d); break; }
                        read+=(uint32_t)rc;
                    }
                    if(cl<0) break;
                }
                for(uint32_t v=0;v<got;v++) if(expected[v]!=g_tbx_save_verify[v]) { cl=TBX_ERR_DOS(0x1d); break; }
                checked+=got;
            }
            rc=tbx_file_close(h);
            if(rc<0) { (void)tbx_file_close(h); cl=rc; }
        }
    }
    if (input >= 0) {
        int32_t in_close = tbx_file_close(input);
        if (in_close < 0) (void)tbx_file_close(input);
        if (in_close < 0) cl = in_close;
    }
    if (pos != n || cl < 0) {
        (void)tbx_file_delete(g_tbx_save_tmp);   /* the old file never moved */
        return cl < 0 ? cl : rc < 0 ? rc : (int32_t)pos;
    }
    /* Refuse a read-only destination before moving it. DOS RENAME alone
     * does not necessarily enforce read-only overwrite protection. */
    rc = tbx_file_open(path, TBX_FILE_WRITE_ONLY);
    if (rc >= 0) {
        h=rc; rc=tbx_file_close(h);
        if(rc<0) (void)tbx_file_close(h);
    }
    if (rc < 0 && rc != TBX_ERR_DOS(2)) goto refuse;
    for (i = 0; i <= dot; i++) g_tbx_save_backup[i] = g_tbx_save_tmp[i];
    g_tbx_save_backup[dot+1] = '~';
    g_tbx_save_backup[dot+4] = 0;
    rc = TBX_ERR_DOS(0x50);
    for (k = 0; k < 100; k++) {
        g_tbx_save_backup[dot+2] = (char)('0' + k/10);
        g_tbx_save_backup[dot+3] = (char)('0' + k%10);
        /* Never borrow the target itself as its backup. */
        for (i=0; path[i] && path[i]==g_tbx_save_backup[i]; i++) {}
        if (!path[i] && !g_tbx_save_backup[i]) continue;
        h = tbx_file_open(g_tbx_save_backup, TBX_FILE_READ_ONLY);
        if (h >= 0) {
            cl = tbx_file_close(h);
            if (cl < 0) (void)tbx_file_close(h);
            if (cl < 0) { rc=cl; goto refuse; }
            continue;
        }
        if (h != TBX_ERR_DOS(2)) { rc=h; goto refuse; }
        rc = tbx_file_rename(path,g_tbx_save_backup);
        if (rc == TBX_ERR_DOS(2)) { rc=0; break; }
        if (rc < 0) goto refuse;
        old_moved=1;
        break;
    }
    if (rc < 0) goto refuse;
    rc = tbx_file_rename(g_tbx_save_tmp, path);
    if (rc < 0) {
        if (old_moved) {
            g_tbx_save_rollback_error = tbx_file_rename(g_tbx_save_backup,path);
            if (g_tbx_save_rollback_error < 0) { tbx_save_recovery_notice(); return rc; }
        }
        goto refuse;
    }
    /* The save has committed. Failed backup cleanup leaves a recoverable
     * extra file and must not report that the save failed. */
    if (old_moved) (void)tbx_file_delete(g_tbx_save_backup);
    return (int32_t)n;
refuse:
    (void)tbx_file_delete(g_tbx_save_tmp);
    return rc;
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
