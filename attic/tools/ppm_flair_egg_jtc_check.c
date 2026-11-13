/* Factory C, initech-2ter (a). Pixel-exact crop vs a SELF-MINTED authored
 * still. It is a regression oracle, not an independent historical reference.
 * No renderer/spec headers enter this grader (Law 2). Ref: fixture README. */
#include <stdio.h>
#include <string.h>
#include <png.h>
enum { W=640,H=480,X=100,Y=70,CW=450,CH=370 };
static unsigned char pixels[W*H*3], crop[CW*CH*3], gold[CW*CH*3];
int main(int argc,char **argv)
{
    int mint = argc == 4 && strcmp(argv[1],"--mint") == 0;
    if (argc != 3 && !mint) return 2;
    FILE *f = fopen(argv[1+mint],"rb");
    char magic[3]; int w,h,v;
    if (!f || fscanf(f,"%2s %d %d %d",magic,&w,&h,&v) != 4 ||
        strcmp(magic,"P6") || w != W || h != H || v != 255) {
        fprintf(stderr,"FAIL JTC pixels: invalid PPM\n"); return 1;
    }
    (void)fgetc(f);
    if (fread(pixels,1,sizeof pixels,f) != sizeof pixels) return 2;
    fclose(f);
    for (int y=0;y<CH;y++) memcpy(crop+y*CW*3,pixels+((y+Y)*W+X)*3,CW*3);
    png_image im = {0}; im.version=PNG_IMAGE_VERSION; im.format=PNG_FORMAT_RGB;
    const char *path = argv[2+mint];
    if (mint) {
        im.width=CW; im.height=CH;
        if (!png_image_write_to_file(&im,path,0,crop,0,0)) return 2;
        printf(">>> JTC self-minted still: %s\n",path); return 0;
    }
    if (!png_image_begin_read_from_file(&im,path)) {
        fprintf(stderr,"FAIL JTC pixels: missing authored still %s\n",path); return 1;
    }
    im.format=PNG_FORMAT_RGB;
    if (im.width != CW || im.height != CH || !png_image_finish_read(&im,0,gold,0,0)) return 2;
    png_image_free(&im);
    for (unsigned i=0;i<sizeof crop;i++) if (crop[i] != gold[i]) {
        fprintf(stderr,"FAIL JTC pixels: (%u,%u) channel %u got=%u expected=%u\n",
            X+(i/3)%CW,Y+i/(CW*3),i%3,crop[i],gold[i]); return 1;
    }
    puts(">>> JTC pixels: exact (166500 pixels, zero tolerance)");
    return 0;
}
