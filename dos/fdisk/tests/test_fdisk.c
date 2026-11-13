/* File-backed adapter for the actual shipped editor core. Independent MBR
 * bytes come from mpartition / sfdisk in fdisk.sh, not this C program. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdisk.h"
int main(int argc,char **argv)
{
    if(argc<3)return 2;
    FILE *f=fopen(argv[2],"r+b");if(!f)return 2;
    uint8_t b[512];if(fread(b,1,512,f)!=512)return 2;
    if(fseek(f,0,SEEK_END)!=0)return 2;
    long size=ftell(f);if(size<512)return 2;
    int rc=-1;
    if(!strcmp(argv[1],"create"))rc=fdisk_create(b,(uint32_t)(size/512),argc>3?(unsigned)atoi(argv[3]):0,1);
    if(!strcmp(argv[1],"active"))rc=fdisk_active(b,2);
    if(!strcmp(argv[1],"delete-N"))rc=fdisk_delete(b,0);
    if(!strcmp(argv[1],"delete-Y"))rc=fdisk_delete(b,1);
    if(rc<0){fprintf(stderr,"RED fdisk operation %s\n",argv[1]);return 1;}
    rewind(f);if(fwrite(b,1,512,f)!=512 || fclose(f))return 2;
    return 0;
}
