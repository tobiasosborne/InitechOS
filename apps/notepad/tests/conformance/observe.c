#include "bytes.h"
#include "tbx_host.h"
#define DA_AUX(s) ((void)(s))
#include "../../notepad.c"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);tbx_host_reset();
 if(!tbx_host_put_file(DA_PATH,fixture_bytes,(uint32_t)n)||tbx_app_main())return 2;
 int kept=g_keep;
 printf("%s\tbytes=%zu kept=%d count=%d page=%d shown=%d",argv[1],n,kept,g_count,g_page,g_te.teLength);
 /* Edit Note Pad, append clipboard clip in Scrapbook; then close/save. */
#ifdef SCRAPBOOK
 tbx_event_t remove={.what=TBX_EVT_MENU,.message=TBX_MENU_WORD(130,6)};
 tbx_app_event(&remove);
 tbx_host.scrap[0]='Z';tbx_host.scrap_len=1;
 tbx_event_t e={.what=TBX_EVT_MENU,.message=TBX_MENU_WORD(130,5)};
#else
 FlairTESetSelect(&g_te,0,g_te.teLength?1:0);
 tbx_event_t e={.what=keyDown,.message='Z'};
#endif
 tbx_app_event(&e);stash();int rc=save();const tbx_host_file_t *f=tbx_host_file(DA_PATH);
 int unchanged=f&&f->size==n&&!memcmp(fixture_bytes,f->data,n);
 printf(" save=%d original-unchanged=%d property=%s\n",rc,unchanged,kept&&!unchanged?"RED":"GREEN");
 return kept&&!unchanged;
}
