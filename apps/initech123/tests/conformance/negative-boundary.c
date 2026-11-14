/* Independent edit of the INTEGER operand to -1, valid signed WK1 integer. */
#include <stdio.h>
int main(void){int c;unsigned n=0;while((c=getchar())!=EOF){if(n==26||n==27)c=255;putchar(c);n++;}return 0;}
