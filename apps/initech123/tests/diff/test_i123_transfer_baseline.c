/* Independent FORMULA body oracle for existing R2.2 mint sheets. These are
 * self-transfer baselines, not claimed to be command-minted Copy/Move goldens.
 * initech-uqah. Factory C; python only runs the existing independent reader. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "i123.h"
static uint8_t mem[2][131072],in[65536],out[65536];
static const char *const names[]={"CIRC01","MINT01","MINT02","MINT03","MINT04","MINT05"};
static int oracle(const char *corpus,const char *reader,const char *file,const char *result)
{
    pid_t pid=fork();if(pid<0)return 1;
    if(!pid) {
        int fd=open(result,O_WRONLY|O_CREAT|O_TRUNC,0600);if(fd<0)_exit(2);
        if(dup2(fd,1)<0)_exit(2);
        close(fd);if(setenv("LOTUS123_DECOMP",corpus,1))_exit(2);
        execlp("python3","python3",reader,"--bytes",file,(char *)0);_exit(2);
    }
    int status=0;if(waitpid(pid,&status,0)<0)return 1;
    return !WIFEXITED(status)||WEXITSTATUS(status);
}
static int equal(const char *a,const char *b)
{
    FILE *x=fopen(a,"rb"),*y=fopen(b,"rb");if(!x||!y)return 0;
    int ca,cb;do {ca=fgetc(x);cb=fgetc(y);}while(ca==cb&&ca!=EOF);
    int nonempty=ftell(x)>0;
    fclose(x);fclose(y);return ca==cb&&nonempty;
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    for(unsigned f=0;f<6;f++) {
        char path[1024],saved[1024],a[1024],b[1024];
        snprintf(path,sizeof path,"%s/goldens/minted/%s.WK1",argv[1],names[f]);
        FILE *fp=fopen(path,"rb");if(!fp){perror("mint golden");return 2;}
        uint32_t n=(uint32_t)fread(in,1,sizeof in,fp);fclose(fp);
        i123_sheet_t src,dst;
        if(i123_sheet_init(&src,mem[0],sizeof mem[0],256)||i123_sheet_init(&dst,mem[1],sizeof mem[1],256)||i123_wk1_read(&src,in,n))return 2;
        src.protec=0; /* allow self-transfer of MINT04; compare only FORMULA bodies */
        snprintf(a,sizeof a,"%s/i123-transfer-ref.txt",argv[2]);
        snprintf(b,sizeof b,"%s/i123-transfer-out.txt",argv[2]);
        snprintf(saved,sizeof saved,"%s/i123-transfer-self.WK1",argv[2]);
        if(oracle(argv[1],argv[3],path,a))return 2;
        i123_rect_t rg={0,0,(uint16_t)src.range[2],(uint16_t)src.range[3]};
        for(int mode=0;mode<2;mode++) {
            int rc=i123_transfer(&src,&dst,rg,0,0,mode);
            if(!rc)rc=i123_wk1_write(&dst,out,sizeof out,&n);
            if(rc){fprintf(stderr,"baseline %s mode %d refused %d\n",names[f],mode,rc);return 1;}
            fp=fopen(saved,"wb");if(!fp)return 2;
            if(fwrite(out,1,n,fp)!=n)return 2;
            fclose(fp);
            if(oracle(argv[1],argv[3],saved,b))return 2;
            if(!equal(a,b)){fprintf(stderr,"RED-REASON baseline %s %s FORMULA body\n",names[f],mode?"Move":"Copy");return 1;}
        }
    }
    puts("i123-transfer-baseline: 12/12 self-transfers match real cached values and FORMULA bytes via wk1_formula_ref.py");return 0;
}
