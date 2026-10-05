/* Toolbox file adapter; initech-tdnl.91. Authored, no local reference for ABI.
 * Ref: spec/toolbox_gate.h Sec 10; docs/design/toolbox-file-verbs.md.
 * Shared artifact logic compiled verbatim by the host oracle. Kernel-owned
 * shadow PSP only: never read the image PSP's JFT, including at death. */
#ifndef INITECH_TBXFILE_H
#define INITECH_TBXFILE_H
#include "toolbox_gate.h"
#include "int21.h"
#include "sft.h"
static void tbxf_init(psp_t *p)
{
    psp_params_t a = { 0 };
    (void)psp_build(p, &a);
}
static int tbxf_span(uint32_t lo, uint32_t len, uint32_t ptr, uint32_t n)
{
#ifdef TBX_MUT_FILE_BOUNDS
    (void)lo; (void)len; (void)ptr; (void)n; return 1;
#else
    if (n == 0) return 1;
    return len <= 0xffffffffu-lo && ptr >= lo && ptr-lo <= len && n <= len-(ptr-lo);
#endif
}
static int tbxf_path(uint32_t lo, uint32_t len, uint32_t ptr)
{
    for (uint32_t i = 0; i < TBX_FILE_PATH_MAX; i++) {
        if (!tbxf_span(lo,len,ptr+i,1)) return 0;
        if (*(const uint8_t *)(uintptr_t)(ptr+i) == 0) return i != 0;
    }
    return 0;
}
static int32_t tbxf_call(psp_t *p, uint32_t lo, uint32_t len,
                         uint32_t code, const uint32_t *a)
{
    int_frame_t f = { 0 };
    sft_entry_t *e = 0;
    uint32_t h = a[0];
    if (code == TBX_FILE_CREATE || code == TBX_FILE_OPEN || code == TBX_FILE_DELETE) {
        if (!tbxf_path(lo,len,a[0])) return TBX_ERR_BADARG;
#ifndef TBX_MUT_FILE_OWNER
        /* Ref: DOS OPEN's device precedence, int21.c dev_open_lookup.
         * Authored file-only gate policy: never CREATE an unreadable disk file
         * whose basename OPEN resolves as a character device. */
        if (code == TBX_FILE_CREATE && int21_path_is_device((const char *)(uintptr_t)a[0]))
            return TBX_ERR_BADARG;
#endif
        if (code == TBX_FILE_OPEN && a[1] > TBX_FILE_READ_WRITE) return TBX_ERR_BADARG;
        f.edx = a[0];
        f.eax = code == TBX_FILE_CREATE ? 0x3c00u :
                 code == TBX_FILE_DELETE ? 0x4100u : 0x3d00u+a[1];
    } else {
#ifndef TBX_MUT_FILE_OWNER
        if (h < 5 || h >= JFT_MAX_ENTRIES || p->jft[h] == JFT_CLOSED) return TBX_ERR_BADARG;
        e = sft_from_handle(p,(uint8_t)h);
        if (e == 0 || e->kind != SFT_KIND_FILE) return TBX_ERR_BADARG;
#else
        if (h < JFT_MAX_ENTRIES) e = sft_from_handle(p,(uint8_t)h);
#endif
        f.ebx = h;
        switch (code) {
        case TBX_FILE_READ:
        case TBX_FILE_WRITE:
            if (e == 0 || (code == TBX_FILE_READ && e->open_mode == SFT_MODE_WRITE) ||
                (code == TBX_FILE_WRITE && e->open_mode == SFT_MODE_READ) ||
                a[2] > 0x7fffffffu || !tbxf_span(lo,len,a[1],a[2])) return TBX_ERR_BADARG;
            f.eax = code == TBX_FILE_READ ? 0x3f00u : 0x4000u;
            f.edx = a[1]; f.ecx = a[2]; break;
        case TBX_FILE_SEEK: {
            if (e == 0 || a[2] > TBX_FILE_FROM_END) return TBX_ERR_BADARG;
            uint32_t base = a[2] == 0 ? 0 : a[2] == 1 ? e->file_offset : e->dir_entry.file_size;
            int32_t delta = (int32_t)a[1];
            if (base > 0x7fffffffu) return TBX_ERR_BADARG;
            if (delta < 0) {
                uint32_t mag = 0u-a[1];
                if (mag > base) return TBX_ERR_BADARG;
                f.edx = base-mag;
            } else {
                if (a[1] > 0x7fffffffu-base) return TBX_ERR_BADARG;
                f.edx = base+a[1];
            }
            f.eax = 0x4200u; break;  /* checked absolute position */
        }
        case TBX_FILE_CLOSE: f.eax = 0x3e00u; break;
        default: return TBX_ERR_BADCODE;
        }
    }
    psp_t *saved = int21_get_psp();
    int21_cwd_snapshot_t cwd = int21_cwd_save();
    int21_set_psp(p); int21_cwd_reset();
#ifdef TBX_MUT_FILE_WRITE_NOOP
    if (code == TBX_FILE_WRITE) f.eax = a[2];
    else
#endif
    int21_dispatch(&f);
    int32_t result = (f.eflags & 1u) ? TBX_ERR_DOS(f.eax & 0xffffu) : (int32_t)f.eax;
    if (!(f.eflags & 1u)) {
        if (code == TBX_FILE_OPEN || code == TBX_FILE_CREATE) {
            result = (int32_t)(f.eax & 0xffffu);
            e = sft_from_handle(p,(uint8_t)result);
            if (e == 0 || e->kind != SFT_KIND_FILE) {
                f.ebx = (uint32_t)result; f.eax = 0x3e00u;
                int21_dispatch(&f); result = TBX_ERR_BADARG;
            }
        } else if (code == TBX_FILE_CLOSE || code == TBX_FILE_DELETE) result = TBX_OK;
    }
    int21_set_psp(saved); int21_cwd_restore(&cwd);
    return result;
}
static uint32_t tbxf_live(const psp_t *p)
{
    uint32_t n = 0;
    for (uint8_t h = 5; h < JFT_MAX_ENTRIES; h++)
        if (p->jft[h] != JFT_CLOSED) n++;
    return n;
}
static void tbxf_reap(psp_t *p)
{
#ifndef TBX_MUT_FILE_LEAK
    sft_close_process(p);
#else
    (void)p;
#endif
}
#endif
