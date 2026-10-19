/*
 * spec/tsr_contract.h -- the InitechDOS TERMINATE-AND-STAY-RESIDENT contract
 * (LOCKED SPEC-DATA, CLAUDE.md Rule 8).
 *
 * bead: initech-6fdt sub-deliverable (2): the kernel mechanics a resident
 *       (Initech SideKick) needs on the DEC-08 flat .COM model -- KEEP, the
 *       InDOS byte, the DOS idle interrupt, the keyboard hook chain and the
 *       80x25 cell save-under. This file IS the Rule-8 deliberate act: every
 *       vector, address and size below is fixed HERE and nowhere else.
 *       STATUS: PM-ratified-pending-operator (2026-10-20). The displaced
 *       vectors (Sec 2) are InitechDOS deviations forced by ADR-0003 DEC-04a.2
 *       and are NOT period numbers; an operator ruling may renumber them.
 *
 * Ref (local, Law 1):
 *   IBM 80X0945 DOS 3.30 Technical Reference (Apr 1987):
 *     p. 6-26/6-27  INT 27h Terminate but Stay Resident (DX = last address plus
 *                   one relative to the PSP; restores 22h/23h/24h like INT 20h;
 *                   "Terminate but stay resident programs do not close files";
 *                   31h is the preferred method).
 *     p. 6-27       "28H - 2EH Reserved for DOS" -- the idle interrupt and the
 *                   InDOS address call (34h, "Reserved by DOS", p. 6-35) are
 *                   undocumented in the TR; their behaviour here is the
 *                   InitechDOS contract stated in Sec 3/4, not a TR citation.
 *     pp. 6-105/106 AH=31h Terminate Process and Remain Resident: AL return
 *                   code, DX memory size in paragraphs; "does not free up any
 *                   other allocation blocks belonging to that process. Files
 *                   opened by the process are not closed".
 *   ADR-0003 Amendment DEC-04a.1/.2: IRQ vectors 0x28-0x37 are interrupt gates
 *     (0x8E) and "shall not be used for any software-interrupt gate"; vectors
 *     0x00-0x1F are CPU exceptions. So INT 28h (= IRQ0 here), INT 09h, INT 10h
 *     and INT 16h cannot keep their period numbers.
 *   Borland SideKick 1.5 Owner's Handbook (Mar 1985) p. 13 ("hold down the Ctrl
 *     and Alt keys at the same time") -- the default chord; the scan copy is in
 *     the parallel research lane (docs/research/sidekick/manual/, initech-6fdt).
 */
#ifndef INITECH_SPEC_TSR_CONTRACT_H
#define INITECH_SPEC_TSR_CONTRACT_H

/* ---- Sec 1: KEEP ----------------------------------------------------------
 * The PSP itself (16 paragraphs) always stays resident: an InitechDOS floor,
 * because the resident's PSP holds the JFT whose files TR p. 6-105 says stay
 * open. A DX below it is raised to it. */
#define TSR_KEEP_MIN_PARAS   0x0010u

/* INT 27h, the period vector (free in the DEC-04a 0x22-0x27 band): EDX = the
 * resident BYTE count from the PSP (the flat analogue of "last address plus one
 * relative to the program's initial DS"), exit code 0, then exactly AH=31h. */
#define TSR_VEC_KEEP27       0x27u

/* ---- Sec 2: displaced vectors (ADR-0003 DEC-04a.2 deviation) --------------
 * INT 09h-equivalent: the keyboard IRQ1 vector the kernel already dispatches
 * (PIC_MASTER_BASE + 1). AH=25h on any vector 0x28..0x37 installs a 0x8E
 * interrupt gate (DEC-04a.1), so a hook runs with IF clear like the kernel's
 * own IRQ handlers. A hook MUST chain to the AH=35h value by simulating an
 * interrupt (pushfd / push cs / call [old]); that previous handler reads port
 * 60h exactly once and issues the EOI. A hook MUST NOT issue INT 21h. */
#define TSR_VEC_KBD_IRQ      0x29u
/* INT 28h-equivalent: the DOS idle interrupt. The console input wait of INT 21h
 * functions 01h-0Ch (and so COMMAND.COM's prompt, which reads with AH=0Ah)
 * raises it on every poll; the default handler is a bare IRETD. A resident may
 * issue INT 21h functions above 0Ch from it while InDOS == 1. Trap gate. */
#define TSR_VEC_IDLE         0x38u
/* INT 10h-equivalent console cell service (vector 0x10 is #MF on a 486):
 *   AL=00h SAVE    -- EDX -> TSR_SCREEN_BYTES writable bytes; CF=0
 *   AL=01h RESTORE -- EDX -> a buffer SAVE filled; redraws every cell and
 *                     the cursor; CF=0
 *   bad AL / EDX not a valid user buffer (DEC-14) -> CF=1, AX=0001h/0009h.
 * Trap gate. */
#define TSR_VEC_SCREEN       0x39u

/* ---- Sec 3: the save-under buffer -----------------------------------------
 * 2000 cells, each {character byte, CGA attribute byte} row-major, then the
 * cursor row byte and column byte: the 4000-byte B800h text page of the period
 * plus the cursor, which a pop-up must also put back. */
#define TSR_SCREEN_CELLS     2000u
#define TSR_SCREEN_BYTES     4002u

/* ---- Sec 4: keyboard flag byte ---------------------------------------------
 * The kernel's IRQ1 service mirrors its modifier state into the BIOS data area
 * keyboard flag byte (segment 0040h offset 0017h of the PC BIOS; flat linear
 * 0x417 here, the region boot_info.h already leaves untouched below 0x500).
 * A hook reads it AFTER its chained call returns. */
#define TSR_KBD_FLAGS_LINEAR 0x00000417u
#define TSR_KBD_FLAG_RSHIFT  0x01u
#define TSR_KBD_FLAG_LSHIFT  0x02u
#define TSR_KBD_FLAG_CTRL    0x04u
#define TSR_KBD_FLAG_ALT     0x08u
#define TSR_KBD_FLAG_CAPS    0x40u
/* SideKick 1.5's default chord: Ctrl and Alt held together. */
#define TSR_CHORD_DEFAULT    (TSR_KBD_FLAG_CTRL | TSR_KBD_FLAG_ALT)

#endif /* INITECH_SPEC_TSR_CONTRACT_H */
