/* Test-only register bridge. Control syntax and iteration stay in command.c;
 * OPEN and FINDFIRST/NEXT use the actual INT21/FAT backend. */
#include "shell_files_host.h"
static env_store_t g_master_env;
static int g_shell_exit;
static uint8_t g_errorlevel;
static int run_pipeline_or_redirect(const char *);
static void cwd_display(char *p) { strcpy(p,"A:\\"); }
static void dos_getcwd(uint8_t d,char *p) { (void)d; *p=0; }
static char prompt_drive(void) { return 'A'; }
static uint8_t dos_getdrive(void) { return 0; }
static void dos_gettime(uint8_t *h,uint8_t *m,uint8_t *s,uint8_t *c)
{ *h=*m=*s=*c=0; }
static void dos_getdate(uint16_t *y,uint8_t *m,uint8_t *d,uint8_t *w)
{ *y=1987; *m=*d=1; *w=4; }
static uint8_t dos_conin(void) { fputs("unexpected CON input\n",stderr);exit(2); }
static uint16_t dos_exec(const char *p,const char *t) { (void)p;(void)t;return 2; }
static uint16_t dos_get_child_status(void) { return 0; }
static void del_path(char *p,const char *s,int n,const char *leaf)
{ memcpy(p,s,(unsigned)n);strcpy(p+n,leaf); }
