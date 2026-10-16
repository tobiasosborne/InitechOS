/* R4.5 disk DA. Ref: period-gui-suite.md sec 5, cp_dialog.h, record.json. */
#ifndef INITECH_CPANELS_H
#define INITECH_CPANELS_H
#include "cpanels_record.h"
#include "cp_dialog.h"
typedef struct cp_state { cp_settings_t live,edit; cp_time_t time,original_time; unsigned panel,closed,error,clock_tab; uint32_t memory[4]; char text[8][64]; } cp_state_t;
typedef struct cp_ops {
 int (*save)(void *,const cp_settings_t *);
 int (*apply)(void *,const cp_settings_t *);
 int (*clock)(void *,const cp_time_t *);
 void (*beep)(void *);
 void *user;
} cp_ops_t;
void cp_init(cp_state_t *,const cp_settings_t *);
void cp_hit(cp_state_t *,unsigned,const cp_ops_t *);
unsigned cp_items(cp_state_t *,cp_item_t *);
#endif
