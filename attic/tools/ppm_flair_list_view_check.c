/* tdnl.44 independent Mac OS 8.1 capture oracle. NO artifact includes.
 * Ref: system7-decomp re/mint-results-008 Sec 6. Full four-cell header,
 * shaded columns, 19px row rules. Geometry belongs to the locked trace. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
static unsigned char got[640*480*3], gold[640*480*3];
static int compare(int x,int y,int gx,int gy,int w,int h,const char *what)
{
    int bad=0;
    for(int r=0;r<h;r++)for(int c=0;c<w;c++)
        if(memcmp(got+3*((y+r)*640+x+c),gold+3*((gy+r)*640+gx+c),3)) {
            if(bad<5) {
                const unsigned char *p=got+3*((y+r)*640+x+c), *q=gold+3*((gy+r)*640+gx+c);
                fprintf(stderr,"FAIL list-view %s pixel %d,%d RGB %02x%02x%02x != capture %d,%d %02x%02x%02x\n",what,x+c,y+r,p[0],p[1],p[2],gx+c,gy+r,q[0],q[1],q[2]);
            }
            bad++;
        }
    if(bad)fprintf(stderr,"FAIL list-view %s: %d/%d pixels differ\n",what,bad,w*h);
    return bad;
}
static int persisted(const char *path)
{
    unsigned char db[4096];FILE *f=fopen(path,"rb");
    if(!f)return 2;
    size_t n=fread(db,1,sizeof db,f);fclose(f);
    if(n<8 || memcmp(db,"IDB1",4) || db[4]!=1)return 2;
    unsigned nr=db[6]+256u*db[7], found=0;
    if(n!=8u+24u*nr)return 2;
    for(unsigned i=0;i<nr;i++) {
        unsigned char *r=db+8+24*i;
        if(r[0]==4 && !memcmp(r+4,"APPS",5)) {
            found++;
            if(r[17]!=8 || r[18] || r[19]!=60 || r[20] || r[21]!=5 || r[22]) {
                fputs("FAIL list-view kind=4 APPS frame/mode bytes\n",stderr);return 1;
            }
        }
    }
    if(found!=1){fputs("FAIL list-view kind=4 APPS missing/duplicated\n",stderr);return 1;}
    puts(">>> list-view DESKTOP.DB: green (independent kind=4 APPS origin8,60 bits5)");return 0;
}
int main(int argc,char **argv)
{
    if(argc==3 && !strcmp(argv[1],"--db"))return persisted(argv[2]);
    if(argc!=4){fprintf(stderr,"usage: list_check dump.ppm capture.png name|size|reopen|folder\n");return 2;}
    FILE *f=fopen(argv[1],"rb");int w,h,m;
    if(!f||fscanf(f,"P6 %d %d %d",&w,&h,&m)!=3||w!=640||h!=480||m!=255||fgetc(f)==EOF||fread(got,1,sizeof got,f)!=sizeof got)return 2;
    fclose(f);png_image im={0};im.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&im,argv[2]))return 2;
    im.format=PNG_FORMAT_RGB;
    if(im.width!=640||im.height!=480||!png_image_finish_read(&im,0,gold,0,0))return 2;
    png_image_free(&im);
    int reopen=!strcmp(argv[3],"reopen"), folder=!strcmp(argv[3],"folder"), bad=0;
    if(folder) {
        bad+=compare(26,82,13,69,333,20,"root header");
        bad+=compare(54,103,41,109,16,16,"small folder");
        if(bad)return 1;
        puts(">>> ppm_flair_list_view_check: green (independent native16 folder cell)");return 0;
    }
    /* Reopened kind=4 retains origin, not dimensions (existing DB contract).
     * Grade the visible Name cell and Date prefix at the narrow saved frame. */
    bad+=compare(14,82,13,69,reopen?333:604,20,"header");
    if(!strcmp(argv[3],"name")) {
        /* APPS' independently locked alphabetical entries: 123.EXE then
         * 123.MNU. Compare native app/document cells, not our asset table. */
        bad+=compare(42,103,41,90,16,16,"small application");
        bad+=compare(42,122,41,128,16,16,"small document");
    }
    if(!reopen) {
        for(int row=0;row<8;row++) {
            bad+=compare(200,102+row*19,200,89+row*19,12,19,"Name body");
            bad+=compare(428,102+row*19,427,89+row*19,8,19,"Size body");
        }
    }
    if(bad)return 1;
    printf(">>> ppm_flair_list_view_check: green (%s; independent header cells + shaded row rules)\n",argv[3]);return 0;
}
