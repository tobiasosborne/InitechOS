/* Independent fdisk_run invariants: the loaded drive is untouched, and each
 * displayed DOS partition uses its supplied drive-letter mapping (MS Guide
 * pp287-295). No expected value is computed by fdisk's table renderer. */
#include <stdio.h>
#include <string.h>
#include "fdisk.h"
#include "test_assert.h"
TEST_HARNESS();
static unsigned reads,writes,at;static char text[8192];static unsigned used;
static const char keys[]="4\r\033\033";
static int read_disk(unsigned d,uint8_t b[512])
{
    (void)d;reads++;memset(b,0,512);b[510]=0x55;b[511]=0xaa;
    /* [63,1943) and [2016,2943) on the fixed 16-head, 63-sector
     * geometry. The stale end-CHS cylinder 45 is deliberately different. */
    const uint8_t first[16]={0x80,0,0,0,1,0,0,45,63,0,0,0,0x58,7,0,0};
    const uint8_t second[16]={0,0,0,0,1,0,0,0,0xe0,7,0,0,0x9f,3,0,0};
    memcpy(b+446,first,16);memcpy(b+462,second,16);return 0;
}
static int write_disk(unsigned d,const uint8_t b[512]){(void)d;(void)b;writes++;return 0;}
static uint32_t cap(unsigned d){return d==0x80?2943:0;}
static int get(void){return at<sizeof keys-1?keys[at++]:27;}
static void put(const char *s){size_t n=strlen(s);if(used+n>=sizeof text)return;memcpy(text+used,s,n+1);used+=(unsigned)n;}
static char letter(unsigned d,unsigned p){(void)d;return p==1?'D':'E';}
int main(void)
{
    fdisk_io io={read_disk,write_disk,cap,get,put,0x80,0,letter,0};
    int rc=fdisk_run(&io);
    CHECK(rc==1 && !reads && !writes,"loaded volume was untouched");
    CHECK(strstr(text,"Cannot use FDISK on the system disk\r\n"),
          "loaded disk refusal explains why FDISK exits");
    if(g_fails)return TEST_SUMMARY("fdisk-session");
    io.loaded_drive=0x81;at=used=0;
    rc=fdisk_run(&io);
    CHECK(!rc && !writes && strstr(text,"D:") && strstr(text,"E:"),
          "partition letters follow the supplied independent mapping");
    const char *row=strstr(text,"D:");
    unsigned first,last,count;
    CHECK(row && sscanf(row+24,"%u %u %u",&first,&last,&count)==3 &&
          first==0 && last==1 && count==2,"partition cylinders follow its LBA extent");
    CHECK(strstr(text,"Total disk space is 3 cylinders."),
          "physical cylinder count includes the final partial cylinder");
    if(g_fails)return TEST_SUMMARY("fdisk-session");
    puts("GREEN FDISK session: loaded drive untouched, independent D/E partition mapping");return TEST_SUMMARY("fdisk-session");
}
