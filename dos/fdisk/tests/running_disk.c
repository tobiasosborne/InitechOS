/* The C: activation plan and published INT 13h AH=60h ABI define these
 * literal map payloads. The fixture substitutes only the interrupt transport.
 * The real fdisk_entry drives fdisk_run. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diskutil.h"
#include "fdisk_gates.h"
#include "test_assert.h"

TEST_HARNESS();
int fdisk_entry(const char *,unsigned);

static const uint8_t partitioned[512] = {
    [450] = 1, [454] = 63, [459] = 64, [510] = 0x55, [511] = 0xaa
};
static const du_map a_running[4] = {
    [0] = {0x81, 0, 0, 2880, 3}, [2] = {0x80, 1, 63, 16384, 0}
};
static const du_map c_running[4] = {
    [0] = {0x81, 0, 0, 2880, 2}, [2] = {0x80, 1, 63, 16384, 1}
};
static const du_map conflicting[4] = {
    [0] = {0x81, 0, 0, 2880, 3}, [2] = {0x80, 1, 63, 16384, 1}
};
static const du_map invalid[4] = {
    [2] = {0x180, 1, 63, 16384, 1}
};

static const du_map *maps;
static unsigned mapped, selected;
static unsigned queries[2], reads[2], writes, restarts;
static uint8_t mbr[512];
static const char *keys;
static char output[4096];
static unsigned output_used;

static void unexpected(const char *gate, unsigned ax)
{
    fprintf(stderr,"ERROR running-disk fixture: unexpected %s AX=%04x\n",gate,ax);
    exit(2);
}

int fdisk_int13(unsigned ax, uintptr_t bx, unsigned *cx, unsigned *dx)
{
    if (ax >= 0x6000 && ax <= 0x6003) {
        unsigned l = ax & 255u;
        *(du_map *)bx = maps[l];
        return mapped & (1u << l) ? 0 : -1;
    }
    unsigned disk = (*dx & 255u) - 0x80u;
    if (disk > 1)
        unexpected("INT 13h",ax);
    switch (ax) {
    case 0x1500:
        queries[disk]++;
        *cx = 0;
        *dx = disk ? ((mapped & 1) ? 2880 : 0) : 16447;
        return *dx ? 3 : 0;
    case 0x0201:
        reads[disk]++;
        if (disk == 0 && *dx == 0x80 && *cx == 1)
            memcpy((void *)bx,mbr,sizeof mbr);
        else if (disk == 0 && *dx == 0x180 && *cx == 1)
            memset((void *)bx,0,512); /* Never a FAT16 BPB. */
        else
            unexpected("INT 13h read",ax);
        return 0;
    case 0x0301:
        if (disk != 0 || *dx != 0x80 || *cx != 1)
            unexpected("INT 13h write",ax);
        writes++;
        memcpy(mbr,(const void *)bx,sizeof mbr);
        return 0;
    default:
        unexpected("INT 13h",ax);
        return -1;
    }
}

unsigned fdisk_int21(unsigned ax, unsigned bx, unsigned cx, uintptr_t dx)
{
    (void)bx;
    switch (ax) {
    case 0x0700: return *keys ? (unsigned char)*keys++ : 27;
    case 0x1900: return selected;
    case 0x4000:
        if(cx<sizeof output-output_used){
            memcpy(output+output_used,(const void *)dx,cx);
            output_used+=cx;
            output[output_used]=0;
        }
        return 0;
    default:
        unexpected("INT 21h",ax);
        return 0;
    }
}

void fdisk_restart(void)
{
    restarts++;
}

static int run(const du_map map[4], unsigned present, unsigned current, const char *input)
{
    maps = map;
    mapped = present;
    selected = current;
    keys = input;
    memset(queries,0,sizeof queries);
    memset(reads,0,sizeof reads);
    writes = restarts = output_used = 0;
    output[0]=0;
    memcpy(mbr,partitioned,sizeof mbr);
    return fdisk_entry("",0);
}

static int untouched(void)
{
    return !queries[0] && !queries[1] && !reads[0] && !reads[1] &&
           !writes && !restarts && !memcmp(mbr,partitioned,sizeof mbr);
}

int main(void)
{
    int rc = run(a_running,0x5,2,"5\r");
    /* One slave capacity query is normal to display option 5. */
    CHECK(rc == 1 && queries[0] == 1 && queries[1] == 1 && reads[0] == 1 &&
          !reads[1] && !writes && !restarts && !memcmp(mbr,partitioned,sizeof mbr),
          "A-running/81h: C selected can view disk 1 but cannot enter disk 2");
    if (g_fails) return TEST_SUMMARY("fdisk-running-disk");

    rc = run(c_running,0x5,0,"3\r1\ry\r\033\033");
    CHECK(rc == 1 && untouched(),
          "C-running/80h: A selected must refuse before accessing disk 1");
    if (g_fails) return TEST_SUMMARY("fdisk-running-disk");

    rc = run(c_running,0x4,2,"3\r1\ry\r\033\033");
    CHECK(rc == 1 && untouched(),"C-only refuses before accessing disk 1");
    rc = run(c_running,0x1,0,"\033");
    CHECK(rc == 1 && untouched(),"a failed map cannot supply the running flag");
    rc = run(c_running,0x0,0,"\033");
    CHECK(rc == 1 && untouched(),"all maps failed: refuse before disk access");
    rc = run(conflicting,0x5,0,"\033");
    CHECK(rc == 1 && untouched(),"conflicting running disks: refuse before disk access");
    rc = run(invalid,0x4,2,"\033");
    CHECK(rc == 1 && untouched(),"invalid physical disk: refuse before disk access");
    output_used=0;
    rc=fdisk_entry(" /Z",3);
    CHECK(rc==1 && !strcmp(output,"Invalid parameter\r\n") && untouched(),
          "FDISK /Z is explicitly refused before accessing the disk");
    return TEST_SUMMARY("fdisk-running-disk");
}
