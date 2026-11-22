/* MTE Finder Interface: identity includes volume and directory; the frame's
 * A root has its own remembered (46,115) view. Equal physical FAT clusters
 * on different volumes must never alias a spatial window or a write target.
 * This test also compiles against the former 16-bit interface to show red. */
#define main single_volume_reference_main
#include "test_finder_windows.c"
#undef main
static uint32_t wrote_parent;
static unsigned wrote_calls;
static int volume_enumerate(void *user,uint32_t dir,finder_enum_cb cb,void *cookie)
{
    (void)user;
    finder_dirent_t e={0};
    /* Untagged zero is the old interface's current C root. */
    if(dir==0 || dir==0x00030000u){e.name83="CFILE.TXT";e.start_cluster=2;}
    else if(dir==0x00010000u){e.name83="AFILE.TXT";e.start_cluster=2;}
    else return -1;
    e.attribute=0x20;e.size=1;
    return cb(&e,cookie);
}
int volume_enumerate16(void *u,uint16_t d,finder_enum_cb c,void *v){return volume_enumerate(u,d,c,v);}
int volume_enumerate32(void *u,uint32_t d,finder_enum_cb c,void *v){return volume_enumerate(u,d,c,v);}
static int volume_mkdir(void *user,const char *name,uint32_t parent)
{
    (void)user;(void)name;wrote_calls++;wrote_parent=parent;return FINDER_WIN_OK;
}
int volume_mkdir16(void *u,const char *n,uint16_t d){return volume_mkdir(u,n,d);}
int volume_mkdir32(void *u,const char *n,uint32_t d){return volume_mkdir(u,n,d);}
int main(void)
{
    static scene_t s;
    scene_init(&s,NULL,0);s.sh.volume_drive=2;
    s.fs.enumerate=_Generic(s.fs.enumerate,
        int(*)(void*,uint16_t,finder_enum_cb,void*):volume_enumerate16,
        int(*)(void*,uint32_t,finder_enum_cb,void*):volume_enumerate32);
    s.fs.mkdir=_Generic(s.fs.mkdir,
        int(*)(void*,const char*,uint16_t):volume_mkdir16,
        int(*)(void*,const char*,uint32_t):volume_mkdir32);
    finder_shell_bind_fs(&s.sh,&s.fs);
    uint32_t croot=0x00030000u,aroot=0x00010000u;
    (void)finder_shell_note_view(&s.sh,croot,"",70,90,0);
    (void)finder_shell_note_view(&s.sh,aroot,"",46,115,0);
    CHECK(s.sh.n_views==2,"C and A root view records are independent");
    int c=-1,a=-1,singleton=-1;
    CHECK(finder_win_open(&s.sh,croot,croot,"",1,&c,&singleton)==0 && !singleton,"open C root");
    CHECK(finder_win_open(&s.sh,aroot,aroot,"",1,&a,&singleton)==0 && !singleton,"open independent A root");
    CHECK(a>=0 && c>=0 && a!=c,"equal physical root clusters have distinct windows");
    if(a>=0 && c>=0){
        finder_window_t *cw=&s.sh.windows[c],*aw=&s.sh.windows[a];
        CHECK(cw->dir_start==croot && aw->dir_start==aroot,"windows retain complete volume-directory identities");
        CHECK(!strcmp(cw->rec.titleHandle,"Drive C Files") && !strcmp(aw->rec.titleHandle,"Drive A Files"),"each root names its own volume");
        rgn_rect_t cf=WindowFrameRect(&cw->rec),af=WindowFrameRect(&aw->rec);
        CHECK(cf.left==70 && cf.top==90 && af.left==46 && af.top==115,"A frame seed never becomes C view state");
        CHECK(cw->view.n==1 && !strcmp(cw->view.icons[0].name,"CFILE.TXT"),"C enumeration remains C");
        CHECK(aw->view.n==1 && !strcmp(aw->view.icons[0].name,"AFILE.TXT"),"A enumeration is independent of C");
        char name[FINDER_DESK_NAME_MAX];uint32_t parent=0;
        /* The output-parent API widens with directory identity too. Use a
         * correctly aligned full object even when the former API writes16. */
        CHECK(finder_win_new_folder(&s.sh,a,name,(void *)&parent)==0,"A window can create a folder");
        CHECK(wrote_calls==1 && wrote_parent==aroot && parent==aroot,"A writes carry A identity and never target C");
    }
    s.sh.n_views=0;
    for(unsigned i=0;i<8;i++)
        CHECK(finder_shell_note_view(&s.sh,croot+i,"",70,90,0)==1,"C keeps its eight view records");
    CHECK(finder_shell_note_view(&s.sh,aroot,"",46,115,0)==1 && s.sh.n_views==9,
          "C's full view cache does not consume A's capacity");
    CHECK(finder_shell_note_view(&s.sh,croot+9,"",1,1,0)==0,
          "each individual volume retains the existing eight-view limit");
    return TEST_SUMMARY("test-finder-volumes");
}
