/* Real WP5.1 recovery: conformance/captures/W06{,-save,-reopen}.
 * The captured edited body is Readable [Bold on] Z, not discarded formatting.
 * Source bytes and saved semantics are independently checked by run.sh. */
#define main previous_face_main
#include "test_iword_face.c"
#undef main

static uint8_t input_bytes[80000];
static uint32_t input_n;
static uint32_t read_input(const char *name)
{
    FILE *f=fopen(name,"rb");
    if(!f){perror(name);exit(2);}
    uint32_t n=(uint32_t)fread(input_bytes,1,sizeof input_bytes,f);
    if(ferror(f)||!feof(f)||fclose(f)){fprintf(stderr,"fixture read failed\n");exit(2);}
    return n;
}
static void seed_input(void)
{
    tbx_host_reset();m_dir=0;
    g_mode=M_EDIT;g_current[0]=0;g_home=g_exiting=g_reveal=0;
    g_word_blocked=0;g_word_recovery[0]=0;
    CHECK(tbx_app_main()==0,"retrieve","launch fresh tenant");
    CHECK(tbx_host_put_file("CASE.WP5",input_bytes,input_n),"retrieve","seed pinned input");
    m_read_limit=2;
}
static void source_unchanged(void)
{
    const tbx_host_file_t *f=tbx_host_file("CASE.WP5");
    CHECK(f&&f->size==input_n&&!memcmp(f->data,input_bytes,input_n),
          "retrieve","Retrieve and Save As preserve original source bytes");
}
static void retrieve_case(void)
{ fkey(F10,SHIFT);tks("CASE.WP5\r"); }
static void write_result(const char *dir,const char *name)
{
    const tbx_host_file_t *f=tbx_host_file(name);
    CHECK(f!=0,"retrieve","saved result exists");
    if(!f)return;
    char path[1024];snprintf(path,sizeof path,"%s/%s",dir,name);
    FILE *out=fopen(path,"wb");
    if(!out||fwrite(f->data,1,f->size,out)!=f->size||fclose(out))exit(2);
}
static void recovered(const char *dir)
{
    static const uint8_t want[]={'R','e','a','d','a','b','l','e',0xc3,12,0xc3};
    seed_input();retrieve_case();
    CHECK(g_mode==M_EDIT&&!strcmp(g_current,"A:\\CASE.WP5"),"recovery","incomplete Bold retrieves as named document");
    uint8_t body[32];
    CHECK(g_ed.doc.len==sizeof want&&iw_doc_read(&g_ed.doc,0,body,sizeof body)==sizeof want&&
          !memcmp(body,want,sizeof want),"recovery","readable text plus repaired Bold-on from real WP capture");
    source_unchanged();
    if(g_mode!=M_EDIT)return;
    g_ed.cur=g_ed.doc.len;tk('Z');
    CHECK(g_ed.dirty,"recovery","append is editable");
    fkey(F10,0);tks("RECOVER.WP5\r");
    CHECK(g_mode==M_EDIT&&!g_ed.dirty,"recovery","recovered document saves");
    source_unchanged();write_result(dir,"RECOVER.WP5");
    /* Relaunch against the saved bytes, rather than leaving the live model. */
    const tbx_host_file_t *saved=tbx_host_file("RECOVER.WP5");
    if(!saved)return;
    uint8_t copy[1024];uint32_t n=saved->size;
    CHECK(n<=sizeof copy,"recovery","saved fixture fits");
    if(n>sizeof copy)return;
    memcpy(copy,saved->data,n);
    seed_input();CHECK(tbx_host_put_file("REOPEN.WP5",copy,n),"recovery","seed saved stream");
    fkey(F10,SHIFT);tks("REOPEN.WP5\r");
    CHECK(g_mode==M_EDIT&&g_ed.doc.len==sizeof want+1&&
          iw_doc_read(&g_ed.doc,0,body,sizeof body)==sizeof want+1&&
          !memcmp(body,want,sizeof want)&&body[sizeof want]=='Z',
          "recovery","fresh tenant reopens complete recovered text and formatting");
}
static void refused(void)
{
    seed_input();tks("Keep me");retrieve_case();
    CHECK(g_mode==M_ERROR&&g_err&&!strcmp(g_err,"ERROR: Incompatible File Format"),
          "recovery","wrong complete mirror still refused");
    uint8_t body[8];
    CHECK(g_ed.doc.len==7&&iw_doc_read(&g_ed.doc,0,body,sizeof body)==7&&!memcmp(body,"Keep me",7),
          "recovery","unsupported corruption keeps existing document");
    source_unchanged();
}
/* W11-save/reopen: all 65210 source x characters survive an appended Z.
 * Check the whole body with literal byte spans, without the product reader. */
static int all_x(uint32_t n)
{
    uint8_t body[80000];
    if(g_ed.doc.len!=n||iw_doc_read(&g_ed.doc,0,body,n)!=n)return 0;
    for(uint32_t i=0;i<n;i++)if(body[i]!='x')return 0;
    return 1;
}
static void large_document(const char *dir)
{
    seed_input();m_read_limit=97;retrieve_case();
    CHECK(g_mode==M_EDIT&&!strcmp(g_current,"A:\\CASE.WP5"),"large","65537-byte captured input retrieves");
    CHECK(all_x(65210),"large","every captured source character is present");
    source_unchanged();
    if(g_mode!=M_EDIT)return;
    g_ed.cur=g_ed.doc.len;tk('Z');
    CHECK(g_mode==M_EDIT&&g_ed.dirty,"large","append beyond the old file ceiling works");
    fkey(F10,0);tks("LARGE.WP5\r");
    CHECK(g_mode==M_EDIT&&!g_ed.dirty,"large","larger document saves");
    source_unchanged();write_result(dir,"LARGE.WP5");
    const tbx_host_file_t *saved=tbx_host_file("LARGE.WP5");
    if(!saved)return;
    uint8_t copy[80000];uint32_t n=saved->size;
    CHECK(n<=sizeof copy,"large","saved stream fits test buffer");
    if(n>sizeof copy)return;
    memcpy(copy,saved->data,n);seed_input();
    CHECK(tbx_host_put_file("REOPEN.WP5",copy,n),"large","seed saved larger stream");
    fkey(F10,SHIFT);tks("REOPEN.WP5\r");
    uint8_t body[65211];
    int same=g_mode==M_EDIT&&g_ed.doc.len==sizeof body&&
        iw_doc_read(&g_ed.doc,0,body,sizeof body)==sizeof body&&body[65210]=='Z';
    for(uint32_t i=0;same&&i<65210;i++)if(body[i]!='x')same=0;
    CHECK(same,"large","fresh tenant reopens every original character and final Z");
}
static void capacity_boundary(const char *dir)
{
    /* Authored 68 KiB limit fits the unchanged 512 KiB tenant BSS contract.
     * Synthetic valid WP text derives only its opaque prefix from W11. */
    memset(input_bytes+327,'x',69633-327);input_n=69632;
    seed_input();m_read_limit=97;retrieve_case();
    CHECK(g_mode==M_EDIT&&all_x(69305),"capacity","whole 69632-byte file opens at limit");
    if(g_mode==M_EDIT){
        g_ed.cur=g_ed.doc.len;tk('Z');
        CHECK(g_mode==M_ERROR&&g_err&&!strcmp(g_err,"ERROR: Document too large")&&all_x(69305),
              "capacity","excess edit refuses visibly without losing existing bytes");
        fkey(F1,0);fkey(F10,0);tks("FULL.WP5\r");
        CHECK(g_mode==M_EDIT&&!g_ed.dirty,"capacity","full document still saves after refused edit");
        source_unchanged();write_result(dir,"FULL.WP5");
    }
    input_n=69633;seed_input();tks("Keep me");retrieve_case();
    uint8_t body[8];
    CHECK(g_mode==M_ERROR&&g_err&&!strcmp(g_err,"ERROR: Document too large"),
          "capacity","one byte beyond limit refuses visibly");
    CHECK(g_ed.doc.len==7&&iw_doc_read(&g_ed.doc,0,body,sizeof body)==7&&!memcmp(body,"Keep me",7),
          "capacity","oversized Retrieve preserves working text");
    source_unchanged();
}
int main(int argc,char **argv)
{
    if(argc!=5)return 2;
    int counts[2]={0,0};g_ctr=counts;
    input_n=read_input(argv[2]);recovered(argv[1]);
    input_n=read_input(argv[3]);refused();
    input_n=read_input(argv[4]);large_document(argv[1]);capacity_boundary(argv[1]);
    printf("test_iword_retrieve: %d checks, %d failures\n",counts[0],counts[1]);
    return counts[1]?1:0;
}
