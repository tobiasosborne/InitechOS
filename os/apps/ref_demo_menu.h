/* tdnl.79: the actual HELLO/NOTES menu resources. Ref: PRD 6.3,
 * spec/flair_tenants_demo.h. Unimplemented demo actions must be dimmed. */
#ifndef INITECH_REF_DEMO_MENU_H
#define INITECH_REF_DEMO_MENU_H
#include "menu.h"
#include "flair_tenants_demo.h"
#ifdef FINDER_MUT_DEMO_LIVE
#define FLAIR_DEMO_ITEMS_ENABLED 1
#else
#define FLAIR_DEMO_ITEMS_ENABLED 0
#endif
static inline MenuItem flair_demo_item(const char *text,char key)
{
    MenuItem it={text,0,key,0,FLAIR_DEMO_ITEMS_ENABLED,0};return it;
}
static const MenuItem flair_ten_notes_file_items[] = {
    { FLAIR_TEN_NOTES_ITEM_NEW,  0, 'N', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 },
    { FLAIR_TEN_NOTES_ITEM_OPEN, 0, 'O', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 },
    { FLAIR_TEN_NOTES_ITEM_SAVE, 0, 'S', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 },
    { FLAIR_TEN_NOTES_ITEM_QUIT, 0, 'Q', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 }
};
static const MenuItem flair_ten_notes_edit_items[] = {
    { FLAIR_TEN_NOTES_ITEM_UNDO,  0, 'Z', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 },
    { FLAIR_TEN_NOTES_ITEM_CUT,   0, 'X', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 },
    { FLAIR_TEN_NOTES_ITEM_COPY,  0, 'C', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 },
    { FLAIR_TEN_NOTES_ITEM_PASTE, 0, 'V', 0, FLAIR_DEMO_ITEMS_ENABLED, 0 }
};
static const MenuItem flair_ten_notes_notes_items[] = {
    { FLAIR_TEN_NOTES_ITEM_ABOUT, 0, 0, 0, FLAIR_DEMO_ITEMS_ENABLED, 0 }
};
static MenuInfo flair_ten_notes_menus[FLAIR_TEN_NOTES_MENU_COUNT] = {
    { (int16_t)(FLAIR_TEN_NOTES_MENU_ID_BASE + 0),
      FLAIR_TEN_NOTES_MENU_FILE_TITLE, flair_ten_notes_file_items, 4, 0 },
    { (int16_t)(FLAIR_TEN_NOTES_MENU_ID_BASE + 1),
      FLAIR_TEN_NOTES_MENU_EDIT_TITLE, flair_ten_notes_edit_items, 4, 0 },
    { (int16_t)(FLAIR_TEN_NOTES_MENU_ID_BASE + 2),
      FLAIR_TEN_NOTES_MENU_NOTES_TITLE, flair_ten_notes_notes_items, 1, 0 }
};
static MenuBar flair_ten_notes_bar __attribute__((unused)) = {
    flair_ten_notes_menus, FLAIR_TEN_NOTES_MENU_COUNT, 0
};
#endif
