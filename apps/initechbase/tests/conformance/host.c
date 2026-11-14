/* Observe unmodified product through the existing DOS-aware host adapter.
 * Uses its normal 2 MiB test heap, not a target-memory or scheduling proof. */
#include <stdio.h>
#include <unistd.h>
#include "w2_host.h"
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap; };
extern ibase_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(ibase_pal_t *);
static void output(ibase_pal_t *p,const char *s,uint32_t n) {
    (void)p;if(fwrite(s,1,n,stdout)!=n)_exit(2);
}
int main(int argc,char **argv) {
    char root[4096];int rc;struct w2_host_env env;
    if(argc!=2||chdir(argv[1])||!getcwd(root,sizeof root))return 2;
    ibase_pal_t *pal=pal_host_make((struct pal_host_cfg){92,3,2,2*1024*1024});
    if(!pal)return 2;
    struct w2_host_config cfg={root,root,16*1024*1024,16*1024*1024,'C'};
    if(w2_host_bind(&env,pal,&cfg))return 2;
    pal->conout=output;xb_interp *ip=xb_interp_make(pal);if(!ip)return 2;
    rc=w2_host_startup(pal,ip,"MINT.PRG");
    fprintf(stderr,"startup_rc=%d\n",rc);
    xb_interp_free(ip);w2_host_unbind(&env);pal_host_free(pal);
    return rc?1:0;
}
