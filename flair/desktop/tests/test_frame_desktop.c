/* Frame geometry.tsv DRIVEA: (46,115), 640-column still. The IDB1 bytes
 * are authored from the published record layout, never from its encoder.
 * The approved frame judgement requires ordinary persisted state. */
#define main windows_suite_main
#include "test_finder_windows.c"
#undef main
int main(int argc, char **argv)
{
    static const uint8_t expected[32]={
        'I','D','B','1',1,0,1,0,
        4,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0, 46,0,115,0,0,0,0
    };
    uint8_t bytes[33], saved[4096];
    static scene_t s;
    int slot=-1;
    CHECK(argc==2,"frame fixture path supplied");
    if(argc!=2) return TEST_SUMMARY("test-frame-desktop");
    FILE *f=fopen(argv[1],"rb");
    CHECK(f!=NULL,"authored Desktop DB seed opens");
    if(!f) return TEST_SUMMARY("test-frame-desktop");
    size_t n=fread(bytes,1,sizeof bytes,f); fclose(f);
    CHECK(n==32 && !memcmp(bytes,expected,32),"seed is literal root-view record at frame coordinates");
    scene_init(&s,MOCK_DIRS,2);
    CHECK(!s.sh.windows[0].open,"loading frame preference does not open a scene");
    CHECK(finder_shell_load_views(&s.sh,bytes,(uint32_t)n)==1,"ordinary view loader accepts frame seed");
    CHECK(finder_win_open(&s.sh,0,0,"",1,&slot,NULL)==FINDER_WIN_OK,"ordinary volume Open succeeds");
    rgn_rect_t frame=WindowFrameRect(&s.sh.windows[slot].rec);
    CHECK(frame.left==46 && frame.top==115,"Drive A opens at independently transcribed still origin");
    CHECK(frame.right-frame.left==FINDER_WIN_DEFAULT_W && frame.bottom-frame.top==FINDER_WIN_DEFAULT_H,
          "version-one record retains documented default size");
    MoveWindow(&s.wm,&s.sh.windows[slot].rec,81,137);
    CHECK(finder_win_close(&s.sh,slot)==FINDER_WIN_OK,"ordinary Close saves changed position");
    n=finder_desk_db_encode_all(&s.sh.desk,s.sh.views,s.sh.n_views,saved,sizeof saved);
    CHECK(n>0,"normal database encoding includes changed window preference");
    scene_init(&s,MOCK_DIRS,2);
    CHECK(finder_shell_load_views(&s.sh,saved,(uint32_t)n)==1,"next session loads saved preference");
    CHECK(finder_win_open(&s.sh,0,0,"",1,&slot,NULL)==FINDER_WIN_OK,"next session opens root normally");
    frame=WindowFrameRect(&s.sh.windows[slot].rec);
    CHECK(frame.left==81 && frame.top==137,"user arrangement supersedes shipped frame preference");
    return TEST_SUMMARY("test-frame-desktop");
}
