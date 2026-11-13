/* W2 factory input parity: mounts/free space and true batch startup.
 * Ref: PRD 6.6; review-w2mint.keep.md B2; mint_capture.sh DOSBox inputs. */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test_assert.h"
#include "w2_host.h"
TEST_HARNESS();
struct pal_host_cfg { uint8_t yy,mm,dd; uint32_t heap; };
extern samir_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(samir_pal_t *);
static unsigned probes;
static void quiet(samir_pal_t *p,const char *s,uint32_t n) { (void)p;(void)s;(void)n; }
static int probe(void *user,xb_interp *ip,const char *verb,const char *args,int *ec)
{
    (void)user;(void)args;(void)ec;
    if(strcmp(verb,"ENVPROBE"))return CMD_UNKNOWN;
    CHECK(xb_interp_ctx(ip)->dot_command==0,"startup and nested DO execute as batch commands");
    probes++;return CMD_OK;
}
static void directory(const char *path)
{ int rc=mkdir(path,0700);CHECK(rc==0||errno==EEXIST,"factory fixture directory exists"); }
static void write_text(const char *path,const char *text)
{
    FILE *f=fopen(path,"wb");CHECK(f!=NULL,"factory fixture opens");
    if(f){CHECK(fwrite(text,1,strlen(text),f)==strlen(text),"factory fixture writes");fclose(f);}
}
static void contents(samir_pal_t *pal,const char *name,const char *want)
{
    char buf[256]={0};pal_fd fd=pal->open(pal,name,PAL_RD);
    CHECK(fd>=0,"DOS fixture path opens through mounted PAL");
    if(fd>=0){int32_t n=pal->read(pal,fd,buf,sizeof buf-1);
        CHECK(n==(int32_t)strlen(want)&&!memcmp(buf,want,strlen(want)),"mounted path reads correct drive/case fixture");
        CHECK(pal->close(pal,fd)==0,"mounted file closes");}
}
int main(void)
{
    char cwd[2048],root[4096],drive[4096],path[8192],buf[2048]={0};
    uint32_t bytes=0;struct w2_host_env env;xb_interp *ip;
    samir_pal_t *pal=pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
    if(!pal||!getcwd(cwd,sizeof cwd))return 2;
    snprintf(root,sizeof root,"%s/build/w2-host-adapter",cwd);directory(root);
    snprintf(drive,sizeof drive,"%s/build/w2-host-adapter/DRIVE",cwd);directory(drive);
    snprintf(path,sizeof path,"%s/Sub",root);directory(path);
    snprintf(path,sizeof path,"%s/BASE.DBF",root);write_text(path,"C fixture");
    snprintf(path,sizeof path,"%s/BASE.DBF",drive);write_text(path,"D fixture");
    snprintf(path,sizeof path,"%s/Sub/MiXeD.TXT",root);write_text(path,"mixed-case fixture");
    CHECK(pal->diskspace(pal,&bytes)==0&&bytes==1048576u,"unadapted FNS PAL retains 1 MiB fixture");
    pal_fd (*original_open)(samir_pal_t *,const char *,int)=pal->open;
    const struct w2_host_config cfg={root,drive,67108864u,33554432u,'C'};
    CHECK(w2_host_bind(&env,pal,&cfg)==0,"explicit W2 mount/free-byte inputs bind");
    CHECK(pal->curdrive(pal)=='C',"W2 logged drive is explicit C");
    CHECK(pal->diskspace(pal,&bytes)==0&&bytes==67108864u,"C free-space input is 64 MiB");
    contents(pal,"C:\\base.dbf","C fixture");contents(pal,"d:BASE.dbf","D fixture");
    contents(pal,"c:/sub/MIXED.txt","mixed-case fixture");
    env.logged_drive='D';
    CHECK(pal->curdrive(pal)=='D'&&pal->diskspace(pal,&bytes)==0&&bytes==33554432u,"D free-space input is independently configurable");
    contents(pal,"BASE.DBF","D fixture");env.logged_drive='C';
    CHECK(pal->open(pal,"Q:\\BASE.DBF",PAL_RD)<0,"unmounted DOS drive fails");
    CHECK(pal->open(pal,"C:\\..\\outside",PAL_RD)<0,"factory mount traversal fails");
    pal_fd fd=pal->open(pal,"D:\\NEW.TXT",PAL_CREATE|PAL_RDWR|PAL_TRUNC);
    CHECK(fd>=0,"create maps into D fixture mount");
    if(fd>=0){CHECK(pal->write(pal,fd,"new",3)==3,"mounted write delegates");pal->close(pal,fd);}
    CHECK(pal->rename(pal,"d:new.txt","D:RENAMED.TXT")==0,"DOS rename maps both names");
    contents(pal,"d:renamed.txt","new");
    CHECK(pal->remove(pal,"D:renamed.txt")==0,"DOS remove maps case-insensitively");
    snprintf(path,sizeof path,"%s/MINT.PRG",root);
    write_text(path,"SET TALK OFF\nSET SAFETY OFF\nSET ALTERNATE TO C:\\BATCH.OUT\nSET ALTERNATE ON\nENVPROBE\n? DISKSPACE()\n? FILE(\"D:\\BASE.DBF\")\nDO CHILD\nSET ALTERNATE TO\nQUIT\n");
    snprintf(path,sizeof path,"%s/CHILD.PRG",root);write_text(path,"ENVPROBE\n? \"CHILD BATCH\"\nRETURN\n");
    pal->conout=quiet;ip=xb_interp_make(pal);
    CHECK(ip!=NULL,"startup interpreter exists");
    if(ip){
        CHECK(xb_interp_add_cmd_hook(ip,probe,NULL)==INTERP_OK,"batch observer hook installs");
        CHECK(w2_host_startup(pal,ip,"MINT.PRG")==INTERP_OK,"disk MINT.PRG runs to QUIT as startup program");
        CHECK(probes==2&&env.input_calls==0,"startup and nested program consume zero REPL commands");
        xb_interp_free(ip);
    }
    snprintf(path,sizeof path,"%s/BATCH.OUT",root);FILE *f=fopen(path,"rb");
    CHECK(f!=NULL,"batch alternate output is on mounted C");
    if(f){fread(buf,1,sizeof buf-1,f);fclose(f);}
    CHECK(strstr(buf,"67108864")&&strstr(buf,".T.")&&strstr(buf,"CHILD BATCH"),"engine observes injected disk space, D mount and nested batch output");
    w2_host_unbind(&env);
    CHECK(pal->open==original_open&&pal->diskspace(pal,&bytes)==0&&bytes==1048576u,"detach restores unchanged FNS PAL callbacks and inputs");
    pal_host_free(pal);return TEST_SUMMARY("test-samir-w2-host-adapter");
}
