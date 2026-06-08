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

/* ------------------------------------------------------------------------ *
 * Boot Sector / BIOS Parameter Block (FAT12/16)
 *
 *   Source: Microsoft FAT File System Specification (1.03 / 2000-11-06),
 *           Sections 3 (BPB) and 4 (Extended BPB); empirically confirmed
 *           against mformat 4.0.43 output on a 1.44 MB image (Law 1:
 *           docs/research/fat12-ground-truth.md Section 1 + Section 6).
 *   ADR-0003 DEC-07 (Sec 5.7): "The on-volume layout shall be: boot sector;
 *           first File Allocation Table; second (redundant) File Allocation
 *           Table; fixed-size root directory; data area." The BPB is the
 *           structure named in ADR-0003 Sec 1.4 (glossary) -- locked here as
 *           data per CLAUDE.md Rule 8 (beads initech-8e7).
 *   Note: the jump instruction and OEM name at offsets 0x00-0x0A are NOT
 *         part of the BPB proper; they precede it and are included here
 *         for struct completeness. The BPB proper begins at 0x0B.
 *   Extended BPB (offset 0x24+) is present when boot_sig == 0x29.
 *   All multi-byte fields are little-endian (Intel byte order).
 *
 *   Offset arithmetic (proven; sums to 62 = 0x3E):
 *     0x00 jmp[3] 0x03 oem_name[8] 0x0B bytes_per_sector(2)
 *     0x0D sectors_per_cluster(1) 0x0E reserved_sectors(2) 0x10 num_fats(1)
 *     0x11 root_entry_count(2) 0x13 total_sectors_16(2) 0x15 media_desc(1)
 *     0x16 sectors_per_fat(2) 0x18 sectors_per_track(2) 0x1A num_heads(2)
 *     0x1C hidden_sectors(4) 0x20 total_sectors_32(4) 0x24 drive_number(1)
 *     0x25 reserved1(1) 0x26 boot_sig(1) 0x27 volume_id(4)
 *     0x2B volume_label[11] 0x36 fs_type[8] -> ends 0x3E = 62.
 * ------------------------------------------------------------------------ */

#pragma pack(push, 1)
typedef struct bpb {
	/* Boot sector prefix (not BPB proper) */
	uint8_t  jmp[3];              /* 0x00: Jump instruction (EB xx 90 or E9 xx xx) */
	uint8_t  oem_name[8];         /* 0x03: OEM name string (e.g. "MTOO4043")       */

	/* BPB proper -- offset 0x0B */
	uint16_t bytes_per_sector;    /* 0x0B: Bytes per sector (512 for floppy)        */
	uint8_t  sectors_per_cluster; /* 0x0D: Sectors per cluster (1 for 1.44 MB)      */
	uint16_t reserved_sectors;    /* 0x0E: Reserved sector count (1 = boot sector)  */
	uint8_t  num_fats;            /* 0x10: Number of FATs (2 = redundant copy)      */
	uint16_t root_entry_count;    /* 0x11: Root directory entry count (224)         */
	uint16_t total_sectors_16;    /* 0x13: Total sectors (2880 for 1.44 MB; 0=FAT32)*/
	uint8_t  media_descriptor;    /* 0x15: Media type (0xF0 = 1.44 MB floppy)       */
	uint16_t sectors_per_fat;     /* 0x16: Sectors per FAT (9 for 1.44 MB)          */
	uint16_t sectors_per_track;   /* 0x18: Sectors per track (18 for 1.44 MB)       */
	uint16_t num_heads;           /* 0x1A: Number of heads (2 for 1.44 MB)          */
	uint32_t hidden_sectors;      /* 0x1C: Hidden sectors (0 for non-partitioned)   */
	uint32_t total_sectors_32;    /* 0x20: Total sectors if total_sectors_16==0     */

	/* Extended BPB (FAT12/FAT16 only) -- offset 0x24 */
	uint8_t  drive_number;        /* 0x24: BIOS drive number (0x00=floppy)          */
	uint8_t  reserved1;           /* 0x25: Reserved (0x00)                          */
	uint8_t  boot_sig;            /* 0x26: Extended boot signature (0x29 = present) */
	uint32_t volume_id;           /* 0x27: Volume serial number (random)            */
	uint8_t  volume_label[11];    /* 0x2B: Volume label (space-padded)              */
	uint8_t  fs_type[8];          /* 0x36: FS type string ("FAT12   " / "FAT16   ") */
} bpb_t;
#pragma pack(pop)

_Static_assert(sizeof(bpb_t) == 62, "bpb_t must be 62 bytes (BPB prefix + BPB + extended BPB; "
               "see docs/research/fat12-ground-truth.md)");

/* Derived geometry macros (ADR-0003 DEC-07; formula verified empirically
 * against the 1.44 MB image in docs/research/fat12-ground-truth.md Sec 2). */
#define BPB_FAT1_SECTOR(b)       ((b)->reserved_sectors)
#define BPB_ROOT_DIR_SECTOR(b)   ((b)->reserved_sectors + (b)->num_fats * (b)->sectors_per_fat)
#define BPB_ROOT_DIR_SECTORS(b)  (((b)->root_entry_count * 32u + (b)->bytes_per_sector - 1u) \
                                   / (b)->bytes_per_sector)
#define BPB_FIRST_DATA_SECTOR(b) (BPB_ROOT_DIR_SECTOR(b) + BPB_ROOT_DIR_SECTORS(b))
#define BPB_CLUSTER_LBA(b, n)    (BPB_FIRST_DATA_SECTOR(b) + ((n) - 2u) * (b)->sectors_per_cluster)

#endif /* INITECH_SPEC_DOS_STRUCTS_H */
