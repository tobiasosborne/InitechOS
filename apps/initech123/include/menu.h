/* initech-uqah: R2.2 menu inventory, spec/i123/menu_ri_offsets.tsv. */
#ifndef I123_MENU_H
#define I123_MENU_H
#include <stdint.h>
typedef struct { uint16_t name, desc, child; } i123_menu_item_t;
typedef struct { uint16_t first, count; } i123_menu_level_t;
extern i123_menu_item_t i123_menu_items[];
extern i123_menu_level_t i123_menu_levels[];
void i123_menu_init(void);
int i123_menu_load(const uint8_t *data, uint32_t len);
uint32_t i123_menu_export(uint8_t *data, uint32_t cap);
const char *i123_menu_string(uint16_t id);
uint32_t i123_menu_count(void);
#ifdef I123_MENU_HOST
extern const char *const i123_menu_paths[];
#endif
#endif
