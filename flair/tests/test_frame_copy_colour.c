/* GM3: live FILE COPY's colours are read from the pinned still's independent
 * colour sheet, never from the painter or its palette source. Generic dialogs
 * retain the captured Platinum E7 face. */
#define main old_dialog_main
#include "test_dialog.c"
#undef main
#include "flair_dac.h"
#include "palette.h"
static uint32_t direct[640*480];
static uint8_t indexed[640*480];
static void reference(const char *path,const char *sample,unsigned out[3])
{
 FILE *f=fopen(path,"r");char line[1024];if(!f){perror(path);exit(2);}
 while(fgets(line,sizeof line,f)){
  char *fields[12],*p=line;unsigned n=0;
  do{fields[n++]=p;p=strchr(p,'\t');if(p)*p++=0;}while(p&&n<12);
  if(n>=9&&!strcmp(fields[0],sample)){
   double r,g,b;if(sscanf(fields[8],"%lf %lf %lf",&r,&g,&b)!=3)exit(2);
   out[0]=(unsigned)(r*255+0.5);out[1]=(unsigned)(g*255+0.5);out[2]=(unsigned)(b*255+0.5);
   fclose(f);return;
  }
 }
 fclose(f);fprintf(stderr,"no frame colour %s\n",sample);exit(2);
}
static void grade(uint32_t rgb,const unsigned want[3],unsigned tolerance,const char *what)
{
 for(unsigned k=0;k<3;k++){
  unsigned got=(rgb>>(16-k*8))&255;int d=(int)got-(int)want[k];if(d<0)d=-d;
  CHECK((unsigned)d<=tolerance,what);
 }
}
int main(int argc,char **argv)
{
 if(argc!=2)return 2;
 unsigned white[3],blue[3];reference(argv[1],"filecopy_content",white);reference(argv[1],"progress_fill",blue);
 for(unsigned depth=8;depth<=32;depth+=24){
  rgn_store_init(&g_mdl_sruc);rgn_store_init(&g_mdl_scont);rgn_store_init(&g_mdl_supd);
  static ControlRecord controls[2];
  DialogPtr dp=FileCopyCancelableDialog(&g_mdl_dr,g_mdl_items,controls,&g_mdl_sruc.r,&g_mdl_scont.r,&g_mdl_supd.r);
  bitmap_t bm={.base=depth==8?(void*)indexed:(void*)direct,.pitch=640*(depth/8),.width=640,.height=480,.bpp=depth,.bytes_per_pixel=depth/8};
  dp->window.port.portBits.bm=bm;dp->window.port.visRgn=0;dp->window.port.clipRgn=0;
  DrawDialog(dp);
  if(depth==32){grade(direct[205*640+300],white,0,"live body matches photographed white");grade(direct[240*640+170],blue,1,"live fill matches photographed blue ratios");}
  else{
   unsigned body=indexed[205*640+300],fill=indexed[240*640+170];
   grade(flair_palette_rgb((uint8_t)body),white,0,"indexed body matches photographed white");
   grade(flair_palette_rgb((uint8_t)fill),blue,1,"indexed fill matches photographed blue");
   uint32_t dac=((uint32_t)flair_dac[fill][0]<<16)|((uint32_t)flair_dac[fill][1]<<8)|flair_dac[fill][2];
   grade(dac,blue,1,"actual DAC source matches independent still blue");
  }
  /* Reusing the same record must reset the live-only style. */
  dp=FileCopyDialog(&g_mdl_dr,g_mdl_items,controls,&g_mdl_sruc.r,&g_mdl_scont.r,&g_mdl_supd.r);
  dp->window.port.portBits.bm=bm;dp->window.port.visRgn=0;dp->window.port.clipRgn=0;
  DrawDialog(dp);
  CHECK(depth==8?indexed[244*640+300]==231:direct[244*640+300]==0xe7e7e7,"generic dialog retains captured Platinum face");
 }
 return TEST_SUMMARY("test-frame-copy-colour");
}
