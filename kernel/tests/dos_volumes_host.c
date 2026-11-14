/* Factory differential, real INT21 + FAT backend on identical mtools images.
 * Ref: ADR-0003 Appendix A, DEC-18; MS-DOS 3.3 UG pp24-26, Reference
 * pp4-11,41-42,50,57-58,340. Expected file bytes come from mtype, below.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "int21.h"
#include "psp.h"
#include "sft.h"
#include "fileio_fat.h"
#include "blockdev_file.h"
#include "find_data.h"

/* A missing seam must fail at runtime on the pre-implementation tree. */
extern int fileio_fat_add_drive(unsigned, fat12_volume_t *) __attribute__((weak));
static psp_t process;
static char path[128], data[4096], cwd[64];
static find_data_t dta;
static void need(int yes, const char *rule)
{ if (!yes) { fprintf(stderr,"RED dos-volumes %s\n",rule); exit(1); } }
static int_frame_t call(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    int_frame_t f={0}; f.eax=ax; f.ebx=bx; f.ecx=cx; f.edx=(uint32_t)dx;
    if (ax==0x4700) f.esi=(uint32_t)(uintptr_t)cwd;
    int21_dispatch(&f); return f;
}
static unsigned ok(unsigned ax,unsigned bx,unsigned cx,uintptr_t dx,const char *rule)
{ int_frame_t f=call(ax,bx,cx,dx); if(f.eflags&1) fprintf(stderr,"INT21 %04X error=%u path=%s\n",ax,f.eax&65535,path); need(!(f.eflags&1),rule); return f.eax; }
static unsigned open_path(const char *s)
{
    strcpy(path,s); unsigned h=ok(0x3d00,0,0,(uintptr_t)path,"PATH open");
    int_frame_t f=call(0x4400,h,0,0);
    unsigned drive=(s[0]=='C'||s[0]=='c')?2:s[0]=='A'||s[0]=='a'?0:int21_current_drive();
    need(!(f.eflags&1) && (f.edx&0xbf)==drive,"i1nu4 IOCTL file drive identity");
    return h;
}
static void cd(const char *s)
{ strcpy(path,s); ok(0x3b00,0,0,(uintptr_t)path,"CWD chdir"); }
static void select_drive(unsigned d)
{
    unsigned n=ok(0x0e00,0,0,d,"SELECT call")&255;
    need(n==5,"SELECT logical drive extent");
    need((ok(0x1900,0,0,0,"GETDISK call")&255)==d,"SELECT current drive");
}
static void read_equal(const char *name,const char *reference)
{
    unsigned h=open_path(name), total=0;
    FILE *f=fopen(reference,"rb"); need(f!=0,"REFERENCE open");
    char expected[4096];
    for (;;) {
        unsigned got=ok(0x3f00,h,sizeof data,(uintptr_t)data,"HANDLE read");
        size_t n=fread(expected,1,sizeof expected,f);
        need(got==n && !memcmp(data,expected,n),"PATH bytes vs mtype");
        total+=got; if(!got) break;
    }
    fclose(f); ok(0x3e00,h,0,0,"HANDLE close");
    need(total!=0,"REFERENCE nonempty");
}
/* DOS 3.3 PRM AH=3Ah: qualify before lookup/current-directory guard. */
static void review_rmdir(void)
{
    select_drive(0);
    strcpy(path,"C:\\EMPTY"); ok(0x3900,0,0,(uintptr_t)path,"review mkdir C EMPTY");
    ok(0x3a00,0,0,(uintptr_t)path,"3rp9i qualified removal uses C, not A");
    strcpy(path,"C:\\SYS"); int_frame_t f=call(0x3a00,0,0,(uintptr_t)path);
    need((f.eflags&1) && (f.eax&65535)==16,"3rp9i protect C current directory");
    strcpy(path,"C:CUR"); ok(0x3900,0,0,(uintptr_t)path,"review mkdir relative C");
    cd("C:CUR");
    strcpy(path,"C:\\SYS\\CUR"); f=call(0x3a00,0,0,(uintptr_t)path);
    need((f.eflags&1) && (f.eax&65535)==16,"3rp9i protect qualified nested current directory");
    cd("C:\\SYS");
    strcpy(path,"C:CUR"); ok(0x3a00,0,0,(uintptr_t)path,"3rp9i qualified relative removal");
    need((call(0x1900,0,0,0).eax&255)==0,"3rp9i default drive preserved");
}
/* DOS 3.3 PRM AH=11h/12h: returned FCB names the drive searched.
 * A and C contain different mtools-minted SAME.TXT bytes. D aliases C's
 * physical volume here; its DOS identity must nevertheless remain D. */
static void review_fcb(fat12_volume_t *c,const char *a_ref,const char *c_ref)
{
    need(!fileio_fat_add_drive(3,c),"review mount D"); cd("D:\\SYS");
    static uint8_t fcb[44], result[44], opened[44];
    for(unsigned d=1;d<=4;d++) {
        if(d==2)continue;
        for(unsigned ext=0;ext<2;ext++) {
            memset(fcb,0,sizeof fcb); unsigned off=ext?7:0;
            if(ext)fcb[0]=255;
            fcb[off]=d; memcpy(fcb+off+1,"SAME    TXT",11);
            ok(0x1a00,0,0,(uintptr_t)result,"review FCB DTA");
            need((call(0x1100,0,0,(uintptr_t)fcb).eax&255)==0,"review FCB search");
            need(result[off]==d,"us273 search result drive byte");
            memset(opened,0,sizeof opened); memcpy(opened,result,off+12);
            need((call(0x0f00,0,0,(uintptr_t)opened).eax&255)==0,"us273 reopen result FCB");
            ok(0x1a00,0,0,(uintptr_t)data,"review record DTA");
            unsigned status=call(0x1400,0,0,(uintptr_t)opened).eax&255;
            need(status==0 || status==3,"us273 record read");
            FILE *ref=fopen(d==1?a_ref:c_ref,"rb"); char expected[128];
            need(ref!=0,"review independent reference"); size_t n=fread(expected,1,128,ref); fclose(ref);
            need(n && !memcmp(data,expected,n),"us273 reopened source vs mtype");
        }
    }
    fileio_fat_drop_drive(3); select_drive(0);
}
int main(int argc,char **argv)
{
    if(argc!=5) return 2;
    blockdev_file_t disks[2]; fat12_volume_t vols[2]; uint8_t sector[512];
    for(unsigned i=0;i<2;i++) {
        need(!blockdev_file_open_rw(disks+i,argv[i+1]),"MOUNT image");
        need(!fat12_mount(vols+i,&disks[i].dev,sector),"MOUNT FAT");
    }
    need(!fileio_fat_bind(vols),"MOUNT A");
    need(fileio_fat_add_drive!=0,"MOUNT second volume API missing");
    need(!fileio_fat_add_drive(2,vols+1),"MOUNT C");
    psp_params_t p={0}; p.alloc_end_linear=0x70000;
    need(!psp_build(&process,&p),"PSP"); sft_init(); int21_set_psp(&process);
    int21_set_lastdrive(26);
    need((call(0x0e00,0,0,0).eax&255)==26,"LIMIT LASTDRIVE=Z");
    int21_set_lastdrive(5);
    select_drive(2); cd("\\SYS");
    select_drive(0); cd("\\USER");
    /* Every DOS path form, including drive-relative and rooted-default. */
    read_equal("SAME.TXT",argv[3]); read_equal(".\\SAME.TXT",argv[3]);
    read_equal("A:SAME.TXT",argv[3]); read_equal("a:same.txt",argv[3]);
    read_equal("\\USER\\SAME.TXT",argv[3]);
    read_equal("A:\\USER\\..\\USER\\SAME.TXT",argv[3]);
    read_equal("C:SAME.TXT",argv[4]); read_equal("c:\\SYS\\SAME.TXT",argv[4]);
    select_drive(2); read_equal("SAME.TXT",argv[4]);
    ok(0x4700,0,0,1,"CWD explicit A"); need(!strcmp(cwd,"USER"),"CWD independent A");
    ok(0x4700,0,0,3,"CWD explicit C"); need(!strcmp(cwd,"SYS"),"CWD independent C");
    /* SFT identity survives both another OPEN and a default-drive switch. */
    unsigned src=open_path("C:SAME.TXT");
    select_drive(0); strcpy(path,"A:\\COPY.TXT");
    unsigned dst=ok(0x3c00,0,0,(uintptr_t)path,"CACHE create A");
    need(!int21_same_file((uint16_t)src,(uint16_t)dst),"HANDLE distinct volumes");
    for (;;) {
        unsigned n=ok(0x3f00,src,sizeof data,(uintptr_t)data,"HANDLE source C");
        if(!n)break;
        need(ok(0x4000,dst,n,(uintptr_t)data,"HANDLE target A")==n,"CACHE write A");
    }
    ok(0x3e00,src,0,0,"HANDLE close C"); ok(0x3e00,dst,0,0,"HANDLE close A");
    /* FINDNEXT must retain its volume after another path operation. */
    ok(0x1a00,0,0,(uintptr_t)&dta,"SEARCH DTA"); strcpy(path,"C:\\SYS\\*.TXT");
    ok(0x4e00,0,0,(uintptr_t)path,"SEARCH first");
    unsigned h=open_path("A:SAME.TXT"); ok(0x3e00,h,0,0,"SEARCH intervening A");
    unsigned count=1;
    while (!(call(0x4f00,0,0,0).eflags&1)) count++;
    need(count==2,"SEARCH retained C volume");
    for(unsigned d=1;d<=3;d+=2) {
        call(0x0e00,0,0,d);
        need((call(0x1900,0,0,0).eax&255)==0,"INVALID selection unchanged");
        strcpy(path,d==1?"B:\\SAME.TXT":"D:\\SAME.TXT");
        int_frame_t bad=call(0x3d00,0,0,(uintptr_t)path);
        need((bad.eflags&1) && (bad.eax&65535)==15,"INVALID path drive");
    }
    if(!getenv("REVIEW_FCB_ONLY")) review_rmdir();
    review_fcb(vols+1,argv[3],argv[4]);
    /* Encyclopedia 3Dh: real FAT directories and volume labels are not
     * ordinary file handles. No JFT/SFT entry may be committed on refusal. */
    const char *nonfiles[]={"C:\\SYS","A:\\REVIEW"};
    for(unsigned i=0;i<2;i++) {
        sft_entry_t before[SFT_MAX_ENTRIES]; psp_t saved=process;
        memcpy(before,g_sft,sizeof before);strcpy(path,nonfiles[i]);
        int_frame_t f=call(0x3d00,0,0,(uintptr_t)path);
        need((f.eflags&1) && (f.eax&65535)==5,"vwo7d real FAT directory/label OPEN denied");
        need(!memcmp(before,g_sft,sizeof before) && !memcmp(&saved,&process,sizeof saved),"vwo7d real FAT refusal preserves tables");
    }
    for(unsigned i=0;i<2;i++) blockdev_file_close(disks+i);
    puts("GREEN dos-volumes host: path forms, SELDISK/GETDISK, per-drive CWD, SFT/COPY and search identity, absent B/D");
    return 0;
}
