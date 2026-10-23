/* Host UI integration over the recording Toolbox mock. Expectations are
 * manual/RI facts, not menu_data. initech-uqah. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
static void ranges(void)
{
    char content[512];i123_val_t v;i123_rect_t r;
    i123_format_number(-0.024,0x70,9,content);CHECK(!strcmp(content,".        "),"R0","manual +/- small value");
    i123_format_number(5.7,0x70,9,content);CHECK(!strcmp(content,"++++++   "),"R0","manual +/- rounding");
    i123_format_number(10,0x70,9,content);CHECK(!strcmp(content,"*********"),"R0","manual +/- overflow");
    i123_format_number(9.999,0x12,12,content);CHECK(!strcmp(content,"   1.00E+01 "),"R0","Sci rounding carries exponent");
    tks("10");right();tks("-a1*2");right();tks("-b1*2\r");
    tks("/cb1..c1\rd1\r");
    CHECK(g_mode==M_READY&&g_dirty,"R1","Copy returns READY and marks unsaved work");
    cell(3,0,content,sizeof content);CHECK(!strcmp(content,"-C1*2"),"R1","Copy relative formula");
    tks("/md1..e1\rf1\r");
    cell(6,0,content,sizeof content);CHECK(!strcmp(content,"-F1*2"),"R2","Move moved formula keeps precedents");
    CHECK(!i123_find(&g_sheet,3,0),"R2","Move erased source");
    tks("/rnctotal\ra1..c1\r");
    CHECK(!i123_name_find(&g_sheet,"TOTAL",5,&r)&&r.c1==2,"R3","Name Create");
    ev(keyDown,0x3f<<8);tks("b2\r");tks("@sum(total)\r");
    i123_cell_value(&g_sheet,1,1,&v);CHECK(v.t==I123_V_NUM&&v.n==30,"R3","Named formula resolves");
    tks("/rff2\ra1\r");CHECK(i123_find(&g_sheet,0,0)->fmt==0x82,"R4","Fixed2 keeps protection flag");
    tks("/rlc");tks("a3\r"); /* no label: permitted no-op */
    tks("/rpa1\r/wgpe");
    ev(keyDown,0x3f<<8);tks("a1\r99\r");
    CHECK(g_mode==M_NOTICE,"R5","Protected typed entry visibly refused");tk(27);
    i123_cell_value(&g_sheet,0,0,&v);CHECK(v.n==10,"R5","Protected cell unchanged");
    tks("/rua1\r99\r");i123_cell_value(&g_sheet,0,0,&v);CHECK(v.n==99,"R5","Unprot permits entry");
    /* initech-3s12 review: under /WGP Enable the absent A4..C6 are protected
     * (default 0xFF, bit 7) and Value/Trans into them is refused; R6 grades
     * Value/Trans, so protection is disabled first. */
    tks("/rva1..c1\ra4\r");CHECK(g_mode==M_NOTICE&&!i123_find(&g_sheet,1,3),"R5","Value into protected absent cells refused");tk(27);
    tks("/wgpd");CHECK(!g_sheet.protec,"R5","Global protection disabled");
    tks("/rva1..c1\ra4\r");CHECK(i123_find(&g_sheet,1,3)->kind==I123_K_NUMBER,"R6","Value replaces formulas");
    tks("/rta1..c1\ra6\r");CHECK(i123_find(&g_sheet,0,6)->kind==I123_K_NUMBER,"R6","Trans converts formulas and exchanges axes");
    tks("/rnta10\r");cell(0,9,content,sizeof content);CHECK(!strcmp(content,"'TOTAL"),"R7","Name Table");
    tks("/rndtotal\r");CHECK(i123_name_find(&g_sheet,"TOTAL",5,&r)!=0,"R7","Name Delete");
    tks("/rea1\r");
    CHECK(i123_find(&g_sheet,0,0)->kind==I123_K_BLANK&&i123_find(&g_sheet,0,0)->fmt==2,"R7","Erase keeps format/protection (manual PDF 221)");
    tks("/wgr");CHECK(line_has(1,"Natural  Columnwise  Rowwise"),"R8","Recalc tree real items");
    CHECK(line_has(2,"Recalculate in natural order"),"R8","Real highlighted description");
    tk(27);CHECK(line_has(1,"Format  Label-Prefix"),"R8","Esc returns one level");tk(3);
    CHECK(line_has(0,"READY"),"R8","Ctrl-Break returns READY");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    m_dir=argv[1];
    g_ctr=mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    launch("range commands",ranges);
    printf("i123-range-menu: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);return g_ctr[1]?1:0;
}
