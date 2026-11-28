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
    b[446+4]=1;b[462+4]=1;return 0;
}
static int write_disk(unsigned d,const uint8_t b[512]){(void)d;(void)b;writes++;return 0;}
static uint32_t cap(unsigned d){return d==0x80?65536:0;}
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
    if(g_fails)return TEST_SUMMARY("fdisk-session");
    puts("GREEN FDISK session: loaded drive untouched, independent D/E partition mapping");return TEST_SUMMARY("fdisk-session");
}
