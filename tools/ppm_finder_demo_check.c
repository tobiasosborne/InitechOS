/* tdnl.79: disabled ink comes from the independent Mac OS 8.1 capture.
 * No artifact palette, glyph strike or menu resource is included. */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv)
{
    png_image im={0};
    unsigned char gold[640*480*3], ppm[640*480*3];
    unsigned char gray[3]={255,255,255};
    char magic[3];int w,h,max,ink=0,black=0;
    FILE *f;
    if(argc!=3) return 2;
    im.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&im,argv[2])) return 2;
    im.format=PNG_FORMAT_RGB;
    if(im.width!=640 || im.height!=480 || !png_image_finish_read(&im,0,gold,0,0)) return 2;
    png_image_free(&im);
    /* Print's disabled ink, x53..60 y55..66 in the native File menu. */
    for(int y=55;y<67;y++) for(int x=53;x<61;x++) {
        unsigned char *p=gold+3*(y*640+x);
        if(p[0] && p[0]<gray[0] && p[0]==p[1] && p[1]==p[2]) memcpy(gray,p,3);
    }
    if(gray[0]==255) {fprintf(stderr,"FAIL disabled capture has no ink\n");return 2;}
    f=fopen(argv[1],"rb");
    if(!f || fscanf(f,"%2s%d%d%d",magic,&w,&h,&max)!=4 || strcmp(magic,"P6") || w!=640 || h!=480 || max!=255) return 2;
    (void)fgetc(f);
    if(fread(ppm,1,sizeof ppm,f)!=sizeof ppm) return 2;
    fclose(f);
    /* HELLO File > New: panel at x0,y39; first cap row43. */
    for(int y=43;y<55;y++) for(int x=19;x<75;x++) {
        unsigned char *p=ppm+3*(y*640+x);
        if(!memcmp(p,gray,3)) ink++;
        if(!p[0] && !p[1] && !p[2]) black++;
    }
    if(ink<20 || black) {fprintf(stderr,"FAIL DEMO_GRAY gray=%d black=%d\n",ink,black);return 1;}
    printf(">>> ppm-finder-demo: green (native disabled ink, %d pixels; zero black)\n",ink);
    return 0;
}
