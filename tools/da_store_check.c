/* Independent FAT file oracle, factory C, tdnl.18. No artifact/spec headers.
 * Ref: spec/notepad/store.json; expected page bytes are command arguments
 * locked in the emulator gate, never taken from the renderer or TE record. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char b[32849];
static unsigned word(int o) { return b[o]|(b[o+1]<<8); }
int main(int argc,char **argv)
{
    if(argc<5) return 2;
    int sb=!strcmp(argv[1],"scrapbook"),slots=sb?32:8,size=16+slots*1026;
    int count=sb?argc-4:8,current=atoi(argv[3]),bad=0;
    FILE *f=fopen(argv[2],"rb");
    if(!f) {printf("RED-REASON disk: missing %s\n",argv[2]);return 1;}
    int n=(int)fread(b,1,sizeof b,f);fclose(f);
    if(n!=size || memcmp(b,sb?"SBK1":"NPAD",4) || word(4)!=1 ||
       word(6)!=(unsigned)count || word(8)!=(unsigned)current || word(10)!=(unsigned)slots) bad++;
    uint32_t h=2166136261u,stored=0;
    for(int i=0;i<size;i++) h=(h^(uint32_t)((i>=12&&i<16)?0:b[i]))*16777619u;
    for(int i=0;i<4;i++) stored|=(uint32_t)b[12+i]<<(8*i);
    bad+=h!=stored;
    for(int p=0;p<slots;p++) {
        const char *want=p<argc-4?argv[4+p]:"";
        int len=(int)strlen(want),off=16+p*1026;
        bad+=word(off)!=(unsigned)len || memcmp(b+off+2,want,(size_t)len)!=0;
        for(int i=len;i<1024;i++) bad+=b[off+2+i]!=0;
    }
    if(bad) printf("RED-REASON disk: %s count/current/pages/checksum differ (%d)\n",argv[1],bad);
    else printf(">>> DA DISK: %s count=%d current=%d; exact text, fixed records, zero padding, FNV1a checksum\n",argv[1],count,current);
    return bad!=0;
}
