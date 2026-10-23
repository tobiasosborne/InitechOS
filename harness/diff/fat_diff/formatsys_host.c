/* Factory adapter only; same freestanding core as the shipped COMs.
 * Reference images and answers come from mtools/dosfstools, not this code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diskutil.h"
#include "block_device.h"
static FILE *disks[2];
static const char *input,*source,*target,*retry;
static unsigned swaps;
static int sector(unsigned d,uint32_t l,void *p,int write) {
 if(d<128||d>129)return -1;
 FILE *f=disks[d-128];if(fseek(f,(long)l*512,SEEK_SET))return -1;
 size_t n=write?fwrite(p,512,1,f):fread(p,512,1,f);fflush(f);return n==1?0:-1;
}
static int rd(void *c,uint32_t l,uint32_t n,void *p) {
 for(unsigned i=0;i<n;i++)if(sector((unsigned)(uintptr_t)c,l+i,(char *)p+i*512,0))return -1;
 return 0;
}
static int wr(void *c,uint32_t l,uint32_t n,const void *p) {
 for(unsigned i=0;i<n;i++)if(sector((unsigned)(uintptr_t)c,l+i,(char *)p+i*512,1))return -1;
 return 0;
}
static int map(unsigned l,du_map *m){return block_device_map(l,m);}
static int get(void){
 int c=*input?(unsigned char)*input++:'N';
 /* Factory medium replacement at the source prompt, before ENTER's retry. */
 if(c=='\n'&&retry){fclose(disks[0]);disks[0]=fopen(retry,"r+b");if(!disks[0])exit(2);retry=0;}
 return c;
}
static void put(const char *p){
 for(const char *q=p;*q;q++)if(*q!='\r')putchar(*q);
 /* Real one-drive swap: close the source medium and open a different image.
  * Never substitute a second-drive copy for the manual's A: -> A: flow. */
 if(source && (strstr(p,"Insert SOURCE")||strstr(p,"Insert FIRST")||
               strstr(p,"Insert TARGET")||strstr(p,"Insert SECOND"))) {
  const char *path=(strstr(p,"TARGET")||strstr(p,"SECOND"))?target:source;
  fclose(disks[1]);disks[1]=fopen(path,"r+b");if(!disks[1])exit(2);swaps++;
 }
}
int main(int argc,char **argv){
 if(argc!=7 && argc!=9)return 2;
 disks[0]=fopen(argv[1],"r+b");disks[1]=fopen(argv[2],"r+b");
 if(!disks[0]||!disks[1])return 2;
 for(unsigned i=0;i<2;i++) {
  fseek(disks[i],0,SEEK_END);long bytes=ftell(disks[i]);
  blockdev_t dev={(void *)(uintptr_t)(128+i),rd,wr};block_device_bind(i,&dev,(uint32_t)(bytes/512));
 }
 /* Factory-only seam: exercise formatting removable A while current=C.
  * Shipped current drive remains A; no application mapping is duplicated. */
 if(getenv("DU_CURRENT_C"))block_device_mounted(128,63,2880,0);
 else block_device_mounted(129,0,2880,0);
 input=argv[5];retry=getenv("DU_RETRY_SOURCE");if(argc==9){source=argv[7];target=argv[8];}
 const du_io io={map,sector,get,put};int rc=diskutil_run(&io,(unsigned)atoi(argv[3]),argv[4]);
 fclose(disks[0]);fclose(disks[1]);
 fprintf(stderr,"diskutil exit=%d swaps=%u\n",rc,swaps);
 return rc==atoi(argv[6])?0:1;
}
