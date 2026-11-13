/* Test binding to the existing real InitechDOS implementation. Kernel internals
 * are confined to this host-only adapter, never a Toolbox include path. */
#include <string.h>
#include "flair_platform.h"
#include "int21.h"
#include "sft.h"
#include "find_data.h"
#include "tbx.h"
_Static_assert(sizeof(find_data_t) == TBX_FIND_REC_DTA_OFF + TBX_FIND_REC_BYTES, "opaque DTA storage");
_Static_assert(offsetof(find_data_t, attr) == TBX_FIND_REC_DTA_OFF, "DOS find payload offset");
_Static_assert(sizeof(tbx_find_rec_t) == TBX_FIND_REC_BYTES, "SDK find payload size");
_Static_assert(FLAIR_PLATFORM_HANDLES == JFT_MAX_ENTRIES, "session handle capacity");
_Static_assert(sizeof(psp_t) <= sizeof(flair_platform_files), "file session storage");
_Static_assert(sizeof(int_frame_t) == sizeof(flair_platform_regs), "DOS frame");
/* Copy records at the boundary: the opaque storage has no C effective-type
 * dependency on PSP layout, nor do register callers alias an int_frame_t. */
void flair_platform_files_init(flair_platform_files *f)
{
    psp_t p; psp_params_t a = {0};
    (void)psp_build(&p,&a); memcpy(f,&p,sizeof p);
}
void flair_platform_files_close(flair_platform_files *f)
{
    psp_t p; memcpy(&p,f,sizeof p); sft_close_process(&p); memcpy(f,&p,sizeof p);
}
uint32_t flair_platform_files_live(const flair_platform_files *f)
{
    psp_t p; uint32_t n=0; memcpy(&p,f,sizeof p);
    for(unsigned h=5;h<JFT_MAX_ENTRIES;h++) n += p.jft[h] != JFT_CLOSED;
    return n;
}
int flair_platform_file_info_get(flair_platform_files *f,uint8_t h,flair_platform_file_info *out)
{
    psp_t p; memcpy(&p,f,sizeof p); sft_entry_t *e=sft_from_handle(&p,h);
    if(!e || e->kind!=SFT_KIND_FILE) return 0;
    out->offset=e->file_offset; out->size=e->dir_entry.file_size; out->mode=e->open_mode;
    return 1;
}
int flair_platform_path_is_device(const char *p) { return int21_path_is_device(p); }
void flair_platform_dos(flair_platform_files *files,flair_platform_regs *regs)
{
    int_frame_t f; psp_t p, *old=int21_get_psp();
    int21_cwd_snapshot_t cwd=int21_cwd_save(); memcpy(&f,regs,sizeof f);
    if(files) { memcpy(&p,files,sizeof p); int21_set_psp(&p); int21_cwd_reset(); }
    int21_dispatch(&f);
    if(files) { memcpy(files,&p,sizeof p); int21_set_psp(old); int21_cwd_restore(&cwd); }
    memcpy(regs,&f,sizeof f);
}
uint32_t flair_platform_swap_dta(uint32_t dta) { return int21_swap_dta(dta); }
int flair_platform_memory(uint32_t *t,uint32_t *f,uint32_t *l) { return int21_mcb_usage(t,f,l); }
