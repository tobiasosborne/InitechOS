/*
 * os/flair/apple_menu.h -- the system Apple menu (THE ARTIFACT; beads
 * initech-tdnl.47 / initech-tdnl.17). Pure, host-buildable: the rows come
 * from the locked table spec/apple_menu_items.h, filtered by an "is it
 * installed" question the caller answers (kmain asks FAT; the host oracle
 * answers from a fixture), and a chosen row is turned back into an action.
 * menu.c drops it behind every bar's Apple glyph (MenuBar_set_apple_menu).
 *
 * MUTANT KNOB (Rule 6): APPLE_MENU_MUT_LIST_ALL -- every table row is listed
 * whether or not its program is installed (the F4-3 omit rule broken: a
 * row that cannot launch). NEVER in a real build.
 *
 * ASCII-clean (Rule 12).
 */
#ifndef INITECH_OS_FLAIR_APPLE_MENU_H
#define INITECH_OS_FLAIR_APPLE_MENU_H

#include <stdint.h>
#include "menu.h"

typedef struct apple_da {
    const char *title;      /* the row text                 */
    const char *dir83;      /* its directory from the root  */
    const char *file83;     /* its program                  */
} apple_da_t;

/* 1 iff dir83\file83 is on the mounted volume. */
typedef int (*apple_installed_fn)(void *user, const char *dir83,
                                  const char *file83);

typedef enum apple_act {
    APPLE_ACT_NONE = 0,
    APPLE_ACT_ABOUT,
    APPLE_ACT_LAUNCH
} apple_act_t;

/* (Re)build the rows: About This Computer..., a divider, then every
 * installed accessory in table order. Returns the menu (static storage, menu
 * id FLAIR_MENU_ID_APPLE); the same pointer every call. */
const MenuInfo *apple_menu_build(apple_installed_fn installed, void *user);

/* The action of a MenuSelect result word; *da is set for APPLE_ACT_LAUNCH. */
apple_act_t apple_menu_action(uint32_t sel, const apple_da_t **da);

/* The locked table (for the oracle). */
const apple_da_t *apple_menu_table(int *count);

#endif /* INITECH_OS_FLAIR_APPLE_MENU_H */
