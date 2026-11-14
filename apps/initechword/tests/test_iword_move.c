/* initech-evky3. Captured WP List Files prompts/selection plus the stronger
 * reported-failure guarantee for BOTH prior file byte streams. No expected
 * bytes come from the product decoder or writer. run.sh reads saved WP files
 * independently. DOS 17 exercises the actual copy/remove fallback. */
#define IWORD_SAVE_FAULTS
#define IWORD_MOVE_FAULTS
#define main previous_face_main
#include "test_iword_face.c"
#undef main
#include <sys/stat.h>
static uint8_t source[1024],previous[1024];
static uint32_t source_n,previous_n;
static const char *target="TARGET.WP5";
static const char *scratch;
static uint32_t load(const char *path,uint8_t *bytes)
{
    FILE *f=fopen(path,"rb");if(!f){perror(path);exit(2);}
    uint32_t n=(uint32_t)fread(bytes,1,1024,f);
    if(!feof(f)||ferror(f)||fclose(f))exit(2);
    return n;
}
static int equal(const char *path,const uint8_t *bytes,uint32_t n)
{
    const tbx_host_file_t *f=tbx_host_file(path);
    return f&&f->size==n&&!memcmp(f->data,bytes,n);
}
static void seed(const char *tag,int fallback,int existing)
{
    static char dir[1024];snprintf(dir,sizeof dir,"%s/%s",scratch,tag);mkdir(dir,0700);
    m_dir=0;tbx_host_reset();
    save_fail_at=save_fail_again=save_call=save_failed=0;save_persistent=save_zero=0;
    target=fallback?"SUB\\TARGET.WP5":"TARGET.WP5";
    move_destination=target;move_source_delete=move_restore=move_retire_copy=0;move_delete_persistent=0;
    g_word_blocked=0;g_word_recovery[0]=0;
    g_mode=M_EDIT;g_home=g_exiting=g_reveal=0;g_current[0]=0;
    CHECK(tbx_app_main()==0,"move","fresh tenant");tks("Keep active.");
    CHECK(tbx_host_put_file("SOURCE.WP5",source,source_n),"move","seed genuine source");
    if(fallback){
        CHECK(tbx_host_put_file("SUB",0,0),"move","seed destination directory");
        ((tbx_host_file_t *)tbx_host_file("SUB"))->attr=16;
    }
    if(existing)CHECK(tbx_host_put_file(target,previous,previous_n),"move","seed distinct genuine target");
    m_dir=dir;fixture_export();
    lf_open("",0);g_lf_sel=0;
    for(uint32_t i=0;i<g_lf_n;i++)if(!strcmp(g_lf_ent[i].name,"SOURCE.WP5"))g_lf_sel=i+2;
    CHECK(g_lf_sel>=2,"move","select source");
    tk('3');tks(target);
    CHECK(g_mode==M_LFNAME,"move","captured Move/Rename name prompt");
    save_call=0;
}
static void result(int existing)
{
    save_fail_at=save_fail_again=0;
    if(g_mode==M_LFERROR||g_mode==M_ERROR){
        CHECK(equal("SOURCE.WP5",source,source_n),"move-failure","source keeps original pathname and bytes");
        CHECK(existing?equal(target,previous,previous_n):!tbx_host_file(target),
              "move-failure","preceding destination opens under its own name");
    }else{
        CHECK(g_mode==M_LIST&&!tbx_host_file("SOURCE.WP5")&&equal(target,source,source_n),
              "move-success","destination has exact source, source name removed");
    }
    uint8_t body[12];
    CHECK(g_ed.doc.len==12&&iw_doc_read(&g_ed.doc,0,body,12)==12&&!memcmp(body,"Keep active.",12),
          "move","active document preserved");
    fixture_export();
}
static unsigned perform(void)
{
    if(g_mode==M_LFNAME)tk('\r');
    if(g_mode==M_LFREPLACE)tk('y');
    return save_call;
}
static void compound(void)
{
    seed("compound-control",1,1);perform();unsigned source_delete=move_source_delete;result(1);
    CHECK(source_delete>0,"move-compound","fallback really removes source");
    seed("source-delete-control",1,1);save_fail_at=source_delete;perform();
    unsigned retire=move_retire_copy,restore=move_restore;result(1);
    CHECK(g_mode==M_LFERROR&&retire>source_delete&&restore>retire,
          "move-compound","single source-delete fault restored preceding destination");
    for(int fault=0;fault<2;fault++){
        seed(fault?"restore-fails":"retire-fails",1,1);
        save_fail_at=source_delete;save_fail_again=fault?restore:retire;perform();
        CHECK(save_failed==2&&g_mode==M_LFERROR&&g_word_blocked,
              "move-compound","paired failure disclosed and writes blocked");
        CHECK(equal("SOURCE.WP5",source,source_n)&&equal(g_word_original,previous,previous_n),
              "move-compound","both previous streams retained; exact prior destination path named");
        CHECK(equal(fault?g_lf_move_stage:target,source,source_n),
              "move-compound","complete replacement copy also retained");
        CHECK(row_is(21,"ERROR: Original file retained as:")&&row_is(22,g_word_original),
              "move-compound","recovery name appears completely in error display");
        save_fail_at=save_fail_again=0;unsigned before=save_call;
        lf_perform();write_file();
        CHECK(save_call==before&&equal(g_word_original,previous,previous_n),
              "move-compound","later file operation/save cannot delete retained predecessor");
        fixture_export();
    }
}
static void retries(void)
{
    seed("persistent-source-delete",1,1);move_delete_persistent=1;perform();
    CHECK(g_mode==M_LFERROR&&move_source_delete>0,"move-delete","persistent delete failure reached final source removal");
    result(1);
    move_delete_persistent=0;tbx_host.file_error[TBX_FILE_DELETE]=0;
    g_mode=M_LFNAME;perform();result(1);
    CHECK(equal("SUB\\TARGET.N00",source,source_n),"move-delete","refused cleanup retained complete copy, never overwritten on retry");
    seed("full-retry",1,1);tbx_host.write_budget=0;perform();
    CHECK(g_mode==M_LFERROR&&g_lf_full&&row_is(23,"Insufficient disk space -- Insert new diskette and press any key"),
          "move-full","WP5.1 captured disk-full retry prompt");result(1);
    tk(' ');CHECK(g_mode==M_LFERROR&&g_lf_full,"move-full","repeated full-disk refusal");result(1);
    tbx_host.write_budget=-1;tk(' ');result(1);
    seed("short-collisions",1,1);
    CHECK(tbx_host_put_file("SUB\\TARGET.M00",previous,previous_n)&&
          tbx_host_put_file("SUB\\TARGET.N00",previous,previous_n),"move-collision","seed unowned complete files");
    fixture_export();m_read_limit=3;m_write_limit=2;perform();result(1);
    CHECK(equal("SUB\\TARGET.M00",previous,previous_n)&&equal("SUB\\TARGET.N00",previous,previous_n),
          "move-collision","occupied names never truncated or deleted");
    seed("long-source",1,1);
    char path[128]="";
    for(int i=0;i<6;i++){
        if(i)strcat(path,"\\");
        strcat(path,"ABCDEFGH");
        CHECK(tbx_host_put_file(path,0,0),"move-long","source parent exists");
        ((tbx_host_file_t *)tbx_host_file(path))->attr=16;
    }
    strcat(path,"\\SOURCE.WP5");
    CHECK(tbx_host_put_file(path,source,source_n),"move-long","long source seeded");
    fixture_export();scopy(g_lf_src,path);scopy(g_lf_dst,target);lf_perform();
    CHECK(g_mode==M_LFERROR&&equal(path,source,source_n)&&equal(target,previous,previous_n),
          "move-long","shared recovery-path limit refuses before replacing either file");
    fixture_export();
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    static int counters[2];g_ctr=counters;scratch=argv[1];
    source_n=load(argv[2],source);previous_n=load(argv[3],previous);
    CHECK(source_n!=previous_n,"move","distinct independently captured documents");
    /* Existing-target matrix includes the final source DELETE from the review.
     * Baselines discover boundaries but never compute expected disk state. */
    for(int existing=0;existing<=1;existing++)for(int fallback=0;fallback<=1;fallback++){
        char tag[80];snprintf(tag,sizeof tag,"e%d-f%d-ok",existing,fallback);seed(tag,fallback,existing);
        unsigned n=perform();result(existing);
        printf("move-boundaries: existing=%d fallback=%d calls=%u\n",existing,fallback,n);
        for(unsigned at=1;at<=n;at++)for(int zero=0;zero<=1;zero++){
            snprintf(tag,sizeof tag,"e%d-f%d-at%u-z%d",existing,fallback,at,zero);seed(tag,fallback,existing);
            save_fail_at=at;save_zero=zero;perform();
            CHECK(save_failed==1,"move-fault","selected file boundary exercised");
            result(existing);
            /* A repeated refusal cannot poison the next attempt. */
            if(g_mode==M_LFERROR){
                g_mode=M_LFNAME;save_call=0;save_fail_at=at;save_failed=0;perform();result(existing);
                if(g_mode==M_LFERROR){g_mode=M_LFNAME;save_call=0;save_fail_at=0;perform();result(existing);}
            }
        }
    }
    compound();retries();
    printf("test_iword_move: %d checks, %d failures\n",counters[0],counters[1]);
    return counters[1]?1:0;
}
