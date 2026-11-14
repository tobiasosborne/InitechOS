/* Authored record.json: literal valid record and corrupt derivatives.
 * The transport's disk loader belongs to FLAIR; cp_decode has no length arg. */
#include "bytes.h"
int main(void){
 const unsigned char good[16]={'C','P','0','1',1,3,1,1,0,2,1,0,0,0,0xfd,0};
 memcpy(bytes,good,16);emit("P01.DAT",16);emit("P02.DAT",15);
 bytes[4]=2;bytes[14]=0xfe;emit("P03.DAT",16);return 0;
}
