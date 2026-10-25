/* W2 factory adapter: DOSBox C:/D: fixture mounts, explicit free bytes and
 * batch startup. Ref: PRD 6.6; pal.h file/curdrive/diskspace contracts;
 * mint_capture.sh autoexec. This never implements SET DEFAULT or SET PATH.
 *
 * Include the existing session wiring to reuse its private registration and
 * disk loader without changing the shipped API (same factory pattern as
 * harness/pubs/samir_inventory.c). W2 links this TU in place of samir_main.c;
 * FNS still links the original TU and original pal_host without adapters. */
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "w2_host.h"
#include "../../../os/samir/samir_main.c"

static struct w2_host_env *active;
static char upper(char c) { return c>='a'&&c<='z'?(char)(c-'a'+'A'):c; }
static int same_name(const char *a,const char *b)
{
    while(*a&&*b&&upper(*a)==upper(*b)){a++;b++;}
    return !*a&&!*b;
}
static int drive_index(char c) { c=upper(c);return c=='C'?0:c=='D'?1:-1; }
/* Resolve DOS separators and case within the selected fixture mount. New
 * filenames retain their requested spelling; later opens are case-insensitive. */
static int host_path(const char *name,char *out,size_t cap)
{
    const char *s=name;
    int drive=drive_index(active->logged_drive);
    size_t used;
    if(!name||!*name)return -PAL_ENOENT;
    if(name[1]==':'){drive=drive_index(name[0]);s+=2;}
    if(drive<0)return -PAL_ENOENT;
    used=strlen(active->roots[drive]);
    if(used>=cap)return -PAL_EACCES;
    memcpy(out,active->roots[drive],used+1);
    while(*s){
        char part[256];size_t n=0;DIR *dir;struct dirent *entry;
        while(*s=='/'||*s=='\\')s++;
        if(!*s)break;
        while(*s&&*s!='/'&&*s!='\\'){
            if(*s==':'||n+1>=sizeof part)return -PAL_EACCES;
            part[n++]=*s++;
        }
        part[n]=0;
        if(!strcmp(part,"."))continue;
        if(!strcmp(part,".."))return -PAL_EACCES; /* fixtures cannot escape mounts */
        dir=opendir(out);
        if(dir){
            while((entry=readdir(dir))!=NULL)if(same_name(part,entry->d_name)){
                n=strlen(entry->d_name);
                if(n>=sizeof part){closedir(dir);return -PAL_EACCES;}
                memcpy(part,entry->d_name,n+1);break;
            }
            closedir(dir);
        }
        if(used+1+n>=cap)return -PAL_EACCES;
        out[used++]='/';memcpy(out+used,part,n+1);used+=n;
    }
    return PAL_OK;
}
static pal_fd mounted_open(samir_pal_t *pal,const char *name,int mode)
{
    char path[W2_HOST_PATH_CAP];int rc=host_path(name,path,sizeof path);
    return rc?rc:active->open(pal,path,mode);
}
static int mounted_remove(samir_pal_t *pal,const char *name)
{
    char path[W2_HOST_PATH_CAP];int rc=host_path(name,path,sizeof path);
    return rc?rc:active->remove(pal,path);
}
static int mounted_rename(samir_pal_t *pal,const char *from,const char *to)
{
    char a[W2_HOST_PATH_CAP],b[W2_HOST_PATH_CAP];
    int rc=host_path(from,a,sizeof a);if(rc)return rc;
    rc=host_path(to,b,sizeof b);return rc?rc:active->rename(pal,a,b);
}
static int mounted_drive(samir_pal_t *pal) { (void)pal;return active->logged_drive; }
static int mounted_space(samir_pal_t *pal,uint32_t *bytes)
{
    int drive=drive_index(active->logged_drive);(void)pal;
    if(drive<0||!bytes)return -PAL_ENOENT;
    *bytes=active->free_bytes[drive];return PAL_OK;
}
static int32_t no_typed_commands(samir_pal_t *pal,char *buf,uint32_t cap)
{
    (void)pal;if(cap)buf[0]=0;active->input_calls++;return -1;
}
int w2_host_bind(struct w2_host_env *env,samir_pal_t *pal,const struct w2_host_config *cfg)
{
    struct stat st;
    if(active||!env||!pal||!cfg||!cfg->c_root||!cfg->d_root||
       cfg->c_root[0]!='/'||cfg->d_root[0]!='/'||drive_index(cfg->logged_drive)<0||
       strlen(cfg->c_root)>=sizeof env->roots[0]||strlen(cfg->d_root)>=sizeof env->roots[1]||
       stat(cfg->c_root,&st)||!S_ISDIR(st.st_mode)||stat(cfg->d_root,&st)||!S_ISDIR(st.st_mode))return -PAL_EACCES;
    memset(env,0,sizeof *env);env->pal=pal;
    strcpy(env->roots[0],cfg->c_root);strcpy(env->roots[1],cfg->d_root);
    env->free_bytes[0]=cfg->c_free_bytes;env->free_bytes[1]=cfg->d_free_bytes;
    env->logged_drive=upper(cfg->logged_drive);
    env->open=pal->open;env->remove=pal->remove;env->rename=pal->rename;
    env->curdrive=pal->curdrive;env->diskspace=pal->diskspace;
    env->conin_line=pal->conin_line;env->conin_line_noecho=pal->conin_line_noecho;
    active=env;pal->open=mounted_open;pal->remove=mounted_remove;pal->rename=mounted_rename;
    pal->curdrive=mounted_drive;pal->diskspace=mounted_space;
    pal->conin_line=no_typed_commands;pal->conin_line_noecho=no_typed_commands;
    return PAL_OK;
}
void w2_host_unbind(struct w2_host_env *env)
{
    if(active!=env)return;
    env->pal->open=env->open;env->pal->remove=env->remove;env->pal->rename=env->rename;
    env->pal->curdrive=env->curdrive;env->pal->diskspace=env->diskspace;
    env->pal->conin_line=env->conin_line;env->pal->conin_line_noecho=env->conin_line_noecho;
    active=NULL;
}
/* The existing session disk loader delivers DO-file inputs to the same proc
 * engine in batch mode. Do not enter repl_loop or its typed-command context.
 * This factory bridge supplies MINT.PRG's FNSTEST and nested fixture programs;
 * it does not implement any W2 SET/verb or replace the procedure fallback. */
static int batch_disk_input(void *user,xb_interp *ip,const char *verb,const char *args,int *ec)
{
    char line[REPL_LINE_CAP];int rc=INTERP_OK,handled;
    uint8_t dot=xb_interp_ctx(ip)->dot_command;(void)user;
    if(!repl_verb_eq(verb,"DO",NULL))return CMD_UNKNOWN;
    if(snprintf(line,sizeof line,"DO %s",args)>=(int)sizeof line){
        if(ec)*ec=10;
        return -INTERP_ERR_SYNTAX;
    }
    handled=repl_try_do_file(ip,line,&rc,ec);
    xb_interp_ctx(ip)->dot_command=dot;
    if(!handled)return CMD_UNKNOWN;
    if(ec&&*ec)return -INTERP_ERR_UNKNOWN_CMD;
    return rc;
}
int w2_host_startup(samir_pal_t *pal,xb_interp *ip,const char *file)
{
    uint32_t len=0;int ec=0,rc;char *source;
    if(!pal||!ip||active==NULL||active->pal!=pal)return -INTERP_ERR_NOMEM;
    rc=xb_interp_add_cmd_hook(ip,batch_disk_input,NULL);if(rc)return rc;
    rc=repl_register_modules(ip);if(rc)return rc;
    xb_interp_ctx(ip)->message=samir_message;
    xb_interp_ctx(ip)->dot_command=0;
    proc_set_suspend(ip,repl_suspend);
    source=repl_load_prg(ip,file,&len,&ec);
    if(!source)return ec?-ec:-INTERP_ERR_NOMEM;
    rc=proc_run_program(ip,source,"MINT");
    xb_interp_ctx(ip)->set_status=0;
    set_close_alternate(ip);samir_status_refresh(ip);wa_close_all(xb_interp_env(ip));
    return rc==INTERP_RC_QUIT?INTERP_OK:rc;
}
