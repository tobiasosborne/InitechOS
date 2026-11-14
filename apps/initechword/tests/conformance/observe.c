#include "bytes.h"
#include "tbx_host.h"
#include "../../app/face_a.c"
int main(int argc,char **argv){
 if(argc!=3)return 2;
 size_t n=input(argv[1]);tbx_host_reset();
 if(!tbx_host_put_file("CASE.WP5",bytes,(uint32_t)n)||tbx_app_main())return 2;
 strcpy(g_path,"CASE.WP5");strcpy(g_shown,"A:\\CASE.WP5");retrieve_file(M_GET_NAME);
 int loaded=g_mode!=M_ERROR;printf("%s\tbytes=%zu loaded=%d body=%u message=%s",argv[1],n,loaded,g_ed.doc.len,g_err?g_err:"");
 if(loaded){
  write_file();const tbx_host_file_t *f=tbx_host_file("CASE.WP5");
  printf(" save-error=%d saved=%u identical=%d",g_mode==M_ERROR,f->size,f->size==n&&!memcmp(bytes,f->data,n));
  FILE *out=fopen(argv[2],"wb");if(!out)return 2;fwrite(f->data,1,f->size,out);fclose(out);
  int rc=iw_doc_insert(&g_ed.doc,0,(const uint8_t *)"Z",1);
  printf(" edit=%d",rc);if(!rc){g_ed.dirty=1;write_file();printf(" edited-save-error=%d",g_mode==M_ERROR);}
 }
 puts("");return 0;
}
