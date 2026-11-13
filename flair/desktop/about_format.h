/* Cold shell formatting, split from kmain. Ref: PRD Sec 6.5; About dialog
 * contract retained from kmain.c. Freestanding, no allocation or host state. */
#ifndef INITECH_ABOUT_FORMAT_H
#define INITECH_ABOUT_FORMAT_H
#include <stdint.h>

void about_format_lines(char memory[48],char unused[48],uint32_t mem_k,
                       uint32_t largest_paras,int bound);
#endif
