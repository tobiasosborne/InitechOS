/* Independent real-software capture via QEMU HMP; no product code linked.
 * Pattern: dos/command/tests/batch_break_mint/mint.c. */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <poll.h>
#include <time.h>
static FILE *mon, *transcript; static int monitor_fd; static const char *out; static pid_t pid;
static void cleanup(void) { if(pid>0){kill(pid,SIGTERM);waitpid(pid,NULL,0);} }
static long long milliseconds(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))exit(2);return (long long)t.tv_sec*1000+t.tv_nsec/1000000;}
/* HMP emits its next prompt only after pmemsave/screendump completes. Keep
 * keyboard pacing separate from this bounded command-completion check. */
static void completed(int quitting){
 const char prompt[]="(qemu) ";size_t matched=0;long long deadline=milliseconds()+5000;
 for(;;){
  long long left=deadline-milliseconds();struct pollfd p={.fd=monitor_fd,.events=POLLIN};
  if(left<=0 || poll(&p,1,(int)left)<=0){fprintf(stderr,"monitor completion timed out\n");exit(2);}
  char c;ssize_t n=read(monitor_fd,&c,1);
  if(n==0 && quitting){fflush(transcript);return;}
  if(n!=1){fprintf(stderr,"monitor closed before command completion\n");exit(2);}
  fputc(c,transcript);
  if(c==prompt[matched])matched++;else matched=c==prompt[0]?1:0;
  if(matched==sizeof prompt-1){fflush(transcript);return;}
 }
}
static void cmd(const char *s){fprintf(mon,"%s\n",s);fflush(mon);completed(!strcmp(s,"quit"));usleep(65000);}
static void snap(const char *name,int picture){
 char p[1024],c[1200]; unsigned char raw[4000];
 snprintf(p,sizeof p,"%s/%s.vga",out,name);snprintf(c,sizeof c,"pmemsave 0xb8000 4000 \"%s\"",p);cmd(c);
 FILE *f=fopen(p,"rb");if(!f || fread(raw,1,4000,f)!=4000){fprintf(stderr,"VGA read failed\n");exit(2);}fclose(f);
 snprintf(p,sizeof p,"%s/%s.txt",out,name);f=fopen(p,"wb");if(!f)exit(2);
 for(int i=0;i<2000;i++){fputc(raw[i*2]?raw[i*2]:' ',f);if(i%80==79)fputc('\n',f);}fclose(f);
 if(picture){snprintf(c,sizeof c,"screendump \"%s/%s.ppm\"",out,name);cmd(c);}
}
static void type(const char *s){
 for(;*s;s++){char k[64],ch=*s; const char *q=NULL;
 if(ch>='A'&&ch<='Z')snprintf(k,sizeof k,"shift-%c",ch+32);
 else if((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9'))snprintf(k,sizeof k,"%c",ch);
 else {switch(ch){case ' ':q="spc";break;case '/':q="slash";break;case '\\':q="backslash";break;case ':':q="shift-semicolon";break;case '.':q="dot";break;case '-':q="minus";break;case '_':q="shift-minus";break;case '=':q="equal";break;case '!':q="shift-1";break;case '\'':q="apostrophe";break;case '+':q="shift-equal";break;case '?':q="shift-slash";break;case '(':q="shift-9";break;case ')':q="shift-0";break;case '*':q="shift-8";break;default:fprintf(stderr,"unsupported char %c\n",ch);exit(2);}snprintf(k,sizeof k,"%s",q);}
 char c[96];snprintf(c,sizeof c,"sendkey %s 30",k);cmd(c);
 }
}
int main(int argc,char **argv){
 if(argc!=6){fprintf(stderr,"drive OUT BOOT BDISK-or-- HDD-or-- STEPS\n");return 2;}out=argv[1];
 int p[2],q[2];if(pipe(p)||pipe(q))return 2;pid=fork();if(pid<0)return 2;
 if(!pid){setpriority(PRIO_PROCESS,0,15);dup2(p[0],0);close(p[0]);close(p[1]);char a[1024],b[1024],h[1024];
 dup2(q[1],1);dup2(q[1],2);close(q[0]);close(q[1]);
 snprintf(a,sizeof a,"file=%s,format=raw,if=floppy,index=0",argv[2]);snprintf(b,sizeof b,"file=%s,format=raw,if=floppy,index=1",argv[3]);snprintf(h,sizeof h,"file=%s,format=raw,if=ide,index=0",argv[4]);
 const char *v[40]={"qemu-system-i386","-display","none","-monitor","stdio","-serial","none","-cpu","486","-m","8","-no-reboot","-rtc","base=1992-03-02T09:30:00,clock=vm","-drive",a,"-boot","a","-nic","none",NULL};int n=20;
 if(strcmp(argv[3],"-")){v[n++]="-drive";v[n++]=b;}if(strcmp(argv[4],"-")){v[n++]="-drive";v[n++]=h;}v[n]=NULL;execvp(v[0],(char *const *)v);_exit(2);
 }
 atexit(cleanup);close(p[0]);mon=fdopen(p[1],"w");if(!mon)return 2;
 close(q[1]);monitor_fd=q[0];char log[1024];snprintf(log,sizeof log,"%s/qemu.log",out);
 transcript=fopen(log,"w");if(!transcript)return 2;completed(0);
 FILE *steps=fopen(argv[5],"r");if(!steps)return 2;char *line=NULL;size_t cap=0;
 while(getline(&line,&cap,steps)>0){line[strcspn(line,"\r\n")]=0;char *arg=strchr(line,' ');if(arg)*arg++=0;else arg="";
 if(!strcmp(line,"sleep"))usleep((useconds_t)(atof(arg)*1000000));
 else if(!strcmp(line,"type"))type(arg);
 else if(!strcmp(line,"key")){char c[256];snprintf(c,sizeof c,"sendkey %s 50",arg);cmd(c);usleep(250000);}
 else if(!strcmp(line,"shot"))snap(arg,1);
 else if(!strcmp(line,"hmp"))cmd(arg);
 else if(!strcmp(line,"wait")){int ok=0;for(int i=0;i<80;i++){snap("wait",0);char pth[1024],txt[2026];snprintf(pth,sizeof pth,"%s/wait.txt",out);FILE*f=fopen(pth,"r");size_t n=fread(txt,1,sizeof txt-1,f);txt[n]=0;fclose(f);if(strstr(txt,arg)){ok=1;break;}usleep(150000);}if(!ok){fprintf(stderr,"wait failed: %s\n",arg);return 2;}}
 else if(!strcmp(line,"end"))break;
 else if(*line && *line!='#'){fprintf(stderr,"unknown step: %s\n",line);return 2;}
 }
 fclose(steps);free(line);cmd("quit");waitpid(pid,NULL,0);pid=0;fclose(mon);close(monitor_fd);fclose(transcript);return 0;
}
