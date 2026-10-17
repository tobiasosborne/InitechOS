/* tdnl.62. Real FAT rails; saturated independent allocation table. A failed
 * DESKTOP.DB bootstrap must not publish an uncommitted zero-byte dirent. */
#define main legacy_db_main
#include "test_desktop_db.c"
#undef main
int main(int argc,char **argv)
{
    blockdev_file_t bf;
    fat12_volume_t vol;
    desktop_db_boot_state_t state;
    desktop_db_reason_t reason;
    dir_entry_t entry;
    uint32_t len;
    if(argc!=2) return 2;
    CHECK(blockdev_file_open_rw(&bf,argv[1])==0,"FULL_DISK opens fixture");
    CHECK(fat12_mount(&vol,&bf.dev,g_sector)==FAT12_OK,"FULL_DISK mounts");
    CHECK(fat12_read_fat(&vol,g_fat,sizeof g_fat)==FAT12_OK,"FULL_DISK reads FAT");
    len=(uint32_t)vol.bpb.sectors_per_fat*512u;
    for(uint32_t c=2;c<vol.total_clusters+2;c++)
        CHECK(fat12_set_entry(g_fat,len,(uint16_t)c,0xfffu)==FAT12_OK,"FULL_DISK saturates FAT");
    CHECK(fat12_flush_fats(&vol,g_fat,len)==FAT12_OK,"FULL_DISK commits saturation");
    CHECK(desktop_db_bootstrap(&vol,g_fat,len,g_sector,g_cluster,g_db,sizeof g_db,&state,&reason)==FAT12_ERR_NO_SPACE,
          "FULL_DISK bootstrap reports NO_SPACE");
    CHECK(fat12_find(&vol,g_sector,"DESKTOP.DB",&entry)==FAT12_ERR_NOT_FOUND,
          "NO_PHANTOM failed write leaves DESKTOP.DB absent");
    blockdev_file_close(&bf);
    return TEST_SUMMARY("test-finder-diskfull");
}
