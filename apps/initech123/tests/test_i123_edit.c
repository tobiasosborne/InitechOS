/* R2.2 Reference pp. 1-22/1-23 and fixtures/edit/EDITKEYS.WK1.
 * Drive actual face keys; tests/run.sh independently reads the saved LABELs. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main

static void ek(int vk, int mods)
{
    tbx_event_t e = {0};
    e.what = keyDown; e.message = vk << 8; e.modifiers = mods;
    if (vk == 0x0f) e.message |= '\t';
    tbx_app_event(&e);
}
static void edit_cases(void)
{
    static const unsigned char keys[][5] = {
        {0x4b}, {0x4b,0x4b,0x4d}, {0x47}, {0x47,0x4f},
        {0x8f}, {0x47,0x0f}, {0xcb}, {0x47,0xcd},
        {0x4b,0x53}, {0x4b,0x0e}, {0x4b,0x4b,0x52},
        {0x47,0x52,0x52}, {0x4f,0x53}, {0x47,0x0e},
        {0x47,0x8f,0x4b}, {0x4f,0x0f,0x4d}
    };
    for (unsigned i=0; i<sizeof keys/sizeof keys[0]; ++i) {
        char addr[20];
        ek(0x3f,0); snprintf(addr,sizeof addr,"A%u",i+1); tks(addr); tk('\r');
        tks("ABCDEFGHIJ\r"); ek(0x3c,0);
        CHECK(g_epos==11 && !g_overwrite,"EDIT","F2 starts at end in insert mode");
        for (unsigned j=0; j<5 && keys[i][j]; ++j) {
            unsigned k=keys[i][j];
            ek(k==0x8f?0x0f:k==0xcb?0x4b:k==0xcd?0x4d:(int)k,
               k==0x8f?0x0200:(k==0xcb||k==0xcd)?0x1000:0);
        }
        if (i!=8 && i!=9) tk('X');
        CHECK(g_mode==M_EDIT && i123_cursor_col(&g_sheet)==0 && i123_cursor_row(&g_sheet)==i,
              "EDIT", "inline key must keep the sheet pointer and EDIT mode");
        tk('\r');
    }
    tks("/fsEDITKEYS\r");
    CHECK(file_exists("EDITKEYS.WK1"),"EDIT","save edited labels for independent readback");
}
static void edit_values(void)
{
    tks("123\r"); ek(0x3c,0); ek(0x4b,0);
    CHECK(g_epos==2,"EDIT","Left moves one character before numeric entry");
    tk('0'); tk('\r'); right();
    tks("+12+3\r"); ek(0x3c,0); ek(0x4b,0); tk('4'); tk('\r'); right();
    ek(0x3c,0); ek(0x47,0); ek(0x53,0); ek(0x0e,0); ek(0x0f,0);
    CHECK(g_epos==0 && g_elen==0,"EDIT","empty boundaries remain at zero");
    tks("Alpha\r"); right();
    tks("Old\r"); ek(0x3c,0); ek(0x4b,0); tk('X'); tk(27); tk(27);
    CHECK(g_mode==M_READY,"EDIT","Esc twice cancels the pending edit");
    right(); tks("123"); ek(0x3c,0); ek(0x47,0); ek(0x53,0); ek(0x3c,0);
    CHECK(g_mode==M_VALUE,"EDIT","F2 during typing then F2 returns to VALUE");
    tk('4'); tk('\r');
    tks("/fsNUMFORM\r");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    m_dir=argv[1];
    g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    g_ctr[0]=g_ctr[1]=0;
    launch("EDIT keys",edit_cases);
    launch("EDIT numeric/formula/cancel",edit_values);
    printf("Initech 123 EDIT: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
    return g_ctr[1]?1:0;
}
