/* Compare whole containers; diagnose first unequal byte, never normalize. */
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    FILE *a=fopen(argv[1],"rb"),*b=fopen(argv[2],"rb");
    if(!a||!b)return 2;
    unsigned header=0;
    if(fseek(a,8,SEEK_SET))return 2;
    int lo=fgetc(a),hi=fgetc(a);if(lo<0||hi<0)return 2;
    header=(unsigned)(lo+hi*256)*16;rewind(a);
    unsigned off=0;int x,y;
    do {
        x=fgetc(a);y=fgetc(b);
        if(x!=y){
            fprintf(stderr,"fixed point differs at offset %u (0x%x): %d vs %d; ",off,off,x,y);
            if(off<32)fprintf(stderr,"MZ header\n");
            else if(off<56)fprintf(stderr,"extended descriptor\n");
            else if(off<header)fprintf(stderr,"relocation table/header padding\n");
            else fprintf(stderr,"load module offset %u (0x%x)\n",off-header,off-header);
            return 1;
        }
        off++;
    }while(x!=EOF);
    if(ferror(a)||ferror(b)||fclose(a)||fclose(b))return 2;
    printf("fixed point: %u bytes equal\n",off-1);return 0;
}
