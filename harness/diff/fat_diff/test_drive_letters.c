/* Independent literals from the PM DEC-18 ruling, not generated answers.
 * MBR bytes: IBM TR pp9-6..9; flat ABI: DEC-04a/17. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block_device.h"
#include "drive_letters.h"
static unsigned char disks[2][512];
static du_map out;
static void need(int b,const char *s){if(!b){fprintf(stderr,"RED drive-letters %s\n",s);exit(1);}}
static void w32(unsigned char *p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(i*8));}
static int rd(void *c,uint32_t l,uint32_t n,void *p){if(l||n!=1)return -1;memcpy(p,disks[(uintptr_t)c],512);return 0;}
static void part(unsigned i,unsigned n,unsigned type,unsigned base,unsigned size){unsigned char *p=disks[i]+446+(n-1)*16;p[4]=(unsigned char)type;w32(p+8,base);w32(p+12,size);disks[i][510]=0x55;disks[i][511]=0xaa;}
int main(void){
 need(DL_function==96 && DL_a_disk==129 && DL_c_disk==128 && DL_d_disk==128 && DL_c_partition==1 && DL_d_partition==2 && DL_map_bytes==20,"SPEC literals");
 for(unsigned i=0;i<2;i++){blockdev_t d={(void *)(uintptr_t)i,rd,0};block_device_bind(i,&d,10080);}
 disks[1][0]=0xeb;disks[1][2]=0x90;disks[1][12]=2;disks[1][13]=1;disks[1][16]=2;
 disks[1][19]=0x40;disks[1][20]=0x0b;disks[1][510]=0x55;disks[1][511]=0xaa;
 part(0,1,1,63,2880);part(0,2,6,4032,4000);
 block_device_mounted(129,0,2880,0);
 need(!block_device_map(0,&out)&&out.disk==129&&out.partition==0&&out.base==0&&out.sectors==2880&&out.flags==3,"A physical identity");
 need(block_device_map(1,&out)!=0,"B unassigned");
 need(!block_device_map(2,&out)&&out.disk==128&&out.partition==1&&out.base==63&&out.sectors==2880&&!out.flags,"C primary 1");
 need(!block_device_map(3,&out)&&out.disk==128&&out.partition==2&&out.base==4032&&out.sectors==4000&&!out.flags,"D primary 2");
 block_device_mounted(128,63,2880,0);
 need(!block_device_map(2,&out)&&out.flags==1,"running pair");
 need(!block_device_map(3,&out)&&out.flags==0,"running pair");
 memset(disks[1],0,512);part(1,1,1,63,2880);block_device_mounted(129,63,2880,0);
 need(!block_device_map(0,&out)&&out.disk==129&&out.partition==1&&out.base==63&&out.sectors==2880&&out.flags==3,"A partition 1");
 int_frame_t f={0};f.eax=0x6000;f.ebx=(uint32_t)(uintptr_t)&out;f.eflags=0x203;int13_dispatch(&f);
 need(!(f.eflags&1)&&!(f.eax&0xff00)&&out.disk==129&&out.base==63,"AH60 flat map ABI");
 f.eax=0x6001;f.eflags=0x202;int13_dispatch(&f);need((f.eflags&1)&&(f.eax&0xff00)==0x100,"AH60 invalid ABI");
 part(0,2,5,4032,4000);need(block_device_map(3,&out)!=0,"extended invalid");
 part(0,2,6,9000,4000);need(block_device_map(3,&out)!=0,"bounds invalid");
 part(0,2,6,4032,4000);disks[0][510]=0;need(block_device_map(2,&out)!=0,"signature invalid");
 puts("GREEN test-drive-letters: A/B/C/D positions, partition bounds/types, running pair and AH60 ABI");return 0;
}
