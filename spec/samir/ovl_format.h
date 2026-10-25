/*
 * spec/samir/ovl_format.h -- the SAMIR.OVL overlay container and the in-slot
 * overlay header (InitechBase overlays Phase B, epic initech-5etom).
 *
 * LOCKED spec-data (CLAUDE.md Rule 8). Changing a value here is a deliberate
 * act with an issue + worklog note; the producer (tools/samir_ovl_pack.c), the
 * consumer (os/samir/core/ovl.c) and the host oracle (harness/diff/dbf_diff/
 * test_samir_ovl.c) all include THIS header, and the oracle pins every
 * offset with literal values so a silent edit here goes RED there.
 *
 * Ground truth (Law 1; docs/design/samir-overlays.md Sec 2):
 *   - dBASE III PLUS 1.1 ships its runtime as DBASE.EXE (133,632 B, MZ) beside
 *     one raw (non-MZ) overlay file DBASE.OVL (266,240 B) that holds many
 *     overlay segments, each carrying its own fixup list
 *     (../dbase3-decomp/re/ghidra-plan.md Sec 0.2; docs/research/
 *     overlays-period-ground-truth.md Sec 1.1/1.2). Every OVL is a multiple of
 *     512 B (same research Sec 1.1) -> SAMIR_OVL_ALIGN.
 *   - DBASE.EXE reads DBASE.OVL itself: open AH=3Dh (file offset 0x204B4),
 *     LSEEK AH=42h to a paragraph-scaled position (0x204FC), READ AH=3Fh
 *     (0x20553), next to the "DBASE.OVL" string (0x20010); it contains no
 *     MOV AX,4B03h (byte scan, design note Sec 2). So a multi-segment overlay
 *     file is read by the program's own manager, not by EXEC AL=03h (which
 *     loads one whole file, IBM DOS 3.30 TR p. 6-196/6-197).
 *   - An overlay belongs to exactly one release of its executable (research
 *     Sec 1.1/4) -> SAMIR_OVL_HDR_STAMP, checked against the stamp the packer
 *     patches into the resident image.
 *   - Missing overlay: DBASE.EXE carries the string at file offset 0x186E0
 *     "\r        Insert System Disk 2 and press ENTER, or press Ctrl-C to
 *     abort.      $" (printed with AH=09h). SAMIR_OVL_PROMPT is that string with
 *     the disk number dropped (InitechBase ships one System diskette,
 *     pubs/manuals/initechbase-volume-one/g1-installing.tex). What dBASE does
 *     at run time without its OVL is NOT minted (research gap G-1): this text
 *     is InitechBase's own refusal, modelled on that string, and claims nothing
 *     about dBASE's behaviour.
 *
 * Integers little-endian. ASCII-clean (Rule 12). No timestamps (Rule 11).
 */
#ifndef SAMIR_OVL_FORMAT_H
#define SAMIR_OVL_FORMAT_H

/* ---- The file: SAMIR.OVL ------------------------------------------------- *
 *
 *   [0x00, 0x20)              file header (below)
 *   [0x20, 0x20 + 16*count)   directory: one 16-byte entry per overlay
 *   then, per overlay i, at dir[i].off:
 *       dir[i].len bytes      the overlay module (linked at the slot address,
 *                             base-relative: the resident load base B is added
 *                             to each relocated dword, exactly as the
 *                             InitechMZ loader does for the resident module,
 *                             ADR-0003 DEC-08a.1)
 *       dir[i].nrel * 4 bytes the module-relative u32 relocation offsets
 *   zero padding to a multiple of SAMIR_OVL_ALIGN.
 */
#define SAMIR_OVL_FILE_NAME      "SAMIR.OVL"
#define SAMIR_OVL_ALIGN          512u

#define SAMIR_OVL_HDR_SIZE       0x20u
#define SAMIR_OVL_HDR_SIG_OFF    0x00u   /* 8 bytes, SAMIR_OVL_SIG            */
#define SAMIR_OVL_HDR_VER_OFF    0x08u   /* u16, SAMIR_OVL_VERSION            */
#define SAMIR_OVL_HDR_COUNT_OFF  0x0Au   /* u16, number of overlays           */
#define SAMIR_OVL_HDR_STAMP_OFF  0x0Cu   /* u32, CRC-32 of [0x20, file end)   */
#define SAMIR_OVL_HDR_SLOT_OFF   0x10u   /* u32, slot length the resident was
                                            linked with (== largest overlay)  */
#define SAMIR_OVL_HDR_FLEN_OFF   0x14u   /* u32, total file length            */
/* 0x18..0x1F reserved, zero. */

#define SAMIR_OVL_SIG            "SAMIROVL"
#define SAMIR_OVL_SIG_LEN        8u
#define SAMIR_OVL_VERSION        1u

#define SAMIR_OVL_DIR_SIZE       16u
#define SAMIR_OVL_DIR_OFF_OFF    0x00u   /* u32, file offset of the module    */
#define SAMIR_OVL_DIR_LEN_OFF    0x04u   /* u32, module bytes                 */
#define SAMIR_OVL_DIR_NREL_OFF   0x08u   /* u32, relocation count             */
#define SAMIR_OVL_DIR_CRC_OFF    0x0Cu   /* u32, CRC-32 of module + reloc table */

/* At most this many overlays; the directory is read into a fixed buffer. */
#define SAMIR_OVL_MAX            8u

/* CRC-32: the IEEE 802.3 polynomial, reflected (0xEDB88320), init and final
 * XOR 0xFFFFFFFF -- the PKZIP/zlib CRC (check value of "123456789" is
 * 0xCBF43926, asserted by the oracle). */
#define SAMIR_OVL_CRC_POLY       0xEDB88320u
#define SAMIR_OVL_CRC_CHECK      0xCBF43926u

/* ---- The slot: what sits at the first byte of every loaded overlay ------- *
 *
 *   +0  u32 magic  SAMIR_OVL_MAGIC ('S','O','V','L')
 *   +4  u32 id     the overlay's index in the directory
 *   +8  u32 entry  absolute address of the overlay's entry function
 *                  (a relocated dword: base-relative in the file)
 */
#define SAMIR_OVL_MAGIC          0x4C564F53u   /* "SOVL" little-endian */
#define SAMIR_OVL_SLOT_HDR_SIZE  12u

/* ---- The refusal (see the ground-truth note above) ----------------------- *
 * Printed (CR first, as DBASE.EXE's AH=09h string) whenever SAMIR.OVL is
 * missing, unreadable, from another build (stamp/slot/count mismatch) or
 * corrupt (CRC mismatch). ENTER retries, Ctrl-C (or end of input) aborts. */
#define SAMIR_OVL_PROMPT \
    "\r        Insert System Disk and press ENTER, or press Ctrl-C to abort.      "
#define SAMIR_OVL_KEY_RETRY      0x0Du   /* ENTER  */
#define SAMIR_OVL_KEY_ABORT      0x03u   /* Ctrl-C */

#endif /* SAMIR_OVL_FORMAT_H */
