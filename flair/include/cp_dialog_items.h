/* Public flat-item Dialog Manager adapter, from legacy-final:spec/cp_dialog.h.
 * The SDK owns cp_item_t and the gate transport; FLAIR owns Dialog records.
 * Host clients link the owner's host-dialog library for control/dialog code. */
#ifndef INITECH_FLAIR_CP_DIALOG_ITEMS_H
#define INITECH_FLAIR_CP_DIALOG_ITEMS_H
#include "dialog.h"
#include "cp_dialog.h"
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
