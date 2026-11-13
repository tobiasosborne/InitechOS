/* Scripted ATA bus oracle. Ref: T13/1321D rev3 Sec 7.4 Command register,
 * Sec 9 PIO protocols; local digest docs/research/khygiene-ground-truth.md.
 * This tests the REAL ata.c, including every port write and error return. */
#include <stdint.h>
#include <string.h>
#include "ata.h"
#include "test_assert.h"
TEST_HARNESS();
static unsigned command, polls, words, early, writes, complete_polls;
static int response, pre_response, flush_response, completion_response;
static ata_ctx_t ctx = {0x1f0,0x3f6,0xf0};
static uint8_t buf[1024];
static void reset(void)
{
    command=polls=words=early=writes=complete_polls=0;
    response=0x48; pre_response=-1; flush_response=0x40; completion_response=0x40;
    for (unsigned i=0;i<512;++i) buf[i]=(uint8_t)(i/2);
}
uint8_t ata_test_inb(uint16_t port)
{
    if (port != 0x1f7) return 0x40;
    if (!command) {
        ++polls;
        if (pre_response >= 0) return (uint8_t)pre_response;
        return polls < 3 ? 0x80 : polls < 5 ? 0x10 : 0x40;
    }
    if (command==0xe7) return (uint8_t)flush_response;
    if (command==0x30 && words==256) {
        ++complete_polls;
        return complete_polls < 3 ? 0x80 : (uint8_t)completion_response;
    }
    if (response==-1) return ++polls < 7 ? 0x80 : 0xff;
    return (uint8_t)response;
}
uint16_t ata_test_inw(uint16_t port)
{
    CHECK(port==0x1f0, "READ data port");
    return (uint16_t)words++;
}
void ata_test_outb(uint16_t port, uint8_t value)
{
    ++writes;
    if (port==0x1f6) return;
    if (port>=0x1f2 && port<=0x1f7 && !command && polls<5) ++early;
    if (port==0x1f7) {
        if (value==0xe7 && complete_polls<3) ++early;
        command=value;
    }
}
void ata_test_outw(uint16_t port, uint16_t value)
{
    CHECK(port==0x1f0 && value==(uint16_t)((words&255u)*0x101u), "WRITE exact PIO word");
    ++words;
}
int main(void)
{
    reset();
    CHECK(ata_read_sectors(&ctx,1,1,buf)==0 && words==256, "READ 256 words");
    CHECK(!early, "bcg.12 BSY=0 DRDY=1 BEFORE taskfile/command writes");
    CHECK(buf[2]==1 && buf[3]==0, "READ little-endian bytes");
    for (unsigned i=0;i<512;++i) buf[i]=(uint8_t)(i/2);
    reset();
    CHECK(ata_write_sectors(&ctx,1,1,buf)==0 && words==256 && command==0xe7,
          "WRITE data then FLUSH");
    CHECK(!early, "bcg.12 final WRITE completion BEFORE FLUSH command");
    for (int op=0;op<2;++op) {
        int (*read_op)(void *,uint32_t,uint32_t,void *)=ata_read_sectors;
#define IO() (op ? ata_write_sectors(&ctx,1,1,buf) : read_op(&ctx,1,1,buf))
        const int statuses[]={0xff,0x80,0x41,0x60,0x40,-1};
        const int errors[]={ATA_ERR_NO_DRIVE,ATA_ERR_TIMEOUT,ATA_ERR_DEVICE,
                            ATA_ERR_DEVICE,ATA_ERR_DEVICE,ATA_ERR_NO_DRIVE};
        for (unsigned i=0;i<6;++i) {
            reset(); response=statuses[i];
            CHECK(IO()==errors[i], "READ/WRITE distinct post-command error code");
        }
        reset(); pre_response=0xff;
        CHECK(IO()==ATA_ERR_NO_DRIVE && !command, "floating bus rejected BEFORE command");
        reset(); pre_response=0x80;
        CHECK(IO()==ATA_ERR_TIMEOUT && !command, "pre-command busy timeout");
        reset(); pre_response=0;
        CHECK(IO()==ATA_ERR_NO_DRIVE && !command, "absent drive status zero");
        reset();
        CHECK((op ? ata_write_sectors(&ctx,0xfffffff,2,buf)
                  : ata_read_sectors(&ctx,0xfffffff,2,buf))==ATA_ERR_ARG && !writes,
              "LBA28 transfer end cannot overflow");
#undef IO
    }
    reset(); completion_response=0x41;
    CHECK(ata_write_sectors(&ctx,1,1,buf)==ATA_ERR_DEVICE && command==0x30,
          "WRITE completion error stops BEFORE FLUSH");
    reset(); flush_response=0x41;
    CHECK(ata_write_sectors(&ctx,1,1,buf)==ATA_ERR_DEVICE, "FLUSH error propagated");
    reset();
    CHECK(ata_read_sectors(0,0,1,buf)==ATA_ERR_ARG &&
          ata_write_sectors(&ctx,0,0,buf)==ATA_ERR_ARG &&
          ata_read_sectors(&ctx,0,257,buf)==ATA_ERR_ARG && !writes,
          "invalid arguments issue no I/O");
    return TEST_SUMMARY("test_ata");
}
