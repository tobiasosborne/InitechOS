/* Independent P6 and captured NFNT widths. Empty-menu policy: IM IV-55.
 * Indexed Platinum title face is the retained idx63 gray, already pinned
 * by test_menu; compare every below-bar pixel at the same pointer position. */
#define main cursor_reader_main
#include "copy_pixels.c"
#undef main
#include "chicago12_nfnt_golden.h"
static const char *titles[]={"File","Edit","Image","Layer","Select","View","Window","Help"};
static int advance(const char *s)
{
 int w=0;
 for(;*s;s++) w+=NFNT5478_AW[(unsigned char)*s-NFNT5478_FIRST];
 return w;
}
int main(int argc,char **argv)
{
 int x=33;
 if(argc==2 && !strcmp(argv[1],"positions")) {
  for(int i=0;i<8;i++) {
   if(i) printf("%d %s %d\n",128+i,titles[i],x+12);
   x+=advance(titles[i])+18;
  }
  return 0;
 }
 if(argc!=5 || load(argv[2],images[0]) || load(argv[3],images[1]) || load(argv[4],images[2])) return 2;
 int found=0;
 for(int i=0;i<8;i++) {
  if(i && !strcmp(argv[1],titles[i])) {found=1;break;}
  x+=advance(titles[i])+18;
 }
 int file_panel=!strcmp(argv[1],"File");
 if(!found && !file_panel) return 2;
 unsigned held_bad=0,released_bad=0;
 for(unsigned y=20;y<480;y++) for(unsigned col=0;col<640;col++) {
  held_bad+=pixel(0,col,y)!=pixel(1,col,y);
  released_bad+=pixel(0,col,y)!=pixel(2,col,y);
 }
 if(file_panel) {
  printf("File panel changed %u below-bar pixels\n",held_bad);
  return held_bad>1000?0:1;
 }
 for(unsigned y=0;y<20;y++) for(unsigned col=0;col<500;col++)
  released_bad+=pixel(0,col,y)!=pixel(2,col,y);
 int highlight=pixel(1,(unsigned)(x+2),3)==0x3f3f3f &&
               pixel(0,(unsigned)(x+2),3)!=pixel(1,(unsigned)(x+2),3);
 printf("empty %s: highlighted=%d below-bar changes=%u release changes=%u\n",argv[1],highlight,held_bad,released_bad);
 return !highlight || held_bad || released_bad ? 1:0;
}
