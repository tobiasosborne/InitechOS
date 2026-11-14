/* R2.2 Reference 4-5/4-6 and tests/fixtures/fill. Erase the complete range
 * column-first, then fill it. A protection failure exposes only the erasure
 * prefix; a memory failure during filling exposes the numeric prefix.
 * The caller operates in a candidate bank and publishes those prefixes. */
#include "i123.h"
#include "i123/wk1_format.h"

int i123_fill_progress(i123_sheet_t *sh,i123_rect_t range,double start,double step,double stop,uint32_t *written)
{
    uint32_t progress=0;
    int rc=I123_OK,done=0;
    double value=start;
    if(written)*written=0;
    if(!sh||range.c0>range.c1||range.r0>range.r1||range.c1>=I123_MAX_COLS||range.r1>=I123_MAX_ROWS)
        return I123_E_ARG;
    if(!(start>=-1e99&&start<=1e99&&step>=-1e99&&step<=1e99&&stop>=-1e99&&stop<=1e99))
        return I123_E_VALUE;
    for(uint32_t col=range.c0;col<=range.c1;col++)for(uint32_t row=range.r0;row<=range.r1;row++) {
        const i123_cell_t *cell=i123_find(sh,(uint16_t)col,(uint16_t)row);
        uint8_t fmt=cell?cell->fmt:WK1_FMT_FRESH_CELL;
        if(sh->protec&&(fmt&128u)){rc=I123_E_PROTECTED;goto out;}
        if(cell) {
            rc=i123_erase(sh,(uint16_t)col,(uint16_t)row);
            if(!rc)rc=i123_set_cell_format(sh,(uint16_t)col,(uint16_t)row,fmt);
            if(rc)goto out;
            progress++;
        }
    }
    for(uint32_t col=range.c0;col<=range.c1;col++)for(uint32_t row=range.r0;row<=range.r1;row++) {
        /* No tolerance: real .1/.1/.3 stops after .2. Zero step repeats. */
        if((step<0.0&&value<stop)||(step>=0.0&&value>stop))done=1;
        if(done)goto out;
        rc=i123_set_number(sh,(uint16_t)col,(uint16_t)row,value);
        if(rc)goto out;
        progress++;
        value+=step;
    }
out:
    if(written)*written=progress;
    return rc;
}
