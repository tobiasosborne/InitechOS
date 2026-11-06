/* Optional isolated BROWSE smoke build while other experiment UIs are in flight.
 * Never used by the normal SAMIRX build. */
#include "ui_apps.h"
int ui_edit(xb_interp *ip, int mode, const char *args, int *ec)
{ (void)ip; (void)mode; (void)args; (void)ec; return CMD_UNKNOWN; }
int ui_create(xb_interp *ip, int modify, const char *args, int *ec)
{ (void)ip; (void)modify; (void)args; (void)ec; return CMD_UNKNOWN; }
int ui_create_report(xb_interp *ip, int modify, const char *args, int *ec)
{ (void)ip; (void)modify; (void)args; (void)ec; return CMD_UNKNOWN; }
int ui_create_label(xb_interp *ip, int modify, const char *args, int *ec)
{ (void)ip; (void)modify; (void)args; (void)ec; return CMD_UNKNOWN; }
int ui_modify_command(xb_interp *ip, const char *args, int *ec)
{ (void)ip; (void)args; (void)ec; return CMD_UNKNOWN; }
int ui_assist(xb_interp *ip, const char *args, int *ec)
{ (void)ip; (void)args; (void)ec; return CMD_UNKNOWN; }
int ui_help(xb_interp *ip, const char *args, int *ec)
{ (void)ip; (void)args; (void)ec; return CMD_UNKNOWN; }
