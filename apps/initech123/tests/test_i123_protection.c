/* Real Lotus R2.2 files and screens decide protection, including partial
 * Name Table writes. Authority: goldens/dataloss/README.md, Lotus section.
 * Compare raw cell/name/protection records; the expected side uses no part
 * of the product codec. Window records differ because this is a GUI face. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

static const char *capture_root, *writes_root, *scenario;
static uint8_t expected[65536], input[65536];
static unsigned load(const char *leaf, uint8_t *bytes)
{
    char path[1024];
    snprintf(path,sizeof path,"%s/%s/%s",capture_root,scenario,leaf);
    FILE *f=fopen(path,"rb");
    if(!f){perror(path);exit(2);}
    unsigned n=(unsigned)fread(bytes,1,65536,f);
    if(ferror(f)||!feof(f)){fclose(f);exit(2);}fclose(f);return n;
}
static unsigned le16(const uint8_t *b){return b[0]+256u*b[1];}
static int selected(unsigned op)
{
    return (op>=0x0c&&op<=0x10)||op==0x33||op==0x47||op==0x24;
}
/* Independent record framing: Lotus WK1 uses u16 opcode, u16 body length.
 * Cell/name order is compared too; both real Lotus and this writer sort it.
 * Other records describe printer, graph and GUI window state, not writes. */
static int same_records(const uint8_t *a,unsigned an,const uint8_t *b,unsigned bn)
{
    unsigned ap=0,bp=0;
    for(;;){
        while(ap+4<=an&&!selected(le16(a+ap)))ap+=4+le16(a+ap+2);
        while(bp+4<=bn&&!selected(le16(b+bp)))bp+=4+le16(b+bp+2);
        if(ap==an&&bp==bn)return 1;
        if(ap+4>an||bp+4>bn)return 0;
        unsigned n=4+le16(a+ap+2),m=4+le16(b+bp+2);
        if(n>an-ap||m>bn-bp||n!=m||memcmp(a+ap,b+bp,n)){
            fprintf(stderr,"%s record mismatch: expected op %04x at %u, got %04x at %u\n",
                    scenario,le16(a+ap),ap,le16(b+bp),bp);return 0;
        }
        ap+=n;bp+=m;
    }
}
static void table_case(void)
{
    unsigned n=load("BEFORE.WK1",input);
    CHECK(tbx_host_put_file("INPUT.WK1",input,n),scenario,"install real BEFORE bytes");
    tks("/frinput\r");
    CHECK(g_mode==M_READY&&!g_dirty,scenario,"retrieve real BEFORE worksheet");
    n=load("result.txt",expected);expected[n]=0;
    int refused=strstr((char *)expected,"Protected cell")!=NULL;
    if(!strcmp(scenario,"labels"))tks("/rnlrD10\r");
    else tks("/rntA10\r");
    CHECK(refused ? (g_mode==M_NOTICE&&line_has(0,"ERROR")&&line_has(1,"Protected cell"))
                  : g_mode==M_READY,scenario,"result agrees with captured screen");
    if(!strcmp(scenario,"allprotected")||!strcmp(scenario,"firstprotected"))
        CHECK(!g_dirty,scenario,"refusal before first write keeps clean worksheet");
    else CHECK(g_dirty,scenario,"allowed writes mark worksheet dirty, including prefix on error");
    if(g_mode==M_NOTICE)tk(27);
    tks("/fsout\r");
    const tbx_host_file_t *f=tbx_host_file("OUT.WK1");
    n=load("AFTER.WK1",expected);
    CHECK(f&&same_records(expected,n,f->data,f->size),scenario,
          "saved cell, name and protection bytes match real AFTER.WK1");
}
static unsigned load_write(const char *prefix,const char *suffix,uint8_t *bytes)
{
    char path[1024];
    snprintf(path,sizeof path,"%s/%s%s.%s",writes_root,prefix,scenario,suffix);
    FILE *f=fopen(path,"rb");if(!f){perror(path);exit(2);}
    unsigned n=(unsigned)fread(bytes,1,65536,f);
    if(ferror(f)||!feof(f)){fclose(f);exit(2);}fclose(f);return n;
}
static void operation_case(void)
{
    static const char *commands[]={
        "/cA1..A2\rD1\r", "/cA1..A2\rD1\r", "/mA1..A2\rD1\r",
        "/mA1..A2\rD1\r", "/rvA1..A2\rD1\r", "/rtA1..B1\rD1\r",
        "/reA1..A2\r", "/rff2\rA1..A2\r", "/ruA1..A2\r",
        "99\r", "99\r", "9\r", "/wey", "/cA1..B2\rD1\r",
        "/mA1..B2\rD1\r", "/cH1..H2\rD1\r", "/cA1\rD1..D2\r",
        "/rtA1..B2\rD1\r", "/rlrA1..A2\r", "/rlrA1..A2\r",
        "/mA1..A2\rA2\r", "/rlrA1..A2\r", "/rntA10\r", "/rntA10\r",
        "/rntA10\r", "/rntA10\r", "/rntIU8192\r", "/mA1..A2\rD1\r",
        "/mA1..A2\rD1\r", "/rvA1..B2\rD1\r", "/reA1..B2\r", "/rlrA1..B2\r"
    };
    unsigned id=(unsigned)atoi(scenario),n=load_write("B","WK1",input);
    CHECK(tbx_host_put_file("INPUT.WK1",input,n),scenario,"install real BEFORE bytes");
    tks("/frinput\r");
    CHECK(g_mode==M_READY&&!g_dirty,scenario,"retrieve real BEFORE worksheet");
    n=load_write("R","txt",expected);expected[n]=0;
    int refused=strstr((char *)expected,"Protected cell")!=NULL;
    int boundary=strstr((char *)expected,"Invalid cell or range address")!=NULL;
    if(id==10||id==11||id==12){
        ev(keyDown,VK_F5<<8);tks(id==11?"H1\r":"A1\r");
        if(id==12){ev(keyDown,VK_F2<<8);tk(8);}
    }
    tks(commands[id-1]);
    CHECK(refused ? (g_mode==M_NOTICE&&line_has(0,"ERROR")&&line_has(1,"Protected cell"))
          : boundary ? (g_mode==M_NOTICE&&line_has(0,"ERROR")&&line_has(1,"Invalid cell or range address"))
          : g_mode==M_READY,scenario,"result agrees with captured screen");
    if(g_mode==M_NOTICE)tk(27);
    tks("/fsout\r");
    const tbx_host_file_t *f=tbx_host_file("OUT.WK1");
    n=load_write("A","WK1",expected);
    CHECK(f&&same_records(expected,n,f->data,f->size),scenario,
          "saved cell, name and protection bytes match real AFTER.WK1");
}
static void forget_output(void)
{ char path[1024];snprintf(path,sizeof path,"%s/OUT.WK1",m_dir);unlink(path); }
/* Integration probe for the shared SDK owner. The protection suite does not
 * claim the SDK's save guarantee. Run explicitly with --save-safety while
 * landing tbx_save.h; the previous file must survive a reported rename error. */
static void save_safety(void)
{
    unsigned n=load("BEFORE.WK1",input);
    CHECK(tbx_host_put_file("INPUT.WK1",input,n),"save","install saved worksheet");
    tks("/frinput\r/wgpd");
    menu_quit();answer(1);tk('\r');
    CHECK(g_mode==M_REPLACE,"save","quit Save offers Replace for the original name");
    tbx_host.file_error[TBX_FILE_RENAME]=5;
    tk('r');
    CHECK(g_mode==M_NOTICE&&g_dirty&&!m_exit_calls,"save","reported error keeps edited sheet open");
    const tbx_host_file_t *f=tbx_host_file("INPUT.WK1");
    CHECK(f&&f->size==n&&!memcmp(f->data,input,n),"save",
          "previous worksheet still opens under its original filename after reported failure");
}
int main(int argc,char **argv)
{
    if(argc!=4&&argc!=5)return 2;
    m_dir=argv[1];capture_root=argv[2];writes_root=argv[3];
    g_ctr=mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    if(argc==5){
        if(strcmp(argv[4],"--save-safety"))return 2;
        scenario="allprotected";launch("save safety",save_safety);
        printf("i123-save-safety: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
        return g_ctr[1]?1:0;
    }
    const char *cases[]={"disabled","allprotected","firstprotected","secondprotected",
                         "terminator","unprotected","labels"};
    for(unsigned i=0;i<sizeof cases/sizeof *cases;i++){
        scenario=cases[i];launch(scenario,table_case);
        /* Each launch saves OUT; the next must exercise a new save. */
        forget_output();
    }
    /* Extend only after retaining the independent captures. */
    for(unsigned i=1;i<=32;i++){
        char id[4];snprintf(id,sizeof id,"%02u",i);scenario=id;
        launch(scenario,operation_case);forget_output();
    }
    printf("i123-protection: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
    return g_ctr[1]?1:0;
}
