/* Round 8 part 2, initech-hztg. Compare compiler policies, not generated
 * expectations: default i386 layout versus ABI global-data alignment.
 * Ref: ADR-0003 records; locked DOS/event/graphics headers and their assertions.
 * Factory compile-only probe; never linked into a kernel or tenant. */
#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"
#include "psp.h"
#include "idt.h"
#include "loader.h"
#include "kbd.h"
#include "event.h"
#include "find_data.h"
#include "window_record.h"
#include "control_record.h"
#include "menu_record.h"
#define TYPE(t) sizeof(t), _Alignof(t)
const uint32_t layout[] __attribute__((section(".layout"), aligned(1))) = {
    TYPE(boot_info_t), TYPE(psp_t), TYPE(dir_entry_t), TYPE(mcb_t),
    TYPE(exec_param_block_t), TYPE(dpb_t), TYPE(bpb_t), TYPE(psp_params_t),
    TYPE(idt_gate_t), TYPE(idt_ptr_t), TYPE(int_frame_t), TYPE(loader_plan_t),
    TYPE(tenant_plan_t), TYPE(kbd_ring_t), TYPE(EventRecord),
    TYPE(flair_raw_event_t), TYPE(flair_raw_ring_t), TYPE(flair_point_t),
    TYPE(GrafPort), TYPE(QDProcs), TYPE(rgn_rect_t), TYPE(region_t)
};
const uint8_t explicit_alignment[3]
    __attribute__((section(".explicit_alignment"), aligned(16))) = {1,2,3};
