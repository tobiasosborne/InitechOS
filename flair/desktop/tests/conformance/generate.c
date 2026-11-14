/* Authored desktop_db.h contract: 8+24*n, max 4096. No product encoder. */
#include "bytes.h"
static void db(unsigned n){memset(bytes,0,sizeof bytes);memcpy(bytes,"IDB1",4);w16(4,1);w16(6,n);}
int main(void){
 db(0);emit("D01.DB",8);emit("D02.DB",7);
 db(1);emit("D03.DB",31);
 db(170);emit("D04.DB",4088);
 db(171);emit("D05.DB",4112);return 0;
}
