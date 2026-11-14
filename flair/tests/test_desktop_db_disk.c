/* Whole-file DB preservation, using the documented IDB1 record layout as an
 * independent fixture, mtools for disk decoding and a fresh product reload. */
#define main filecopy_disk_main
#include "test_filecopy_disk.c"
#undef main
#include "desktop/desktop_db.h"
static const unsigned char prior_db[80]={
    'I','D','B','1',1,0,3,0,
    1,0,0,0,'D','R','I','V','E',' ','A',0,0,0,0,0,0,47,0,93,0,0,0,0,
    5,1,7,0,'R','E','A','D','M','0','0','1',0,0,0,0,0,0,0,0,0,0,0,0,
    6,0,7,0,'R','E','A','D','M','E','.','T','X','T',0,0,0,0,0,0,0,0,0,0
};
static unsigned char next_db[80],cluster[512];
/* The mformat 1.44 MB fixture has its root at sector 19. Refuse both writes
 * that try to restore/publish DESKTOP.DB after the old name has moved. */
static int fail_db_commit_restore(uint32_t lba,uint32_t n,const void *bytes)
{
    unsigned char root[14*512]; int publishing=0,backed_up=0;
    if(lba<19 || lba>=33) return 0;
    for(uint32_t off=0;off<n*512;off+=32)
        if(!memcmp((const unsigned char *)bytes+off,"DESKTOP DB ",11)) publishing=1;
    if(!publishing) return 0;
    CHECK(fseek(disk,19*512,SEEK_SET)==0 && fread(root,1,sizeof root,disk)==sizeof root,"read independent root snapshot");
    for(unsigned off=0;off<sizeof root;off+=32) if(!memcmp(root+off,"DESKTOP ~00",11)) backed_up=1;
    return backed_up;
}
static int db_write(void)
{
    uint32_t len; void *fat=fileio_fat_fat_buffer(&len);
    return desktop_db_write(&vol,fat,len,sec,cluster,next_db,sizeof next_db);
}
static void db_check(const void *want)
{
    unsigned char data[81]; uint32_t len,got=0; FILE *f;
    command("mcopy -o -i copy.img ::DESKTOP.DB observed.db");
    f=fopen("observed.db","rb"); CHECK(f!=0,"mtools DB output");
    if(f) { CHECK(fread(data,1,sizeof data,f)==80 && !memcmp(data,want,80),"independent persisted positions and Trash origin"); fclose(f); }
    CHECK(fileio_fat_bind(&vol)==0,"fresh FAT cache on reload");
    void *fat=fileio_fat_fat_buffer(&len);
    CHECK(desktop_db_read(&vol,fat,len,sec,cluster,data,sizeof data,&got)==0 && got==80 && !memcmp(data,want,80),"reload complete DB after save");
}
int main(void)
{
    FILE *f; unsigned nw;
    memcpy(next_db,prior_db,80); next_db[25]=99; next_db[34]=12; next_db[58]=12;
    command("mformat -C -i base.img -f 1440 ::");
    f=fopen("prior.db","wb"); fwrite(prior_db,1,80,f); fclose(f);
    command("mcopy -i base.img prior.db ::DESKTOP.DB");
    setup(); CHECK(db_write()==0,"successful DB save"); nw=writes; db_check(next_db); fclose(disk);
    for(unsigned i=1;i<=nw;i++) {
        setup(); fail_write=i; int rc=db_write(); fail_write=0;
        printf("DB sector fault %u rc=%d hit=%u\n",i,rc,hit);
        CHECK(hit==1,"DB fault reached"); db_check(rc<0?prior_db:next_db); fclose(disk);
    }
    setup(); corrupt_sector=1; CHECK(db_write()<0 && hit==1,"DB verifies silently corrupted stage"); db_check(prior_db); fclose(disk);
    setup(); sector_fault=fail_db_commit_restore;
    CHECK(db_write()<0 && hit==2,"DB final commit and restore both fail"); sector_fault=0;
    CHECK(desktop_db_recovery() && !strcmp(desktop_db_recovery(),"DESKTOP.~00"),"DB error exposes exact previous version name");
    const char *text[3]; CHECK(desktop_db_recovery_alert(text)==3,"three-line Finder recovery alert");
    CHECK(!strcmp(text[0],"The disk could not be updated.") &&
          !strcmp(text[1],"The previous Desktop file is saved as:") &&
          !strcmp(text[2],"DESKTOP.~00"),"Finder recovery wording names the retained previous file");
    for(unsigned i=0;i<3;i++) {
        unsigned width=0; for(unsigned k=0;text[i][k];k++) width+=(unsigned)chicago_advance((unsigned char)text[i][k]);
        CHECK(width<=285,"complete Finder recovery line fits alert");
    }
    recovered_stream("mcopy -o -i copy.img '::DESKTOP.~00' recovered.dat",prior_db,sizeof prior_db);
    recovered_stream("mcopy -o -i copy.img '::DESKTOP.$00' recovered.dat",next_db,sizeof next_db);
    fclose(disk);
    return TEST_SUMMARY("test-desktop-db-disk");
}
