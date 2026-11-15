/* Independent C:/A: default-vs-I/O cases for the resident startup binding.
 * DOS maintains the default drive separately from the selected backend.
 * Ref: kernel/include/int21.h drive selector contract and delivery judgement.
 * These stand-ins model only those published calls, not the binding algorithm. */
#include <string.h>
#include "test_assert.h"
#include "volume_binding.h"
#include "fileio_fat.h"
#include "int21.h"
TEST_HARNESS();
static fat12_volume_t a,c;
static uint8_t afat[512],cfat[512],window[1024];
static fat12_fat_window_t win;
static unsigned default_drive,selected,selects,missing;
static int21_cwd_snapshot_t cwd;
unsigned int21_current_drive(void) {return default_drive;}
unsigned int21_io_drive(void) {return selected;}
int21_cwd_snapshot_t int21_cwd_save(void) {return cwd;}
void int21_cwd_restore(const int21_cwd_snapshot_t *s) {cwd=*s;selected=s->io_drive;}
uint16_t int21_select_path(const char **path)
{
    const char *p=*path;
    selects++;
    CHECK(strlen(p)==3 && p[1]==':' && p[2]=='\\',"select explicit root of named drive");
    unsigned drive=(unsigned)(p[0]-'A');
    if(drive==missing) return 15;
    selected=drive; cwd.io_drive=(uint8_t)drive; *path=p+2;return 0;
}
const fat12_volume_t *fileio_fat_current_volume(void) {return selected==2?&c:&a;}
void *fileio_fat_fat_buffer(uint32_t *n)
{
    *n=selected==2 && c.fat_window?0:512;
    return selected==2?(c.fat_window?NULL:cfat):afat;
}
static void scenario(unsigned def,unsigned previous,int absent,int fat16)
{
    finder_volume_binding_t got={0};
    default_drive=def;selected=previous;selects=0;missing=absent?def:99;
    memset(&cwd,0,sizeof cwd);cwd.current_drive=(uint8_t)def;cwd.io_drive=(uint8_t)previous;
    strcpy(cwd.path,"SAMPLES");cwd.start_cluster=19;
    strcpy(cwd.paths[0],"DATA");strcpy(cwd.paths[2],"APPS");cwd.starts[0]=7;cwd.starts[2]=11;
    int21_cwd_snapshot_t saved=cwd;
    win.buf=window;c.fat_window=fat16?&win:NULL;
    int rc=finder_bind_startup_volume(&got);
    CHECK(selects==1,"startup selects default explicitly even when a backend was already current");
    CHECK(!memcmp(&cwd,&saved,sizeof cwd) && selected==previous,"startup restores every caller CWD and I/O selection");
    if(absent) {
        CHECK(rc==FAT12_ERR_PARTITION && !got.volume && !got.fat && !got.fat_len,
              "missing default fails loudly without borrowing the last backend");
    } else {
        CHECK(rc==0 && got.drive==def && got.volume==(def==2?&c:&a),"Finder record belongs to default drive, never last I/O drive");
        CHECK(got.fat==(def==2?(fat16?window:cfat):afat) && got.fat_len==(fat16?0:512),
              "volume and cache captured under the same selection, including FAT16 window");
    }
}
int main(void)
{
    scenario(2,0,0,0); /* installed C:, optional A: most recently accessed */
    scenario(2,2,0,0); /* installed C: alone */
    scenario(0,2,0,0); /* existing A: product, prior C: I/O */
    scenario(2,0,1,0); /* no silent fallback to another user's disk */
    scenario(2,0,0,1); /* record/cache pairing for read-only FAT16 */
    return TEST_SUMMARY("test-volume-binding");
}
