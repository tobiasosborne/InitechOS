/* FDISK primary-partition editor. Ref: IBM DOS 3.30 TR pp9-6..9;
 * MS-DOS 3.3 User's Guide pp287..295 (menus, columns, confirmation/restart).
 * MS-DOS UG p353: primary-already-exists message.
 * DEC-17 PM ruling adds type06 and the loaded-volume refusal. Factory tests
 * compare this SAME core with mpartition/sfdisk on identical images. */
#include "fdisk.h"
#define CYL_SECTORS 1008u /* DEC-17 fixed 16*63, not the mounted BPB */
static uint32_t r32(const uint8_t *p)
{ return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void w32(uint8_t *p,uint32_t v)
{ for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
static int dos(unsigned t){return t==1 || t==4 || t==6;}
static uint8_t *entry(uint8_t *m,unsigned i){return m+446+16*i;}
static void signature(uint8_t *m)
{
    m[510]=0x55;m[511]=0xaa;
#ifdef FDISK_MUT_SIGNATURE
    m[510]=0;
#endif
}
static void chs(uint8_t *p,uint32_t lba)
{
    uint32_t c=lba/CYL_SECTORS,h=(lba/63)%16,s=lba%63+1;
    p[0]=(uint8_t)h;p[1]=(uint8_t)(s|((c>>2)&0xc0));p[2]=(uint8_t)c;
}
static unsigned cylinders(uint32_t n)
{ unsigned c=n/CYL_SECTORS;return c>1024?1024:c; }
static int gap(uint8_t *m,uint32_t n,unsigned *first,unsigned *size)
{
    unsigned total=cylinders(n),run=0,best=0,start=0;
    for(unsigned c=0;c<total;c++) {
        uint32_t lo=c?c*CYL_SECTORS:63,hi=(c+1)*CYL_SECTORS;
        int used=0;
        for(unsigned i=0;i<4;i++) {
            uint8_t *p=entry(m,i);if(!p[4])continue;
            uint32_t a=r32(p+8),z=r32(p+12);
            if(!z || a>=n || z>n-a)return -1;
            if(a<hi && a+z>lo)used=1;
        }
        if(used)run=0;
        else {
            if(!run)start=c;
            run++;
            if(run>best){best=run;*first=start;}
        }
    }
    *size=best;return best?0:-1;
}
static void mark_active(uint8_t *m,unsigned partition)
{
    for(unsigned i=0;i<4;i++)entry(m,i)[0]=0;
    entry(m,partition-1)[0]=0x80;
}
int fdisk_active(uint8_t m[512],unsigned partition)
{
    if(!partition || partition>4 || !entry(m,partition-1)[4])return -1;
#ifdef FDISK_MUT_ACTIVE
    partition=partition%4+1;
#endif
    mark_active(m,partition);signature(m);return 0;
}
int fdisk_create(uint8_t m[512],uint32_t n,unsigned count,int active)
{
    unsigned slot=4,first=0,available=0;
    for(unsigned i=0;i<4;i++) {
        if(dos(entry(m,i)[4]))return -1;
        if(slot==4 && !entry(m,i)[4])slot=i;
    }
    if(slot==4 || gap(m,n,&first,&available))return -2;
    if(!count)count=available;
    if(count>available)return -3;
    uint32_t start=first?first*CYL_SECTORS:63,end=(first+count)*CYL_SECTORS;
    uint32_t length=end-start;
    uint8_t *p=entry(m,slot);for(unsigned j=0;j<16;j++)p[j]=0;
    /* mpartition's independent type-size policy (host differential): type01
     * through 4095 sectors, 04 through 65535, 06 above (PM subset decision). */
    p[4]=length<4096?1:length<65536?4:6;
    chs(p+1,start);chs(p+5,end-1);
#ifdef FDISK_MUT_CHS
    p[5]--;
#endif
    w32(p+8,start);w32(p+12,length);signature(m);
    if(active)mark_active(m,slot+1);
    return 0;
}
int fdisk_delete(uint8_t m[512],int confirmed)
{
#ifndef FDISK_MUT_CONFIRM
    if(!confirmed)return 0;
#else
    (void)confirmed;
#endif
    for(unsigned i=0;i<4;i++)if(dos(entry(m,i)[4])) {
        for(unsigned j=0;j<16;j++)entry(m,i)[j]=0;
        signature(m);return 0;
    }
    return -1;
}
static const fdisk_io *io;
static uint8_t mbr[512];
static unsigned drive;
static uint32_t sectors;
static void put(const char *s){io->put(s);}
static void number(unsigned n)
{
    char b[12];unsigned i=sizeof b;b[--i]=0;
    do{b[--i]=(char)('0'+n%10);n/=10;}while(n);
    put(b+i);
}
static void heading(const char *title)
{
    put("\r\n");put(title);put("\r\n\r\nCurrent Fixed Disk Drive: ");
    number(drive-0x7f);put("\r\n\r\n");
}
static void options(void){put("Press ESC to return to Fdisk Options\r\n");}
static void rownum(char *b,unsigned end,unsigned n)
{ do{b[end--]=(char)('0'+n%10);n/=10;}while(n); }
static void table(void)
{
    put("Partition  Status   Type  Start  End  Size\r\n");
    for(unsigned i=0;i<4;i++) {
        uint8_t *p=entry(mbr,i);if(!p[4])continue;
        char b[46];for(unsigned j=0;j<43;j++)b[j]=' ';
        b[43]='\r';b[44]='\n';b[45]=0;
        if(dos(p[4])){b[1]='C';b[2]=':';}
        b[5]=(char)('1'+i);if(p[0]==0x80)b[13]='A';
        const char *type=dos(p[4])?"PRI DOS":p[4]==5?"EXT DOS":"non DOS";
        for(unsigned j=0;j<7;j++)b[18+j]=type[j];
        unsigned a=p[3]|((p[2]&192u)<<2),z=p[7]|((p[6]&192u)<<2);
        rownum(b,30,a);rownum(b,36,z);rownum(b,42,z>=a?z-a+1:0);
        put(b);
    }
    put("\r\nTotal disk space is ");number(cylinders(sectors));put(" cylinders.\r\n\r\n");
}
/* Raw console reads, default values accepted with Return; ESC aborts any menu.
 * MS-DOS UG p287. Echo here because AH07 deliberately does not echo. */
static int line(char out[9])
{
    unsigned n=0;
    for(;;) {
        int c=io->get();if(c<0 || c==27)return -1;
        if(c=='\r' || c=='\n'){out[n]=0;put("\r\n");return (int)n;}
        if(c==8 && n){n--;put("\b \b");}
        else if(c>=32 && c<127 && n<8) {
            out[n++]=(char)c;char s[2]={(char)c,0};put(s);
        }
    }
}
static int choice(unsigned def)
{
    char b[9];int n=line(b);if(n<0)return -1;
    if(!n)return (int)def;
    unsigned v=0;for(int i=0;i<n;i++){if(b[i]<'0'||b[i]>'9')return 0;v=v*10+(unsigned)(b[i]-'0');}
    return (int)v;
}
static int yes(int def)
{ char b[9];int n=line(b);return n<0?-1:!n?def:b[0]=='y'||b[0]=='Y'; }
static void waitesc(void){while(io->get()!=27){} }
static int save(void){return io->write(drive,mbr);}
int fdisk_run(const fdisk_io *ops)
{
    io=ops;drive=0x80;int changed=0;
    for(;;) {
        /* PM ruling: loaded volume is not an editing target. The present
         * single-volume loader is bound exclusively to ATA primary slave. */
#ifndef FDISK_MUT_LOADED
        if(drive==io->loaded_drive)return 1;
#endif
        sectors=io->capacity(drive);
        if(!sectors || io->read(drive,mbr))return 1;
        heading("Disk Options");
        put("Choose one of the following:\r\n\r\n"
            "    1. Create DOS Partition\r\n"
            "    2. Change Active Partition\r\n"
            "    3. Delete DOS Partition\r\n"
            "    4. Display Partition Data\r\n");
        int second=io->capacity(0x81)!=0;
        if(second)put("    5. Select Next Fixed Disk Drive\r\n");
        put("\r\nEnter choice:    [1]\r\n\r\nPress ESC to return to DOS\r\n");
        int c=choice(1);
        if(c<0) {
            if(changed) {
                put("\r\nSystem will now restart\r\n\r\nInsert DOS diskette in drive A:\r\nPress any key when ready . . .\r\n");
                io->get();
                if(io->restart)io->restart();
            }
            return 0;
        }
        if(c==5 && second){drive=drive==0x80?0x81:0x80;continue;}
        if(c==4){heading("Display Partition Information");table();options();waitesc();continue;}
        if(c==2) {
            heading("Change Active Partition");table();
            put("Enter the number of the partition you\r\nwant to make active..................: [1]\r\n");options();
            int n=choice(1);if(n<0)continue;
            if(!fdisk_active(mbr,(unsigned)n)){if(save())return 1;changed=1;}
            options();waitesc();continue;
        }
        if(c==3) {
            heading("Delete DOS Partition");
            put("    1. Delete Primary DOS Partition\r\n    2. Delete Extended DOS Partition\r\n"
                "    3. Delete Logical DOS Drive(s) in\r\n       the Extended DOS Partition\r\n\r\nEnter choice:    [ ]\r\n");options();
            if(choice(0)!=1)continue;
            heading("Delete Primary DOS Partition");table();
            put("Warning! Data in Primary DOS\r\npartition will be lost. Do you wish\r\nto continue.........................?  [N]\r\n\r\n");options();
            int y=yes(0);if(y<0)continue;
            uint8_t before[64];for(unsigned i=0;i<64;i++)before[i]=mbr[446+i];
            /* Pass No to the mutation-proven edit primitive as well. */
            if(!fdisk_delete(mbr,y)) {
                int altered=0;
                for(unsigned i=0;i<64;i++)if(before[i]!=mbr[446+i])altered=1;
                if(altered){if(save())return 1;changed=1;}
            }
            options();waitesc();continue;
        }
        if(c==1) {
            heading("Create DOS Partition");
            put("    1. Create Primary DOS Partition\r\n    2. Create Extended DOS Partition\r\n\r\nEnter choice:    [1]\r\n");options();
            if(choice(1)!=1)continue;
            heading("Create Primary DOS Partition");
            int exists=0;for(unsigned i=0;i<4;i++)if(dos(entry(mbr,i)[4]))exists=1;
            if(exists){put("Primary DOS partition already exists\r\n");options();waitesc();continue;}
            put("Do you wish to use the maximum size\r\nfor a DOS partition and make the DOS\r\npartition active (Y/N).........? [Y]\r\n\r\n");options();
            int y=yes(1);if(y<0)continue;
            unsigned n=0;
            if(!y) {
                unsigned first=0,available=0;
                if(gap(mbr,sectors,&first,&available))continue;
                heading("Create Primary DOS Partition");table();
                put("Maximum space available for partition\r\nis ");number(available);put(" cylinders.\r\n\r\nEnter partition size............ [ ");number(available);put("]\r\n");options();
                int size=choice(available);if(size<=0)continue;n=(unsigned)size;
            }
            if(!fdisk_create(mbr,sectors,n,y)) {
                if(save())return 1;
                changed=1;
                if(y) {
                    put("\r\nSystem will now restart\r\n\r\nInsert DOS diskette in drive A:\r\nPress any key when ready . . .\r\n");
                    io->get();if(io->restart)io->restart();return 0;
                }
            }
            options();waitesc();
        }
    }
}
