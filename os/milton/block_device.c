/* Physical INT13, DEC-17. IBM BIOS TR pp2-58..67; IBM DOS TR pp9-8..9.
 * Evidence: docs/research/block-device-ground-truth.md. No FAT origin here. */
#include "block_device.h"
#include "block_device_contract.h"
#include "ata.h"
#include <stddef.h>
static blockdev_t disks[2];
static uint32_t sizes[2];
static int initialized;
static uint8_t mounted_drive;
static uint32_t mounted_base,mounted_size;
static void (*write_notice)(uint32_t,uint32_t,const void *);
void block_device_bind(unsigned i,const blockdev_t *d,uint32_t n)
{ initialized=1; if(i<2){disks[i]=*d;sizes[i]=n;} }
void block_device_mounted(uint8_t d,uint32_t b,uint32_t n,void(*w)(uint32_t,uint32_t,const void *))
{ mounted_drive=d;mounted_base=b;mounted_size=n;write_notice=w; }
static void init(void)
{
    if(initialized)return;
    initialized=1;
#ifndef BLOCK_DEVICE_HOST
    static ata_ctx_t ctx[2];
    ata_ctx_init_primary_master(&ctx[0]);ata_ctx_init_primary_slave(&ctx[1]);
    for(unsigned i=0;i<2;i++) {
        ata_blockdev_init(&disks[i],&ctx[i]);
        if(ata_identify(&ctx[i],&sizes[i])!=0)sizes[i]=0;
    }
#endif
}
int block_device_chs(uint16_t cx,uint8_t head,uint32_t sectors,uint32_t *lba)
{
    uint32_t cylinder=(cx>>8)|((cx&0xc0u)<<2),sector=cx&63u;
    if(!sector || sector>BD_sectors_per_track || head>=BD_heads)return -1;
    unsigned heads=BD_heads;
#ifdef BLOCK_DEVICE_MUT_HEADS
    heads--;
#endif
    uint32_t l=(cylinder*heads+head)*BD_sectors_per_track+sector-1;
    if(l>=sectors)return -1;
    *lba=l;return 0;
}
uint8_t block_device_status(int e)
{
    switch(e) {
    case 0:return BD_status_ok;
    case ATA_ERR_ARG:return BD_status_invalid;
    case ATA_ERR_NO_DRIVE:case ATA_ERR_TIMEOUT:return BD_status_timeout;
    default:return BD_status_controller;
    }
}
static void result(int_frame_t *f,unsigned s)
{
    f->eax=(f->eax&~0xff00u)|(s<<8);f->eflags&=~1u;
#ifndef BLOCK_DEVICE_MUT_CARRY
    if(s)f->eflags|=1u;
#endif
}
void int13_dispatch(int_frame_t *f)
{
    unsigned ah=(f->eax>>8)&255u,drive=f->edx&255u;
    init();
    /* spec/hardware.json contracts ATA PIO, no FDC. */
    if(drive<BD_master_drive || drive>BD_slave_drive) {
        if(ah==0x15 && drive>=0x80) {
            f->ecx&=~65535u;f->edx&=~65535u;result(f,0);
        } else result(f,BD_status_invalid);
        return;
    }
    unsigned i=drive-BD_master_drive;
    if(ah!=0 && ah!=2 && ah!=3 && ah!=8 && ah!=0x15) {
        result(f,BD_status_invalid);return;
    }
    if(ah==0x15) {
        f->ecx=(f->ecx&~65535u)|(sizes[i]>>16);
        f->edx=(f->edx&~65535u)|(sizes[i]&65535u);
        result(f,0);if(sizes[i])f->eax|=0x0300u; /* type, not error */
        return;
    }
    if(!sizes[i]) {
        if(ah==8){f->ecx&=~65535u;f->edx&=~65535u;}
        result(f,ah==8?BD_status_parameters:BD_status_timeout);return;
    }
    if(ah==0) {
        int rc=0;
#ifndef BLOCK_DEVICE_HOST
        rc=ata_reset(disks[i].ctx);
#endif
        result(f,block_device_status(rc));return;
    }
    if(ah==8) {
        uint32_t c=sizes[i]/(BD_heads*BD_sectors_per_track);
        if(!c)c=1;
        if(c>BD_max_cylinders)c=BD_max_cylinders;
        c--;
        f->ecx=(f->ecx&~65535u)|((c&255u)<<8)|((c>>2)&192u)|BD_sectors_per_track;
        f->edx=(f->edx&~65535u)|((BD_heads-1)<<8)|((sizes[0]!=0)+(sizes[1]!=0));
        result(f,0);return;
    }
    if(ah!=2 && ah!=3){result(f,BD_status_invalid);return;}
    uint32_t lba,count=f->eax&255u;
    if(!count || !f->ebx || f->ebx>0xffffffffu-count*BD_sector_bytes) {
        result(f,BD_status_invalid);return;
    }
    if(block_device_chs((uint16_t)f->ecx,(uint8_t)(f->edx>>8),sizes[i],&lba)!=0 ||
       count>sizes[i]-lba){result(f,BD_status_sector_not_found);return;}
    void *buf=(void *)(uintptr_t)f->ebx;int rc;
    if(ah==2)rc=disks[i].read_sectors(disks[i].ctx,lba,count,buf);
    else {
        if(!disks[i].write_sectors){result(f,BD_status_write_protected);return;}
#ifdef BLOCK_DEVICE_MUT_RELATIVE
        lba+=mounted_base;
#endif
        rc=disks[i].write_sectors(disks[i].ctx,lba,count,buf);
        if(!rc && write_notice && drive==mounted_drive) {
            uint32_t lo=lba>mounted_base?lba:mounted_base;
            uint32_t hi=lba+count<mounted_base+mounted_size?lba+count:mounted_base+mounted_size;
            if(lo<hi)write_notice(lo-mounted_base,hi-lo,(const uint8_t *)buf+(lo-lba)*BD_sector_bytes);
        }
    }
    result(f,block_device_status(rc));
}
