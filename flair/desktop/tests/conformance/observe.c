/* Header validator only. Bootstrap disk writes require a real FAT fixture,
 * so no preservation claim is inferred from this bounded-memory probe. */
#include "bytes.h"
#include "../../desktop_db.h"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);desktop_db_reason_t why;
 int rc=desktop_db_validate(bytes,(uint32_t)n,&why);
 printf("%s\tbytes=%zu validate=%d reason=%d bootstrap=PENDING\n",argv[1],n,rc,why);return 0;
}
