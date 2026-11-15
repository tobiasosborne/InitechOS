/* External QMP observer for the unchanged FILE COPY pixel oracle. QMP's
 * stop command freezes a running CPU at an arbitrary instruction: drawn=1
 * alone is insufficient while cursor_erase is restoring the save-under.
 * Accept only an ordinary idle yield with the complete cursor visible. */
#define _POSIX_C_SOURCE 200809L
#include <elf.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "capture_state.h"
static int fd, stopped;
static unsigned sequence;
static char reply[32768];
static long long now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (long long)t.tv_sec*1000+t.tv_nsec/1000000;}
static int send_command(const char *verb,const char *args)
{
    char command[4096],id[40];snprintf(id,sizeof id,"copy-capture-%u",++sequence);
    int n=snprintf(command,sizeof command,"{\"execute\":\"%s\",\"arguments\":{%s},\"id\":\"%s\"}\n",verb,args,id);
    if(n<0 || (size_t)n>=sizeof command)return -1;
    for(int off=0;off<n;){ssize_t w=write(fd,command+off,(size_t)(n-off));if(w<0&&errno==EINTR)continue;if(w<=0)return -1;off+=(int)w;}
    long long end=now()+2000;size_t used=0;
    while(now()<end){struct pollfd p={fd,POLLIN,0};int r=poll(&p,1,(int)(end-now()));if(r<0&&errno==EINTR)continue;if(r<=0)return -1;
        char c;if(read(fd,&c,1)!=1)return -1;
        if(used+1>=sizeof reply)return -1;
        reply[used++]=c;if(c!='\n')continue;reply[used]=0;
        if(strstr(reply,id))return strstr(reply,"\"error\"")?-1:0;
        used=0;
    }return -1;
}
static int quoted(char *out,size_t cap,const char *in)
{
    size_t j=0;for(size_t i=0;in[i];i++){unsigned char c=(unsigned char)in[i];if(c<32)return -1;if(c=='"'||c=='\\'){if(j+1>=cap)return -1;out[j++]='\\';}if(j+1>=cap)return -1;out[j++]=(char)c;}out[j]=0;return 0;
}
static int symbol(FILE *f,const char *name,uint32_t *address,uint32_t *size)
{
    Elf32_Ehdr eh;if(fseek(f,0,SEEK_SET)||fread(&eh,sizeof eh,1,f)!=1||memcmp(eh.e_ident,ELFMAG,SELFMAG)||eh.e_ident[EI_CLASS]!=ELFCLASS32||eh.e_ident[EI_DATA]!=ELFDATA2LSB||eh.e_shentsize!=sizeof(Elf32_Shdr))return -1;
    for(unsigned i=0;i<eh.e_shnum;i++){
        Elf32_Shdr sh,strings;
        if(fseek(f,(long)(eh.e_shoff+i*sizeof sh),SEEK_SET)||fread(&sh,sizeof sh,1,f)!=1)return -1;
        if(sh.sh_type!=SHT_SYMTAB)continue;
        if(sh.sh_link>=eh.e_shnum||sh.sh_entsize!=sizeof(Elf32_Sym)||fseek(f,(long)(eh.e_shoff+sh.sh_link*sizeof strings),SEEK_SET)||fread(&strings,sizeof strings,1,f)!=1)return -1;
        for(unsigned j=0;j<sh.sh_size/sizeof(Elf32_Sym);j++){
            Elf32_Sym s;char text[128];
            if(fseek(f,(long)(sh.sh_offset+j*sizeof s),SEEK_SET)||fread(&s,sizeof s,1,f)!=1)return -1;
            if(s.st_name>=strings.sh_size)continue;
            if(fseek(f,(long)(strings.sh_offset+s.st_name),SEEK_SET))return -1;
            size_t k=0;int c;while(k+1<sizeof text&&(c=fgetc(f))!=EOF&&c)text[k++]=(char)c;text[k]=0;
            if(!strcmp(text,name)){*address=s.st_value;*size=s.st_size;return 0;}
        }
    }return -1;
}
static int memory(uint32_t address,size_t size,const char *path,void *out)
{
    char q[2048],args[2300];if(quoted(q,sizeof q,path))return -1;
    snprintf(args,sizeof args,"\"val\":%u,\"size\":%zu,\"filename\":\"%s\"",address,size,q);
    if(send_command("pmemsave",args))return -1;
    FILE *f=fopen(path,"rb");if(!f)return -1;int ok=fread(out,size,1,f)==1;fclose(f);unlink(path);return ok?0:-1;
}
int main(int argc,char **argv)
{
    uint32_t cursor,idle,size,cursor_size;int rc=2;
    if(argc!=5||strcmp(argv[1],"--fd"))return 2;
    fd=atoi(argv[2]);FILE *elf=fopen(argv[3],"rb");if(!elf)return 2;
    int bad=symbol(elf,"g_cursor",&cursor,&cursor_size)||symbol(elf,"flair_live_yield",&idle,&size);
    fclose(elf);if(bad||!size||cursor_size!=sizeof(copy_cursor_state))return 2;
    char mem[2048],q[2048],args[2300];if(snprintf(mem,sizeof mem,"%s.memory",argv[4])>=(int)sizeof mem||quoted(q,sizeof q,argv[4]))return 2;
    int copying=strstr(argv[4],"/copy.ppm.screen")!=NULL;
    long long end=now()+5000;unsigned attempts=0;
    for(;;){
        if(now()>=end){rc=1;break;}
        copy_cursor_state state;uint32_t pc;
        stopped=1;
        if(send_command("stop",""))break;
        attempts++;
        if(send_command("human-monitor-command","\"command-line\":\"info registers\""))break;
        char *eip=strstr(reply,"EIP=");if(!eip||sscanf(eip+4,"%x",&pc)!=1)break;
        if(memory(cursor,sizeof state,mem,&state))break;
        if(copy_capture_ready(pc,idle,size,&state)){
            snprintf(args,sizeof args,"\"filename\":\"%s\"",q);
            if(send_command("screendump",args))break;
            fprintf(stderr,"FILECOPY stable capture: idle yield, cursor visible, copy=%d (%u samples)\n",copying,attempts);
            rc=0;break;
        }
        if(send_command("cont",""))break;
        stopped=0;
        struct timespec delay={0,1000000};nanosleep(&delay,NULL);
    }
    if(stopped&&send_command("cont",""))rc=2;
    unlink(mem);
    if(rc)fprintf(stderr,"FILECOPY stable capture failed: no complete cursor at idle yield (%u samples)\n",attempts);
    return rc;
}
