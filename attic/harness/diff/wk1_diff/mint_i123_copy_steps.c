/* Factory-only translation of mint_drive steps to DOSBox-X AUTOTYPE.
 * AUTOTYPE's comma inserts one PACE delay. Capture is real DX-CAPTURE. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
int main(int argc,char **argv) {
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"r");if(!f)return 2;
 printf("[sdl]\noutput=surface\nvideodriver=dummy\n[dosbox]\nmachine=svga_s3\nmemsize=8\ncaptures=%s/capture\nquit warning=false\n[cpu]\ncputype=386\ncycles=max\n[autoexec]\nmount c %s/work\nc:\nautotype -w 4 -p 0.2",argv[2],argv[2]);
 char l[512];unsigned ticks=0;
 while(fgets(l,sizeof l,f)) {
  l[strcspn(l,"\r\n")]=0;
  if(!strncmp(l,"type ",5))for(char *p=l+5;*p;p++) {
   const char *key=0;
   switch(*p){case '/':key="slash";break;case '.':key="period";break;case '+':key="kp_plus";break;case '*':key="kp_multiply";break;case '$':key="dollar";break;}
   if(key)printf(" %s",key);else printf(" %c",tolower((unsigned char)*p));ticks++;
  } else if(!strncmp(l,"key ",4)) {
   char *k=l+4;
   if(!strcmp(k,"Return"))k="enter";else if(!strcmp(k,"Escape"))k="esc";else if(!strcmp(k,"BackSpace"))k="bspace";
   putchar(' ');for(char *p=k;*p;p++)putchar(tolower((unsigned char)*p));ticks++;
  } else if(!strncmp(l,"shot ",5)) {
   for(int j=0;j<4;j++){printf(" ,");ticks++;}
   fprintf(stderr,"%s %.1f\n",l+5,4.0+ticks*.2-.3);
  } else if(!strncmp(l,"sleep ",6)&&ticks) {
   unsigned n=(unsigned)(atof(l+6)*5+1);while(n--){printf(" ,");ticks++;}
  }
 }
 printf(" , , slash q y\ndx-capture /V 123\n");fclose(f);return 0;
}
