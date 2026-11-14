/* Independent P6 RGB comparator. Exact whole image by default; an explicit
 * binary mask restricts enforcement to independently referenced pixels. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { W=640,H=480,N=W*H*3 };
static unsigned char *read_ppm(const char *path)
{
    FILE *f=fopen(path,"rb");
    char magic[3];unsigned w,h,max;
    if(!f || fscanf(f,"%2s %u %u %u",magic,&w,&h,&max)!=4 ||
       strcmp(magic,"P6") || w!=W || h!=H || max!=255 || fgetc(f)!='\n') {
        fprintf(stderr,"invalid 640x480 PPM: %s\n",path);exit(2);
    }
    unsigned char *p=malloc(N);
    if(!p || fread(p,1,N,f)!=N || fgetc(f)!=EOF || ferror(f)) {
        fprintf(stderr,"invalid PPM body: %s\n",path);exit(2);
    }
    fclose(f);return p;
}
int main(int argc,char **argv)
{
    int informational=argc>1 && !strcmp(argv[1],"--informational");
    if(informational) {argc--;argv++;}
    if(argc!=3 && argc!=4)return 2;
    unsigned char *a=read_ppm(argv[1]), *b=read_ppm(argv[2]);
    unsigned char *mask=argc==4?read_ppm(argv[3]):NULL;
    unsigned changed=0,total=0;
    for(unsigned i=0;i<N;i+=3) {
        if(mask) {
            if((mask[i]!=0 && mask[i]!=255) || mask[i]!=mask[i+1] || mask[i]!=mask[i+2]) {
                fputs("FAIL pixels: mask must be binary RGB\n",stderr);return 2;
            }
            if(!mask[i])continue;
        }
        total++;
        if(memcmp(a+i,b+i,3)) {
            if(changed<4)printf("pixel (%u,%u): reference #%02x%02x%02x actual #%02x%02x%02x\n",
                i/3%W,i/3/W,a[i],a[i+1],a[i+2],b[i],b[i+1],b[i+2]);
            changed++;
        }
    }
    if(!total) {fputs("FAIL pixels: empty reference mask\n",stderr);return 2;}
    printf("pixels %s: %u/%u differ (%s)\n",mask?"reference mask":"whole desktop",
           changed,total,informational?"INFORMATIONAL: no independent whole-screen reference":"ENFORCED");
    free(a);free(b);free(mask);
    return informational?0:changed!=0;
}
