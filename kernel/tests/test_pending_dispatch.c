/* Independent DOS service vectors for migrated mutations. IBM DOS3.30 TR
 * FCB pp.6-67/75/76 and7-13..16; attributes p.5-11; MKDIR reserved devices
 * User's Guide p17. Literals and invariant canaries, no product-derived oracle. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "int21.h"
#include "dos_structs.h"
#include "find_data.h"
#include "devices.h"
#include "test_assert.h"
TEST_HARNESS();
static uint8_t fcb[64], dta[256], attr_seen;
static char path[64];
static dir_entry_t sample;
static unsigned mkdir_calls;
static uint8_t abs_buffer[512];
static unsigned abs_reads;
static int read_c(uint32_t lba,uint32_t count,void *out)
{ if(lba || count!=1)return -1;abs_reads++;memset(out,'C',512);return 0; }
static uint16_t open_file(const char *name,uint16_t dir,struct dir_entry *entry,uint32_t *slot)
{ (void)dir; if(strcmp(name,"TEST.BIN"))return 2;*entry=sample;*slot=0;return 0; }
static uint16_t create_file(const char *name,uint16_t dir,struct dir_entry *entry,uint32_t *slot)
{ return open_file(name,dir,entry,slot); }
static uint16_t read_file(const struct dir_entry *entry,uint32_t off,uint8_t *buf,uint32_t len,uint32_t *got)
{ (void)entry;*got=off<3?3-off:0;if(*got>len)*got=len;if(*got)memcpy(buf,"ABC"+off,*got);return 0; }
static uint16_t chmod_file(const char *name,uint16_t dir,int set,uint8_t *attr)
{ (void)name;(void)dir;if(set)attr_seen=*attr;return 0; }
static uint16_t enumerate(uint32_t index,uint16_t dir,struct dir_entry *entry,int *found)
{ (void)dir;*found=index==0;*entry=sample;return 0; }
static uint16_t mkdir_file(const char *name,uint16_t dir)
{ (void)name;(void)dir;mkdir_calls++;return 0; }
static const int21_file_backend_t backend={.open=open_file,.read_at=read_file,
 .create=create_file,.chmod=chmod_file,.dir_entry=enumerate,.mkdir=mkdir_file};
static int_frame_t call(unsigned ax,void *p)
{ int_frame_t r={0};r.eax=ax;r.edx=(uintptr_t)p;int21_dispatch(&r);return r; }
int main(void)
{
    int21_set_file_backend(&backend);
    memcpy(sample.filename,"TEST    ",8);memcpy(sample.extension,"BIN",3);
    sample.start_cluster=2;sample.file_size=3;sample.attribute=0x20;
    memcpy(fcb+1,"TEST    BIN",11);
    int_frame_t r=call(0x0f00,fcb);
    CHECK((r.eax&255)==0,"FCB OPEN literal TEST.BIN");
    memset(dta,0xa5,sizeof dta);(void)call(0x1a00,dta);
    r=call(0x1400,fcb);
    CHECK((r.eax&255)==3,"partial sequential record returns AL3");
    CHECK(!memcmp(dta,"ABC",3),"partial record preserves three input bytes");
    unsigned zero=1;for(unsigned i=3;i<128;i++)if(dta[i])zero=0;
    CHECK(zero && dta[128]==0xa5,"partial record zero-fills through128 and preserves canary");
    CHECK(fcb[32]==1 && fcb[12]==0 && fcb[13]==0,"sequential partial read advances one record");
    memset(fcb,0,sizeof fcb);fcb[0]=255;fcb[6]=6;
    memcpy(fcb+8,"TEST    BIN",11);r=call(0x1600,fcb);
    CHECK((r.eax&255)==0 && attr_seen==0x26,"extended FCB CREATE forwards H/S plus archive attributes");
    /* FIND labels are one eleven-character field, never an8.3 split. */
    memcpy(sample.filename,"ELEVEN C",8);memcpy(sample.extension,"HAR",3);
    sample.attribute=8;(void)call(0x1a00,dta);strcpy(path,"*.*");
    memset(&r,0,sizeof r);
    r.eax=0x4e00;r.ecx=8;r.edx=(uintptr_t)path;r.eflags=0;int21_dispatch(&r);
    CHECK(!(r.eflags&1) && !strcmp(((find_data_t *)dta)->fname,"ELEVEN CHAR"),
          "FIND returns literal full eleven-byte volume label");
    devices_init();int21_set_devices(devices_head(),NULL);
    strcpy(path,"NUL");r=call(0x3900,path);
    CHECK((r.eflags&1) && (r.eax&65535)==5 && mkdir_calls==0,
          "MKDIR refuses reserved device before calling filesystem");
    int21_absdisk_backend_t absolute={.read=read_c,.total_sectors=1};
    int21_set_absdisk_drive(2,&absolute);
    memset(&r,0,sizeof r);r.eax=2;r.ecx=1;r.ebx=(uintptr_t)abs_buffer;
    int25_dispatch(&r);
    CHECK(!(r.eflags&1) && abs_reads==1 && abs_buffer[0]=='C' && abs_buffer[511]=='C',
          "INT25 mounted C reads C backend and both ends of sector");
    return TEST_SUMMARY("pending-dispatch");
}
