/* ui.c -- W4 full-screen command dispatch (EXPERIMENT initech-7cbu8).
 * Built ONLY into SAMIRX.EXE (-DSAMIR_W4). One cmd hook, registered right
 * after the work-area hook so bare APPEND / EDIT reach the full-screen editor
 * before the mutate module; any verb form this layer does not own returns
 * CMD_UNKNOWN and falls through to the shipped engine unchanged.
 */
#include "samir/interp.h"
#include "samir/rt.h"
#include "ui_apps.h"

static int ui_word(const char *v, const char *kw)
{   /* III+ 4-letter abbreviation: v is a prefix (>=4 chars) of kw, or equal */
    int n = 0;
    while (v[n] && kw[n] && v[n] == kw[n]) n++;
    if (v[n]) return 0;
    return kw[n] == 0 || n >= 4;
}
static const char *ui_skip(const char *s) { while (*s == ' ') s++; return s; }
/* first word of args upper-cased into w (cap 16); returns rest */
static const char *ui_next(const char *s, char *w)
{
    int n = 0;
    s = ui_skip(s);
    while (*s && *s != ' ' && n < 15) { char c = *s++; if (c >= 'a' && c <= 'z') c -= 32; w[n++] = c; }
    while (*s && *s != ' ') s++;
    w[n] = 0;
    return ui_skip(s);
}

/* SAMIRX variants link only some apps (Makefile SAMIRX_UI -> -DUI_HAVE_<file>);
 * a verb whose app is not linked falls through (CMD_UNKNOWN). */
#ifdef UI_HAVE_browse
#define C_ui_browse(...) ui_browse(__VA_ARGS__)
#else
#define C_ui_browse(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_edit
#define C_ui_edit(...) ui_edit(__VA_ARGS__)
#else
#define C_ui_edit(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_assist
#define C_ui_assist(...) ui_assist(__VA_ARGS__)
#else
#define C_ui_assist(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_help
#define C_ui_help(...) ui_help(__VA_ARGS__)
#else
#define C_ui_help(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_create
#define C_ui_create(...) ui_create(__VA_ARGS__)
#else
#define C_ui_create(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_modcmd
#define C_ui_modify_command(...) ui_modify_command(__VA_ARGS__)
#else
#define C_ui_modify_command(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_frmdesign
#define C_ui_create_report(...) ui_create_report(__VA_ARGS__)
#else
#define C_ui_create_report(...) CMD_UNKNOWN
#endif
#ifdef UI_HAVE_lbldesign
#define C_ui_create_label(...) ui_create_label(__VA_ARGS__)
#else
#define C_ui_create_label(...) CMD_UNKNOWN
#endif
static int ui_hook(void *user, xb_interp *ip, const char *verb, const char *args, int *ec)
{
    char w[16];
    const char *rest;
    (void)user;
    *ec = 0;
    if (ui_word(verb, "BROWSE")) return C_ui_browse(ip, args, ec);
    if (ui_word(verb, "EDIT") || ui_word(verb, "CHANGE")) return C_ui_edit(ip, UI_EDIT_EDIT, args, ec);
    if (ui_word(verb, "APPEND") && !*ui_skip(args)) return C_ui_edit(ip, UI_EDIT_APPEND, args, ec);
    if (ui_word(verb, "INSERT") && !*ui_skip(args)) return C_ui_edit(ip, UI_EDIT_INSERT, args, ec);
    if (ui_word(verb, "ASSIST")) return C_ui_assist(ip, args, ec);
    if (ui_word(verb, "HELP")) return C_ui_help(ip, args, ec);
    if (ui_word(verb, "CREATE") || ui_word(verb, "MODIFY")) {
        int modify = verb[0] == 'M';
        rest = ui_next(args, w);
        (void)rest;
        if (ui_word(w, "STRUCTURE")) return modify ? C_ui_create(ip, 1, rest, ec) : CMD_UNKNOWN;
        if (ui_word(w, "COMMAND") || ui_word(w, "FILE")) return C_ui_modify_command(ip, rest, ec);
        if (ui_word(w, "REPORT")) return C_ui_create_report(ip, modify, rest, ec);
        if (ui_word(w, "LABEL")) return C_ui_create_label(ip, modify, rest, ec);
        if (ui_word(w, "QUERY") || ui_word(w, "SCREEN") || ui_word(w, "VIEW")) return CMD_UNKNOWN;
        if (!modify) return C_ui_create(ip, 0, args, ec);   /* CREATE <file> */
        return CMD_UNKNOWN;
    }
    return CMD_UNKNOWN;
}

int ui_register(xb_interp *ip);
int ui_register(xb_interp *ip)
{
    return xb_interp_add_cmd_hook(ip, ui_hook, (void *)0);
}
