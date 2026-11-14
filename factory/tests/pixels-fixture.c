/* Synthetic transport/comparator inputs, not product visual goldens. */
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    FILE *f=fopen(argv[2],"wb");if(!f)return 2;
    fputs("P6\n640 480\n255\n",f);
    for(int y=0;y<480;y++)for(int x=0;x<640;x++) {
        int value=!strcmp(argv[1],"white") || (!strcmp(argv[1],"point") && x==300 && y==240)?255:0;
        if(!strcmp(argv[1],"bad-mask") && x==300 && y==240)value=127;
        for(int c=0;c<3;c++)if(fputc(value,f)==EOF)return 2;
    }
    return fclose(f)!=0;
}
