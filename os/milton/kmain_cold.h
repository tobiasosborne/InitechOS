/* Cold shell formatting, split from kmain. Ref: PRD Sec 6.5; About dialog
 * contract retained from kmain.c. Freestanding, no allocation or host state. */
#ifndef INITECH_KMAIN_COLD_H
#define INITECH_KMAIN_COLD_H
#include <stdint.h>
__attribute__((cold,noinline))
void kmain_about_lines(char memory[48],char unused[48],uint32_t mem_k,
                       uint32_t largest_paras,int bound);
#endif
