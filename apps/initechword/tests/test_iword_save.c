/* Save authority: real WP5.1 collision/full/readonly captures in
 * goldens/dataloss/wordperfect, plus phase-2 original-name preservation.
 * Each reported error is inspected before dismissal/retry. No product codec
 * computes expected bytes; run.sh also decodes every exported WP5/BK! with
 * the independently pinned WP5 reader. */
#define IWORD_SAVE_FAULTS
#define main previous_face_main
#include "test_iword_face.c"
#undef main
#include <sys/stat.h>

static uint8_t original[65536], older[65536];
static uint32_t original_n, older_n;
static unsigned total_calls;
static uint32_t load(const char *path,uint8_t *b)
{
    FILE *f=fopen(path,"rb"); if(!f){perror(path);exit(2);}
    uint32_t n=(uint32_t)fread(b,1,65536,f);fclose(f);return n;
}
static int disk_is(const char *path,const uint8_t *b,uint32_t n)
{
    const tbx_host_file_t *f=tbx_host_file(path);
    return f && f->size==n && !memcmp(f->data,b,n);
}
static void seed(const char *dir,int backup,int exists,int readonly)
{
    save_fail_at=save_fail_again=save_call=save_failed=0;save_persistent=save_zero=save_reclaim_backup=0;
    tbx_host_reset();m_dir=0;g_mode=M_EDIT;g_current[0]=0;g_home=0;g_exiting=0;g_reveal=0;
    CHECK(tbx_app_main()==0,"save","launch");
    g_settings[6]=(uint8_t)backup;
    if(exists) CHECK(tbx_host_put_file("NAME.WP5",original,original_n),"save","seed captured original");
    CHECK(tbx_host_put_file("NAME.BK!",older,older_n),"save","seed captured older backup");
    if(readonly)((tbx_host_file_t *)tbx_host_file("NAME.BK!"))->attr=0x21;
    /* Retrieve through the actual Toolbox; retain formatting from real WP. */
    if(exists){fkey(F10,SHIFT);tks("NAME.WP5\r");}
    else tks("New document");
    tks("REVISION");
    CHECK(g_ed.dirty,"save","edited document dirty");
    m_dir=dir;if(dir){mkdir(dir,0700);fixture_export();}
    save_call=0;
}
static void save(void)
{
    fkey(F10,0);tks("NAME.WP5\r");
    if(g_mode==M_REPLACE)tk('y');
}
static void save_as(const char *name)
{
    fkey(F10,0);tks(name);tk('\r');
    if(g_mode==M_REPLACE)tk('y');
}
static void check_result(int backup,int exists)
{
    unsigned calls=save_call;save_fail_at=0;
    if(g_mode==M_ERROR){
        CHECK(g_ed.dirty,"save-failure","edited document stays dirty");
        CHECK(!strcmp(g_current,exists?"A:\\NAME.WP5":""),"save-failure","current name unchanged");
        if(exists)CHECK(disk_is("NAME.WP5",original,original_n),"save-failure","previous document opens under its own name");
        else CHECK(!tbx_host_file("NAME.WP5"),"save-failure","failed first save creates no document");
    }else{
        CHECK(g_mode==M_EDIT&&!g_ed.dirty,"save-success","save completed");
        const tbx_host_file_t *f=tbx_host_file("NAME.WP5");
        CHECK(f&&f->size>(exists?original_n:327u),"save-success","new version committed");
        if(backup&&exists) CHECK(disk_is("NAME.BK!",original,original_n),"save-success","exact preceding version replaces older backup");
        else CHECK(disk_is("NAME.BK!",older,older_n),"save-success","backup off or first save retains older backup");
    }
    /* A failed backup rotation may remove the older backup on today's helper;
     * it must never expose a partial WP5 file as an original backup. */
    if(tbx_host_file("NAME.BK!"))CHECK(disk_is("NAME.BK!",original,original_n)||disk_is("NAME.BK!",older,older_n),"save","backup is complete captured document");
    fixture_export();
    for(int i=0;i<TBX_HOST_HANDLES;i++)CHECK(!tbx_host.handles[i].used,"save","no leaked handle");
    total_calls=calls;
}
/* Every ordered pair of failed boundaries (negative results), backup on and
 * off. A reported failure must leave the captured previous document openable
 * under NAME.WP5. Each such disk state is written out flat and run.sh hashes
 * every copy against the pinned capture, outside Word and the SDK fake. */
static unsigned pair_failures, pair_exports;
static void export_previous(const char *dir,int backup,unsigned a,unsigned b)
{
    char path[1100];
    const tbx_host_file_t *f=tbx_host_file("NAME.WP5");
    pair_failures++;
    if(!f)return;                        /* counted, not exported: run.sh goes red */
    snprintf(path,sizeof path,"%s/b%d-%u-%u.WP5",dir,backup,a,b);
    FILE *o=fopen(path,"wb");
    if(!o||fwrite(f->data,1,f->size,o)!=f->size||fclose(o)){perror(path);exit(2);}
    pair_exports++;
}
static void pairs(const char *root,int backup)
{
    char dir[1024],out[1024];
    snprintf(dir,sizeof dir,"%s/pairs-b%d-baseline",root,backup);
    seed(dir,backup,1,0);save();check_result(backup,1);
    unsigned n=total_calls;
    snprintf(out,sizeof out,"%s/pairs",root);mkdir(out,0700);
    for(unsigned a=1;a<n;a++)for(unsigned b=a+1;b<=n;b++){
        seed(0,backup,1,0);save_fail_at=a;save_fail_again=b;save();
        CHECK(save_failed>=1,"save-pair","first selected boundary was exercised");
        if(g_mode==M_ERROR)export_previous(out,backup,a,b);
        check_result(backup,1);
    }
    /* After the last pair the editor still holds the edit: an immediate retry
     * under the same name, and a save to a fresh name, both complete. */
    seed(0,backup,1,0);save_fail_at=n-1;save_fail_again=n;save();
    CHECK(g_mode==M_ERROR,"save-retry","final-boundary pair reports failure");
    check_result(backup,1);
    tk(' ');fkey(F1,0);save();
    CHECK(g_mode==M_EDIT&&!g_ed.dirty&&!strcmp(g_current,"A:\\NAME.WP5"),"save-retry","retry after a reported failure saves");
    const tbx_host_file_t *f=tbx_host_file("NAME.WP5");
    CHECK(f&&f->size>original_n,"save-retry","retry committed the edited version");
    if(backup)CHECK(disk_is("NAME.BK!",original,original_n),"save-retry","retry rotates the retained previous version");
    seed(0,backup,1,0);save_fail_at=n-1;save_fail_again=n;save();check_result(backup,1);
    tk(' ');fkey(F1,0);save_as("FRESH.WP5");
    CHECK(g_mode==M_EDIT&&!g_ed.dirty&&tbx_host_file("FRESH.WP5"),"save-retry","fresh name after a reported failure saves");
    CHECK(disk_is("NAME.WP5",original,original_n),"save-retry","fresh-name save leaves the previous document alone");
}
/* A 53-character path: every single failed boundary keeps the previous
 * document under its own long name. */
static void long_path(const char *root)
{
    static const char target[]="ABCDEFGH\\IJKLMNOP\\QRSTUVWX\\ABCDEFGH\\IJKLMNOP\\NAME.WP5";
    char dir[1024],parent[128];unsigned calls=0;
    for(unsigned fault=0;fault<=calls;fault++){
        snprintf(dir,sizeof dir,"%s/long-%u",root,fault);seed(fault?0:dir,0,1,0);
        for(unsigned i=0;target[i];i++)if(target[i]=='\\'){
            memcpy(parent,target,i);parent[i]=0;
            CHECK(tbx_host_put_file(parent,0,0),"save-long","seed parent directory");
            ((tbx_host_file_t *)tbx_host_file(parent))->attr=16;
        }
        CHECK(tbx_host_put_file(target,original,original_n),"save-long","seed long-path document");
        fixture_export();save_call=0;save_fail_at=fault;
        save_as(target);
        if(!fault){calls=save_call;CHECK(g_mode==M_EDIT&&!g_ed.dirty,"save-long","baseline save");continue;}
        CHECK(save_failed==1,"save-long","selected boundary was exercised once");
        if(g_mode==M_ERROR)CHECK(disk_is(target,original,original_n),"save-long","previous document keeps its long name");
        else CHECK(tbx_host_file(target)&&tbx_host_file(target)->size>original_n,"save-long","nonfatal fault still commits");
        save_fail_at=0;
    }
}
int main(int argc,char **argv)
{
    static int ctr[2];g_ctr=ctr;
    if(argc!=4)return 2;
    original_n=load(argv[2],original);older_n=load(argv[3],older);
    char dir[1024];
    for(int exists=0;exists<=1;exists++)for(int backup=0;backup<=1;backup++){
        snprintf(dir,sizeof dir,"%s/e%d-b%d-ok",argv[1],exists,backup);
        seed(dir,backup,exists,0);save();check_result(backup,exists);
        unsigned n=total_calls;
        printf("save-boundaries: existing=%d backup=%d calls=%u; negative and zero variants\n",exists,backup,n);
        for(unsigned at=1;at<=n;at++)for(int zero=0;zero<=1;zero++){
            snprintf(dir,sizeof dir,"%s/e%d-b%d-f%u-z%d",argv[1],exists,backup,at,zero);
            seed(dir,backup,exists,0);save_fail_at=at;save_zero=zero;save();
            CHECK(save_failed==1,"save-fault","selected boundary was exercised once");
            check_result(backup,exists);
        }
    }
    pairs(argv[1],0);pairs(argv[1],1);long_path(argv[1]);
    printf("save-pairs: reported failures=%u exported=%u\n",pair_failures,pair_exports);
    /* Short transfers and disk exhaustion in both backup and document writes.
     * write_budget models DOS short/zero writes, independent of the editor. */
    for(int backup=0;backup<=1;backup++)for(unsigned budget=0;budget<4;budget++){
        snprintf(dir,sizeof dir,"%s/full-b%d-%u",argv[1],backup,budget);
        seed(dir,backup,1,0);
        int limit=budget==0?0:budget==1?100:budget==2?(int)original_n-1:
                  (int)original_n+(backup?200:7);
        tbx_host.write_budget=limit;m_read_limit=3;m_write_limit=2;
        save();CHECK(g_mode==M_ERROR,"save-full","full-disk save reports failure");
        check_result(backup,1);
    }
    static const unsigned ops[]={TBX_FILE_OPEN,TBX_FILE_CREATE_NEW,TBX_FILE_READ,
        TBX_FILE_WRITE,TBX_FILE_SEEK,TBX_FILE_CLOSE,TBX_FILE_DELETE,TBX_FILE_RENAME};
    for(unsigned i=0;i<sizeof ops/sizeof ops[0];i++){
        snprintf(dir,sizeof dir,"%s/persistent-%u",argv[1],i);seed(dir,1,1,0);
        tbx_host.file_error[ops[i]]=5;save();
        CHECK(g_mode==M_ERROR,"save-persistent","persistent file error reported");
        check_result(1,1);
    }
    snprintf(dir,sizeof dir,"%s/captured-full",argv[1]);seed(dir,1,1,0);
    tbx_host.write_budget=0;save_reclaim_backup=1;save();
    CHECK(g_mode==M_ERROR,"save-captured-full","zero-free-byte save reports failure");
    CHECK(disk_is("NAME.BK!",original,original_n),"save-captured-full","captured full-disk backup rotation plus original-name invariant");
    unsigned files=0;
    for(int i=0;i<TBX_HOST_FILES;i++)files+=tbx_host.files[i].used!=0;
    CHECK(files==2,"save-captured-full","only original and backup remain, with no partial document");
    check_result(1,1);
    snprintf(dir,sizeof dir,"%s/collisions",argv[1]);seed(dir,1,1,0);
    CHECK(tbx_host_put_file("NAME.W00",older,older_n)&&tbx_host_put_file("NAME.N00",older,older_n)&&tbx_host_put_file("NAME.$$$",older,older_n),"save","seed unrelated scratch files");
    fixture_export();save();check_result(1,1);
    CHECK(disk_is("NAME.W00",older,older_n)&&disk_is("NAME.N00",older,older_n)&&disk_is("NAME.$$$",older,older_n),"save","unowned scratch names are untouched");
    snprintf(dir,sizeof dir,"%s/short",argv[1]);seed(dir,1,1,0);
    m_read_limit=3;m_write_limit=2;save();check_result(1,1);
    /* The third save rotates the immediately preceding save, not the retrieved
     * original. This conservation property uses bytes read from disk, never
     * an expected value produced by the WP5 writer. */
    const tbx_host_file_t *previous=tbx_host_file("NAME.WP5");
    uint8_t preceding[65536];uint32_t preceding_n=previous->size;
    memcpy(preceding,previous->data,preceding_n);tk('X');save();
    CHECK(g_mode==M_EDIT&&!g_ed.dirty,"save-third","third save completes");
    CHECK(disk_is("NAME.BK!",preceding,preceding_n),"save-third","backup equals immediately preceding disk version");
    fixture_export();
    /* A backup is an ordinary WP5 document. Its captured NAME.BK! spelling
     * must also be accepted by the document filename prompt. */
    doc_reset();g_current[0]=0;g_mode=M_EDIT;
    fkey(F10,SHIFT);tks("NAME.BK!\r");
    CHECK(g_mode==M_EDIT&&!strcmp(g_current,"A:\\NAME.BK!")&&!g_ed.dirty,
          "save-retrieve-backup","generated .BK! opens through Retrieve");
    snprintf(dir,sizeof dir,"%s/readonly",argv[1]);seed(dir,1,1,1);save();
    CHECK(g_mode==M_ERROR,"save-readonly","read-only backup refuses save");
    CHECK(disk_is("NAME.WP5",original,original_n)&&disk_is("NAME.BK!",older,older_n),"save-readonly","both files unchanged (real WP control)");
    CHECK(tbx_host_file("NAME.BK!")->attr&1,"save-readonly","read-only attribute retained");
    check_result(1,1);
    printf("test-iword-save: %d checks, %d failures\n",ctr[0],ctr[1]);
    return ctr[1]?1:0;
}
