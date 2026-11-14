/* initech-uqah: R2.2 menu inventory, spec/i123/menu_ri_offsets.tsv. */
#ifndef I123_MENU_H
#define I123_MENU_H
#include <stdint.h>
/* Version 2 resource: I123, u16 version/string-bytes/string-count/item-count;
 * packed NUL strings, u16 offsets, items (four u16), levels (two u16), then
 * little-endian FNV-1a of all preceding bytes. Counts describe the shipped
 * sub-tree. action is the original RI sibling ID, independent of position. */
typedef struct { uint16_t name, desc, child, action; } i123_menu_item_t;
typedef struct { uint16_t first, count; } i123_menu_level_t;
extern i123_menu_item_t i123_menu_items[];
extern i123_menu_level_t i123_menu_levels[];
void i123_menu_init(void);
int i123_menu_load(const uint8_t *data, uint32_t len);
uint32_t i123_menu_export(uint8_t *data, uint32_t cap);
const char *i123_menu_string(uint16_t id);
uint32_t i123_menu_count(void);
enum { I123_ROOT_WORKSHEET=0,I123_ROOT_RANGE=1,I123_ROOT_COPY=2,
       I123_ROOT_MOVE=3,I123_ROOT_FILE=4,I123_ROOT_DATA=7,I123_ROOT_QUIT=10 };
#ifdef I123_MENU_HOST
extern const char *const i123_menu_paths[];
#endif
#endif
