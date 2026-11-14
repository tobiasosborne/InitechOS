/* R2.2 Reference 4-5/4-6 and independent fixtures/fill captures.
 * Tests run actual keys and saved files are read outside the product codec. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
struct fill_case {const char *file,*start,*step,*stop;int special;};
static const struct fill_case *which;
static void fill_case(void)
{
    char input[80];
    for(unsigned col=0;col<2;col++)for(unsigned row=1;row<=3;row++) {
        ev(keyDown,0x3f00);snprintf(input,sizeof input,"%c%u\r'KEEP-%c%u\r",'A'+col,row,'A'+col,row);tks(input);
    }
    if(which->special==1)tks("/rff2\rA1..B3\r");
    if(which->special==2)tks("/ruA1..B3\r/rpB2\r/wgpe");
    if(which->special==3){ev(keyDown,0x3f00);tks("D1\r10\r/rncBASE\rD1\r");}
    tks("/df");
    CHECK(g_mode==M_RANGE && line_has(1,"Enter fill range:"),"FILL","Fill offers its real range prompt");
    CHECK(line_has(1,which->special==3?"Enter fill range: D1_":"Enter fill range: B3_"),"FILL","default is the selected single cell");
    tks("A1..B3\r");
    CHECK(g_mode==M_RANGE && line_has(2,"Start: 0"),"FILL","fresh default Start is zero");
    tks(which->start);tk('\r');tks(which->step);tk('\r');tks(which->stop);tk('\r');
    CHECK(which->special==2 ? (g_mode==M_NOTICE&&line_has(0,"ERROR")) : g_mode==M_READY,
          "FILL","operation mode agrees with captured R2.2");
    if(which->special==2)CHECK(g_error&&!strcmp(g_error,"Protected cell"),"FILL","captured protection diagnostic");
    for(int i=0;i<5;i++)tk(27);
    snprintf(input,sizeof input,"/fs%s\r",which->file);tks(input);
    CHECK(file_exists(which->file),"FILL","completed/error worksheet is saved for independent readback");
}
static void defaults(void)
{
    ev(keyDown,0x3f00);tks("D1\r10\r/rncBASE\rD1\r/dfA1..B3\rBASE\r2+3\rBASE+20\r");
    ev(keyDown,0x3f00);tks("D1\r20\r/dfA10..B12\r");
    CHECK(line_has(2,"Start: 10"),"FILL","default stores evaluated Start, not BASE");tk('\r');
    CHECK(line_has(2,"Step: 5"),"FILL","default stores evaluated Step");tk('\r');
    CHECK(line_has(2,"Stop: 30"),"FILL","default stores evaluated Stop");tk('\r');
    tks("/fsREUSE\r/frREUSE\r/dfA20..B22\r");
    CHECK(line_has(2,"Start: 0"),"FILL","Retrieve resets Start");tks("99\r");
    CHECK(line_has(2,"Step: 1"),"FILL","Retrieve resets Step");tk('\r');
    CHECK(line_has(2,"Stop: 8191"),"FILL","Retrieve resets Stop");
    for(int i=0;i<5;i++)tk(27);
    CHECK(!g_dirty,"FILL","canceling scalar input does not dirty a saved worksheet");
    tks("/dfA30..B32\r");CHECK(line_has(2,"Start: 99"),"FILL","accepted Start remains a default after Cancel");
    for(int i=0;i<5;i++)tk(27);
    tks("/fsCANCEL\r/wey/dfA1\r");
    CHECK(line_has(2,"Start: 0"),"FILL","Worksheet Erase resets Start");tk('\r');
    CHECK(line_has(2,"Step: 1"),"FILL","Worksheet Erase resets Step");tk('\r');
    CHECK(line_has(2,"Stop: 8191"),"FILL","Worksheet Erase resets Stop");
}
static void memory(void)
{
    ev(keyDown,0x3f00);tks("B1\rKEEP\r/dfA1..IV8192\r\r\r1e99\r");
    CHECK(g_mode==M_NOTICE&&g_error&&!strcmp(g_error,"Memory full"),"FILL","captured memory diagnostic");
    CHECK(g_sheet.ncells==2048,"FILL","native capacity publishes the filled prefix, not the old sheet");
    for(int i=0;i<5;i++)tk(27);
    tks("/fsMEMORY\r");
}
static void invalid(void)
{
    tks("/dfA1\rBOGUS\r");
    CHECK(g_mode==M_NOTICE&&g_error&&!strcmp(g_error,"Invalid number input"),"FILL","captured scalar diagnostic");
    tk(27);
    CHECK(g_mode==M_READY&&!g_dirty&&!g_sheet.ncells,"FILL","Esc after invalid scalar returns READY without writing");
}
int main(int argc,char **argv)
{
    static const struct fill_case cases[]={
        {"BASIC.WK1","10","2","16",0}, {"FRAC.WK1","0.5","0.25","1.25",0},
        {"DESC.WK1","10","-2","4",0}, {"FMT.WK1","10","2","16",1},
        {"PROT.WK1","1","1","6",2}, {"EXPR.WK1","BASE","2+3","BASE+20",3},
        {"ZERO.WK1","10","0","20",0}, {"BADDIR.WK1","10","1","0",0},
        {"DECIMAL.WK1","0.1","0.1","0.3",0}
    };
    if(argc!=2)return 2;
    m_dir=argv[1];g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    g_ctr[0]=g_ctr[1]=0;
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){which=&cases[i];launch(which->file,fill_case);}
    launch("reused defaults and Cancel",defaults);
    launch("memory prefix",memory);
    launch("invalid scalar",invalid);
    printf("Initech 123 Fill: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);return g_ctr[1]?1:0;
}
