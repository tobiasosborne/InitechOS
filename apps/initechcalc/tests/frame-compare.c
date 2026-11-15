/* The surviving still beside a booted screen, for the operator's eye: the
 * design's photograph comparison is reported guidance, never a pass/fail
 * (CRT blur, perspective, compression and the one-bar ruling prevent exact
 * photographic colour). Writes a 1280 x 480 binary PPM: on the left the
 * still's visible raster (geometry.tsv SCREEN) resampled to 640 x 480 by area
 * averaging, on the right the screen. Nothing is graded here.
 *
 *   frame-compare still.ppm x0 y0 x1 y1 screen.ppm out.ppm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char *load(const char *path,unsigned *w,unsigned *h)
{
    FILE *f=fopen(path,"rb"); char magic[3]; unsigned max; unsigned char *p;
    if(!f) return 0;
    if(fscanf(f,"%2s %u %u %u",magic,w,h,&max)!=4||strcmp(magic,"P6")||max!=255||fgetc(f)!='\n') { fclose(f); return 0; }
    p=malloc((size_t)*w**h*3);
    if(!p||fread(p,1,(size_t)*w**h*3,f)!=(size_t)*w**h*3) { free(p); fclose(f); return 0; }
    fclose(f); return p;
}
int main(int argc,char **argv)
{
    unsigned sw,sh,cw,ch; unsigned char *still,*shot; static unsigned char out[480][1280][3];
    if(argc!=8) { fputs("usage: frame-compare still.ppm x0 y0 x1 y1 screen.ppm out.ppm\n",stderr); return 2; }
    int x0=atoi(argv[2]),y0=atoi(argv[3]),x1=atoi(argv[4]),y1=atoi(argv[5]);
    if(!(still=load(argv[1],&sw,&sh))||!(shot=load(argv[6],&cw,&ch))||cw!=640||ch!=480
       ||x0<0||y0<0||x1>(int)sw||y1>(int)sh||x1<=x0||y1<=y0) { fputs("frame-compare: bad input\n",stderr); return 2; }
    for(int y=0;y<480;y++) for(int x=0;x<640;x++) {
        int ax=x0+(x1-x0)*x/640, bx=x0+(x1-x0)*(x+1)/640, ay=y0+(y1-y0)*y/480, by=y0+(y1-y0)*(y+1)/480;
        if(bx<=ax) bx=ax+1;
        if(by<=ay) by=ay+1;
        for(int c=0;c<3;c++) {
            unsigned long s=0,n=0;
            for(int v=ay;v<by;v++) for(int u=ax;u<bx;u++) { s+=still[((size_t)v*sw+u)*3+c]; n++; }
            out[y][x][c]=(unsigned char)(s/n);
            out[y][640+x][c]=shot[((size_t)y*640+x)*3+c];
        }
    }
    FILE *f=fopen(argv[7],"wb");
    if(!f||fprintf(f,"P6\n1280 480\n255\n")<0||fwrite(out,1,sizeof out,f)!=sizeof out||fclose(f)) { fputs("frame-compare: cannot write\n",stderr); return 2; }
    printf("frame-compare: the still beside the screen in %s (guidance, not graded)\n",argv[7]);
    return 0;
}
