/* App-specific fixture boundaries and aliases over the shared SDK fake.
 * Existing scripts inspect host files between events. Import before opening a
 * transaction and export on close/rename/delete, so the independent readers
 * keep seeing the same files. All file semantics live in sdk/host. */
#include "tbx_host.h"
static volatile int *g_ctr;
#define CHECK(cond, id, msg) do { g_ctr[0]++; if (!(cond)) { g_ctr[1]++; \
    fprintf(stderr, "  FAIL %s %s (line %d)\n", (id), (msg), __LINE__); } } while (0)
#define SCR_W TBX_HOST_W
#define SCR_H TBX_HOST_H
#define m_scr (tbx_host.scr)
#define m_inv (tbx_host.inv)
#define m_face (tbx_host.face)
#define m_col (tbx_host.col)
#define m_exit_calls (tbx_host.exit_calls)
#define m_exit_rc (tbx_host.exit_rc)
#define m_read_limit (tbx_host.read_limit)
#define m_write_limit (tbx_host.write_limit)
#define m_clock (tbx_host.clock)
#define m_free (tbx_host.free_bytes)
#define m_find_calls (tbx_host.find_calls)
static const char *m_dir;
typedef struct m_dirent { const char *dir; const char *name; uint8_t attr; uint16_t time, date; uint32_t size; } m_dirent_t;
static const m_dirent_t *m_dirents;
static int m_ndirents;
static void host_name(const char *path, char *out)
{
    int n = snprintf(out, 1024, "%s/%s", m_dir, path);
    if (n < 0 || n >= 1024) { fprintf(stderr,"fixture path too long\n");exit(2); }
    for (char *p = out + strlen(m_dir) + 1; *p; p++) if (*p == '\\') *p = '/';
}
static void fixture_import(void)
{
    if (m_dir && !tbx_host_import(m_dir)) { fprintf(stderr,"fixture import failed\n"); exit(2); }
#ifdef IWORD_LF_OPS_MOCK
    for (int i=0;i<TBX_HOST_FILES;i++) if (tbx_host.files[i].used) {
        tbx_host_file_t *f=&tbx_host.files[i];
        size_t n=strlen(f->name);
        if(n>=8&&!strcmp(f->name+n-8,"LOCK.TXT"))f->attr=0x21;
    }
#endif
}
static void fixture_export(void)
{
    if(m_dir && !tbx_host_export(m_dir)) { fprintf(stderr,"fixture export failed\n"); exit(2); }
}
static int32_t fixture_open(const char *p,uint32_t m)
{ fixture_import();return tbx_file_open(p,m); }
static int32_t fixture_close(int32_t h)
{ int32_t rc=tbx_file_close(h);fixture_export();return rc; }
static int32_t fixture_create_new(const char *p)
{ fixture_import();return tbx_file_create_new(p); }
static int32_t fixture_delete(const char *p)
{ int32_t rc=tbx_file_delete(p);fixture_export();return rc; }
static int32_t fixture_rename(const char *p,const char *q)
{ int32_t rc=tbx_file_rename(p,q);fixture_export();return rc; }
/* The List Files capture supplies a fixed directory with exact DOS metadata.
 * This is test injection, separate from the shared fake's find implementation. */
static int32_t fixture_find(const char *p,uint32_t attr,void *buf,uint32_t max)
{
    fixture_import();
    if(m_dirents) {
        /* Install the captured volume in captured order, retaining file bytes. */
        for(int j=0;j<m_ndirents;j++) {
            const m_dirent_t *e=&m_dirents[j];char name[128];
            snprintf(name,sizeof name,"%s%s",e->dir,e->name);
            const tbx_host_file_t *old=tbx_host_file(name);
            if(!old) {
                uint8_t *bytes=calloc(e->size?e->size:1,1);
                if(!bytes||!tbx_host_put_file(name,bytes,e->size))exit(2);
                free(bytes);
            }
            tbx_host_file_t *f=(tbx_host_file_t *)tbx_host_file(name);
            f->attr=e->attr;f->time=e->time;f->date=e->date;
        }
    }
    return tbx_file_find(p,attr,buf,max);
}
#define tbx_file_open fixture_open
#define tbx_file_close fixture_close
#define tbx_file_create_new fixture_create_new
#define tbx_file_delete fixture_delete
#define tbx_file_rename fixture_rename
#define tbx_file_find fixture_find
