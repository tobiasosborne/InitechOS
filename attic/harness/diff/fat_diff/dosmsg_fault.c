/* Factory-only disconnected-device fixture. The live ATA read remains intact;
 * removal is simulated at the block-device seam on the sentinel's first sector.
 * Law 1: ata.h ATA_ERR_NO_DRIVE; DOS 3.3 Appendix F p.350/PDF435. */
#include "ata.h"
static blockdev_t original;
static int unavailable_read(void *ctx,uint32_t lba,uint32_t count,void *buf)
{
    (void)ctx;
    int rc=original.read_sectors(original.ctx,lba,count,buf);
    static const char sentinel[]="DOSMSG-FAULT";
    if (!rc) {
        const unsigned char *p=buf;
        unsigned i=0;
        while (i<sizeof(sentinel)-1 && p[i]==(unsigned char)sentinel[i]) i++;
        if (i==sizeof(sentinel)-1) return ATA_ERR_NO_DRIVE;
    }
    return rc;
}
void dosmsg_fixture_arm(blockdev_t *disk)
{
    original=*disk;
    disk->read_sectors=unavailable_read;
}
