/* Independent DOS 3.30 reference capture: HMP + physical VGA text memory.
 * No InitechOS headers or implementation are linked into this mint. */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
static FILE *monitor;
static const char *output;
static unsigned number;
static pid_t emulator;
static void stop(void)
{
    if (emulator>0) { kill(emulator,SIGTERM); waitpid(emulator,NULL,0); }
}
static void command(const char *text)
{
    fprintf(monitor,"%s\n",text);fflush(monitor);usleep(180000);
}
static int screen(const char *expect)
{
    char path[512],cmd[600],text[2001];unsigned char bytes[4000];
    snprintf(path,sizeof path,"%s/vga.bin",output);
    snprintf(cmd,sizeof cmd,"pmemsave 0xb8000 4000 \"%s\"",path);command(cmd);
    FILE *f=fopen(path,"rb");if(!f)return 0;
    size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);if(n!=sizeof bytes)return 0;
    for(unsigned i=0;i<2000;i++)text[i]=bytes[i*2]?(char)bytes[i*2]:' ';
    text[2000]=0;if(expect && !strstr(text,expect))return 0;
    snprintf(path,sizeof path,"%s/screen-%02u.txt",output,number++);
    f=fopen(path,"w");if(!f)exit(2);
    for(unsigned i=0;i<25;i++)fprintf(f,"%.*s\n",80,text+i*80);
    fclose(f);return 1;
}
static void wait_screen(const char *text)
{
    for(unsigned i=0;i<90;i++){if(screen(text))return;usleep(100000);}
    fprintf(stderr,"reference screen timeout: %s\n",text);exit(2);
}
int main(int argc,char **argv)
{
    if(argc!=4 || !strchr("ynx",argv[3][0]) || argv[3][1])return 2;
    output=argv[1];int channel[2];if(pipe(channel))return 2;
    emulator=fork();if(emulator<0)return 2;
    if(!emulator){
        char path[512],disk[512];dup2(channel[0],0);close(channel[0]);close(channel[1]);
        snprintf(path,sizeof path,"%s/qemu.log",output);
        FILE *log=fopen(path,"w");if(!log)_exit(2);
        dup2(fileno(log),1);dup2(fileno(log),2);
        snprintf(disk,sizeof disk,"file=%s,format=raw,if=floppy,index=0",argv[2]);
        execlp("qemu-system-i386","qemu-system-i386","-display","none",
            "-monitor","stdio","-serial","none","-cpu","486","-m","16",
            "-no-reboot","-rtc","base=1992-03-02T09:30:00","-drive",disk,"-boot","a",NULL);
        _exit(2);
    }
    atexit(stop);close(channel[0]);monitor=fdopen(channel[1],"w");if(!monitor)return 2;
    wait_screen("Strike a key");command("sendkey ctrl-c");
    wait_screen("Terminate batch job (Y/N)?");
    char answer[16];snprintf(answer,sizeof answer,"sendkey %c",argv[3][0]);command(answer);
    usleep(700000);screen(NULL);command("quit");waitpid(emulator,NULL,0);emulator=0;
    fclose(monitor);return 0;
}
