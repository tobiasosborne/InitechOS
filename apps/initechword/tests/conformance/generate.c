/* Prefix is an exact real WP5.1 MINT02 capture, not product output.
 * Alterations below are adversarial derivatives, NOT WP5.0/6.0 samples.
 * Authority: wordperfect51-decomp/specs/file-formats/wp5-{header-and-prefix,body-codes}.md. */
#include "bytes.h"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);emit("W01.WP5",n);
 emit("W02.WP5",0);emit("W03.WP5",15);emit("W04.WP5",326);
 emit("W05.WP5",n-1);
 used=327;memcpy(bytes+used,"Readable",8);used+=8;
 bytes[used++]=0xc3;bytes[used++]=12;emit("W06.WP5",used);
 bytes[used++]=0xc4;emit("W07.WP5",used);
 memset(bytes+327,'x',65000);emit("W08.WP5",65327);
 used=327;for(unsigned i=0;i<1000;i++){
  const unsigned char run[]={0xc3,12,0xc3,'b',0xc4,12,0xc4,10};
  memcpy(bytes+used,run,sizeof run);used+=sizeof run;
 }emit("W09.WP5",used);
 memset(bytes+327,'x',65537-327);emit("W10.WP5",65536);emit("W11.WP5",65537);
 return 0;
}
