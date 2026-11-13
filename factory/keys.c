/* DOS session -> shared comma-separated QEMU/Bochs key script. */
#include <stdio.h>
#include <stdlib.h>
static int first=1;
static void key(const char *s) {printf("%s%s",first?"":",",s); first=0;}
int main(void) {
 int c;
 while((c=getchar())!=EOF) {
  char s[2]={(char)c,0};
  if(c>='A' && c<='Z')c+='a'-'A';
  s[0]=(char)c;
  if(c=='\r')continue;
  if(c=='\n')key("ret"); else if(c==' ')key("spc");
  else if(c=='>')key("shift-dot"); else if(c=='<')key("shift-comma");
  else if(c=='|')key("shift-backslash"); else if(c=='"')key("shift-apostrophe");
  else if(c=='\\')key("backslash"); else if(c==':')key("shift-semicolon");
  else if(c=='.')key("dot"); else if(c=='/')key("slash");
  else if(c=='=')key("equal"); else if(c=='-')key("minus");
  else if(c=='%')key("shift-5"); else if(c>='a'&&c<='z')key(s);
  else if(c=='@')key("shift-2");
  else if(c>='0'&&c<='9')key(s);
  else {fprintf(stderr,"unsupported session character: %d\n",c); return 1;}
 }
 putchar('\n');return ferror(stdin)?1:0;
}
