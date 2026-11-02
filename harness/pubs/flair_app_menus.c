/*
 * harness/pubs/flair_app_menus.c -- the menu bar of ONE shipped FLAIR disk
 * application, for the FLAIR manuals' GC-01 gate (bead initech-ipu5.11;
 * PUB-REF-0001 GC-01, AN-14).
 *
 * Built once per application by pubs/flair.mk with -DAPP_SRC="<its .c>":
 * this file INCLUDES the application's own source -- the very file the
 * tenant build compiles -- and walks the tbx_menubar_t `mbar` it hands
 * SETMBAR, so the inventory is the shipped resource, never a copy (the
 * single-TU probe pattern of harness/pubs/int21_inventory.c; unreferenced
 * application code is dropped by --gc-sections, so the Toolbox traps it
 * calls need not link on the host).  Prints, for every item that is not a
 * divider:
 *
 *   <APP>><Menu>><Item>><Key>><State>|menu
 *
 * APP is the application's tbx_app_name (CALC, CALCULAT, CTENANT, TEXT,
 * PAINT); Key is the shipped command-key character or '-'; State is on or
 * off, the enable byte the bar ships with (the Menu Manager reads it when the
 * menu is pulled down; an application may change it at run time, and the
 * manual says when).  The MacRoman ellipsis \311 prints as "...".
 * ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include APP_SRC

static void out(const char *s)
{
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == 0xC9) fputs("...", stdout);
        else if (c < 32 || c > 126 || c == '|' || c == '>') {
            fprintf(stderr, "!!! test-pubs RED FLAIR-INVENTORY: %s: unprintable menu text\n", tbx_app_name);
            exit(1);
        } else putchar(c);
    }
}

int main(void)
{
    if (mbar.n_menus == 0) {
        fprintf(stderr, "!!! test-pubs RED FLAIR-INVENTORY: %s: empty menu bar\n", tbx_app_name);
        return 1;
    }
    for (unsigned m = 0; m < mbar.n_menus; m++) {
        const tbx_menuinfo_t *mi = &mbar.menus[m];
        for (unsigned i = 0; i < mi->n_items; i++) {
            const tbx_menuitem_t *it = &mi->items[i];
            if (it->is_divider) continue;
            printf("%s>", tbx_app_name);
            out(mi->title);
            putchar('>');
            out(it->text);
            printf(">%c>%s|menu\n", it->cmd_char ? it->cmd_char : '-', it->enabled ? "on" : "off");
        }
    }
    return ferror(stdout) ? 1 : 0;
}
