/* CHKDSK engine (artifact, freestanding C).
 * Law 1: MS-DOS 3.3 User's Guide p43 (/F,/V/report), pp318-319
 * (fragmentation/prompt/read-only), p334 (invalid start), p344 (lost chains).
 * IBM DOS Reference pp7-48..51 (paths, columns, FILE0000.CHK), A-46
 * (report BOTH cross-linked files, never automatically repair a cross-link).
 * Disk encoding: ADR-0003 B.1/DEC-07; docs/research/fat12-ground-truth.md 1-4;
 * FAT16: docs/research/fat16-ground-truth.md; memory: ADR-0003 DEC-03/B.3,
 * os/milton/mcb.c. No host services, timestamps, or embedded host paths.
 * Stream sectors; six 8KiB bitmaps cover FAT16 without a geometry-sized stack.
 */
#include "chkdsk.h"
#include "dos_messages.h" /* DEC-13/App C; MS-DOS 3.3 pp.43,330. */
#include "dospath.h"     /* q45q: drive-qualified operands (DEC-18). */

#define MAP 8192u
typedef struct scan {
    chkdsk_io_t *io;
    uint32_t fat, spf, root, data, clusters, total, roots, spc;
    uint32_t free, hidden_bytes, dir_bytes, user_bytes, hidden, dirs, users;
    uint32_t lost, chains, matched, fragmented, cache_lba;
    uint16_t eoc, bad, reserved;
    uint8_t bits, fats, fix, verbose, error, failed, incomplete, pass;
    uint8_t cache[512];
    uint8_t owned[MAP], duplicate[MAP], local[MAP], seen[MAP], incoming[MAP], visited[MAP];
    char operand[128], path[128];
} scan_t;
/* Static storage belongs to this .COM; crt0 clears it on EVERY launch. */
static scan_t s;
static void zero(void *v,uint32_t n) {uint8_t*p=v;while(n--)*p++=0;}
static void copy(void *v,const void *a,uint32_t n){uint8_t*d=v;const uint8_t*p=a;while(n--)*d++=*p++;}
static uint32_t len(const char*p){uint32_t n=0;while(p[n])n++;return n;}
static uint16_t u16(const uint8_t*p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t u32(const uint8_t*p){return u16(p)|((uint32_t)u16(p+2)<<16);}
static void w16(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void w32(uint8_t*p,uint32_t v){w16(p,v);w16(p+2,v>>16);}
static int bit(const uint8_t*p,uint32_t c){return (p[c>>3]>>(c&7))&1;}
static void mark(uint8_t*p,uint32_t c){p[c>>3]|=(uint8_t)(1u<<(c&7));}
static int valid(uint32_t c){return c>=2 && c<s.clusters+2 && c<s.reserved;}
static void put(const char*p){s.io->put(s.io->ctx,p);}
static void nl(void){put("\r\n");}
static void num(uint32_t v,uint32_t width) {
    char b[16];uint32_t n=0;do{b[n++]=(char)('0'+v%10);v/=10;}while(v);
    while(width>n){put(" ");width--;}
    while(n){char c[2]={b[--n],0};put(c);}
}
static void line(uint32_t n,const char*t){num(n,9);put(t);nl();}
static int read_sector(uint32_t lba,uint8_t*p) {
    if(lba>=s.total || s.io->disk.read_sectors(s.io->disk.ctx,lba,1,p)) {
        s.failed=1;return -1;
    }return 0;
}
static int write_sector(uint32_t lba,const uint8_t*p) {
#ifndef CHKDSK_MUT_NOFIX
    if(!s.fix)return 0;
#endif
    if(lba>=s.total || !s.io->disk.write_sectors ||
       s.io->disk.write_sectors(s.io->disk.ctx,lba,1,p)){s.failed=1;return -1;}
    s.cache_lba=0xffffffffu;return 0;
}
static uint8_t fat_byte(uint32_t o) {
    uint32_t l=s.fat+o/512;
    if(s.cache_lba!=l){if(read_sector(l,s.cache))return 0;s.cache_lba=l;}
    return s.cache[o%512];
}
static uint16_t next(uint32_t c) {
    uint32_t o=s.bits==12?c+c/2:c*2;
    uint16_t v=fat_byte(o);v|=(uint16_t)fat_byte(o+1)<<8;
    return s.bits==16?v:(c&1?v>>4:v&4095);
}
static int fat_set(uint32_t c,uint16_t v) {
    uint32_t o=s.bits==12?c+c/2:c*2;
    uint8_t p[512];
    unsigned copies=s.fats;
#ifdef CHKDSK_MUT_MIRROR
    copies=1;
#endif
    /* Read-modify-write each byte separately: FAT12 entry 341 straddles a
     * sector; preserve the adjacent packed nibble in EVERY redundant FAT. */
    for(unsigned k=0;k<copies;k++)for(unsigned j=0;j<2;j++) {
        uint32_t l=s.fat+k*s.spf+(o+j)/512,at=(o+j)%512;
        if(read_sector(l,p))return -1;
        if(s.bits==16)p[at]=(uint8_t)(v>>(j*8));
        else if(c&1)p[at]=j?(uint8_t)(v>>4):(uint8_t)((p[at]&15)|(v<<4));
        else p[at]=j?(uint8_t)((p[at]&240)|(v>>8)):(uint8_t)v;
        if(write_sector(l,p))return -1;
    }return 0;
}
static uint32_t cluster_lba(uint32_t c){return s.data+(c-2)*s.spc;}
static int allocated(uint16_t v){return v!=0 && v!=s.bad && !(v>=s.reserved && v<s.eoc);}
static void name83(const uint8_t*e,char*p){unsigned n=0;for(unsigned i=0;i<8&&e[i]!=' ';i++)p[n++]=(char)e[i];
    if(e[8]!=' '){p[n++]='.';for(unsigned i=8;i<11&&e[i]!=' ';i++)p[n++]=(char)e[i];}p[n]=0;}
static char upper(char c){return c>='a'&&c<='z'?(char)(c-32):c;}
static int equal(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static int wild(const char*p,const char*n) {
    if(equal(p,"*.*"))return 1;
    while(*p){if(*p=='*'){p++;if(!*p)return 1;do{if(wild(p,n))return 1;}while(*n++);return 0;}
        if(!*n || (*p!='?'&&*p!=*n))return 0;
        p++;n++;}return !*n;
}
static int selected(void) {
    if(!s.operand[0])return 0;
    const char *p=s.operand,*n=s.path;
    /* '*' does not consume a directory separator: compare parent separately. */
    uint32_t a=len(p),b=len(n);while(a&&p[a-1]!='\\')a--;while(b&&n[b-1]!='\\')b--;
    if(a!=b)return 0;
    for(uint32_t i=0;i<a;i++)if(p[i]!=n[i])return 0;
    return wild(p+a,n+b);
}
#ifndef CHKDSK_MUT_INVALID
static void diagnostic(const char*t){put(s.path);nl();put(t);nl();s.error=1;}
#endif
static uint32_t walk_file(uint8_t*e,uint32_t lba,uint32_t at) {
    uint32_t c=u16(e+26),previous=0,units=0,runs=0;int invalid=0,changed=0;
    zero(s.local,MAP);
    if(c==0 && u32(e+28)==0)return 0;
    if(!valid(c)){invalid=1;c=0;}
    while(valid(c)&&!s.failed) {
        if(bit(s.local,c)){invalid=1;break;}
        mark(s.local,c);
        if(s.pass==0){
            if(bit(s.owned,c))mark(s.duplicate,c);
            mark(s.owned,c);
        }
        if(!previous||c!=previous+1)runs++;
        units++;previous=c;
        uint16_t v=next(c);
        if(v>=s.eoc)break;
        if(!valid(v)){invalid=1;break;}
        c=v;
    }
    if(s.pass==0 && invalid) {
#ifndef CHKDSK_MUT_INVALID
        diagnostic(MSG_DOS_0042);
#endif
        if(s.fix) {
            if(previous){if(fat_set(previous,s.eoc))return units;}
            else {w16(e+26,0);units=0;}
            uint32_t cap=units*s.spc*512;if(u32(e+28)>cap || !previous)w32(e+28,cap);
            changed=1;
        }
    }
    if(changed){uint8_t p[512];if(!read_sector(lba,p)){copy(p+at,e,32);(void)write_sector(lba,p);}}
    if(s.pass==1) {
#ifndef CHKDSK_MUT_CROSS
        /* Second pass names BOTH owners of each shared cluster. A-46 explicitly
         * forbids automatic correction, even with /F. */
        for(uint32_t n=2;n<s.clusters+2;n++)if(bit(s.local,n)&&bit(s.duplicate,n)){
            put(s.path);nl();put(MSG_DOS_0043);num(n,0);nl();s.error=1;
        }
#endif
        if(selected()){
            s.matched++;
#ifdef CHKDSK_MUT_FRAG
            runs=1;
#endif
            if(runs>1){s.fragmented++;put(s.path);put("\r\n     Contains ");num(runs,0);put(" non-contiguous blocks\r\n");}
        }
    }
    return units;
}
static void volume(const uint8_t*e) {
    char label[12];copy(label,e,11);label[11]=0;
    uint16_t date=u16(e+24),time=u16(e+22);unsigned month=(date>>5)&15;
    static const char *months[]={"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
    put("Volume ");put(label);put("  Created ");put(months[month>=1&&month<=12?month-1:0]);put(" ");
    num((date&31)?date&31:1,2);put(", ");num(1980+(date>>9),4);put(" ");num(time>>11,2);put(":");
    unsigned minutes=(time>>5)&63;if(minutes<10)put("0");num(minutes,0);nl();nl();
}
static void directory(uint32_t start,unsigned depth) {
    if(depth>32){s.incomplete=1;s.failed=1;return;}
    if(start){if(!valid(start)||bit(s.visited,start)){s.incomplete=1;return;}mark(s.visited,start);}
    uint32_t c=start,steps=0,index=0,base_len=len(s.path);
    do {
        uint32_t sectors=start?s.spc:(s.roots*32+511)/512;
        uint32_t first=start?cluster_lba(c):s.root;
        for(uint32_t j=0;j<sectors&&!s.failed;j++)for(unsigned at=0;at<512;at+=32,index++) {
            uint8_t p[512],e[32];char name[13];
            if(!start&&index>=s.roots)return;
            if(read_sector(first+j,p))return;
            copy(e,p+at,32);
            if(!e[0])return;
            if(e[0]==0xe5||e[11]==15||e[0]=='.')continue;
            if(e[11]&8){if(s.pass==0){s.hidden++;volume(e);}continue;}
            name83(e,name);uint32_t n=len(name);
            if(base_len+n+2>=sizeof(s.path)){s.incomplete=1;continue;}
            copy(s.path+base_len,name,n+1);
            if(s.pass==0 && s.verbose && !(e[11]&16)) {
#ifndef CHKDSK_MUT_VERBOSE
                put(s.path);nl();
#endif
            }
            uint32_t units=walk_file(e,first+j,at);
            if(s.pass==0) {
                if(e[11]&16){s.dirs++;s.dir_bytes+=units*s.spc*512;}
                else if(e[11]&2){s.hidden++;s.hidden_bytes+=units*s.spc*512;}
                else{s.users++;s.user_bytes+=units*s.spc*512;}
            }
            if(e[11]&16){
                if(!valid(u16(e+26))){
                    if(s.pass==0){put(MSG_DOS_0044);put(s.path);put(MSG_DOS_0045 "\r\n");}
                    s.incomplete=1;
                }else{s.path[base_len+n]='\\';s.path[base_len+n+1]=0;directory(u16(e+26),depth+1);}
            }
            s.path[base_len]=0;
        }
        if(!start)break;
        c=next(c);if(c>=s.eoc)break;
        if(!valid(c)||++steps>s.clusters){s.incomplete=1;return;}
    }while(!s.failed);
}
static int orphan(uint32_t c){return valid(c)&&!bit(s.owned,c)&&allocated(next(c));}
static uint32_t lost_chain(uint32_t head,int action) {
    uint32_t c=head,n=0,previous=0;
    while(orphan(c)&&!bit(s.seen,c)&&!s.failed) {
        uint16_t v=next(c);mark(s.seen,c);n++;previous=c;
        if(action==2)(void)fat_set(c,0);
        c=v;
    }
    if(action==1 && previous && next(previous)<s.eoc)(void)fat_set(previous,s.eoc);
    return n;
}
static int recover(uint32_t head,uint32_t units) {
    uint8_t p[512],e[32];char name[12]="FILE0000CHK";unsigned number;
    for(number=0;number<10000;number++) {
        unsigned v=number;for(int i=7;i>=4;i--){name[i]=(char)('0'+v%10);v/=10;}
        int exists=0;
        for(uint32_t i=0;i<s.roots;i++) {
            if(read_sector(s.root+i/16,p))return -1;
            uint8_t *a=p+(i%16)*32;
            if(!a[0])break;
            if(a[0]!=0xe5){unsigned k=0;while(k<11&&a[k]==(uint8_t)name[k])k++;if(k==11){exists=1;break;}}
        }
        if(!exists)break;
    }
    if(number==10000){s.failed=1;return -1;}
    for(uint32_t i=0;i<s.roots;i++) {
        if(read_sector(s.root+i/16,p))return -1;
        uint8_t*a=p+(i%16)*32;if(a[0]&&a[0]!=0xe5)continue;
        zero(e,32);copy(e,name,11);w16(e+26,head);w32(e+28,units*s.spc*512);
        copy(a,e,32);return write_sector(s.root+i/16,p);
    }
    put(MSG_DOS_0046 "\r\n");s.failed=1;return -1;
}
static int answer(void) {
    /* IBM 7-49: Y/N must be followed by Enter. Redirected handle 0 is valid. */
    int a=0,c;
    while((c=s.io->get(s.io->ctx))>=0){if(c=='\r'||c=='\n'){if(a)return a;}else if(!a&&(upper((char)c)=='Y'||upper((char)c)=='N'))a=upper((char)c);else a=0;}
    return 'N';
}
static void lost(void) {
#ifdef CHKDSK_MUT_LOST
    return;
#endif
    for(uint32_t c=2;c<s.clusters+2&&!s.failed;c++)if(orphan(c)) {
        s.lost++;uint16_t v=next(c);if(valid(v)&&orphan(v))mark(s.incoming,v);
    }
    for(unsigned pass=0;pass<2;pass++)for(uint32_t c=2;c<s.clusters+2&&!s.failed;c++)
        if(orphan(c)&&!bit(s.seen,c)&&(pass||!bit(s.incoming,c))){s.chains++;(void)lost_chain(c,0);}
    if(!s.lost||s.failed||s.incomplete)return;
    s.error=1;num(s.lost,0);put(" lost allocation units found in ");num(s.chains,0);put(" chains.\r\nConvert lost chains to files (Y/N)?");
    int a=answer();nl();
    if(!s.fix){num(s.lost*s.spc*512,0);put(" bytes would be freed\r\n");}
#ifndef CHKDSK_MUT_NOFIX
    if(!s.fix)return;
#endif
#ifdef CHKDSK_MUT_RECOVER
    if(a=='Y')a='N';
#endif
#ifdef CHKDSK_MUT_DISCARD
    if(a=='N')return;
#endif
    zero(s.seen,MAP);
    for(unsigned pass=0;pass<2;pass++)for(uint32_t c=2;c<s.clusters+2&&!s.failed;c++)
        if(orphan(c)&&!bit(s.seen,c)&&(pass||!bit(s.incoming,c))) {
            uint32_t n=lost_chain(c,a=='Y'?1:2);
            if(a=='Y'){
                if(recover(c,n))return;
                s.users++;s.user_bytes+=n*s.spc*512;
            }
        }
}
static int parse(const char*t) {
    char raw[128];unsigned n=0;int operand=0;
    while(*t){if(*t==' '||*t=='\t'){t++;continue;}
        if(*t=='/'){char c=upper(*++t);if(!c)return -1;t++;if(c=='F')s.fix=1;else if(c=='V')s.verbose=1;else return -1;
            if(*t&&*t!='/'&&*t!=' '&&*t!='\t')return -1;
        }else{if(operand++)return -1;while(*t&&*t!=' '&&*t!='\t'&&*t!='/'){if(n+1>=sizeof(raw))return -1;raw[n++]=upper(*t++);}raw[n]=0;}
    }
    if(!n)return 0;
    const char*p=raw;if(n>=2&&p[1]==':'){if(p[0]<'A'||p[0]>'Z')return -1;s.io->drive=(uint8_t)(p[0]-'A');p+=2;}
    if(!*p)return 0;
    /* CHKDSK [drive:][pathname] (MS-DOS 3.3 Reference p.43): a relative
     * pathname starts in THAT drive's directory, not the default's. */
    int rc=dospath_full(raw,s.io->drive,s.io->cwd,s.operand,sizeof s.operand);
    return rc==DOSPATH_BAD_DRIVE?-2:rc?-1:0;
}
int chkdsk_run(chkdsk_io_t *io,const char*tail) {
    zero(&s,sizeof(s));s.io=io;s.cache_lba=0xffffffffu;s.total=1;
    int bad=parse(tail);
    if(bad){put(bad==-2?MSG_DOS_0004 "\r\n":MSG_DOS_0048 "\r\n");return -1;}
    uint8_t b[512];if(read_sector(0,b))goto failed;
    s.spc=b[13];s.fat=u16(b+14);s.fats=b[16];s.roots=u16(b+17);s.spf=u16(b+22);
    s.total=u16(b+19)?u16(b+19):u32(b+32);s.root=s.fat+s.fats*s.spf;
    s.data=s.root+(s.roots*32+511)/512;
    if(u16(b+11)!=512||!s.spc||(s.spc&(s.spc-1))||!s.fat||!s.fats||s.fats>2||!s.spf||!s.roots||s.data>=s.total)goto failed;
    s.clusters=(s.total-s.data)/s.spc;
    if(!s.clusters||s.clusters>=65525)goto failed;
    s.bits=s.clusters<4085?12:16;s.eoc=s.bits==12?0xff8:0xfff8;s.bad=s.bits==12?0xff7:0xfff7;s.reserved=s.bits==12?0xff0:0xfff0;
    if((s.clusters+2)*(s.bits==12?3u:4u)>s.spf*1024)goto failed;
    s.path[0]=(char)('A'+io->drive);s.path[1]=':';s.path[2]='\\';s.path[3]=0;
    directory(0,0);s.pass=1;zero(s.visited,MAP);directory(0,0);
    if(s.operand[0]&&!s.matched){put(MSG_DOS_0003 "\r\n");s.error=1;}
    else if(s.operand[0]&&!s.fragmented)put("All specified file(s) are contiguous\r\n");
    lost();
    if(s.error&&!s.fix)put(MSG_DOS_0047 "\r\n");
    for(uint32_t c=2;c<s.clusters+2&&!s.failed;c++)if(next(c)==0)s.free++;
#ifdef CHKDSK_MUT_REPORT
    s.hidden=0;
#endif
    line(s.clusters*s.spc*512," bytes total disk space");
    num(s.hidden_bytes,9);put(" bytes in ");num(s.hidden,0);put(" hidden files\r\n");
    num(s.dir_bytes,9);put(" bytes in ");num(s.dirs,0);put(" directories\r\n");
    num(s.user_bytes,9);put(" bytes in ");num(s.users,0);put(" user files\r\n");
    line(s.free*s.spc*512," bytes available on disk");nl();
    if(!mcb_chain_intact(&io->arena))goto failed;
    uint32_t total=0,free=0,at=0;
    /* Use the live chain, including fragmented arenas and non-free owners.
     * Memory figures are DATA bytes, exactly mcb.c's paragraph accounting. */
    while(at<io->arena.total_paras){mcb_t*m=(mcb_t*)(io->arena.base+at*16);total+=(uint32_t)m->size_paras*16;
        if(m->owner==MCB_OWNER_FREE)free+=(uint32_t)m->size_paras*16;
        at+=1u+m->size_paras;}
#ifdef CHKDSK_MUT_MEMORY
    free=total=655360;
#endif
    line(total," bytes total memory");line(free,MSG_DOS_0025);
    if(s.failed)goto failed;
    return s.error?1:0;
failed:
    put(MSG_DOS_0049 "\r\n");return -1;
}
