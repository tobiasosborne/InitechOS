/* Input is the literal desktop journey, not an expected serializer result.
 * WK1 layout/RPN: Lotus corpus tools/wk1_ref.py and minted @SUM records. */
#include "i123.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    uint8_t pool[]={2,0xfe,0xbf,0,0x80,0xff,0xbf,0,0x80,80,1,3};
    i123_cell_t cells[3]={{.kind=I123_K_INTEGER,.fmt=255,.v={123}},
        {.col=1,.kind=I123_K_NUMBER,.fmt=255},
        {.col=2,.kind=I123_K_FORMULA,.fmt=255,.len=12}};
    double number=1.5, cached=124.5;
    memcpy(cells[1].v,&number,8);memcpy(cells[2].v,&cached,8);
    uint8_t carry[]={6,0,8,0,0,0,0,0,2,0,0,0}; /* normal RANGE decoder emits [] */
    i123_sheet_t sh={.cells=cells,.ncells=3,.pool=pool,.bof_version=0x406,
        .carry=carry,.carry_len=sizeof carry,.carry_cellpos=sizeof carry};
    uint8_t bytes[4096];uint32_t len;
    if(argc!=2 || i123_wk1_write(&sh,bytes,sizeof bytes,&len))return 1;
    FILE *f=fopen(argv[1],"wb");if(!f)return 1;
    int ok=fwrite(bytes,1,len,f)==len;return fclose(f)||!ok;
}
