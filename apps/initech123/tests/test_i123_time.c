/* Real R2.2 glyphs are read directly from captured VGA character bytes;
 * saved format/value records are checked by an independent WK1 reader. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
static void captured_cells(const char *name,unsigned rows,unsigned cols,const unsigned *width)
{
    char path[512];unsigned char vga[4000];
    snprintf(path,sizeof path,"tests/fixtures/time-formats/%s.vga",name);
    FILE *f=fopen(path,"rb");if(!f||fread(vga,1,sizeof vga,f)!=sizeof vga)_exit(2);fclose(f);
    for(unsigned row=0;row<rows;row++)for(unsigned col=0,offset=4;col<cols;offset+=width[col++]) {
        char actual[241],expected[241];uint32_t spill=0;
        for(unsigned i=0;i<width[col];i++)expected[i]=(char)vga[2*((4+row)*80+offset+i)];
        expected[width[col]]=0;
        i123_cell_text(&g_sheet,i123_find(&g_sheet,(uint16_t)col,(uint16_t)row),width[col],actual,&spill);
        if(strcmp(actual,expected))fprintf(stderr,"time %s %u/%u: wanted [%s], got [%s]\n",name,col,row,expected,actual);
        CHECK(!strcmp(actual,expected),"TIME","cell glyphs/alignment agree with real R2.2 VGA");
    }
}
static void formats(void)
{
    static const char *values[]={"0","0.5","0.999999","1.5","-0.25","0.00000578","0.50000578","0.99999999","0.99965"};
    static const unsigned wide[]={16,16,16},narrow[]={10,16,16};
    char input[64];
    tks("/wgc16\r");
    for(unsigned row=0;row<9;row++)for(unsigned col=0;col<2;col++) {
        ev(keyDown,0x3f00);snprintf(input,sizeof input,"%c%u\r%s\r",'A'+col,row+1,values[row]);tks(input);
    }
    tks("/rfdt1A1..A9\r/rfdt2B1..B9\r");
    captured_cells("formats",9,2,wide);
    tks("/fsTIMEREF\r");
    ev(keyDown,0x3f00);tks("C1\r+0.5\r");ev(keyDown,0x3f00);tks("C2\r0.50069444\r");
    tks("/wgfdt1");captured_cells("global/global_seconds",2,3,wide);tks("/fsGLOBAL1\r");
    tks("/wgfdt2/fsGLOBAL2\r");ev(keyDown,0x4700);tks("/wcs10\r");
    captured_cells("global/narrow_seconds",9,3,narrow);tks("/fsNARROW\r");
}
static void more(void)
{
    static const char *values[]={"0.0416667","0.0625","73050.75","1e99","0.00000579","0.50034722222"};
    static const unsigned width[]={16,16};char input[64];
    tks("/wgc16\r");
    for(unsigned row=0;row<6;row++)for(unsigned col=0;col<2;col++) {
        ev(keyDown,0x3f00);snprintf(input,sizeof input,"%c%u\r%s\r",'A'+col,row+1,values[row]);tks(input);
    }
    tks("/rfdt1A1..A6\r/rfdt2B1..B6\r");captured_cells("more/more",6,2,width);
    tks("/fsMOREREF\r");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    m_dir=argv[1];g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    g_ctr[0]=g_ctr[1]=0;launch("fixed AM/PM formats",formats);
    launch("hours, large days and second rounding",more);
    printf("Initech 123 time formats: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
    return g_ctr[1]?1:0;
}
