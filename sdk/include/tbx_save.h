/* Staged document saving: phase-2 failed-save ruling, initech-7mb66.
 * CREATE_NEW in the destination directory; write, close and read back every
 * byte; close the input; FILE_REPLACE commits an existing destination in one
 * directory-sector write. A previously absent destination uses non-overwriting
 * RENAME. No previous version is renamed or deleted by this helper.
 * A zero REPLACE return is committed even if kernel maintenance failed:
 * its staging chain has been adopted and must never be unlinked by us.
 * A stage whose cleanup DELETE is refused is named to the user and is never
 * touched again: it is provably ours only within the save that created it
 * (CREATE_NEW), and the user may replace it before the next save. The next
 * save creates another stage exclusively (initech-ui9dg).
 * Authored safe-save policy; DOS 3.3 EDLIN writes before publishing (UG p164).
 * This guarantees reported results, not crash atomicity. A replacing save
 * needs free space for the complete new stream while the old remains.
 * Returns n on success, short WRITE count on disk full, negative TBX error.
 * Paths and buffers must be static module/BSS storage (gate Sec 10).
 */
#ifndef INITECH_TBX_SAVE_H
#define INITECH_TBX_SAVE_H
#include "tbx.h"

static char g_tbx_save_tmp[TBX_FILE_PATH_MAX];
static char g_tbx_save_notice[3u * TBX_ALERT_LINE_MAX] TBX_IN_IMAGE;
static unsigned char g_tbx_save_chunk[1024];
static unsigned char g_tbx_save_verify[1024];

/* Existing ALERT geometry: three lines, 285 pixels. At most 20 characters
 * per path line fits even the widest Chicago glyph (14 px, NFNT 5478's
 * locked advances in flair/include/assets/chicago12.h). Never truncate a name.
 * A save whose recovery name cannot fit refuses before creating a stage. */
static void tbx_save_named_notice(const char *heading,const char *path)
{
    uint32_t n=0,j=0;
    while(path[n]) n++;
    if(n<=40) for(uint32_t i=0;heading[i];i++) g_tbx_save_notice[j++]=heading[i];
    for(uint32_t i=0;i<n;i++) {
        if(i && i%20u==0) g_tbx_save_notice[j++]='\r';
        g_tbx_save_notice[j++]=path[i];
    }
    g_tbx_save_notice[j]=0;
    /* Queue this before the caller's ordinary error alert. The gate owns a
     * copy and refuses that later alert as BUSY, preserving the recovery name. */
    (void)tbx_alert(g_tbx_save_notice,"OK",0,0);
}
static int32_t tbx_save_discard_stage(void)
{
    int32_t rc=tbx_file_delete(g_tbx_save_tmp);
    if(rc<0 && rc!=TBX_ERR_DOS(2)) rc=tbx_file_delete(g_tbx_save_tmp);
    return rc==TBX_ERR_DOS(2) ? 0 : rc;
}

/* initech-7aou: the SAME transaction also consumes a file handle. The input
 * is closed before committing; short reads/writes and either failed close
 * discard only our temporary. This avoids a document-sized copy limit. */
static int32_t tbx_file_save_feed(const char *path, const void *buf, uint32_t n, int32_t input)
{
    const unsigned char *b = (const unsigned char *)buf;
    uint32_t i = 0, dot = 0, pos = 0, k;
    int32_t h = TBX_ERR_DOS(0x50), rc = 0, cl;

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
     * rewound input stream, before publication. */
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
        rc = cl < 0 ? cl : rc < 0 ? rc : (int32_t)pos;
        goto refuse;                         /* the old file never moved */
    }
    /* Both closed and verified. DOS2 means no destination: non-overwriting
     * RENAME is sufficient and cannot destroy an existing name. */
    rc = tbx_file_replace(g_tbx_save_tmp,path);
    if(rc==TBX_ERR_DOS(2)) rc=tbx_file_rename(g_tbx_save_tmp,path);
    if(rc<0) goto refuse;
    return (int32_t)n;
refuse:
    cl=tbx_save_discard_stage();
    if(cl<0) tbx_save_named_notice("Temporary file remains:\r",g_tbx_save_tmp);
    return rc;
}

static __attribute__((unused)) int32_t tbx_file_save(const char *path, const void *buf, uint32_t n)
{
    return tbx_file_save_feed(path, buf, n, -1);
}

/* Copy through the staged transaction, never CREATE over a destination.
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
