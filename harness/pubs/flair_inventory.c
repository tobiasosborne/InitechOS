/*
 * harness/pubs/flair_inventory.c -- the SHIPPED FLAIR surface, for the
 * FLAIR manuals' GC-01 gate (bead initech-ipu5.11; PUB-REF-0001 GC-01,
 * CP-10, AN-14, NT-13).
 *
 * The FLAIR counterpart of harness/pubs/int21_inventory.c and
 * dos_surface_inventory.c: the inventory is READ OUT OF THE SHIPPED SOURCE,
 * never typed.  It prints one NAME|KIND line per user-visible item:
 *
 *   <CTX>><Menu>><Item>><Key>><State>|menu
 *       one menu item.  CTX is the bar that carries it: SYSTEM (the top,
 *       shell-owned bar), APPLE (the Apple menu), FINDER (the Finder's bar),
 *       HELLO and NOTES (the two resident windows' bars), TENANTFX (the
 *       assembler sample application).  Key is the command-key character
 *       (the shipped cmdChar) or '-'; State is on (enabled at rest), off
 *       (drawn dimmed at rest) or cond (a Finder item the shell enables and
 *       dims by a predicate, os/flair/finder_menu.c FINDER_MENU_PREDS).  An
 *       item text's MacRoman ellipsis (\311) is written "...".  Dividers are
 *       not items.
 *   APPS\<FILE>|file and <FILE>|file
 *       every file the Makefile ships on the INITECH data volume
 *       (FLAIR_FIXTURES, passed as arguments).
 *
 * The disk applications written in C (CALC, CALCULAT, CTENANT, TEXT, PAINT)
 * print their own bars through harness/pubs/flair_app_menus.c, which
 * includes each application's source; the two lists are concatenated by
 * pubs/flair.mk.
 *
 * Sources, all read here and nowhere copied:
 *   os/flair/finder_menu.c        the Finder bar + its predicate map
 *                                 (included whole: the map is static)
 *   os/flair/apple_menu.c,        the Apple menu: About This Computer... and
 *   spec/apple_menu_items.h       every accessory whose program ships
 *   os/flair/shell_menus.h        the item list behind every SYSTEM title
 *   spec/assets/menu_canon.h      HELLO's bar titles (the Photoshop bar)
 *   <kmain.i>                     os/milton/kmain.c PREPROCESSED with the
 *                                 flagship kernel's -D set: the SYSTEM
 *                                 titles (fd_sys_titles), HELLO's item pair
 *                                 and the NOTES bar (flair_ten_notes_*)
 *   os/apps/tenantfx.asm          the TENANTFX bar (its MINFO/MITEM/MDIV
 *                                 macros and db strings)
 *
 * Usage: flair_inventory <kmain.i> <tenantfx.asm> <fixture path>...
 * Fails loud (Rule 2) when a source no longer has the shape this reads.
 * Host C, built by pubs/flair.mk with the Menu Manager include set.
 * ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../os/flair/finder_menu.c"
#include "../../os/flair/finder_cmd.c"
#include "../../os/flair/apple_menu.c"
#include "shell_menus.h"
#include "menu_canon.h"

static void die(const char *what, const char *detail)
{
    fflush(stdout);
    fprintf(stderr, "!!! test-pubs RED FLAIR-INVENTORY: %s: %s\n", what, detail);
    exit(1);
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *s;
    if (!f) die("cannot open", path);
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0) die("cannot size", path);
    rewind(f);
    s = malloc((size_t)n + 1);
    if (!s || fread(s, 1, (size_t)n, f) != (size_t)n) die("cannot read", path);
    s[n] = 0;
    fclose(f);
    return s;
}

/* Item text as printed: the MacRoman ellipsis 0xC9 becomes "...". */
static void text_out(const char *t, char *out, size_t cap)
{
    size_t n = 0;
    for (; *t && n + 4 < cap; t++) {
        if ((unsigned char)*t == 0xC9) { memcpy(out + n, "...", 3); n += 3; }
        else if ((unsigned char)*t < 32 || (unsigned char)*t > 126 || *t == '|' || *t == '>')
            die("item text", "unprintable or separator byte in a menu item");
        else out[n++] = *t;
    }
    out[n] = 0;
}

static void item_line(const char *ctx, const char *menu, const char *text, char key, const char *state)
{
    char t[128], m[64];
    text_out(text, t, sizeof t);
    text_out(menu, m, sizeof m);
    if (!*t || !*m) die(ctx, "empty menu or item text");
    if (key && (key < 33 || key > 126)) die(ctx, "command key outside printable ASCII");
    printf("%s>%s>%s>%c>%s|menu\n", ctx, m, t, key ? key : '-', state);
}

static void walk_menu(const char *ctx, const MenuInfo *mi, int menu_ix, const fm_pred_row_t *preds, int npreds)
{
    for (int i = 0; i < (int)mi->n_items; i++) {
        const MenuItem *it = &mi->items[i];
        const char *st;
        if (it->is_divider) continue;
        st = it->enabled ? "on" : "off";
        for (int p = 0; p < npreds; p++)
            if (preds[p].menu_ix == menu_ix && preds[p].item_ix == i) st = "cond";
        item_line(ctx, mi->title, it->text, it->cmdChar, st);
    }
}

/* ---- tiny C-initializer reader over the preprocessed kmain ------------- */
static const char *skipws(const char *p)
{
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* "str" -> buf; returns the position after the closing quote. */
static const char *cstring(const char *p, char *buf, size_t cap, const char *where)
{
    size_t n = 0;
    p = skipws(p);
    if (*p != '"') die(where, "expected a string literal");
    for (p++; *p && *p != '"'; p++) {
        int c = (unsigned char)*p;
        if (c == '\\') {
            p++;
            if (*p >= '0' && *p <= '7') {
                c = 0;
                for (int k = 0; k < 3 && *p >= '0' && *p <= '7'; k++, p++) c = c * 8 + (*p - '0');
                p--;
            } else c = (unsigned char)*p;
        }
        if (n + 1 >= cap) die(where, "string too long");
        buf[n++] = (char)c;
    }
    if (*p != '"') die(where, "unterminated string");
    buf[n] = 0;
    p++;
    /* adjacent literals concatenate */
    {
        const char *q = skipws(p);
        if (*q == '"') {
            char more[128];
            p = cstring(q, more, sizeof more, where);
            if (n + strlen(more) + 1 > cap) die(where, "string too long");
            strcat(buf, more);
        }
    }
    return p;
}

/* A scalar initializer: 'c', a number, or a cast like (uint8_t)1. */
static const char *scalar(const char *p, long *v, const char *where)
{
    p = skipws(p);
    while (*p == '(') {                         /* a cast: skip it */
        const char *q = strchr(p, ')');
        if (!q) die(where, "bad cast");
        p = skipws(q + 1);
    }
    if (*p == '\'') {
        if (p[1] == '\\') die(where, "escaped character literal");
        *v = (unsigned char)p[1];
        if (p[2] != '\'') die(where, "bad character literal");
        return p + 3;
    }
    if (!isdigit((unsigned char)*p)) die(where, "expected a number or a character");
    *v = strtol(p, (char **)&p, 0);
    while (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L') p++;
    return p;
}

/* One MenuItem initializer { "text", mark, cmd, style, enabled, divider }. */
static const char *menu_item(const char *p, MenuItem *it, char *store, const char *where)
{
    long mark, cmd, style, en, div;
    p = skipws(p);
    if (*p != '{') die(where, "expected {");
    p = cstring(p + 1, store, 64, where);
    p = skipws(p); if (*p++ != ',') die(where, "expected ,");
    p = scalar(p, &mark, where); p = skipws(p); if (*p++ != ',') die(where, "expected ,");
    p = scalar(p, &cmd, where);  p = skipws(p); if (*p++ != ',') die(where, "expected ,");
    p = scalar(p, &style, where); p = skipws(p); if (*p++ != ',') die(where, "expected ,");
    p = scalar(p, &en, where);   p = skipws(p); if (*p++ != ',') die(where, "expected ,");
    p = scalar(p, &div, where);  p = skipws(p);
    if (*p++ != '}') die(where, "expected }");
    it->text = store; it->mark = (char)mark; it->cmdChar = (char)cmd;
    it->style = (uint8_t)style; it->enabled = (uint8_t)en; it->is_divider = (uint8_t)div;
    return p;
}

static const char *find(const char *src, const char *needle, const char *where)
{
    const char *p = strstr(src, needle);
    if (!p) die(where, needle);
    if (strstr(p + 1, needle)) die(where, "found twice -- the source changed shape");
    return p + strlen(needle);
}

/* static const MenuItem NAME[] = { {..}, {..} }; -> items */
static int item_array(const char *src, const char *name, MenuItem *items, char store[][64], int cap)
{
    char needle[128];
    int n = 0;
    snprintf(needle, sizeof needle, "MenuItem %s[] = {", name);
    const char *p = find(src, needle, "kmain.i");
    for (;;) {
        p = skipws(p);
        if (*p == '}') break;
        if (n == cap) die(name, "too many items");
        p = menu_item(p, &items[n], store[n], name);
        n++;
        p = skipws(p);
        if (*p == ',') p++;
    }
    if (!n) die(name, "no items");
    return n;
}

/* ---- TENANTFX: its MINFO / MITEM / MDIV macros -------------------------- */
static const char *asm_string(const char *src, const char *label)
{
    static char buf[16][64];
    static int k;
    char needle[80];
    const char *p;
    snprintf(needle, sizeof needle, "\n%s:", label);
    p = strstr(src, needle);
    if (!p) die("tenantfx.asm", label);
    p = skipws(p + strlen(needle));
    if (strncmp(p, "db \"", 4)) die("tenantfx.asm: no db string for", label);
    char *b = buf[k++ & 15];
    size_t n = 0;
    for (p += 4; *p && *p != '"' && n < 63; p++) b[n++] = *p;
    b[n] = 0;
    return b;
}

static void tenantfx(const char *path)
{
    char *src = slurp(path);
    /* the shipped bar is `mbar` (the About command's mbar2 adds Info) */
    const char *bars[2] = { "\nmbar_menus:", "\nmbar2_menus:" };
    char seen[64][160];
    int nseen = 0;
    for (int b = 0; b < 2; b++) {
        const char *p = strstr(src, bars[b]);
        if (!p) die("tenantfx.asm", bars[b] + 1);
        p += strlen(bars[b]);
        for (;;) {
            char title[32], items[32];
            int n;
            p = skipws(p);
            if (strncmp(p, "MINFO", 5)) break;
            if (sscanf(p, "MINFO %*d, %31[^,], %31[^,], %d", title, items, &n) != 3)
                die("tenantfx.asm", "bad MINFO line");
            const char *t = asm_string(src, title);
            char lab[40];
            snprintf(lab, sizeof lab, "\n%s:", items);
            const char *q = strstr(src, lab);
            if (!q) die("tenantfx.asm", items);
            q += strlen(lab);
            for (int i = 0; i < n; i++) {
                q = skipws(q);
                if (!strncmp(q, "MDIV", 4)) { q += 4; continue; }
                char s[32], ks[8];
                int en;
                if (sscanf(q, "MITEM %31[^,], %7[^,], %d", s, ks, &en) != 3) die("tenantfx.asm", "bad MITEM line");
                char key = (ks[0] == '\'') ? ks[1] : 0;
                char line[200];
                snprintf(line, sizeof line, "%s>%s>%c>%s", t, asm_string(src, s), key ? key : '-', en ? "on" : "off");
                int dup = 0;
                for (int k = 0; k < nseen; k++) if (!strcmp(seen[k], line)) dup = 1;
                if (!dup) {
                    if (nseen == 64) die("tenantfx.asm", "too many items");
                    strcpy(seen[nseen++], line);
                    item_line("TENANTFX", t, asm_string(src, s), key, en ? "on" : "off");
                }
                q = strchr(q, '\n');
                if (!q) die("tenantfx.asm", "truncated");
            }
            p = strchr(p, '\n');
            if (!p) break;
        }
    }
    free(src);
}

/* ---- the installed question the kernel asks FAT, asked of FLAIR_FIXTURES */
static int g_nfix;
static char **g_fix;
static int installed(void *user, const char *dir83, const char *file83)
{
    char want[64];
    (void)user;
    snprintf(want, sizeof want, "%s/%s", dir83, file83);
    for (int i = 0; i < g_nfix; i++) if (!strcmp(g_fix[i], want)) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 4) die("usage", "flair_inventory <kmain.i> <tenantfx.asm> <fixture>...");
    char *k = slurp(argv[1]);
    g_fix = argv + 3;
    g_nfix = argc - 3;

    /* SYSTEM: every title of the top bar drops the one shared list. */
    {
        const char *p = find(k, "fd_sys_titles[4] = {", "kmain.i");
        int n = 0;
        for (;;) {
            char title[32];
            p = skipws(p);
            if (*p == '}') break;
            p = cstring(p, title, sizeof title, "fd_sys_titles");
            MenuInfo mi = { (int16_t)(128 + n), title, SHELL_SYS_MENU_ITEMS, SHELL_SYS_MENU_ITEMS_N, 0 };
            walk_menu("SYSTEM", &mi, -1, NULL, 0);
            n++;
            p = skipws(p);
            if (*p == ',') p++;
        }
        if (n != 4) die("fd_sys_titles", "expected four titles");
    }

    /* APPLE: About This Computer... + every installed accessory. */
    {
        const MenuInfo *mi = apple_menu_build(installed, NULL);
        static char apple[] = "Apple";
        MenuInfo copy = *mi;
        copy.title = apple;
        walk_menu("APPLE", &copy, -1, NULL, 0);
    }

    /* FINDER: the bar, with the predicate-driven rows marked cond. */
    {
        MenuBar *bar = finder_menu_bar();
        for (int m = 0; m < (int)bar->n_menus; m++)
            walk_menu("FINDER", &bar->menus[m], m, FINDER_MENU_PREDS, (int)FINDER_MENU_PREDS_N);
    }

    /* HELLO: the eight canon titles, each dropping kmain's item pair. */
    {
        static const char *const titles[] = {
            FLAIR_CANON_MENU_ITEM_0, FLAIR_CANON_MENU_ITEM_1, FLAIR_CANON_MENU_ITEM_2,
            FLAIR_CANON_MENU_ITEM_3, FLAIR_CANON_MENU_ITEM_4, FLAIR_CANON_MENU_ITEM_5,
            FLAIR_CANON_MENU_ITEM_6, FLAIR_CANON_MENU_ITEM_7 };
        MenuItem pair[2];
        static char store[2][64];
        const char *p = find(k, "it[0] = (MenuItem)", "kmain.i HELLO item 0");
        menu_item(p, &pair[0], store[0], "HELLO item 0");
        p = find(k, "it[1] = (MenuItem)", "kmain.i HELLO item 1");
        menu_item(p, &pair[1], store[1], "HELLO item 1");
        for (int m = 0; m < FLAIR_CANON_PHOTOSHOP_MENU_COUNT; m++) {
            MenuInfo mi = { (int16_t)m, titles[m], pair, 2, 0 };
            walk_menu("HELLO", &mi, -1, NULL, 0);
        }
    }

    /* NOTES: its three-menu fixture bar. */
    {
        static const char *const arrays[3] = {
            "flair_ten_notes_file_items", "flair_ten_notes_edit_items", "flair_ten_notes_notes_items" };
        const char *p = find(k, "MenuInfo flair_ten_notes_menus[3] = {", "kmain.i NOTES menus");
        for (int m = 0; m < 3; m++) {
            char title[32];
            MenuItem items[16];
            static char store[16][64];
            const char *q = strchr(p, '"');
            if (!q) die("NOTES", "menu title");
            p = cstring(q, title, sizeof title, "NOTES title");
            const char *arr = skipws(p);
            if (*arr == ',') arr = skipws(arr + 1);
            if (strncmp(arr, arrays[m], strlen(arrays[m]))) die("NOTES", "menus[] no longer names the item arrays in order");
            int n = item_array(k, arrays[m], items, store, 16);
            MenuInfo mi = { (int16_t)m, title, items, (uint16_t)n, 0 };
            walk_menu("NOTES", &mi, -1, NULL, 0);
        }
    }

    tenantfx(argv[2]);

    /* The INITECH volume. */
    for (int i = 0; i < g_nfix; i++) {
        char path[96];
        size_t n = strlen(g_fix[i]);
        if (n >= sizeof path) die("fixture", g_fix[i]);
        for (size_t j = 0; j <= n; j++) path[j] = g_fix[i][j] == '/' ? '\\' : g_fix[i][j];
        printf("%s|file\n", path);
    }
    free(k);
    return ferror(stdout) ? 1 : 0;
}
