/* Original C transcript oracle for the Toolbox's callback-based serial sink.
 * Ref: PRD Sec 8; os/flair/tbxgate.c tputs/tputu/tputi/tputx contracts. */
#include <stdio.h>
#include <stdint.h>
static void tputs(const char *s) { fputs(s,stdout); }
static void tputu(uint32_t v) { printf("%u",v); }
static void tputi(int32_t v) { printf("%d",v); }
static void tputx(uint32_t v,int digits) { printf("%0*x",digits,v); }
int main(void)
{
    const char *s="TEXT.EXE";
    for(unsigned i=0;i<20;i++) {
        tputs("TENANT-GATE ax="); tputx(i,4);
        tputs(" win="); tputu(i); tputs(" name="); tputs(s); tputs("\n");
    }
    tputs("signed="); tputi(INT32_MIN); tputs(" unsigned="); tputu(UINT32_MAX);
    tputs("\n");
    if(s) tputs("controlled=");
    tputi(-5); tputs("\n");
    return 0;
}
