/* The original About-dialog text/units, independent of its moved formatter.
 * Ref: kmain.c About This Computer contract; PRD Sec 6.5. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kmain_cold.h"
int main(void)
{
    static const struct {uint32_t mem,largest;int bound;const char *a,*b;} cases[]={
        {66560,32768,1,"Built-in Memory: 66560K","Largest Unused Block: 512K"},
        {1024,32768,0,"Built-in Memory: 1024K","Largest Unused Block: none"},
        {UINT32_MAX,UINT32_MAX,1,"Built-in Memory: 4294967295K","Largest Unused Block: 67108863K"}
    };
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++) {
        char a[49],b[49];memset(a,'X',sizeof a);memset(b,'X',sizeof b);
        kmain_about_lines(a,b,cases[i].mem,cases[i].largest,cases[i].bound);
        if(strcmp(a,cases[i].a)||strcmp(b,cases[i].b)||a[48]!='X'||b[48]!='X') {
            printf("!!! cold-about FAIL: case %u text/units/bounds differ\n",i);return 1;
        }
    }
    puts(">>> test-kernel-cold-about: green (memory units, unbound text, uint32 limits, bounds)");
    return 0;
}
