/* Private gate layout checks. Sources: sdk/include/toolbox_gate.h Sections
 * 5, 6 and 9; sdk/tests/layout.c; legacy-final:os/flair/menu.h.
 * Pointer-bearing native host types are never interpreted as i386 wire data.
 */
#ifndef INITECH_FLAIR_TBX_LAYOUT_H
#define INITECH_FLAIR_TBX_LAYOUT_H
#include <stddef.h>
#include "tbx.h"
#include "menu.h"
#include "region_algebra.h"
#define PIN_SIZE(t,n) _Static_assert(sizeof(t) == (n), #t " wire size")
#define PIN_OFF(t,f,n) _Static_assert(offsetof(t,f) == (n), #t "." #f " wire offset")
#define MATCH_SIZE(a,b) _Static_assert(sizeof(a) == sizeof(b), #a " / " #b " size")
#define MATCH_OFF(a,f,b,g) _Static_assert(offsetof(a,f) == offsetof(b,g), #a "." #f " / " #b "." #g)
MATCH_SIZE(rgn_rect_t, tbx_rect_t);
MATCH_OFF(rgn_rect_t, top, tbx_rect_t, top);
MATCH_OFF(rgn_rect_t, left, tbx_rect_t, left);
MATCH_OFF(rgn_rect_t, bottom, tbx_rect_t, bottom);
MATCH_OFF(rgn_rect_t, right, tbx_rect_t, right);
PIN_SIZE(tbx_event_t, FLAIR_EVBUF_BYTES);
PIN_OFF(tbx_event_t, what, FLAIR_EVBUF_WHAT_OFF);
PIN_OFF(tbx_event_t, message, FLAIR_EVBUF_MSG_OFF);
PIN_OFF(tbx_event_t, modifiers, FLAIR_EVBUF_MODS_OFF);
PIN_OFF(tbx_event_t, when, FLAIR_EVBUF_WHEN_OFF);
PIN_OFF(tbx_event_t, whereh, FLAIR_EVBUF_WHEREH_OFF);
PIN_OFF(tbx_event_t, wherev, FLAIR_EVBUF_WHEREV_OFF);
#if UINTPTR_MAX == 0xFFFFFFFFu
#define WIRE_SIZE(t,n) sizeof(t)
#define WIRE_OFF(t,f,n) offsetof(t,f)
PIN_SIZE(tbx_rec_t, FLAIR_TENANT_REC_BYTES);
PIN_OFF(tbx_rec_t, tag, FLAIR_TENANT_REC_TAG_OFF);
PIN_OFF(tbx_rec_t, name, FLAIR_TENANT_REC_NAME_OFF);
PIN_OFF(tbx_rec_t, image_base, FLAIR_TENANT_REC_BASE_OFF);
PIN_OFF(tbx_rec_t, image_len, FLAIR_TENANT_REC_LEN_OFF);
PIN_OFF(tbx_rec_t, evbuf, FLAIR_TENANT_REC_EVB_OFF);
PIN_OFF(tbx_rec_t, proc, FLAIR_TENANT_REC_PROC_OFF);
PIN_SIZE(tbx_menubar_t, TBX_MBAR_BYTES);
PIN_OFF(tbx_menubar_t, menus, TBX_MBAR_MENUS_OFF);
PIN_OFF(tbx_menubar_t, n_menus, TBX_MBAR_COUNT_OFF);
PIN_OFF(tbx_menubar_t, has_apple, TBX_MBAR_APPLE_OFF);
PIN_SIZE(tbx_menuinfo_t, TBX_MINFO_BYTES);
PIN_OFF(tbx_menuinfo_t, menu_id, TBX_MINFO_ID_OFF);
PIN_OFF(tbx_menuinfo_t, title, TBX_MINFO_TITLE_OFF);
PIN_OFF(tbx_menuinfo_t, items, TBX_MINFO_ITEMS_OFF);
PIN_OFF(tbx_menuinfo_t, n_items, TBX_MINFO_COUNT_OFF);
PIN_SIZE(tbx_menuitem_t, TBX_MITEM_BYTES);
PIN_OFF(tbx_menuitem_t, text, TBX_MITEM_TEXT_OFF);
MATCH_SIZE(MenuBar, tbx_menubar_t);
MATCH_OFF(MenuBar, menus, tbx_menubar_t, menus);
MATCH_OFF(MenuBar, n_menus, tbx_menubar_t, n_menus);
MATCH_OFF(MenuBar, has_apple, tbx_menubar_t, has_apple);
MATCH_SIZE(MenuInfo, tbx_menuinfo_t);
MATCH_OFF(MenuInfo, menuID, tbx_menuinfo_t, menu_id);
MATCH_OFF(MenuInfo, title, tbx_menuinfo_t, title);
MATCH_OFF(MenuInfo, items, tbx_menuinfo_t, items);
MATCH_OFF(MenuInfo, n_items, tbx_menuinfo_t, n_items);
MATCH_OFF(MenuInfo, menuWidth, tbx_menuinfo_t, menu_width);
MATCH_SIZE(MenuItem, tbx_menuitem_t);
MATCH_OFF(MenuItem, text, tbx_menuitem_t, text);
MATCH_OFF(MenuItem, mark, tbx_menuitem_t, mark);
MATCH_OFF(MenuItem, cmdChar, tbx_menuitem_t, cmd_char);
MATCH_OFF(MenuItem, style, tbx_menuitem_t, style);
MATCH_OFF(MenuItem, enabled, tbx_menuitem_t, enabled);
MATCH_OFF(MenuItem, is_divider, tbx_menuitem_t, is_divider);
PIN_SIZE(cp_item_t, 16);
PIN_OFF(cp_item_t, left, 0);
PIN_OFF(cp_item_t, top, 2);
PIN_OFF(cp_item_t, right, 4);
PIN_OFF(cp_item_t, bottom, 6);
PIN_OFF(cp_item_t, type, 8);
PIN_OFF(cp_item_t, value, 10);
PIN_OFF(cp_item_t, text, 12);
#else
/* The hosted routing suite reads flat i386 bytes, never native host records. */
#define WIRE_SIZE(t,n) (n)
#define WIRE_OFF(t,f,n) (n)
#endif
#undef PIN_SIZE
#undef PIN_OFF
#undef MATCH_SIZE
#undef MATCH_OFF
#endif
