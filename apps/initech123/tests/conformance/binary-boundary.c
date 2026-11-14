/* WK1 RPN INT(1), INT(2), ADD, n PAREN, RETURN. */
#include <stdio.h>
#include <stdlib.h>
static void w(unsigned n){putchar(n&255);putchar((n>>8)&255);}
int main(int argc,char **argv){if(argc!=2)return 2;unsigned n=(unsigned)atoi(argv[1]);w(0);w(2);w(0x406);w(16);w(23+n);putchar(0x7f);w(0);w(0);for(int i=0;i<8;i++)putchar(0);w(n+8);putchar(5);w(1);putchar(5);w(2);putchar(9);for(unsigned i=0;i<n;i++)putchar(4);putchar(3);w(1);w(0);return 0;}
