/* 0qw1 execution-state oracle. MS-DOS 3.3 Reference PAUSE p.149 / PDF234;
 * independent DOS 3.30 nested CALL capture: Y cancels every frame; N resumes
 * CHILD_AFTER and PARENT_AFTER exactly once. Input is key bytes, not lines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define COMMAND_BATCH_HOST
#include "command.c"
#include "batch.c"
#include "env.c"
static int run_pipeline_or_redirect(const char *s)
{
    if(dispatch_control(s))return 0;
    if(!strcmp(s,"BRKCHILD < INPUT.TXT > OUT.TXT")) {
        redirected=1;run_external("BRKCHILD","");redirected=0;return 0;
    }
    if(!strcmp(s,"ECHO LOOPING"))loop_count++;
    dos_puts_raw(s);dos_puts_raw("\r\n");return 0;
}
static unsigned count(const char *s)
{unsigned n=0;const char *p=output;while((p=strstr(p,s))){n++;p+=strlen(s);}return n;}
static void check(int v,const char *s)
{if(!v){fprintf(stderr,"RED batch-break: %s\n",s);exit(1);}}
static void reference(const char *leaf,int continued)
{
    char path[128],text[4096];
    snprintf(path,sizeof path,"harness/diff/fat_diff/batch_break_mint/%s",leaf);
    FILE *f=fopen(path,"rb");check(f!=0,"independent DOS 3.30 mint missing");
    size_t n=fread(text,1,sizeof text-1,f);fclose(f);text[n]=0;
    check(strstr(text,"Terminate batch job (Y/N)?")!=0,"mint question bytes");
    if(!strncmp(leaf,"exec-",5)) {
        check((strstr(text,"EXEC_AFTER")!=0)==continued,"mint EXEC child next-line decision");
    } else {
        check((strstr(text,"CHILD_AFTER")!=0)==continued,"mint child frame cancellation");
        check((strstr(text,"PARENT_AFTER")!=0)==continued,"mint parent frame cancellation");
    }
}
static void run(const char *main,const char *child,const char *parent,const char *keys,unsigned n)
{
    memset(output,0,sizeof output);output_n=input_pos=loop_count=poll_count=exec_count=0;
    redirected=child_cleaned=child_status=0;
    input=keys;input_count=n;files[0]=main;files[1]=child;files[2]=parent;
    g_shell_break.interrupted=0;g_shell_exit=0;env_init(&g_master_env);
    const char *av[]={"MAIN.BAT"};run_batch("MAIN.BAT",av,1);
    check(g_batch_depth==0 && !g_batch_frame,"CALL frames restored");
}
int main(void)
{
    reference("nested-y.txt",0);reference("nested-n.txt",1);reference("invalid-x.txt",0);
    reference("exec-y.txt",0);reference("exec-n.txt",1);
    const char *pause="@ECHO OFF\r\nPAUSE\r\nECHO AFTER-PAUSE\r\n";
    run(pause,0,0,"\003n",2);
    check(count("Terminate batch job (Y/N)?")==1,"PAUSE asks catalogue question");
    check(count("ECHO AFTER-PAUSE")==1,"N continues exactly once");
    check(!g_shell_break.interrupted,"N clears resident recovery latch");
    run(pause,0,0,"\003y",2);
    check(!count("AFTER-PAUSE"),"Y terminates before post-question sentinel");
    check(g_shell_break.interrupted,"Y protects enclosing CALL handle cleanup");
    run(pause,0,0,"\003xn",3);
    check(count("Terminate batch job (Y/N)?")==2,"invalid answer reprompts");
    check(count("ECHO AFTER-PAUSE")==1,"invalid then N continues once");
    run(pause,0,0,"\003\003n",3);
    check(count("Terminate batch job (Y/N)?")==2 && count("ECHO AFTER-PAUSE")==1,
          "second Ctrl-C at question reprompts without losing resume state");
    const char *child="@ECHO OFF\r\nPAUSE\r\nECHO CHILD_AFTER\r\n";
    const char *parent="@ECHO OFF\r\nCALL CHILD\r\nECHO PARENT_AFTER\r\n";
    const char *main="@ECHO OFF\r\nCALL PARENT\r\nECHO MAIN_AFTER\r\n";
    run(main,child,parent,"\003Y",2);
    check(count("Terminate batch job (Y/N)?")==1,"nested Y asks once");
    check(!count("_AFTER"),"nested Y cancels every active batch frame");
    run(main,child,parent,"\003N",2);
    check(count("ECHO CHILD_AFTER")==1 && count("ECHO PARENT_AFTER")==1 && count("ECHO MAIN_AFTER")==1,"nested N resumes each frame once");
    /* One physical loop iteration then N, then Y. No PAUSE/CON read in loop. */
    run("@ECHO OFF\r\n:again\r\nECHO LOOPING\r\nGOTO again\r\n",0,0,"ny",2);
    check(poll_count && count("Terminate batch job (Y/N)?")==2,"LOOP boundary detects Ctrl-C, N then Y");
    check(loop_count==1,"loop resumes at saved next physical line");
    /* REVIEW blocking 1: actual run_external must preserve child's break
     * status until run_batch's next boundary, after EXEC and redirection. */
    const char *external="@ECHO OFF\r\nBRKCHILD < INPUT.TXT > OUT.TXT\r\nECHO EXEC_AFTER\r\n";
    run(external,0,0,"\003n",2);
    check(exec_count==1 && child_cleaned,"EXEC child terminates and cleans up");
    check(count("Terminate batch job (Y/N)?")==1,"EXEC child asks after cleanup");
    check(count("ECHO EXEC_AFTER")==1 && input_pos==2,"EXEC child N continues next line exactly once");
    run(external,0,0,"\003y",2);
    check(count("Terminate batch job (Y/N)?")==1,"EXEC child Y asks once");
    check(!count("EXEC_AFTER") && input_pos==2,"EXEC child Y cancels next line");
    exec_status=0x0001u;run(external,0,0,"",0);
    check(!count("Terminate batch job") && count("ECHO EXEC_AFTER")==1 && g_errorlevel==1,
          "normal nonzero exit is ERRORLEVEL, not a break");
    exec_status=0x0300u;run(external,0,0,"",0);
    check(!count("Terminate batch job") && count("ECHO EXEC_AFTER")==1,
          "TSR termination is not a break");
    exec_status=0x0100u;
    output_n=input_pos=0;output[0]=0;input="\003";input_count=1;
    g_shell_break.interrupted=0;run_external("BRKCHILD","");
    check(!g_batch_depth && !count("Terminate batch job") && input_pos==1,
          "interactive EXEC child never asks a batch-only question");
    puts("GREEN batch-break: PAUSE Y/N, invalid key, LOOP, nested CALL, EXEC child Y/N after cleanup");return 0;
}
