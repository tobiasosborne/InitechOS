/* LOCKED: ADR-0009 Amendment DEC-03a (RATIFIED 2026-11-06), initech-ayj36.
 * DEC-03a.3/.4/.8: emitter, PAL, linker and size oracle share this contract.
 * Minimum heap is USABLE bytes, excluding the native 16-byte block header.
 * The old 93776-byte budget and 4096-byte headroom quota are retired (.8). */
#ifndef INITECH_SAMIR_MEMORY_CONTRACT_H
#define INITECH_SAMIR_MEMORY_CONTRACT_H
#include "initechmz_extended.h"
#define SAMIR_MODULE_MAX INITECHMZ_EXT_MODULE_MAX
#define SAMIR_LOADER_RESERVE 65536u
#define SAMIR_MINALLOC_PARAS 2058u
#define SAMIR_MINIMUM_HEAP_BYTES 32912u
#define SAMIR_PAL_REGION1_BYTES 3952u
#endif
