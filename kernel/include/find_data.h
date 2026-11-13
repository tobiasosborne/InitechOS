/*
 * spec/find_data.h -- InitechDOS FINDFIRST/FINDNEXT Disk Transfer Area layout.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8): the 43-byte find-data block INT 21h
 * AH=4Eh (FINDFIRST) / AH=4Fh (FINDNEXT) write into the current DTA is a
 * contract a DIR-style program reads at fixed field offsets. It is data, not a
 * magic struct buried in a .c file. Changing any field offset/size is a
 * deliberate act (Rule 8): it ripples into every program that walks a find-data
 * block and into the int21.c FINDFIRST/FINDNEXT writer + its oracle.
 *
 * Source / Law 1 citations (all local):
 *   DOS 3.3 Programmer's Reference Manual, INT 21h AH=4Eh (FINDFIRST): the DTA
 *     find-data layout (drive/search-attr + 11-byte search template internal
 *     use; then the found entry's attribute, packed time, packed date, 32-bit
 *     size, and the formatted ASCIIZ 8.3 name).
 *   docs/research/fs-mount-sft-ground-truth.md Sec 4.5 (the 43-byte table;
 *     corrected by dww to the real-DOS offsets reproduced here).
 *   spec/dos_structs.h (dir_entry_t: the on-disk 8.3 fields this is rendered
 *     from; mtime/mdate are copied through unchanged -- DOS packed format).
 *
 * Field offsets match REAL DOS 3.3 (the dww fix). The found-entry fields sit
 * AFTER the 21-byte (0x15) region DOS reserves for FINDNEXT's internal resume
 * state: attr@0x15, time@0x16, date@0x18, size@0x1A, name@0x1E. Total = 43
 * bytes, asserted at compile time via _Static_assert + per-field offsetof
 * (C11) -- the byte-level oracle bites the build if the layout drifts. FACTORY
 * contract data (C); read by the artifact int21.c FINDFIRST/FINDNEXT writer and
 * by any DTA-walking program (dir_program.asm / irqstorm_program.asm @ 0x1E).
 *
 * NOTE (Law 1 reconciliation, dww): the older simplified layout (attr@0x0C ...
 * name@0x15) came from fs-mount-sft-ground-truth.md Sec 4.5, which encoded a
 * compressed table byte-INCOMPATIBLE with real DOS; the brief Sec 4.5 has been
 * corrected to match this header, so the two local sources now agree.
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */
#ifndef INITECH_SPEC_FIND_DATA_H
#define INITECH_SPEC_FIND_DATA_H

#include <stdint.h>
#include <stddef.h>   /* offsetof -- the real-DOS byte-offset oracle (dww) */

/* The DTA find-data block is exactly 43 bytes (DOS 3.3 PRM AH=4Eh). */
#define FIND_DATA_SIZE 43u

/* The DTA defaults to PSP:0x80 (the command-tail field doubles as the default
 * Disk Transfer Area; ADR-0003 App B.2 / spec/dos_structs.h psp_t.cmd_tail).
 * AH=1Ah SETDTA moves it; AH=2Fh GETDTA returns it. */
#define DTA_DEFAULT_PSP_OFFSET 0x80u

#pragma pack(push, 1)
typedef struct find_data {
    /* 0x00..0x14 -- the 21-byte region DOS reserves for FINDNEXT's internal
     * resume state. A real-DOS program never reads it; it must be present so the
     * found-entry fields land at their real-DOS offsets and the 43-byte DTA is
     * fully covered (no out-of-struct write). (DOS 3.3 PRM AH=4Eh.) */
    uint8_t  drive_attr;     /* 0x00: drive letter (bits 0-6) + remote bit       */
    uint8_t  pattern[11];    /* 0x01: 11-byte 8.3 search template (no dot)       */
    uint8_t  search_attr;    /* 0x0C: search attribute mask from FINDFIRST ECX   */
    uint16_t dir_entry;      /* 0x0D: directory entry index (internal resume)    */
    uint16_t parent_cluster; /* 0x0F: start cluster of the parent directory      */
    uint8_t  reserved1[4];   /* 0x11: reserved (internal)                        */
    /* 0x15..0x2A -- the found-entry fields a DIR-style program reads. */
    uint8_t  attr;           /* 0x15: attribute of the found entry               */
    uint16_t ftime;          /* 0x16: time of last modification (DOS packed)     */
    uint16_t fdate;          /* 0x18: date of last modification (DOS packed)     */
    uint32_t fsize;          /* 0x1A: file size in bytes                         */
    char     fname[13];      /* 0x1E: formatted 8.3 name, NUL-terminated (12+NUL)*/
} find_data_t;
#pragma pack(pop)

/* Size: the found name ends at 0x1E + 13 = 0x2B = 43 bytes, exactly the DOS
 * find-record (DOS 3.3 PRM AH=4Eh). No trailing pad is needed -- the reserved
 * bytes live in the 0x00..0x14 internal region, where real DOS keeps them. */
_Static_assert(sizeof(find_data_t) == FIND_DATA_SIZE,
               "find_data_t must be 43 bytes (DOS 3.3 Programmer's Reference "
               "Manual AH=4Eh)");

/* Real-DOS byte-offset anchor (the dww fix; Law 5 differential). These are the
 * offsets a real-DOS program -- or a differential test vs real DOS FINDFIRST --
 * reads. Perturbing the layout breaks the build here (the oracle bites at
 * compile time; mutation-proof). Ref: DOS 3.3 PRM AH=4Eh. */
_Static_assert(offsetof(find_data_t, pattern)     == 0x01, "search template @ 0x01");
_Static_assert(offsetof(find_data_t, search_attr) == 0x0C, "search attribute @ 0x0C");
_Static_assert(offsetof(find_data_t, attr)        == 0x15, "found attribute @ 0x15");
_Static_assert(offsetof(find_data_t, ftime)       == 0x16, "found time @ 0x16");
_Static_assert(offsetof(find_data_t, fdate)       == 0x18, "found date @ 0x18");
_Static_assert(offsetof(find_data_t, fsize)       == 0x1A, "found size @ 0x1A");
_Static_assert(offsetof(find_data_t, fname)       == 0x1E, "found name @ 0x1E");

#endif /* INITECH_SPEC_FIND_DATA_H */
