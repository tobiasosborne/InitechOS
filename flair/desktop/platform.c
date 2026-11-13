/* Target binding to the real InitechDOS public contracts. File-session logic
 * follows the host-verified adapter in flair/tests/platform_dos.c. */
#include "flair_platform.h"
void *memcpy(void *dst, const void *src, size_t n);
#include "int21.h"
#include "sft.h"
#include "find_data.h"
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
#include "loader.h"
#include "io.h"
#include "desktop.h"
#include "panic.h"
extern int tbx_tenant_call(uint32_t fn, uint32_t psp);
extern void tbx_tenant_abort(void);
extern void serial_puts(const char *s);
void *memcpy(void *dst, const void *src, size_t n);
int flair_platform_load(const char *name, uint16_t dir, flair_platform_carve carve,
                        void *user, void **block, flair_platform_plan *out)
{
    tenant_plan_t p;
    loader_status_t s = loader_load_tenant(name, dir, carve, user, block, &p);
    if (s == LOADER_OK) {
        out->psp_addr=p.psp_addr; out->load_base=p.load_base;
        out->module_len=p.module_len; out->entry=p.entry; out->block_len=p.block_len;
    }
    return s;
}
int flair_platform_tenant_call(uint32_t fn, uint32_t psp) { return tbx_tenant_call(fn,psp); }
_Noreturn void flair_platform_tenant_abort(void) { tbx_tenant_abort(); __builtin_unreachable(); }
void flair_platform_desktop_pattern(unsigned p) { desktop_set_pattern(p); }
void flair_platform_speaker(unsigned hz)
{
    if (!hz) { outb(0x61, inb(0x61)&~3u); return; }
    unsigned d=1193182u/hz;
    outb(0x43,0xb6); outb(0x42,d); outb(0x42,d>>8); outb(0x61,inb(0x61)|3u);
}
void flair_platform_serial(const char *s) { serial_puts(s); }
_Noreturn void flair_platform_panic(const char *s)
{
    panic_show_reason(PANIC_REASON_NONE, s);
    for (;;) __asm__ volatile("cli; hlt");
}
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d=dst; const unsigned char *s=src;
    while(n--) *d++=*s++;
    return dst;
}
