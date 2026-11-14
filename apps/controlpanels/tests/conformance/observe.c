/* Decode boundary only: the SDK host's SETTINGS GET does not load disk.
 * Exact-length refusal is a test-side precondition, not observed app I/O. */
#include "bytes.h"
#include "cpanels_record.h"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);cp_settings_t s;cp_defaults(&s);
 if(n!=16){printf("%s\tbytes=%zu disk-load=PENDING (no length-bearing decoder)\n",argv[1],n);return 0;}
 int rc=cp_decode(&s,bytes);
 printf("%s\tbytes=%zu decoded=%d pattern=%u tracking=%u clock=%u\n",argv[1],n,rc,s.pattern,s.tracking,s.clock);
 return 0;
}
