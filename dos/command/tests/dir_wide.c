/* Format names enumerated independently by mtools. The shell gate compares
 * these bytes to genuine DOS, including tabs, directories and final rows. */
#include <stdio.h>
#include <string.h>
#include "command.h"
int main(void)
{
    char name[128],cell[17];unsigned col=0;
    while (fgets(name,sizeof name,stdin)) {
        name[strcspn(name,"\r\n")]=0;
        unsigned n=(unsigned)strlen(name),attr=0;
        if (n && name[n-1]=='/') { name[--n]=0;attr=0x10; }
        int len=cmd_format_dir_wide(name,(uint8_t)attr,col,cell);
        fwrite(cell,1,(unsigned)len,stdout);
        if (++col==5) { fputs("\r\n",stdout);col=0; }
    }
    if (col) fputs("\r\n",stdout);
    return ferror(stdin) || ferror(stdout) ? 2:0;
}
