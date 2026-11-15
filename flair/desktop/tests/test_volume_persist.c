/* Exercise the actual resident persistence function over independently
 * authored per-volume DB bytes. Public FAT bindings are narrow observers;
 * no Finder code supplies the expected records or the target volume. */
#include <stdio.h>
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
static fat12_volume_t a,c;
const fat12_volume_t *g_finder_vol=&c;
unsigned g_finder_drive=2;
static uint8_t afat[4608],cfat[4608],sector[512];
void *g_finder_fat=cfat,*g_finder_secbuf=sector;
uint32_t g_finder_fat_len=sizeof cfat;
uint8_t g_finder_clusbuf[FINDER_FS_MAX_SPC*512];
uint8_t g_finder_db_buf[FINDER_DB_MAX_BYTES];
int g_finder_db_dirty;
finder_win_status_t g_finder_io_error;
static uint8_t saved[2][FINDER_DB_MAX_BYTES];
static uint32_t lengths[2];
static unsigned writes[2];
void serial_puts(const char *s){(void)s;}
void serial_putu(uint32_t n){(void)n;}
void serial_puti(int32_t n){(void)n;}
void serial_putc(char c1){(void)c1;}
const fat12_volume_t *fileio_fat_drive_volume(unsigned drive,void **fat,uint32_t *len)
{
    *fat=NULL;*len=0;
    if(drive!=0 && drive!=2)return NULL;
    *fat=drive==0?afat:cfat;*len=sizeof afat;return drive==0?&a:&c;
}
int __wrap_desktop_db_write(const fat12_volume_t *vol,void *fat,uint32_t fat_len,
    void *sec,void *cluster,const void *bytes,uint32_t len)
{
    (void)sec;(void)cluster;int i=vol==&a?0:1;
    CHECK(vol==&a || vol==&c,"write names one of the actual mounted volumes");
    CHECK(fat==(i==0?(void*)afat:(void*)cfat) && fat_len==sizeof afat,"each DB uses its own matching FAT cache");
    if(len>sizeof saved[i])return FAT12_ERR_BUFFER;
    memcpy(saved[i],bytes,len);lengths[i]=len;writes[i]++;return 0;
}
int __wrap_desktop_db_read(const fat12_volume_t *vol,const void *fat,uint32_t fat_len,
    void *sec,void *cluster,void *out,uint32_t cap,uint32_t *len)
{
    (void)fat;(void)fat_len;(void)sec;(void)cluster;int i=vol==&a?0:1;
    if(!lengths[i])return FAT12_ERR_NOT_FOUND;
    if(lengths[i]>cap)return FAT12_ERR_BUFFER;
    memcpy(out,saved[i],lengths[i]);*len=lengths[i];return 0;
}
const char *__wrap_desktop_db_recovery(void){return NULL;}
/* Locked IDB1 version1: root view, then an unrenamed Trash origin. */
static const uint8_t want_c[56]={
 [0]='I',[1]='D',[2]='B',[3]='1',[4]=1,[6]=2,
 [8]=4,[25]=70,[27]=90,[32]=5,
 [36]='S',[37]='A',[38]='M',[39]='E',[40]='.',[41]='T',[42]='X',[43]='T'
};
static const uint8_t want_a[56]={
 [0]='I',[1]='D',[2]='B',[3]='1',[4]=1,[6]=2,
 [8]=4,[25]=46,[27]=115,[32]=5,
 [36]='S',[37]='A',[38]='M',[39]='E',[40]='.',[41]='T',[42]='X',[43]='T'
};
int main(void)
{
    c.fat_type=a.fat_type=FAT_TYPE_FAT12;shell.volume_drive=2;
    shell.desk.icons=shell.desk_icons;shell.n_views=shell.n_origins=2;
    shell.views[0]=(finder_view_rec_t){.dir_start=0x30000,.x=70,.y=90};
    shell.views[1]=(finder_view_rec_t){.dir_start=0x10000,.x=46,.y=115};
    shell.origins[0]=(finder_origin_rec_t){.dir_start=0x30000,.name83="SAME.TXT",.original83="SAME.TXT"};
    shell.origins[1]=(finder_origin_rec_t){.dir_start=0x10000,.name83="SAME.TXT",.original83="SAME.TXT"};
    g_finder_db_dirty=1;finder_desk_persist();
    CHECK(writes[0]==1 && writes[1]==1,"both volumes persist their own changed state");
    CHECK(lengths[1]==sizeof want_c && !memcmp(saved[1],want_c,sizeof want_c),"C DB contains only C view and C Trash origin");
    CHECK(lengths[0]==sizeof want_a && !memcmp(saved[0],want_a,sizeof want_a),"A DB contains only A frame view and A Trash origin");
    unsigned ac=writes[0],cc=writes[1];
    g_finder_db_dirty=1;finder_desk_persist();
    CHECK(writes[0]==ac && writes[1]==cc,"identical state performs no disk writes on either volume");
    shell.views[1].x=100;g_finder_db_dirty=1;finder_desk_persist();
    uint8_t moved_a[56];memcpy(moved_a,want_a,sizeof moved_a);moved_a[25]=100;
    CHECK(writes[1]==cc && lengths[1]==sizeof want_c && !memcmp(saved[1],want_c,sizeof want_c),"changing A never rewrites C");
    CHECK(writes[0]==ac+1 && lengths[0]==sizeof moved_a && !memcmp(saved[0],moved_a,sizeof moved_a),"only A receives A's moved view");
    ac=writes[0];
    shell.n_views=shell.n_origins=1;shell.views[0].x=80;
    g_finder_db_dirty=1;finder_desk_persist();
    CHECK(writes[0]==ac && lengths[0]==sizeof moved_a && !memcmp(saved[0],moved_a,sizeof moved_a),
          "an OS-mounted A without a loaded Finder context is never overwritten");
    static finder_shell_t cold;
    cold.volume_drive=2;cold.desk.icons=cold.desk_icons;
    CHECK(finder_volume_db_load(&cold,2,want_c,sizeof want_c)==2,"cold C imports its view and origin");
    CHECK(finder_volume_db_load(&cold,0,want_a,sizeof want_a)==2,"cold A imports without replacing C");
    const finder_view_rec_t *cv=finder_shell_find_view(&cold,0x30000);
    const finder_view_rec_t *av=finder_shell_find_view(&cold,0x10000);
    CHECK(cold.n_views==2 && cv && av && cv->x==70 && cv->y==90 && av->x==46 && av->y==115,
          "cold views retain independent volume identities and positions");
    CHECK(cold.n_origins==2 && cold.origins[0].dir_start==0x30000 && cold.origins[1].dir_start==0x10000,
          "identical staged names keep independent per-volume origins");
    uint8_t encoded[56];
    CHECK(finder_volume_db_encode(&cold,2,encoded,sizeof encoded)==sizeof want_c && !memcmp(encoded,want_c,sizeof encoded),
          "C exports the independent literal DB after cold import");
    CHECK(finder_volume_db_encode(&cold,0,encoded,sizeof encoded)==sizeof want_a && !memcmp(encoded,want_a,sizeof encoded),
          "A exports the independent literal DB after cold import");
    (void)finder_volume_db_load(&cold,0,want_a,sizeof want_a);
    CHECK(cold.n_views==2 && cold.n_origins==2 && finder_shell_find_view(&cold,0x30000),
          "reloading A neither duplicates records nor evicts C");
    return TEST_SUMMARY("test-volume-persist");
}
