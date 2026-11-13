/* Fail-loud binding for services outside the routing suite's scope. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flair_platform.h"
static _Noreturn void unused_service(void) { fputs("FAIL: unused platform service reached\n",stderr); abort(); }
void flair_platform_files_init(flair_platform_files *p) { memset(p,0,sizeof *p); }
void flair_platform_files_close(flair_platform_files *p) { (void)p; }
uint32_t flair_platform_files_live(const flair_platform_files *p) { (void)p; return 0; }
int flair_platform_file_info_get(flair_platform_files *p,uint8_t h,flair_platform_file_info *i) { (void)p;(void)h;(void)i; unused_service(); }
int flair_platform_path_is_device(const char *p) { (void)p; unused_service(); }
void flair_platform_dos(flair_platform_files *p,flair_platform_regs *r) { (void)p;(void)r; unused_service(); }
uint32_t flair_platform_swap_dta(uint32_t d) { (void)d; unused_service(); }
int flair_platform_memory(uint32_t *t,uint32_t *f,uint32_t *l) { (void)t;(void)f;(void)l; unused_service(); }
void flair_platform_desktop_pattern(unsigned p) { (void)p; unused_service(); }
void flair_platform_speaker(unsigned hz) { (void)hz; unused_service(); }
_Noreturn void flair_platform_panic(const char *why) { fprintf(stderr,"PANIC tbx: %s\nHALTED\n",why); abort(); }
void flair_platform_serial(const char *s) { fputs(s,stderr); }
