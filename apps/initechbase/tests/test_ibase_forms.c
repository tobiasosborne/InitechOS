/* Actual supported command prefixes against independent real III+ screens.
 * Paint starts from a different captured page: CLEAR must erase old content.
 * No editor/GET/READ scenario is executed or claimed here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_assert.h"
#include "screen_file.h"
#include "w2_host.h"
#include "ibase/screen.h"
TEST_HARNESS();
struct pal_host_cfg {uint8_t yy,mm,dd;uint32_t heap;};
extern ibase_pal_t *pal_host_make(struct pal_host_cfg);
extern void pal_host_free(ibase_pal_t *);
static screen_file page;
static ibase_screen model;
static void draw(void *u,uint8_t r,uint8_t c,uint8_t a,const char *s,uint32_t n)
{
    (void)u;
    if(r>=25 || c+n>80)screen_die("product paint exceeded page");
    for(uint32_t i=0;i<n;i++){page.bytes[2*(r*80+c+i)]=(uint8_t)s[i];page.bytes[2*(r*80+c+i)+1]=a;}
}
static int paint(ibase_pal_t *p,uint8_t r,uint8_t c,const char *s,uint32_t n)
{
    (void)p;
    if(!n){if(r>25 || c>=80)return -PAL_EIO;model.row=r;model.col=c;return 0;}
    if(r>=25 || c+n>80)return -PAL_EIO;
    memcpy(model.text[r]+c,s,n);draw(NULL,r,c,model.attr,s,n);return 0;
}
static int say(ibase_pal_t *p,uint8_t r,uint8_t c,const char *s,uint32_t n)
{(void)p;ibase_screen_at(&model,r,c,s,n);return 0;}
static void cursor(ibase_pal_t *p,uint8_t r,uint8_t c)
{(void)p;model.row=r;model.col=c;}
static void output(ibase_pal_t *p,const char *s,uint32_t n)
{(void)p;ibase_screen_write(&model,s,n);}
static void copy(const char *from,const char *to)
{
    FILE *a=screen_open(from,"rb"),*b=screen_open(to,"wb");int c;
    while((c=fgetc(a))!=EOF)if(fputc(c,b)==EOF)screen_die("fixture copy failed");
    fclose(a);fclose(b);
}
int main(void)
{
    static const struct{const char *dir,*name,*prompt_capture;int cleared;} cases[]={
        {"at-to-a","TA",NULL,0},{"at-to-a","TB",NULL,0},{"at-to-a","TD",NULL,0},
        {"at-to-b","TE",NULL,0},{"at-large","LA",NULL,0},{"at-large","LB",NULL,0},
        {"at-clear-d","DD",NULL,0},{"at-clear-d","DC",NULL,0},
        {"at-dot-c","NFILL","e1",0},{"at-dot-c","CF1","e4",1},
        {"prompt-tail","TAIL","tail",1}
    };
    char root[2048];if(!getcwd(root,sizeof root))return 2;
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
        char temp[]="build/forms-target.XXXXXX",work[4096],path[8192],line[512];FILE *in,*out;
        screen_file expected;struct w2_host_env env;
        const char *base=!strcmp(cases[i].dir,"prompt-tail")?"tests":"tests/xbase_prog_diff/screen-mint";
        ibase_pal_t *pal=pal_host_make((struct pal_host_cfg){86,7,24,2*1024*1024});
        if(!pal || !mkdtemp(temp))return 2;
        snprintf(work,sizeof work,"%s/%s",root,temp);
        snprintf(path,sizeof path,"%s/MINT.PRG",work);out=screen_open(path,"wb");
        fputs("SET TALK OFF\nSET SAFETY OFF\nSET STATUS OFF\n",out);
        snprintf(path,sizeof path,"%s/%s/SETUP/%s.PRG",base,cases[i].dir,cases[i].name);
        in=screen_open(path,"rb");
        while(fgets(line,sizeof line,in) && strncmp(line,"DO WHILE INKEY()",16))fputs(line,out);
        fclose(in);fclose(out);
        if(!strcmp(cases[i].dir,"at-clear-d")){
            snprintf(path,sizeof path,"%s/CLIENTS.DBF",work);
            copy("tests/xbase_prog_diff/screen-mint/at-clear-d/SETUP/CLIENTS.DBF",path);
        }
        ibase_screen_init(&model,draw,NULL);
        screen_read("tests/xbase_prog_diff/screen-mint/at-to-a/TB.screen",&page);
        for(unsigned cell=0;cell<2000;cell++)model.text[cell/80][cell%80]=(char)page.bytes[2*cell];
        struct w2_host_config cfg={work,work,16*1024*1024,16*1024*1024,'C'};
        CHECK(w2_host_bind(&env,pal,&cfg)==0,"DOS-aware actual command binding");
        pal->paint=paint;pal->say_screen=say;pal->gotoxy=cursor;pal->conout=output;
        xb_interp *ip=xb_interp_make(pal);CHECK(ip!=NULL,"forms interpreter constructed");
        if(!ip)return 2;
        int rc=w2_host_startup(pal,ip,"MINT.PRG");
        if(rc)fprintf(stderr,"RED forms prefix %s/%s rc=%d error=%d\n",cases[i].dir,cases[i].name,rc,ibase_last_error(ip));
        CHECK(rc==0,"captured supported forms prefix completes");
        if(cases[i].prompt_capture){
            ibase_screen_prompt(&model,1,cases[i].cleared);
            ibase_screen_write(&model,". ",2);
            page.bytes[4000]=model.row;page.bytes[4001]=model.col;
        }else{
            page.bytes[4000]=(uint8_t)xb_interp_ctx(ip)->screen_row;
            page.bytes[4001]=(uint8_t)xb_interp_ctx(ip)->screen_col;
        }
        snprintf(path,sizeof path,"%s/%s/%s.screen",base,cases[i].dir,
                 cases[i].prompt_capture?cases[i].prompt_capture:cases[i].name);
        screen_read(path,&expected);
        if(memcmp(page.bytes,expected.bytes,4002))fprintf(stderr,"RED actual forms screen %s/%s\n",cases[i].dir,cases[i].name);
        CHECK(!memcmp(page.bytes,expected.bytes,4002),"all target characters, attributes and cursor equal real III+");
        snprintf(path,sizeof path,"%s/actual.screen",work);screen_write(path,&page);
        xb_interp_free(ip);w2_host_unbind(&env);pal_host_free(pal);
    }
    return TEST_SUMMARY("test-ibase-forms");
}
