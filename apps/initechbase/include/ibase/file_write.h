/* Failed-save preservation: operator ruling, Phase 2 BRIEF-COMMON, item 1.
 * Same-directory DOS 8.3 staging. This is reported-error recovery, not crash
 * atomicity. No write to the destination occurs before the stage is closed.
 * References: PAL INT 21h 3Ch/3Eh/41h/56h; tests/write-safety/README.md. */
#ifndef IBASE_FILE_WRITE_H
#define IBASE_FILE_WRITE_H
#include "ibase/pal.h"
#include "ibase/rt.h"
typedef struct {
    ibase_pal_t *pal;
    pal_fd fd;
    char name[260];
    char stage[260], backup[260];
} ibase_file_write;
static inline int ibase_file_begin(ibase_file_write *t,ibase_pal_t *p,const char *name)
{
    unsigned dir=0,i,j;pal_fd f;
    t->pal=p;t->fd=-1;
    if(rt_strlen(name)>247u)return -PAL_EACCES;
    rt_memcpy(t->name,name,rt_strlen(name)+1u);
    for(i=0;name[i];i++)if(name[i]=='/' || name[i]=='\\' || name[i]==':')dir=i+1;
    rt_memcpy(t->stage,name,dir);rt_memcpy(t->backup,name,dir);
    /* Check access before staging: renaming a read-only destination must not
     * bypass the ordinary open-for-write refusal. */
    f=p->open(p,name,PAL_RDWR);
    if(f>=0){if(p->close(p,f))return -PAL_EIO;}
    else if(f!=-PAL_ENOENT)return f;
    for(i=0;i<1000;i++){
        char leaf[]="IB000000.TMP";
        leaf[5]=(char)('0'+i/100);leaf[6]=(char)('0'+i/10%10);leaf[7]=(char)('0'+i%10);
        rt_memcpy(t->stage+dir,leaf,sizeof leaf);
        for(j=0;j<sizeof leaf;j++)t->backup[dir+j]=leaf[j];
        rt_memcpy(t->backup+dir+9,"BAK",4);
        f=p->open(p,t->stage,PAL_RD);
        if(f>=0){p->close(p,f);continue;}if(f!=-PAL_ENOENT)return f;
        f=p->open(p,t->backup,PAL_RD);
        if(f>=0){p->close(p,f);continue;}if(f!=-PAL_ENOENT)return f;
        t->fd=p->open(p,t->stage,PAL_RDWR|PAL_CREATE|PAL_TRUNC);
        return t->fd<0?t->fd:0;
    }
    return -PAL_EACCES;
}
static inline void ibase_file_abort(ibase_file_write *t)
{
    if(t->fd>=0){t->pal->close(t->pal,t->fd);t->fd=-1;}
    t->pal->remove(t->pal,t->stage);
}
static inline int ibase_file_commit(ibase_file_write *t,pal_fd *live)
{
    ibase_pal_t *p=t->pal;int old=0,rc;pal_fd f;
    rc=p->close(p,t->fd);t->fd=-1;
    if(rc){ibase_file_abort(t);return rc;}
    if(live && *live>=0){rc=p->close(p,*live);*live=-1;if(rc){ibase_file_abort(t);return rc;}}
    f=p->open(p,t->name,PAL_RD);
    if(f>=0){
        rc=p->close(p,f);if(rc){ibase_file_abort(t);return rc;}
        rc=p->rename(p,t->name,t->backup);
        if(rc){ibase_file_abort(t);return rc;}old=1;
    }else if(f!=-PAL_ENOENT){ibase_file_abort(t);return f;}
    rc=p->rename(p,t->stage,t->name);
    if(!rc && live){*live=p->open(p,t->name,PAL_RDWR);if(*live<0)rc=(int)*live;}
    if(rc){
        /* Keep the old bytes even if a second independent metadata failure
         * prevents restoration. Never delete the recovery copy in that case. */
        if(old){
            p->remove(p,t->name);
            if(p->rename(p,t->backup,t->name))return -PAL_EIO;
        }
        ibase_file_abort(t);return rc;
    }
    if(old)p->remove(p,t->backup); /* cleanup failure does not fail a saved file */
    return 0;
}
#endif
