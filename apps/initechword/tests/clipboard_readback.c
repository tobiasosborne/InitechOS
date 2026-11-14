/* Independent reader of Note Pad's NPAD v1 store (notepad/store.json).
 * Native Finder clipboard journey: paste the original CR snapshot into page3.
 * No app headers/objects, renderer, codec or expected-value generator linked. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static unsigned char b[8225];
static unsigned word(unsigned p){return b[p]+256u*b[p+1];}
int main(int argc,char **argv)
{
 if(argc!=2)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(b,1,sizeof b,f);fclose(f);
 if(n!=8224||memcmp(b,"NPAD",4)||word(4)!=1||word(6)!=8||word(8)>=8||word(10)!=8)return 1;
 uint32_t hash=2166136261u,stored=(uint32_t)b[12]|(uint32_t)b[13]<<8|(uint32_t)b[14]<<16|(uint32_t)b[15]<<24;
 for(unsigned i=0;i<8224;i++)hash=(hash^(i>=12&&i<16?0:b[i]))*16777619u;
 if(hash!=stored)return 1;
 for(unsigned p=0;p<8;p++)if(word(16+p*1026)>1024)return 1;
 const char expected[]="first\rsecond\rthird\rfourth\rfifth\rsixth\rseventh\runiquetail";
 if(word(2068)!=sizeof expected-1||memcmp(b+2070,expected,sizeof expected-1))return 1;
 puts("Finder clipboard: complete literal CR TEXT survived viewer paging and Note Pad paste; NPAD checksum valid");return 0;
}
