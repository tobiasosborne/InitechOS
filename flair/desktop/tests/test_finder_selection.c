/* initech-nylrt: serial acknowledges the actual selected name, including a
 * loaded-host prefix reset. Literal transport contract requested by factory;
 * selection itself follows existing Event Manager type-ahead semantics. */
#define main windows_main
#include "test_finder_windows.c"
#undef main
static unsigned messages;
static char last[128];
static void selected(void *user,const char *line)
{ (void)user;messages++;snprintf(last,sizeof last,"%s",line); }
static void key(FlairApp *app,unsigned ch,unsigned tick,unsigned modifiers)
{
 EventRecord ev={0};ev.what=keyDown;ev.message=ch;ev.when=tick;ev.modifiers=modifiers;
 finder_shell_procs.event(app,&ev);
}
int main(void)
{
 static scene_t s;FinderCtx ctx={0};FlairApp app={0};int slot=-1;
 scene_init(&s,MOCK_DIRS,2);ctx.trace=selected;finder_shell_bind_ctx(&s.sh,&ctx);
 CHECK(finder_win_open(&s.sh,0,0,"",1,&slot,NULL)==0,"selection root opens");
 app.userData=&s.sh;s.sh.app=&app;
 key(&app,'R',100,0);
 CHECK(messages==1&&!strcmp(last,"FINDER-WIN-SELECT win=0 name=README.TXT count=1"),"selection marker names real prefix result");
 key(&app,'E',101,0);
 CHECK(messages==2&&!strcmp(last,"FINDER-WIN-SELECT win=0 name=README.TXT count=1"),"unchanged selected result still acknowledges delivered prefix");
 key(&app,'Z',102,0);CHECK(messages==2,"unmatched prefix has no false acknowledgement");
 key(&app,'R',300,0);CHECK(messages==3,"expired prefix acknowledges new real selection");
 key(&app,'R',301,FLAIR_EVT_MOD_CONTROL_KEY);CHECK(messages==3,"command chord is not a type-ahead selection");
 return TEST_SUMMARY("test-finder-selection");
}
