/* Authored TEXT bytes. Expected property: finish/refuse visibly; no silent
 * loss on Save. CRLF->CR and LF->CR are the documented app contract. */
#include "bytes.h"
int main(void){
 memcpy(bytes,"alpha\r\nbravo\n\ncharlie\r",22);emit("T01.TXT",22);
 memset(bytes,'x',4097);emit("T02.TXT",4096);emit("T03.TXT",4097);
 memcpy(bytes,"KEEP-FIRST\rKEEP-LAST\r",20);emit("T04.TXT",20);
 for(unsigned i=0;i<4096;i+=2){bytes[i]=13;bytes[i+1]=10;}
 bytes[4096]='z';emit("T05.TXT",4097);return 0;
}
