/* Factory inputs only; expected output is minted by real III+.
 * Ref: PRD 6.6; specs/file-formats/dbf.md sec 2/5 and dbt.md sec 2/5.
 * Reuse the existing DBF/DBT writers (mint_clients_dbf.c pattern). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "samir/dbf.h"
#include "samir/dbt.h"
#include "samir/value.h"
struct pal_host_cfg { uint8_t yy, mm, dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static int table(samir_pal_t *pal, const char *path, int variant)
{
    const dbf_field_spec fields[] = {{"NAME",'C',10,0},{"CITY",'C',12,0},{"BAL",'N',8,2}};
    const char *names[] = {"PESTON","WADDAMS","EXTRA"};
    const char *cities[] = {"SECOND","REMOTE","UNMATCHED"};
    /* Matching transaction keys descend physically: ignoring RANDOM must lose PESTON. */
    const unsigned order[] = {1,0,2};
    dbf_table *t = NULL;
    if (dbf_create(pal,path,fields,3,&t)) { fprintf(stderr,"create %s failed\n",path); return 1; }
    for (unsigned i=0;i<(variant?1u:3u);i++) {
        unsigned row_index=variant?i:order[i];
        const char *n=variant==1?"DRIVE_D":variant==2?"PATH_DATA":names[row_index];
        const char *c=variant?"FIXTURE":cities[row_index];
        xb_val row[] = {xb_c(n,(uint32_t)strlen(n)),xb_c(c,(uint32_t)strlen(c)),xb_n(variant?99:10*(row_index+1))};
        if (dbf_append_rec(t,row,0)) { fprintf(stderr,"append failed\n"); dbf_close(t); return 1; }
    }
    if(dbf_flush(t)) { fprintf(stderr,"flush failed\n"); dbf_close(t); return 1; }
    dbf_close(t);
    return 0;
}
int main(int argc,char **argv)
{
    char path[2048]; dbf_table *t=NULL; dbt_file *m=NULL; uint32_t block=0;
    const char text[]="Alpha bravo charlie delta echo foxtrot golf hotel india juliet kilo lima mike november oscar papa.";
    const dbf_field_spec field={"NOTE",'M',10,0};
    samir_pal_t *pal=pal_host_make((struct pal_host_cfg){86,7,24,0});
    if(argc!=2||!pal)return 2;
    snprintf(path,sizeof path,"%s/OTHER.DBF",argv[1]);if(table(pal,path,0))return 3;
    snprintf(path,sizeof path,"%s/DRIVE/BASE.DBF",argv[1]);if(table(pal,path,1))return 3;
    snprintf(path,sizeof path,"%s/DATA/TARGET.DBF",argv[1]);if(table(pal,path,2))return 3;
    snprintf(path,sizeof path,"%s/MEMO.DBT",argv[1]);
    if(dbt_create(pal,path,&m)||dbt_append(m,(const uint8_t*)text,sizeof text-1,&block)||dbt_close(m))return 3;
    snprintf(path,sizeof path,"%s/MEMO.DBF",argv[1]);
    if(dbf_create(pal,path,&field,1,&t))return 4;
    xb_val row=xb_n(block);
    if(dbf_append_rec(t,&row,0)||dbf_flush(t))return 5;
    dbf_close(t);pal_host_free(pal);return 0;
}
