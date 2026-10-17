/* About This Computer's cold text path, mechanically moved from kmain.c.
 * Ref: PRD Sec 6.5; kmain.c About contract: actual probed KiB, largest MCB
 * block in paragraphs / 64, and "none" for an unbound arena. The bounded
 * append routines and every visible byte are unchanged. Rule 11/12.
 */
#include "kmain_cold.h"
static int about_put(char *o,int at,const char *s)
{
    while(*s && at<47) o[at++]=*s++;
    o[at]='\0';
    return at;
}
static int about_putu(char *o,int at,uint32_t v)
{
    char t[12]; int n=0;
    do {t[n++]=(char)('0'+(int)(v%10u));v/=10u;} while(v!=0u);
    while(n>0 && at<47) o[at++]=t[--n];
    o[at]='\0';
    return at;
}
void kmain_about_lines(char memory[48],char unused[48],uint32_t mem_k,
                       uint32_t largest_paras,int bound)
{
    int at=about_put(memory,0,"Built-in Memory: ");
    at=about_putu(memory,at,mem_k);
    (void)about_put(memory,at,"K");
    at=about_put(unused,0,"Largest Unused Block: ");
#ifdef KMAIN_ABOUT_MUT_NONE
    bound=1;
#endif
    if(bound) {
        at=about_putu(unused,at,largest_paras/64u);
        (void)about_put(unused,at,"K");
    } else (void)about_put(unused,at,"none");
}
