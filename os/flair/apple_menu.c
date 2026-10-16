/*
 * os/flair/apple_menu.c -- the system Apple menu (THE ARTIFACT; beads
 * initech-tdnl.47 / initech-tdnl.17). See apple_menu.h; the rows' ground
 * truth is spec/apple_menu_items.h. Freestanding, no malloc.
 * ASCII-clean (Rule 12).
 */
#include "apple_menu.h"
#include "apple_menu_items.h"

static const apple_da_t g_das[APPLE_MENU_DA_COUNT] = APPLE_MENU_DA_TABLE;

static MenuItem   g_items[2 + APPLE_MENU_DA_COUNT];
static int        g_row_da[2 + APPLE_MENU_DA_COUNT];   /* row -> table index */
static MenuInfo   g_apple = { (int16_t)FLAIR_MENU_ID_APPLE, "", g_items, 0u, 0 };

static void set_item(MenuItem *it, const char *text, uint8_t divider)
{
    it->text = text;
    it->mark = 0;
    it->cmdChar = 0;
    it->style = 0u;
    it->enabled = (uint8_t)(divider ? 0u : 1u);
    it->is_divider = divider;
}

const MenuInfo *apple_menu_build(apple_installed_fn installed, void *user)
{
    int n = 0;
    set_item(&g_items[n], APPLE_MENU_ABOUT_TEXT, 0u); g_row_da[n++] = -1;
    set_item(&g_items[n], "-", 1u);                   g_row_da[n++] = -1;
    for (int i = 0; i < APPLE_MENU_DA_COUNT; i++) {
#if !defined(APPLE_MENU_MUT_LIST_ALL)
        if (installed == (apple_installed_fn)0 ||
            !installed(user, g_das[i].dir83, g_das[i].file83))
            continue;                         /* F4-3: omit, never gray */
#else
        (void)installed; (void)user;
#endif
        set_item(&g_items[n], g_das[i].title, 0u);
        g_row_da[n++] = i;
    }
    if (n == 2) n = 1;                        /* nothing installed: no divider */
    g_apple.n_items = (uint16_t)n;
    g_apple.menuWidth = 0;                    /* the rows changed: re-measure */
    return &g_apple;
}

apple_act_t apple_menu_action(uint32_t sel, const apple_da_t **da)
{
    int item = (int)MenuResultItem(sel);
    if (da != (const apple_da_t **)0) *da = (const apple_da_t *)0;
    if (MenuResultID(sel) != (int16_t)FLAIR_MENU_ID_APPLE || item < 1 ||
        item > (int)g_apple.n_items)
        return APPLE_ACT_NONE;
    if (item == 1) return APPLE_ACT_ABOUT;
    if (g_row_da[item - 1] < 0) return APPLE_ACT_NONE;
    if (da != (const apple_da_t **)0) *da = &g_das[g_row_da[item - 1]];
    return APPLE_ACT_LAUNCH;
}

const apple_da_t *apple_menu_table(int *count)
{
    if (count != (int *)0) *count = APPLE_MENU_DA_COUNT;
    return g_das;
}
