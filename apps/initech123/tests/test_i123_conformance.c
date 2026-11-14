/* Assertions independent of OBSERVED.txt: literals transcribed from retained
 * real R2.2 VGA captures. No new reference run is part of tier 0. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
static const char *name, *message;
static int accepted;
static void load_case(void)
{
    tks("KEEP\r");
    unsigned bank=g_bank; char before[32]; cell(0,0,before,sizeof before);
    char cmd[64];snprintf(cmd,sizeof cmd,"/fr%s\r",name);tks(cmd);
    CHECK((g_mode!=M_NOTICE)==accepted,"LOAD","acceptance agrees with real R2.2");
    if(!accepted){
        CHECK(g_mode==M_NOTICE && g_notice && !strcmp(g_notice,message),"LOAD","exact real R2.2 diagnostic");
        char after[32];cell(0,0,after,sizeof after);
        CHECK(g_bank==bank && g_dirty && !strcmp(before,after),"LOAD","refused load preserves current worksheet and dirty state");
    }
}
static void edit240(void)
{
    tks("/frL10\r");
    ev(keyDown,0x3c00);tk('\r');tks("/fsL10F2\r");
    CHECK(file_exists("L10F2.WK1"),"L10","save exact no-op EDIT fixture for independent real-Lotus comparison");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    m_dir=argv[1];g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    g_ctr[0]=g_ctr[1]=0;
    name="L05";message="Part of file is missing";accepted=0;launch(name,load_case);
    launch("L10 F2/Return",edit240);
    printf("Initech 123 conformance: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
    return g_ctr[1]?1:0;
}
