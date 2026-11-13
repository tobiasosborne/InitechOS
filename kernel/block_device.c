/* Physical INT13, DEC-17. IBM BIOS TR pp2-58..67; IBM DOS TR pp9-8..9.
 * Evidence: docs/research/block-device-ground-truth.md. No FAT origin here. */
#include "block_device.h"
#include "block_device_contract.h"
#include "drive_letters.h"
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
/* L042 (docs/audits/2026-10-23-dos): FORMAT/SYS write C:/D: through this
 * vector, behind those volumes' mounted FAT caches. DOS 3.3 keeps one view
 * of a drive (UG p119: SYS, then COPY COMMAND.COM onto the same disk), so
 * every mounted fixed volume hears its raw writes, volume-relative. */
static du_map watched[2];
static void (*watch_notice[2])(unsigned,uint32_t,uint32_t,const void *);
void block_device_watch(unsigned letter,const du_map *m,void(*w)(unsigned,uint32_t,uint32_t,const void *))
{ if(letter==2||letter==3){watched[letter-2]=*m;watch_notice[letter-2]=w;} }
static void overlap(uint32_t lba,uint32_t count,const uint8_t *buf,uint32_t base,uint32_t size,unsigned letter,
                    void(*a)(uint32_t,uint32_t,const void *),void(*b)(unsigned,uint32_t,uint32_t,const void *))
{
    uint32_t lo=lba>base?lba:base,hi=lba+count<base+size?lba+count:base+size;
    if(lo>=hi)return;
    if(a)a(lo-base,hi-lo,buf+(lo-lba)*BD_sector_bytes);
    else b(letter,lo-base,hi-lo,buf+(lo-lba)*BD_sector_bytes);
}
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
/* DEC-18: one physical-position resolver for every external utility.
 * PM ruling + DEC-07a, IBM DOS TR pp9-6..9. Does not mount/switch DOS. */
static uint32_t map32(const uint8_t *p)
{ return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
int block_device_map(unsigned letter,du_map *m)
{
    init();
    if(!m || letter==1 || letter>3)return -1;
    unsigned drive=letter==0?DL_a_disk:letter==2?DL_c_disk:DL_d_disk;
#ifdef DRIVE_MUT_DISK
    if(!letter)drive=0x80;
#endif
    unsigned part=letter==0?DL_a_partition:letter==2?DL_c_partition:DL_d_partition;
    unsigned i=drive-0x80;uint8_t b[512];
    if(i>1 || !sizes[i] || disks[i].read_sectors(disks[i].ctx,0,1,b))return -1;
    uint32_t base=0,n=sizes[i];
    if(!letter && (b[0]==0xe9 || (b[0]==0xeb && b[2]==0x90)) &&
       b[11]==0 && b[12]==2 && b[13] && b[16]==2) {
        part=0;n=b[19]|((uint32_t)b[20]<<8);if(!n)n=map32(b+32);
        if(!n || n>sizes[i] || b[510]!=0x55 || b[511]!=0xaa)return -1;
    }
    else {
        if(!part)part=1;
        if(b[510]!=0x55 || b[511]!=0xaa)return -1;
        const uint8_t *p=b+446+(part-1)*16;
        if(p[4]!=1 && p[4]!=4 && p[4]!=6)return -1;
        base=map32(p+8);n=map32(p+12);
        if(!base || !n || base>=sizes[i] || n>sizes[i]-base)return -1;
    }
    m->disk=drive;m->partition=part;m->base=base;m->sectors=n;
    m->flags=(!letter?DL_removable_flag:0) |
        ((drive==mounted_drive && base==mounted_base)?DL_running_flag:0);
#ifdef DRIVE_MUT_RUNNING
    if(drive==mounted_drive)m->flags|=DL_running_flag;
#endif
    return 0;
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
void KERNEL_ASM_ABI int13_dispatch(int_frame_t *f)
{
    unsigned ah=(f->eax>>8)&255u,drive=f->edx&255u;
    init();
    if(ah==DL_function) {
        if(!f->ebx || f->ebx>0xffffffffu-DL_map_bytes ||
           block_device_map(f->eax&255u,(du_map *)(uintptr_t)f->ebx))
            result(f,BD_status_invalid);
        else result(f,0);
        return;
    }
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
        if(!rc && write_notice && drive==mounted_drive)
            overlap(lba,count,buf,mounted_base,mounted_size,0,write_notice,0);
#ifndef BLOCK_DEVICE_MUT_WATCH
        for(unsigned w=0;!rc && w<2;w++)
            if(watch_notice[w] && drive==watched[w].disk)
                overlap(lba,count,buf,watched[w].base,watched[w].sectors,w+2,0,watch_notice[w]);
#endif
    }
    result(f,block_device_status(rc));
}
