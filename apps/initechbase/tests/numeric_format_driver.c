/* Descriptor display differential. Expected bytes come only from Python.
 * Ref: Using III+ U5-144/PDF 256 (cells); line framing: real SF-numlist. */
#include <stdio.h>
#include <stdint.h>
#include "ibase/interp.h"
#include "ibase/workarea.h"
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap_size; };
ibase_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(ibase_pal_t *);
int query_register(xb_interp *);
static void output(ibase_pal_t *p,const char *s,uint32_t n)
{ (void)p; if(fwrite(s,1,n,stdout)!=n) _Exit(2); }
int main(int argc,char **argv)
{
    ibase_pal_t *p=pal_host_make((struct pal_host_cfg){85,1,1,1048576});
    xb_interp *ip=xb_interp_make(p);
    int rc=2;
    if((argc!=2 && argc!=3) || !ip) return 2;
    p->conout=output;
    xb_interp_ctx(ip)->set_heading=0;
    /* The U5-144 example is at the dot prompt. Real M005U and SF-numlist
     * supply opening CR LF here too; Enter itself is CR-only (at-enter). */
    xb_interp_ctx(ip)->dot_command=1;
    if(query_register(ip) || wa_set_open(xb_interp_env(ip),1,argv[1],"CASE",NULL)) return 2;
    rc=ibase_do(ip,argc==3?"LIST OFF\nDISPLAY ALL OFF\nGO TOP\nDISPLAY OFF\n":"LIST OFF\nLIST OFF V\nDISPLAY ALL OFF\nGO TOP\nDISPLAY OFF\n");
    xb_interp_free(ip);pal_host_free(p);
    return rc?1:0;
}
