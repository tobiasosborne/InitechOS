/* R2.2 width-range reference captures, independent saved-record postcheck. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
static void widths(void)
{
    tks("/wcs10\r");ev(keyDown,0x3f00);tks("D1\r/wcs14\r");
    tks("/wccsB1..C4\r");
    CHECK(g_mode==M_RANGE && line_has(1,"Select a width for range of columns (1..240):"),"WIDTH","range precedes width prompt");
    tks("12\r/fsRANGESET\r");
    tks("/wccrB1..C4\r/wgc11\r/fsRANGERST\r");
    tks("/wccsIV1\r240\r/fsRANGEIV\r");
    tks("/wccsB1..C1\r0\r");
    CHECK(g_mode==M_NOTICE && g_error && !strcmp(g_error,"Invalid number input"),"WIDTH","zero width refuses with real diagnostic");
    tk(27);
    CHECK(g_mode==M_RANGE && !g_rlen,"WIDTH","Esc from invalid width restores the default prompt");
    tk(27);tk(27);tk(27);tk(3);
    tks("/wccsB1..C1\r15");tk(27);tk(27);tk(27);tk(3);
    CHECK(g_mode==M_READY&&!g_dirty,"WIDTH","Escape cancels width input; Ctrl-Break exits the remaining menus");
    tks("/fsRANGECAN\r");
}
static void boundaries(void)
{
    tks("KEEP\r/fsKEEPBEF\r/wccsA1..B1\r241\r");
    CHECK(g_mode==M_NOTICE && g_error && !strcmp(g_error,"Invalid number input"),"WIDTH","241 refuses as captured in the isolated reference run");
    tk(27);tks("1.5\r");
    CHECK(g_mode==M_NOTICE && g_error && !strcmp(g_error,"Invalid number input"),"WIDTH","fractional width refuses");
    tk(27);tk(27);tk(27);tk(27);
    tks("/wccsA0..B1\r");
    CHECK(g_mode==M_NOTICE,"WIDTH","invalid range refuses before changing widths");
    tk(27);tks("/wccsA1..B1\r1\r/fsKEEPAFT\r");
    CHECK(i123_colwidth(&g_sheet,0)==1 && i123_colwidth(&g_sheet,1)==1,"WIDTH","manual's minimum width applies inclusively");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    m_dir=argv[1];
    g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    g_ctr[0]=g_ctr[1]=0;launch("column range widths",widths);
    launch("width boundaries preserve cells",boundaries);
    printf("Initech 123 width range: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
    return g_ctr[1]?1:0;
}
