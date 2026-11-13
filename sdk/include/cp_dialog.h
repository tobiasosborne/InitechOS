/* Deliberate Rule-8 tenant ABI extension, tdnl.19. Transport to the ONE Dialog
 * Manager. Items content-local, copied to kernel stack, maximum 24. Text must
 * be ASCIZ in loaded module; no pointers retained. ev NULL draws, otherwise
 * DialogHandleEvent returns a 1-based item. No second tracking/control path. */
#ifndef INITECH_CP_DIALOG_H
#define INITECH_CP_DIALOG_H
#include <stdint.h>
#define CP_MAX_ITEMS 24u
#define TBX_DIALOG 0x0092u
#define TBX_SETTINGS 0x0093u
#define TBX_CLOCK 0x0094u
#define TBX_MEMORY 0x0095u
#define TBX_BEEP 0x0096u
#define TBX_ARGC_DIALOG 6u
#define TBX_ARGC_SETTINGS 2u
#define TBX_ARGC_CLOCK 2u
#define TBX_ARGC_MEMORY 1u
#define TBX_ARGC_BEEP 0u
/* SETTINGS(mode,record): mode0 encode current, mode1 validate/apply (no write).
 * CLOCK(mode,time): mode0 read, mode1 set date+time through DOS seam.
 * MEMORY(out[4]): total K, system K (total minus live MCB free), largest K,
 * disk cache K (0: no block cache is reserved). BEEP(): selected speaker tone.
 * DIALOG(win,items,n,default,cancel,event) as described above. */
typedef struct cp_item {
 int16_t left,top,right,bottom;
 uint16_t type; /* 0 statText, 1 pushButton, 2 checkBox, 3 radioButton */
 int16_t value;
 const char *text;
} cp_item_t;
#endif /* flat ABI */
