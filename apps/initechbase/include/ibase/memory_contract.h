/* App policy over kernel/include/initechmz_extended.h's native loader contract.
 * Keep a module-sized runtime tail for overlays, BSS and the first PAL region.
 * The linker checks its actual extent AND the complete high-memory footprint
 * against memory_map.h's guard; this is independent of conventional DOS memory.
 * Minimum heap is usable bytes, excluding the native 16-byte block header. */
#ifndef INITECH_IBASE_MEMORY_CONTRACT_H
#define INITECH_IBASE_MEMORY_CONTRACT_H
#include "initechmz_extended.h"
#define IBASE_MODULE_MAX INITECHMZ_EXT_MODULE_MAX
#define IBASE_LOADER_RESERVE INITECHMZ_EXT_MODULE_MAX
#define IBASE_MINALLOC_PARAS 2058u
#define IBASE_MINIMUM_HEAP_BYTES 32912u
#define IBASE_PAL_REGION1_BYTES 3952u
#endif
