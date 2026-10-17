/* Freestanding disk utilities. Law 1: ADR-0003 Appendix A/B.1, DEC-17/18;
 * MS-DOS 3.3 UG pp26,59-63,74-76,119,318,332-337,340,348,358;
 * IBM DOS 3.30 TR pp1-7,2-31,5-4..11. No host runtime or clock.
 * Physical translation belongs to the adapter; letter mapping to the kernel.
 * FAT metadata is streamed, so FAT16 never requires a whole FAT in memory. */
#include "diskutil.h"
static const du_io *io;
static uint8_t copybuf[32768];
static unsigned u16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return u16(p)|((uint32_t)u16(p+2)<<16);}
static void w16(uint8_t *p,unsigned v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,uint32_t v){w16(p,v);w16(p+2,v>>16);}
static void zero(void *p,unsigned n){uint8_t *b=p;while(n--)*b++=0;}
static void cp(void *d,const void *s,unsigned n){uint8_t *a=d;const uint8_t *b=s;while(n--)*a++=*b++;}
static int eq(const void *a,const void *b,unsigned n){const uint8_t *x=a,*y=b;while(n--)if(*x++!=*y++)return 0;return 1;}
static char upper(char c){return c>='a'&&c<='z'?(char)(c-32):c;}
static void put(const char *s){io->put(s);}
static void num(uint32_t v){char b[11];unsigned n=0;do{b[n++]=(char)('0'+v%10);v/=10;}while(v);char out[12];for(unsigned i=0;i<n;i++)out[i]=b[n-1-i];out[n]=0;put(out);}
static void line(const char *p){put(p);put("\r\n");}
static void drive_text(const char *prefix,unsigned d,const char *suffix){char c[2]={(char)('A'+d),0};put(prefix);put(c);put(suffix);}
static int error(const char *s,int rc){line(s);return rc;}
static int input(char *p,unsigned cap){unsigned n=0;zero(p,cap+1);for(;;){int c=io->get();if(c==3)return -1;if(c==13||c==10){p[n]=0;put("\r\n");return (int)n;}if(c==8){if(n){n--;put("\b \b");}}else if(c>=32&&c<127&&n<cap){p[n++]=upper((char)c);char b[2]={(char)c,0};put(b);}}}
static int yes(void){int c;do{c=upper((char)io->get());if(c==3)return 0;}while(c!='Y'&&c!='N');char b[2]={(char)c,0};line(b);return c=='Y';}
typedef struct {du_map m;uint8_t boot[512];unsigned spf,root,roots,data,bits,clusters;} volume;
static int sec(const du_map *m,uint32_t n,void *p,int write){if(n>=m->sectors)return -1;return io->sector(m->disk,m->base+n,p,write);}
static int parse(volume *v){
 uint8_t *b=v->boot;unsigned total=u16(b+19);if(!total)total=u32(b+32);
 if(u16(b+11)!=512 || b[13]!=1 || u16(b+14)!=1 || b[16]!=2 || !u16(b+22) || !u16(b+17) || total>v->m.sectors || b[510]!=0x55 || b[511]!=0xaa)return -1;
 v->spf=u16(b+22);v->roots=(u16(b+17)+15)/16;v->root=1+2*v->spf;v->data=v->root+v->roots;
 if(total<=v->data)return -1;
 v->clusters=total-v->data;v->bits=v->clusters<4085?12:16;
 if(v->clusters>=65525 || ((v->clusters+2)*v->bits+7)/8>v->spf*512)return -1;
 return 0;
}
static int mount(volume *v,const du_map *m){v->m=*m;return sec(m,0,v->boot,0)||parse(v);}
static int fatget(const volume *v,unsigned c,unsigned *out){
 unsigned off=v->bits==12?c+c/2:c*2;uint8_t b[512],next[512];
 if(off+1>=v->spf*512 || sec(&v->m,1+off/512,b,0))return -1;
 unsigned x=b[off%512];if(off%512==511){if(sec(&v->m,2+off/512,next,0))return -1;x|=(unsigned)next[0]<<8;}else x|=(unsigned)b[off%512+1]<<8;
 *out=v->bits==16?x:c&1?x>>4:x&4095;return 0;
}
static int fatset(const volume *v,unsigned c,unsigned x){
 unsigned off=v->bits==12?c+c/2:c*2;uint8_t b[1024];unsigned n=off%512==511?2:1;
 if(off+1>=v->spf*512)return -1;
 for(unsigned i=0;i<n;i++)if(sec(&v->m,1+off/512+i,b+i*512,0))return -1;
 unsigned prev=u16(b+off%512);if(v->bits==12)x=c&1?(prev&15)|(x<<4):(prev&0xf000)|(x&4095);
 w16(b+off%512,x);
 for(unsigned f=0;f<2;f++)for(unsigned i=0;i<n;i++)if(sec(&v->m,1+f*v->spf+off/512+i,b+i*512,1))return -1;
 return 0;
}
static int entry(const volume *v,unsigned n,uint8_t e[32],int write){
 uint8_t b[512];if(n>=v->roots*16 || sec(&v->m,v->root+n/16,b,0))return -1;
 if(!write){cp(e,b+(n%16)*32,32);return 0;}
 cp(b+(n%16)*32,e,32);return sec(&v->m,v->root+n/16,b,1);
}
static const char names[3][12]={"IO      SYS","INITDOS SYS","COMMAND COM"};
static int find(const volume *v,const char *name,uint8_t e[32]){
 for(unsigned n=0;n<v->roots*16;n++){if(entry(v,n,e,0))return -1;if(!e[0])return -1;if(e[0]!=0xe5&&!(e[11]&24)&&eq(e,name,11))return 0;}return -1;
}
static int source(volume *v,uint8_t files[3][32],unsigned count){du_map m;if(io->map(2,&m)||mount(v,&m))return -1;for(unsigned i=0;i<count;i++)if(find(v,names[i],files[i]))return -1;return 0;}
static int freecount(const volume *v,unsigned *n){*n=0;for(unsigned c=2;c<v->clusters+2;c++){unsigned x;if(fatget(v,c,&x))return -1;if(!x)(*n)++;}return 0;}
static int freechain(const volume *v,unsigned c){for(unsigned n=0;c>=2&&c<v->clusters+2&&n<v->clusters;n++){unsigned next;if(fatget(v,c,&next)||fatset(v,c,0))return -1;c=next;}return 0;}
/* Validate the full source chain before the first destination write. */
static int chain(const volume *v,const uint8_t *e,int contiguous){
 unsigned count=(u32(e+28)+511)/512,c=u16(e+26);if(!count||count>v->clusters)return -1;
 for(unsigned i=0;i<count;i++){
  unsigned next;if(c<2||c>=v->clusters+2||fatget(v,c,&next))return -1;
  if(i+1==count){if(next<(v->bits==12?0xff8u:0xfff8u))return -1;}
  else if(contiguous && next!=c+1)return -1;
  c=next;
 }
 return 0;
}
static int transfer(volume *dst,volume *src,uint8_t files[3][32],unsigned count,int formatted,unsigned *used){
 uint8_t old[3][32];unsigned needed=0,available=0;int same=dst->m.disk==src->m.disk&&dst->m.base==src->m.base;
 *used=0;
 for(unsigned i=0;i<count;i++){
  if(!(same&&formatted)&&chain(src,files[i],0))return -1;
  needed+=(u32(files[i]+28)+511)/512;
  if(entry(dst,i,old[i],0))return -1;
  if(old[i][0] && old[i][0]!=0xe5 && !eq(old[i],names[i],11))return -1;
 }
 if(same&&!formatted){for(unsigned i=0;i<2;i++){files[i][11]=7;if(entry(dst,i,files[i],1))return -1;}return sec(&dst->m,0,dst->boot,1);}
 if(freecount(dst,&available))return -1;
 for(unsigned i=0;i<count;i++)if(old[i][0]&&old[i][0]!=0xe5)available+=(u32(old[i]+28)+511)/512;
 if(available<needed)return -1;
 for(unsigned i=0;i<count;i++)if(old[i][0]&&old[i][0]!=0xe5&&freechain(dst,u16(old[i]+26)))return -1;
 unsigned scan=2;
 for(unsigned i=0;i<count;i++){
  unsigned source_c=u16(files[i]+26),first=0,prev=0,n=(u32(files[i]+28)+511)/512;
  uint8_t buf[512],e[32];cp(e,files[i],32);e[11]=(uint8_t)(i<2?7:32);
  for(unsigned j=0;j<n;j++){
   unsigned value,next;
   for(;;){if(scan>=dst->clusters+2||fatget(dst,scan,&value))return -1;if(!value)break;scan++;}
   if(!first)first=scan;
   if(sec(&src->m,src->data+source_c-2,buf,0))return -1;
   if(same&&formatted)next=source_c+1;else if(fatget(src,source_c,&next))return -1;
   if(sec(&dst->m,dst->data+scan-2,buf,1) || fatset(dst,scan,dst->bits==12?0xfff:0xffff) || (prev&&fatset(dst,prev,scan)))return -1;
   prev=scan++;source_c=next;
  }
  w16(e+26,first);w16(e+22,0);w16(e+24,0x21); /* fixed DOS epoch, Rule 11 */
  if(entry(dst,i,e,1))return -1;
  *used+=n*512;
 }
#ifdef DU_MUT_ORDER
 uint8_t e[32],f[32];if(entry(dst,0,e,0)||entry(dst,1,f,0)||entry(dst,0,f,1)||entry(dst,1,e,1))return -1;
#endif
 return sec(&dst->m,0,dst->boot,1);
}
typedef struct {unsigned letter[2],letters,s,v,b,one,four,eight,tracks,sectors;} options;
static int options_parse(options *o,unsigned kind,const char *tail){
 zero(o,sizeof *o);
 while(*tail){while(*tail==' '||*tail=='\t')tail++;if(!*tail)break;
  char c=upper(*tail++);
  if(c=='/'){
   c=upper(*tail++);
   if(c=='S'&&kind==0)o->s=1;else if(c=='V'&&kind==0)o->v=1;else if(c=='B'&&kind==0)o->b=1;
   else if(c=='1'&&kind!=1)o->one=1;else if(c=='4'&&kind==0)o->four=1;else if(c=='8'&&(kind==0||kind==3))o->eight=1;
   else if((c=='T'||c=='N')&&kind==0&&*tail++==':'){
    unsigned n=0;if(*tail<'0'||*tail>'9')return -1;
    while(*tail>='0'&&*tail<='9'){n=n*10+(unsigned)(*tail++-'0');if(n>255)return -1;}
    if(c=='T')o->tracks=n;else o->sectors=n;
   }else return -1;
  }else if(c>='A'&&c<='Z'&&*tail==':'&&o->letters<2){tail++;o->letter[o->letters++]=(unsigned)(c-'A');}
  else return -1;
  if(*tail && *tail!=' '&&*tail!='\t'&&*tail!='/')return -1;
 }
 if(kind<2 && o->letters!=1)return -1;
 if(o->b&&(o->s||o->v))return -1;
 if(kind>=2){if(!o->letters)o->letter[0]=0;if(o->letters<2)o->letter[1]=0;}
 return 0;
}
static int layout(volume *v,const options *o,const uint8_t template[512]){
 uint32_t total=v->m.sectors;unsigned heads=16,spt=63,roots=total<4096?14:32;uint8_t media=0xf8;
 if(v->m.flags&DU_REMOVABLE){
  heads=o->one?1:2;spt=o->eight?8:o->sectors?o->sectors:total==2400?15:total==1440||total<=720?9:18;
  unsigned tracks=o->tracks?o->tracks:(total<=720||o->four?40:80);
  if(o->four){tracks=40;spt=o->eight?8:9;}
  total=tracks*heads*spt;if(total>v->m.sectors)return -1;
  roots=total<=360?4:total<=720?7:14;
  media=total==320?0xfe:total==360?0xfc:total==640?0xff:total==720?0xfd:total==1440||total==2400?0xf9:0xf0;
 }else if(o->one||o->four||o->eight||o->tracks||o->sectors||o->b)return -1;
 if(!(v->m.flags&DU_REMOVABLE)&&total==2880){heads=2;spt=18;}
 if(total>65535)return -2;
 unsigned bits=12,spf,clusters;
 for(;;){for(spf=1;;spf++){if(total<=1+roots+2*spf)return -1;clusters=total-1-roots-2*spf;if(((clusters+2)*bits+7)/8<=spf*512)break;}if(bits==12&&clusters>=4085){bits=16;continue;}break;}
 if(clusters>=65525)return -2;
 cp(v->boot,template,512);uint8_t *b=v->boot;
 w16(b+11,512);b[13]=1;w16(b+14,1);b[16]=2;w16(b+17,roots*16);w16(b+19,total);b[21]=media;w16(b+22,spf);w16(b+24,spt);w16(b+26,heads);w32(b+28,v->m.base);w32(b+32,0);
 b[36]=(uint8_t)v->m.disk;cp(b+43,"NO NAME    ",11);cp(b+54,bits==12?"FAT12   ":"FAT16   ",8);
#ifdef DU_MUT_MEDIA
 b[21]^=1;
#endif
 return parse(v);
}
static void label_name(const volume *v,char out[12]){uint8_t e[32];zero(out,12);for(unsigned n=0;n<v->roots*16;n++){if(entry(v,n,e,0)||!e[0])return;if(e[0]!=0xe5&&e[11]==8){cp(out,e,11);unsigned i=11;while(i&&out[i-1]==' ')out[--i]=0;out[i]=0;return;}}}
static int format_run(const options *o,const du_map *m){
 unsigned letter=o->letter[0];
#ifndef DU_MUT_RUNNING
 if(m->flags&DU_RUNNING){drive_text("Format not supported on drive ",letter,":\r\n");return 4;}
#endif
 volume src,dst;uint8_t files[3][32],template[512];du_map cm;
 if(io->map(2,&cm)||sec(&cm,0,template,0))return error("Format failure",4);
 if((o->s||o->b)&&source(&src,files,o->s?3:2))return error("Insufficient memory for system transfer",4);
 dst.m=*m;int rc=layout(&dst,o,template);
 if(rc)return error(rc==-2?"Format requires more than one sector per cluster":"Format failure",4);
 if(o->s||o->b){unsigned count=o->s?3:2,need=0;for(unsigned i=0;i<count;i++){if(chain(&src,files[i],0))return error("Format failure",4);need+=(u32(files[i]+28)+511)/512;}if(need>dst.clusters)return error("No room for system on destination disk",4);
  if(src.m.disk==m->disk&&src.m.base==m->base){unsigned start=2;if(src.data!=dst.data)return error("Insufficient memory for system transfer",4);for(unsigned i=0;i<3;i++){if(chain(&src,files[i],1)||u16(files[i]+26)!=start)return error("Insufficient memory for system transfer",4);start+=(u32(files[i]+28)+511)/512;}}
 }
 for(;;){
  char answer[12];volume old;
  if(m->flags&DU_REMOVABLE){drive_text("Insert new diskette for drive ",letter,":\r\nand strike ENTER when ready\r\n");if(input(answer,0)<0)return 3;}
  else {
   if(!mount(&old,m)){char label[12];label_name(&old,label);drive_text("Enter current Volume Label for drive ",letter,": ");if(input(answer,11)<0)return 3;if(!eq(label,answer,12))return error("Invalid Volume ID Format failure",4);}
   line("WARNING, ALL DATA ON NON-REMOVABLE DISK");drive_text("DRIVE ",letter,": WILL BE LOST!\r\nProceed with Format (Y/N)?");if(input(answer,11)<0)return 3;if(answer[0]!='Y')return 5;
  }
  line("Formatting...");
#ifndef DU_MUT_LABEL
  cp(dst.boot+43,"NO NAME    ",11); /* UG p75: ENTER means no label. */
#endif
  uint8_t b[512];zero(b,512);
  /* Clear FATs/root only. Data sectors carry no invented new file content. */
  for(unsigned s=1;s<dst.data;s++)if(sec(m,s,b,1))return error("Format failure",4);
  b[0]=dst.boot[21];b[1]=b[2]=255;if(dst.bits==16)b[3]=255;
#ifdef DU_MUT_FAT0
  b[0]=0;
#endif
  if(sec(m,1,b,1)||sec(m,1+dst.spf,b,1)||sec(m,0,dst.boot,1))return error("Format failure",4);
  line("Format complete");unsigned used=0;
  if(o->s||o->b){unsigned count=o->s?3:2;
#ifdef DU_MUT_COMMAND
   count=2;
#endif
   if(transfer(&dst,&src,files,count,1,&used))return error("No room for system on destination disk",4);
   if(o->b) {
    /* UG p74: /B leaves room for a later operating system. Reserve the
     * two HSR root slots and their clusters; no executable system content.
     * SYS p119 can replace these reservations without moving other files. */
    uint8_t empty[512];zero(empty,512);
#ifdef DU_MUT_RESERVE
    empty[0]=1; /* Rule 6: /B must reserve space without system content. */
#endif
    for(unsigned i=0;i<2;i++){
     uint8_t e[32];if(entry(&dst,i,e,0))return error("Format failure",4);
     unsigned c=u16(e+26),n=(u32(e+28)+511)/512;
     for(unsigned j=0;j<n;j++,c++)if(sec(m,dst.data+c-2,empty,1))return error("Format failure",4);
    }
   }else line("System transferred");}
  if(o->v){put("Volume label (11 characters, ENTER for none)?");int n=input(answer,11);if(n<0)return 3;
   if(n){uint8_t e[32];zero(e,32);for(unsigned i=0;i<11;i++)e[i]=i<(unsigned)n?(uint8_t)answer[i]:' ';e[11]=8;w16(e+24,0x21);if(entry(&dst,o->s?3:0,e,1))return error("Format failure",4);cp(dst.boot+43,e,11);if(sec(m,0,dst.boot,1))return error("Format failure",4);}
  }
  num(dst.clusters*512);line(" bytes total disk space");if(o->s){num(used);line(" bytes used by system");}num(dst.clusters*512-used);line(" bytes available on disk");
  if(!(m->flags&DU_REMOVABLE))return 0;
  put("Format another (Y/N)?");if(!yes())return 0;
 }
}
static int sys_run(const du_map *m){volume src,dst;uint8_t files[3][32];unsigned used;
 if(source(&src,files,2)||mount(&dst,m))return error("No room for system on destination disk",1);
 /* Preserve destination BPB, use the existing InitechDOS VBR code. */
 cp(dst.boot,src.boot,11);cp(dst.boot+62,src.boot+62,450);
 if(transfer(&dst,&src,files,2,0,&used))return error("No room for system on destination disk",1);
 return error("System transferred",0);
}
static void swap_prompt(int compare,int target,unsigned letter){
 drive_text(compare?(target?"Insert SECOND diskette into drive ":"Insert FIRST diskette into drive "):(target?"Insert TARGET diskette into drive ":"Insert SOURCE diskette in drive "),letter,":\r\n");
 if(!compare)line("Press any key when ready . . .");
 (void)io->get();
}
static int copy_run(const options *o,int compare){
 du_map m[2];for(unsigned i=0;i<2;i++){if(io->map(o->letter[i],m+i))return error("Invalid drive specification",4);if(!(m[i].flags&DU_REMOVABLE))return error("Specified drive does not exist,\r\nor is non-removable",4);}
 int result=0;
 do{
  swap_prompt(compare,0,o->letter[0]);uint8_t b[512];if(sec(m,0,b,0))return error("FIRST diskette bad or incompatible",3);
  unsigned total=u16(b+19),spt=u16(b+24),heads=u16(b+26);if(u16(b+11)!=512||!total||total>m[0].sectors||!spt||spt>63||!heads||heads>2)return error("FIRST diskette bad or incompatible",3);
  unsigned count=sizeof copybuf/512;
  result=0;
  for(unsigned first=0;first<total;first+=count){unsigned n=total-first;if(n>count)n=count;
   if(first)swap_prompt(compare,0,o->letter[0]);
   for(unsigned j=0;j<n;j++)if(sec(m,first+j,copybuf+j*512,0))return error("FIRST diskette bad or incompatible",3);
   swap_prompt(compare,1,o->letter[1]);
   for(unsigned j=0;j<n;j++){
    unsigned l=first+j,side=(l/spt)%heads,track=l/(spt*heads);
    if((o->one&&side)||(o->eight&&l%spt>=8))continue;
#ifdef DU_MUT_COMPARE
    if(compare&&l+1==total)continue;
#endif
    if(compare){if(sec(m+1,l,b,0))return error("Compare process ended",3);if(!eq(b,copybuf+j*512,512)){put("Compare error on disk\r\nside ");num(side);put(", track ");num(track);put("\r\n");result=1;}}
    else if(sec(m+1,l,copybuf+j*512,1))return error("Target diskette may be unusable",3);
   }
  }
  if(compare&&!result)line("Compare OK");
  put(compare?"Compare another diskette (Y/N)?":"Copy another diskette (Y/N)?");
 }while(yes());
 return result;
}
int diskutil_run(const du_io *ops,unsigned kind,const char *tail){io=ops;options o;if(options_parse(&o,kind,tail))return error("Invalid parameter",4);
 if(kind>=2)return copy_run(&o,kind==3);
 du_map m;if(io->map(o.letter[0],&m))return error("Invalid drive specification",4);
 return kind?sys_run(&m):format_run(&o,&m);
}
