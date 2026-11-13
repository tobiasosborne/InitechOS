/* Independently pinned i386 record layouts from legacy-final gate offsets,
 * native EventRecord declarations and cp_dialog.h. Compiled for TARGET. */
#include "tbx.h"
#include <stddef.h>
#define SIZE(t,n) _Static_assert(sizeof(t)==(n),#t " size")
#define OFF(t,f,n) _Static_assert(offsetof(t,f)==(n),#t "." #f)
_Static_assert(sizeof(void *)==4,"target must be i386, not host");
SIZE(flair_point_t,4);OFF(flair_point_t,v,0);OFF(flair_point_t,h,2);
SIZE(tbx_rect_t,8);OFF(tbx_rect_t,top,0);OFF(tbx_rect_t,left,2);OFF(tbx_rect_t,bottom,4);OFF(tbx_rect_t,right,6);
SIZE(EventRecord,20);OFF(EventRecord,what,0);OFF(EventRecord,message,4);OFF(EventRecord,when,8);OFF(EventRecord,where,12);OFF(EventRecord,modifiers,16);
SIZE(tbx_event_t,24);OFF(tbx_event_t,what,0);OFF(tbx_event_t,message,4);OFF(tbx_event_t,modifiers,8);OFF(tbx_event_t,when,12);OFF(tbx_event_t,whereh,16);OFF(tbx_event_t,wherev,20);
SIZE(tbx_rec_t,24);OFF(tbx_rec_t,tag,0);OFF(tbx_rec_t,name,4);OFF(tbx_rec_t,image_base,8);OFF(tbx_rec_t,image_len,12);OFF(tbx_rec_t,evbuf,16);OFF(tbx_rec_t,proc,20);
SIZE(tbx_menuitem_t,12);OFF(tbx_menuitem_t,text,0);OFF(tbx_menuitem_t,mark,4);OFF(tbx_menuitem_t,cmd_char,5);OFF(tbx_menuitem_t,style,6);OFF(tbx_menuitem_t,enabled,7);OFF(tbx_menuitem_t,is_divider,8);OFF(tbx_menuitem_t,pad,9);
SIZE(tbx_menuinfo_t,16);OFF(tbx_menuinfo_t,menu_id,0);OFF(tbx_menuinfo_t,pad,2);OFF(tbx_menuinfo_t,title,4);OFF(tbx_menuinfo_t,items,8);OFF(tbx_menuinfo_t,n_items,12);OFF(tbx_menuinfo_t,menu_width,14);
SIZE(tbx_menubar_t,8);OFF(tbx_menubar_t,menus,0);OFF(tbx_menubar_t,n_menus,4);OFF(tbx_menubar_t,has_apple,6);OFF(tbx_menubar_t,pad,7);
SIZE(tbx_find_rec_t,22);OFF(tbx_find_rec_t,attr,0);OFF(tbx_find_rec_t,time,1);OFF(tbx_find_rec_t,date,3);OFF(tbx_find_rec_t,size,5);OFF(tbx_find_rec_t,name,9);
SIZE(cp_time_t,8);OFF(cp_time_t,year,0);OFF(cp_time_t,month,2);OFF(cp_time_t,day,3);OFF(cp_time_t,hour,4);OFF(cp_time_t,minute,5);OFF(cp_time_t,second,6);OFF(cp_time_t,pad,7);
SIZE(cp_settings_t,6);OFF(cp_settings_t,pattern,0);OFF(cp_settings_t,tracking,1);OFF(cp_settings_t,double_click,2);OFF(cp_settings_t,sound,3);OFF(cp_settings_t,volume,4);OFF(cp_settings_t,clock,5);
SIZE(cp_item_t,16);OFF(cp_item_t,left,0);OFF(cp_item_t,top,2);OFF(cp_item_t,right,4);OFF(cp_item_t,bottom,6);OFF(cp_item_t,type,8);OFF(cp_item_t,value,10);OFF(cp_item_t,text,12);
