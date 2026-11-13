/* Compare only unchanged, unobscured legacy regions. Independent input images;
 * no palette, layout or drawing code is linked into this factory reader. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char *read_ppm(const char *path)
{
    FILE *f=fopen(path,"rb");
    char magic[3]; unsigned w,h,max;
    if(!f || fscanf(f,"%2s %u %u %u",magic,&w,&h,&max)!=4 ||
       strcmp(magic,"P6") || w!=640 || h!=480 || max!=255 || fgetc(f)!='\n') {
        fprintf(stderr,"invalid 640x480 PPM: %s\n",path); exit(1);
    }
    unsigned char *p=malloc(640*480*3);
    if(!p || fread(p,3,640*480,f)!=640*480 || fgetc(f)!=EOF) exit(1);
    fclose(f); return p;
}
int main(int argc,char **argv)
{
    if(argc!=3) return 2;
    unsigned char *a=read_ppm(argv[1]), *b=read_ppm(argv[2]);
    const struct {const char *name;unsigned l,t,r,b;} regions[]={
        {"system-menu-bar",0,0,640,20}, {"disk-icon",575,45,630,100},
        {"Trash-icon",575,390,630,475}, {"desktop-pattern",0,40,19,480}
    };
    for(unsigned k=0;k<sizeof regions/sizeof regions[0];k++) {
        unsigned changed=0,total=0;
        for(unsigned y=regions[k].t;y<regions[k].b;y++)
            for(unsigned x=regions[k].l;x<regions[k].r;x++) {
                unsigned p=(y*640+x)*3;
                changed+=memcmp(a+p,b+p,3)!=0; total++;
            }
        printf("pixels %s: %u/%u differ from legacy (informational)\n",regions[k].name,changed,total);
    }
    free(a);free(b);return 0;
}
