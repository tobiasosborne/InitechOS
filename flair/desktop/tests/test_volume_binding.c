/* Independent C:/A: cases for the resident Finder startup binding.
 * The Finder belongs to the installed system volume: the kernel's initial
 * binding, published as fileio_fat_system_drive(), independent of AH=0Eh
 * default-drive changes and of the last backend selected by another
 * service. Its record and FAT cache come from fileio_fat_drive_volume(),
 * which names the volume without selecting it.
 * Refs: kernel/include/fileio_fat.h (both contracts); C activation plan
 * step 8 (boot C, type A: then FLAIR, the Finder still opens C); accepted
 * GM8 delivery design (C carries the system, A is optional data).
 * These stand-ins model only those published calls, not the binding
 * algorithm. The DOS drive/CWD calls are modelled so that any use of them
 * is counted: the binding must neither need nor disturb them. */
#include <string.h>
#include "test_assert.h"
#include "volume_binding.h"
#include "fileio_fat.h"
#include "int21.h"
TEST_HARNESS();
static fat12_volume_t a,c;
static uint8_t afat[512],cfat[512],window[1024];
static fat12_fat_window_t win;
static unsigned default_drive,selected,selects;
static int system_drive,dropped;
static int21_cwd_snapshot_t cwd;
unsigned int21_current_drive(void) {return default_drive;}
unsigned int21_io_drive(void) {return selected;}
int21_cwd_snapshot_t int21_cwd_save(void) {return cwd;}
void int21_cwd_restore(const int21_cwd_snapshot_t *s) {cwd=*s;selected=s->io_drive;}
uint16_t int21_select_path(const char **path)
{
    const char *p=*path;
    selects++;
    if(strlen(p)!=3 || p[1]!=':' || p[2]!='\\') return 15;
    selected=(unsigned)(p[0]-'A'); cwd.io_drive=(uint8_t)selected; *path=p+2; return 0;
}
/* The DOS I/O volume: whatever drive was selected last. */
const fat12_volume_t *fileio_fat_current_volume(void) {return selected==2?&c:&a;}
void *fileio_fat_fat_buffer(uint32_t *n)
{
    *n=selected==2 && c.fat_window?0:512;
    return selected==2?(c.fat_window?NULL:cfat):afat;
}
int fileio_fat_system_drive(void) {return system_drive;}
const fat12_volume_t *fileio_fat_drive_volume(unsigned letter, void **fat,
                                             uint32_t *fat_len)
{
    const fat12_volume_t *v=0; void *f=0; uint32_t n=0;
    if(letter==2 && !(dropped&4)) {v=&c; f=c.fat_window?NULL:cfat; n=c.fat_window?0:512;}
    if(letter==0 && !(dropped&1)) {v=&a; f=afat; n=512;}
    if(fat) *fat=f;
    if(fat_len) *fat_len=n;
    return v;
}
/* sys: initial binding letter (-1 absent); def: DOS default drive (what the
 * user last typed, e.g. A: before FLAIR); io: last selected backend. */
static void scenario(const char *why,int sys,unsigned def,unsigned io,
                     int gone,int fat16)
{
    finder_volume_binding_t got;
    memset(&got,0x5a,sizeof got);
    system_drive=sys;default_drive=def;selected=io;selects=0;dropped=gone;
    memset(&cwd,0,sizeof cwd);cwd.current_drive=(uint8_t)def;cwd.io_drive=(uint8_t)io;
    strcpy(cwd.path,"SAMPLES");cwd.start_cluster=19;
    strcpy(cwd.paths[0],"DATA");strcpy(cwd.paths[2],"APPS");cwd.starts[0]=7;cwd.starts[2]=11;
    int21_cwd_snapshot_t saved=cwd;
    win.buf=window;c.fat_window=fat16?&win:NULL;
    int rc=finder_bind_startup_volume(&got);
    printf("  case: %s\n",why);
    CHECK(!memcmp(&cwd,&saved,sizeof cwd) && selected==io && default_drive==def,
          "binding leaves every CWD, the default drive and the I/O selection unchanged");
    CHECK(selects==0,"binding names the system volume without selecting a DOS drive");
    if(sys<0 || (gone & (1<<sys))) {
        CHECK(rc==FAT12_ERR_PARTITION && !got.volume && !got.fat && !got.fat_len,
              "absent system volume fails loudly without borrowing another drive");
        return;
    }
    CHECK(rc==0 && got.drive==(unsigned)sys && got.volume==(sys==2?&c:&a),
          "Finder record is the system volume, never the default or last I/O drive");
    CHECK(got.fat==(sys==2?(fat16?(void *)window:(void *)cfat):(void *)afat) &&
          got.fat_len==(fat16?0u:512u),
          "the named volume's own FAT cache, including the FAT16 window");
}
int main(void)
{
    scenario("installed C:, user typed A: before FLAIR",2,0,0,0,0);
    scenario("installed C:, default C:, A: accessed last",2,2,0,0,0);
    scenario("installed C: alone",2,2,2,0,0);
    scenario("legacy A: system (startup fallback until plan step 21), default C:",0,2,2,0,0);
    scenario("no system mount: no silent fallback to the default drive",-1,0,0,0,0);
    scenario("system C: dropped after a failed remount",2,0,0,4,0);
    scenario("read-only FAT16 system C: keeps its window buffer",2,0,0,0,1);
    return TEST_SUMMARY("test-volume-binding");
}
