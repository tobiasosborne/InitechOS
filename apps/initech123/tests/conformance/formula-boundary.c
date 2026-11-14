/* Independent WK1 constructor; RPN constant 1, n PAREN tokens, RETURN.
 * File-format authority: lotus123-decomp/specs/file-formats/wk1-format.md. */
#include <stdio.h>
#include <stdlib.h>
static void w16(unsigned n) { putchar(n&255); putchar((n>>8)&255); }
int main(int argc,char **argv) {
 if(argc!=2 && argc!=3)return 2;
 unsigned n=(unsigned)atoi(argv[1]);unsigned wide=argc==3;
 unsigned code=n+(wide?10:4);
 w16(0);w16(2);w16(0x406);
 w16(16);w16(15+code);putchar(0x7f);w16(0);w16(0);
 for(int i=0;i<8;i++)putchar(0);
 w16(code);
 if(wide){putchar(0);for(int i=0;i<6;i++)putchar(0);putchar(0xf0);putchar(0x3f);}
 else{putchar(5);w16(1);}
 for(unsigned i=0;i<n;i++)putchar(4);
 putchar(3);w16(1);w16(0);return ferror(stdout)?2:0;
}
