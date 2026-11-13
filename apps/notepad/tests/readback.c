/* Independent reader of the claimed store.json byte contract. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv) {
    unsigned char b[32849];
    if(argc!=4) return 2;
    FILE *f=fopen(argv[1],"rb"); if(!f) return 2;
    size_t n=fread(b,1,sizeof b,f); fclose(f);
    unsigned slots=!strcmp(argv[2],"NPAD")?8:32;
    if(n!=16+slots*1026 || memcmp(b,argv[2],4)) return 1;
    uint32_t h=2166136261u, stored=0;
    for(size_t i=0;i<n;i++) h=(h^(i>=12&&i<16?0:b[i]))*16777619u;
    for(unsigned i=0;i<4;i++) stored|=(uint32_t)b[12+i]<<(8*i);
    if(h!=stored || b[4]!=1 || b[5] || b[10]!=slots || b[11]) return 1;
    unsigned len=b[16]|(b[17]<<8);
    if(len!=strlen(argv[3]) || memcmp(b+18,argv[3],len)) return 1;
    printf("store: %zu bytes, valid independent checksum, page 1 = %s\n",n,argv[3]);
    return 0;
}
