/* Same-directory owned stages for FILE COPY, phase-2 GM3 ruling.
 * Publication uses the native replacement binding in tbxgate.c. */
#ifndef FLAIR_FILE_REPLACE_H
#define FLAIR_FILE_REPLACE_H
#include <stdint.h>
#include "toolbox_gate.h"
typedef int32_t (*flair_file_op)(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
typedef struct {
    char stage[TBX_FILE_PATH_MAX];
    int staged;
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
    t->staged=0;
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
    if(t->staged) {
        int32_t rc=op(0x4100,0,0,flair_file_ptr(t->stage),0);
        if(rc>=0 || rc==TBX_ERR_DOS(2)) t->staged=0;
    }
}

#endif
