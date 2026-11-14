/* Independent P6 screen reader for GM3 live cursor transitions.
 * Hourglass rows are the frozen clean-room canon described in cursors.h and
 * PRD Appendix A, transcribed here rather than included from product data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static unsigned char images[3][640*480*3];
static int load(const char *path,unsigned char *pixels)
{
 FILE *f=fopen(path,"rb"); char magic[3]; int w,h,max,c;
 if(!f) return 1;
 if(fscanf(f,"%2s %d %d %d",magic,&w,&h,&max)!=4 || strcmp(magic,"P6") ||
    w!=640 || h!=480 || max!=255 || (c=fgetc(f))==EOF ||
    (c!='\n' && c!=' ' && c!='\r') || fread(pixels,1,sizeof images[0],f)!=sizeof images[0]) { fclose(f); return 1; }
 fclose(f); return 0;
}
static uint32_t pixel(unsigned image,unsigned x,unsigned y)
{
 unsigned i=(y*640+x)*3;
 return (uint32_t)images[image][i]<<16 | (uint32_t)images[image][i+1]<<8 | images[image][i+2];
}
int main(int argc,char **argv)
{
 static const uint16_t glass[16]={0xffff,0xffff,0x7ffe,0x3ffc,0x1ff8,0x0ff0,0x07e0,0x0180,
                                 0x0180,0x07e0,0x0ff0,0x1ff8,0x3ffc,0x7ffe,0xffff,0xffff};
 if(argc!=4 || load(argv[1],images[0]) || load(argv[3],images[2]) ||
    (strcmp(argv[2],"-") && load(argv[2],images[1]))) return 2;
 unsigned restored_bad=0,busy_bad=0,arrow_ink=0;
 for(unsigned y=330;y<375;y++) for(unsigned x=500;x<550;x++) {
  restored_bad+=pixel(0,x,y)!=pixel(2,x,y);
  arrow_ink+=pixel(0,x,y)==0;
  if(strcmp(argv[2],"-")) {
   uint32_t want=0x8ddcdc;
   if(x>=512 && x<528 && y>=342 && y<358 && (glass[y-342]&(0x8000u>>(x-512)))) want=0;
   busy_bad+=pixel(1,x,y)!=want;
  }
 }
 printf("live cursor: hourglass mismatches=%u, restored mismatches=%u, idle ink=%u\n",busy_bad,restored_bad,arrow_ink);
 return busy_bad || restored_bad || !arrow_ink ? 1:0;
}
