/* Independent physical-line inputs, executed by the pinned OEM COMMAND.COM. */
#include <stdio.h>
#include <string.h>
int main(void) {
    printf("@ECHO OFF\r\nB:\r\n");
    for(int n=124;n<=132;n++) {
        char prefix[64]; sprintf(prefix,"ECHO >E%d.OUT ",n);
        fputs(prefix,stdout);
        for(int i=(int)strlen(prefix);i<n-1;i++)putchar('x');
        printf("Z\r\n");
    }
    printf("REM ");
    for(int i=0;i<512;i++)putchar('r');
    printf(" DEL KEEP.DAT\r\nECHO REM-AFTER>REM.OUT\r\n");
    for(int spaces=122;spaces<=123;spaces++) {
        printf("CD \\MASK%d\r\nECHO MASK-%d-BEGIN\r\nDEL",spaces,spaces);
        for(int i=0;i<spaces;i++)putchar(' ');
        printf("*.TMP\r\nECHO MASK-%d-END\r\n",spaces);
    }
    printf("ECHO CAPTURE-DONE\r\n");
    return 0;
}
