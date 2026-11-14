/* Recoverable replacement for cooperative callers (GM3/4/6, phase-2 ruling).
 * Authored policy, DOS 3.3 EDLIN rename-original precedent (UG p.164).
 * No crash-atomicity claim. The binding must make failed renames non-mutating.
 * Rollback failure is returned separately; callers must retain both names. */
#ifndef FLAIR_FILE_REPLACE_H
#define FLAIR_FILE_REPLACE_H
#include <stdint.h>
#include "toolbox_gate.h"
typedef int32_t (*flair_file_op)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef struct {
    char stage[TBX_FILE_PATH_MAX], backup[TBX_FILE_PATH_MAX];
    int staged, old_moved;
    int32_t rollback_error;
} flair_file_replace;
static inline uint32_t flair_file_ptr(const void *p) { return (uint32_t)(uintptr_t)p; }
static inline int flair_file_name(char *out,const char *path,char kind,unsigned candidate)
{
    unsigned i=0,dot=0;
    while(path[i]) {
        if(i>=TBX_FILE_PATH_MAX-1u) return 0;
        if(path[i]=='\\' || path[i]=='/' || path[i]==':') dot=0;
        else if(path[i]=='.') dot=i;
        i++;
    }
    if(!dot) dot=i;
    if(dot+5u>TBX_FILE_PATH_MAX || dot+4u>60u) return 0;
    for(i=0;i<dot;i++) out[i]=path[i];
    out[dot]='.'; out[dot+1]=kind; out[dot+2]=(char)('0'+candidate/10);
    out[dot+3]=(char)('0'+candidate%10); out[dot+4]=0;
    /* ASCII DOS names, case-insensitive. Never reserve the destination. */
    for(i=0;out[i] && path[i];i++) {
        unsigned a=(unsigned char)out[i],b=(unsigned char)path[i];
        if(a>='a' && a<='z') a-=32;
        if(b>='a' && b<='z') b-=32;
        if(a!=b) return 1;
    }
    return out[i]!=path[i];
}
static inline int32_t flair_file_stage(flair_file_replace *t,const char *path,flair_file_op op)
{
    t->staged=t->old_moved=0; t->rollback_error=0;
    if(!flair_file_name(t->stage,path,'$',0) && !flair_file_name(t->stage,path,'$',1))
        return TBX_ERR_BADARG;
    for(unsigned k=0;k<100;k++) {
        if(!flair_file_name(t->stage,path,'$',k)) continue;
        int32_t h=op(0x5b00,0,0,flair_file_ptr(t->stage),0);
        if(h==TBX_ERR_DOS(0x50)) continue;
        if(h>=0) t->staged=1;
        return h;
    }
    return TBX_ERR_DOS(0x50);
}
static inline void flair_file_discard(flair_file_replace *t,flair_file_op op)
{
    if(t->staged && !t->old_moved) {
        (void)op(0x4100,0,0,flair_file_ptr(t->stage),0);
        t->staged=0;
    }
}
static inline int32_t flair_file_commit(flair_file_replace *t,const char *path,flair_file_op op)
{
    int32_t rc=TBX_ERR_DOS(0x50);
    rc=op(0x3d01,0,0,flair_file_ptr(path),0);
    if(rc>=0) {
        uint32_t h=(uint32_t)rc;
        rc=op(0x3e00,h,0,0,0);
        if(rc<0) (void)op(0x3e00,h,0,0,0);
    }
    if(rc<0 && rc!=TBX_ERR_DOS(2)) return rc;
    rc=TBX_ERR_DOS(0x50);
    /* Probe a free backup name without truncating any existing remnant.
     * No tenant can run between probe and rename (cooperative scheduling). */
    for(unsigned k=0;k<100;k++) {
        if(!flair_file_name(t->backup,path,'~',k)) continue;
        rc=op(0x3d00,0,0,flair_file_ptr(t->backup),0);
        if(rc>=0) {
            uint32_t h=(uint32_t)rc;
            rc=op(0x3e00,h,0,0,0);
            if(rc<0) (void)op(0x3e00,h,0,0,0);
            if(rc<0) return rc;
            rc=TBX_ERR_DOS(0x50); continue;
        }
        if(rc!=TBX_ERR_DOS(2)) return rc;
        rc=op(0x5600,0,0,flair_file_ptr(path),flair_file_ptr(t->backup));
        if(rc==TBX_ERR_DOS(2)) { rc=0; break; } /* first save */
        if(rc<0) return rc;
        t->old_moved=1; break;
    }
    if(rc<0) return rc;
    rc=op(0x5600,0,0,flair_file_ptr(t->stage),flair_file_ptr(path));
    if(rc<0) {
        if(t->old_moved) {
            t->rollback_error=op(0x5600,0,0,flair_file_ptr(t->backup),flair_file_ptr(path));
            if(t->rollback_error>=0) t->old_moved=0;
        }
        return rc;
    }
    t->staged=0;
    if(t->old_moved) {
        /* Committed. Cleanup failure is not a failed save: the new file opens
         * under its own name and the complete old file remains discoverable. */
        (void)op(0x4100,0,0,flair_file_ptr(t->backup),0);
        t->old_moved=0;
    }
    return TBX_OK;
}
#endif
