/* Independent on-disk oracle; no artifact code linked. IBM TR pp5-4..11;
 * SYS MS-DOS UG p119. Input reference minted only by mtools. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned u16(const unsigned char *p){return p[0]+256u*p[1];}
static void need(int x,const char *p){if(!x){fprintf(stderr,"RED formatsys %s\n",p);exit(1);}}
int main(int n,char **v){
 if(n!=5)return 2;
 FILE *f=fopen(v[1],"rb");if(!f)return 2;
 need(!fseek(f,0,SEEK_END),"size");long bytes=ftell(f);rewind(f);
 need(bytes>=163840 && bytes<=33554432,"size");
 size_t size=(size_t)bytes;unsigned char *a=malloc(size),*b=malloc(size);need(a&&b,"oracle memory");
 need(fread(a,1,size,f)==size,"size");fclose(f);
 f=fopen(v[2],"rb");if(!f)return 2;need(fread(b,1,size,f)==size,"ref size");fclose(f);
 need(a[21]==(!strcmp(v[3],"floppy")?b[21]:0xf8),"BPB media byte");
 need(!memcmp(a+11,b+11,25),"BPB fields vs mformat");
 need(a[13]==1 && a[510]==0x55 && a[511]==0xaa,"spc/signature");
 unsigned spf=u16(a+22),root=1+2*spf;unsigned char *e=a+root*512;
 need(a[512]==a[21] && a[513]==255 && a[514]==255,"FAT[0] media/EOC");
 need(!memcmp(a+512,a+(1+spf)*512,spf*512),"FAT mirrors");
 /* Boot code after DOS BPB must equal the boot-system mint's mformat VBR. */
 unsigned char boot[512];f=fopen(v[4],"rb");if(!f)return 2;need(!fseek(f,63*512,SEEK_SET)&&fread(boot,1,512,f)==512,"source VBR");fclose(f);
 need(!memcmp(a+62,boot+62,450),"boot sector code vs mk_boot_system");
 if(!strcmp(v[3],"empty")||!strcmp(v[3],"floppy"))need(!memcmp(a+512,b+512,size-512),"empty FAT/root/data vs mformat");
 else {
  need(!memcmp(e,"IO      SYS",11) && !memcmp(e+32,"INITDOS SYS",11),"root entry order");
  need(e[11]==7 && e[43]==7,"system HSR");
  if(!strcmp(v[3],"system"))need(!memcmp(e+64,"COMMAND COM",11) && e[75]==32,"/S COMMAND.COM");
  else need(e[64]==0,"SYS excludes COMMAND.COM");
 }
 puts("GREEN formatsys independent image layout");return 0;
}
