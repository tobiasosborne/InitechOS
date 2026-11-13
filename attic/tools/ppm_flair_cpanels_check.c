/* Factory C / tdnl.19. Independent runtime Mac capture, no render header.
 * Compare complete OK button + default ring including its label to s8_alert.
 * Read-only corpus, exact RGB. Composed panel layouts have no native golden. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "menubar_clock_oracle.h"
int main(int argc,char **argv) {
 if(argc!=4 && argc!=5)return 2;
 int pattern=strcmp(argv[1],"pattern")==0;

 FILE *f=fopen(argv[pattern?2:1],"rb");int w,h,max;char magic[3];
 if(!f||fscanf(f,"%2s%d%d%d",magic,&w,&h,&max)!=4||strcmp(magic,"P6")||max!=255||w!=640||h!=480)return 2;
 (void)fgetc(f);unsigned char *pix=malloc((size_t)w*h*3);
 if(!pix)return 2;
 if(fread(pix,3,(size_t)w*h,f)!=(size_t)w*h)return 2;
 fclose(f);
 if(pattern) {
   unsigned char tile[8]={0};int index=atoi(argv[4]);f=fopen(argv[3],"rb");
   if(!f || index<0 || index>8 || (index && (fseek(f,2+8*(index-1),SEEK_SET) || fread(tile,1,8,f)!=8)))return 2;
   fclose(f);unsigned bad=0;
   for(unsigned y=440;y<448;y++)for(unsigned x=24;x<88;x++) {
      unsigned char want=(tile[y%8]&(128u>>(x%8)))?0:255;
      unsigned char *a=pix+(y*(unsigned)w+x)*3;
      bad+=index ? (a[0]!=want || a[1]!=want || a[2]!=want) : (a[0]!=141 || a[1]!=220 || a[2]!=220);
   }
   free(pix);if(bad){printf("FAIL CP_BOOT_PATTERN %u/512\n",bad);return 1;}
   puts(index?"CP_BOOT_PATTERN GREEN512 pixels vs independent PAT#_0.bin":"CP_BOOT_PATTERN GREEN512 pixels vs locked teal #8DDCDC");return 0;
 }
 png_image im={0};im.version=PNG_IMAGE_VERSION;
 if(!png_image_begin_read_from_file(&im,argv[2]))return 2;
 im.format=PNG_FORMAT_RGB;unsigned char *ref=malloc(PNG_IMAGE_SIZE(im));
 if(im.width!=640 || im.height!=480 || !ref || !png_image_finish_read(&im,0,ref,0,0))return 2;
 uint32_t expected[26][65];
 for(int y=0;y<26;y++)for(int x=0;x<65;x++) {
   const unsigned char *b=ref+((155+y)*im.width+432+x)*3;
   expected[y][x]=((uint32_t)b[0]<<16)|((uint32_t)b[1]<<8)|b[2];
   /* PRD6.4: Charcoal label -> Chicago, same already-graded control.
    * Replace only the flat face's label area, then independently paint every
    * expected Chicago pixel from NFNT5478. All1690 pixels remain graded. */
   if(x>=23 && x<43 && y>=7 && y<18)expected[y][x]=0xe7e7e7;
 }
 /* Chicago O advance8, K advance9: centre the17px cell in59px, NFNT height15.
  * Ref: independent NFNT5478 OW table, not the artifact's text_measure. */
 if(oracle_chicago_text(argv[3],"OK",&expected[0][0],65,26,24,5))return 2;
 unsigned bad=0,face_bad=0;
 /* NewDocumentWindow content at(126,112); local OK(309,220) 59x20.
  * Golden OK(435,158), ring expands by3. Exclusive 65x26. */
 for(int y=0;y<26;y++)for(int x=0;x<65;x++) {
   const unsigned char *a=pix+((329+y)*w+432+x)*3;
   uint32_t actual=((uint32_t)a[0]<<16)|((uint32_t)a[1]<<8)|a[2];
   if(actual!=expected[y][x]) {if(bad<12)printf("CP_DIFF %d,%d %06x != %06x\n",x,y,actual,expected[y][x]);bad++;}
 }
 if(argc==5 && !pattern) {
   /* Radio labels and cleared corners inherit the dialog's captured face;
    * no assertion about the unminted radio SHAPE. Ref: controls.md face E7. */
   const int points[2][2]={{170,164},{150,166}};
   const unsigned char *face=ref+(165*640+440)*3;
   for(unsigned i=0;i<2;i++) {
     const unsigned char *a=pix+(points[i][1]*640+points[i][0])*3;
     if(memcmp(a,face,3)){printf("FAIL CP_RADIO_FACE %d,%d\n",points[i][0],points[i][1]);face_bad++;}
   }
 }
 free(pix);free(ref);png_image_free(&im);
 if(bad){printf("FAIL CP_CHROME independent OK/ring mismatches=%u/1690\n",bad);return 1;}
 if(face_bad)return 1;
 if(argc==5)puts("CP_RADIO_FACE GREEN2 pixels vs captured dialog face");
 puts("CP_CHROME GREEN 1690 pixels vs s8_alert_modal.png OK/default-ring");return 0;
}
