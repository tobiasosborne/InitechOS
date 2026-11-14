/* Deterministic catalogue compiler. Source: include/dos_messages.json. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char buf[65536];
int main(void) {
 size_t n=fread(buf,1,sizeof(buf)-1,stdin); buf[n]=0;
 puts("/* Generated from kernel/include/dos_messages.json. */");
 puts("#ifndef INITECH_DOS_MESSAGES_H\n#define INITECH_DOS_MESSAGES_H");
 for(unsigned i=1;i<=68;i++) {
  char key[32]; snprintf(key,sizeof key,"\"MSG-DOS-%04u\"",i);
  char *p=strstr(buf,key); if(!p)return 1;
  p=strchr(p+strlen(key),':'); if(!p)return 1;
  while(*++p && *p!='"'){} if(!*p)return 1;
  printf("#define MSG_DOS_%04u \"",i);
  for(++p;*p && *p!='"';p++) {
   if((unsigned char)*p>=128)return 1;
   putchar(*p);
   if(*p=='\\') {if(!p[1])return 1; putchar(*++p);}
  }
  if(*p!='"')return 1;
  puts("\"");
 }
 puts("#endif"); return 0;
}
