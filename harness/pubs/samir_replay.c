/* PUB-REF-0001 GC-03. Same host PAL/interpreter route as dbf_diff.
 * stdout is the interpreter's bytes, without trimming or normalisation. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "samir/interp.h"
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
samir_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(samir_pal_t *);
int query_register(xb_interp *);
int set_register(xb_interp *);
static void output(samir_pal_t *p,const char *s,uint32_t n)
{ (void)p; if(fwrite(s,1,n,stdout)!=n) _Exit(2); }
int main(int argc,char **argv)
{
    char src[65536]; size_t n; FILE *f;
    if(argc!=2 || !(f=fopen(argv[1],"rb"))) return 2;
    n=fread(src,1,sizeof(src)-1,f);
    if(ferror(f) || !feof(f)) { fclose(f); return 2; }
    fclose(f);src[n]=0;
    samir_pal_t *p=pal_host_make((struct pal_host_cfg){92,3,1,1048576});
    xb_interp *ip=p?xb_interp_make(p):NULL;
    if(!ip) return 2;
    p->conout=output;
    int rc=query_register(ip);
    if(!rc) rc=set_register(ip);
    if(!rc) rc=samir_do(ip,src);
    if(rc) fprintf(stderr,"!!! test-pubs RED SAMIR-REPLAY: %s error=%d\n",argv[1],samir_last_error(ip));
    xb_interp_free(ip);pal_host_free(p);
    return rc?1:0;
}
