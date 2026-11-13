/* initech-r7gf execute-completely-or-refuse-before-side-effects contract.
 * GOLDEN-MASTER item 2; Reference manual ch4-batch.tex sequential execution.
 * The bounded refusal is operator-authorized, not a historical size limit.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COMMAND_BATCH_HOST
#include "command.c"
#include "batch.c"
#include "env.c"
static int old_intact, after;
static const char *grown;
static int run_pipeline_or_redirect(const char *s)
{
    if(dispatch_control(s)) return 0;
    if(!strcmp(s,"DEL OLD")) old_intact=0;
    if(!strcmp(s,"ECHO END")) after++;
    if(!strcmp(s,"GROW")) files[0]=grown;
    dos_puts_raw(s); dos_puts_raw("\r\n"); return 0;
}
static void check(int ok,const char *s)
{ if(!ok){fprintf(stderr,"RED batch-capacity: %s\n",s);exit(1);} }
static void run(const char *src,const char *child)
{
    memset(output,0,sizeof output); output_n=0; old_intact=1; after=0;
    loop_count=poll_count=exec_count=0; input_pos=input_count=0;
    g_shell_exit=0; g_shell_break.interrupted=0;
    env_init(&g_master_env); files[0]=src; files[1]=child; files[2]=0;
    const char *av[]={"MAIN.BAT"}; run_batch("MAIN.BAT",av,1);
    check(!g_batch_depth && !g_batch_frame,"frames restored");
}
static void script(char *p,unsigned n)
{
    const char *first="@ECHO OFF\r\nDEL OLD\r\nGOTO END\r\n";
    const char *last=":END\r\nECHO END\r\n";
    unsigned pos=(unsigned)strlen(first), tail=n-(unsigned)strlen(last);
    memcpy(p,first,pos);
    while(tail-pos>=6){memcpy(p+pos,"REM \r\n",6);pos+=6;}
    while(pos<tail)p[pos++]='\n';
    strcpy(p+tail,last);
}
int main(void)
{
    char src[5002];
    for(unsigned n=4095;n<=4097;n++) {
        script(src,n); run(src,0);
        if(n<=4096) check(!old_intact && after==1,"4095/4096 execute through final label and sentinel");
        else check(old_intact && !after && strstr(output,"Insufficient memory"),
                   "4097 refuses visibly before DEL OLD");
    }
    /* A physical command split by the old cap must never execute its prefix. */
    script(src,5000); memcpy(src+4090,"ECHO SPLIT",10); run(src,0);
    check(old_intact && !after && strstr(output,"Insufficient memory"),"large split script refuses before mutation");
    grown=src;
    run("@ECHO OFF\r\nCALL CHILD\r\nDEL OLD\r\nECHO END\r\n","@ECHO OFF\r\nGROW\r\n");
    check(old_intact && !after && strstr(output,"Insufficient memory"),"CALL reload uses same visible all-or-nothing refusal");
    puts("GREEN batch-capacity: 4095/4096 complete, 4097/5000 and CALL reload refuse before mutation");
    return 0;
}
