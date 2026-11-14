/* Independent FAT12 fixture damage, BPB/root-directory format: MS-DOS 3.30
 * Technical Reference disk format. Reserved cluster FF0h cannot be read as
 * ordinary file data. No kernel reader or product headers are linked. */
#include <stdio.h>
#include <stdint.h>
static unsigned word(const unsigned char *p){return p[0]+256u*p[1];}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"r+b");if(!f)return 2;
    unsigned char b[512],entry[32];if(fread(b,1,512,f)!=512)return 2;
    unsigned bytes=word(b+11),entries=word(b+17);
    unsigned root=(word(b+14)+b[16]*word(b+22))*bytes;
    for(unsigned i=0;i<entries;i++) {
        if(fseek(f,(long)(root+32*i),SEEK_SET) || fread(entry,1,32,f)!=32)return 2;
        if(entry[0]=='B' && entry[1]=='R' && entry[2]=='O' && entry[3]=='K') {
            entry[26]=0xf0;entry[27]=0x0f;entry[28]=1;entry[29]=entry[30]=entry[31]=0;
            if(fseek(f,(long)(root+32*i),SEEK_SET) || fwrite(entry,1,32,f)!=32)return 2;
            return fclose(f)!=0;
        }
    }
    return 2;
}
