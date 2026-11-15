/* Move is copy-then-delete across directories (operator, 2026-11-15;
 * WP5.1 Reference347-353 and captured Move/replace prompts). Once replacement
 * commits, deleting the source is cleanup: refusal leaves two complete copies
 * and a named notice. The independent WP reader/hash checks every exported
 * disk state. No renderer/codec computes expected bytes. */
#define IWORD_SAVE_FAULTS
#define IWORD_MOVE_FAULTS
#define main previous_face_main
#include "test_iword_face.c"
#undef main
#include <sys/stat.h>
static uint8_t source[1024],previous[1024];
static uint32_t source_n,previous_n;
static const char *target,*scratch;
static const char *case_name;
static int has_row(int line,const char *text)
{ size_t n=strlen(text);return !memcmp(m_scr[line],text,n)&&m_scr[line][n]==' '; }
static uint32_t load(const char *path,uint8_t *bytes)
{ FILE *f=fopen(path,"rb");if(!f){perror(path);exit(2);}uint32_t n=fread(bytes,1,1024,f);if(!feof(f)||ferror(f)||fclose(f))exit(2);return n; }
static int equal(const char *path,const uint8_t *bytes,uint32_t n)
{ const tbx_host_file_t *f=tbx_host_file(path);return f&&f->size==n&&!memcmp(f->data,bytes,n); }
static void seed(const char *tag,int cross,int existing)
{
 static char dir[1024];snprintf(dir,sizeof dir,"%s/%s",scratch,tag);if(mkdir(dir,0700))exit(2);
 m_dir=0;tbx_host_reset();save_fail_at=save_fail_again=save_call=save_failed=0;save_persistent=save_zero=0;
 target=cross?"SUB\\TARGET.WP5":"TARGET.WP5";move_destination=target;move_source_path="SOURCE.WP5";
 move_source_delete=move_restore=move_retire_copy=0;move_delete_persistent=0;
 g_mode=M_EDIT;g_home=g_exiting=g_reveal=0;g_current[0]=0;
 CHECK(tbx_app_main()==0,"move","fresh tenant");tks("Keep active.");
 CHECK(tbx_host_put_file("SOURCE.WP5",source,source_n),"move","seed captured source");
 if(cross){CHECK(tbx_host_put_file("SUB",0,0),"move","seed directory");((tbx_host_file_t*)tbx_host_file("SUB"))->attr=16;}
 if(existing)CHECK(tbx_host_put_file(target,previous,previous_n),"move","seed distinct captured target");
 case_name=tag;m_dir=dir;fixture_export();lf_open("",0);g_lf_sel=0;
 for(uint32_t i=0;i<g_lf_n;i++)if(!strcmp(g_lf_ent[i].name,"SOURCE.WP5"))g_lf_sel=i+2;
 CHECK(g_lf_sel>=2,"move","select source");tk('3');tks(target);
 CHECK(g_mode==M_LFNAME,"move","captured Move/Rename prompt");save_call=0;
}
static unsigned perform(void)
{ if(g_mode==M_LFNAME)tk('\r');if(g_mode==M_LFREPLACE)tk('y');return save_call; }
static void result(int existing)
{
 save_fail_at=save_fail_again=0;
 /* Warning text is the observable success distinction, not a hidden flag. */
 int warning=has_row(21,"File moved; source remains:")||has_row(21,"File moved; list unavailable:");
 int failed=(g_mode==M_LFERROR||g_mode==M_ERROR)&&!warning;
 if(failed){
  CHECK(equal("SOURCE.WP5",source,source_n),"move-failure","source retains original name and bytes");
  CHECK(existing?equal(target,previous,previous_n):!tbx_host_file(target),"move-failure","previous target retains original name and bytes");
 }else{
  CHECK(equal(target,source,source_n),"move-commit","destination has exact captured source");
  if(tbx_host_file("SOURCE.WP5")){
   CHECK(equal("SOURCE.WP5",source,source_n),"move-cleanup","both complete copies retained");
   CHECK(warning&&row_is(22,"SOURCE.WP5"),"move-cleanup","period notice names exact source left behind");
  }
 }
 uint8_t body[12];CHECK(g_ed.doc.len==12&&iw_doc_read(&g_ed.doc,0,body,12)==12&&!memcmp(body,"Keep active.",12),"move","active text preserved");
 fixture_export();
 printf("move-readback %s %s %s %d SOURCE.WP5\n",failed?"failed":tbx_host_file("SOURCE.WP5")?"left":"moved",case_name,target,existing);
}
static void cleanup(void)
{
 seed("cleanup-control",1,1);perform();unsigned at=move_source_delete;result(1);
 CHECK(at>0,"move-cleanup","cross-directory control deletes source");
 for(unsigned later=at+1;later<=at+8;later++){
  char tag[40];snprintf(tag,sizeof tag,"paired-%u",later);
  seed(tag,1,1);save_fail_at=at;save_fail_again=later;perform();
  CHECK(save_failed>=1,"move-paired","source cleanup fault reached");result(1);
  CHECK(equal(target,source,source_n)&&equal("SOURCE.WP5",source,source_n),"move-paired","later failure never rolls back or hides either complete copy");
 }
 seed("cleanup-refused",1,1);save_fail_at=at;perform();result(1);
 CHECK(save_failed==1&&equal(target,source,source_n)&&equal("SOURCE.WP5",source,source_n),"move-cleanup","source-delete refusal commits replacement and retains both files");
 CHECK(row_is(21,"File moved; source remains:")&&row_is(22,"SOURCE.WP5"),"move-cleanup","warning identifies complete source left behind");
 unsigned before=save_call;tk(' ');CHECK(save_call==before,"move-cleanup","acknowledgement cannot repeat committed operation");
 /* Another document save must remain usable after a cleanup refusal. */
 g_mode=M_EDIT;g_settings[6]=0;g_nlen=0;g_name[0]=0;g_fresh=0;
 g_mode=M_SAVE_NAME;tks("AFTER.WP5");tk('\r');
 CHECK(tbx_host_file("AFTER.WP5")!=0,"move-cleanup","later save is not globally blocked");fixture_export();
 seed("long-source-notice",1,1);
 static char path[128];path[0]=0;
 for(unsigned i=0;i<9;i++){
  if(i)strcat(path,"\\");
  strcat(path,"ABCDEFGH");
  CHECK(tbx_host_put_file(path,0,0),"move-long","source parent seeded");((tbx_host_file_t*)tbx_host_file(path))->attr=16;
 }
 strcat(path,"\\SOURCE.WP5");
 CHECK(tbx_host_put_file(path,source,source_n),"move-long","long source seeded");fixture_export();
 scopy(g_lf_src,path);scopy(g_lf_dst,target);move_source_path=path;move_delete_persistent=1;lf_perform();paint();
 CHECK(equal(path,source,source_n)&&equal(target,source,source_n),"move-long","source cleanup refusal keeps both exact complete files");
 char first[SCR_W+1];memcpy(first,path,SCR_W);first[SCR_W]=0;
 CHECK(row_is(21,"File moved; source remains:")&&row_is(22,first)&&row_is(23,path+SCR_W),"move-long","complete long source path spans two rows without truncation");fixture_export();
 printf("move-readback left long-source-notice %s 1 %s\n",target,path);
}
int main(int argc,char **argv)
{
 if(argc!=4)return 2;
 static int counters[2];g_ctr=counters;scratch=argv[1];
 source_n=load(argv[2],source);previous_n=load(argv[3],previous);
 CHECK(source_n!=previous_n,"move","distinct independently captured documents");
 for(int e=0;e<=1;e++)for(int c=0;c<=1;c++){
  char tag[80];snprintf(tag,sizeof tag,"e%d-c%d-ok",e,c);seed(tag,c,e);unsigned n=perform();result(e);
  for(unsigned at=1;at<=n;at++){
   snprintf(tag,sizeof tag,"e%d-c%d-at%u",e,c,at);seed(tag,c,e);save_fail_at=at;perform();
   CHECK(save_failed==1,"move-fault","chosen file boundary exercised");result(e);
  }
 }
 cleanup();
 printf("test_iword_move: %d checks, %d failures\n",counters[0],counters[1]);return counters[1]?1:0;
}
