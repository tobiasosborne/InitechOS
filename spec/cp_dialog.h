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
#if defined(INITECH_OS_FLAIR_DIALOG_H) && !defined(INITECH_CP_DIALOG_ITEMS)
#define INITECH_CP_DIALOG_ITEMS
static inline void cp_dialog_items(DialogRecord *d,DialogItem *items,
    ControlRecord *controls,const cp_item_t *flat,unsigned n,int x,int y) {
    d->items=items; d->itemCount=(uint16_t)n;
    for(unsigned i=0;i<n;i++) {
        const cp_item_t *p=&flat[i];
        rgn_rect_t r={(int16_t)(p->top+y),(int16_t)(p->left+x),
                     (int16_t)(p->bottom+y),(int16_t)(p->right+x)};
        items[i]=(DialogItem){p->type?ctrlItem:statText,r,p->text,0,(uint8_t)(p->type!=0),{0,0,0}};
        if(p->type) {control_init(&controls[i],(flair_ctrl_type_t)(p->type-1),r,p->value,0,1,1,p->text);items[i].ctrl=&controls[i];}
    }
}
#endif
