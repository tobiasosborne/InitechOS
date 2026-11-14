#include "bytes.h"
#include "tbx_host.h"
#include "../../calc.c"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);tbx_host_reset();
 if(!tbx_host_put_file("APPS\\LEDGER.TXT",bytes,(uint32_t)n)||!tbx_host_put_file("APPS\\TABLES.TXT","GOOD LEDGER\r\n",13)||tbx_app_main())return 2;
 /* There is no Open verb. Launch draws the built-in ledger, then Save
 * requests a copy of the on-disk ledger. Record the requested source bytes;
 * the SDK fake does not execute FILE COPY, so do NOT claim disk overwrite. */
 begin_save();tbx_event_t e={.what=TBX_EVT_ALERT,.message=TBX_ALERT_DEFAULT};tbx_app_event(&e);
 const tbx_host_file_t *f=tbx_host_file(tbx_host.copy_src);
 printf("%s\tbytes=%zu windows=%d copy-src=%s copy-dst=%s requested-bytes=%u alerts=%d\n",argv[1],n,tbx_host.nwindows,tbx_host.copy_src,tbx_host.copy_dst,f?f->size:0,tbx_host.alerts);
 return 0;
}
