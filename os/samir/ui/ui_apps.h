/* ui_apps.h -- W4 app entry points (EXPERIMENT initech-7cbu8). Each returns a
 * cmd_status (CMD_OK / CMD_UNKNOWN) or a negative interp error with *ec set to
 * the dBASE message ordinal. args = operands after the verb (trimmed). */
#ifndef SAMIR_UI_APPS_H
#define SAMIR_UI_APPS_H
#include "samir/interp.h"
enum { UI_EDIT_EDIT = 0, UI_EDIT_APPEND = 1, UI_EDIT_INSERT = 2 };
int ui_browse(xb_interp *ip, const char *args, int *ec);           /* ui/browse.c */
int ui_edit(xb_interp *ip, int mode, const char *args, int *ec);   /* ui/edit.c */
int ui_assist(xb_interp *ip, const char *args, int *ec);           /* ui/assist.c */
int ui_help(xb_interp *ip, const char *args, int *ec);             /* ui/help.c */
int ui_create(xb_interp *ip, int modify, const char *args, int *ec); /* ui/create.c: CREATE <f> / MODIFY STRUCTURE */
int ui_modify_command(xb_interp *ip, const char *args, int *ec);   /* ui/modcmd.c */
int ui_modify_file(xb_interp *ip, const char *args, int *ec);      /* ui/modcmd.c: no default ext */
int ui_create_report(xb_interp *ip, int modify, const char *args, int *ec); /* ui/frmdesign.c */
int ui_create_label(xb_interp *ip, int modify, const char *args, int *ec);  /* ui/lbldesign.c */
#endif
