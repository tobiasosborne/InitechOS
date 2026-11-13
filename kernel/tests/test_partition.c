/* Factory mount/error oracle. Ref: IBM DOS 3.30 TR pp2-31,9-6..9;
 * ADR-0003 DEC-07a (require hidden/start equality, mount-layer origin).
 * Expected sectors are from mpartition/mformat, never the implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"
#include "blockdev_file.h"
static unsigned char disk[2943*512], saved[sizeof disk];
static unsigned char sector[512];
static int read_disk(void *ctx, uint32_t l, uint32_t n, void *p)
{
    (void)ctx;
    if (l >= 2943 || n > 2943-l) return -1;
    memcpy(p, disk+l*512, n*512); return 0;
}
static void w32(unsigned char *p, unsigned v)
{ for (unsigned i=0;i<4;i++) p[i]=(unsigned char)(v>>(i*8)); }
static void require(int ok, const char *s, int rc)
{ if(!ok){fprintf(stderr,"RED partition %s rc=%d\n",s,rc);exit(1);} }
static void check(int want, const char *name)
{
    fat12_volume_t v;
    blockdev_t d={NULL,read_disk,NULL};
    int rc=fat12_mount(&v,&d,sector);
    require(rc==want,name,rc);
    if(!rc) {
        require(v.base_lba==v.bpb.hidden_sectors,"origin equals hidden sectors",rc);
        require(fat12_read_sectors(&v,v.total_sectors,1,sector)==FAT12_ERR_PARTITION,
                "read beyond volume",rc);
        require(fat12_read_sectors(&v,1,0xffffffffu,sector)==FAT12_ERR_PARTITION,
                "sector count overflow",rc);
    }
}
static void spec_check(const char *path)
{
    char b[4096]={0};FILE *f=fopen(path,"rb");if(!f)exit(2);
    size_t n=fread(b,1,sizeof b-1,f);fclose(f);require(n<sizeof b-1,"SPEC length",0);
    const char *tokens[]={"\"table_offset\": 446,","\"entry_count\": 4,",
        "\"entry_size\": 16,","\"type_offset\": 4,","\"start_offset\": 8,",
        "\"length_offset\": 12,","\"signature_offset\": 510,",
        "\"signature_le\": 43605,","\"fat_types\": [1, 4, 6],",
        "\"translation_heads\": 16,","\"translation_sectors_per_track\": 63,",
        "\"first_partition_lba\": 63,",
        "\"selection\": \"active FAT else first FAT\",",
        "\"partition_error\": -20,",
        "\"absolute_sector\": \"device LBA = volume base_lba + logical sector\",",
        "\"hidden_sectors\": \"equal base_lba; mismatch fails loud (provisional DEC-07a)\",",
        "\"offset_layer\": \"volume mount; raw blockdev stays device-relative\",",
        "\"raw_detection\": \"E9 or EB xx 90; media F0 or F8..FF; validated BPB\","};
    for(unsigned i=0;i<sizeof tokens/sizeof tokens[0];i++)
        require(strstr(b,tokens[i])!=NULL,"SPEC contract drift",0);
    puts("GREEN test-partition-spec: IBM table layout and DEC-07a origin contract");
}
int main(int argc,char **argv)
{
    if(argc<2)return 2;
    if(argc==3 && !strcmp(argv[1],"--spec")){spec_check(argv[2]);return 0;}
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    size_t n=fread(disk,1,sizeof disk,f);fclose(f);
    if(n<2880*512)return 2;
    if(argc==3) {
        f=fopen(argv[1],"r+b");if(!f)return 2;
        unsigned base=disk[0]==0xeb||disk[0]==0xe9?0:63;
        if(!strcmp(argv[2],"corrupt")) {
            unsigned char *b=disk+base*512;
            unsigned reserved=b[14]+256u*b[15], spf=b[22]+256u*b[23];
            unsigned root=reserved+b[16]*spf, c=0;
            for(unsigned i=0;i<224;i++) {
                unsigned char *e=b+root*512+i*32;
                if(!memcmp(e,"INPUT   TXT",11))c=e[26]+256u*e[27];
            }
            if(c<2)return 2;
            unsigned o=c+c/2;
            for(unsigned k=0;k<b[16];k++) {
                unsigned at=(base+reserved+k*spf)*512+o;
                unsigned v=disk[at]+256u*disk[at+1];
                v=c&1?(v&15)|16:(v&0xf000)|1;
                fseek(f,at,SEEK_SET);fputc(v&255,f);fputc(v>>8,f);
            }
        } else if(!strcmp(argv[2],"EXTENDED")) {
            fseek(f,450,SEEK_SET);fputc(5,f);
        } else if(!strcmp(argv[2],"SIGNATURE")) {
            fseek(f,510,SEEK_SET);fputc(0,f);
        } else if(!strcmp(argv[2],"HIDDEN")) {
            fseek(f,63*512+28,SEEK_SET);fputc(0,f);
        } else return 2;
        return fclose(f)?2:0;
    }
    memcpy(saved,disk,sizeof disk);
    check(0,"valid MBR");
    disk[450]=5;check(-20,"EXTENDED must reject");
    memcpy(disk,saved,sizeof disk);disk[510]=0;check(-2,"SIGNATURE must reject");
    memcpy(disk,saved,sizeof disk);disk[63*512+28]=0;check(-20,"HIDDEN mismatch must reject");
    memcpy(disk,saved,sizeof disk);disk[63*512+510]=0;check(-2,"VBR signature must reject");
    memcpy(disk,saved,sizeof disk);disk[446]=0x81;check(-20,"bad boot indicator");
    memcpy(disk,saved,sizeof disk);w32(disk+454,0);check(-20,"zero start");
    memcpy(disk,saved,sizeof disk);w32(disk+458,2879);check(-20,"BPB exceeds partition");
    memcpy(disk,saved,sizeof disk);w32(disk+454,0xfffffff0u);check(-20,"partition end overflow");
    memcpy(disk,saved,sizeof disk);disk[450]=0x0f;check(-20,"extended LBA must reject");
    memcpy(disk,saved,sizeof disk);disk[446]=0;check(0,"first inactive FAT");
    /* An active second FAT entry wins over an unreadable inactive first. */
    memcpy(disk+462,disk+446,16);disk[462]=0x80;w32(disk+454,1);check(0,"active FAT preference");
    memcpy(disk,saved,sizeof disk);disk[450]=4;check(0,"type 04");
    disk[450]=6;check(0,"type 06 accepted per lane compatibility contract");
    memcpy(disk,saved,sizeof disk);disk[446]=0x80;
    memcpy(disk+462,disk+446,16);check(-20,"multiple active partitions");
    memcpy(disk,saved,sizeof disk);disk[63*512+21]=0;check(-20,"invalid partition media");
    memcpy(disk,saved+63*512,512);w32(disk+28,0);
    check(0,"superfloppy content on ATA");
    disk[0]=0xe9;check(0,"DOS near jump");
    disk[0]=0;check(-20,"superfloppy requires jump");
    puts("GREEN partition validation: active/first FAT, hidden equality, lengths, signatures, extended rejection");
    return 0;
}
