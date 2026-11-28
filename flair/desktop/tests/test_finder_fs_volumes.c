/* The Finder filesystem binding on two mounted volumes (C activation plan
 * step 10). tests/finder-fs-volumes.sh mints an installed-geometry C: and a
 * DATA A: with mtools; both have \SUB at the same physical cluster and
 * disjoint payloads. Every operation is called with A: identities while the
 * system binding names C:; the script then checks each image with mtools.
 * A binding that ignores the identity's volume enumerates or alters C:.
 * Ref: MTE Finder Interface (a specification names its volume and its
 * parent directory); kernel/include/fileio_fat.h fileio_fat_drive_volume. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define main windows_suite_main
#include "test_finder_windows.c"
#undef main
#include "finder_actions.h"
#include "fileio_fat.h"
#include "finder_ops.h"
#include "volume_db.h"
#include "desktop_db.h"
extern const finder_fs_t g_finder_fs_binding;
const fat12_volume_t *g_finder_vol;
void *g_finder_fat,*g_finder_secbuf;
uint32_t g_finder_fat_len;
unsigned g_finder_drive=2;
uint8_t g_finder_dirbuf[FINDER_FS_MAX_SPC*512];
uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC*512];
void serial_puts(const char *s){(void)s;}
void serial_puti(int32_t n){(void)n;}
void serial_putc(char c){(void)c;}
typedef struct disk { uint8_t *bytes; size_t len; unsigned writes; } disk_t;
static disk_t disks[2]; /* 0 = A, 1 = C */
static fat12_volume_t vols[2];
static uint8_t fats[2][16384],sector[512];
static int rd(void *ctx,uint32_t lba,uint32_t n,void *out)
{
    disk_t *d=ctx;
    if((uint64_t)(lba+n)*512>d->len) return -1;
    memcpy(out,d->bytes+(size_t)lba*512,(size_t)n*512); return 0;
}
static int wr(void *ctx,uint32_t lba,uint32_t n,const void *in)
{
    disk_t *d=ctx; d->writes++;
    if((uint64_t)(lba+n)*512>d->len) return -1;
    memcpy(d->bytes+(size_t)lba*512,in,(size_t)n*512); return 0;
}
static blockdev_t devs[2]={{&disks[0],rd,wr},{&disks[1],rd,wr}};
static uint32_t fat_bytes(int i){return (uint32_t)vols[i].bpb.sectors_per_fat*512u;}
const fat12_volume_t *fileio_fat_drive_volume(unsigned drive,void **fat,uint32_t *len)
{
    int i=drive==0?0:drive==2?1:-1;
    if(fat)*fat=NULL;
    if(len)*len=0;
    if(i<0) return NULL;
    if(fat)*fat=fats[i];
    if(len)*len=fat_bytes(i);
    return &vols[i];
}
static void load(int i,const char *path)
{
    FILE *fp=fopen(path,"rb");
    if(!fp) exit(2);
    fseek(fp,0,SEEK_END); disks[i].len=(size_t)ftell(fp); rewind(fp);
    disks[i].bytes=malloc(disks[i].len);
    if(!disks[i].bytes || fread(disks[i].bytes,1,disks[i].len,fp)!=disks[i].len) exit(2);
    fclose(fp);
    CHECK(fat12_mount(&vols[i],&devs[i],sector)==FAT12_OK,"mtools image mounts");
    CHECK(fat12_read_fat(&vols[i],fats[i],sizeof fats[i])==FAT12_OK,"its FAT loads");
}
static void store(int i,const char *path)
{
    FILE *fp=fopen(path,"wb");
    if(!fp || fwrite(disks[i].bytes,1,disks[i].len,fp)!=disks[i].len) exit(2);
    fclose(fp);
}
typedef struct seen { int n; char names[16][13]; finder_dir_id_t ids[16]; } seen_t;
static int collect(const finder_dirent_t *e,void *user)
{
    seen_t *s=user;
    if(s->n<16){snprintf(s->names[s->n],13,"%s",e->name83);s->ids[s->n]=e->start_cluster;s->n++;}
    return 0;
}
static int has(const seen_t *s,const char *name,finder_dir_id_t *id)
{
    for(int i=0;i<s->n;i++) if(!strcmp(s->names[i],name)){if(id)*id=s->ids[i];return 1;}
    return 0;
}
static int named_item(const finder_window_t *w, const char *name, uint32_t size)
{
    for (int i = 0; i < w->view.n; i++)
        if (!strcmp(w->view.icons[i].name, name) && w->meta[i].size == size)
            return i;
    return -1;
}

static void trash_scene(scene_t *s, const finder_fs_t *fs)
{
    scene_init(s, MOCK_DIRS, 2);
    finder_shell_bind_fs(&s->sh, fs);
    s->sh.volume_drive = 2;
    int idx = finder_desk_add(&s->sh.desk, FINDER_ICON_VOLUME, "DATA", 10, 30, 1);
    finder_desk_set_cluster(&s->sh.desk, idx, finder_dir_make(0, 0));
}

static void check_trash(const finder_fs_t *fs, finder_dir_id_t csub,
                        finder_dir_id_t asub)
{
    static scene_t s, reboot;
    finder_tgt_t trash = { .kind = FINDER_TGT_TRASH, .slot = -1, .idx = -1 };
    finder_move_result_t move;
    uint8_t db[2][DESKTOP_DB_MAX_BYTES];
    uint32_t len[2] = {0, 0};
    int slot = -1;
    const unsigned drives[] = {2, 0};
    const finder_dir_id_t dirs[] = {csub, asub};
    const uint32_t sizes[] = {9, 17};

    trash_scene(&s, fs);
    for (int i = 0; i < 2; i++) {
        CHECK(fs->read_partial(NULL, "DESKTOP.DB", finder_dir_make(drives[i], 0),
                               0, sizeof db[i], db[i], &len[i]) == FINDER_WIN_OK &&
              finder_volume_db_load(&s.sh, drives[i], db[i], len[i]) == 15,
              "load 15 independently authored Trash origins per volume");
    }
    for (int i = 0; i < 2; i++) {
        CHECK(finder_win_open(&s.sh, finder_dir_make(drives[i], 0), dirs[i],
                              i == 0 ? "SUB" : "DATAIN", 0, &slot, NULL) == FINDER_WIN_OK,
              "open the source folder on its own volume");
        int idx = named_item(&s.sh.windows[slot], "SAME.TXT", sizes[i]);
        CHECK(idx >= 0 &&
              finder_ops_drop(&s.sh, slot, idx, &trash, 0, 0, &move) == FINDER_WIN_OK &&
              !move.renamed && !move.origin_lost,
              "Trash stages SAME.TXT on its own volume and remembers a 16th origin");
    }
    CHECK(s.sh.n_origins == 32, "each volume keeps sixteen origins");
    trash_scene(&reboot, fs);
    for (int i = 0; i < 2; i++) {
        len[i] = finder_volume_db_encode(&s.sh, drives[i], db[i], sizeof db[i]);
        CHECK(len[i] && finder_volume_db_load(&reboot.sh, drives[i], db[i], len[i]) == 16,
              "each Desktop DB survives reload with its own sixteen origins");
    }
    slot = -1;
    CHECK(finder_win_open_trash(&reboot.sh, &slot, NULL) == FINDER_WIN_OK &&
          reboot.sh.windows[slot].view.n == 32,
          "Trash lists items from both volumes");
    if (slot < 0) return;
    for (int i = 1; i >= 0; i--) {
        int idx = named_item(&reboot.sh.windows[slot], "SAME.TXT", sizes[i]);
        CHECK(idx >= 0 && finder_ops_put_away(&reboot.sh, slot, idx) == FINDER_WIN_OK,
              "same-name Put Away restores the selected volume and origin");
    }
    uint16_t purged = 0, refused = 0;
    uint8_t dirty = 0;
    CHECK(finder_shell_empty_trash(&reboot.sh, &purged, &refused, &dirty) ==
          FINDER_WIN_ERR_LOCKED && purged == 29 && refused == 1,
          "Empty Trash purges each writable item and keeps the locked A: item");
    CHECK(dirty && reboot.sh.n_origins == 1 &&
          finder_dir_drive(reboot.sh.origins[0].dir_start, 2) == 0,
          "only the locked A: origin survives even though C: held the same name");
}

int main(int argc,char **argv)
{
    const finder_fs_t *fs=&g_finder_fs_binding;
    finder_dir_id_t aroot=finder_dir_make(0,0),asub=0,csub=0;
    seen_t s;
    uint8_t buf[64]; uint32_t got=0,slot=0,put=0;
    char trash[13];
    if(argc!=3) return 2;
    load(1,argv[1]); load(0,argv[2]);
    g_finder_vol=&vols[1]; g_finder_fat=fats[1]; g_finder_fat_len=fat_bytes(1);
    g_finder_secbuf=sector;
    memset(&s,0,sizeof s);
    CHECK(fs->enumerate(NULL,0,collect,&s)>=0 && has(&s,"CONLY.TXT",NULL) &&
          has(&s,"SUB",&csub) && !has(&s,"AONLY.TXT",NULL),
          "an untagged root is the system C:");
    memset(&s,0,sizeof s);
    CHECK(fs->enumerate(NULL,aroot,collect,&s)>=0 && has(&s,"AONLY.TXT",NULL) &&
          !has(&s,"CONLY.TXT",NULL) && has(&s,"SUB",&asub),
          "the A: root enumerates A:");
    CHECK(finder_dir_cluster(asub)==finder_dir_cluster(csub) && asub!=csub &&
          finder_dir_drive(asub,2)==0,
          "A:\\SUB keeps its A: identity although C:\\SUB has the same cluster");
    memset(buf,0,sizeof buf);
    CHECK(fs->read_partial(NULL,"FILE.TXT",asub,0,sizeof buf-1,buf,&got)==0 &&
          !strncmp((char *)buf,"A payload",9),"A:\\SUB\\FILE.TXT reads A: bytes");
    CHECK(fs->dir_ident(NULL,asub,aroot,"SUB")==0,"A:\\SUB is identified on A:");
    CHECK(fs->dir_ident(NULL,asub,0,"SUB")!=0,"a parent on another volume is stale");
    CHECK(fs->mkdir(NULL,"NEWA",aroot)==0,"New Folder in the A: root");
    CHECK(fs->create(NULL,"NEW.TXT",asub,&slot)==0 &&
          fs->write_partial(NULL,asub,slot,0,(const uint8_t *)"written on A\r\n",14,&put)==0 &&
          put==14,"create and write A:\\SUB\\NEW.TXT");
    CHECK(fs->move(NULL,"AONLY.TXT",aroot,"AONLY.TXT",asub)==0,"move A:\\AONLY.TXT into A:\\SUB");
    CHECK(fs->unlink(NULL,"GONE.TXT",asub)==0,"delete A:\\SUB\\GONE.TXT");
    CHECK(fs->trash_name(NULL,"FILE.TXT",asub,trash)==0,"Trash name probe on A:");
    unsigned before=disks[1].writes+disks[0].writes;
    CHECK(fs->move(NULL,"CONLY.TXT",0,"CONLY.TXT",aroot)!=0 &&
          disks[1].writes+disks[0].writes==before,
          "a move between volumes is refused before any write");
    CHECK(fs->mkdir(NULL,"NEWC",0)==0,"New Folder in the system root still writes C:");
    memset(&s,0,sizeof s);
    finder_dir_id_t data_in = FINDER_DIR_INVALID;
    CHECK(fs->enumerate(NULL,aroot,collect,&s)>=0 && has(&s,"DATAIN",&data_in),
          "independent A: origin directory is present");
    check_trash(fs, finder_dir_make(2, finder_dir_cluster(csub)), data_in);
    store(1,argv[1]); store(0,argv[2]);
    return TEST_SUMMARY("test-finder-fs-volumes");
}
