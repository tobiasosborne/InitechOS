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
 *   docs/research/fs-mount-sft-ground-truth.md Sec 4.5 (the 43-byte table +
 *     the exact struct, reproduced here verbatim with the _Static_assert).
 *   spec/dos_structs.h (dir_entry_t: the on-disk 8.3 fields this is rendered
 *     from; mtime/mdate are copied through unchanged -- DOS packed format).
 *
 * Field names and offsets match the brief (Sec 4.5) EXACTLY. Total = 43 bytes,
 * asserted at compile time via _Static_assert (C11). FACTORY contract data (C);
 * read by the artifact int21.c FINDFIRST/FINDNEXT writer.
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11).
 */
#ifndef INITECH_SPEC_FIND_DATA_H
#define INITECH_SPEC_FIND_DATA_H

#include <stdint.h>

/* The DTA find-data block is exactly 43 bytes (DOS 3.3 PRM AH=4Eh). */
#define FIND_DATA_SIZE 43u

/* The DTA defaults to PSP:0x80 (the command-tail field doubles as the default
 * Disk Transfer Area; ADR-0003 App B.2 / spec/dos_structs.h psp_t.cmd_tail).
 * AH=1Ah SETDTA moves it; AH=2Fh GETDTA returns it. */
#define DTA_DEFAULT_PSP_OFFSET 0x80u

#pragma pack(push, 1)
typedef struct find_data {
    uint8_t  drive_attr;    /* 0x00: internal (drive + search attribute)        */
    uint8_t  pattern[11];   /* 0x01: 11-byte 8.3 search template (no dot)       */
    uint8_t  attr;          /* 0x0C: attribute of the found entry               */
    uint16_t ftime;         /* 0x0D: time of last modification (DOS packed)     */
    uint16_t fdate;         /* 0x0F: date of last modification (DOS packed)     */
    uint32_t fsize;         /* 0x11: file size in bytes                         */
    char     fname[13];     /* 0x15: formatted 8.3 name, NUL-terminated (12+NUL)*/
    uint8_t  reserved[9];   /* 0x22: pad to the 43-byte DTA find-record total   */
} find_data_t;
#pragma pack(pop)

/* The found-entry fields end at 0x15 + 13 = 0x22 = 34 bytes; the DOS find-record
 * the DTA reserves is 43 bytes (DOS 3.3 PRM AH=4Eh), so the struct carries 9
 * trailing reserved bytes. A DIR program reads only the named fields at their
 * fixed offsets; the reserved tail is never inspected but must be present so the
 * caller's 43-byte DTA is fully covered (no out-of-struct write). The struct is
 * sized to the full record EXACTLY (Rule 8). */
_Static_assert(sizeof(find_data_t) == FIND_DATA_SIZE,
               "find_data_t must be 43 bytes (DOS 3.3 Programmer's Reference "
               "Manual AH=4Eh; fs-mount-sft-ground-truth.md Sec 4.5)");

#endif /* INITECH_SPEC_FIND_DATA_H */
