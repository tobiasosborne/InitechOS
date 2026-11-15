/* Independent fdisk_run invariants: the loaded drive is untouched, and each
 * displayed DOS partition uses its supplied drive-letter mapping (MS Guide
 * pp287-295). No expected value is computed by fdisk's table renderer. */
#include <stdio.h>
#include <string.h>
#include "fdisk.h"
#include "test_assert.h"
TEST_HARNESS();
static unsigned reads,writes,at,restarts,empty;static char text[8192];static unsigned used;
static const char *keys="4\r\033\033";
static int read_disk(unsigned d,uint8_t b[512])
{
    (void)d;reads++;memset(b,0,512);b[510]=0x55;b[511]=0xaa;
    if(!empty){b[446+4]=1;b[462+4]=1;}return 0;
}
static int write_disk(unsigned d,const uint8_t b[512]){(void)d;(void)b;writes++;return 0;}
static uint32_t cap(unsigned d){return d==0x80?65536:0;}
static int get(void){return keys[at]?keys[at++]:27;}
static void put(const char *s){size_t n=strlen(s);if(used+n>=sizeof text)return;memcpy(text+used,s,n+1);used+=(unsigned)n;}
static char letter(unsigned d,unsigned p){(void)d;return p==1?'D':'E';}
static void restart(void){restarts++;}
static void reset(const char *input,unsigned blank)
{ keys=input;empty=blank;reads=writes=at=restarts=used=0;text[0]=0; }
static int restart_prompt(void)
{
    /* Genuine DOS 3.30 fixed-disk capture: compat33-restart/RESTART.TXT.
     * The operator's ATA-only ruling removes its floppy insertion line. */
    return strstr(text,"System will now restart\r\n\r\nPress any key when ready . . .\r\n")
        && !strstr(text,"diskette") && !strstr(text,"drive A:");
}
int main(void)
{
    fdisk_io io={read_disk,write_disk,cap,get,put,0x80,0,letter,0};
    int rc=fdisk_run(&io);
    CHECK(rc==1 && !reads && !writes,"loaded volume was untouched");
    if(g_fails)return TEST_SUMMARY("fdisk-session");
    io.loaded_drive=0x81;at=used=0;
    rc=fdisk_run(&io);
    CHECK(!rc && !writes && strstr(text,"D:") && strstr(text,"E:"),
          "partition letters follow the supplied independent mapping");
    if(g_fails)return TEST_SUMMARY("fdisk-session");
    io.restart=restart;
    reset("2\r2\r\033\033x",0);
    rc=fdisk_run(&io);
    CHECK(!rc && writes==1 && restarts==1,"changed partition restarts on exit");
    CHECK(restart_prompt(),"changed-partition restart retains captured lines without diskette instruction");
    reset("1\r1\ry\rx",1);
    rc=fdisk_run(&io);
    CHECK(!rc && writes==1 && restarts==1,"maximum primary partition restarts immediately");
    CHECK(restart_prompt(),"new-primary restart retains captured lines without diskette instruction");
    if(!g_fails)puts("GREEN FDISK session: loaded drive, independent letters and both fixed-disk restart paths");
    return TEST_SUMMARY("fdisk-session");
}
