/* Finder's shipping GM4 projection. Scope and frozen omissions: README.
 * Macintosh System Software User's Guide V6, pp.87-90,118-119; the approved
 * phase-2 split retains original command IDs while closing row gaps. */
#include "finder_menu.h"
#include "finder_windows.h"
#define ITEM(t,k,on) {t,0,k,0,on,0}
#define DIV {"-",0,0,0,0,1}
static MenuItem finder_apple_items[]={ITEM("About This Computer",0,0)};
static MenuInfo finder_apple_menu={0,"",finder_apple_items,1,0};
static MenuItem finder_file_items[]={
 ITEM("New Folder",'N',1),ITEM("Open",'O',0),ITEM("Close Window",'W',0),DIV,
 ITEM("Get Info",'I',0),ITEM("Put Away",'Y',0)
};
static MenuItem finder_edit_items[]={ITEM("Select All",'A',1),ITEM("Show Clipboard",0,1)};
static MenuItem finder_view_items[]={
 ITEM("by Icons",0,0),ITEM("by Small Icon",0,0),ITEM("by Name",0,0),
 ITEM("by Size",0,0),ITEM("by Kind",0,0),ITEM("by Date",0,0),DIV,
 ITEM("Clean Up",0,0),ITEM("Arrange (by Name)",0,0)
};
static MenuItem finder_special_items[]={
 ITEM("Clean Up",0,0),ITEM("Empty Trash",0,0),DIV,
 ITEM("Restart",0,1),ITEM("Shut Down",0,1),DIV,ITEM("Exit to InitechDOS",0,1)
};
#define MENU(id,title,items) {id,title,items,sizeof items/sizeof items[0],0}
static MenuInfo finder_menus[]={
 MENU(512,"File",finder_file_items),MENU(513,"Edit",finder_edit_items),
 MENU(514,"View",finder_view_items),MENU(515,"Special",finder_special_items)
};
static MenuBar finder_bar={finder_menus,4,1};
MenuBar *finder_menu_bar(void){return &finder_bar;}
MenuInfo *finder_menu_apple(void){return &finder_apple_menu;}
void finder_menu_refresh_enables(const FinderCtx *fx)
{
 finder_file_items[0].enabled=finder_pred_new_folder_here(fx);
 finder_file_items[1].enabled=finder_pred_selection_openable(fx);
 finder_file_items[2].enabled=finder_pred_front_is_diskwin(fx);
 finder_file_items[4].enabled=finder_pred_selection_infoable(fx);
 finder_file_items[5].enabled=finder_pred_selection_putaway(fx);
 static const uint16_t modes[]={0,2,1,5,9,13};
 for(unsigned i=0;i<9;i++)if(i!=6)finder_view_items[i].enabled=finder_pred_front_is_diskwin(fx);
 for(unsigned i=0;i<6;i++)finder_view_items[i].mark=
  fx&&fx->front_is_diskwin&&(fx->view_bits&FINDER_VIEW_MASK)==modes[i]?FINDER_MENU_MARK_CHECK:0;
 finder_special_items[0].enabled=finder_pred_front_is_diskwin(fx);
 finder_special_items[1].enabled=finder_pred_trash_nonempty(fx);
}
