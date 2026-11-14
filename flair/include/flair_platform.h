/* Host services required by the Toolbox. No kernel implementation types.
 * DOS register layout: IBM DOS 3.30 INT 21h, flat form of sdk/toolbox_gate.h.
 * Loader result values preserve legacy loader_status_t for serial diagnostics.
 * See NOTES-FOR-KERNEL.md for binding and lifetime requirements. */
#ifndef FLAIR_PLATFORM_H
#define FLAIR_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
typedef struct {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t ds, es, fs, gs, vector, err_code, eip, cs, eflags;
} flair_platform_regs;
_Static_assert(sizeof(flair_platform_regs) == 68, "INT 81h frame size");
_Static_assert(offsetof(flair_platform_regs, eax) == 28, "INT 81h AX slot");
/* Caller-owned storage for a private file session; the binding may use an
 * out-of-line handle. Never expose the loaded program's writable PSP here. */
typedef union { uint32_t align; uint8_t opaque[256]; } flair_platform_files;
typedef struct {
    uint32_t offset, size, slot;
    uint16_t directory;
    uint8_t mode, drive;
} flair_platform_file_info;
typedef struct {
    uint32_t psp_addr, load_base, module_len, entry, block_len;
} flair_platform_plan;
typedef void *(*flair_platform_carve)(uint32_t len, void *user);
enum { FLAIR_PLATFORM_LOAD_OK = 0, FLAIR_PLATFORM_LOAD_NOMEM = 13 };
enum { FLAIR_PLATFORM_HANDLES = 20 };
enum { FLAIR_PLATFORM_READ = 0, FLAIR_PLATFORM_WRITE = 1, FLAIR_PLATFORM_READ_WRITE = 2 };
void flair_platform_files_init(flair_platform_files *files);
/* Bind once when FLAIR mounts its data volume, before launching tenants. */
void flair_platform_bind_files(unsigned drive);
void flair_platform_files_close(flair_platform_files *files);
/* Return 1 only for a live disk-file handle owned by this session.
 * offset/size are bytes; mode uses the constants above. Never expose devices. */
int flair_platform_file_info_get(flair_platform_files *files, uint8_t handle,
                                  flair_platform_file_info *info);
uint32_t flair_platform_files_live(const flair_platform_files *files);
int flair_platform_path_is_device(const char *path);
/* With files != NULL: select the bound data volume and root CWDs, dispatch,
 * restore caller PSP, all drive CWDs, default and I/O drives even on error.
 * Explicit mounted-drive paths are supported from that drive's root.
 * NULL: system date/time service. */
void flair_platform_dos(flair_platform_files *files, flair_platform_regs *regs);
uint32_t flair_platform_swap_dta(uint32_t address);
int flair_platform_memory(uint32_t *total, uint32_t *free_paras, uint32_t *largest);
int flair_platform_load(const char *name83, uint16_t dir, flair_platform_carve carve,
                         void *user, void **block, flair_platform_plan *plan);
int flair_platform_tenant_call(uint32_t fn, uint32_t psp);
_Noreturn void flair_platform_tenant_abort(void);
void flair_platform_desktop_pattern(unsigned pattern);
void flair_platform_speaker(unsigned hz); /* zero turns channel 2 off */
void flair_platform_serial(const char *text);
_Noreturn void flair_platform_panic(const char *why);
#endif
