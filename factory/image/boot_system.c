/* Independent factory layout oracle and fault injector, initech-kod.
 * Ref: IBM DOS 3.30 Technical Reference printed 1-7, 5-4, 5-7..11
 * (PDF 24,111,114..118), 9-6..7 (PDF393..394, MBR partition table);
 * MS-DOS 3.3 User's Guide p119 (PDF204).
 * No artifact loader code is linked here. mtools/fsck judge the same bytes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Large enough for the C: system disk, 20 cylinders x 16 heads x 63. */
static unsigned char disk[20160*512], seen[4096];
static unsigned u16(const unsigned char *p) { return p[0]+256u*p[1]; }
static unsigned u32(const unsigned char *p) { return u16(p)+65536u*u16(p+2); }
static void w32(unsigned char *p,unsigned n) { for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i)); }
static void need(int yes,const char *rule) { if(!yes){fprintf(stderr,"RED boot-system %s\n",rule);exit(1);} }
static void fat_set(unsigned char *f,unsigned c,unsigned v) {
 unsigned o=c+c/2, n=u16(f+o);
 n=c&1?(n&15)|(v<<4):(n&0xf000)|v;f[o]=(unsigned char)n;f[o+1]=(unsigned char)(n>>8);
}
int main(int argc,char **argv) {
 if (argc != 3 && !(argc == 5 && !strcmp(argv[2], "check"))) return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t bytes=fread(disk,1,sizeof disk,f);fclose(f);
 need(bytes>=448*512,"image size");
 if(!strcmp(argv[2],"partition")) {
  /* 46 cylinders, 2 heads, 32 sectors; partition at 63, length 2880. */
  unsigned char *p=disk+446;memset(p,0,64);
  p[0]=0x80;p[1]=1;p[2]=32;p[4]=1;p[5]=1;p[6]=31;p[7]=45;
  w32(p+8,63);w32(p+12,2880);
 } else if(!strcmp(argv[2],"partition-c")) {
  /* The C: system disk (delivery, initech-tlhqy): 20 cylinders, 16 heads,
   * 63 sectors; one FAT12 partition at LBA 63, 16384 sectors (8 MiB).
   * CHS from LBA: c = lba/(16*63), h = lba/63 % 16, s = lba%63 + 1
   * (IBM DOS 3.30 TR 9-6..7). End LBA 16446 = c16 h5 s4. */
  need(bytes==20160*512,"C: disk is 20160 sectors");
  unsigned char *p=disk+446;memset(p,0,64);
  p[0]=0x80;p[1]=1;p[2]=1;p[3]=0;p[4]=1;p[5]=5;p[6]=4;p[7]=16;
  w32(p+8,63);w32(p+12,16384);
 } else {
  need(disk[450]==1 && u32(disk+454)==63,"FAT boot partition missing");
  unsigned char *b=disk+63*512;
  unsigned res=u16(b+14),spf=u16(b+22),spc=b[13];
  need(u16(b+11)==512 && spc && b[16]==2 && spf,"BPB");
  unsigned root=res+2*spf,data=root+(u16(b+17)*32+511)/512;
  need(data<u16(b+19) && 63u+u16(b+19)<=bytes/512,"BPB bounds");
  unsigned char *e=b+root*512,*fat=b+res*512;
  if(!strcmp(argv[2],"ORDER")) { memmove(e+32,e,64);memset(e,0,32);memcpy(e,"FIRST   TXT",11); }
  else if(!strcmp(argv[2],"ATTR_R")) e[11]&=~1u;
  else if(!strcmp(argv[2],"CHAIN")) {
   for(unsigned k=0;k<2;k++)fat_set(fat+k*spf*512,u16(e+32+26),0xfff);
  } else if(!strcmp(argv[2],"MISSING")) e[0]=0xe5;
  else if(!strcmp(argv[2],"FRAGMENT")) {
   /* Independently relocate BOTH files into nonadjacent free clusters.
    * mcopy must still recover their bytes; the real loader must follow FAT. */
   for(unsigned i=0;i<2;i++,e+=32) {
    unsigned c=u16(e+26),n=(u32(e+28)+spc*512-1)/(spc*512);
    unsigned start=i?1000:700;
    for(unsigned j=0;j<n;j++) {
     unsigned off=c+c/2,next=u16(fat+off);
     next=c&1?next>>4:next&0xfff;
     unsigned dest=start+j*2;
     need(dest<2+(u16(b+19)-data)/spc,"fragment space");
     memcpy(b+(data+(dest-2)*spc)*512,b+(data+(c-2)*spc)*512,spc*512);
     for(unsigned k=0;k<2;k++) {
      fat_set(fat+k*spf*512,c,0);
      fat_set(fat+k*spf*512,dest,j+1==n?0xfff:dest+2);
     }
     c=next;
    }
    e[26]=(unsigned char)start;e[27]=(unsigned char)(start>>8);
   }
  }
  else if(!strcmp(argv[2],"check")) {
   const char *names[]={"IO      SYS","INITDOS SYS"};unsigned last=0;
   for(unsigned i=0;i<2;i++,e+=32) {
    need(!memcmp(e,names[i],11),"ORDER first two root entries");
    need(e[11]==7,"ATTR_R HSR attributes");
    unsigned c=u16(e+26),n=u32(e+28),count=0;
    /* IO.SYS loads at 0:9000 below stage2's buffers; INITDOS.SYS's old
     * 196608-byte cap is gone (boot/README.md: stage2 checks its length
     * and BSS against the reserved memory ceiling), so its chain bounds
     * here are the volume's. */
    need(n && (i || n<=4096),"file budget");
    FILE *source = argc == 5 ? fopen(argv[3+i], "rb") : NULL;
    if (argc == 5 && !source) { perror(argv[3+i]); return 2; }
    unsigned expected=1+(n-1)/(spc*512);
    while(count<expected) {
     need(c>=2 && c<2+(u16(b+19)-data)/spc && c<sizeof seen && !seen[c],"CHAIN bounds/cycle/overlap");
     seen[c]=1;
     if (source) {
      unsigned remaining = n - count*spc*512;
      unsigned take = remaining < spc*512 ? remaining : spc*512;
      const unsigned char *payload = b+(data+(c-2)*spc)*512;
      for (unsigned j=0; j<take; j++)
       need(fgetc(source)==payload[j], i ? "INITDOS.SYS bytes" : "IO.SYS bytes");
     }
     if(count)need(c==last+1,"CHAIN contiguous build layout");
     unsigned off=c+c/2;need(off+1<spf*512,"CHAIN FAT bounds");
     unsigned next=u16(fat+off);next=c&1?next>>4:next&0xfff;last=c;count++;
     if(count==expected)need(next>=0xff8,"CHAIN exact EOC");
     else need(next>=2 && next<0xff0,"CHAIN premature EOC");
     c=next;
    }
    if (source) {
     need(fgetc(source)==EOF && !ferror(source), "source file length");
     fclose(source);
    }
    printf("GREEN boot-system %s: root[%u] HSR, %u contiguous clusters, %u bytes\n",i?"INITDOS.SYS":"IO.SYS",i,count,n);
   }
   need(!memcmp(fat,fat+spf*512,spf*512),"FAT mirrors");return 0;
  } else return 2;
 }
 f=fopen(argv[1],"wb");if(!f)return 2;
 int ok=fwrite(disk,1,bytes,f)==bytes;return fclose(f)||!ok?2:0;
}
