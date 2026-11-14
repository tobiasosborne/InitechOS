/* Reference 5-15..16; input and saved WK1 bytes captured in real R2.2.
 * The independent reader compares every cell/format/name/settings record. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
static const char *which;static unsigned number;
static void import_case(void)
{
    char path[256],save[32];unsigned char input[32768];
    snprintf(path,sizeof path,"tests/fixtures/files/import/%s/INPUT.PRN",which);
    FILE *f=fopen(path,"rb");if(!f)_exit(2);size_t n=fread(input,1,sizeof input,f);if(ferror(f)||fclose(f))_exit(2);
    CHECK(tbx_host_put_file("INPUT.PRN",input,(uint32_t)n),"IMPORT","install independently authored raw input");
    tks("/dfA1..A20\r99\r0\r100\r/rff2\rA1..A20\r");
    if(!strcmp(which,"PROTECTED"))tks("/rpA2\r/wgpe");
    if(!strcmp(which,"PREFIX"))tks("/ruA1..A20\r/rpA2\r/wgpe");
    if(!strcmp(which,"RIGHT"))tks("/wglr");
    if(!strcmp(which,"EDGE")){ev(keyDown,0x3f00);tks("A8191\r");}
    snprintf(save,sizeof save,"/fsSEED%02u\r/fit",number);tks(save);
    CHECK(line_has(1,"Enter name of file to import:"),"IMPORT","Text opens the captured file selector");
    tks("INPUT.PRN\r");
    int full=!strcmp(which,"FULL"),prot=!strcmp(which,"PROTECTED")||!strcmp(which,"PREFIX");
    int longline=!strcmp(which,"LONG")||!strcmp(which,"AFTERLONG")||!strcmp(which,"TABLIMIT");
    if(full||prot||longline) {
        const char *error=full?"Memory full":prot?"Protected cell":"Input line too long (240 characters max)";
        CHECK(g_mode==M_NOTICE&&g_notice&&!strcmp(g_notice,error),"IMPORT","captured diagnostic (native capacity for full case)");
    } else CHECK(g_mode==M_READY,"IMPORT","completed import returns READY");
    if(!strcmp(which,"LONG")||!strcmp(which,"PROTECTED"))CHECK(!g_dirty,"IMPORT","error before first write leaves saved sheet clean");
    for(unsigned i=0;i<6;i++)tk(27);
    snprintf(save,sizeof save,"/fsI%02u\r",number);tks(save);
    snprintf(save,sizeof save,"I%02u.WK1",number);CHECK(file_exists(save),"IMPORT","saved result for independent WK1 comparison");
}
int main(int argc,char **argv)
{
    static const char *cases[]={"CR","LF","CTRLZ","MIX","LONG","PROTECTED","EDGE","RIGHT","FULL","PREFIX","AFTERLONG","MAX240","CONTROLS","TABLIMIT","OTHER"};
    if(argc!=2)return 2;
    m_dir=argv[1];g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);if(g_ctr==MAP_FAILED)return 2;
    for(number=0;number<sizeof cases/sizeof cases[0];number++){which=cases[number];launch(which,import_case);}
    printf("Initech 123 Import Text: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);return g_ctr[1]?1:0;
}
