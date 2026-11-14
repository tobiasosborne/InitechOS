/* One-menu-bar ruling in CLAUDE.md, Hazards that are still true.
 * Independent native geometry contract: public FLAIR chrome metrics. */
#define main i123_quit_oracle_main
#include "test_i123_quit.c"
#undef main
#include "chrome_metrics.h"
static void initial_frame(void)
{
    CHECK(tbx_host.nwindows==1,"FRAME","exactly one document window");
    CHECK(tbx_host.windows[0].t==FLAIR_CHROME_MENUBAR_H,"FRAME",
          "document structure-region top must meet the single menu bar");
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    m_dir=argv[1];
    g_ctr=mmap(0,2*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    if(g_ctr==MAP_FAILED)return 2;
    g_ctr[0]=g_ctr[1]=0;launch("initial document frame",initial_frame);
    printf("Initech 123 frame: %d checks, %d failures\n",g_ctr[0],g_ctr[1]);
    return g_ctr[1]?1:0;
}
