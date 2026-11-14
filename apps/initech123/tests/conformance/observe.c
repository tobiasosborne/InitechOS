/* Observations, not expected values. Drives the real Retrieve and Save face
 * with SDK file storage; no scenario is compiled into the target. */
#include "bytes.h"
#include "tbx_host.h"
#include "../../app/face_a.c"
int main(int argc,char **argv){
 if(argc!=3)return 2;
 size_t n=input(argv[1]);tbx_host_reset();
 if(!tbx_host_put_file("CASE.WK1",bytes,(uint32_t)n)||tbx_app_main())return 2;
 strcpy(g_name,"CASE.WK1");g_nlen=8;retrieve();
 int loaded=g_mode!=M_NOTICE;printf("%s\tbytes=%zu loaded=%d cells=%u message=%s",argv[1],n,loaded,g_sheet.ncells,g_notice?g_notice:"");
 if(loaded){
  uint32_t wn=0;int rc=i123_wk1_write(&g_sheet,g_wk,sizeof g_wk,&wn);
  printf(" serialize=%d",rc);
  if(!rc){
   g_wk_len=wn;write_sheet();
   const tbx_host_file_t *f=tbx_host_file("CASE.WK1");
   printf(" saved=%u identical=%d",f->size,f->size==n&&!memcmp(bytes,f->data,n));
   FILE *out=fopen(argv[2],"wb");if(!out)return 2;fwrite(f->data,1,f->size,out);fclose(out);
   const i123_cell_t *cell=i123_find(&g_sheet,0,0);
   if(cell && cell->kind==I123_K_LABEL && cell->len==241){
    tbx_event_t e={.what=keyDown,.message=0x3c00};tbx_app_event(&e);
    e.message=0x1c0d;tbx_app_event(&e);cell=i123_find(&g_sheet,0,0);
    printf(" F2-Enter-label-bytes=%u",cell?cell->len:0);
    uint32_t fn=0;
    if(!i123_wk1_write(&g_sheet,g_wk,sizeof g_wk,&fn)){
     g_wk_len=fn;write_sheet();f=tbx_host_file("CASE.WK1");
     char path[1024];snprintf(path,sizeof path,"%s.f2",argv[2]);
     FILE *edited=fopen(path,"wb");if(!edited)return 2;
     fwrite(f->data,1,f->size,edited);fclose(edited);
    }
   }
   rc=i123_set_number(&g_sheet,0,0,73);printf(" edit=%d",rc);
   if(!rc){rc=i123_wk1_write(&g_sheet,g_wk,sizeof g_wk,&g_wk_len);if(!rc)write_sheet();printf(" edited-save=%d",rc);}
  }
 }
 puts("");return 0;
}
