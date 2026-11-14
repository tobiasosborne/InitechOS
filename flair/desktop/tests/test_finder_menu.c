/* Shipping manifest plus independent predicate truth table and both Menu
 * Manager input routes. The original permanently gray rows are approved GM4
 * omissions; the complete manifest lives test-side in test_finder_commands.c. */
#define FINDER_COMMANDS_NO_MAIN
#include "test_finder_commands.c"
static int allowed(finder_cmd_id id,const FinderCtx *f)
{
 switch(id){
 case FCMD_NEW_FOLDER:return !f->front_is_trash;
 case FCMD_OPEN:return f->selection_openable;
 case FCMD_CLOSE_WINDOW:case FCMD_CLEANUP:case FCMD_ARRANGE_BY_NAME:
 case FCMD_VIEW_ICONS:case FCMD_VIEW_SMALL:case FCMD_VIEW_NAME:
 case FCMD_VIEW_SIZE:case FCMD_VIEW_KIND:case FCMD_VIEW_DATE:return f->front_is_diskwin;
 case FCMD_GET_INFO:return f->selection_infoable;
 case FCMD_PUT_AWAY:return f->selection_putaway;
 case FCMD_EMPTY_TRASH:return f->trash_nonempty;
 default:return 1;
 }
}
int main(void)
{
 manifest();MenuBar *bar=finder_menu_bar();
 for(unsigned state=0;state<64;state++){
  FinderCtx f;memset(&f,0,sizeof f);
  f.front_is_trash=!!(state&1);f.selection_openable=!!(state&2);
  f.front_is_diskwin=!!(state&4);f.selection_infoable=!!(state&8);
  f.selection_putaway=!!(state&16);f.trash_nonempty=!!(state&32);
  finder_menu_refresh_enables(&f);
  for(unsigned m=0;m<bar->n_menus;m++)for(unsigned i=0;i<bar->menus[m].n_items;i++){
   const MenuItem *it=&bar->menus[m].items[i];
   if(it->is_divider){CHECK(!MenuInfo_item_selectable(bar,m,i),"divider has no mouse route");continue;}
   const finder_cmd_t *cmd=finder_cmd_lookup(bar->menus[m].menuID,i+1);
   CHECK(cmd!=NULL,"each surviving row has a dispatch identity");if(!cmd)continue;
   int on=allowed(cmd->id,&f);
   CHECK(it->enabled==on && finder_cmd_enabled(cmd,&f)==on,"resource and dispatch obey independent contextual truth table");
   CHECK(MenuInfo_item_selectable(bar,m,i)==on,"mouse route agrees with supported context");
   if(it->cmdChar)CHECK(MenuKey(bar,it->cmdChar)==(on?MenuResult(bar->menus[m].menuID,i+1):0),"native shortcut reaches the same exact row");
  }
 }
 static const uint16_t modes[]={0,2,1,5,9,13};
 FinderCtx f;memset(&f,0,sizeof f);f.front_is_diskwin=1;
 for(unsigned mode=0;mode<6;mode++){
  f.view_bits=modes[mode];finder_menu_refresh_enables(&f);
  for(unsigned i=0;i<6;i++)CHECK((bar->menus[2].items[i].mark!=0)==(i==mode),"exactly the active supported View mode is checked");
 }
 return TEST_SUMMARY("test-finder-menu");
}
