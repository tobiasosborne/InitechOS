/* Execute the actual flat WRITE.COM instructions on the host CPU; intercept
 * INT21 with a signal adapter. Independent literal file bytes and four faults
 * grade the probe, not the filesystem. CLAUDE.md, The oracle is the truth.
 * This instruction subset (including balanced PUSH/POP) also runs in long
 * mode; the probe never returns, and siglongjmp restores the host ABI.
 */
#define _GNU_SOURCE
#include <signal.h>
#include <setjmp.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static sigjmp_buf escaped;
static volatile sig_atomic_t mode, exit_code, pos, wrote, read_phase;
static char output[512]; static unsigned used;
static void interrupt(int sig,siginfo_t *info,void *ctx)
{
    (void)sig;(void)info;
    greg_t *r=((ucontext_t *)ctx)->uc_mcontext.gregs;
    uint8_t *ip=(uint8_t *)(uintptr_t)r[REG_RIP];
    if(ip[0]!=0xcd || ip[1]!=0x21) { exit_code=99;siglongjmp(escaped,1); }
    unsigned ah=((unsigned)r[REG_RAX]>>8)&255;
    unsigned n=(unsigned)r[REG_RCX]; uint8_t *p=(uint8_t *)(uintptr_t)r[REG_RDX];
    r[REG_EFL]&=~1;
    switch(ah) {
    case 0x3c:r[REG_RAX]=5;break;
    case 0x3d:read_phase=1;r[REG_RAX]=5;break;
    case 0x40:
        if(r[REG_RBX]==5) wrote=n==7 && !memcmp(p,"hello\r\n",7);
        r[REG_RAX]=n;break;
    case 0x3e:
        if(read_phase && mode==4){r[REG_EFL]|=1;r[REG_RAX]=5;}
        break;
    case 0x3f:
        if(mode==1){r[REG_EFL]|=1;r[REG_RAX]=5;break;}
        if(mode==2)n=0;
        else { if(n>7u-(unsigned)pos)n=7u-(unsigned)pos; memcpy(p,"hello\r\n"+pos,n);if(mode==3&&n)p[0]='X';pos+=(int)n; }
        r[REG_RAX]=n;break;
    case 0x09:
        while(*p!='$' && used+1<sizeof output)output[used++]=(char)*p++;
        output[used]=0;break;
    case 0x4c:exit_code=(unsigned)r[REG_RAX]&255;siglongjmp(escaped,1);
    default:exit_code=98;siglongjmp(escaped,1);
    }
    r[REG_RIP]+=2;
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    void *mem=mmap((void *)0x40000,65536,PROT_READ|PROT_WRITE|PROT_EXEC,
                   MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(mem==MAP_FAILED){perror("probe mmap");return 2;}
    uint8_t binary[8192];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t size=fread(binary,1,sizeof binary,f);fclose(f);if(!size||size==sizeof binary)return 2;
    struct sigaction sa={0};sa.sa_sigaction=interrupt;sa.sa_flags=SA_SIGINFO;
    sigaction(SIGSEGV,&sa,0);
    unsigned failed=0;
    for(mode=0;mode<5;mode++) {
        memcpy((uint8_t *)mem+0x100,binary,size);pos=wrote=read_phase=0;used=0;output[0]=0;exit_code=97;
        if(!sigsetjmp(escaped,1))((void (*)(void))((uint8_t *)mem+0x100))();
        int ok=wrote && (mode ? exit_code==1 && !strstr(output,"WRITE-READBACK-OK")
                                            : exit_code==0 && strstr(output,"WRITE-READBACK-OK"));
        printf("%s WRITE probe fault=%d exit=%d persisted=%d output=%s\n",ok?"GREEN":"RED",mode,exit_code,wrote,output);
        if(!ok)failed++;
    }
    return failed?1:0;
}
