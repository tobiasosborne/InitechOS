/* ATA-volume terminology is authored for the operator drive-A ruling.
 * Geometry, switches and the published removable bit keep the DOS 3.30
 * contract; historical reference captures retain their original wording. */
/* Freestanding disk utilities. the repository contract: ADR-0003 Appendix A/B.1, DEC-17/18;
 * MS-DOS 3.3 UG pp26,59-63,74-76,119,318,332-337,340,348,358;
 * IBM DOS 3.30 TR pp1-7,2-31,5-4..11. No host runtime or clock.
 * Physical translation belongs to the adapter; letter mapping to the kernel.
 * FAT metadata is streamed, so FAT16 never requires a whole FAT in memory. */
#include "diskutil.h"
#include "dos_messages.h"   /* DEC-13: the controlled catalogue, MSG_DOS_0004 */
#include "fat_policy.h"
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
static int protected(unsigned letter){
 const char *p=MSG_DOS_0008;char c[2]={(char)('A'+letter),0};
 while(*p){if(p[0]=='%'&&p[1]=='c'){put(c);p+=2;}else{char b[2]={*p++,0};put(b);}}
 put("\r\n");return 4;
}
static int input(char *p,unsigned cap){unsigned n=0;zero(p,cap+1);for(;;){int c=io->get();if(c==3)return -1;if(c==13||c==10){p[n]=0;put("\r\n");return (int)n;}if(c==8){if(n){n--;put("\b \b");}}else if(c>=32&&c<127&&n<cap){p[n++]=upper((char)c);char b[2]={(char)c,0};put(b);}}}
static int yes(void){int c;do{c=upper((char)io->get());if(c==3)return 0;}while(c!='Y'&&c!='N');char b[2]={(char)c,0};line(b);return c=='Y';}
typedef struct {du_map m;uint8_t boot[512];unsigned spc,spf,root,roots,data,bits,clusters;} volume;
static int sec(const du_map *m,uint32_t n,void *p,int write){if(n>=m->sectors)return -1;return io->sector(m->disk,m->base+n,p,write);}
static int fat16_target(const du_map *m){uint8_t b[512];return !sec(m,0,b,0)&&dos_fat16(b);}
static int parse(volume *v){
 uint8_t *b=v->boot;unsigned total=u16(b+19);if(!total)total=u32(b+32);
 v->spc=b[13];
 if(u16(b+11)!=512 || !v->spc || v->spc>8 || (v->spc&(v->spc-1)) ||
    u16(b+14)!=1 || b[16]!=2 || !u16(b+22) || !u16(b+17) || total>v->m.sectors || b[510]!=0x55 || b[511]!=0xaa)return -1;
 v->spf=u16(b+22);v->roots=(u16(b+17)+15)/16;v->root=1+2*v->spf;v->data=v->root+v->roots;
 if(total<=v->data)return -1;
 v->clusters=(total-v->data)/v->spc;v->bits=v->clusters<4085?12:16;
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
typedef struct {
 volume system, command;
 uint8_t files[3][32];
 int split;
} transfer_source;
static int source(transfer_source *src,unsigned count){
 du_map m;
 src->split=0;
 if(io->map(2,&m)||mount(&src->system,&m))return -1;
 for(unsigned i=0;i<2;i++)if(find(&src->system,names[i],src->files[i]))return -1;
 if(count==2 || !find(&src->system,names[2],src->files[2]))return 0;
 /* The two-file legacy master boots the resident shell from A:. The
  * legacy pair and this source split expire at the installed C: cutover. */
 if((src->system.m.flags&DU_RUNNING) || io->map(0,&m) ||
    !(m.flags&DU_RUNNING) || mount(&src->command,&m) ||
    find(&src->command,names[2],src->files[2]))return -1;
 src->split=1;
 return 0;
}
static const volume *file_volume(const transfer_source *src,unsigned i){
 return i==2 && src->split ? &src->command : &src->system;
}
static int same_volume(const du_map *a,const du_map *b){
 return a->disk==b->disk && a->base==b->base;
}
static unsigned file_sectors(const uint8_t e[32]){
 uint32_t bytes=u32(e+28);
 return bytes/512+(bytes%512!=0);
}
static unsigned file_clusters(const volume *v,const uint8_t e[32]){
 return (file_sectors(e)+v->spc-1)/v->spc;
}
static int freecount(const volume *v,unsigned *n){*n=0;for(unsigned c=2;c<v->clusters+2;c++){unsigned x;if(fatget(v,c,&x))return -1;if(!x)(*n)++;}return 0;}
static int freechain(const volume *v,unsigned c){for(unsigned n=0;c>=2&&c<v->clusters+2&&n<v->clusters;n++){unsigned next;if(fatget(v,c,&next)||fatset(v,c,0))return -1;c=next;}return 0;}
/* Validate the full source chain before the first destination write. */
static int chain(const volume *v,const uint8_t *e,int contiguous){
 unsigned count=file_clusters(v,e),c=u16(e+26);if(!count||count>v->clusters)return -1;
 for(unsigned i=0;i<count;i++){
  unsigned next;if(c<2||c>=v->clusters+2||fatget(v,c,&next))return -1;
  if(i+1==count){if(next<(v->bits==12?0xff8u:0xfff8u))return -1;}
  else if(contiguous && next!=c+1)return -1;
  c=next;
 }
 return 0;
}
static int system_name(const uint8_t e[32],unsigned count){
 if(e[11]&24)return 0;
 for(unsigned i=0;i<count;i++)if(eq(e,names[i],11))return 1;
 return 0;
}
static int transfer(volume *dst,const transfer_source *src,unsigned count,int formatted,unsigned *used){
 unsigned needed=0,available=0,held=0,command_slot=dst->roots*16;
 *used=0;
 for(unsigned i=0;i<count;i++){
  /* FORMAT has already validated its input before clearing the FAT.
   * SYS refuses a destination that also contains any of its inputs. */
  if(!formatted && (same_volume(&dst->m,&file_volume(src,i)->m) ||
                    chain(file_volume(src,i),src->files[i],0)))return -1;
  needed+=file_clusters(dst,src->files[i]);
 }
 /* Genuine DOS 3.30 (sys-elsewheresys): a system-named file anywhere in
  * the root is replaced in slots 0/1. COMMAND.COM uses the first free
  * slot after those two, leaving unrelated files in place. */
 for(unsigned n=0;n<dst->roots*16;n++){
  uint8_t e[32];if(entry(dst,n,e,0))return -1;
  int live=e[0] && e[0]!=0xe5, replace=live && system_name(e,count);
  if(n<2 && live && !replace)return -1; /* DOS 3.30 sys-occupied */
  if(n>=2 && (!live || replace) && command_slot==dst->roots*16)command_slot=n;
  if(replace)held+=file_clusters(dst,e);
  if(!e[0]){if(n<2)command_slot=2;break;}
 }
 if(count==3 && command_slot==dst->roots*16)return -1;
 if(freecount(dst,&available))return -1;
 available+=held;
 if(available<needed)return -1;
 for(unsigned n=0;n<dst->roots*16;n++){
  uint8_t e[32];if(entry(dst,n,e,0))return -1;if(!e[0])break;
  if(e[0]==0xe5||(e[11]&24))continue;
  if(system_name(e,count)){if(freechain(dst,u16(e+26)))return -1;e[0]=0xe5;if(entry(dst,n,e,1))return -1;}
 }
 unsigned scan=2;
 for(unsigned i=0;i<count;i++){
  const volume *from=file_volume(src,i);
  int same=same_volume(&dst->m,&from->m);
  unsigned source_c=u16(src->files[i]+26),source_sector=0,remaining=file_sectors(src->files[i]);
  unsigned first=0,prev=0,n=file_clusters(dst,src->files[i]);
  uint8_t buf[512],e[32];cp(e,src->files[i],32);e[11]=(uint8_t)(i<2?7:32);
  for(unsigned j=0;j<n;j++){
   unsigned value;
   for(;;){if(scan>=dst->clusters+2||fatget(dst,scan,&value))return -1;if(!value)break;scan++;}
   if(!first)first=scan;
   for(unsigned k=0;k<dst->spc;k++){
    if(remaining){
     if(sec(&from->m,from->data+(source_c-2)*from->spc+source_sector,buf,0))return -1;
     if(++source_sector==from->spc && remaining>1){
      source_sector=0;
      if(same)source_c++;
      else if(fatget(from,source_c,&source_c))return -1;
     }
     remaining--;
    }else zero(buf,sizeof buf);
    if(sec(&dst->m,dst->data+(scan-2)*dst->spc+k,buf,1))return -1;
   }
   if(fatset(dst,scan,dst->bits==12?0xfff:0xffff) || (prev&&fatset(dst,prev,scan)))return -1;
   prev=scan++;
  }
  w16(e+26,first);w16(e+22,0);w16(e+24,0x21); /* fixed DOS epoch, the repository contract */
  if(entry(dst,i<2?i:command_slot,e,1))return -1;
  *used+=n*dst->spc*512;
 }
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
 return parse(v);
}
static void label_name(const volume *v,char out[12]){uint8_t e[32];zero(out,12);for(unsigned n=0;n<v->roots*16;n++){if(entry(v,n,e,0)||!e[0])return;if(e[0]!=0xe5&&e[11]==8){cp(out,e,11);unsigned i=11;while(i&&out[i-1]==' ')out[--i]=0;out[i]=0;return;}}}
static int format_run(const options *o,const du_map *m){
 unsigned letter=o->letter[0];
 if(fat16_target(m))return protected(letter);
 if(m->flags&DU_RUNNING){drive_text("Format not supported on drive ",letter,":\r\n");return 4;}
 /* DOS 3.30 NATIVE4T/NATIVE8N reject contradictory dimensions
  * before prompting for media or replacing the existing filesystem. */
 unsigned fixed_spt=o->eight?8:9;
 if((o->four && o->tracks && o->tracks!=40) ||
    ((o->four || o->eight) && o->sectors && o->sectors!=fixed_spt))
  return error("Parameters not compatible\r\nFormat failure",4);
 /* DOS 3.30 NATIVE1 refuses one-sided formatting on 1.44 MB media. */
 if((m->flags&DU_REMOVABLE) && m->sectors==2880 && o->one)
  return error("Parameters not Supported by Drive\r\nFormat failure",4);
 transfer_source src;volume dst;uint8_t template[512];du_map cm;
 unsigned count=o->s?3:2;
 if(o->s||o->b){
  /* MS-DOS 3.3 UG p336 (PDF421), audit L041: unavailable source files
   * prompt for a DOS disk; p337's memory error is a different condition.
   * DEC-18 fixes the source at C:. Retry the lookup before ANY writes. */
  while(source(&src,count)){
   char answer[1];drive_text("Insert DOS volume in drive ",2,":\r\nand strike ENTER when ready\r\n");
   if(input(answer,0)<0)return 3;
  }
  cp(template,src.system.boot,512);
 }else if(io->map(2,&cm)||sec(&cm,0,template,0))return error("Format failure",4);
 dst.m=*m;int rc=layout(&dst,o,template);
 if(rc)return error(rc==-2?"Format requires more than one sector per cluster":"Format failure",4);
 if(dst.bits==16)return protected(letter);
 if(o->s||o->b){
  unsigned start=2;
  for(unsigned i=0;i<count;i++){
   const volume *from=file_volume(&src,i);
   if(chain(from,src.files[i],0))return error("Format failure",4);
   if(same_volume(&from->m,m) &&
      (from->data!=dst.data || from->spc!=dst.spc ||
       chain(from,src.files[i],1) || u16(src.files[i]+26)!=start))
    return error("Insufficient memory for system transfer",4);
   start+=file_clusters(&dst,src.files[i]);
  }
  if(start-2>dst.clusters)return error("No room for system on destination disk",4);
 }
 for(;;){
  char answer[12];volume old;
  if(m->flags&DU_REMOVABLE){drive_text("Insert new volume for drive ",letter,":\r\nand strike ENTER when ready\r\n");if(input(answer,0)<0)return 3;}
  else {
   if(!mount(&old,m)){char label[12];label_name(&old,label);drive_text("Enter current Volume Label for drive ",letter,": ");if(input(answer,11)<0)return 3;if(!eq(label,answer,12))return error("Invalid Volume ID Format failure",4);}
   line("WARNING, ALL DATA ON NON-REMOVABLE DISK");drive_text("DRIVE ",letter,": WILL BE LOST!\r\nProceed with Format (Y/N)?");if(input(answer,11)<0)return 3;if(answer[0]!='Y')return 5;
  }
   if(fat16_target(m))return protected(letter);
   line("Formatting...");
  cp(dst.boot+43,"NO NAME    ",11); /* UG p75: ENTER means no label. */
  uint8_t b[512];zero(b,512);
  /* Clear FATs/root only. Data sectors carry no invented new file content. */
  for(unsigned s=1;s<dst.data;s++)if(sec(m,s,b,1))return error("Format failure",4);
  b[0]=dst.boot[21];b[1]=b[2]=255;if(dst.bits==16)b[3]=255;
  if(sec(m,1,b,1)||sec(m,1+dst.spf,b,1)||sec(m,0,dst.boot,1))return error("Format failure",4);
  line("Format complete");unsigned used=0;
  if(o->s||o->b){
   if(transfer(&dst,&src,count,1,&used))return error("No room for system on destination disk",4);
   if(o->b) {
    /* UG p74: /B leaves room for a later operating system. Reserve the
     * two HSR root slots and their clusters; no executable system content.
     * SYS p119 can replace these reservations without moving other files. */
    uint8_t empty[512];zero(empty,512);
    for(unsigned i=0;i<2;i++){
     uint8_t e[32];if(entry(&dst,i,e,0))return error("Format failure",4);
     unsigned c=u16(e+26),n=file_clusters(&dst,e);
     for(unsigned j=0;j<n*dst.spc;j++)if(sec(m,dst.data+(c-2)*dst.spc+j,empty,1))return error("Format failure",4);
    }
   }else line("System transferred");}
  if(o->v){put("Volume label (11 characters, ENTER for none)?");int n=input(answer,11);if(n<0)return 3;
   if(n){uint8_t e[32];zero(e,32);for(unsigned i=0;i<11;i++)e[i]=i<(unsigned)n?(uint8_t)answer[i]:' ';e[11]=8;w16(e+24,0x21);if(entry(&dst,o->s?3:0,e,1))return error("Format failure",4);cp(dst.boot+43,e,11);if(sec(m,0,dst.boot,1))return error("Format failure",4);}
  }
  unsigned capacity=dst.clusters*dst.spc*512;
  num(capacity);line(" bytes total disk space");if(o->s){num(used);line(" bytes used by system");}num(capacity-used);line(" bytes available on disk");
  if(!(m->flags&DU_REMOVABLE))return 0;
  put("Format another (Y/N)?");if(!yes())return 0;
 }
}
static int sys_run(const du_map *m,unsigned letter){transfer_source src;volume dst;unsigned used;du_map sm;
 if(fat16_target(m))return protected(letter);
 /* Genuine DOS 3.30 (sys-self) refuses its source drive with Invalid
  * drive specification. The Initech source is fixed at C: (DEC-18), so
  * refuse exactly that volume; any other volume stays transferable. */
 if(!io->map(2,&sm)&&same_volume(&sm,m))return error(MSG_DOS_0004,4);
 if(source(&src,3)||mount(&dst,m))return error("No room for system on destination disk",1);
 if(src.split&&same_volume(&src.command.m,m))return error(MSG_DOS_0004,4);
 /* Preserve destination BPB, use the existing InitechDOS VBR code. */
 cp(dst.boot,src.system.boot,11);cp(dst.boot+62,src.system.boot+62,450);
 if(transfer(&dst,&src,3,0,&used))return error("No room for system on destination disk",1);
 return error("System transferred",0);
}
/* Genuine DOS 3.30 (dos33/reference/diskcopy-break): Ctrl-C or Ctrl-Break
 * (byte 03h through the adapter) at a swap prompt echoes ^C, exits the
 * utility and writes nothing further. Returns -1 on that cancel. */
static int swap_prompt(int compare,int target,unsigned letter){
 drive_text(compare?(target?"Insert SECOND volume into drive ":"Insert FIRST volume into drive "):(target?"Insert TARGET volume into drive ":"Insert SOURCE volume in drive "),letter,":\r\n");
 if(!compare)line("Press any key when ready . . .");
 if(io->get()==3){line("^C");return -1;}
 return 0;
}
static int copy_run(const options *o,int compare){
 du_map m[2];for(unsigned i=0;i<2;i++){if(io->map(o->letter[i],m+i))return error(MSG_DOS_0004,4);}
 if(!(m[1].flags&DU_REMOVABLE)&&fat16_target(m+1))return protected(o->letter[1]);
 for(unsigned i=0;i<2;i++)if(!(m[i].flags&DU_REMOVABLE))return error("Specified drive does not exist,\r\nor is non-removable",4);
 int result=0;
 do{
  if(swap_prompt(compare,0,o->letter[0]))return 3;
  uint8_t b[512];if(sec(m,0,b,0))return error("FIRST volume bad or incompatible",3);
  unsigned total=u16(b+19),spt=u16(b+24),heads=u16(b+26);if(u16(b+11)!=512||!total||total>m[0].sectors||!spt||spt>63||!heads||heads>2)return error("FIRST volume bad or incompatible",3);
  unsigned count=sizeof copybuf/512;
  result=0;
  for(unsigned first=0;first<total;first+=count){unsigned n=total-first;if(n>count)n=count;
   if(first&&swap_prompt(compare,0,o->letter[0]))return 3;
   for(unsigned j=0;j<n;j++)if(sec(m,first+j,copybuf+j*512,0))return error("FIRST volume bad or incompatible",3);
   if(swap_prompt(compare,1,o->letter[1]))return 3;
   if(fat16_target(m+1))return protected(o->letter[1]);
   for(unsigned j=0;j<n;j++){
    unsigned l=first+j,side=(l/spt)%heads,track=l/(spt*heads);
    if((o->one&&side)||(o->eight&&l%spt>=8))continue;
    if(compare){if(sec(m+1,l,b,0))return error("Compare process ended",3);if(!eq(b,copybuf+j*512,512)){put("Compare error on disk\r\nside ");num(side);put(", track ");num(track);put("\r\n");result=1;}}
    else if(sec(m+1,l,copybuf+j*512,1))return error("Target volume may be unusable",3);
   }
  }
  if(compare&&!result)line("Compare OK");
  put(compare?"Compare another volume (Y/N)?":"Copy another volume (Y/N)?");
 }while(yes());
 return result;
}
int diskutil_run(const du_io *ops,unsigned kind,const char *tail){io=ops;options o;if(options_parse(&o,kind,tail)) {
  /* Genuine compat33/U09/U10 retain the complete refusal, including BEL. */
  if(kind>=2){put("\r\n");line(MSG_DOS_0064);line("Do not specify filename(s)");
   line(kind==2?"Command Format: DISKCOPY d: d: [/1]":"Command format: DISKCOMP d: d:[/1][/8]");put("\007");return 4;}
  return error(MSG_DOS_0064,4);
 }
 if(kind>=2)return copy_run(&o,kind==3);
 du_map m;if(io->map(o.letter[0],&m))return error(MSG_DOS_0004,4);
 return kind?sys_run(&m,o.letter[0]):format_run(&o,&m);
}
