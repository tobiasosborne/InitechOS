/* Independent corruption + reference-diff tool, FACTORY C only.
 * Ref: ADR-0003 B.1; fat12-ground-truth.md 1-4; MS-DOS 3.3 p319/334/344.
 * Never links chkdsk.c or fat12.c. fsck.fat and mtools judge these SAME bytes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static unsigned char *b; static size_t size;
static unsigned fat, spf, root, data, count, spc, bits;
static unsigned rd(const unsigned char *p) { return p[0]+256u*p[1]; }
static unsigned r32(const unsigned char *p) { return rd(p)+65536u*rd(p+2); }
static void w16(unsigned char *p,unsigned v){p[0]=v;p[1]=v>>8;}
static void load(const char *p) {
 FILE *f=fopen(p,"rb"); if(!f){perror(p);exit(2);} fseek(f,0,SEEK_END);
 size=ftell(f); rewind(f); b=malloc(size); if(!b||fread(b,1,size,f)!=size)exit(2);fclose(f);
 fat=rd(b+14);spf=rd(b+22);root=fat+b[16]*spf;spc=b[13];
 data=root+(rd(b+17)*32+511)/512;count=((rd(b+19)?rd(b+19):r32(b+32))-data)/spc;
 bits=count<4085?12:16;
}
static unsigned get(unsigned n) {
 unsigned o=bits==12?n+n/2:n*2,v=rd(b+fat*512+o);
 return bits==16?v:(n&1?v>>4:v&4095);
}
static void set(unsigned n,unsigned v) {
 for(unsigned k=0;k<b[16];k++) {
  unsigned o=(fat+k*spf)*512+(bits==12?n+n/2:n*2);
  if(bits==16)w16(b+o,v);else if(n&1)w16(b+o,(rd(b+o)&15)|(v<<4));
  else w16(b+o,(rd(b+o)&0xf000)|(v&4095));
 }
}
static unsigned char *find_entry(const char *name) {
 for(unsigned i=0;i<rd(b+17);i++) {unsigned char *e=b+root*512+i*32;
  if(!memcmp(e,name,11))return e;
 } return NULL;
}
static unsigned char *entry(const char *name) {
 unsigned char *e=find_entry(name);if(e)return e;
 fprintf(stderr,"missing fixture %.11s\n",name);exit(2);
}
static void save(const char *p){FILE*f=fopen(p,"wb");if(!f||fwrite(b,1,size,f)!=size||fclose(f))exit(2);}
static void require(int yes,const char *rule) {if(!yes){fprintf(stderr,"RED CHKDSK-%s\n",rule);exit(1);}}
static void normalize(unsigned char *p) {
 /* Only volatile directory date/time and the documented REC/CHK name differ.
  * All FATs, all original directory fields, data, BPB and slack still diff. */
 for(unsigned s=root;s<data;s++) for(unsigned o=0;o<512;o+=32) {
  unsigned char *e=p+s*512+o;
  if(!e[0]||e[0]==0xe5)continue;
  memset(e+12,0,14);
  if(!memcmp(e,"FSCK0000REC",11))memcpy(e,"FILE0000CHK",11);
 }
}
int main(int argc,char **argv) {
 if(argc<3)return 2;
 load(argv[2]);
 if(!strcmp(argv[1],"corrupt")) {
  if(argc!=4)return 2;
  /* Pin the volume-label creation stamp; no host clock leaks into fixtures. */
  unsigned char *vol=entry("CHKTEST    "); w16(vol+22,0); w16(vol+24,33);
  unsigned a=rd(entry("ALPHA   TXT")+26),z=rd(entry("BRAVO   TXT")+26);
  if(!strcmp(argv[3],"lost")) {
   require(get(700)==0 && get(701)==0,"FIXTURE");set(700,701);set(701,bits==12?4095:65535);
   memset(b+(data+(700-2)*spc)*512,'L',spc*512);
   memset(b+(data+(701-2)*spc)*512,'M',spc*512);
  }else if(!strcmp(argv[3],"cross")) {
   set(z,0); w16(entry("BRAVO   TXT")+26,a);
  }else if(!strcmp(argv[3],"invalid"))set(a,1);
  else if(!strcmp(argv[3],"badstart"))w16(entry("EMPTY   TXT")+26,count+10);
  else if(!strcmp(argv[3],"fragment")) {
   unsigned char *e=entry("FRAG    TXT");unsigned c=rd(e+26),n=get(c);
   while(get(n)<(bits==12?0xff8u:0xfff8u)){c=n;n=get(n);}
   require(get(710)==0,"FIXTURE");
   memcpy(b+(data+(710-2)*spc)*512,b+(data+(n-2)*spc)*512,spc*512);
   set(c,710);set(710,get(n));set(n,0);
  }else if(strcmp(argv[3],"clean"))return 2;
  save(argv[2]);return 0;
 }
 if(!strcmp(argv[1],"compare")) {
  if(argc!=4)return 2; normalize(b);unsigned char *a=b;size_t n=size;
  load(argv[3]);normalize(b);require(size==n && !memcmp(a,b,n),"REPAIR-DIFF");
  puts("GREEN CHKDSK-REPAIR-DIFF (all FATs, directory metadata, data and slack)");return 0;
 }
 if(!strcmp(argv[1],"check")) {
  if(argc!=6)return 2;const char *scenario=argv[3],*mode=argv[5];
  FILE*f=fopen(argv[4],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
  char*t=calloc(1,n+1);if(!t||fread(t,1,n,f)!=(size_t)n)return 2;fclose(f);
  /* CRLF is a transport property. Preserve every other byte, including columns. */
  char*d=t;for(char*s=t;*s;s++)if(*s!='\r')*d++=*s;*d=0;
  require(strstr(t,"Volume CHKTEST      Created JAN  1, 1980  0:00\n")!=0,"REPORT-LABEL");
  require(strstr(t,"bytes total disk space\n") && strstr(t,"bytes in 2 hidden files\n") &&
          strstr(t,"bytes in 2 directories\n") && strstr(t,"bytes in ") &&
          strstr(t," user files\n") && strstr(t,"bytes available on disk\n"),"REPORT");
  /* Independently counted fixture: seven user files, five one-cluster files
   * plus 2560-byte FRAG and zero-length EMPTY. Two subdirectories, one hidden
   * data file, and the label (counted as hidden by IBM 7-50).
   * The emulator additionally carries CHKDSK.COM and AUTOEXEC.BAT. */
  unsigned unit=spc*512,users=7,userbytes=5*unit+((2560+unit-1)/unit)*unit;
  if(!strncmp(mode,"emu",3)) {
   users+=2;
   userbytes+=((r32(entry("CHKDSK  COM")+28)+unit-1)/unit)*unit;
   userbytes+=((r32(entry("AUTOEXECBAT")+28)+unit-1)/unit)*unit;
  }
  if(strstr(mode,"recover")){users++;userbytes+=2*unit;}
  char report[256];
  snprintf(report,sizeof(report),"%9u bytes total disk space\n%9u bytes in 2 hidden files\n%9u bytes in 2 directories\n%9u bytes in %u user files\n",
           count*unit,unit,2*unit,userbytes,users);
  if(!strncmp(mode,"host",4))require(strstr(t,"     4064 bytes total memory\n     3808 bytes free\n")!=0,"MEMORY");
  else {
   unsigned image_bytes=r32(entry("CHKDSK  COM")+28);
   unsigned base=(0x40100u+image_bytes+0x10000u+15u)&~15u;
   unsigned memory=0x6f000u-base-16u;char expected[128];
   snprintf(expected,sizeof(expected),"%9u bytes total memory\n%9u bytes free\n",memory,memory);
   require(strstr(t,expected)!=0,"MEMORY");
  }
  if(!strcmp(scenario,"clean")||!strcmp(scenario,"fragment")) {
   require(strstr(t,"A:\\SUB\\DEEP\\NOTE.TXT\n")!=0,"VERBOSE");
   require(strstr(t," lost ")==0 && strstr(t,"cross-linked")==0 && strstr(t,"invalid cluster")==0,"CLEAN");
   if(!strcmp(scenario,"fragment"))require(strstr(t,"A:\\FRAG.TXT\n     Contains 2 non-contiguous blocks\n")!=0,"FRAG");
  }
  if(!strcmp(scenario,"cross")) {
   require(strstr(t,"A:\\ALPHA.TXT\nFile is cross-linked:\non cluster ")!=0 &&
           strstr(t,"A:\\BRAVO.TXT\nFile is cross-linked:\non cluster ")!=0,"CROSS");
  }
  if(!strcmp(scenario,"invalid")||!strcmp(scenario,"badstart"))
   require(strstr(t,"Has invalid cluster, file truncated\n")!=0,"INVALID");
  if(!strcmp(scenario,"lost")) {
   require(strstr(t,"2 lost allocation units found in 1 chains.\nConvert lost chains to files (Y/N)?")!=0,"LOST");
   if(strstr(mode,"recover"))require(find_entry("FILE0000CHK")!=NULL &&
                    rd(entry("FILE0000CHK")+26)==700 && r32(entry("FILE0000CHK")+28)==2*spc*512 && get(700)==701,"RECOVER");
   if(strstr(mode,"discard"))require(get(700)==0 && get(701)==0,"DISCARD");
  }
  if(!strcmp(mode,"dry"))require(strstr(t,"Corrections will not be written to disk\n")!=0,"NOFIX");
  if(strstr(mode,"recover")||strstr(mode,"discard"))require(!memcmp(b+fat*512,b+(fat+spf)*512,spf*512),"MIRROR");
  require(strstr(t,report)!=NULL,"REPORT-COLUMNS-BYTES");
  puts("GREEN CHKDSK manual report/verdict"); return 0;
 }
 return 2;
}
