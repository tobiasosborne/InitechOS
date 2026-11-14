/* Writable factory adapter. mtools constructs/reads the filesystem; every
 * sector write is counted even if it would merely rewrite identical bytes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dosutil.h"
#include "chkdsk.h"
#include "fdisk.h"
#include "fat_policy.h"
static FILE *disk;
static unsigned writes,base;
static const char *keys;
static int readsec(void *ctx,uint32_t l,uint32_t n,void *p)
{ (void)ctx;return fseek(disk,(long)(base+l)*512,SEEK_SET)||fread(p,512,n,disk)!=n?-1:0; }
static int writesec(void *ctx,uint32_t l,uint32_t n,const void *p)
{ (void)ctx;writes+=n;return fseek(disk,(long)(base+l)*512,SEEK_SET)||fwrite(p,512,n,disk)!=n||fflush(disk)?-1:0; }
int du_sector(int wr,unsigned drive,unsigned l,uint8_t b[512])
{ return drive?-1:wr?writesec(0,l,1,b):readsec(0,l,1,b); }
int du_drive(void){return 0;}
int du_cwd(unsigned d,char b[64]){b[0]=0;return d?-1:0;}
int du_list(const char *p,unsigned n,du_entry *e){(void)p;(void)n;(void)e;return 0;}
int du_attr(const char *p,int set,uint8_t *a){(void)p;(void)set;(void)a;return -1;}
int du_line(char b[128]){b[0]=0;return 0;}
void du_puts(const char *p){fputs(p,stdout);}
void du_con_puts(const char *p){fputs(p,stderr);}
static void put(void *ctx,const char *p){(void)ctx;du_puts(p);}
static int get(void){return *keys?(unsigned char)*keys++:27;}
static int chget(void *ctx){(void)ctx;return 'N';}
static int rd(unsigned d,uint8_t b[512]){(void)d;return readsec(0,0,1,b);}
static int wr(unsigned d,const uint8_t b[512]){(void)d;return writesec(0,0,1,b);}
static uint32_t cap(unsigned d){return d==128?20000:0;}
static char letter(unsigned d,unsigned p){(void)d;return p==1?'D':0;}
static char protect(unsigned d,unsigned p){
 (void)d;if(p!=1)return 0;
 uint8_t b[512];unsigned save=base;base=63;int rc=readsec(0,0,1,b);base=save;
 return !rc&&dos_fat16(b)?'D':0;
}
int main(int argc,char **argv){
 if(argc!=4 || !(disk=fopen(argv[1],"r+b")))return 2;
 keys=argv[3];int rc;
 if(!strcmp(argv[2],"label"))rc=label_main(argv[3]);
 else if(!strcmp(argv[2],"chkdsk")){
  static uint8_t memory[4096];chkdsk_io_t io={0};
  io.disk=(blockdev_t){0,readsec,writesec};io.put=put;io.get=chget;io.cwd=du_cwd;
  mcb_init(&io.arena,memory,sizeof memory/16);
  rc=chkdsk_run(&io,argv[3]);
 }else{
  fdisk_io io={rd,wr,cap,get,du_puts,129,0,letter,protect};
  rc=fdisk_run(&io);
 }
 fclose(disk);fprintf(stderr,"result=%d writes=%u\n",rc,writes);return 0;
}
