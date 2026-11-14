/* Independent Mac OS 8.1 mint-008 icon capture reader. Expected RGB is
 * recovered only from captured PNGs, never from product art/render tables.
 * See factory/PIXEL-REFERENCES.md for origins, hashes and mask scope. */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { W=640,H=480,N=W*H*3 };
static unsigned char *load(const char *dir,const char *name)
{
    char path[2048];png_image im;memset(&im,0,sizeof im);im.version=PNG_IMAGE_VERSION;
    if(snprintf(path,sizeof path,"%s/%s",dir,name)>=(int)sizeof path ||
       !png_image_begin_read_from_file(&im,path))return NULL;
    im.format=PNG_FORMAT_RGB;
    if(im.width!=W || im.height!=H)return NULL;
    unsigned char *p=malloc(N);
    if(!p || !png_image_finish_read(&im,NULL,p,0,NULL)) {free(p);p=NULL;}
    png_image_free(&im);return p;
}
static int save(const char *path,const unsigned char *p)
{
    FILE *f=fopen(path,"wb");if(!f)return 1;
    fprintf(f,"P6\n640 480\n255\n");int ok=fwrite(p,1,N,f)==N;return fclose(f)||!ok;
}
int main(int argc,char **argv)
{
    if(argc!=4 && (argc!=5 || strcmp(argv[4],"selected-volume")))return 2;
    unsigned char *image=load(argv[1],"s8_icons_desktop_empty.png");
    unsigned char *wall=load(argv[1],"s8_icons_trash_full_sel.png");
    unsigned char *selected[2]={load(argv[1],"s8_icons_floppy_sel.png"),
        load(argv[1],"s8_icons_trash_empty_sel.png")};
    unsigned char *ref=calloc(N,1), *mask=calloc(N,1);
    if(!image || !wall || !selected[0] || !selected[1] || !ref || !mask) {fputs("FAIL icon reference load\n",stderr);return 2;}
    const struct {int source_y,dest_y,population;} icons[]={{156,28,934},{412,404,644}};
    for(unsigned k=0;k<2;k++) {
        unsigned count=0;
        for(int y=0;y<32;y++)for(int x=0;x<32;x++) {
            int sx=576+x,sy=icons[k].source_y+y;
            int by=148+((sy-148)%128+128)%128, bx=128+sx%128;
            const unsigned char *rgb=image+3*(sy*W+sx), *bg=wall+3*(by*W+bx);
            if(!memcmp(rgb,bg,3) && !memcmp(selected[k]+3*(sy*W+sx),bg,3))continue;
            int dest=3*((icons[k].dest_y+y)*W+584+x);
            const unsigned char *expected=argc==5 && k==0?selected[k]+3*(sy*W+sx):rgb;
            memcpy(ref+dest,expected,3);memset(mask+dest,255,3);count++;
        }
        if(count!=(unsigned)icons[k].population) {
            fprintf(stderr,"FAIL icon reference population: %u expected %d\n",count,icons[k].population);return 1;
        }
    }
    int rc=save(argv[2],ref)||save(argv[3],mask);
    free(image);free(wall);free(selected[0]);free(selected[1]);free(ref);free(mask);return rc;
}
