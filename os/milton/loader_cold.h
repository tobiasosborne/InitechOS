/* DEC-03a.7, initech-jarzu; ADR-0014 generated COLD export/import seam. */
#ifndef INITECH_LOADER_COLD_H
#define INITECH_LOADER_COLD_H
#include "loader.h"
#include "mz.h"
loader_status_t loader_ovl_relocs(loader_ovl_read_fn,void *,const mz_image_t *,uint8_t *,uint32_t,uint8_t *,int);
loader_status_t loader_high_plan(const mz_image_t *,const char *,uint32_t,loader_plan_t *);
loader_status_t loader_prepare_core(const uint8_t *,uint32_t,const char *,uint32_t,int,loader_plan_t *);
#if !defined(__STDC_HOSTED__) || !__STDC_HOSTED__
void ldr_serial_puts(const char *);
__attribute__((noreturn)) void loader_panic_foreign_mz(const char *);
#endif
#endif
