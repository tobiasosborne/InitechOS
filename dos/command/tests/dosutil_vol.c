/* MS-DOS 3.3 User's Guide p.126: VOL is an internal command. */
#include <stdio.h>
#include "command.h"
int main(void)
{
    cmd_line_t p;
    cmd_parse("vol a:",&p);
    if (p.kind==CMD_EXTERNAL) { printf("FAIL: V1 VOL internal dispatch\nINITECH_CHECKS 1 failures=1\n"); return 1; }
    puts("PASS: V1 VOL internal dispatch"); return 0;
}
