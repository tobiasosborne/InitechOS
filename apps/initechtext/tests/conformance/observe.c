#include "bytes.h"
#include "tbx_host.h"
#include "../../itext.c"
int main(int argc,char **argv){
 if(argc!=3)return 2;
 size_t n=input(argv[1]);tbx_host_reset();
 if(!tbx_host_put_file("CASE.TXT",bytes,(uint32_t)n)||tbx_app_main())return 2;
 tbx_host.read_limit=(uint32_t)atoi(argv[2]);
 int loaded=doc_load("CASE.TXT");
 printf("%s limit=%s\tbytes=%zu loaded=%d shown=%d alerts=%d message=%s",argv[1],argv[2],n,loaded,g_te.teLength,tbx_host.alerts,tbx_host.alert_text);
 int failed=0;
 if(loaded){
  int rc=doc_write();const tbx_host_file_t *f=tbx_host_file("CASE.TXT");
  printf(" save=%d saved=%u",rc,f->size);
  /* T04 has twenty ordinary TEXT bytes and no normalization. A short
   * successful read must not authorize overwriting the full document. */
  if(tbx_host.read_limit && rc && (f->size!=n||memcmp(bytes,f->data,n)))failed=1;
  tbx_event_t e={.what=keyDown,.message='Z'};tbx_app_event(&e);
  printf(" edit-save=%d",doc_write());
 }
 printf(" property=%s\n",failed?"RED":"GREEN");return failed;
}
