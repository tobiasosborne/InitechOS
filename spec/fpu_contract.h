/*
 * spec/fpu_contract.h -- the locked x87 boot control word (beads initech-zj6w).
 *
 * LOCKED spec-data (CLAUDE.md Rule 8). The C-header companion of the
 * spec/hardware.json "fpu" row: the kernel's boot-path x87 bring-up
 * (os/milton/fpu.h fpu_init, called from sysinit_early) loads THIS value with
 * FLDCW after FNINIT, and test-hardware-spec cross-checks it against the JSON
 * "boot_control_word" field so the two can never drift silently.
 *
 * DELIBERATE-ACT NOTE (Rule 8): introduced by beads initech-zj6w / initech-yjlo
 * (operator ruling 2026-10-03: hardware x87 for Initech 123; SAMIR stays
 * soft-float, ADR-0009 DEC-01 unchanged). Changing this value is a deliberate
 * act with an issue + worklog note.
 *
 * VALUE: 0x037F == the x87 control word FNINIT/FINIT establishes (Intel SDM
 * Vol 1 Sec 8.1.5 "x87 FPU Control Word": after FINIT the control word is
 * 037FH -- all six exceptions masked (bits 0-5), PC = 11B (64-bit
 * significand, double-extended), RC = 00B (round to nearest), bit 6 reserved
 * set). The kernel deliberately does NOT choose 53-bit precision: precision
 * control is an APPLICATION policy (Initech 123 sets its own with FLDCW), the
 * kernel only guarantees a known, deterministic starting state (Rule 11).
 *
 * Why the kernel must load it at all (measured, beads initech-zj6w RED run):
 * the post-RESET x87 control word is 0040H on Bochs (all exceptions UNMASKED,
 * Intel SDM Vol 3A Table 9-1 "Processor States Following Power-up, Reset, or
 * INIT"), while QEMU/SeaBIOS hands over 037FH. Without the kernel's FNINIT the
 * same x87 sequence produced 63 on QEMU and 0 on Bochs.
 *
 * Citation note (Law 1): no local copy of the Intel SDM exists under docs/ or
 * spec/; the section numbers above are recorded for a reviewer to check
 * against the SDM and are NOT backed by a local file.
 *
 * ASCII-clean (Rule 12). Header-only, no code (shared by kernel + host oracle).
 */
#ifndef INITECH_SPEC_FPU_CONTRACT_H
#define INITECH_SPEC_FPU_CONTRACT_H

/* The x87 control word the kernel leaves loaded after boot (FNINIT value). */
#define X87_BOOT_CONTROL_WORD 0x037Fu

#endif /* INITECH_SPEC_FPU_CONTRACT_H */
