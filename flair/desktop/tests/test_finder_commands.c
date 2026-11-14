/* Revised operator-approved GM4 projection. Expected names, row identities
 * and effects are authored here, independent of the shipping arrays. Mac
 * System Software Guide V6 printed87-90,118-119 supplies Info/Clipboard. */
#define main old_windows_main
#include "test_finder_windows.c"
#undef main
#include "finder_menu.h"
#include "scrap.h"
static const char *const file_names[]={"New Folder","Open","Close Window","-","Get Info","Put Away"};
static const char *const edit_names[]={"Select All","Show Clipboard"};
static const char *const view_names[]={"by Icons","by Small Icon","by Name","by Size","by Kind","by Date","-","Clean Up","Arrange (by Name)"};
static const char *const special_names[]={"Clean Up","Empty Trash","-","Restart","Shut Down","-","Exit to InitechDOS"};
static void manifest(void)
{
    const char *const *names[]={file_names,edit_names,view_names,special_names};
    const char *titles[]={"File","Edit","View","Special"};
    const unsigned counts[]={6,2,9,7};
    const char *keys[]={"NOW\0IY","A\0","",""};
    const unsigned ids[4][9]={{2,3,4,0,5,7},{8,22},{14,17,18,19,20,21,0,9,10},{9,11,0,12,13,0,16}};
    MenuBar *bar=finder_menu_bar();CHECK(bar->n_menus==4,"sweep exactly four Finder titles");
    FinderCtx ctx;memset(&ctx,0,sizeof ctx);finder_menu_refresh_enables(&ctx);
    for(unsigned m=0;m<4&&m<bar->n_menus;m++){
        const MenuInfo *mi=&bar->menus[m];
        CHECK(mi->menuID==512+(int)m&&!strcmp(mi->title,titles[m]),"sweep stable menu identity/title");
        CHECK(mi->n_items==counts[m],"sweep exact row count including dividers");
        for(unsigned i=0;i<counts[m]&&i<mi->n_items;i++){
            const MenuItem *it=&mi->items[i];const finder_cmd_t *cmd=finder_cmd_lookup(512+(int)m,i+1);
            CHECK(!strcmp(it->text,names[m][i]),"sweep exact ordered command/omission manifest");
            CHECK(it->is_divider==(ids[m][i]==0),"sweep exact divider positions");
            CHECK(ids[m][i] ? cmd&&cmd->id==(finder_cmd_id)ids[m][i] : cmd==NULL,"sweep menu row reaches stable semantic command ID");
            CHECK(it->cmdChar==(m<2?keys[m][i]:0),"sweep literal shortcut manifest");
        }
    }
    for(const char *p="DXCVZF";*p;p++)CHECK(MenuKey(bar,*p)==0,"sweep removed shortcuts have no Finder route");
    CHECK(finder_shell_implements(FCMD_GET_INFO),"sweep Get Info has an execution route");
    CHECK(finder_shell_implements((finder_cmd_id)22),"sweep Clipboard has an execution route");
    {
        const flair_point_t old_exit={.h=170,.v=143}, new_exit={.h=170,.v=105};
        CHECK(MenuSelect(bar,(flair_point_t){.h=170,.v=10},&old_exit,1)==0,"old factory Exit coordinate is outside shortened menu");
        CHECK(MenuSelect(bar,(flair_point_t){.h=170,.v=10},&new_exit,1)==MenuResult(515,7),"reviewable factory Exit coordinate reaches the retained command");
    }
}
#include "finder_viewers.h"
#ifndef FINDER_COMMANDS_NO_MAIN
int main(void){manifest();info_contexts();clipboard();gestures();return TEST_SUMMARY("test-finder-commands");}
#endif
