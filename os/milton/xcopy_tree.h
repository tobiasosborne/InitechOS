/* XCOPY/TREE flat-program seam. Ref: ADR-0003 Appendix A, DEC-08;
 * spec/int21h_calling_convention.json. uintptr_t permits the host oracle to
 * marshal pointers without changing the artifact's 32-bit register ABI. */
#ifndef INITECH_DOSUTIL_H
#define INITECH_DOSUTIL_H
#include <stdint.h>
typedef struct du_regs {
    uint32_t a, b, c;
    uintptr_t d, s;
} du_regs;
int du_call(du_regs *r); /* nonzero = carry set */
int du_main(int tree, char *tail);
#endif
