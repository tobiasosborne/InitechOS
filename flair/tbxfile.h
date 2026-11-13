/* Toolbox file adapter; initech-tdnl.91. Authored, no local reference for ABI.
 * Ref: spec/toolbox_gate.h Sec 10/10b; docs/design/toolbox-file-verbs.md.
 * Shared artifact logic compiled verbatim by the host oracle. Kernel-owned
 * shadow PSP only: never read the image PSP's JFT, including at death. */
#ifndef INITECH_TBXFILE_H
#define INITECH_TBXFILE_H
#include "toolbox_gate.h"
#include "flair_platform.h"
static void tbxf_init(flair_platform_files *p)
{
    flair_platform_files_init(p);
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
static int32_t tbxf_call(flair_platform_files *p, uint32_t lo, uint32_t len,
                         uint32_t code, const uint32_t *a)
{
    flair_platform_regs f = { 0 };
    flair_platform_file_info info, *e = 0;
    uint32_t h = a[0];
    /* Sec 10b: CREATE_NEW is CREATE's checks over AH=5Bh (never truncates). */
    int creat = code == TBX_FILE_CREATE || code == TBX_FILE_CREATE_NEW;
    if (creat || code == TBX_FILE_OPEN || code == TBX_FILE_DELETE ||
        code == TBX_FILE_RENAME) {
        if (!tbxf_path(lo,len,a[0])) return TBX_ERR_BADARG;
        /* Sec 10b (initech-mcep): RENAME is AH=56h, the new name in EDI (the
         * flat form of ES:DI, int21.c do_rename); it refuses an existing
         * target exactly as DOS 3.3 does (Tech Ref 6-206; AX=5). */
        if (code == TBX_FILE_RENAME && !tbxf_path(lo,len,a[1])) return TBX_ERR_BADARG;
#ifndef TBX_MUT_FILE_RENAME_EDI
        f.edi = a[1];   /* MUTANT (test-tbx-file-mutant F5) drops the new name */
#endif
#ifndef TBX_MUT_FILE_OWNER
        /* Ref: DOS OPEN's device precedence, int21.c dev_open_lookup.
         * Authored file-only gate policy: never CREATE an unreadable disk file
         * whose basename OPEN resolves as a character device. */
        if (creat && flair_platform_path_is_device((const char *)(uintptr_t)a[0]))
            return TBX_ERR_BADARG;
#endif
        if (code == TBX_FILE_OPEN && a[1] > TBX_FILE_READ_WRITE) return TBX_ERR_BADARG;
        f.edx = a[0];
        f.eax = code == TBX_FILE_CREATE ? 0x3c00u :
#ifndef TBX_MUT_FILE_CREATE_NEW_TRUNC
                 code == TBX_FILE_CREATE_NEW ? 0x5b00u :
#else
                 /* MUTANT (test-tbx-file-mutant F6): CREATE_NEW truncates. */
                 code == TBX_FILE_CREATE_NEW ? 0x3c00u :
#endif
                 code == TBX_FILE_DELETE ? 0x4100u :
                 code == TBX_FILE_RENAME ? 0x5600u : 0x3d00u+a[1];
    } else {
#ifndef TBX_MUT_FILE_OWNER
        if (h < 5 || h >= FLAIR_PLATFORM_HANDLES) return TBX_ERR_BADARG;
        e = flair_platform_file_info_get(p,(uint8_t)h,&info) ? &info : 0;
        if (e == 0) return TBX_ERR_BADARG;
#else
        if (h < FLAIR_PLATFORM_HANDLES) e = flair_platform_file_info_get(p,(uint8_t)h,&info) ? &info : 0;
#endif
        f.ebx = h;
        switch (code) {
        case TBX_FILE_READ:
        case TBX_FILE_WRITE:
            if (e == 0 || (code == TBX_FILE_READ && e->mode == FLAIR_PLATFORM_WRITE) ||
                (code == TBX_FILE_WRITE && e->mode == FLAIR_PLATFORM_READ) ||
                a[2] > 0x7fffffffu || !tbxf_span(lo,len,a[1],a[2])) return TBX_ERR_BADARG;
            f.eax = code == TBX_FILE_READ ? 0x3f00u : 0x4000u;
            f.edx = a[1]; f.ecx = a[2]; break;
        case TBX_FILE_SEEK: {
            if (e == 0 || a[2] > TBX_FILE_FROM_END) return TBX_ERR_BADARG;
            uint32_t base = a[2] == 0 ? 0 : a[2] == 1 ? e->offset : e->size;
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
#ifdef TBX_MUT_FILE_WRITE_NOOP
    if (code == TBX_FILE_WRITE) f.eax = a[2];
    else
#endif
    flair_platform_dos(p,&f);
    int32_t result = (f.eflags & 1u) ? TBX_ERR_DOS(f.eax & 0xffffu) : (int32_t)f.eax;
    if (!(f.eflags & 1u)) {
        if (code == TBX_FILE_OPEN || creat) {
            result = (int32_t)(f.eax & 0xffffu);
            e = flair_platform_file_info_get(p,(uint8_t)result,&info) ? &info : 0;
            if (e == 0) {
                f.ebx = (uint32_t)result; f.eax = 0x3e00u;
                flair_platform_dos(p,&f); result = TBX_ERR_BADARG;
            }
        } else if (code == TBX_FILE_CLOSE || code == TBX_FILE_DELETE ||
                   code == TBX_FILE_RENAME) result = TBX_OK;
    }
    return result;
}
static uint32_t tbxf_live(const flair_platform_files *p)
{
    return flair_platform_files_live(p);
}
static void tbxf_reap(flair_platform_files *p)
{
#ifndef TBX_MUT_FILE_LEAK
    flair_platform_files_close(p);
#else
    (void)p;
#endif
}
#endif
