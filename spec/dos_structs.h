/*
 * spec/dos_structs.h -- InitechDOS on-disk and in-memory structure layouts.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8). Transcribed VERBATIM from ADR-0003
 * Appendix B (On-Disk and In-Memory Structure Layouts).
 *
 * Source: docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md, Appendix B:
 *   B.1 Directory Entry (32 bytes)            -> dir_entry_t
 *   B.2 Program Segment Prefix (256 bytes)    -> psp_t   (selected offsets)
 *   B.3 Memory Control Block / Arena (16 bytes) -> mcb_t
 *
 * DEC-05 (Sec 5.5): the PSP is constructed in accordance with Appendix B.2,
 * including the INT 20h at 00h, env-segment pointer at 2Ch, command-tail and
 * default DTA at 80h, and the two default FCBs. DEC-07 (Sec 5.7): directory
 * entries conform to Appendix B.1 (8.3 name, attribute byte, cluster-chain).
 * DEC-03 (Sec 5.3): the MCB arena header bears the 'M'/'Z' signature, an
 * owner identifier, and a size in 16-byte paragraphs.
 *
 * Field names and offsets match the ADR EXACTLY. Sizes are asserted at
 * compile time via _Static_assert (C11). This header is FACTORY contract
 * data (C); the artifact (Pascal) is verified against it.
 */
#ifndef INITECH_SPEC_DOS_STRUCTS_H
#define INITECH_SPEC_DOS_STRUCTS_H

#include <stdint.h>

/* ------------------------------------------------------------------------ *
 * B.1 Directory Entry (32 bytes)
 *
 *   Offset Size Field
 *   00h    8    Filename (space-padded, upper-case)
 *   08h    3    Extension (space-padded, upper-case)
 *   0Bh    1    Attribute byte (RO 01h / Hidden 02h / System 04h /
 *               VolLabel 08h / Directory 10h / Archive 20h)
 *   0Ch    10   Reserved
 *   16h    2    Time of last modification (two-second resolution)
 *   18h    2    Date of last modification
 *   1Ah    2    Starting cluster
 *   1Ch    4    File size in bytes
 *
 *   First byte 00h denotes an unused entry (end of directory);
 *   E5h denotes a deleted entry.
 * ------------------------------------------------------------------------ */

/* Attribute byte bit values (ADR-0003 Appendix B.1). */
#define DIR_ATTR_READONLY   0x01u /* RO       */
#define DIR_ATTR_HIDDEN     0x02u /* Hidden   */
#define DIR_ATTR_SYSTEM     0x04u /* System   */
#define DIR_ATTR_VOLLABEL   0x08u /* VolLabel */
#define DIR_ATTR_DIRECTORY  0x10u /* Directory*/
#define DIR_ATTR_ARCHIVE    0x20u /* Archive  */

/* Special first-byte sentinels (ADR-0003 Appendix B.1). */
#define DIR_NAME_FREE       0x00u /* unused entry / end of directory */
#define DIR_NAME_DELETED    0xE5u /* deleted entry                   */

#pragma pack(push, 1)
typedef struct dir_entry {
	uint8_t  filename[8];      /* 00h: Filename (space-padded, upper-case)   */
	uint8_t  extension[3];     /* 08h: Extension (space-padded, upper-case)  */
	uint8_t  attribute;        /* 0Bh: Attribute byte                        */
	uint8_t  reserved[10];     /* 0Ch: Reserved                              */
	uint16_t mtime;            /* 16h: Time of last modification (2s res)    */
	uint16_t mdate;            /* 18h: Date of last modification             */
	uint16_t start_cluster;    /* 1Ah: Starting cluster                      */
	uint32_t file_size;        /* 1Ch: File size in bytes                    */
} dir_entry_t;
#pragma pack(pop)

_Static_assert(sizeof(dir_entry_t) == 32, "dir_entry_t must be 32 bytes (ADR-0003 B.1)");

/* ------------------------------------------------------------------------ *
 * B.2 Program Segment Prefix (256 bytes; selected offsets)
 *
 *   Offset Field
 *   00h    INT 20h instruction (legacy termination)
 *   02h    Segment of first byte beyond allocated memory
 *   0Ah-15h Saved INT 22h / 23h / 24h vectors
 *   16h    Parent Program Segment Prefix
 *   18h-2Bh Job File Table (20 handle entries)
 *   2Ch    Environment-block segment
 *   50h    INT 21h / far-return entry
 *   5Ch / 6Ch Default File Control Blocks (#1, #2)
 *   80h    Command-tail length and text; also default Disk Transfer Area
 *
 * The ADR gives SELECTED offsets; unspecified gaps are explicit reserved
 * fields so the total is exactly 256 bytes (DEC-05, Sec 5.5).
 * ------------------------------------------------------------------------ */
#pragma pack(push, 1)
typedef struct psp {
	uint8_t  int20[2];          /* 00h: INT 20h instruction (legacy term.)   */
	uint16_t alloc_end_seg;     /* 02h: Segment of first byte beyond mem      */
	uint8_t  reserved_04[6];    /* 04h..09h: (unspecified gap)                */
	uint8_t  saved_vectors[12]; /* 0Ah-15h: Saved INT 22h/23h/24h vectors     */
	uint16_t parent_psp;        /* 16h: Parent Program Segment Prefix         */
	uint8_t  jft[20];           /* 18h-2Bh: Job File Table (20 handle entries)*/
	uint16_t env_seg;           /* 2Ch: Environment-block segment             */
	uint8_t  reserved_2e[34];   /* 2Eh..4Fh: (unspecified gap)                */
	uint8_t  int21_entry[12];   /* 50h: INT 21h / far-return entry            */
	uint8_t  reserved_5c[16];   /* 5Ch: Default FCB #1                        */
	uint8_t  reserved_6c[20];   /* 6Ch: Default FCB #2                        */
	uint8_t  cmd_tail[128];     /* 80h: Command-tail len+text; default DTA    */
} psp_t;
#pragma pack(pop)

_Static_assert(sizeof(psp_t) == 256, "psp_t must be 256 bytes (ADR-0003 B.2)");

/* ------------------------------------------------------------------------ *
 * B.3 Memory Control Block / Arena Header (16 bytes)
 *
 *   Offset Size Field
 *   00h    1    Signature: 'M' (non-terminal) or 'Z' (terminal)
 *   01h    2    Owner identifier (0 = free)
 *   03h    2    Block size in 16-byte paragraphs
 *   05h    11   Reserved
 * ------------------------------------------------------------------------ */

/* Arena signature bytes (ADR-0003 Appendix B.3 / DEC-03). */
#define MCB_SIG_NONTERMINAL 'M' /* non-terminal block */
#define MCB_SIG_TERMINAL    'Z' /* terminal block     */

#pragma pack(push, 1)
typedef struct mcb {
	uint8_t  signature;    /* 00h: 'M' (non-terminal) or 'Z' (terminal)   */
	uint16_t owner;        /* 01h: Owner identifier (0 = free)            */
	uint16_t size_paras;   /* 03h: Block size in 16-byte paragraphs       */
	uint8_t  reserved[11]; /* 05h: Reserved                               */
} mcb_t;
#pragma pack(pop)

_Static_assert(sizeof(mcb_t) == 16, "mcb_t must be 16 bytes (ADR-0003 B.3)");

#endif /* INITECH_SPEC_DOS_STRUCTS_H */
