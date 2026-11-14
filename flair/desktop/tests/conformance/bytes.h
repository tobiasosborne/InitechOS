/* Test-side byte construction only; no product codec or product constants. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char bytes[200000];
static size_t used;
static void w16(size_t p,unsigned v){bytes[p]=(unsigned char)v;bytes[p+1]=(unsigned char)(v>>8);}
static void w32(size_t p,uint32_t v){w16(p,v&65535u);w16(p+2,v>>16);}
static void emit(const char *name,size_t n){
 FILE *f=fopen(name,"wb");if(!f||fwrite(bytes,1,n,f)!=n||fclose(f)){perror(name);exit(2);}
}
static size_t input(const char *name){
 FILE *f=fopen(name,"rb");if(!f){perror(name);exit(2);}size_t n=fread(bytes,1,sizeof bytes,f);
 if(ferror(f)||!feof(f)){fprintf(stderr,"input too large\n");exit(2);}fclose(f);return n;
}
