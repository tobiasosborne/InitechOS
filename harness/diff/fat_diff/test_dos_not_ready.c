/* vlhp: MS-DOS 3.3 Reference Appendix F p.350/PDF435; ADR-0003
 * Appendix A/C + ata.h ATA_ERR_NO_DRIVE. Independent console literal. */
#include <stdio.h>
#include <string.h>
#include "ata.h"
#include "int21.h"
#include "blockdev_file.h"

static blockdev_file_t backing;
static int removed;
static char text[512];
static unsigned used;
static void capture(char c) { if (used+1<sizeof(text)) text[used++]=c; }
static int fail_key(void) { return 'f'; }
static int read_disk(void *ctx,uint32_t lba,uint32_t n,void *buf)
{
    (void)ctx;
    if (removed) return ATA_ERR_NO_DRIVE;
    return backing.dev.read_sectors(backing.dev.ctx,lba,n,buf);
}
int main(int argc,char **argv)
{
    if (argc!=2 || blockdev_file_open(&backing,argv[1])) return 2;
    blockdev_t inner={ .read_sectors=read_disk };
    crit_blockdev_t wrapped;
    unsigned char actual[512],expected[512];
    crit_blockdev_init(&wrapped,&inner);
    int21_set_sink(capture);
    int21_set_conin(fail_key,NULL);
    crit_blockdev_set_hook(int21_run_critical_error);
    if (wrapped.dev.read_sectors(wrapped.dev.ctx,0,1,actual) ||
        backing.dev.read_sectors(backing.dev.ctx,0,1,expected) ||
        memcmp(actual,expected,sizeof(actual)) || used) return 2;
    removed=1;
    int rc=wrapped.dev.read_sectors(wrapped.dev.ctx,0,1,actual);
    if (rc!=ATA_ERR_NO_DRIVE || strcmp(text,
        "Not ready reading drive A:\r\nAbort, Retry, Fail?")) {
        fprintf(stderr,"RED dos-not-ready: missing exact MSG-DOS-0006 before A/R/F (got %s)\n",text);
        return 1;
    }
    blockdev_file_close(&backing);
    puts("GREEN test-dos-not-ready: disconnected backend -> exact MSG-DOS-0006 + A/R/F; healthy reads unchanged");
    return 0;
}
