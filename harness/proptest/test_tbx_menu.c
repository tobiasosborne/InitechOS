/*
 * test_tbx_menu.c -- A DISK TENANT HEARS ITS OWN MENU CHOICES (HOST oracle, C).
 *
 * bead: initech-tdnl.31 (audit 2026-10-04 pass 2 G03: the tenant's File > Quit
 *       (^Q) and Fixture > About were enabled, selectable, logged -- and inert;
 *       Ctrl-Q arrived as plain key events).
 *
 * Drives the REAL route decisions the kernel compiles (os/flair/tbxmenu.h,
 * included by os/flair/tbxgate.c) over the REAL Menu Manager (menu.c MenuKey):
 *
 *   M1 THE KEY ROUTE (MTE Listing 2-6, p. 2-44: DoMenuCommand(MenuKey(key))
 *      when the Command key is down and event.what = keyDown; PC Ctrl IS Cmd):
 *      Ctrl-q, Ctrl-Q and Cmd-q on the tenant's bar resolve to File > Quit;
 *      a plain 'q' (no chord), an autoKey/keyUp chord, a chord bound to a
 *      DISABLED item (Ctrl-X: Cut) and an unbound chord (Ctrl-W) resolve to
 *      nothing (they stay ordinary keys).
 *   M2 THE CONVERGENCE (MTE p. 3-78: "In either case, the application passes
 *      the function result returned by MenuSelect or MenuKey as a parameter to
 *      the DoMenuCommand procedure"): the mouse route's MenuSelect word and the
 *      key route's MenuKey word for File > Quit become a TBX_EVT_MENU event
 *      whose flat FlairEvent What/Message (the 8 bytes DoMenuCommand reads) are
 *      IDENTICAL, Message == (129 << 16) | 1 (hand-typed, not computed).
 *   M3 BAR IDENTITY: a choice made in ANOTHER bar is never the tenant's -- even
 *      a bar whose menus carry the SAME ids and items (identity is the bar the
 *      shell tracked, not its contents); nothing chosen (0, or an item word of
 *      0) delivers nothing; a NULL owner (no SETMBAR) delivers nothing.
 *   M4 THE VALUE: TBX_EVT_MENU == 0x0051 and aliases no Event Manager code
 *      (MTE Table 2-1: 0..15, kHighLevelEvent 23) -- hand-typed list.
 *
 * Expectations are HAND-TYPED (Law 2): the result words, the item/key tables
 * (transcribed from os/apps/tenantfx.asm's `mbar` resource by hand), the
 * event codes.
 *
 * MUTANTS (Rule 6; -D on this TU, which reaches tbxmenu.h):
 *   TBX_MUT_MENU_KEY_PLAIN  -- the chord is never resolved (M1 RED: "Ctrl-q")
 *   TBX_MUT_MENU_ANY_BAR    -- the identity guard is gone (M3 RED: "another")
 *
 * ASCII-clean (Rule 12).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tbxmenu.h"
#include "test_assert.h"

TEST_HARNESS();

/* os/apps/tenantfx.asm `mbar`, transcribed by hand. */
static const MenuItem file_items[] = {
    { "Quit", 0, 'Q', 0, 1, 0 },
};
static const MenuItem edit_items[] = {
    { "Undo", 0, 'Z', 0, 0, 0 },
    { "-",    0, 0,   0, 0, 1 },
    { "Cut",  0, 'X', 0, 0, 0 },
    { "Copy", 0, 'C', 0, 0, 0 },
    { "Paste",0, 'V', 0, 0, 0 },
};
static const MenuItem fixture_items[] = {
    { "About TenantFix", 0, 0, 0, 1, 0 },
};
static MenuInfo tenant_menus[] = {
    { 129, "File",    file_items,    1, 0 },
    { 130, "Edit",    edit_items,    5, 0 },
    { 131, "Fixture", fixture_items, 1, 0 },
};
static MenuBar tenant_bar = { tenant_menus, 3, 1 };

/* A DIFFERENT bar with the SAME content: identity, not equality, decides. */
static MenuInfo twin_menus[] = {
    { 129, "File",    file_items,    1, 0 },
    { 130, "Edit",    edit_items,    5, 0 },
    { 131, "Fixture", fixture_items, 1, 0 },
};
static MenuBar twin_bar = { twin_menus, 3, 1 };

#define QUIT_WORD  0x00810001u   /* (129 << 16) | 1, hand-typed */
#define ABOUT_WORD 0x00830001u   /* (131 << 16) | 1, hand-typed */

static EventRecord key(uint16_t what, char ch, uint8_t vkey, uint16_t mods)
{
    EventRecord e;
    memset(&e, 0, sizeof e);
    e.what = what;
    e.message = ((uint32_t)vkey << 8) | (uint32_t)(unsigned char)ch;
    e.modifiers = mods;
    e.when = 1234u;
    e.where.h = 300;
    e.where.v = 200;
    return e;
}

static void m1_key_route(void)
{
    EventRecord e;

    e = key(keyDown, 'q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == QUIT_WORD,
          "M1 Ctrl-q on the tenant bar is File > Quit (129,1)");
    e = key(keyDown, 'Q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY |
                                FLAIR_EVT_MOD_SHIFT_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == QUIT_WORD,
          "M1 Ctrl-Shift-Q (upper case) is File > Quit too");
    e = key(keyDown, 'q', 0x10, FLAIR_EVT_MOD_CMD_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == QUIT_WORD,
          "M1 Cmd-q (a real Command bit) is File > Quit");

    e = key(keyDown, 'q', 0x10, 0u);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == 0u,
          "M1 a plain q (no chord) is NOT a menu command");
    e = key(autoKey, 'q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == 0u,
          "M1 an autoKey chord is not a menu command (keyDown only)");
    e = key(keyUp, 'q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == 0u,
          "M1 a keyUp chord is not a menu command");
    e = key(keyDown, 'x', 0x2D, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == 0u,
          "M1 Ctrl-x is bound to a DISABLED item (Cut): nothing");
    e = key(keyDown, 'w', 0x11, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(tbx_menu_key_sel(&tenant_bar, &e) == 0u,
          "M1 Ctrl-w is bound to nothing in the tenant bar");
    e = key(keyDown, 'q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);
    CHECK(tbx_menu_key_sel((const MenuBar *)0, &e) == 0u,
          "M1 a tenant with no bar (no SETMBAR) has no command keys");
}

static void m2_convergence(void)
{
    EventRecord down, chord, from_mouse, from_key;
    int32_t eb_mouse[6], eb_key[6];

    memset(&down, 0, sizeof down);
    down.what = mouseDown;
    down.where.h = 38;          /* the File title in band 2 */
    down.where.v = 30;
    down.when = 77u;
    chord = key(keyDown, 'q', 0x10, FLAIR_EVT_MOD_CONTROL_KEY);

    memset(&from_mouse, 0xA5, sizeof from_mouse);
    memset(&from_key, 0x5A, sizeof from_key);
    CHECK(tbx_menu_event(&tenant_bar, &tenant_bar, QUIT_WORD, &down,
                         &from_mouse) == 1,
          "M2 the mouse route's File > Quit is the tenant's to hear");
    CHECK(tbx_menu_event(&tenant_bar, &tenant_bar,
                         tbx_menu_key_sel(&tenant_bar, &chord), &chord,
                         &from_key) == 1,
          "M2 the key route's File > Quit is the tenant's to hear");

    tbx_evbuf_fill(eb_mouse, &from_mouse);
    tbx_evbuf_fill(eb_key, &from_key);
    CHECK(eb_mouse[FLAIR_EVBUF_WHAT_OFF / 4] == 0x51 &&
          eb_key[FLAIR_EVBUF_WHAT_OFF / 4] == 0x51,
          "M2 both routes push What == TBX_EVT_MENU (0x51)");
    CHECK(eb_mouse[FLAIR_EVBUF_MSG_OFF / 4] == (int32_t)0x00810001 &&
          eb_key[FLAIR_EVBUF_MSG_OFF / 4] == (int32_t)0x00810001,
          "M2 both routes push Message == (129 << 16) | 1");
    CHECK(memcmp(eb_mouse, eb_key, 8) == 0,
          "M2 the What/Message DoMenuCommand reads are byte-identical");
    CHECK(eb_mouse[FLAIR_EVBUF_WHEREH_OFF / 4] == 38 &&
          eb_mouse[FLAIR_EVBUF_WHEREV_OFF / 4] == 30 &&
          eb_mouse[FLAIR_EVBUF_WHEN_OFF / 4] == 77,
          "M2 the mouse event carries its trigger's where/when");
    CHECK(eb_key[FLAIR_EVBUF_MODS_OFF / 4] ==
          (int32_t)FLAIR_EVT_MOD_CONTROL_KEY,
          "M2 the key event carries its trigger's modifiers");

    CHECK(tbx_menu_event(&tenant_bar, &tenant_bar, ABOUT_WORD, &down,
                         &from_mouse) == 1 &&
          from_mouse.message == 0x00830001u,
          "M2 Fixture > About pushes (131 << 16) | 1");
}

static void m3_identity(void)
{
    EventRecord down, out;

    memset(&down, 0, sizeof down);
    down.what = mouseDown;
    memset(&out, 0, sizeof out);
    out.what = 0x77u;   /* sentinel: untouched on refusal */

    CHECK(tbx_menu_event(&tenant_bar, &twin_bar, QUIT_WORD, &down, &out) == 0
          && out.what == 0x77u,
          "M3 a choice in another bar (same ids/items) is not the tenant's");
    CHECK(tbx_menu_event(&tenant_bar, &tenant_bar, 0u, &down, &out) == 0 &&
          out.what == 0x77u,
          "M3 nothing chosen (0) delivers nothing");
    CHECK(tbx_menu_event(&tenant_bar, &tenant_bar, 0x00810000u, &down, &out)
          == 0 && out.what == 0x77u,
          "M3 an item word of 0 delivers nothing");
    CHECK(tbx_menu_event((const MenuBar *)0, &tenant_bar, QUIT_WORD, &down,
                         &out) == 0,
          "M3 a tenant with no installed bar hears nothing");
}

static void m4_value(void)
{
    static const unsigned mac_codes[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 23
    };
    int clash = 0;
    CHECK(TBX_EVT_MENU == 0x0051u, "M4 TBX_EVT_MENU is 0x0051");
    for (unsigned i = 0; i < sizeof mac_codes / sizeof mac_codes[0]; i++)
        if (TBX_EVT_MENU == mac_codes[i]) clash = 1;
    CHECK(!clash, "M4 TBX_EVT_MENU aliases no Event Manager code");
}

int main(void)
{
    m1_key_route();
    m2_convergence();
    m3_identity();
    m4_value();
    return TEST_SUMMARY("test_tbx_menu");
}
