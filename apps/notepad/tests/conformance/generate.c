/* Independent construction from include/notepad/store.json, an authored
 * contract. Never calls the app encoder/checksum. */
#include "bytes.h"
static void checksum(unsigned n){
 uint32_t h=2166136261u;memset(fixture_bytes+12,0,4);
 for(unsigned i=0;i<n;i++)h=(h^fixture_bytes[i])*16777619u;w32(12,h);
}
int main(void){
 for(unsigned kind=0;kind<2;kind++){
  unsigned slots=kind?32:8,n=16+slots*1026;char name[20];
  memset(fixture_bytes,0,sizeof fixture_bytes);memcpy(fixture_bytes,kind?"SBK1":"NPAD",4);
  w16(4,1);w16(6,slots);w16(8,slots-1);w16(10,slots);
  for(unsigned p=0;p<slots;p++){w16(16+p*1026,1024);memset(fixture_bytes+18+p*1026,'A'+p%26,1024);}
  checksum(n);snprintf(name,sizeof name,"%c01.DAT",kind?'S':'N');emit(name,n);
  snprintf(name,sizeof name,"%c02.DAT",kind?'S':'N');emit(name,n-1);
  fixture_bytes[n-1]^=1;snprintf(name,sizeof name,"%c03.DAT",kind?'S':'N');emit(name,n);fixture_bytes[n-1]^=1;
  w16(16,1025);checksum(n);snprintf(name,sizeof name,"%c04.DAT",kind?'S':'N');emit(name,n);
 }return 0;
}
