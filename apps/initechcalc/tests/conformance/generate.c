/* Ledger is TEXT; no import command exists. Exercise launch/save on changed
 * source media, not an invented Ledger Open command. */
#include "bytes.h"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 size_t n=input(argv[1]);emit("C01.TXT",n);emit("C02.TXT",23);
 memset(bytes,'x',8192);emit("C03.TXT",8192);return 0;
}
