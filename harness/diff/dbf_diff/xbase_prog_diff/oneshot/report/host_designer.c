/* Host integration driver: replays the same KEYS through the real shared kit.
 * Factory-only code; never linked into SAMIR.EXE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "samir/interp.h"
#include "samir/rt.h"
#include "ui_apps.h"
struct pal_host_cfg {uint8_t yy,mm,dd;uint32_t heap_size;};
samir_pal_t *pal_host_make(struct pal_host_cfg);
void pal_host_free(samir_pal_t *);
static int input[16384],used,pos;
static char cells[25][80];
static char dump[32][128];static int ndump;
static void quiet(samir_pal_t *p,const char *s,uint32_t n)
{(void)p;fwrite(s,1,n,stdout);}
static int paint(samir_pal_t *p,uint8_t row,uint8_t col,const char *s,uint32_t n)
{
    (void)p;for(uint32_t i=0;i<n && row<25 && col+i<80;i++)cells[row][col+i]=s[i];return 0;
}
static int32_t key(samir_pal_t *p)
{
    (void)p;
    while(pos<used) {
        int k=input[pos++];
        if(k>=0x1000) {
            FILE *out=fopen(dump[k-0x1000],"wb");if(!out)exit(2);
            for(int row=0;row<25;row++){fwrite(cells[row],1,80,out);fputc('\n',out);}fclose(out);
        } else return k;
    }
    return -1;
}
static int32_t line(samir_pal_t *p,char *buf,uint32_t cap)
{
    uint32_t n=0;int k;
    while((k=key(p))>=0) {
        if(k=='\r'){buf[n]=0;return (int32_t)n;}
        if(k>=32 && k<127 && n+1<cap)buf[n++]=(char)k;
    }
    return -1;
}
static void add(int k){if(used==16384)exit(2);input[used++]=k;}
static void load(const char *path)
{
    FILE *f=fopen(path,"r");char b[1024];if(!f)exit(2);
    while(fgets(b,sizeof b,f)) {
        b[strcspn(b,"\r\n")]=0;
        if(b[0]=='T'){for(int n=1;b[n];n++)add((unsigned char)b[n]);}
        else if(b[0]=='D') {
            if(ndump==32)exit(2);
            snprintf(dump[ndump],128,"%s.host.txt",b+1);add(0x1000+ndump++);
        } else if(b[0]=='k') {
            const char *s=b+1;int code=-1;
            if(!strcmp(s,"ret"))code=13;
            else if(!strcmp(s,"esc"))code=27;
            else if(!strcmp(s,"tab"))code=9;
            else if(!strcmp(s,"up"))code=0x148;
            else if(!strcmp(s,"down"))code=0x150;
            else if(!strcmp(s,"left"))code=0x14B;
            else if(!strcmp(s,"right"))code=0x14D;
            else if(!strcmp(s,"pgup"))code=0x149;
            else if(!strcmp(s,"pgdn"))code=0x151;
            else if(!strcmp(s,"ctrl-end"))code=0x175;
            else if(!strcmp(s,"shift-9"))code='(';
            else if(!strcmp(s,"shift-0"))code=')';
            else if(!strcmp(s,"comma"))code=',';
            else if(!strcmp(s,"f10"))code=0x144;
            else if(!strncmp(s,"ctrl-",5) && strlen(s)==6)code=s[5]-'a'+1;
            else if(strlen(s)==1)code=s[0];
            if(code<0){fprintf(stderr,"unknown key %s\n",s);exit(2);}add(code);
        }
    }
    fclose(f);
}
static int hook(void *u,xb_interp *ip,const char *verb,const char *args,int *ec)
{
    int modify=verb[0]=='M',n;(void)u;
    if(strcmp(verb,"CREATE") && strcmp(verb,"MODIFY"))return CMD_UNKNOWN;
    if((n=rt_kw_match(args,"REPORT")))return ui_create_report(ip,modify,args+n,ec);
    if((n=rt_kw_match(args,"LABEL")))return ui_create_label(ip,modify,args+n,ec);
    return CMD_UNKNOWN;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    load(argv[1]);memset(cells,' ',sizeof cells);
    samir_pal_t *p=pal_host_make((struct pal_host_cfg){86,7,24,1024*1024});
    if(!p)return 2;
    p->conin_char=key;p->conin_line=line;p->conout=quiet;p->paint=paint;
    xb_interp *ip=xb_interp_make(p);if(!ip)return 2;
    xb_interp_add_cmd_hook(ip,hook,0);
    int rc=samir_repl(p,ip),err=samir_last_error(ip);
    xb_interp_free(ip);pal_host_free(p);
    if(rc || err){fprintf(stderr,"repl=%d last_error=%d\n",rc,err);return 1;}
    return 0;
}
