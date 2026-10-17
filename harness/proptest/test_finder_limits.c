/* tdnl.61/.63/.84/.64. Explicit listings, actions and expected names.
 * Ref: issue descriptions; PRD 6.5; IM Finder keyboard selection semantics. */
#define main legacy_windows_main
#include "test_finder_windows.c"
#undef main

int main(void)
{
    static scene_t s;
    static mock_entry_t many[70];
    static char names[70][13];
    mock_dir_t dir={0,many,69};
    int slot=-1;
    FinderCtx ctx={0};
    FlairApp app={0};
    EventRecord ev={0};
    for (int i=0;i<70;i++) {
        snprintf(names[i],13,"F%02d",i);
        many[i]=(mock_entry_t){names[i],0x10,0,(uint16_t)(100+i)};
    }
    scene_init(&s,&dir,1);
    CHECK(finder_win_open(&s.sh,0,0,"",1,&slot,0)==FINDER_WIN_OK,
          "ICONS_HEAP opens 69-entry root");
    if(!s.sh.windows[0].open) return TEST_SUMMARY("test-finder-limits");
    CHECK(s.sh.windows[0].view.n==69 && s.sh.windows[0].dropped==0,
          "ICONS_HEAP all 69 entries reachable");
    for(int i=0;i<5;i++)
        CHECK(finder_win_open(&s.sh,0,(uint16_t)(100+i),names[i],0,&slot,0)==FINDER_WIN_OK,
              "WINDOWS_HEAP fifth folder opens");
    (void)finder_win_open(&s.sh,0,0,"",1,&slot,0);
    finder_shell_bind_ctx(&s.sh,&ctx);
    s.sh.windows[0].view.icons[5].selected=1;
    s.sh.windows[0].view.icons[6].selected=1;
    finder_shell_sync_ctx(&s.sh);
    CHECK(ctx.exec(ctx.shell,FCMD_OPEN),"OPEN_ALL dispatch accepted");
    CHECK(finder_win_find(&s.sh,105)>=0 && finder_win_find(&s.sh,106)>=0,
          "OPEN_ALL opens both selected folders");
    (void)finder_win_open(&s.sh,0,0,"",1,&slot,0);
    app.userData=&s.sh;
    ev.what=keyDown; ev.message=0x4d00; ev.when=1;
    finder_shell_procs.event(&app,&ev);
    CHECK(s.sh.windows[0].view.icons[7].selected,
          "KEY_SELECT right arrow advances selection");
    ev.message='F'; ev.when=10; finder_shell_procs.event(&app,&ev);
    ev.message='6'; ev.when=11; finder_shell_procs.event(&app,&ev);
    ev.message='8'; ev.when=12; finder_shell_procs.event(&app,&ev);
    CHECK(s.sh.windows[0].view.icons[68].selected && finder_desk_selection_count(&s.sh.windows[0].view)==1,
          "KEY_SELECT prefix F68 selects final entry");
    ev.message=0x4d00;ev.when=20;finder_shell_procs.event(&app,&ev);
    ev.message='F';ev.when=200;finder_shell_procs.event(&app,&ev);
    ev.message='6';ev.when=270;finder_shell_procs.event(&app,&ev);
    CHECK(s.sh.windows[0].view.icons[60].selected,
          "KEY_SELECT 70 PIT ticks remain inside the one-second prefix interval");
    CHECK(flair_alloc(&s.icons,FLAIR_CLASS_GENERAL,flair_heap_avail(&s.icons)-32u)!=0,
          "ICONS_HEAP exhaust the real icon arena");
    dir.n=70;
    CHECK(finder_win_populate(&s.sh,0)==FINDER_WIN_ERR_MEMORY &&
          s.sh.windows[0].view.n==69 && !strcmp(s.sh.windows[0].view.icons[68].name,"F68"),
          "ICONS_HEAP real allocation refusal preserves committed listing");
    return TEST_SUMMARY("test-finder-limits");
}
