/* Reference 12-33..35; real R2.2 raw VGA and byte-identical WK1 captures. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
static const char *scenario;
static void screen(const char *path)
{
    unsigned char vga[4000];char full[512];
    snprintf(full,sizeof full,"tests/fixtures/zero/%s",path);
    FILE *f=fopen(full,"rb");if(!f||fread(vga,1,sizeof vga,f)!=sizeof vga)_exit(2);fclose(f);
    for(unsigned r=0;r<11;r++) {
        char wanted[73];for(unsigned x=0;x<72;x++)wanted[x]=(char)vga[2*((r+4)*80+x+4)];wanted[72]=0;
        if(memcmp(wanted,m_scr[r+4]+4,72))fprintf(stderr,"zero %s row %u: wanted [%s], got [%.*s]\n",path,r, wanted,72,m_scr[r+4]+4);
        CHECK(!memcmp(wanted,m_scr[r+4]+4,72),"ZERO","whole row agrees with captured R2.2 glyphs");
    }
}
static void zero_case(void)
{
    char path[128],input[128];
    tks("/wgc12\r");
    static const char *values[]={"0","+A1","'0","0.05","0","+A1","0","0","0","@NA","@ERR"};
    for(unsigned r=0;r<11;r++){ev(keyDown,0x3f00);snprintf(input,sizeof input,"A%u\r%s\r",r+1,values[r]);tks(input);}
    tks("/rff0\rA4\r/rfhA5\r/rftA6..A7\r/rfdt1A8\r/rfd1A9\r");
    snprintf(input,sizeof input,"/fs%sB\r",scenario);tks(input);
    snprintf(path,sizeof path,"%s/before.vga",scenario);screen(path);
    tks("/wgz");CHECK(line_has(1,"No  Yes  Label")&&line_has(2,"Display zero values"),"ZERO","all three choices, No selected by default");
    if(!strcmp(scenario,"yes"))tk('y');
    else {
        tk('l');CHECK(line_has(0,"EDIT")&&line_has(1,"Enter label (can include label prefix): _"),"ZERO","initial label is empty EDIT");
        const char *text=!strcmp(scenario,"left")?"'NIL":!strcmp(scenario,"right")?"\"NIL":!strcmp(scenario,"center")?"^NIL":!strcmp(scenario,"repeat")?"\\ab":!strcmp(scenario,"plain")?"NIL":"";
        tks(text);tk('\r');
    }
    CHECK(line_has(0,"READY")&&!g_dirty,"ZERO","display choice leaves saved worksheet clean");
    snprintf(path,sizeof path,"%s/result.vga",scenario);screen(path);
    snprintf(input,sizeof input,"/fs%sA\r",scenario);tks(input);
    tks("/wgzn");snprintf(path,sizeof path,"%s/no_again.vga",scenario);screen(path);
    tks("/wgzy/wgz\r");screen(path); /* menu always starts on No, even after Yes */
    tks("/wgzy/wgzl");CHECK(line_has(1,"Enter label (can include label prefix): \"_"),"ZERO","Yes supplies a right-label prefix");
    tks("NIL\r/wgzl");tk(27);CHECK(line_has(0,"EDIT"),"ZERO","first Esc clears nonempty label field");tk(27);
    CHECK(line_has(1,"No  Yes  Label")&&line_has(2,"Replace zeros with a specified label"),"ZERO","second Esc returns to Label choice");
    for(unsigned i=0;i<4;i++)tk(27);
    ev(keyDown,0x4300);screen("right/result.vga");
    snprintf(input,sizeof input,"/fr%sA\r",scenario);tks(input);screen(path);
    tks("/wgzy/wey0\r/wgzl");CHECK(line_has(1,"Enter label (can include label prefix): _"),"ZERO","Worksheet Erase resets display state");
}
static void long_case(void)
{
    tks("/frLEFTB\r/wgzl'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOP\r");
    screen("long-steady/result.vga");
    tks("/wgzn/wgzl");for(unsigned i=0;i<240;i++)tk('a');tk('\r');tks("/wgzl");
    for(unsigned i=0;i<239;i++)tk(8);
    CHECK(line_has(1,"Enter label (can include label prefix): \"_"),"ZERO","unprefixed maximum reopens as prefix plus 239 characters");
    tk(8);CHECK(line_has(1,"Enter label (can include label prefix): _"),"ZERO","240th backspace empties maximum field");
    for(unsigned i=0;i<5;i++)tk(27);
    CHECK(!g_dirty,"ZERO","long-label editing does not dirty sheet");
}
static void protected_case(void)
{
    tks("/frLEFTB\r/rpA1..A11\r/wgpe");ev(keyDown,0x3f00);tks("A1\r/wgzy");
    screen("protected/protected_zero.vga");
    CHECK(line_has(0,"A1: 0"),"ZERO","protected cell control panel keeps its numeric value");
    tks("/fsPROTECT\r");
}
int main(int argc,char **argv)
{
    static const char *cases[]={"yes","left","right","center","repeat","plain","empty"};
    if(argc!=2)return 2;
    m_dir=argv[1];g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);if(g_ctr==MAP_FAILED)return 2;
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){scenario=cases[i];launch(scenario,zero_case);}launch("long label",long_case);
    launch("protected zeros",protected_case);
    printf("Initech 123 Zero: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);return g_ctr[1]?1:0;
}
