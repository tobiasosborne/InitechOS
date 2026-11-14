/* GM3 and phase-2 failed-save ruling: independent mtools disk readback.
 * Real copy engine -> DOS -> FAT12. Fail each DOS operation and each sector
 * write once; the oracle compares literal old/source bytes using mcopy. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
#include "fileio_fat.h"
#include "devices.h"
#include "tbxgate.c"
#include "tbx_save.h"
#include "filecopy_cancel.h"
#define flair_platform_dos real_platform_dos
#include "platform_dos.c"
#undef flair_platform_dos
TEST_HARNESS();
static unsigned fail_call, calls, fail_write, writes, hit, fail_rollbacks, corrupt_sector, deletes;
static FILE *disk;
static int (*sector_fault)(uint32_t,uint32_t,const void *);
static int rd(void *p,uint32_t lba,uint32_t n,void *b)
{ FILE *media=p?p:disk; return fseek(media,(long)lba*512,SEEK_SET) || fread(b,512,n,media)!=n ? -1:0; }
static int wr(void *p,uint32_t lba,uint32_t n,const void *b)
{
    FILE *media=p?p:disk;
    if (++writes==fail_write) { hit++; return -1; }
    if(sector_fault && sector_fault(lba,n,b)) { hit++; return -1; }
    unsigned char corrupt[512];
    unsigned nonzero=0;
    for(unsigned i=0;i<512 && n==1;i++) nonzero|=((const unsigned char *)b)[i];
    if(corrupt_sector && lba>=33 && n==1 && nonzero) {
        memcpy(corrupt,b,512); corrupt[3]^=0x40; b=corrupt;
        corrupt_sector=0; hit++;
    }
    return fseek(media,(long)lba*512,SEEK_SET) || fwrite(b,512,n,media)!=n || fflush(media) ? -1:0;
}
void flair_platform_dos(flair_platform_files *p,flair_platform_regs *r)
{
    if(r->eax==0x4100) deletes++;
    if(fail_rollbacks && r->eax==0x5600 && strcmp((const char *)(uintptr_t)r->edx,"TARGET.DAT")) {
        hit++; r->eax=5; r->eflags|=1; return;
    }
    if (++calls==fail_call) { hit++; r->eax=5; r->eflags|=1; return; }
    real_platform_dos(p,r);
}
static void command(const char *s) { CHECK(system(s)==0,s); }
static fat12_volume_t vol;
static blockdev_t dev={0,rd,wr};
static unsigned char sec[512], source[513];
static const char old[]="Previous document: never lose these bytes.\r\n";
static char reported_error[3u * TBX_ALERT_LINE_MAX];
int32_t tbx_alert(const char *text,const char *def,const char *cancel,const char *other)
{ (void)def; (void)cancel; (void)other; strcpy(reported_error,text); return 0; }
static void setup(void)
{
    command("cp base.img copy.img");
    disk=fopen("copy.img","r+b"); CHECK(disk!=0,"open disk");
    CHECK(fat12_mount(&vol,&dev,sec)==0,"mount FAT12");
    CHECK(fileio_fat_bind(&vol)==0,"bind real DOS FAT");
    sft_init(); tbxf_init(&g_slot.files);
    devices_init(); int21_set_devices(devices_head(),0);
    strcpy(g_slot.fc_src,"SOURCE.DAT"); strcpy(g_slot.fc_dst,"TARGET.DAT");
    calls=writes=hit=deletes=0;
}
static void verify(const void *want,size_t len)
{
    unsigned char out[sizeof source]; FILE *f;
    fflush(disk);
    remove("observed.dat");
    command("mcopy -o -i copy.img ::TARGET.DAT observed.dat");
    f=fopen("observed.dat","rb"); CHECK(f!=0,"independent reader output");
    if(f) { size_t n=fread(out,1,sizeof out,f); CHECK(n==len && !memcmp(out,want,len),"destination bytes invariant (mtools)"); fclose(f); }
    command("mcopy -o -i copy.img ::SOURCE.DAT observed.dat");
    f=fopen("observed.dat","rb");
    if(f) { size_t n=fread(out,1,sizeof out,f); CHECK(n==sizeof source && !memcmp(out,source,n),"source bytes invariant (mtools)"); fclose(f); }
}
static int32_t run(void)
{
    uint32_t size=0; int32_t rc=tbx_fc_open(&size);
    if(rc>=0) rc=tbx_fc_step(size);
    tbx_fc_close(); return rc;
}
static int32_t file_gate(uint32_t code,uint32_t a,uint32_t b,uint32_t c)
{ uint32_t args[]={a,b,c}; return tbxf_call(&g_slot.files,1,0xfffffffeu,code,args); }
int32_t tbx_file_open(const char *p,uint32_t mode) { return file_gate(TBX_FILE_OPEN,flair_file_ptr(p),(uint32_t)mode,0); }
int32_t tbx_file_create_new(const char *p) { return file_gate(TBX_FILE_CREATE_NEW,flair_file_ptr(p),0,0); }
int32_t tbx_file_read(int32_t h,void *p,uint32_t n) { return file_gate(TBX_FILE_READ,(uint32_t)h,flair_file_ptr(p),n); }
int32_t tbx_file_write(int32_t h,const void *p,uint32_t n) { return file_gate(TBX_FILE_WRITE,(uint32_t)h,flair_file_ptr(p),n); }
int32_t tbx_file_close(int32_t h) { return file_gate(TBX_FILE_CLOSE,(uint32_t)h,0,0); }
int32_t tbx_file_seek(int32_t h,int32_t n,uint32_t origin) { return file_gate(TBX_FILE_SEEK,(uint32_t)h,(uint32_t)n,(uint32_t)origin); }
int32_t tbx_file_delete(const char *p) { return file_gate(TBX_FILE_DELETE,flair_file_ptr(p),0,0); }
int32_t tbx_file_rename(const char *a,const char *b) { return file_gate(TBX_FILE_RENAME,flair_file_ptr(a),flair_file_ptr(b),0); }
static int32_t save_run(int from_file)
{
    if(!from_file) return tbx_file_save("TARGET.DAT",source,sizeof source);
    int32_t rc=tbx_file_copy_save("SOURCE.DAT","TARGET.DAT");
    return rc==TBX_OK?(int32_t)sizeof source:rc;
}
static void save_sweep(int from_file)
{
    unsigned nc,nw;
    setup(); CHECK(save_run(from_file)==sizeof source,"successful SDK save");
    nc=calls; nw=writes; verify(source,sizeof source); tbxf_reap(&g_slot.files); fclose(disk);
    for(unsigned i=1;i<=nc;i++) {
        setup(); fail_call=i; int32_t rc=save_run(from_file); fail_call=0;
        printf("SDK%s DOS fault %u rc=%d hit=%u\n",from_file?" input":"",i,rc,hit);
        CHECK(hit==1,"SDK DOS fault reached"); verify(rc==sizeof source?(const void *)source:old,rc==sizeof source?sizeof source:sizeof old-1);
        CHECK(tbxf_live(&g_slot.files)==0,"SDK DOS failure releases handles");
        tbxf_reap(&g_slot.files); fclose(disk);
    }
    for(unsigned i=1;i<=nw;i++) {
        setup(); fail_write=i; int32_t rc=save_run(from_file); fail_write=0;
        printf("SDK%s sector fault %u rc=%d hit=%u\n",from_file?" input":"",i,rc,hit);
        CHECK(hit==1,"SDK sector fault reached"); verify(rc==sizeof source?(const void *)source:old,rc==sizeof source?sizeof source:sizeof old-1);
        CHECK(tbxf_live(&g_slot.files)==0,"SDK sector failure releases handles");
        tbxf_reap(&g_slot.files); fclose(disk);
    }
}
static void reserve(const char *name)
{
    int32_t h=tbx_file_create_new(name); CHECK(h>=5,"reserve collision fixture");
    if(h>=5) { CHECK(tbx_file_write(h,old,sizeof old-1)==sizeof old-1,"collision bytes"); CHECK(tbx_file_close(h)==0,"close collision"); }
}
static void extra_cases(void)
{
    static char collision[32]; uint32_t n;
    CHECK(flair_filecopy_cancel(3,27,0),"Escape cancels");
    CHECK(flair_filecopy_cancel(5,'.',0x100),"Command-period repeat cancels");
    CHECK(!flair_filecopy_cancel(3,'.',0),"plain period is not cancel");
    CHECK(!flair_filecopy_cancel(4,27,0),"key release is not cancel");
    const char *aliases[]={"SOURCE.DAT","source.dat","A:\\SOURCE.DAT","\\SOURCE.DAT","A:\\.\\SOURCE.DAT"};
    for(unsigned i=0;i<sizeof aliases/sizeof aliases[0];i++) {
        setup(); strcpy(g_slot.fc_dst,aliases[i]); CHECK(tbx_fc_open(&n)<0,"resolved same-file alias refused");
        CHECK(tbxf_live(&g_slot.files)==0,"alias refusal closes handles"); verify(old,sizeof old-1); fclose(disk);
    }
    setup(); strcpy(g_slot.fc_dst,"NUL.TXT"); CHECK(tbx_fc_open(&n)<0,"device refused before copy"); verify(old,sizeof old-1); fclose(disk);
    setup();
    for(unsigned i=0;i<11;i++) { snprintf(collision,sizeof collision,"TARGET.$$%c",i?(char)('0'+i-1):'$'); reserve(collision); }
    CHECK(tbx_file_save("TARGET.DAT",source,sizeof source)==TBX_ERR_DOS(0x50),"all eleven SDK temporary collisions refuse");
    verify(old,sizeof old-1); fclose(disk);
    setup();
    for(unsigned i=0;i<100;i++) { snprintf(collision,sizeof collision,"TARGET.$%02u",i); reserve(collision); }
    CHECK(tbx_fc_open(&n)==TBX_ERR_DOS(0x50),"all FILE COPY temporary collisions refuse");
    verify(old,sizeof old-1); fclose(disk);
    /* Fill every available cluster independently, then remount before testing
     * both writers. An initial stage creation may succeed; transfer must not. */
    command("cp base.img clean.img");
    command("dd if=/dev/zero of=fill.dat bs=512 count=2844 2>/dev/null");
    command("mcopy -i base.img fill.dat ::FILL.DAT");
    setup(); CHECK(run()<0,"full-volume FILE COPY refuses"); verify(old,sizeof old-1); fclose(disk);
    setup(); CHECK(tbx_file_save("TARGET.DAT",source,sizeof source)!=sizeof source,"full-volume SDK save refuses"); verify(old,sizeof old-1); fclose(disk);
    command("cp clean.img base.img");
    setup(); corrupt_sector=1; CHECK(run()<0 && hit==1,"copy verifies silently corrupted stage"); verify(old,sizeof old-1); fclose(disk);
    setup(); corrupt_sector=1; CHECK(tbx_file_save("TARGET.DAT",source,sizeof source)<0 && hit==1,"SDK verifies silently corrupted stage"); verify(old,sizeof old-1); fclose(disk);
    setup(); fclose(disk);
    FILE *cfg=fopen("rate.cfg","wb"); fputs("PROGRESS_BAR_RATE=37\r\n",cfg); fclose(cfg);
    command("mmd -i copy.img ::APPS"); command("mcopy -i copy.img rate.cfg ::APPS/MICHAEL.CFG");
    disk=fopen("copy.img","r+b"); CHECK(fileio_fat_bind(&vol)==0,"remount configured rate fixture");
    CHECK(tbx_fc_rate()==37,"FILE COPY uses configured disk rate"); fclose(disk);
}
static void recovered_stream(const char *cmd,const void *want,size_t len)
{
    unsigned char data[sizeof source]; command(cmd);
    FILE *f=fopen("recovered.dat","rb"); CHECK(f!=0,"complete recovery stream exists");
    if(f) { CHECK(fread(data,1,sizeof data,f)==len && !memcmp(data,want,len),"recovery bytes match independent fixture"); fclose(f); }
}
static void double_failure(int sdk)
{
    setup(); fail_rollbacks=1;
    int32_t rc=sdk?tbx_file_save("TARGET.DAT",source,sizeof source):run();
    fail_rollbacks=0;
    printf("%s commit+rollback failure rc=%d faults=%u\n",sdk?"SDK":"FILE COPY",rc,hit);
    CHECK(rc<0 && hit==2,"commit and restore both failed");
    CHECK(deletes==0,"double failure deletes nothing");
    if(sdk) CHECK(strstr(reported_error,"TARGET.~00")!=0,"SDK error names exact previous version");
    else CHECK(g_slot.alert && !strcmp(g_slot.alert_text[1],"TARGET.~00"),"FILE COPY error names exact previous version");
    recovered_stream("mcopy -o -i copy.img '::TARGET.~00' recovered.dat",old,sizeof old-1);
    recovered_stream(sdk?"mcopy -o -i copy.img '::TARGET.$$$' recovered.dat":"mcopy -o -i copy.img '::TARGET.$00' recovered.dat",source,sizeof source);
    recovered_stream("mcopy -o -i copy.img ::SOURCE.DAT recovered.dat",source,sizeof source);
    CHECK(tbxf_live(&g_slot.files)==0,"double failure closes every handle");
    fclose(disk);
}
int main(int argc,char **argv)
{
    FILE *f; unsigned nc,nw;
    for(unsigned i=0;i<sizeof source;i++) source[i]=(unsigned char)(i*31u+7u);
    f=fopen("old.dat","wb"); CHECK(f!=0,"old fixture"); fwrite(old,1,sizeof old-1,f); fclose(f);
    f=fopen("source.dat","wb"); CHECK(f!=0,"source fixture"); fwrite(source,1,sizeof source,f); fclose(f);
    command("mformat -C -i base.img -f 1440 ::");
    command("mcopy -i base.img old.dat ::TARGET.DAT");
    command("mcopy -i base.img source.dat ::SOURCE.DAT");
    if(argc>1 && !strcmp(argv[1],"--rollback-failure")) {
        double_failure(1); double_failure(0);
        return TEST_SUMMARY("test-reported-failure-invariant");
    }
    setup(); CHECK(run()==sizeof source,"successful copy"); nc=calls; nw=writes; verify(source,sizeof source); fclose(disk);
    for(unsigned i=1;i<=nc;i++) {
        setup(); fail_call=i; int32_t rc=run(); fail_call=0;
        printf("DOS fault %u rc=%d hit=%u\n",i,rc,hit);
        CHECK(hit==1,"DOS fault reached"); verify(rc<0?old:(const void *)source,rc<0?sizeof old-1:sizeof source);
        CHECK(tbxf_live(&g_slot.files)==0,"copy DOS failure releases handles");
        tbxf_reap(&g_slot.files); fclose(disk);
    }
    for(unsigned i=1;i<=nw;i++) {
        setup(); fail_write=i; int32_t rc=run(); fail_write=0;
        printf("sector fault %u rc=%d hit=%u\n",i,rc,hit);
        CHECK(hit==1,"sector fault reached"); verify(rc<0?old:(const void *)source,rc<0?sizeof old-1:sizeof source);
        CHECK(tbxf_live(&g_slot.files)==0,"copy sector failure releases handles");
        tbxf_reap(&g_slot.files); fclose(disk);
    }
    for(unsigned i=0;i<sizeof source;i+=128) {
        uint32_t n; setup(); CHECK(tbx_fc_open(&n)==0,"cancel open"); CHECK(tbx_fc_step(i)>=0,"cancel progress"); tbx_fc_close();
        verify(old,sizeof old-1); CHECK(tbxf_live(&g_slot.files)==0,"cancel closes handles"); fclose(disk);
    }
    save_sweep(0); save_sweep(1);
    extra_cases();
    double_failure(1); double_failure(0);
    return TEST_SUMMARY("test-filecopy-disk");
}
