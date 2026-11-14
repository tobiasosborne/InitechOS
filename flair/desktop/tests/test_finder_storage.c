/* Invariant oracle for the operator's 2026-11-14 FAT12-only write policy.
 * mtools supplies the image; no bytes may change and no write callback may
 * run for FAT16, even for directory-only edits. FAT12 is a positive control.
 */
#define main windows_suite_main
#include "test_finder_windows.c"
#undef main
#include <stdlib.h>
#include "finder_actions.h"
#include "desktop_db.h"
#include "finder_ops.h"
#include "finder_storage.h"
extern const finder_fs_t g_finder_fs_binding;
const fat12_volume_t *g_finder_vol;
void *g_finder_fat, *g_finder_secbuf;
uint32_t g_finder_fat_len;
uint8_t g_finder_dirbuf[FINDER_FS_MAX_SPC*512];
uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC*512];
void serial_puts(const char *s) { (void)s; }
void serial_puti(int32_t n) { (void)n; }
void serial_putc(char c) { (void)c; }
static unsigned writes;
static uint8_t *disk, *before;
static size_t disk_len;
static int item_index(finder_desk_t *view, const char *name)
{
    for (int i=0;i<view->n;i++) if (!strcmp(view->icons[i].name,name)) return i;
    return -1;
}
static int rd(void *ctx,uint32_t lba,uint32_t n,void *out)
{
    (void)ctx;
    if ((uint64_t)(lba+n)*512>disk_len) return -1;
    memcpy(out,disk+(size_t)lba*512,(size_t)n*512); return 0;
}
static int wr(void *ctx,uint32_t lba,uint32_t n,const void *in)
{
    (void)ctx; writes++;
    if ((uint64_t)(lba+n)*512>disk_len) return -1;
    memcpy(disk+(size_t)lba*512,in,(size_t)n*512); return 0;
}
static void unchanged(int rc,int expected,const char *operation)
{
    printf("%s rc=%d writes=%u\n",operation,rc,writes);
    CHECK(rc==expected,operation);
    CHECK(writes==0,"refusal before first sector write");
    CHECK(memcmp(disk,before,disk_len)==0,"all image bytes unchanged");
    memcpy(disk,before,disk_len); writes=0;
}
static void copy_out(const char *destination, const uint8_t source[21])
{
    uint8_t *saved_disk=disk;
    size_t saved_len=disk_len;
    fat12_volume_t dest;
    blockdev_t dev={NULL,rd,wr};
    uint8_t sec[512],fat[4608]; uint32_t slot=0,put=0;
    FILE *fp=fopen(destination,"r+b");
    CHECK(fp!=NULL,"copy OUT opens independent FAT12 destination");
    if(!fp) return;
    fseek(fp,0,SEEK_END); disk_len=(size_t)ftell(fp); rewind(fp);
    disk=malloc(disk_len);
    if(!disk || fread(disk,1,disk_len,fp)!=disk_len) exit(2);
    CHECK(fat12_mount(&dest,&dev,sec)==0 && dest.fat_type==FAT_TYPE_FAT12,"copy OUT destination is FAT12");
    CHECK(rd(NULL,dest.first_fat_sector,9,fat)==0,"copy OUT loads destination FAT");
    g_finder_vol=&dest; g_finder_fat=fat; g_finder_fat_len=sizeof fat;
    CHECK(finder_fat_create(NULL,"OUT.TXT",0,&slot)==0,"copy OUT creates on FAT12");
    CHECK(finder_fat_write_partial(NULL,0,slot,0,source,21,&put)==0 && put==21,
          "copy OUT writes bytes actually read from FAT16");
    rewind(fp); CHECK(fwrite(disk,1,disk_len,fp)==disk_len,"copy OUT stores destination image");
    fclose(fp);free(disk);disk=saved_disk;disk_len=saved_len;
}
int main(int argc,char **argv)
{
    FILE *fp;
    fat12_volume_t v;
    blockdev_t dev={NULL,rd,wr};
    fat12_fat_window_t win;
    uint8_t window[1024],fat[4608],sec[512],db[8]={'I','D','B','1',1,0,0,0};
    uint8_t bytes[128],source[21]; uint32_t got=0,slot=0;
    dir_entry_t entry;
    desktop_db_boot_state_t state; desktop_db_reason_t reason;
    desktop_trash_state_t trash;
    static scene_t scene;
    int root=-1, kind;
    if(argc!=3 && argc!=4) return 2;
    kind=atoi(argv[2]); fp=fopen(argv[1],"r+b"); if(!fp) return 2;
    fseek(fp,0,SEEK_END); disk_len=(size_t)ftell(fp); rewind(fp);
    disk=malloc(disk_len); before=malloc(disk_len);
    if(!disk || !before || fread(disk,1,disk_len,fp)!=disk_len) return 2;
    memcpy(before,disk,disk_len);
    CHECK(fat12_mount(&v,&dev,sec)==FAT12_OK,"mtools image mounts");
    CHECK((int)v.fat_type==kind,"mtools independently minted the requested FAT type");
    if(kind==16) {
        fat12_fat_window_init(&win,window,sizeof window); v.fat_window=&win;
        g_finder_fat=finder_storage_read_cache(&v,NULL); g_finder_fat_len=0;
        CHECK(g_finder_fat==window,"NULL whole FAT uses real two-sector read window");
    } else {
        CHECK(rd(NULL,v.first_fat_sector,9,fat)==0,"FAT12 whole FAT loaded");
        g_finder_fat=fat; g_finder_fat_len=sizeof fat;
    }
    g_finder_vol=&v; g_finder_secbuf=sec;
    scene_init(&scene,MOCK_DIRS,2);
    finder_shell_bind_fs(&scene.sh,&g_finder_fs_binding);
    CHECK(finder_win_open(&scene.sh,0,0,"",1,&root,NULL)==FINDER_WIN_OK,"Finder opens volume");
    CHECK(finder_fat_read_partial(NULL,"SOURCE.TXT",0,0,sizeof bytes,bytes,&got)==0 &&
          got==21 && !memcmp(bytes,"Finder source bytes\r\n",21),"Finder FAT16/FAT12 reads preserved");
    uint32_t total=0;
    for (;;) {
        uint32_t chunk=0;
        int rc=finder_fat_read_partial(NULL,"CHAIN.TXT",2,total,sizeof bytes,bytes,&chunk);
        CHECK(rc==0,"Finder opens/reads a file inside FAT16/FAT12 folder");
        if(rc || !chunk) break;
        int equal=1;
        for(uint32_t i=0;i<chunk;i++)
            if(bytes[i]!=(uint8_t)"Finder source bytes\r\n"[(total+i)%21]) equal=0;
        CHECK(equal,"nested chain bytes match independent literal pattern");
        total+=chunk;
        if(total>2100) break;
    }
    CHECK(total==2100,"Finder reads entire multi-cluster nested stream");
    CHECK(finder_fat_read_partial(NULL,"SOURCE.TXT",0,0,sizeof bytes,bytes,&got)==0 && got==21,
          "reopen source after nested file");
    memcpy(source,bytes,sizeof source);
    CHECK(fat12_find_slot_in(&v,g_finder_fat,g_finder_fat_len,0,sec,"SOURCE.TXT",&entry,&slot)==0,"source slot");
    if(kind==16) {
        /* -21 is the new Finder write-protect status; literal makes red-first
         * compile against the unfixed binding, without a missing-symbol red. */
        unchanged(finder_fat_mkdir(NULL,"NEWFOLD",0),-21,"mkdir");
        unchanged(finder_fat_move(NULL,"SOURCE.TXT",0,"RENAMED.TXT",0),-21,"rename");
        unchanged(finder_fat_move(NULL,"SOURCE.TXT",0,"SOURCE.TXT",3),-21,"move");
        unchanged(finder_fat_unlink(NULL,"SOURCE.TXT",0),-21,"unlink");
        unchanged(finder_fat_rmdir(NULL,"EMPTY",0),-21,"rmdir");
        unchanged(finder_fat_create(NULL,"COPY.TXT",0,&got),-21,"copy/create");
        unchanged(finder_fat_write_partial(NULL,0,slot,0,bytes,1,&got),-21,"positioned write");
        unchanged(desktop_db_write(&v,g_finder_fat,0,sec,g_finder_clusbuf,db,8),FAT12_ERR_ACCESS,"Desktop DB save");
        unchanged(desktop_db_bootstrap(&v,g_finder_fat,0,sec,g_finder_clusbuf,db,8,&state,&reason),FAT12_ERR_ACCESS,"Desktop DB bootstrap");
        unchanged(desktop_trash_ensure(&v,g_finder_fat,0,sec,g_finder_clusbuf,&trash),FAT12_ERR_ACCESS,"Trash ensure/hide");
        unchanged(finder_win_new_folder(&scene.sh,root,NULL,NULL),-21,"New Folder command");
        int idx=item_index(&scene.sh.windows[root].view,"SOURCE.TXT");
        CHECK(idx>=0,"source visible in real Finder model");
        finder_tgt_t target={0}; finder_move_result_t result;
        target.kind=FINDER_TGT_FOLDER; target.slot=root;
        target.idx=item_index(&scene.sh.windows[root].view,"FOLDER"); target.dir=2;
        unchanged(finder_ops_drop(&scene.sh,root,idx,&target,0,0,&result),-21,"drag move");
        target.kind=FINDER_TGT_TRASH; target.slot=-1;
        unchanged(finder_ops_drop(&scene.sh,root,idx,&target,0,0,&result),-21,"Trash move");
        uint16_t purged=0,refused=0; uint8_t dirty=0;
        unchanged(finder_shell_delete_item(&scene.sh,root,idx,&purged,&dirty),-21,"Delete Now file");
        idx=item_index(&scene.sh.windows[root].view,"EMPTY");
        unchanged(finder_shell_delete_item(&scene.sh,root,idx,&purged,&dirty),-21,"Delete Now folder");
        unchanged(finder_shell_empty_trash(&scene.sh,&purged,&refused,&dirty),-21,"Empty Trash");
        CHECK(purged==0,"Empty Trash purges nothing");
        int trash_slot=-1;
        CHECK(finder_win_open_trash(&scene.sh,&trash_slot,NULL)==0,"FAT16 Trash still readable");
        idx=item_index(&scene.sh.windows[trash_slot].view,"GONE.TXT");
        scene.sh.n_origins=1;
        memset(&scene.sh.origins[0],0,sizeof scene.sh.origins[0]);
        strcpy(scene.sh.origins[0].name83,"GONE.TXT");
        strcpy(scene.sh.origins[0].original83,"RESTORED.TXT");
        unchanged(finder_ops_put_away(&scene.sh,trash_slot,idx),-21,"Put Away");
        char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
        CHECK(finder_error_alert_text((finder_win_status_t)-21,"",lines)==1 &&
              !strcmp(lines[0],"Write protect error."),"catalogue write-protect wording in Finder alert");
    } else {
        CHECK(finder_fat_mkdir(NULL,"NEWFOLD",0)==0,"FAT12 New Folder succeeds");
        CHECK(finder_fat_create(NULL,"COPY.TXT",0,&slot)==0,"FAT12 copy create succeeds");
        CHECK(finder_fat_write_partial(NULL,0,slot,0,bytes,got,&got)==0,"FAT12 copy write succeeds");
        CHECK(desktop_db_write(&v,fat,sizeof fat,sec,g_finder_clusbuf,db,8)==0,"FAT12 Desktop DB save succeeds");
        CHECK(writes>0,"positive control actually writes sectors");
    }
    rewind(fp); CHECK(fwrite(disk,1,disk_len,fp)==disk_len,"write host result image"); fclose(fp);
    if(kind==16 && argc==4) copy_out(argv[3],source);
    free(disk);free(before);
    return TEST_SUMMARY("test-finder-storage");
}
