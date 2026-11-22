/* Two real FAT12 roots minted by mtools (tests/volume-db-disk.sh), each
 * holding a SAME.TXT at the same physical cluster and an independently
 * authored 56-byte IDB1 DESKTOP.DB. The kernel FAT engine reads both; the
 * Finder imports each into its own volume identity, exports the same bytes,
 * and the resident persist writes only the volume whose state changed. The
 * script then compares the written images with mtools. No Finder code
 * supplies an expected record. Ref: MTE Finder Interface (a specification
 * names its volume and its directory); C activation plan step 9. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_assert.h"
#include "finder_actions.h"
#include "desktop_db.h"
#include "fileio_fat.h"
#include "volume_db.h"
TEST_HARNESS();
static finder_shell_t shell;
finder_shell_t *g_finder_shell=&shell;
finder_desk_t *g_finder_desk=&shell.desk;
const fat12_volume_t *g_finder_vol;
unsigned g_finder_drive=2;
static uint8_t sector[512];
void *g_finder_fat,*g_finder_secbuf=sector;
uint32_t g_finder_fat_len;
uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC*512];
uint8_t g_finder_db_buf[FINDER_DB_MAX_BYTES];
int g_finder_db_dirty;
finder_win_status_t g_finder_io_error;
void serial_puts(const char *s){(void)s;}
void serial_putu(uint32_t n){(void)n;}
void serial_puti(int32_t n){(void)n;}
void serial_putc(char c1){(void)c1;}
typedef struct disk { uint8_t *bytes; size_t len; unsigned writes; int fail_reads; } disk_t;
static disk_t disks[2]; /* 0 = A, 1 = C */
static fat12_volume_t vols[2];
static uint8_t fats[2][16384];
static int rd(void *ctx,uint32_t lba,uint32_t n,void *out)
{
    disk_t *d=ctx;
    if(d->fail_reads || (uint64_t)(lba+n)*512>d->len) return -1;
    memcpy(out,d->bytes+(size_t)lba*512,(size_t)n*512); return 0;
}
static int wr(void *ctx,uint32_t lba,uint32_t n,const void *in)
{
    disk_t *d=ctx; d->writes++;
    if((uint64_t)(lba+n)*512>d->len) return -1;
    memcpy(d->bytes+(size_t)lba*512,in,(size_t)n*512); return 0;
}
static blockdev_t devs[2]={{&disks[0],rd,wr},{&disks[1],rd,wr}};
/* The published getter, modelled over the two mounted images. */
const fat12_volume_t *fileio_fat_drive_volume(unsigned drive,void **fat,uint32_t *len)
{
    int i=drive==0?0:drive==2?1:-1;
    *fat=NULL;*len=0;
    if(i<0) return NULL;
    *fat=fats[i];*len=(uint32_t)((uint32_t)vols[i].bpb.sectors_per_fat*512u);
    return &vols[i];
}
static void load_image(int i,const char *path)
{
    FILE *fp=fopen(path,"rb");
    if(!fp) exit(2);
    fseek(fp,0,SEEK_END); disks[i].len=(size_t)ftell(fp); rewind(fp);
    disks[i].bytes=malloc(disks[i].len);
    if(!disks[i].bytes || fread(disks[i].bytes,1,disks[i].len,fp)!=disks[i].len) exit(2);
    fclose(fp);
    CHECK(fat12_mount(&vols[i],&devs[i],sector)==FAT12_OK && vols[i].fat_type==FAT_TYPE_FAT12,
          "mtools FAT12 image mounts");
    CHECK(fat12_read_fat(&vols[i],fats[i],sizeof fats[i])==FAT12_OK,"its FAT loads");
}
static void store_image(int i,const char *path)
{
    FILE *fp=fopen(path,"wb");
    if(!fp || fwrite(disks[i].bytes,1,disks[i].len,fp)!=disks[i].len) exit(2);
    fclose(fp);
}
static uint32_t read_db(int i,uint8_t *out,uint32_t cap)
{
    uint32_t len=0;
    CHECK(desktop_db_read(&vols[i],fats[i],sizeof fats[i],sector,g_finder_clusbuf,
                          out,cap,&len)==FAT12_OK,"DESKTOP.DB reads through the FAT engine");
    return len;
}
int main(int argc,char **argv)
{
    static uint8_t adb[FINDER_DB_MAX_BYTES],cdb[FINDER_DB_MAX_BYTES],out[FINDER_DB_MAX_BYTES];
    uint32_t alen,clen;
    dir_entry_t e[2];
    if(argc!=3) return 2;
    load_image(1,argv[1]); load_image(0,argv[2]);
    /* The resident globals name the system C:, as volumes.c binds them. */
    g_finder_vol=&vols[1]; g_finder_fat=fats[1];
    g_finder_fat_len=(uint32_t)vols[1].bpb.sectors_per_fat*512u;
    CHECK(fat12_find(&vols[0],sector,"SAME.TXT",&e[0])==FAT12_OK &&
          fat12_find(&vols[1],sector,"SAME.TXT",&e[1])==FAT12_OK &&
          e[0].start_cluster==e[1].start_cluster,
          "(fixture) SAME.TXT has the same physical cluster on both roots");
    clen=read_db(1,cdb,sizeof cdb); alen=read_db(0,adb,sizeof adb);
    CHECK(clen==56 && alen==56,"both DESKTOP.DB files are the authored 56 bytes");
    shell.volume_drive=2; shell.desk.icons=shell.desk_icons;
    CHECK(finder_volume_db_load(&shell,2,cdb,clen)==2,"C imports its root view and origin");
    CHECK(finder_volume_db_load(&shell,0,adb,alen)==2,"A imports without replacing C");
    const finder_view_rec_t *cv=finder_shell_find_view(&shell,finder_dir_make(2,0));
    const finder_view_rec_t *av=finder_shell_find_view(&shell,finder_dir_make(0,0));
    CHECK(shell.n_views==2 && cv && av && cv!=av,"equal root clusters keep two view identities");
    CHECK(cv && av && cv->x==70 && cv->y==90 && av->x==46 && av->y==115,
          "each root keeps its own authored position");
    CHECK(shell.n_origins==2 && shell.origins[0].dir_start!=shell.origins[1].dir_start,
          "same-named origins stay on their own volume");
    CHECK(finder_volume_db_encode(&shell,2,out,sizeof out)==clen && !memcmp(out,cdb,clen),
          "C exports exactly the bytes mtools stored");
    CHECK(finder_volume_db_encode(&shell,0,out,sizeof out)==alen && !memcmp(out,adb,alen),
          "A exports exactly the bytes mtools stored");
    /* Unchanged state: neither disk is written. */
    g_finder_db_dirty=1; finder_desk_persist();
    CHECK(disks[0].writes==0 && disks[1].writes==0,"unchanged state writes neither disk");
    /* A read failure on A must not overwrite A, and keeps the state dirty. */
    for(unsigned i=0;i<shell.n_views;i++)
        if(shell.views[i].dir_start==finder_dir_make(0,0)) shell.views[i].x=120;
    disks[0].fail_reads=1; g_finder_db_dirty=1; finder_desk_persist();
    disks[0].fail_reads=0;
    CHECK(disks[0].writes==0 && disks[1].writes==0 && g_finder_db_dirty==1,
          "a failed A read preserves A and C and retries later");
    /* Move A's root view: only A is written. */
    for(unsigned i=0;i<shell.n_views;i++)
        if(shell.views[i].dir_start==finder_dir_make(0,0)) shell.views[i].x=100;
    g_finder_db_dirty=1; finder_desk_persist();
    CHECK(disks[1].writes==0,"moving A's view never writes C");
    CHECK(disks[0].writes>0 && g_finder_db_dirty==0,"A receives its moved view");
    store_image(1,argv[1]); store_image(0,argv[2]);
    return TEST_SUMMARY("test-volume-db-disk");
}
