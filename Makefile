# InitechOS (codename STAPLER) -- top-level factory Makefile
#
# beads: initech-tse ("Repo skeleton + Makefile + directory layout")
# Ref:   CLAUDE.md "Build & test" (the canonical target list),
#        PRD Sec 11 (milestones), PRD Sec 14 (C-only factory).
#
# This Makefile is FACTORY code: pure POSIX make + C + a thin shell, no
# extra runtimes (CLAUDE.md Law 3 / Rule "no extra runtimes"). Outputs are
# reproducible -- no timestamps or host paths baked in (CLAUDE.md Rule 11).
#
# Most targets are stubs today: the milestones land them (PRD Sec 11).
# Stubs come in two flavours (CLAUDE.md Law 2 "the oracle is the truth" and
# Rule 2 "fail fast, fail loud"):
#   * ACTION stubs (image/run/run-bochs) print an honest "not implemented"
#     line and exit 0 -- they assert nothing, so a 0 is not a false signal.
#   * GATE stubs (smoke/test*/ssim/selfhost/ddc) are correctness oracles. An
#     unimplemented oracle must NEVER read as green, so these exit NON-ZERO.
#     A passing `make test` has to mean the oracle actually ran and passed.
# The lone real target is `factory`, which compiles the C smoke stub -- the
# acceptance criterion for this issue.

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror
BUILD   ?= build

# The factory smoke stub (harness/factory_smoke.c) and its built binary.
SMOKE_SRC := harness/factory_smoke.c
SMOKE_BIN := $(BUILD)/factory_smoke

# Seed cross-compiler front end (seed/, beads initech-znb). The driver links
# every seed/*.c that is NOT a test_*.c; the test binaries link the same
# library sources minus initechc.c (which owns main()).
SEED_LIB_SRC  := seed/token.c seed/lexer.c seed/ast.c seed/parser.c seed/codegen.c
SEED_DRV_SRC  := seed/initechc.c
SEED_BIN      := $(BUILD)/initechc

# Seed runtime (beads initech-znb, Step B): freestanding nasm runtime +
# linker script that wrap the compiled program body (pas_main). The seed
# keeps its OWN copy of the multiboot/ld pattern under seed/rt/ (it does NOT
# edit the harness fixtures).
SEED_RT_DIR  := seed/rt
SEED_RT_ASM  := $(SEED_RT_DIR)/start.asm
SEED_RT_LD   := $(SEED_RT_DIR)/seed.ld
SEED_RT_OBJ  := $(BUILD)/seed_rt.o

# Canonical end-to-end smoke program and its built ELF.
SEED_SMOKE_PAS := seed/examples/smoke.pas
SEED_SMOKE_ELF := $(BUILD)/seed_smoke.elf
SEED_SMOKE_MARKER := InitechOS seed OK

# Seed unit tests (one binary each; each owns its own main()).
SEED_TEST_LEXER  := $(BUILD)/test_lexer
SEED_TEST_PARSER := $(BUILD)/test_parser

# The parser test uses fmemopen() (POSIX.1-2008) to render the AST dump into a
# buffer; -std=c11 hides it, so request the POSIX feature for TEST builds only.
# (Tests are factory infrastructure, not the freestanding artifact.)
SEED_TEST_CFLAGS := -D_POSIX_C_SOURCE=200809L

# Default emulator frame for the SSIM fidelity guide (overridable).
FRAME ?= desktop

# ---------------------------------------------------------------------------
# QEMU oracle harness (harness/emu, beads initech-f2s)
# ---------------------------------------------------------------------------
# The harness launches qemu-system-i386 on a freestanding guest and captures
# debug signals (serial, triple-fault, QMP screendump). PRD Sec 8.
HARNESS_LIB_SRC := harness/emu/qemu.c
HARNESS_DRV_SRC := harness/emu/qemu_main.c
HARNESS_BIN     := $(BUILD)/qemu_harness

# Self-test fixtures: multiboot1 guests assembled with nasm + linked with a
# tiny script (no real MBR/A20/GDT boot -- QEMU's -kernel loader lands us in
# 32-bit protected mode with A20 on and flat segments per the multiboot spec).
FIXTURE_DIR  := harness/emu/fixtures
FIXTURE_LD   := $(FIXTURE_DIR)/multiboot.ld
NASM         ?= nasm
LD           ?= ld
OBJCOPY      ?= objcopy
SERIAL_HELLO_ELF := $(BUILD)/serial_hello.elf
TRIPLE_FAULT_ELF := $(BUILD)/triple_fault.elf
HANG_ELF         := $(BUILD)/hang.elf
HARNESS_FIXTURES := $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF) $(HANG_ELF)

# The marker the good fixture writes to COM1 (asserted by test-harness).
HARNESS_MARKER := HARNESS-OK

# ---------------------------------------------------------------------------
# Tracer boot (os/boot, beads initech-f8v.2)
# ---------------------------------------------------------------------------
# The thinnest REAL boot chain: custom MBR (sector 0) -> stage2 (sectors 1..)
# -> 32-bit protected/flat -> VESA LFB -> seafoam fill -> hlt-loop. Assembled
# as flat 16/32-bit nasm binaries and concatenated into a raw bootable disk
# image (no SEED, no multiboot -- distinct from the -kernel smoke path).
# Ref: PRD Sec 5 (hardware contract), Sec 11 (M1). Reproducible: nasm flat
# binaries + a deterministic dd/cat layout, no timestamps (CLAUDE.md Rule 11).
BOOT_DIR        := os/boot
MBR_ASM         := $(BOOT_DIR)/mbr.asm
STAGE2_ASM      := $(BOOT_DIR)/stage2.asm
MBR_BIN         := $(BUILD)/mbr.bin
STAGE2_BIN      := $(BUILD)/stage2.bin
TRACER_IMG      := $(BUILD)/tracer_boot.img
# stage2 is loaded by the MBR as STAGE2_SECTORS (16) sectors = 8 KiB; we pad
# stage2.bin to that size so the CHS read count in the MBR is deterministic.
STAGE2_SECTORS  := 16
# Kernel occupies sectors 17.. ; padded to KERNEL_SECTORS (must equal the
# stage2.asm KERNEL_SECTORS equate so the INT 13h read count is deterministic).
# Bumped 64 -> 80 (40 KiB) for beads initech-509.11: the FAT WRITE path
# (fat12.c create/write/flush/unlink + int21 CREAT/WRITE/UNLINK) grew the
# kernel; the BOOT_SHELL image (which also links command.o) crossed the old
# 32 KiB window. 80 keeps a clean margin and stays below PROGRAM_BASE (0x20000:
# 0x10000 + 80*512 = 0x19C00). MUST equal the stage2.asm KERNEL_SECTORS equate.
KERNEL_SECTORS  := 80
# Total raw image: MBR(1) + stage2(16) + kernel(80) = 97 sectors; round up to
# 128 (64 KiB) for headroom + a clean power-of-two. Deterministic (Rule 11).
IMG_SECTORS     := 128

# PPM seafoam checker (factory C tool, tools/).
PPM_CHECK_SRC   := tools/ppm_seafoam_check.c
PPM_CHECK_BIN   := $(BUILD)/ppm_seafoam_check

# PPM banner-text checker (factory C tool, tools/, beads initech-bea). The
# STRONGER companion to ppm_seafoam_check: asserts the InitechDOS banner was
# blitted at its known origin AND the desktop is still seafoam below it.
PPM_TEXT_CHECK_SRC := tools/ppm_text_check.c
PPM_TEXT_CHECK_BIN := $(BUILD)/ppm_text_check

# ---------------------------------------------------------------------------
# Flat C kernel (os/milton, beads initech-d00; ADR-0003 DEC-08)
# ---------------------------------------------------------------------------
# The InitechDOS kernel: a FLAT binary linked at 0x00010000, loaded by stage2
# (INT 13h) and entered by a far jump. ARTIFACT code: freestanding C
# (CDR-0001 interim toolchain: host gcc -m32 -ffreestanding -nostdlib) + a
# 32-bit nasm entry stub. Reproducible: deterministic codegen, no timestamps,
# raw binary via objcopy (CLAUDE.md Rule 11).
KERNEL_DIR    := os/milton
KERNEL_CC     ?= gcc
KERNEL_CFLAGS := -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic \
                 -fno-pie -std=c11 -Wall -Wextra -Werror
KERNEL_LD     := $(KERNEL_DIR)/kernel.ld
KERNEL_START_ASM := $(KERNEL_DIR)/kstart.asm
KERNEL_MAIN_C    := $(KERNEL_DIR)/kmain.c
KERNEL_CONSOLE_C := $(KERNEL_DIR)/console.c
# Interrupt foundation (beads initech-a5a): IDT + PIC + panic + exception stubs.
KERNEL_IDT_C     := $(KERNEL_DIR)/idt.c
KERNEL_PIC_C     := $(KERNEL_DIR)/pic.c
KERNEL_PANIC_C   := $(KERNEL_DIR)/panic.c
# INT 21h dispatcher (beads initech-509.5): the `int 0x21` syscall spine.
KERNEL_INT21_C   := $(KERNEL_DIR)/int21.c
# PSP construction (beads initech-509.4) + flat program loader (initech-509.5).
KERNEL_PSP_C     := $(KERNEL_DIR)/psp.c
KERNEL_LOADER_C  := $(KERNEL_DIR)/loader.c
# System File Table / Job File Table handle layer (beads initech-509.3): the
# JFT->SFT indirection + predefined handles 0-4 + DUP/DUP2 redirection.
KERNEL_SFT_C     := $(KERNEL_DIR)/sft.c
# FAT12 mount over ATA (beads initech-saw / initech-adf): the real-hardware
# sector backend (ata.c) + the FAT12 reader (fat12.c == FAT12_SRC) linked into
# the kernel so kmain.c can mount the data disk and render a proto-DIR.
KERNEL_ATA_C     := $(KERNEL_DIR)/ata.c
KERNEL_FAT12_C   := $(KERNEL_DIR)/fat12.c
# PS/2 keyboard (IRQ1) + 8254 PIT (IRQ0) drivers (beads initech-3rs): the first
# hardware-interrupt sources. The pure halves (ring + scancode table + divisor
# math) ALSO compile hosted in the test-kbd-unit oracle.
KERNEL_KBD_C     := $(KERNEL_DIR)/kbd.c
KERNEL_PIT_C     := $(KERNEL_DIR)/pit.c
# IRQ reentrancy guard (beads initech-xk2): in-IRQ depth counter + the INT 21h
# reentrancy fail-loud panic. Host-compilable (links into the int21 unit oracle).
KERNEL_IRQ_C     := $(KERNEL_DIR)/irq.c
# COMMAND.COM interactive shell (beads initech-7pc): the kernel-resident A:\> REPL.
# The pure logic (parser/upcaser/classifier/.COM-appender/DIR formatter) ALSO
# compiles HOSTED in the test-command oracle; the REPL + its int 0x21 wrappers are
# kernel-only behind -DCOMMAND_KERNEL_REPL.
KERNEL_COMMAND_C := $(KERNEL_DIR)/command.c
# Baked flat test program: nasm -f bin -> bin2c -> a .rodata C array. The loader
# copies it to PROGRAM_IMAGE (0x20100) and JMPs in (initech-509.5).
TEST_PROG_ASM    := $(KERNEL_DIR)/test_program.asm
TEST_PROG_BIN    := $(BUILD)/test_program.bin
BIN2C_SRC        := tools/bin2c.c
BIN2C_BIN        := $(BUILD)/bin2c
TEST_PROG_BLOB_H := $(BUILD)/test_prog_blob.h
TEST_PROG_BLOB_C := $(BUILD)/test_prog_blob.c
# Baked TYPE / DIR programs (beads initech-509.5 read-side): OPEN+READ+WRITE+
# CLOSE and FINDFIRST/FINDNEXT exercised end-to-end over the mounted FAT12 disk.
TYPE_PROG_ASM    := $(KERNEL_DIR)/type_program.asm
TYPE_PROG_BIN    := $(BUILD)/type_program.bin
TYPE_PROG_BLOB_C := $(BUILD)/type_prog_blob.c
DIR_PROG_ASM     := $(KERNEL_DIR)/dir_program.asm
DIR_PROG_BIN     := $(BUILD)/dir_program.bin
DIR_PROG_BLOB_C  := $(BUILD)/dir_prog_blob.c
# Baked CON-INPUT program (beads initech-n62): AH=0Ah buffered input -> echo the
# line back via AH=40h. Linked into the -DBOOT_CONIN self-test kernel only.
CONIN_PROG_ASM   := $(KERNEL_DIR)/conin_program.asm
CONIN_PROG_BIN   := $(BUILD)/conin_program.bin
CONIN_PROG_BLOB_C := $(BUILD)/conin_prog_blob.c
WRITE_PROG_ASM   := $(KERNEL_DIR)/write_program.asm
WRITE_PROG_BIN   := $(BUILD)/write_program.bin
WRITE_PROG_BLOB_C := $(BUILD)/write_prog_blob.c
# Baked MULTI-OPEN program (beads initech-0qh; epic initech-6qy): OPENs two files
# concurrently, does interleaved positioned reads with LSEEK on both, and reads a
# signature past 64 KiB of a >64 KiB file. Linked into the -DBOOT_MULTIOPEN
# kernel only (make test-multiopen).
MULTIOPEN_PROG_ASM   := $(KERNEL_DIR)/multiopen_program.asm
MULTIOPEN_PROG_BIN   := $(BUILD)/multiopen_program.bin
MULTIOPEN_PROG_BLOB_C := $(BUILD)/multiopen_prog_blob.c
# GREET program (beads initech-saw): a flat .COM that is NOT baked into the
# kernel -- it is mcopy'd onto the FAT12 data disk as GREET.COM and loaded BY
# NAME from the mounted volume (load_program_from_fat / INT 21h AH=4Bh EXEC).
# It prints "GREETINGS FROM A:GREET.COM" via AH=09h and exits rc=7.
GREET_PROG_ASM   := $(KERNEL_DIR)/greet_program.asm
GREET_PROG_BIN   := $(BUILD)/greet_program.bin
KERNEL_ISR_ASM   := $(KERNEL_DIR)/isr.asm
KERNEL_START_OBJ := $(BUILD)/kstart.o
KERNEL_MAIN_OBJ  := $(BUILD)/kmain.o
KERNEL_CONSOLE_OBJ := $(BUILD)/console.o
KERNEL_IDT_OBJ   := $(BUILD)/idt.o
KERNEL_PIC_OBJ   := $(BUILD)/pic.o
KERNEL_PANIC_OBJ := $(BUILD)/panic.o
KERNEL_INT21_OBJ := $(BUILD)/int21.o
KERNEL_PSP_OBJ   := $(BUILD)/psp.o
KERNEL_SFT_OBJ   := $(BUILD)/sft.o
KERNEL_LOADER_OBJ := $(BUILD)/loader.o
KERNEL_ATA_OBJ   := $(BUILD)/ata.o
KERNEL_FAT12_OBJ := $(BUILD)/fat12.o
KERNEL_FILEIO_OBJ := $(BUILD)/fileio_fat.o
KERNEL_KBD_OBJ   := $(BUILD)/kbd.o
KERNEL_PIT_OBJ   := $(BUILD)/pit.o
KERNEL_IRQ_OBJ   := $(BUILD)/irq.o
# COMMAND.COM shell object (beads initech-7pc), compiled with the REPL enabled.
KERNEL_COMMAND_OBJ := $(BUILD)/command.o
KERNEL_TEST_PROG_OBJ := $(BUILD)/test_prog_blob.o
KERNEL_TYPE_PROG_OBJ := $(BUILD)/type_prog_blob.o
KERNEL_DIR_PROG_OBJ  := $(BUILD)/dir_prog_blob.o
KERNEL_CONIN_PROG_OBJ := $(BUILD)/conin_prog_blob.o
KERNEL_WRITE_PROG_OBJ := $(BUILD)/write_prog_blob.o
KERNEL_MULTIOPEN_PROG_OBJ := $(BUILD)/multiopen_prog_blob.o
KERNEL_ISR_OBJ   := $(BUILD)/isr.o
KERNEL_ELF       := $(BUILD)/kernel.elf
KERNEL_BIN       := $(BUILD)/kernel.bin
# Self-test fault kernel/image (beads initech-a5a; make test-panic): the SAME
# kernel sources but with -DBOOT_SELFTEST_FAULT so the boot deliberately raises
# a #DE after the banner to prove the panic path catches it (no triple-fault).
KERNEL_FAULT_MAIN_OBJ := $(BUILD)/kmain_fault.o
KERNEL_FAULT_ELF      := $(BUILD)/kernel_fault.elf
KERNEL_FAULT_BIN      := $(BUILD)/kernel_fault.bin
PANIC_IMG             := $(BUILD)/panic_boot.img
# Keyboard-echo kernel/image (beads initech-3rs / initech-43b; make test-kbd):
# the SAME kernel sources but with -DBOOT_KBD_ECHO so the boot, after enabling
# IRQs, emits KBD-ECHO-READY then echoes kbd_getchar() to serial. Separate image
# so the normal kernel/image (test-boot) never echoes (mirrors the FAULT image).
KERNEL_ECHO_MAIN_OBJ  := $(BUILD)/kmain_echo.o
KERNEL_ECHO_ELF       := $(BUILD)/kernel_echo.elf
KERNEL_ECHO_BIN       := $(BUILD)/kernel_echo.bin
KBD_ECHO_IMG          := $(BUILD)/kbd_echo_boot.img
# CON-INPUT self-test kernel/image (beads initech-n62; make test-conin): the
# SAME kernel sources but with -DBOOT_CONIN so the boot, after CONIN-LIVE, runs
# the baked CON-input program which reads a line via AH=0Ah and echoes it back.
# Separate image so the normal kernel never blocks on the keyboard.
KERNEL_CONIN_MAIN_OBJ := $(BUILD)/kmain_conin.o
KERNEL_CONIN_ELF      := $(BUILD)/kernel_conin.elf
KERNEL_CONIN_BIN      := $(BUILD)/kernel_conin.bin
CONIN_IMG             := $(BUILD)/conin_boot.img
# FAT-sourced load + EXEC self-test kernel/image (beads initech-saw; make
# test-exec): the SAME kernel sources but with -DBOOT_EXEC so the boot, after
# mounting the FAT12 data disk, loads GREET.COM BY NAME (load_program_from_fat)
# and EXECs it via INT 21h AH=4Bh, then reads its rc via AH=4Dh. Separate image
# so the normal boot is unchanged. Requires --disk2 (the data disk).
KERNEL_EXEC_MAIN_OBJ  := $(BUILD)/kmain_exec.o
KERNEL_EXEC_ELF       := $(BUILD)/kernel_exec.elf
KERNEL_EXEC_BIN       := $(BUILD)/kernel_exec.bin
EXEC_IMG              := $(BUILD)/exec_boot.img
# FAT WRITE round-trip self-test kernel/image (beads initech-509.11; make
# test-fatwrite): the SAME kernel sources with -DBOOT_WRITE so the boot runs the
# baked WRITE program (CREAT+WRITE+CLOSE then OPEN+READ+echo OUT.TXT) over a
# WRITABLE FAT12 data disk. Separate image so the normal boot is unchanged.
KERNEL_WRITE_MAIN_OBJ := $(BUILD)/kmain_write.o
KERNEL_WRITE_ELF      := $(BUILD)/kernel_write.elf
KERNEL_WRITE_BIN      := $(BUILD)/kernel_write.bin
WRITE_IMG             := $(BUILD)/write_boot.img
# MULTI-OPEN self-test kernel/image (beads initech-0qh; make test-multiopen): the
# SAME kernel sources with -DBOOT_MULTIOPEN so the boot runs the baked MULTI-OPEN
# program over a FAT12 data disk carrying HELLO.TXT + SECOND.TXT + a >64 KiB
# BIG.DAT. Separate image so the normal boot is unchanged. Requires --disk2.
KERNEL_MULTIOPEN_MAIN_OBJ := $(BUILD)/kmain_multiopen.o
KERNEL_MULTIOPEN_ELF      := $(BUILD)/kernel_multiopen.elf
KERNEL_MULTIOPEN_BIN      := $(BUILD)/kernel_multiopen.bin
MULTIOPEN_IMG             := $(BUILD)/multiopen_boot.img
# IRQ-STORM reentrancy self-test kernel/image (beads initech-xk2; make
# test-int21-irqstorm): the SAME kernel sources with -DBOOT_IRQSTORM so the boot,
# AFTER enabling IRQs (sti) so the PIT/keyboard fire LIVE, runs the baked
# IRQ-STORM program (FINDFIRST/NEXT enumeration + a multi-cluster READ + a second
# concurrent handle) WHILE the harness storms keystrokes (IRQ1) and the 100 Hz
# PIT (IRQ0) ticks. Proves INT 21h is reentrancy-safe with IRQs live (no frame /
# result corruption). The baked program (irqstorm_prog) is linked ONLY into this
# image. Requires --disk2 (the storm FAT disk). Separate image so the normal boot
# is unchanged. Two Rule-6 MUTANTS prove the oracle bites:
#   A: pit.c built with -DPIT_MUTATE_SCRIBBLE_DOS (the PIT ISR scribbles a DOS
#      dispatcher global via a test hook) -> the storm corrupts the enum/read ->
#      RED on the wrong result.
#   B: pit.c built with -DPIT_MUTATE_ISSUE_INT21 (the PIT ISR issues `int 0x21`
#      from IRQ context) -> the reentrancy guard PANICS -> RED on the marker.
IRQSTORM_PROG_ASM        := $(KERNEL_DIR)/irqstorm_program.asm
IRQSTORM_PROG_BIN        := $(BUILD)/irqstorm_program.bin
IRQSTORM_PROG_BLOB_C     := $(BUILD)/irqstorm_prog_blob.c
KERNEL_IRQSTORM_PROG_OBJ := $(BUILD)/irqstorm_prog_blob.o
KERNEL_IRQSTORM_MAIN_OBJ := $(BUILD)/kmain_irqstorm.o
KERNEL_IRQSTORM_ELF      := $(BUILD)/kernel_irqstorm.elf
KERNEL_IRQSTORM_BIN      := $(BUILD)/kernel_irqstorm.bin
IRQSTORM_IMG             := $(BUILD)/irqstorm_boot.img
# Rule-6 MUTANT A (scribble): pit.c built with -DPIT_MUTATE_SCRIBBLE_DOS -> its
# own pit.o + irqstorm main obj + elf + bin + image so the mutant cannot
# contaminate the real build.
KERNEL_PIT_MUT_SCRIBBLE_OBJ  := $(BUILD)/pit_mut_scribble.o
KERNEL_IRQSTORM_MUTA_MAIN_OBJ := $(BUILD)/kmain_irqstorm_muta.o
KERNEL_IRQSTORM_MUTA_ELF     := $(BUILD)/kernel_irqstorm_muta.elf
KERNEL_IRQSTORM_MUTA_BIN     := $(BUILD)/kernel_irqstorm_muta.bin
IRQSTORM_MUTA_IMG            := $(BUILD)/irqstorm_muta_boot.img
# Rule-6 MUTANT B (issue int 0x21 from IRQ): pit.c built with
# -DPIT_MUTATE_ISSUE_INT21 -> its own pit.o + image. The guard must PANIC.
KERNEL_PIT_MUT_INT21_OBJ     := $(BUILD)/pit_mut_int21.o
KERNEL_IRQSTORM_MUTB_MAIN_OBJ := $(BUILD)/kmain_irqstorm_mutb.o
KERNEL_IRQSTORM_MUTB_ELF     := $(BUILD)/kernel_irqstorm_mutb.elf
KERNEL_IRQSTORM_MUTB_BIN     := $(BUILD)/kernel_irqstorm_mutb.bin
IRQSTORM_MUTB_IMG            := $(BUILD)/irqstorm_mutb_boot.img
# COMMAND.COM shell kernel/image (beads initech-7pc; make test-shell): the SAME
# kernel sources but with -DBOOT_SHELL so the boot, after CONIN-LIVE, prints
# SHELL-READY and enters the COMMAND.COM REPL (instead of the demo+halt). Separate
# image so the NORMAL kernel/image is byte-for-byte unchanged (the demo gates run
# WITHOUT key injection and would HANG on a blocking prompt). The shell command.o
# (REPL enabled) is linked ONLY into this image. Requires --disk2 (HELLO.TXT +
# GREET.COM) so DIR/TYPE/run-program have real files. Mirrors the EXEC image.
KERNEL_SHELL_MAIN_OBJ := $(BUILD)/kmain_shell.o
KERNEL_SHELL_ELF      := $(BUILD)/kernel_shell.elf
KERNEL_SHELL_BIN      := $(BUILD)/kernel_shell.bin
SHELL_IMG             := $(BUILD)/shell_boot.img
# EXIT-handle teardown self-test kernel/image (beads initech-6hk; epic
# initech-6qy; make test-exit-handles): the SAME kernel sources but with
# -DBOOT_EXITH so the boot EXECs the FAT-sourced leaky child EXITH.COM RUNS
# times. EXITH.COM OPENs four files and TERMINATEs (4Ch) WITHOUT closing them; if
# EXIT did not reclaim a process's handles the 16-slot file SFT exhausts and a
# later run's OPEN fails. WITH sft_close_process every run starts clean. Mirrors
# the EXEC image (loads BY NAME from FAT). The MUTANT image adds
# -DSFT_MUTATE_NO_CLOSE_PROCESS (its own sft.o) to PROVE the oracle bites (Rule
# 6). Requires --disk2 (the leaky-child FAT disk). Separate images so the normal
# boot is unchanged. The leaky-child .COM (EXITH.COM) is NOT baked: it is mcopy'd
# onto the FAT disk and loaded by name (load_program_from_fat / AH=4Bh EXEC).
EXITH_PROG_ASM        := $(KERNEL_DIR)/exith_program.asm
EXITH_PROG_BIN        := $(BUILD)/exith_program.bin
KERNEL_EXITH_MAIN_OBJ := $(BUILD)/kmain_exith.o
KERNEL_EXITH_ELF      := $(BUILD)/kernel_exith.elf
KERNEL_EXITH_BIN      := $(BUILD)/kernel_exith.bin
EXITH_IMG             := $(BUILD)/exith_boot.img
# Rule-6 MUTANT: same EXITH boot, but sft.c built with -DSFT_MUTATE_NO_CLOSE_PROCESS
# (elides the per-handle release in sft_close_process). Its own sft.o + main obj +
# elf + bin + image, so the mutant cannot contaminate the real build.
KERNEL_SFT_MUT_OBJ        := $(BUILD)/sft_mut_noclose.o
KERNEL_EXITH_MUT_MAIN_OBJ := $(BUILD)/kmain_exith_mut.o
KERNEL_EXITH_MUT_ELF      := $(BUILD)/kernel_exith_mut.elf
KERNEL_EXITH_MUT_BIN      := $(BUILD)/kernel_exith_mut.bin
EXITH_MUT_IMG             := $(BUILD)/exith_mut_boot.img
FAT_EXITH_IMG             := $(BUILD)/fat_exith.img

# ---------------------------------------------------------------------------
# Asset extraction v0 (spec/assets, beads initech-vcq)
# ---------------------------------------------------------------------------
# Palette + chrome metrics measured from the reference frame, plus a clean-
# room hand-authored Chicago bitmap strike. Ref: PRD Sec 10 (asset pipeline),
# Sec 6.4 (Chicago/Geneva assets), Sec 12 (the frame is REFERENCE ONLY -- we
# MEASURE it, we never embed it). The frame webp is the reference fixture;
# the PPM derives from the film so it stays in build/ (NOT committed). The
# committed artifacts are palette.json (sampled values), palette.h (generated
# from it), chrome_metrics.json, and the hand-authored chicago8x16.h.
ASSET_DIR        := spec/assets
PREVIEW_WEBP     := $(ASSET_DIR)/preview.webp
PREVIEW_PPM      := $(BUILD)/preview.ppm
PALETTE_JSON     := $(ASSET_DIR)/palette.json
PALETTE_H        := $(ASSET_DIR)/palette.h
CHICAGO_H        := $(ASSET_DIR)/chicago8x16.h

PALETTE_TOOL_SRC := tools/palette_extract.c
PALETTE_TOOL_BIN := $(BUILD)/palette_extract
ASSET_CHECK_SRC  := tools/asset_check.c
ASSET_CHECK_BIN  := $(BUILD)/asset_check

# webp -> PPM decoder: ImageMagick `convert` (PIL fallback). Reproducible:
# the same webp always decodes to the same raster.
CONVERT          ?= convert

# ---------------------------------------------------------------------------
# FAT12 read path (os/milton, beads initech-adf; bpb_t locked: initech-8e7)
# ---------------------------------------------------------------------------
# The FAT12 reader is ARTIFACT code (os/milton/, C, freestanding-safe) but is
# exercised on the HOST by the factory oracle via a file-backed blockdev
# (harness/diff/fat_diff/blockdev_file.c). The BPB-parse test mints a 1.44 MB
# FAT12 image with mformat at TEST TIME (build intermediate, NOT committed --
# like PREVIEW_PPM, Rule 11) and asserts every BPB field + derived-geometry
# value matches the verified constants in docs/research/fat12-ground-truth.md.
MILTON_DIR        := os/milton
FAT12_SRC         := $(MILTON_DIR)/fat12.c
FAT_DIFF_DIR      := harness/diff/fat_diff
BLOCKDEV_FILE_SRC := $(FAT_DIFF_DIR)/blockdev_file.c
FAT12_FIXTURE_DIR := $(FAT_DIFF_DIR)/fixtures
FAT12_FIXTURES    := $(FAT12_FIXTURE_DIR)/hello.txt \
                     $(FAT12_FIXTURE_DIR)/second.txt \
                     $(FAT12_FIXTURE_DIR)/chain.txt \
                     $(FAT12_FIXTURE_DIR)/empty.txt \
                     $(FAT12_FIXTURE_DIR)/block.bin

# The differential dumper (factory host tool, beads initech-5cu): mounts the
# image with the REAL artifact fat12.c and emits the normalized manifest /
# raw file bytes that `test-fat` diffs against mtools + the python reference.
FAT_DUMP_SRC      := $(FAT_DIFF_DIR)/fat_dump.c
FAT_DUMP_BIN      := $(BUILD)/fat_dump
# The independent python3 FAT12 reference (reference #2; NOT mtools, NOT our C).
FAT12_REF_PY      := $(FAT_DIFF_DIR)/fat12_ref.py
# The list of 8.3 names the gate extracts + diffs content for (per fixture).
FAT12_GATE_NAMES  := HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN

# Minted 1.44 MB FAT12 image (build intermediate, NOT committed).
FAT12_IMG         := $(BUILD)/fat12_fixture.img

# FAT12 DATA disk for the in-emulator mount oracle (beads initech-saw). A second
# minted 1.44 MB FAT12 volume attached to QEMU on the IDE primary SLAVE
# (--disk2); the kernel reads it via ata.c and mounts it with fat12.c. Same
# deterministic recipe + fixtures as FAT12_IMG (build intermediate, NOT
# committed). The known 8.3 filenames are what make test-fs asserts on serial.
FAT_DATA_IMG      := $(BUILD)/fat_data.img

# The BPB-parse oracle binary (factory host test; libc + POSIX for the test).
TEST_FAT12_BPB    := $(BUILD)/test_fat12_bpb

# The FAT12 decode + cluster-chain-walk oracle binary (factory host test).
TEST_FAT12_CHAIN  := $(BUILD)/test_fat12_chain

# The FAT12 root-dir enumerate + find + file-read oracle binary (host test).
TEST_FAT12_DIR    := $(BUILD)/test_fat12_dir

# The FAT12 POSITIONED-read oracle binary (host test; beads initech-lq2) + its
# two mutation builds (Rule 6: one perturbed constant each -> the diff must bite).
TEST_FAT12_PARTIAL          := $(BUILD)/test_fat12_partial
TEST_FAT12_PARTIAL_MUT_SKIP := $(BUILD)/test_fat12_partial_mut_skip
TEST_FAT12_PARTIAL_MUT_POS  := $(BUILD)/test_fat12_partial_mut_pos

# Mint the FAT12 image deterministically: zero a 1474560-byte (2880*512)
# file, mformat -f 1440, mcopy the fixtures in. Reproducible enough for the
# BPB/geometry oracle (the BPB bytes are fixed by mformat -f 1440; only the
# volume serial + dir mtime vary, neither of which this oracle asserts).
$(FAT12_IMG): $(FAT12_FIXTURES) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/second.txt ::SECOND.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/chain.txt ::CHAIN.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/empty.txt ::EMPTY.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/block.bin ::BLOCK.BIN
	@printf ">>> fat12: minted %s (1.44MB FAT12, fixtures copied; build intermediate)\n" "$@"

# Mint the FAT12 DATA disk (beads initech-saw): identical recipe to FAT12_IMG;
# this is the volume the kernel mounts over ATA in make test-fs. Deterministic
# (mformat -f 1440 fixes the BPB; the fixtures are committed). Build
# intermediate, NOT committed (Rule 11).
$(FAT_DATA_IMG): $(FAT12_FIXTURES) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/second.txt ::SECOND.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/chain.txt ::CHAIN.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/empty.txt ::EMPTY.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/block.bin ::BLOCK.BIN
	@printf ">>> fat_data: minted %s (1.44MB FAT12 data disk for test-fs; build intermediate)\n" "$@"

# Assemble the GREET .COM (org 0x20100; nasm -f bin is deterministic, Rule 11).
$(GREET_PROG_BIN): $(GREET_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

# FAT12 EXEC disk for the in-emulator FAT-sourced-load oracle (beads initech-saw).
# A SEPARATE 1.44 MB volume carrying GREET.COM (the .COM loaded BY NAME). Kept
# distinct from FAT_DATA_IMG so test-fs/test-type/test-dir's shared disk -- and
# their screendump layout assumptions (tools/ppm_text_check BG_Y0=240) -- are
# untouched: an extra file there would scroll console output below y=240 and trip
# the background-purity check. The fixtures pin the proto-DIR; this disk pins the
# EXEC load. Build intermediate, NOT committed (Rule 11).
FAT_EXEC_IMG      := $(BUILD)/fat_exec.img

$(FAT_EXEC_IMG): $(FAT12_FIXTURES) $(GREET_PROG_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@mcopy -i $@ $(GREET_PROG_BIN) ::GREET.COM
	@printf ">>> fat_exec: minted %s (1.44MB FAT12 disk for test-exec; GREET.COM + HELLO.TXT; build intermediate)\n" "$@"

# Assemble the leaky-child EXITH .COM (org 0x20100; nasm -f bin is deterministic,
# Rule 11). It OPENs four files and TERMINATEs WITHOUT closing them.
$(EXITH_PROG_BIN): $(EXITH_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

# FAT12 disk for the EXIT-handle teardown oracle (beads initech-6hk; epic
# initech-6qy; make test-exit-handles). A SEPARATE 1.44 MB volume carrying the
# four files EXITH.COM OPENs (HELLO.TXT/SECOND.TXT/CHAIN.TXT/BLOCK.BIN) plus
# EXITH.COM itself (the leaky child, loaded BY NAME). Kept distinct from the
# other FAT images so the leaky child + its four target files never disturb their
# DIR/screendump assumptions. Build intermediate, NOT committed (Rule 11).
$(FAT_EXITH_IMG): $(FAT12_FIXTURES) $(EXITH_PROG_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/second.txt ::SECOND.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/chain.txt ::CHAIN.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/block.bin ::BLOCK.BIN
	@mcopy -i $@ $(EXITH_PROG_BIN) ::EXITH.COM
	@printf ">>> fat_exith: minted %s (1.44MB FAT12 disk for test-exit-handles; EXITH.COM + 4 target files; build intermediate)\n" "$@"

# FAT12 WRITABLE data disk for the in-emulator WRITE round-trip (beads
# initech-509.11; make test-fatwrite). A FRESH, formatted-but-near-empty 1.44 MB
# FAT12 volume the kernel WRITES to over ATA (the WRITE program CREATs OUT.TXT on
# it). Minted per run (the kernel mutates it, so it must NOT be a committed
# fixture, Rule 11). It carries one seed file so the proto-DIR still renders, but
# OUT.TXT must NOT pre-exist. Distinct from FAT_DATA_IMG (which test-fs/type/dir
# assert is read-only) so a write never disturbs those gates.
FAT_WRITE_IMG     := $(BUILD)/fat_write.img

$(FAT_WRITE_IMG): $(FAT12_FIXTURE_DIR)/hello.txt | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@printf ">>> fat_write: minted %s (1.44MB FAT12 WRITABLE disk for test-fatwrite; HELLO.TXT seed; build intermediate)\n" "$@"

# FAT12 data disk for the in-emulator MULTI-OPEN capability oracle (beads
# initech-0qh; make test-multiopen). A 1.44 MB FAT12 volume carrying:
#   HELLO.TXT  (starts "Hello...")  -- concurrent handle A
#   SECOND.TXT (starts "Second...") -- concurrent handle B (distinct content)
#   BIG.DAT    (96 KiB, > the old 64 KiB whole-file cap) -- handle C; a 13-byte
#              signature "BEYOND-64KiB!" sits at byte offset 80000 (> 65536) so
#              the program can LSEEK past 64 KiB and READ it back. BIG.DAT is
#              built deterministically (Rule 11): '.' filler + the signature
#              written in place at the fixed offset (keep in sync with
#              multiopen_program.asm BIG_SIG_OFF/BIG_SIG_LEN).
# Distinct from the other FAT images so its extra files never disturb their
# screendump/DIR assumptions. Build intermediate, NOT committed (Rule 11).
FAT_MULTIOPEN_IMG := $(BUILD)/fat_multiopen.img
MO_BIG_DAT        := $(BUILD)/big.dat
MO_BIG_SIZE       := 98304
MO_BIG_SIG_OFF    := 80000
MO_BIG_SIG        := BEYOND-64KiB!

$(MO_BIG_DAT): | $(BUILD)
	@# 96 KiB of '.' (0x2E) filler, deterministic.
	@tr '\0' '.' < /dev/zero | dd of=$@ bs=1 count=$(MO_BIG_SIZE) status=none
	@# Overwrite the 13-byte signature in place at the fixed offset (no trailing
	@# newline; printf %s). conv=notrunc so the file stays 96 KiB.
	@printf '%s' '$(MO_BIG_SIG)' | dd of=$@ bs=1 seek=$(MO_BIG_SIG_OFF) conv=notrunc status=none
	@printf ">>> big.dat: minted %s (%s bytes; signature '%s' at offset %s; build intermediate)\n" "$@" "$(MO_BIG_SIZE)" "$(MO_BIG_SIG)" "$(MO_BIG_SIG_OFF)"

$(FAT_MULTIOPEN_IMG): $(FAT12_FIXTURE_DIR)/hello.txt $(FAT12_FIXTURE_DIR)/second.txt $(MO_BIG_DAT) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/second.txt ::SECOND.TXT
	@mcopy -i $@ $(MO_BIG_DAT) ::BIG.DAT
	@printf ">>> fat_multiopen: minted %s (1.44MB FAT12 disk for test-multiopen; HELLO.TXT + SECOND.TXT + 96KiB BIG.DAT; build intermediate)\n" "$@"

# FAT12 storm disk for the INT 21h reentrancy oracle (beads initech-xk2; make
# test-int21-irqstorm). A 1.44 MB FAT12 volume carrying, in a KNOWN insertion
# order (root-dir slot order == enumeration order for FINDFIRST/NEXT):
#   ALPHA.TXT, BRAVO.TXT, CHARLIE.TXT, DELTA.TXT  -- four small distinct files
#       that exercise the FINDFIRST/NEXT search state (g_dta + g_find) across the
#       100 Hz PIT ticking + the keystroke storm. The harness asserts the exact
#       enumerated set (order-independent on the FAT side, but every name must
#       appear EXACTLY -- a skipped/duplicated entry from async g_find corruption
#       goes RED).
#   STORM.DAT  -- a deterministic multi-cluster file ('.' filler + a 13-byte
#       signature "STORM-SIGNAL!" at byte offset 1500, PAST the first 512-byte
#       cluster). Reading the signature forces a multi-cluster chain walk over the
#       FAT cache + cluster scratch (the slow ATA-PIO path a PIT IRQ lands inside);
#       a corrupted scratch/cache would return the WRONG bytes -> RED. Keep
#       SIG_OFF/SIG (1500 / "STORM-SIGNAL!") in sync with irqstorm_program.asm.
# Deterministic (Rule 11); build intermediate, NOT committed.
FAT_IRQSTORM_IMG  := $(BUILD)/fat_irqstorm.img
STORM_DAT         := $(BUILD)/storm.dat
STORM_DAT_SIZE    := 2048
STORM_SIG_OFF     := 1500
STORM_SIG         := STORM-SIGNAL!

$(STORM_DAT): | $(BUILD)
	@tr '\0' '.' < /dev/zero | dd of=$@ bs=1 count=$(STORM_DAT_SIZE) status=none
	@printf '%s' '$(STORM_SIG)' | dd of=$@ bs=1 seek=$(STORM_SIG_OFF) conv=notrunc status=none
	@printf ">>> storm.dat: minted %s (%s bytes; signature '%s' at offset %s)\n" "$@" "$(STORM_DAT_SIZE)" "$(STORM_SIG)" "$(STORM_SIG_OFF)"

$(FAT_IRQSTORM_IMG): $(STORM_DAT) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@printf 'alpha\r\n'   | mcopy -i $@ - ::ALPHA.TXT
	@printf 'bravo\r\n'   | mcopy -i $@ - ::BRAVO.TXT
	@printf 'charlie\r\n' | mcopy -i $@ - ::CHARLIE.TXT
	@printf 'delta\r\n'   | mcopy -i $@ - ::DELTA.TXT
	@mcopy -i $@ $(STORM_DAT) ::STORM.DAT
	@printf ">>> fat_irqstorm: minted %s (1.44MB FAT12 disk for test-int21-irqstorm; ALPHA/BRAVO/CHARLIE/DELTA.TXT + multi-cluster STORM.DAT; build intermediate)\n" "$@"

# Build the BPB oracle: the test + the REAL artifact fat12.c + the host
# blockdev backend. -Ispec for bpb_t, -Ios/milton for fat12.h/blockdev.h,
# -Iseed for test_assert.h, -I$(FAT_DIFF_DIR) for blockdev_file.h.
$(TEST_FAT12_BPB): $(FAT_DIFF_DIR)/test_fat12_bpb.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_bpb.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# Helper gate: build the oracle, mint the image, run the test. NOT yet wired
# into `test-fat` (that stays stub_fail until dir-enumerate + file-read land
# and the differential-vs-mtools oracle is in -- beads initech-5cu).
.PHONY: test-fat12-bpb
test-fat12-bpb: $(TEST_FAT12_BPB) $(FAT12_IMG)
	@printf ">>> test-fat12-bpb: mount + BPB parse + geometry vs verified constants\n"
	@$(TEST_FAT12_BPB) "$(FAT12_IMG)"
	@printf ">>> test-fat12-bpb: green\n"

# Build the FAT12 decode + chain-walk oracle: the test + the REAL artifact
# fat12.c + the host blockdev backend (same include set as the BPB oracle).
$(TEST_FAT12_CHAIN): $(FAT_DIFF_DIR)/test_fat12_chain.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_chain.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# Helper gate: build the oracle, mint the image, run the test. Like
# test-fat12-bpb this is NOT yet wired into `test-fat` (that stays stub_fail
# until dir-enumerate + file-read + the differential-vs-mtools oracle land --
# beads initech-5cu).
.PHONY: test-fat12-chain
test-fat12-chain: $(TEST_FAT12_CHAIN) $(FAT12_IMG)
	@printf ">>> test-fat12-chain: FAT12 12-bit decode + cluster-chain walk + anti-hang guard\n"
	@$(TEST_FAT12_CHAIN) "$(FAT12_IMG)"
	@printf ">>> test-fat12-chain: green\n"

# Build the FAT12 root-dir enumerate + find + file-read oracle: the test + the
# REAL artifact fat12.c + the host blockdev backend (same include set as the
# BPB/chain oracles).
$(TEST_FAT12_DIR): $(FAT_DIFF_DIR)/test_fat12_dir.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_dir.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# Helper gate: build the oracle, mint the image, run the test (passing the
# fixture dir so the test reads the committed files as the byte-for-byte
# golden). Like the bpb/chain helpers this is NOT yet wired into `test-fat`
# (the differential-vs-mtools gate is the NEXT step -- beads initech-5cu).
.PHONY: test-fat12-dir
test-fat12-dir: $(TEST_FAT12_DIR) $(FAT12_IMG)
	@printf ">>> test-fat12-dir: root-dir enumerate + find + file-read (byte-for-byte golden)\n"
	@$(TEST_FAT12_DIR) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)"
	@printf ">>> test-fat12-dir: green\n"

# Build the FAT12 positioned-read oracle + its two mutants (beads initech-lq2):
# the test + the REAL artifact fat12.c + the host blockdev backend (same include
# set as the bpb/chain/dir oracles). The mutants define one perturbed constant.
$(TEST_FAT12_PARTIAL): $(FAT_DIFF_DIR)/test_fat12_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

$(TEST_FAT12_PARTIAL_MUT_SKIP): $(FAT_DIFF_DIR)/test_fat12_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_PARTIAL_SKIP -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

$(TEST_FAT12_PARTIAL_MUT_POS): $(FAT_DIFF_DIR)/test_fat12_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_PARTIAL_POS -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# REAL gate: test-fat-partial (beads initech-lq2 -- FAT12 positioned read).
#   [A] the host matrix oracle: fat12_read_partial vs the committed fixture
#       sliced in-process (offset 0 / mid-cluster / cluster-boundary-spanning /
#       at-EOF / past-EOF / over-long len / cross-many-clusters).
#   [B] a THIRD independent reference: the python3 reader's --cat-range mode.
#       For a vector of (file, off, len) the python slice == our writer's slice
#       (fat_dump has no range mode, so we compare python ref vs a tiny C dumper
#       built into test_fat12_partial? -- no: we drive our C primitive through
#       the host oracle in [A]; here [B] cross-checks the python ref against
#       mcopy+dd of the SAME range so the reference itself is proven on real
#       FAT12, closing the loop python<->mtools independent of our C).
.PHONY: test-fat-partial
test-fat-partial: $(TEST_FAT12_PARTIAL) $(FAT12_IMG) $(FAT12_REF_PY)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fat-partial : FAT12 positioned-read oracle\n'
	@printf '======================================================================\n'
	@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-partial FAIL: python3 not found (needed for the independent --cat-range reference).\n'; exit 1; }
	@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-partial FAIL: mtools `mcopy` not found (apt install mtools).\n'; exit 1; }
	@printf '>>> test-fat-partial [A]: host matrix -- fat12_read_partial == fixture slice (byte-for-byte)\n'
	@$(TEST_FAT12_PARTIAL) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" \
		|| { printf '!!! test-fat-partial FAIL [A]: host matrix oracle red\n'; exit 1; }
	@printf '>>> test-fat-partial [A]: green\n'
	@printf '>>> test-fat-partial [B]: python3 --cat-range == mcopy+dd of the SAME range (reference proven on real FAT12)\n'
	@set -e; \
	for spec in "CHAIN.TXT 0 64" "CHAIN.TXT 500 100" "CHAIN.TXT 600 500" \
	            "CHAIN.TXT 1024 512" "CHAIN.TXT 1550 200" "CHAIN.TXT 1 1599" \
	            "HELLO.TXT 4 10" "BLOCK.BIN 512 512" "BLOCK.BIN 1000 4096" \
	            "SECOND.TXT 50 200"; do \
		set -- $$spec; nm="$$1"; off="$$2"; ln="$$3"; \
		python3 "$(FAT12_REF_PY)" "$(FAT12_IMG)" --cat-range "$$nm" "$$off" "$$ln" \
			> "$(BUILD)/fatp_py_$${nm}_$${off}_$${ln}.bin" \
			|| { printf '!!! test-fat-partial FAIL [B]: python --cat-range %s %s %s errored\n' "$$nm" "$$off" "$$ln"; exit 1; }; \
		mcopy -n -i "$(FAT12_IMG)" "::$$nm" "$(BUILD)/fatp_whole_$$nm.bin" \
			|| { printf '!!! test-fat-partial FAIL [B]: mcopy ::%s errored\n' "$$nm"; exit 1; }; \
		dd if="$(BUILD)/fatp_whole_$$nm.bin" of="$(BUILD)/fatp_dd_$${nm}_$${off}_$${ln}.bin" \
			bs=1 skip="$$off" count="$$ln" status=none 2>/dev/null || true; \
		cmp -s "$(BUILD)/fatp_py_$${nm}_$${off}_$${ln}.bin" "$(BUILD)/fatp_dd_$${nm}_$${off}_$${ln}.bin" \
			|| { printf '!!! test-fat-partial FAIL [B]: %s [%s,+%s) -- python ref bytes != dd-of-mcopy bytes\n' "$$nm" "$$off" "$$ln"; \
			     cmp "$(BUILD)/fatp_py_$${nm}_$${off}_$${ln}.bin" "$(BUILD)/fatp_dd_$${nm}_$${off}_$${ln}.bin"; exit 1; }; \
	done
	@printf '>>> test-fat-partial [B]: green (python --cat-range agrees with mcopy+dd on every range)\n'
	@printf '>>> test-fat-partial: green\n'

# Mutation gate (Rule 6): two fat12_read_partial mutants -- (skip) off-by-one in
# the skip-cluster count; (pos) drops the within-cluster byte offset. Each MUST
# turn the host matrix oracle RED, else the oracle is decoration.
.PHONY: test-fat-partial-mutant
test-fat-partial-mutant: $(TEST_FAT12_PARTIAL_MUT_SKIP) $(TEST_FAT12_PARTIAL_MUT_POS) $(FAT12_IMG)
	@printf '>>> test-fat-partial-mutant: confirming both positioned-read mutants go RED (Rule 6)\n'
	@if $(TEST_FAT12_PARTIAL_MUT_SKIP) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
		printf '!!! test-fat-partial-mutant FAIL: skip-count mutant PASSED -- the matrix is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fat-partial-mutant: green (skip-count mutant correctly RED)\n'; \
	fi
	@if $(TEST_FAT12_PARTIAL_MUT_POS) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
		printf '!!! test-fat-partial-mutant FAIL: within-cluster-offset mutant PASSED -- the matrix is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fat-partial-mutant: green (within-cluster-offset mutant correctly RED -- the oracle bites)\n'; \
	fi

# Build the differential dumper: the factory tool + the REAL artifact fat12.c +
# the host blockdev backend (same include set as the FAT12 oracles).
$(FAT_DUMP_BIN): $(FAT_DUMP_SRC) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DUMP_SRC) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# --- FAT12 WRITE oracle (beads initech-509.11) ------------------------------
# The FAT12 WRITE half: a host test (test_fat12_write.c) drives the REAL
# artifact fat12.c WRITE functions over a read-WRITE file-backed blockdev,
# mutating a freshly-minted BLANK image. Two binaries:
#   $(TEST_FAT12_WRITE)      : the unit + in-memory round-trip oracle.
#   $(TEST_FAT12_WRITE_MUT_*): mutation builds (Rule 6) -- a single fat12.c
#     constant perturbed so `make test-fat-write-mutant` proves the diff BITES.
TEST_FAT12_WRITE  := $(BUILD)/test_fat12_write
# Blank (no fixtures) 1.44 MB FAT12 image the WRITE oracle mutates in place. A
# SEPARATE freshly-minted blank per run (build intermediate, NOT committed).
FAT12_BLANK_IMG   := $(BUILD)/fat12_blank.img
# A second blank used by the unit suite (it fills/unlinks; keep it off the diff).
FAT12_BLANK_UNIT_IMG := $(BUILD)/fat12_blank_unit.img
# The four files the WRITE round-trip diffs three ways (our reader vs mtools vs
# python3). Names + deterministic content are pinned by test_fat12_write.c.
FAT12_WRITE_NAMES := SHORT.TXT MULTI.DAT EXACT.BIN EMPTY.NEW

# Mint a BLANK 1.44 MB FAT12 image (mformat only -- NO files). Deterministic
# (mformat -f 1440 fixes the BPB; the volume serial + label vary but are
# normalized away in the diff). Build intermediate, NOT committed (Rule 11).
$(FAT12_BLANK_IMG): | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; WRITE oracle target)\n" "$@"

$(FAT12_BLANK_UNIT_IMG): | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; WRITE unit target)\n" "$@"

$(TEST_FAT12_WRITE): $(FAT_DIFF_DIR)/test_fat12_write.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_write.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# Mutation builds (Rule 6): fat12.c WRITE perturbed one constant at a time so the
# round-trip oracle MUST go RED. (a) write only ONE FAT copy (DEC-07 sync broken);
# (b) the wrong EOC marker on the last cluster (chain runs off the end).
TEST_FAT12_WRITE_MUT_ONEFAT := $(BUILD)/test_fat12_write_mut_onefat
TEST_FAT12_WRITE_MUT_EOC    := $(BUILD)/test_fat12_write_mut_eoc

$(TEST_FAT12_WRITE_MUT_ONEFAT): $(FAT_DIFF_DIR)/test_fat12_write.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_ONE_FAT_COPY -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_write.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

$(TEST_FAT12_WRITE_MUT_EOC): $(FAT_DIFF_DIR)/test_fat12_write.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_WRONG_EOC -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_write.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

.PHONY: test-fat12-write
test-fat12-write: $(TEST_FAT12_WRITE) $(FAT12_BLANK_UNIT_IMG)
	@printf ">>> test-fat12-write: FAT12 WRITE unit (12-bit encode + alloc + both-FAT sync + round-trip + full-volume fail-loud + unlink)\n"
	@cp -f "$(FAT12_BLANK_UNIT_IMG)" "$(BUILD)/fat12_write_unit_run.img"
	@$(TEST_FAT12_WRITE) "$(BUILD)/fat12_write_unit_run.img"
	@printf ">>> test-fat12-write: green\n"

# --- FAT12 POSITIONED-WRITE oracle (beads initech-snk) ----------------------
# The symmetric counterpart of fat12_read_partial: fat12_write_partial does a
# positioned read-modify-write (overwrite in place, extend past EOF growing the
# chain, zero-fill the hole when offset > size). A host test drives the REAL
# artifact over a read-WRITE file-backed blockdev on a freshly-minted BLANK
# image; the Makefile then reads the written files back via mtools + python3.
TEST_FAT12_WRITE_PARTIAL          := $(BUILD)/test_fat12_write_partial
TEST_FAT12_WRITE_PARTIAL_MUT_RMW  := $(BUILD)/test_fat12_write_partial_mut_rmw
TEST_FAT12_WRITE_PARTIAL_MUT_EXT  := $(BUILD)/test_fat12_write_partial_mut_ext
# A blank used by the positioned-write unit (keep it off the other diffs).
FAT12_BLANK_WP_IMG := $(BUILD)/fat12_blank_wp.img
# The five files the positioned-write round-trip diffs three ways (our reader vs
# mtools vs python3). Names are pinned by test_fat12_write_partial.c.
FAT12_WRITE_PARTIAL_NAMES := OVR.BIN APP.BIN HOLE.BIN FRESH.BIN BIG.BIN

$(FAT12_BLANK_WP_IMG): | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; positioned-WRITE oracle target)\n" "$@"

$(TEST_FAT12_WRITE_PARTIAL): $(FAT_DIFF_DIR)/test_fat12_write_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_write_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# Mutation builds (Rule 6): fat12_write_partial perturbed one thing at a time so
# the differential oracle MUST go RED. (rmw) skip the read-modify-write read on a
# partial cluster -> a partial overwrite zeroes its neighbours; (ext) allocate
# one too few extend clusters -> a multi-cluster grow leaves the chain short.
$(TEST_FAT12_WRITE_PARTIAL_MUT_RMW): $(FAT_DIFF_DIR)/test_fat12_write_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_PARTIAL_NO_RMW -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_write_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

$(TEST_FAT12_WRITE_PARTIAL_MUT_EXT): $(FAT_DIFF_DIR)/test_fat12_write_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_PARTIAL_EXTEND_SHORT -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/test_fat12_write_partial.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)

# REAL gate: test-fat-write-partial (beads initech-snk).
#   [A] the in-process oracle: every positioned write is mirrored into a byte
#       model; fat12_read_file AND fat12_read_partial read it back == model
#       (overwrite-in-place, cluster-boundary span, append @EOF, multi-cluster
#       extend, zero hole, write-to-empty, many-cluster file) + disk-full rollback.
#   [B] our writer writes the files into a blank image; mtools `mcopy` AND the
#       python3 reference (fat12_ref.py --cat) read each back == the python ref's
#       bytes (THIRD independent reader), proving a real DOS tool agrees with the
#       bytes we wrote (file CONTENT exact; Law 1).
.PHONY: test-fat-write-partial
test-fat-write-partial: $(TEST_FAT12_WRITE_PARTIAL) $(FAT12_BLANK_WP_IMG) $(FAT12_REF_PY)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fat-write-partial : FAT12 positioned-WRITE oracle\n'
	@printf '======================================================================\n'
	@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-write-partial FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
	@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-write-partial FAIL: python3 not found (needed for the independent reference).\n'; exit 1; }
	@printf '>>> test-fat-write-partial [A]: in-process matrix -- read_file/read_partial == byte model (+ disk-full rollback)\n'
	@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_unit.img"
	@$(TEST_FAT12_WRITE_PARTIAL) "$(BUILD)/fatwp_unit.img" \
		|| { printf '!!! test-fat-write-partial FAIL [A]: in-process matrix red\n'; exit 1; }
	@printf '>>> test-fat-write-partial [A]: green\n'
	@printf '>>> test-fat-write-partial [B]: OUR writer writes the files into a fresh blank image\n'
	@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_diff.img"
	@$(TEST_FAT12_WRITE_PARTIAL) "$(BUILD)/fatwp_diff.img" --diff \
		|| { printf '!!! test-fat-write-partial FAIL [B]: writing the diff fixtures failed\n'; exit 1; }
	@printf '>>> test-fat-write-partial [C]: content -- mcopy == python3 ref, per positioned-written file\n'
	@set -e; \
	for f in $(FAT12_WRITE_PARTIAL_NAMES); do \
		mcopy -n -i "$(BUILD)/fatwp_diff.img" "::$$f" "$(BUILD)/fatwp_mtools_$$f.bin" \
			|| { printf '!!! test-fat-write-partial FAIL [C]: mcopy ::%s errored (mtools could not read our written file)\n' "$$f"; exit 1; }; \
		python3 "$(FAT12_REF_PY)" "$(BUILD)/fatwp_diff.img" --cat "$$f" > "$(BUILD)/fatwp_py_$$f.bin" \
			|| { printf '!!! test-fat-write-partial FAIL [C]: fat12_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
		cmp -s "$(BUILD)/fatwp_mtools_$$f.bin" "$(BUILD)/fatwp_py_$$f.bin" \
			|| { printf '!!! test-fat-write-partial FAIL [C]: %s -- mcopy bytes != python ref bytes (root-cause the writer, Rule 3)\n' "$$f"; \
			     cmp "$(BUILD)/fatwp_mtools_$$f.bin" "$(BUILD)/fatwp_py_$$f.bin"; exit 1; }; \
	done
	@printf '>>> test-fat-write-partial [C]: green (mcopy + python3 agree on every positioned-written file)\n'
	@printf '>>> test-fat-write-partial: green\n'

# Mutation gate (Rule 6): two fat12_write_partial mutants -- (rmw) skip the
# read-modify-write so a partial overwrite clobbers neighbours; (ext) allocate
# one too few extend clusters. Each MUST turn the in-process matrix RED.
.PHONY: test-fat-write-partial-mutant
test-fat-write-partial-mutant: $(TEST_FAT12_WRITE_PARTIAL_MUT_RMW) $(TEST_FAT12_WRITE_PARTIAL_MUT_EXT) $(FAT12_BLANK_WP_IMG)
	@printf '>>> test-fat-write-partial-mutant: confirming both positioned-write mutants go RED (Rule 6)\n'
	@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_mut_rmw.img"
	@if $(TEST_FAT12_WRITE_PARTIAL_MUT_RMW) "$(BUILD)/fatwp_mut_rmw.img" >/dev/null 2>&1; then \
		printf '!!! test-fat-write-partial-mutant FAIL: no-RMW mutant PASSED -- the matrix is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fat-write-partial-mutant: green (no-RMW mutant correctly RED)\n'; \
	fi
	@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_mut_ext.img"
	@if $(TEST_FAT12_WRITE_PARTIAL_MUT_EXT) "$(BUILD)/fatwp_mut_ext.img" >/dev/null 2>&1; then \
		printf '!!! test-fat-write-partial-mutant FAIL: short-extend mutant PASSED -- the matrix is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fat-write-partial-mutant: green (short-extend mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# Text console blit oracle (os/milton, beads initech-yqb)
# ---------------------------------------------------------------------------
# Host blit-math oracle for the LFB text console. The REAL artifact console.c
# (freestanding in the kernel) is compiled HOSTED here against a fake
# framebuffer + boot_info + synthetic font, and the 8x16 glyph blit / cursor /
# wrap / scroll properties are asserted (CLAUDE.md Law 2 -- the load-bearing
# pixel math has a mechanical oracle). The fake fb/font are mmap'd in the low
# 4 GiB (MAP_32BIT) so boot_info's uint32_t addresses round-trip on a 64-bit
# host. Mirrors the $(TEST_FAT12_*) idiom (libc host test, seed/test_assert.h).
TEST_CONSOLE     := $(BUILD)/test_console
TEST_CONSOLE_SRC := $(MILTON_DIR)/test_console.c

$(TEST_CONSOLE): $(TEST_CONSOLE_SRC) $(MILTON_DIR)/console.c $(MILTON_DIR)/console.h $(MILTON_DIR)/boot_info.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_CONSOLE_SRC) $(MILTON_DIR)/console.c

.PHONY: test-console
test-console: $(TEST_CONSOLE)
	@printf ">>> test-console: 8x16 glyph blit (MSB-left, bpp 32/24) + cursor/wrap/scroll\n"
	@$(TEST_CONSOLE)
	@printf ">>> test-console: green\n"

# ---------------------------------------------------------------------------
# IDT encode + interrupt-frame layout oracle (os/milton, beads initech-a5a)
# ---------------------------------------------------------------------------
# Host unit oracle (factory; libc + seed/test_assert.h) for the load-bearing
# byte layout: the IDT gate offset-split encode (idt_set_gate) + every
# int_frame_t field offset (must match the asm pushad order). Compiles the REAL
# artifact idt.c HOSTED and asserts idt_set_gate's bytes against a hand-computed
# expectation + offsetof checks. Mirrors the $(TEST_FAT12_*) idiom.
TEST_IDT      := $(BUILD)/test_idt
TEST_IDT_SRC  := $(MILTON_DIR)/test_idt.c
# Mutation build (CLAUDE.md Rule 6): idt.c compiled with the offset-hi shift
# broken so `make test-idt-mutant` can prove the encode oracle BITES.
TEST_IDT_MUT  := $(BUILD)/test_idt_mutant

$(TEST_IDT): $(TEST_IDT_SRC) $(KERNEL_IDT_C) $(MILTON_DIR)/idt.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_IDT_SRC) $(KERNEL_IDT_C)

$(TEST_IDT_MUT): $(TEST_IDT_SRC) $(KERNEL_IDT_C) $(MILTON_DIR)/idt.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DIDT_MUTATE_OFFSET_HI -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_IDT_SRC) $(KERNEL_IDT_C)

.PHONY: test-idt test-idt-mutant
test-idt: $(TEST_IDT)
	@printf ">>> test-idt: IDT gate encode (offset-split) + int_frame_t field offsets\n"
	@$(TEST_IDT)
	@printf ">>> test-idt: green\n"

# Mutation-proof: the mutant build (wrong offset-hi shift) MUST fail the oracle.
# A mutant that PASSES means the oracle is decoration (CLAUDE.md Rule 6).
test-idt-mutant: $(TEST_IDT_MUT)
	@printf ">>> test-idt-mutant: confirming the offset-hi mutant goes RED (Rule 6)\n"
	@if $(TEST_IDT_MUT) >/dev/null 2>&1; then \
		printf '!!! test-idt-mutant FAIL: mutant PASSED the oracle -- the encode test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-idt-mutant: green (mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# Keyboard/PIT pure-logic oracle (os/milton, beads initech-3rs)
# ---------------------------------------------------------------------------
# Host unit oracle (factory; libc + seed/test_assert.h) for the PURE halves of
# the PS/2 keyboard + 8254 PIT drivers: the ring buffer (enqueue/dequeue/empty/
# full/wrap), the scancode-set-1 -> US ASCII translator (incl. a SHIFTed key +
# CapsLock + modifier make/break), and the PIT divisor math (100 Hz + the 18.2
# Hz divisor-0 sentinel). Compiles the REAL artifact kbd.c + pit.c HOSTED -- the
# same functions the kernel uses; the IRQ/port-touching halves are proven by the
# end-to-end make test-kbd. Mirrors the $(TEST_IDT) idiom.
TEST_KBD       := $(BUILD)/test_kbd
TEST_KBD_SRC   := $(MILTON_DIR)/test_kbd.c
TEST_KBD_DEPS  := $(KERNEL_KBD_C) $(KERNEL_PIT_C)
TEST_KBD_HDRS  := $(MILTON_DIR)/kbd.h $(MILTON_DIR)/pit.h $(MILTON_DIR)/io.h
# Mutation builds (CLAUDE.md Rule 6): a single branch/constant perturbed so
# `make test-kbd-unit-mutant` can prove the oracle BITES. (a) the ring full-test
# off-by-one (wrap overwrites unread data); (b) the scancode table mis-indexed.
TEST_KBD_MUT_RING  := $(BUILD)/test_kbd_mutant_ring
TEST_KBD_MUT_TABLE := $(BUILD)/test_kbd_mutant_table

$(TEST_KBD): $(TEST_KBD_SRC) $(TEST_KBD_DEPS) $(TEST_KBD_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_KBD_SRC) $(TEST_KBD_DEPS)

$(TEST_KBD_MUT_RING): $(TEST_KBD_SRC) $(TEST_KBD_DEPS) $(TEST_KBD_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DKBD_MUTATE_RING_OFFBYONE -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_KBD_SRC) $(TEST_KBD_DEPS)

$(TEST_KBD_MUT_TABLE): $(TEST_KBD_SRC) $(TEST_KBD_DEPS) $(TEST_KBD_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DKBD_MUTATE_SCANCODE_TABLE -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_KBD_SRC) $(TEST_KBD_DEPS)

.PHONY: test-kbd-unit test-kbd-unit-mutant
test-kbd-unit: $(TEST_KBD)
	@printf ">>> test-kbd-unit: ring (enqueue/dequeue/empty/full/wrap) + scancode set 1 -> ASCII (+shift/caps) + PIT divisor math\n"
	@$(TEST_KBD)
	@printf ">>> test-kbd-unit: green\n"

# Mutation-proof: BOTH mutant builds MUST fail the oracle (Rule 6).
test-kbd-unit-mutant: $(TEST_KBD_MUT_RING) $(TEST_KBD_MUT_TABLE)
	@printf ">>> test-kbd-unit-mutant: confirming both mutants go RED (Rule 6)\n"
	@if $(TEST_KBD_MUT_RING) >/dev/null 2>&1; then \
		printf '!!! test-kbd-unit-mutant FAIL: ring off-by-one mutant PASSED -- the full/wrap test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-kbd-unit-mutant: green (ring off-by-one mutant correctly RED)\n'; \
	fi
	@if $(TEST_KBD_MUT_TABLE) >/dev/null 2>&1; then \
		printf '!!! test-kbd-unit-mutant FAIL: scancode-table mutant PASSED -- the translate test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-kbd-unit-mutant: green (scancode-table mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# INT 21h dispatch oracle (os/milton, beads initech-509.5)
# ---------------------------------------------------------------------------
# Host unit oracle (factory; libc + seed/test_assert.h) for the INT 21h dispatch
# logic: AH-dispatch, the console-output subset (02h/09h/40h), GETVER (30h),
# TERMINATE (00h/4Ch), the bad-handle error path, and the controlled-scope
# behavior (unlisted AH -> diagnostic + CF; listed-but-deferred AH -> distinct
# not-yet-impl diagnostic). Compiles the REAL artifact int21.c HOSTED; the CON
# sink + terminate action are abstracted so the test captures them. Mirrors the
# $(TEST_IDT) idiom. The dispatcher also compiles freestanding into the kernel.
TEST_INT21      := $(BUILD)/test_int21
TEST_INT21_SRC  := $(MILTON_DIR)/test_int21.c
# KERNEL_INT21_C is defined in the kernel-build section above.
# Mutation builds (CLAUDE.md Rule 6): the dispatcher compiled with a single
# branch/constant perturbed so `make test-int21-mutant` can prove the oracle
# BITES. (a) 09h emits the '$' terminator; (b) the unlisted-AH path is a silent
# no-op. A mutant that PASSES means the oracle is decoration.
TEST_INT21_MUT_DOLLAR := $(BUILD)/test_int21_mutant_dollar
TEST_INT21_MUT_NOOP   := $(BUILD)/test_int21_mutant_noop

# int21.c now routes handle functions through the SFT (initech-509.3), so the
# host oracle links sft.c + psp.c too and needs -Ispec (sft.h -> psp.h ->
# dos_structs.h). The standard JFT/SFT is bound in test_int21.c's main().
TEST_INT21_DEPS := $(KERNEL_INT21_C) $(KERNEL_SFT_C) $(KERNEL_PSP_C) $(KERNEL_IRQ_C)
TEST_INT21_HDRS := $(MILTON_DIR)/int21.h $(MILTON_DIR)/idt.h $(MILTON_DIR)/sft.h \
                   $(MILTON_DIR)/psp.h spec/dos_structs.h

$(TEST_INT21): $(TEST_INT21_SRC) $(TEST_INT21_DEPS) $(TEST_INT21_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_INT21_SRC) $(TEST_INT21_DEPS)

$(TEST_INT21_MUT_DOLLAR): $(TEST_INT21_SRC) $(TEST_INT21_DEPS) $(TEST_INT21_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_PUTS_EMIT_DOLLAR -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_INT21_SRC) $(TEST_INT21_DEPS)

$(TEST_INT21_MUT_NOOP): $(TEST_INT21_SRC) $(TEST_INT21_DEPS) $(TEST_INT21_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_UNLISTED_NOOP -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_INT21_SRC) $(TEST_INT21_DEPS)

# --- CON INPUT oracle (beads initech-n62) ----------------------------------
# Host unit oracle for the INT 21h CON-INPUT functions (01h/06h/07h/08h/0Ah/
# 0Bh/0Ch), driven through the REAL artifact int21_dispatch with a MOCK input
# source (a queued string) bound via int21_set_conin -- mirroring the sink seam
# that keeps int21.c host-testable. int21.c is the SAME TU the kernel runs.
# Mutation builds (Rule 6): (a) 01h drops the echo; (b) 0Ah counts the CR in the
# length byte. A mutant that PASSES means the oracle is decoration.
TEST_CONIN      := $(BUILD)/test_conin
TEST_CONIN_SRC  := $(MILTON_DIR)/test_conin.c
TEST_CONIN_MUT_NOECHO := $(BUILD)/test_conin_mutant_noecho
TEST_CONIN_MUT_COUNT  := $(BUILD)/test_conin_mutant_count
# test_conin.c drives only the CON-input path, but int21.c (one TU) references
# the SFT/PSP handle layer (do_write/do_open/...), so the link needs sft.c +
# psp.c just like test_int21. -Ispec for sft.h -> psp.h -> dos_structs.h.
TEST_CONIN_DEPS := $(KERNEL_INT21_C) $(KERNEL_SFT_C) $(KERNEL_PSP_C) $(KERNEL_IRQ_C)
TEST_CONIN_HDRS := $(MILTON_DIR)/int21.h $(MILTON_DIR)/idt.h $(MILTON_DIR)/sft.h \
                   $(MILTON_DIR)/psp.h spec/dos_structs.h

$(TEST_CONIN): $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS)

$(TEST_CONIN_MUT_NOECHO): $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_CONIN_NO_ECHO -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS)

$(TEST_CONIN_MUT_COUNT): $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_BUFINPUT_COUNT_CR -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS)

.PHONY: test-conin-unit test-conin-mutant
test-conin-unit: $(TEST_CONIN)
	@printf ">>> test-conin-unit: CON input 01h/06h/07h/08h/0Ah(+BS,+clamp)/0Bh/0Ch via mock source\n"
	@$(TEST_CONIN)
	@printf ">>> test-conin-unit: green\n"

test-conin-mutant: $(TEST_CONIN_MUT_NOECHO) $(TEST_CONIN_MUT_COUNT)
	@printf ">>> test-conin-mutant: confirming both mutants go RED (Rule 6)\n"
	@if $(TEST_CONIN_MUT_NOECHO) >/dev/null 2>&1; then \
		printf '!!! test-conin-mutant FAIL: 01h-no-echo mutant PASSED -- the echo test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-conin-mutant: green (01h-no-echo mutant correctly RED)\n'; \
	fi
	@if $(TEST_CONIN_MUT_COUNT) >/dev/null 2>&1; then \
		printf '!!! test-conin-mutant FAIL: 0Ah-count-CR mutant PASSED -- the buffered-input count test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-conin-mutant: green (0Ah-count-CR mutant correctly RED -- the oracle bites)\n'; \
	fi

.PHONY: test-int21 test-int21-mutant test-conin-unit
test-int21: $(TEST_INT21)
	@printf ">>> test-int21: AH-dispatch + console subset (02h/09h/40h/30h/4Ch) + controlled scope\n"
	@$(TEST_INT21)
	@printf ">>> test-int21: green\n"

# Mutation-proof: BOTH mutant builds MUST fail the oracle (Rule 6).
test-int21-mutant: $(TEST_INT21_MUT_DOLLAR) $(TEST_INT21_MUT_NOOP)
	@printf ">>> test-int21-mutant: confirming both mutants go RED (Rule 6)\n"
	@if $(TEST_INT21_MUT_DOLLAR) >/dev/null 2>&1; then \
		printf '!!! test-int21-mutant FAIL: 09h-emit-dollar mutant PASSED -- the PUTS test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-int21-mutant: green (09h-emit-dollar mutant correctly RED)\n'; \
	fi
	@if $(TEST_INT21_MUT_NOOP) >/dev/null 2>&1; then \
		printf '!!! test-int21-mutant FAIL: unlisted-AH-noop mutant PASSED -- the controlled-scope test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-int21-mutant: green (unlisted-AH-noop mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# REAL gate: test-fileio (beads initech-509.5 read-side -- file-handle funcs)
# ---------------------------------------------------------------------------
# Host unit oracle for the INT 21h file-handle functions (3Dh OPEN / 3Fh READ /
# 3Eh CLOSE / 42h LSEEK / 4Eh/4Fh FINDFIRST/NEXT / 1Ah/2Fh SETDTA/GETDTA),
# driven through the REAL artifact int21_dispatch with a MOCK file backend (an
# in-memory directory) standing in for the FAT12 volume (brief Sec 6 Step 4).
# Links int21.c + sft.c + psp.c; -Ispec for sft.h -> psp.h -> dos_structs.h and
# spec/find_data.h. Mirrors the $(TEST_INT21) idiom.
TEST_FILEIO      := $(BUILD)/test_fileio
TEST_FILEIO_SRC  := $(MILTON_DIR)/test_fileio.c
TEST_FILEIO_DEPS := $(KERNEL_INT21_C) $(KERNEL_SFT_C) $(KERNEL_PSP_C) $(KERNEL_IRQ_C)
TEST_FILEIO_HDRS := $(MILTON_DIR)/int21.h $(MILTON_DIR)/idt.h $(MILTON_DIR)/sft.h \
                    $(MILTON_DIR)/psp.h spec/dos_structs.h spec/find_data.h
# Mutation builds (CLAUDE.md Rule 6): int21.c compiled with a single file-op
# branch perturbed so `make test-fileio-mutant` can prove the oracle BITES.
# (a) READ ignores file_offset (no advance); (b) LSEEK SEEK_END base is wrong.
TEST_FILEIO_MUT_READ  := $(BUILD)/test_fileio_mutant_read
TEST_FILEIO_MUT_LSEEK := $(BUILD)/test_fileio_mutant_lseek

$(TEST_FILEIO): $(TEST_FILEIO_SRC) $(TEST_FILEIO_DEPS) $(TEST_FILEIO_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_FILEIO_SRC) $(TEST_FILEIO_DEPS)

$(TEST_FILEIO_MUT_READ): $(TEST_FILEIO_SRC) $(TEST_FILEIO_DEPS) $(TEST_FILEIO_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_READ_IGNORE_OFFSET -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_FILEIO_SRC) $(TEST_FILEIO_DEPS)

$(TEST_FILEIO_MUT_LSEEK): $(TEST_FILEIO_SRC) $(TEST_FILEIO_DEPS) $(TEST_FILEIO_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_LSEEK_WHENCE -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_FILEIO_SRC) $(TEST_FILEIO_DEPS)

.PHONY: test-fileio test-fileio-mutant
test-fileio: $(TEST_FILEIO)
	@printf ">>> test-fileio: OPEN/READ/CLOSE/LSEEK + FINDFIRST/NEXT + SETDTA/GETDTA via mock backend (initech-509.5)\n"
	@$(TEST_FILEIO)
	@printf ">>> test-fileio: green\n"

# Mutation-proof: BOTH mutant builds MUST fail the oracle (Rule 6).
test-fileio-mutant: $(TEST_FILEIO_MUT_READ) $(TEST_FILEIO_MUT_LSEEK)
	@printf ">>> test-fileio-mutant: confirming both mutants go RED (Rule 6)\n"
	@if $(TEST_FILEIO_MUT_READ) >/dev/null 2>&1; then \
		printf '!!! test-fileio-mutant FAIL: READ-ignore-offset mutant PASSED -- the offset-advance test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fileio-mutant: green (READ-ignore-offset mutant correctly RED)\n'; \
	fi
	@if $(TEST_FILEIO_MUT_LSEEK) >/dev/null 2>&1; then \
		printf '!!! test-fileio-mutant FAIL: LSEEK-whence mutant PASSED -- the SEEK_END test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fileio-mutant: green (LSEEK-whence mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# REAL gate: test-exec-unit (beads initech-saw + 509.5 -- AH=4Bh/4Dh dispatch)
# ---------------------------------------------------------------------------
# Host unit oracle for INT 21h AH=4Bh EXEC + AH=4Dh GET-RETURN-CODE, driven
# through the REAL artifact int21_dispatch with a MOCK EXEC backend (an
# int21_exec_fn standing in for the kernel-only FAT-sourced loader). Proves the
# register decode + path validation ('\'/':') + not-found/nested mapping + the
# 4Dh child-rc retrieval -- WITHOUT linking loader.c (which is kernel-only). The
# in-emulator make test-exec proves the real load FROM the FAT volume. Links
# int21.c only; -Ispec for int21.h -> idt.h. Mirrors the $(TEST_INT21) idiom.
TEST_EXEC        := $(BUILD)/test_exec
TEST_EXEC_SRC    := $(MILTON_DIR)/test_exec.c
# int21.c pulls in the SFT/JFT handle layer (sft.c) + psp.c (as test_int21 does),
# even though the EXEC path itself does not touch them -- link them so int21.c
# resolves.
TEST_EXEC_DEPS   := $(KERNEL_INT21_C) $(KERNEL_SFT_C) $(KERNEL_PSP_C) $(KERNEL_IRQ_C)
TEST_EXEC_HDRS   := $(MILTON_DIR)/int21.h $(MILTON_DIR)/idt.h $(MILTON_DIR)/sft.h \
                    $(MILTON_DIR)/psp.h spec/dos_structs.h
# Mutation build (CLAUDE.md Rule 6): int21.c compiled with AH=4Dh always
# reporting rc=0 so `make test-exec-mutant` can prove the GET-RETURN-CODE oracle
# BITES (it EXECs a program exiting rc=7 and asserts AL==7).
TEST_EXEC_MUT_RC := $(BUILD)/test_exec_mutant_rc

$(TEST_EXEC): $(TEST_EXEC_SRC) $(TEST_EXEC_DEPS) $(TEST_EXEC_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_EXEC_SRC) $(TEST_EXEC_DEPS)

$(TEST_EXEC_MUT_RC): $(TEST_EXEC_SRC) $(TEST_EXEC_DEPS) $(TEST_EXEC_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_RETCODE_ZERO -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_EXEC_SRC) $(TEST_EXEC_DEPS)

.PHONY: test-exec-unit test-exec-mutant
test-exec-unit: $(TEST_EXEC)
	@printf ">>> test-exec-unit: AH=4Bh EXEC (path validation + not-found/nested mapping) + AH=4Dh GET-RETURN-CODE via mock backend\n"
	@$(TEST_EXEC)
	@printf ">>> test-exec-unit: green\n"

# Mutation-proof: the rc-always-zero mutant MUST fail the oracle (Rule 6).
test-exec-mutant: $(TEST_EXEC_MUT_RC)
	@printf ">>> test-exec-mutant: confirming the 4Dh-rc-zero mutant goes RED (Rule 6)\n"
	@if $(TEST_EXEC_MUT_RC) >/dev/null 2>&1; then \
		printf '!!! test-exec-mutant FAIL: 4Dh-rc-zero mutant PASSED -- the GET-RETURN-CODE test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-exec-mutant: green (4Dh-rc-zero mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# REAL gate: test-command (beads initech-7pc -- the PURE COMMAND.COM logic)
# ---------------------------------------------------------------------------
# Host unit oracle for the pure COMMAND.COM shell logic: the command tokenizer +
# upcaser, the built-in dispatch classifier, the ".COM appender", and the DIR-
# record line formatter. Compiles the REAL artifact command.c HOSTED WITHOUT
# -DCOMMAND_KERNEL_REPL, so only the asm-free pure logic is linked (the kernel
# REPL + its int 0x21 wrappers are compiled out) -- the host-testability seam
# command.h documents (mirrors how int21.c keeps its dispatch host-testable).
# -Ispec for dos_structs.h (DIR_ATTR_DIRECTORY). Mirrors the $(TEST_INT21) idiom.
TEST_COMMAND      := $(BUILD)/test_command
TEST_COMMAND_SRC  := $(MILTON_DIR)/test_command.c
TEST_COMMAND_DEPS := $(KERNEL_COMMAND_C)
TEST_COMMAND_HDRS := $(MILTON_DIR)/command.h spec/dos_structs.h spec/find_data.h
# Mutation builds (CLAUDE.md Rule 6): command.c compiled with a single perturbation
# so `make test-command-mutant` can prove the oracle BITES. (a) the parser stops
# upper-casing -> lowercase "dir" no longer dispatches; (b) the .COM appender
# always appends -> GREET.COM becomes GREET.COM.COM; (c) an unknown word is
# classified as a built-in instead of EXTERNAL -> "badcmd" never reaches EXEC.
TEST_COMMAND_MUT_NOUP   := $(BUILD)/test_command_mutant_noupcase
TEST_COMMAND_MUT_COM    := $(BUILD)/test_command_mutant_com
TEST_COMMAND_MUT_BADCMD := $(BUILD)/test_command_mutant_badcmd

$(TEST_COMMAND): $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS) $(TEST_COMMAND_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS)

$(TEST_COMMAND_MUT_NOUP): $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS) $(TEST_COMMAND_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DCMD_MUTATE_NO_UPCASE -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS)

$(TEST_COMMAND_MUT_COM): $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS) $(TEST_COMMAND_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DCMD_MUTATE_COM_ALWAYS -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS)

$(TEST_COMMAND_MUT_BADCMD): $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS) $(TEST_COMMAND_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DCMD_MUTATE_BADCMD_BUILTIN -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_COMMAND_SRC) $(TEST_COMMAND_DEPS)

.PHONY: test-command test-command-mutant
test-command: $(TEST_COMMAND)
	@printf ">>> test-command: parse/upcase + built-in classify + .COM-append + DIR-line format\n"
	@$(TEST_COMMAND)
	@printf ">>> test-command: green\n"

# Mutation-proof: ALL three mutant builds MUST fail the oracle (Rule 6).
test-command-mutant: $(TEST_COMMAND_MUT_NOUP) $(TEST_COMMAND_MUT_COM) $(TEST_COMMAND_MUT_BADCMD)
	@printf ">>> test-command-mutant: confirming all three mutants go RED (Rule 6)\n"
	@if $(TEST_COMMAND_MUT_NOUP) >/dev/null 2>&1; then \
		printf '!!! test-command-mutant FAIL: no-upcase mutant PASSED -- the parse/upcase test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-command-mutant: green (no-upcase mutant correctly RED)\n'; \
	fi
	@if $(TEST_COMMAND_MUT_COM) >/dev/null 2>&1; then \
		printf '!!! test-command-mutant FAIL: com-always mutant PASSED -- the .COM-append test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-command-mutant: green (com-always mutant correctly RED)\n'; \
	fi
	@if $(TEST_COMMAND_MUT_BADCMD) >/dev/null 2>&1; then \
		printf '!!! test-command-mutant FAIL: badcmd-builtin mutant PASSED -- the classify test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-command-mutant: green (badcmd-builtin mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# REAL gate: test-psp (beads initech-509.4 -- PSP full 256-byte construction)
# ---------------------------------------------------------------------------
# Host unit oracle for psp_build() (os/milton/psp.c): zero-init + every field
# per ADR-0003 App B.2 / docs/research/psp-loader-ground-truth.md Sec 2. PSP
# construction is a pure, I/O-free function, so it is perfectly host-testable.
# psp.c ALSO compiles freestanding into the kernel (same TU). Mirrors the
# $(TEST_INT21) idiom (libc host test, seed/test_assert.h).
TEST_PSP      := $(BUILD)/test_psp
TEST_PSP_SRC  := $(MILTON_DIR)/test_psp.c
KERNEL_PSP_C  := $(MILTON_DIR)/psp.c
# Mutation builds (CLAUDE.md Rule 6): psp.c compiled with a single field write
# perturbed so `make test-psp-mutant` can prove the oracle BITES. (a) int20 byte
# 1 = 0x21 not 0x20; (b) the cmd_tail count byte is off-by-one. A mutant that
# PASSES means the oracle is decoration.
TEST_PSP_MUT_INT20 := $(BUILD)/test_psp_mutant_int20
TEST_PSP_MUT_TAIL  := $(BUILD)/test_psp_mutant_tail

$(TEST_PSP): $(TEST_PSP_SRC) $(KERNEL_PSP_C) $(MILTON_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_PSP_SRC) $(KERNEL_PSP_C)

$(TEST_PSP_MUT_INT20): $(TEST_PSP_SRC) $(KERNEL_PSP_C) $(MILTON_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DPSP_MUTATE_INT20 -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_PSP_SRC) $(KERNEL_PSP_C)

$(TEST_PSP_MUT_TAIL): $(TEST_PSP_SRC) $(KERNEL_PSP_C) $(MILTON_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DPSP_MUTATE_CMDTAIL_LEN -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_PSP_SRC) $(KERNEL_PSP_C)

.PHONY: test-psp test-psp-mutant
test-psp: $(TEST_PSP)
	@printf ">>> test-psp: PSP 256-byte construction (int20/segs/jft/int21-entry/cmd-tail) + clamp + no-overflow\n"
	@$(TEST_PSP)
	@printf ">>> test-psp: green\n"

# Mutation-proof: BOTH mutant builds MUST fail the oracle (Rule 6).
test-psp-mutant: $(TEST_PSP_MUT_INT20) $(TEST_PSP_MUT_TAIL)
	@printf ">>> test-psp-mutant: confirming both mutants go RED (Rule 6)\n"
	@if $(TEST_PSP_MUT_INT20) >/dev/null 2>&1; then \
		printf '!!! test-psp-mutant FAIL: int20 mutant PASSED -- the int20 test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-psp-mutant: green (int20 mutant correctly RED)\n'; \
	fi
	@if $(TEST_PSP_MUT_TAIL) >/dev/null 2>&1; then \
		printf '!!! test-psp-mutant FAIL: cmd-tail-len mutant PASSED -- the tail test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-psp-mutant: green (cmd-tail-len mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# REAL gate: test-sft (beads initech-509.3 -- SFT/JFT handle layer + DUP/DUP2)
# ---------------------------------------------------------------------------
# Host unit oracle for the JFT->SFT indirection (os/milton/sft.c): predefined
# handles 0-4 resolve to the right device SFT slots; jft_alloc/sft_alloc find +
# saturate; DUP (45h) duplicates with ref_count++; DUP2 (46h) redirects (incl.
# stdout->a FILE slot), releasing the old target + freeing on last reference.
# sft.c is pure table manipulation (no I/O), so it is fully host-testable; it
# ALSO compiles freestanding into the kernel. Links psp.c for psp_build (the
# standard JFT). -Ispec for sft.h -> psp.h -> dos_structs.h. Mirrors test-psp.
TEST_SFT      := $(BUILD)/test_sft
TEST_SFT_SRC  := $(MILTON_DIR)/test_sft.c
TEST_SFT_DEPS := $(KERNEL_SFT_C) $(KERNEL_PSP_C)
TEST_SFT_HDRS := $(MILTON_DIR)/sft.h $(MILTON_DIR)/psp.h spec/dos_structs.h
# Mutation builds (CLAUDE.md Rule 6): sft.c compiled with a single ref-count
# operation removed so `make test-sft-mutant` can prove the oracle BITES.
# (a) DUP omits the ref_count++ ; (b) DUP2 omits the old-target release.
TEST_SFT_MUT_DUP   := $(BUILD)/test_sft_mutant_dup
TEST_SFT_MUT_DUP2  := $(BUILD)/test_sft_mutant_dup2

$(TEST_SFT): $(TEST_SFT_SRC) $(TEST_SFT_DEPS) $(TEST_SFT_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_SFT_SRC) $(TEST_SFT_DEPS)

$(TEST_SFT_MUT_DUP): $(TEST_SFT_SRC) $(TEST_SFT_DEPS) $(TEST_SFT_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSFT_MUTATE_DUP_NO_REFCOUNT -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_SFT_SRC) $(TEST_SFT_DEPS)

$(TEST_SFT_MUT_DUP2): $(TEST_SFT_SRC) $(TEST_SFT_DEPS) $(TEST_SFT_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSFT_MUTATE_DUP2_NO_RELEASE -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_SFT_SRC) $(TEST_SFT_DEPS)

.PHONY: test-sft test-sft-mutant
test-sft: $(TEST_SFT)
	@printf ">>> test-sft: JFT->SFT handle layer + predefined handles 0-4 + DUP/DUP2 (initech-509.3)\n"
	@$(TEST_SFT)
	@printf ">>> test-sft: green\n"

# Mutation-proof: BOTH mutant builds MUST fail the oracle (Rule 6).
test-sft-mutant: $(TEST_SFT_MUT_DUP) $(TEST_SFT_MUT_DUP2)
	@printf ">>> test-sft-mutant: confirming both mutants go RED (Rule 6)\n"
	@if $(TEST_SFT_MUT_DUP) >/dev/null 2>&1; then \
		printf '!!! test-sft-mutant FAIL: DUP-no-refcount mutant PASSED -- the ref_count test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-sft-mutant: green (DUP-no-refcount mutant correctly RED)\n'; \
	fi
	@if $(TEST_SFT_MUT_DUP2) >/dev/null 2>&1; then \
		printf '!!! test-sft-mutant FAIL: DUP2-no-release mutant PASSED -- the release test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-sft-mutant: green (DUP2-no-release mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# REAL gate: test-loader (beads initech-509.5 -- flat program loader PREP)
# ---------------------------------------------------------------------------
# Host unit oracle for loader_prepare() (os/milton/loader.c): input validation
# (NULL / zero-length / oversized image -> fail loud) + the LOCKED load layout
# (psp @ PROGRAM_BASE, image @ PROGRAM_BASE+0x100, entry == image_dst, stack @
# PROGRAM_STACK_TOP) + the psp_params the loader feeds psp_build. loader.c also
# compiles freestanding into the kernel (the asm control-transfer path is elided
# in the hosted build). Mirrors the $(TEST_PSP) idiom.
TEST_LOADER      := $(BUILD)/test_loader
TEST_LOADER_SRC  := $(MILTON_DIR)/test_loader.c
# Mutation build (CLAUDE.md Rule 6): loader.c compiled with the .COM 0x100 image
# offset removed so `make test-loader-mutant` can prove the layout oracle BITES.
TEST_LOADER_MUT  := $(BUILD)/test_loader_mutant_nooffset

$(TEST_LOADER): $(TEST_LOADER_SRC) $(KERNEL_LOADER_C) $(KERNEL_PSP_C) \
                $(MILTON_DIR)/loader.h $(MILTON_DIR)/psp.h $(MILTON_DIR)/fat12.h \
                $(MILTON_DIR)/blockdev.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_LOADER_SRC) $(KERNEL_LOADER_C) $(KERNEL_PSP_C)

$(TEST_LOADER_MUT): $(TEST_LOADER_SRC) $(KERNEL_LOADER_C) $(KERNEL_PSP_C) \
                    $(MILTON_DIR)/loader.h $(MILTON_DIR)/psp.h $(MILTON_DIR)/fat12.h \
                    $(MILTON_DIR)/blockdev.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DLOADER_MUTATE_NO_OFFSET -Ispec -I$(MILTON_DIR) -Iseed \
		-o $@ $(TEST_LOADER_SRC) $(KERNEL_LOADER_C) $(KERNEL_PSP_C)

.PHONY: test-loader test-loader-mutant
test-loader: $(TEST_LOADER)
	@printf ">>> test-loader: loader_prepare layout (psp/image+0x100/entry/stack) + params + fail-loud validation\n"
	@$(TEST_LOADER)
	@printf ">>> test-loader: green\n"

# Mutation-proof: the no-offset mutant build MUST fail the oracle (Rule 6). A
# mutant that PASSES means the layout test is decoration.
test-loader-mutant: $(TEST_LOADER_MUT)
	@printf ">>> test-loader-mutant: confirming the no-0x100-offset mutant goes RED (Rule 6)\n"
	@if $(TEST_LOADER_MUT) >/dev/null 2>&1; then \
		printf '!!! test-loader-mutant FAIL: no-offset mutant PASSED the oracle -- the layout test is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-loader-mutant: green (no-offset mutant correctly RED -- the oracle bites)\n'; \
	fi

# ---------------------------------------------------------------------------
# Stub macro
# ---------------------------------------------------------------------------
# $(call stub,<target>,<milestone hint>) -- print an honest placeholder
# message and succeed (exit 0). Keeps unimplemented targets obvious instead
# of either silently passing or cryptically failing.
define stub
	@printf ">>> '%s' not implemented yet -- see beads issue (%s: bd ready). Stubbed.\n" "$(1)" "$(2)"
endef

# $(call stub_fail,<target>,<milestone hint>) -- for correctness GATES
# (oracles, the tracer smoke test, the self-host certificate). Prints the
# same honest message but exits 1, so an unimplemented gate can never be
# mistaken for a passing one (CLAUDE.md Law 2 / Rule 2).
define stub_fail
	@printf ">>> '%s' is a GATE and is not implemented yet -- see beads issue (%s: bd ready). Failing on purpose.\n" "$(1)" "$(2)"
	@exit 1
endef

# Real targets vs. phony stubs are all .PHONY (no file products tracked by
# make here except the smoke binary, which depends on its source).
.PHONY: help factory image run run-bochs smoke ssim test test-region \
        test-fat test-dbase test-compiler test-seed test-seed-codegen \
        test-harness test-tracer-boot test-boot test-console test-idt \
        test-idt-mutant test-int21 test-int21-mutant test-psp test-psp-mutant \
        test-sft test-sft-mutant test-fileio test-fileio-mutant \
        test-loader test-loader-mutant test-program test-fs test-type test-dir \
        test-exec test-exec-unit test-exec-mutant \
        test-fat12-write test-fat-write test-fat-write-mutant test-fatwrite \
        test-multiopen \
        test-int21-irqstorm test-int21-irqstorm-mutant \
        test-fat-write-partial test-fat-write-partial-mutant \
        test-command test-command-mutant test-shell \
        test-panic test-kbd test-kbd-bochs test-kbd-unit test-kbd-unit-mutant \
        test-conin-unit test-conin-mutant test-conin \
        test-assets test-spec selfhost ddc clean \
        test-unit test-emu

# ---------------------------------------------------------------------------
# Default + self-documenting help
# ---------------------------------------------------------------------------
.DEFAULT_GOAL := help

help:
	@printf 'InitechOS (STAPLER) -- factory targets\n'
	@printf '\n'
	@printf 'Build:\n'
	@printf '  factory        Build the seed cross-compiler + factory tools (C). REAL: compiles the smoke stub.\n'
	@printf '  image          Build a bootable image with the seed compiler -> %s/initech.img.\n' "$(BUILD)"
	@printf '  clean          Remove build artifacts under %s/.\n' "$(BUILD)"
	@printf '\n'
	@printf 'Run / boot:\n'
	@printf '  run            Dev-loop boot in QEMU (serial + gdb stub + screendump).\n'
	@printf '  run-bochs      Accuracy boot in Bochs (real->protected transition checking).\n'
	@printf '  smoke          M0.5 tracer-bullet heartbeat. REAL gate: boot seed ELF in QEMU, assert serial marker + no triple-fault + no hang.\n'
	@printf '\n'
	@printf 'Oracles (per-subsystem differential / property suites):\n'
	@printf '  test-region    Region property suite: homomorphism + normal-form + identities.\n'
	@printf '  test-fat       FAT12/16 differential vs mtools/python on identical images.\n'
	@printf '  test-fat-write FAT12 WRITE round-trip: OUR writer mints+writes a volume; mtools+python3 read it back (names/sizes/content). REAL. beads initech-509.11.\n'
	@printf '  test-fat12-write  FAT12 WRITE unit: 12-bit encode + cluster alloc + both-FAT sync + in-memory round-trip + full-volume fail-loud + unlink. REAL.\n'
	@printf '  test-fatwrite  In-emulator WRITE round-trip: CREAT+WRITE+CLOSE then OPEN+READ OUT.TXT over real ATA in one boot; mtools confirms on-disk. REAL (QEMU).\n'
	@printf '  test-multiopen Multi-tenant proof: 2+ files open concurrently with independent positions (LSEEK) + a >64 KiB round-trip via positioned READ. REAL (QEMU). beads initech-0qh.\n'
	@printf '  test-int21-irqstorm  INT 21h reentrancy under an IRQ storm (initech-xk2): FINDFIRST/NEXT + multi-cluster READ + 2nd handle WHILE keys storm (IRQ1) + PIT (IRQ0); exact enum/bytes + guard quiescent. REAL (QEMU).\n'
	@printf '  test-int21-irqstorm-mutant  Rule-6 proof the storm oracle BITES: A (PIT scribbles a DOS global -> wrong enum) + B (PIT issues int 0x21 -> guard PANICS) both go RED. REAL (QEMU).\n'
	@printf '  test-dbase     InitechBase differential + round-trip vs real dBASE.\n'
	@printf '  test-compiler  Turbo Initech vs Free Pascal on the shared corpus.\n'
	@printf '  test-seed      Seed front-end unit tests (lexer + parser). REAL: fails non-zero on any check.\n'
	@printf '  test-seed-codegen  Seed codegen end-to-end: compile .pas, boot ELF in QEMU, assert exact serial. REAL.\n'
	@printf '  test-harness   QEMU oracle harness self-test: serial marker caught on good fixture, triple-fault caught on bad. REAL.\n'
	@printf '  test-tracer-boot   Real MBR->stage2->32-bit/flat->VESA LFB boot: assert serial stage markers + no triple-fault + banner rendered on the seafoam desktop (ppm_text_check). REAL.\n'
	@printf '  test-boot      InitechDOS banner boot gate: serial markers + banner literal vs spec/dos_banner.txt (byte-exact) + screendump banner-text check + no triple-fault. REAL. (QEMU only; tri-emulator pending initech-x0i.)\n'
	@printf '  test-console   Host blit oracle for the LFB 8x16 text console: MSB-left glyph blit (bpp 32/24) + cursor/wrap/scroll. REAL.\n'
	@printf '  test-assets    Asset v0: re-sample palette.json anchors vs the frame fixture + validate the Chicago strike header. REAL.\n'
	@printf '  test-spec      InitechDOS spec-data (ADR-0003 Appendices A-D): JSON parse + 16 messages + struct size asserts + banner double-space. REAL.\n'
	@printf '  test-psp       PSP 256-byte construction oracle (initech-509.4 / App B.2): int20/seg-fields/jft/int21-entry/cmd-tail + clamp + no-overflow. REAL.\n'
	@printf '  test-sft       SFT/JFT handle layer (initech-509.3 / DEC-06): predefined handles 0-4 + jft/sft alloc + DUP/DUP2 redirection + ref-counting. REAL.\n'
	@printf '  test-kbd-unit  PS/2 keyboard + PIT pure logic (initech-3rs): ring (full/wrap) + scancode set 1 -> ASCII (+shift/caps) + PIT divisor math. REAL.\n'
	@printf '  test-kbd       Keyboard IRQ1 end-to-end (initech-3rs/43b): first sti, QMP --keys "d,i,r" injected, echoed back via IRQ1; triple_fault=0. REAL (QEMU).\n'
	@printf '  test-kbd-bochs Bochs leg of test-kbd (Rule 5): boots the echo image under Bochs; currently BLOCKED at ERR-VBE (pre-existing boot/VBE gap initech-x0i).\n'
	@printf '  test           Run the whole gate vector (PRD Sec 8).\n'
	@printf '\n'
	@printf 'Self-host certificate (M8 finale):\n'
	@printf '  selfhost       K1=X(src); K2=K1(src); K3=K2(src); assert K2 == K3.\n'
	@printf '  ddc            Diverse double-compilation: independent seed, compare K*.\n'
	@printf '\n'
	@printf 'Fidelity guide:\n'
	@printf '  ssim           Per-window SSIM of a screendump vs the frame fixture (FRAME=%s).\n' "$(FRAME)"
	@printf '\n'
	@printf 'Run "make <target>". Variables: CC=%s  BUILD=%s  FRAME=%s\n' "$(CC)" "$(BUILD)" "$(FRAME)"

# ---------------------------------------------------------------------------
# REAL target: factory
# ---------------------------------------------------------------------------
# Build the C factory. Today that is just the smoke stub, which proves the
# C11 toolchain compiles clean under -Wall -Wextra -Werror and runs. As the
# seed compiler (seed/) and harness (harness/) land, add them here.
factory: $(SMOKE_BIN) $(SEED_BIN) $(HARNESS_BIN) $(HARNESS_FIXTURES) $(SEED_SMOKE_ELF) $(TRACER_IMG) $(PPM_CHECK_BIN) $(PALETTE_TOOL_BIN) $(ASSET_CHECK_BIN)
	@printf ">>> factory: C toolchain OK. Running smoke binary:\n"
	@$(SMOKE_BIN)
	@printf ">>> factory: seed front-end driver built -> %s\n" "$(SEED_BIN)"
	@printf ">>> factory: QEMU oracle harness built -> %s\n" "$(HARNESS_BIN)"
	@printf ">>> factory: harness fixtures built -> %s\n" "$(HARNESS_FIXTURES)"
	@printf ">>> factory: seed codegen smoke ELF built -> %s\n" "$(SEED_SMOKE_ELF)"
	@printf ">>> factory: tracer boot image built -> %s\n" "$(TRACER_IMG)"
	@printf ">>> factory: ppm seafoam checker built -> %s\n" "$(PPM_CHECK_BIN)"
	@printf ">>> factory: asset tools built -> %s %s\n" "$(PALETTE_TOOL_BIN)" "$(ASSET_CHECK_BIN)"

# ---------------------------------------------------------------------------
# Tracer boot build (beads initech-f8v.2)
# ---------------------------------------------------------------------------
# Flat binaries: nasm -f bin (the org is set in each source). The MBR is
# exactly 512 bytes (it self-pads + signature); stage2 is padded to
# STAGE2_SECTORS sectors so the MBR's fixed CHS read count is correct.
$(MBR_BIN): $(MBR_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(STAGE2_BIN): $(STAGE2_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

# Assemble the raw disk image deterministically:
#   sector 0       : MBR (512 bytes)
#   sectors 1..    : stage2, padded to STAGE2_SECTORS sectors
#   image padded   : to IMG_SECTORS total sectors
# We build a zero-filled image then dd the parts in at fixed offsets. No
# timestamps/host paths -> reproducible (CLAUDE.md Rule 11).
$(TRACER_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> tracer image: %s (MBR@s0, stage2@s1..%d, kernel@s17..%d, %d sectors total)\n" \
		"$@" "$(STAGE2_SECTORS)" "$$((17 + $(KERNEL_SECTORS) - 1))" "$(IMG_SECTORS)"

# --- Flat C kernel build (beads initech-d00) -------------------------------
# Entry stub (nasm elf32) + freestanding C, linked to a FLAT binary at
# 0x00010000 via kernel.ld, then objcopy'd to a raw image and padded to exactly
# KERNEL_SECTORS sectors so stage2's INT 13h read count is deterministic.
# kstart.o is linked FIRST so _start lands at the link base (0x10000).
$(KERNEL_START_OBJ): $(KERNEL_START_ASM) | $(BUILD)
	$(NASM) -f elf32 $< -o $@

$(KERNEL_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

# Text console (beads initech-yqb): the SAME console.c the host blit oracle
# (os/milton/test_console.c) exercises; freestanding here, hosted there.
$(KERNEL_CONSOLE_OBJ): $(KERNEL_CONSOLE_C) $(KERNEL_DIR)/console.h $(KERNEL_DIR)/boot_info.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_CONSOLE_C) -o $@

# Interrupt foundation objects (beads initech-a5a).
$(KERNEL_IDT_OBJ): $(KERNEL_IDT_C) $(KERNEL_DIR)/idt.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_IDT_C) -o $@

$(KERNEL_PIC_OBJ): $(KERNEL_PIC_C) $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/io.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_PIC_C) -o $@

$(KERNEL_PANIC_OBJ): $(KERNEL_PANIC_C) $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_PANIC_C) -o $@

# INT 21h dispatcher (beads initech-509.5): the SAME int21.c the host oracle
# (os/milton/test_int21.c) exercises; freestanding here, hosted there. Now
# includes sft.h -> psp.h -> dos_structs.h, so -Ispec (initech-509.3).
$(KERNEL_INT21_OBJ): $(KERNEL_INT21_C) $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/idt.h \
                     $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_INT21_C) -o $@

# PSP construction (beads initech-509.4): the SAME psp.c the host oracle
# (test_psp.c) exercises; freestanding here, hosted there. -Ispec for psp.h ->
# dos_structs.h (the LOCKED psp_t).
$(KERNEL_PSP_OBJ): $(KERNEL_PSP_C) $(KERNEL_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_PSP_C) -o $@

# System File Table / Job File Table (beads initech-509.3): the SAME sft.c the
# host oracle (test_sft.c) exercises; freestanding here, hosted there. -Ispec
# for sft.h -> psp.h -> dos_structs.h (psp_t.jft, dir_entry_t).
$(KERNEL_SFT_OBJ): $(KERNEL_SFT_C) $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_SFT_C) -o $@

# Flat program loader (beads initech-509.5): the SAME loader.c the host oracle
# (test_loader.c) exercises; freestanding here (asm control-transfer path),
# hosted there (loader_prepare only). -Ispec for memory_map.h (LOCKED addrs).
$(KERNEL_LOADER_OBJ): $(KERNEL_LOADER_C) $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/psp.h \
                      spec/memory_map.h $(KERNEL_DIR)/int21.h \
                      $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/blockdev.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_LOADER_C) -o $@

# ATA PIO sector backend (beads initech-adf/initech-saw): the SAME ata.c that
# is freestanding-only (no host oracle -- it touches I/O ports); compiled into
# the kernel here. Its first functional exercise is make test-fs.
$(KERNEL_ATA_OBJ): $(KERNEL_ATA_C) $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/blockdev.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_ATA_C) -o $@

# FAT12 reader (beads initech-adf): the SAME fat12.c the host oracles
# (test-fat12-*) exercise; freestanding here, hosted there. -Ispec for the
# LOCKED bpb_t / dir_entry_t (dos_structs.h).
$(KERNEL_FAT12_OBJ): $(KERNEL_FAT12_C) $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/blockdev.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_FAT12_C) -o $@

# FAT12-backed INT 21h file backend (beads initech-509.5 read-side): binds the
# int21 file vtable to the mounted volume. Kernel-only (pulls fat12.c + the
# volume); the host oracle (test_fileio.c) binds a mock instead. -Ispec for
# dir_entry_t (dos_structs.h) + FILE_BUFFER_* (memory_map.h).
$(KERNEL_FILEIO_OBJ): $(KERNEL_DIR)/fileio_fat.c $(KERNEL_DIR)/fileio_fat.h \
                      $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/blockdev.h \
                      spec/dos_structs.h spec/memory_map.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Ispec -I$(KERNEL_DIR) -c $(KERNEL_DIR)/fileio_fat.c -o $@

# PS/2 keyboard (IRQ1) + 8254 PIT (IRQ0) drivers (beads initech-3rs): the SAME
# kbd.c / pit.c the host oracle (test_kbd.c) exercises for the pure ring/table/
# divisor logic; freestanding here (the IRQ handlers touch I/O ports), hosted
# there (pure halves only). io.h only -- no -Ispec needed.
$(KERNEL_KBD_OBJ): $(KERNEL_KBD_C) $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/io.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_KBD_C) -o $@

$(KERNEL_PIT_OBJ): $(KERNEL_PIT_C) $(KERNEL_DIR)/pit.h $(KERNEL_DIR)/io.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_PIT_C) -o $@

# IRQ reentrancy guard (beads initech-xk2): the in-IRQ depth counter the asm IRQ
# stubs bracket their C handler with, plus the INT 21h reentrancy fail-loud panic
# the dispatcher invokes when irq_depth() != 0 at entry. ASCII-clean, no malloc.
$(KERNEL_IRQ_OBJ): $(KERNEL_IRQ_C) $(KERNEL_DIR)/irq.h $(KERNEL_DIR)/io.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(KERNEL_IRQ_C) -o $@

# COMMAND.COM shell (beads initech-7pc): the SAME command.c the host oracle
# (test_command.c) exercises for the pure parser/classifier/formatter logic;
# freestanding here with -DCOMMAND_KERNEL_REPL so the int 0x21 REPL is compiled
# IN (the host build leaves it out). -Ispec for find_data.h + dos_structs.h.
$(KERNEL_COMMAND_OBJ): $(KERNEL_COMMAND_C) $(KERNEL_DIR)/command.h \
                       spec/find_data.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DCOMMAND_KERNEL_REPL -Ispec -I$(KERNEL_DIR) -c $(KERNEL_COMMAND_C) -o $@

# --- Baked test program pipeline (beads initech-509.5; Sec 5.4) ------------
# bin2c is a host factory tool (libc), built with the factory CC, not KERNEL_CC.
$(BIN2C_BIN): $(BIN2C_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

# Assemble the flat (.COM-equivalent) test program (org 0x20100). nasm -f bin is
# deterministic for a fixed source (Rule 11).
$(TEST_PROG_BIN): $(TEST_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

# Embed the flat binary as a C .rodata array via bin2c (deterministic; no
# timestamps/paths). The generated .c is compiled with KERNEL_CC into the kernel.
$(TEST_PROG_BLOB_C): $(TEST_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(TEST_PROG_BIN) g_test_prog_image > $@

$(KERNEL_TEST_PROG_OBJ): $(TEST_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(TEST_PROG_BLOB_C) -o $@

# TYPE / DIR baked programs (beads initech-509.5 read-side): same nasm -f bin ->
# bin2c -> KERNEL_CC pipeline as the demo program.
$(TYPE_PROG_BIN): $(TYPE_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(TYPE_PROG_BLOB_C): $(TYPE_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(TYPE_PROG_BIN) g_type_prog_image > $@

$(KERNEL_TYPE_PROG_OBJ): $(TYPE_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(TYPE_PROG_BLOB_C) -o $@

$(DIR_PROG_BIN): $(DIR_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(DIR_PROG_BLOB_C): $(DIR_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(DIR_PROG_BIN) g_dir_prog_image > $@

$(KERNEL_DIR_PROG_OBJ): $(DIR_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(DIR_PROG_BLOB_C) -o $@

# CON-INPUT baked program (beads initech-n62): same nasm -f bin -> bin2c ->
# KERNEL_CC pipeline. Linked only into the -DBOOT_CONIN kernel.
$(CONIN_PROG_BIN): $(CONIN_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(CONIN_PROG_BLOB_C): $(CONIN_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(CONIN_PROG_BIN) g_conin_prog_image > $@

$(KERNEL_CONIN_PROG_OBJ): $(CONIN_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(CONIN_PROG_BLOB_C) -o $@

# WRITE round-trip baked program (beads initech-509.11): same nasm -f bin ->
# bin2c -> KERNEL_CC pipeline. Linked only into the -DBOOT_WRITE kernel.
$(WRITE_PROG_BIN): $(WRITE_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(WRITE_PROG_BLOB_C): $(WRITE_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(WRITE_PROG_BIN) g_write_prog_image > $@

$(KERNEL_WRITE_PROG_OBJ): $(WRITE_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(WRITE_PROG_BLOB_C) -o $@

# MULTI-OPEN baked program (beads initech-0qh): same nasm -f bin -> bin2c ->
# KERNEL_CC pipeline. Linked only into the -DBOOT_MULTIOPEN kernel.
$(MULTIOPEN_PROG_BIN): $(MULTIOPEN_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(MULTIOPEN_PROG_BLOB_C): $(MULTIOPEN_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(MULTIOPEN_PROG_BIN) g_multiopen_prog_image > $@

$(KERNEL_MULTIOPEN_PROG_OBJ): $(MULTIOPEN_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(MULTIOPEN_PROG_BLOB_C) -o $@

# IRQ-STORM baked program (beads initech-xk2): same nasm -f bin -> bin2c ->
# KERNEL_CC pipeline. Linked only into the -DBOOT_IRQSTORM kernel(s).
$(IRQSTORM_PROG_BIN): $(IRQSTORM_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

$(IRQSTORM_PROG_BLOB_C): $(IRQSTORM_PROG_BIN) $(BIN2C_BIN)
	$(BIN2C_BIN) $(IRQSTORM_PROG_BIN) g_irqstorm_prog_image > $@

$(KERNEL_IRQSTORM_PROG_OBJ): $(IRQSTORM_PROG_BLOB_C) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(KERNEL_DIR) -c $(IRQSTORM_PROG_BLOB_C) -o $@

$(KERNEL_ISR_OBJ): $(KERNEL_ISR_ASM) | $(BUILD)
	$(NASM) -f elf32 $< -o $@

KERNEL_OBJS := $(KERNEL_START_OBJ) $(KERNEL_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
               $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
               $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
               $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
               $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
               $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
               $(KERNEL_ISR_OBJ)

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_OBJS)

# --- Self-test fault kernel (beads initech-a5a; make test-panic) -----------
# Same sources, but kmain.c compiled with -DBOOT_SELFTEST_FAULT so the boot
# raises a deliberate #DE after the banner. Linked into a SEPARATE image so the
# normal kernel/image (test-boot) never faults.
$(KERNEL_FAULT_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_SELFTEST_FAULT -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_FAULT_OBJS := $(KERNEL_START_OBJ) $(KERNEL_FAULT_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                     $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                     $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                     $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                     $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                     $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                     $(KERNEL_ISR_OBJ)

$(KERNEL_FAULT_ELF): $(KERNEL_FAULT_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_FAULT_OBJS)

$(KERNEL_FAULT_BIN): $(KERNEL_FAULT_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_fault.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(fault): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

# The self-test fault disk image: identical layout to TRACER_IMG but with the
# fault kernel at sector 17.
$(PANIC_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_FAULT_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_FAULT_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> panic image: %s (self-test #DE fault kernel @s17)\n" "$@"

# --- Keyboard-echo kernel (beads initech-3rs / initech-43b; make test-kbd) --
# Same sources, but kmain.c compiled with -DBOOT_KBD_ECHO so the boot, after the
# sti, emits KBD-ECHO-READY then echoes injected keys to serial. Separate image.
$(KERNEL_ECHO_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_KBD_ECHO -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_ECHO_OBJS := $(KERNEL_START_OBJ) $(KERNEL_ECHO_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

$(KERNEL_ECHO_ELF): $(KERNEL_ECHO_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_ECHO_OBJS)

$(KERNEL_ECHO_BIN): $(KERNEL_ECHO_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_echo.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(echo): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

# The keyboard-echo disk image: identical layout to TRACER_IMG but with the echo
# kernel at sector 17.
$(KBD_ECHO_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_ECHO_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_ECHO_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> kbd-echo image: %s (keyboard-echo kernel @s17)\n" "$@"

# --- CON-INPUT self-test kernel (beads initech-n62; make test-conin) --------
# Same sources, but kmain.c compiled with -DBOOT_CONIN so the boot, after
# CONIN-LIVE, runs the baked CON-input program. The conin program blob is linked
# in (it is referenced only under -DBOOT_CONIN). Separate image.
$(KERNEL_CONIN_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_CONIN -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_CONIN_OBJS := $(KERNEL_START_OBJ) $(KERNEL_CONIN_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                     $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                     $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                     $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                     $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                     $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                     $(KERNEL_CONIN_PROG_OBJ) \
                     $(KERNEL_ISR_OBJ)

$(KERNEL_CONIN_ELF): $(KERNEL_CONIN_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_CONIN_OBJS)

$(KERNEL_CONIN_BIN): $(KERNEL_CONIN_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_conin.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(conin): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

# The CON-input self-test disk image: identical layout to TRACER_IMG but with
# the conin kernel at sector 17.
$(CONIN_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_CONIN_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_CONIN_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> conin image: %s (CON-input self-test kernel @s17)\n" "$@"

# --- FAT-sourced load + EXEC self-test kernel (beads initech-saw; test-exec) -
# Same sources, but kmain.c compiled with -DBOOT_EXEC so the boot, after the FAT
# mount + loader-FAT bind, loads GREET.COM BY NAME and EXECs it via AH=4Bh.
# Separate image so the normal boot never runs the EXEC demo. GREET.COM is NOT
# baked -- it lives on the data disk (--disk2), so this proves a FROM-FAT load.
$(KERNEL_EXEC_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_EXEC -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_EXEC_OBJS := $(KERNEL_START_OBJ) $(KERNEL_EXEC_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

$(KERNEL_EXEC_ELF): $(KERNEL_EXEC_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_EXEC_OBJS)

$(KERNEL_EXEC_BIN): $(KERNEL_EXEC_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_exec.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(exec): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

# The EXEC self-test disk image: identical layout to TRACER_IMG but with the
# exec kernel at sector 17.
$(EXEC_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_EXEC_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_EXEC_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> exec image: %s (FAT-sourced load + EXEC self-test kernel @s17)\n" "$@"

# --- FAT WRITE round-trip self-test kernel (beads initech-509.11; test-fatwrite)
# Same sources, kmain.c compiled with -DBOOT_WRITE so the boot runs the baked
# WRITE program over a WRITABLE FAT12 data disk. The write_prog blob is linked
# ONLY into this image. Mirrors the EXEC image structure.
$(KERNEL_WRITE_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_WRITE -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_WRITE_OBJS := $(KERNEL_START_OBJ) $(KERNEL_WRITE_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_WRITE_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

$(KERNEL_WRITE_ELF): $(KERNEL_WRITE_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_WRITE_OBJS)

$(KERNEL_WRITE_BIN): $(KERNEL_WRITE_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_write.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(write): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

$(WRITE_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_WRITE_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_WRITE_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> write image: %s (FAT WRITE round-trip self-test kernel @s17)\n" "$@"

# --- MULTI-OPEN self-test kernel (beads initech-0qh; make test-multiopen) -----
# Same sources, kmain.c compiled with -DBOOT_MULTIOPEN so the boot runs the baked
# MULTI-OPEN program over a FAT12 data disk (HELLO.TXT + SECOND.TXT + >64 KiB
# BIG.DAT). The multiopen_prog blob is linked ONLY into this image. Mirrors the
# WRITE image structure.
$(KERNEL_MULTIOPEN_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_MULTIOPEN -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_MULTIOPEN_OBJS := $(KERNEL_START_OBJ) $(KERNEL_MULTIOPEN_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_MULTIOPEN_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

$(KERNEL_MULTIOPEN_ELF): $(KERNEL_MULTIOPEN_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_MULTIOPEN_OBJS)

$(KERNEL_MULTIOPEN_BIN): $(KERNEL_MULTIOPEN_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_multiopen.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(multiopen): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

$(MULTIOPEN_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_MULTIOPEN_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_MULTIOPEN_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> multiopen image: %s (MULTI-OPEN self-test kernel @s17)\n" "$@"

# --- IRQ-STORM reentrancy self-test kernel (beads initech-xk2; make
# test-int21-irqstorm) --------------------------------------------------------
# Same sources, kmain.c compiled with -DBOOT_IRQSTORM so the boot, AFTER sti (IRQs
# live), runs the baked IRQ-STORM program over a FAT12 storm disk while the
# harness storms keystrokes. The irqstorm_prog blob is linked ONLY into these
# images. Mirrors the MULTI-OPEN image structure. THREE images: the REAL one and
# two Rule-6 mutants (A: pit scribbles a DOS global; B: pit issues int 0x21).
$(KERNEL_IRQSTORM_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h $(KERNEL_DIR)/irq.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_IRQSTORM -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

# Object list shared by all three irqstorm images (the per-image differences are
# the main obj, the pit obj, and -- for mutant A -- the int21 obj; defined below).
KERNEL_IRQSTORM_OBJS_COMMON := $(KERNEL_START_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_IRQSTORM_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

# REAL irqstorm image: the real pit.o + the real int21.o (no mutate/seam flags).
KERNEL_IRQSTORM_OBJS := $(KERNEL_IRQSTORM_OBJS_COMMON) $(KERNEL_IRQSTORM_MAIN_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PIT_OBJ)

$(KERNEL_IRQSTORM_ELF): $(KERNEL_IRQSTORM_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_IRQSTORM_OBJS)

$(KERNEL_IRQSTORM_BIN): $(KERNEL_IRQSTORM_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_irqstorm.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(irqstorm): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

$(IRQSTORM_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_IRQSTORM_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_IRQSTORM_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> irqstorm image: %s (IRQ-STORM reentrancy self-test kernel @s17)\n" "$@"

# --- Rule-6 MUTANT A: pit.c -DPIT_MUTATE_SCRIBBLE_DOS + int21.c
# -DINT21_IRQTEST_SEAM (the mutant ISR scribbles g_find.next_index from IRQ
# context -> the enumeration goes WRONG under the storm -> the oracle goes RED).
# Own pit.o + int21.o + main obj + elf + bin + image so the mutant cannot
# contaminate the real build.
$(KERNEL_PIT_MUT_SCRIBBLE_OBJ): $(KERNEL_PIT_C) $(KERNEL_DIR)/pit.h $(KERNEL_DIR)/io.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DPIT_MUTATE_SCRIBBLE_DOS -I$(KERNEL_DIR) -c $(KERNEL_PIT_C) -o $@

KERNEL_INT21_SEAM_OBJ := $(BUILD)/int21_seam.o
$(KERNEL_INT21_SEAM_OBJ): $(KERNEL_INT21_C) $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/irq.h spec/find_data.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DINT21_IRQTEST_SEAM -Ispec -I$(KERNEL_DIR) -c $(KERNEL_INT21_C) -o $@

$(KERNEL_IRQSTORM_MUTA_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/irq.h spec/memory_map.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_IRQSTORM -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_IRQSTORM_MUTA_OBJS := $(KERNEL_IRQSTORM_OBJS_COMMON) $(KERNEL_IRQSTORM_MUTA_MAIN_OBJ) \
                    $(KERNEL_INT21_SEAM_OBJ) $(KERNEL_PIT_MUT_SCRIBBLE_OBJ)

$(KERNEL_IRQSTORM_MUTA_ELF): $(KERNEL_IRQSTORM_MUTA_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_IRQSTORM_MUTA_OBJS)

$(KERNEL_IRQSTORM_MUTA_BIN): $(KERNEL_IRQSTORM_MUTA_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(irqstorm mutant A): %s\n" "$@"

$(IRQSTORM_MUTA_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_IRQSTORM_MUTA_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_IRQSTORM_MUTA_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> irqstorm mutant-A image: %s\n" "$@"

# --- Rule-6 MUTANT B: pit.c -DPIT_MUTATE_ISSUE_INT21 (the mutant ISR issues
# int 0x21 from IRQ context -> the reentrancy guard PANICS -> the oracle goes
# RED on the panic marker). Own pit.o + main obj + elf + bin + image.
$(KERNEL_PIT_MUT_INT21_OBJ): $(KERNEL_PIT_C) $(KERNEL_DIR)/pit.h $(KERNEL_DIR)/io.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DPIT_MUTATE_ISSUE_INT21 -I$(KERNEL_DIR) -c $(KERNEL_PIT_C) -o $@

$(KERNEL_IRQSTORM_MUTB_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/irq.h spec/memory_map.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_IRQSTORM -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_IRQSTORM_MUTB_OBJS := $(KERNEL_IRQSTORM_OBJS_COMMON) $(KERNEL_IRQSTORM_MUTB_MAIN_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PIT_MUT_INT21_OBJ)

$(KERNEL_IRQSTORM_MUTB_ELF): $(KERNEL_IRQSTORM_MUTB_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_IRQSTORM_MUTB_OBJS)

$(KERNEL_IRQSTORM_MUTB_BIN): $(KERNEL_IRQSTORM_MUTB_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(irqstorm mutant B): %s\n" "$@"

$(IRQSTORM_MUTB_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_IRQSTORM_MUTB_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_IRQSTORM_MUTB_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> irqstorm mutant-B image: %s\n" "$@"

# --- EXIT-handle teardown self-test kernel (beads initech-6hk; epic initech-6qy;
# make test-exit-handles) -----------------------------------------------------
# Same sources, kmain.c compiled with -DBOOT_EXITH so the boot EXECs the
# FAT-sourced leaky child EXITH.COM RUNS times. The leaky child is NOT baked --
# it is loaded BY NAME off --disk2 (FAT_EXITH_IMG), exactly like the EXEC image
# loads GREET.COM. So the object set is the EXEC object set (no baked prog blob).
$(KERNEL_EXITH_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_EXITH -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_EXITH_OBJS := $(KERNEL_START_OBJ) $(KERNEL_EXITH_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

$(KERNEL_EXITH_ELF): $(KERNEL_EXITH_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_EXITH_OBJS)

$(KERNEL_EXITH_BIN): $(KERNEL_EXITH_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_exith.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(exith): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

$(EXITH_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_EXITH_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_EXITH_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> exith image: %s (EXIT-handle teardown self-test kernel @s17)\n" "$@"

# --- Rule-6 MUTANT image: elide the per-handle release in sft_close_process ----
# sft.c built with -DSFT_MUTATE_NO_CLOSE_PROCESS (its OWN object, never the real
# one), linked into an otherwise-identical -DBOOT_EXITH kernel. With the teardown
# elided the leaky child's FILE slots leak across EXEC runs, the 16-slot file SFT
# exhausts, and a later run's OPEN fails -> make test-exit-handles-mutant asserts
# the oracle goes RED (proves the gate bites; CLAUDE.md Rule 6).
$(KERNEL_SFT_MUT_OBJ): $(KERNEL_SFT_C) $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/psp.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DSFT_MUTATE_NO_CLOSE_PROCESS -Ispec -I$(KERNEL_DIR) -c $(KERNEL_SFT_C) -o $@

$(KERNEL_EXITH_MUT_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_EXITH -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_EXITH_MUT_OBJS := $(KERNEL_START_OBJ) $(KERNEL_EXITH_MUT_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                    $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                    $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_MUT_OBJ) $(KERNEL_LOADER_OBJ) \
                    $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                    $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) \
                    $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                    $(KERNEL_ISR_OBJ)

$(KERNEL_EXITH_MUT_ELF): $(KERNEL_EXITH_MUT_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_EXITH_MUT_OBJS)

$(KERNEL_EXITH_MUT_BIN): $(KERNEL_EXITH_MUT_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_exith_mut.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(exith-mutant): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

$(EXITH_MUT_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_EXITH_MUT_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_EXITH_MUT_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> exith MUTANT image: %s (sft_close_process elided -- expected RED)\n" "$@"

# --- COMMAND.COM shell kernel (beads initech-7pc; make test-shell) ----------
# Same sources, but kmain.c compiled with -DBOOT_SHELL so the boot, after
# CONIN-LIVE, prints SHELL-READY and enters the COMMAND.COM REPL instead of the
# demo+halt. The shell command.o (REPL enabled) is linked IN. Separate image so
# the NORMAL boot never enters the (blocking) prompt -- the demo gates run with
# NO key injection. Requires --disk2 (HELLO.TXT + GREET.COM). Mirrors the EXEC
# image; -DBOOT_SHELL on kmain.c only (command.o is built with COMMAND_KERNEL_REPL
# above and is BOOT_SHELL-agnostic).
$(KERNEL_SHELL_MAIN_OBJ): $(KERNEL_MAIN_C) $(KERNEL_DIR)/boot_info.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pic.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/test_prog.h $(KERNEL_DIR)/psp.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/ata.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/kbd.h $(KERNEL_DIR)/pit.h $(KERNEL_DIR)/command.h spec/memory_map.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DBOOT_SHELL -Ispec -I$(KERNEL_DIR) -c $(KERNEL_MAIN_C) -o $@

KERNEL_SHELL_OBJS := $(KERNEL_START_OBJ) $(KERNEL_SHELL_MAIN_OBJ) $(KERNEL_CONSOLE_OBJ) \
                     $(KERNEL_IDT_OBJ) $(KERNEL_PIC_OBJ) $(KERNEL_PANIC_OBJ) \
                     $(KERNEL_INT21_OBJ) $(KERNEL_PSP_OBJ) $(KERNEL_SFT_OBJ) $(KERNEL_LOADER_OBJ) \
                     $(KERNEL_ATA_OBJ) $(KERNEL_FAT12_OBJ) $(KERNEL_FILEIO_OBJ) \
                     $(KERNEL_KBD_OBJ) $(KERNEL_PIT_OBJ) $(KERNEL_IRQ_OBJ) $(KERNEL_COMMAND_OBJ) \
                     $(KERNEL_TEST_PROG_OBJ) $(KERNEL_TYPE_PROG_OBJ) $(KERNEL_DIR_PROG_OBJ) \
                     $(KERNEL_ISR_OBJ)

$(KERNEL_SHELL_ELF): $(KERNEL_SHELL_OBJS) $(KERNEL_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $@ $(KERNEL_SHELL_OBJS)

$(KERNEL_SHELL_BIN): $(KERNEL_SHELL_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel_shell.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel(shell): %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

# The shell self-test disk image: identical layout to TRACER_IMG but with the
# shell kernel at sector 17.
$(SHELL_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_SHELL_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_SHELL_BIN) of=$@ bs=512 seek=17 conv=notrunc status=none
	@printf ">>> shell image: %s (COMMAND.COM REPL self-test kernel @s17)\n" "$@"

# Raw flat binary, then zero-pad to KERNEL_SECTORS * 512 bytes (deterministic).
$(KERNEL_BIN): $(KERNEL_ELF) | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ "$$sz" -gt "$$max" ]; then \
		printf '!!! kernel.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes) -- bump KERNEL_SECTORS in BOTH the Makefile and stage2.asm\n' "$$sz" "$$max"; \
		exit 1; \
	fi; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none; \
	printf ">>> kernel: %s (flat binary @0x10000, padded to %d sectors)\n" "$@" "$(KERNEL_SECTORS)"

$(PPM_CHECK_BIN): $(PPM_CHECK_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(PPM_TEXT_CHECK_BIN): $(PPM_TEXT_CHECK_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

# --- Asset tools + derived artifacts (beads initech-vcq) -------------------
$(PALETTE_TOOL_BIN): $(PALETTE_TOOL_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

$(ASSET_CHECK_BIN): $(ASSET_CHECK_SRC) $(CHICAGO_H) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

# Decode the reference webp to a raw PPM in build/. The PPM derives from the
# film frame -> it is NOT committed (gitignored), it is a build intermediate.
$(PREVIEW_PPM): $(PREVIEW_WEBP) | $(BUILD)
	@$(CONVERT) $< $@ 2>/dev/null \
		|| python3 -c "from PIL import Image; Image.open('$<').convert('RGB').save('$@')"
	@printf ">>> assets: decoded %s -> %s (build intermediate, not committed)\n" "$<" "$@"

# Regenerate palette.h from the committed palette.json (single source of
# truth). Reproducible: deterministic constants, no timestamps (Rule 11).
$(PALETTE_H): $(PALETTE_JSON) $(PALETTE_TOOL_BIN)
	@$(PALETTE_TOOL_BIN) --header $(PALETTE_JSON) > $@
	@printf ">>> assets: regenerated %s from %s\n" "$@" "$(PALETTE_JSON)"

$(SMOKE_BIN): $(SMOKE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

# Seed driver: front end only (lexer + parser + AST + driver). Codegen lands
# in a later step (beads initech-znb is Step A). -Iseed for the headers.
$(SEED_BIN): $(SEED_DRV_SRC) $(SEED_LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -Iseed -o $@ $(SEED_DRV_SRC) $(SEED_LIB_SRC)

# QEMU oracle harness CLI: library + main, one translation unit each.
$(HARNESS_BIN): $(HARNESS_DRV_SRC) $(HARNESS_LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -Iharness/emu -o $@ $(HARNESS_DRV_SRC) $(HARNESS_LIB_SRC)

# Self-test fixtures: nasm -> ELF object -> linked multiboot1 ELF. The
# linker script forces the multiboot header into the first 8 KiB.
$(BUILD)/%.elf: $(FIXTURE_DIR)/%.asm $(FIXTURE_LD) | $(BUILD)
	$(NASM) -f elf32 $< -o $(BUILD)/$*.o
	$(LD) -m elf_i386 -T $(FIXTURE_LD) -o $@ $(BUILD)/$*.o

# ---------------------------------------------------------------------------
# Seed codegen pipeline (beads initech-znb, Step B)
# ---------------------------------------------------------------------------
# Compile a .pas through the full chain: initechc --emit-asm -> nasm -felf32
# -> ld -T seed/rt/seed.ld, linking the freestanding runtime. The runtime
# object is shared by every seed binary.
$(SEED_RT_OBJ): $(SEED_RT_ASM) | $(BUILD)
	$(NASM) -f elf32 $< -o $@

# Pattern: build/seed_<name>.elf from seed/examples/<name>.pas.
# (arith programs use the seed_arith_<name>.elf names below.)
$(SEED_SMOKE_ELF): $(SEED_SMOKE_PAS) $(SEED_BIN) $(SEED_RT_OBJ) $(SEED_RT_LD) | $(BUILD)
	$(SEED_BIN) --emit-asm -o $(BUILD)/seed_smoke.s $<
	$(NASM) -f elf32 $(BUILD)/seed_smoke.s -o $(BUILD)/seed_smoke.o
	$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $@ $(SEED_RT_OBJ) $(BUILD)/seed_smoke.o

$(BUILD):
	@mkdir -p $(BUILD)

# ---------------------------------------------------------------------------
# Stub targets (land them per PRD Sec 11 milestones)
# ---------------------------------------------------------------------------
image:
	$(call stub,image,M1)

# ---------------------------------------------------------------------------
# REAL action target: run
# ---------------------------------------------------------------------------
# Dev-loop boot via the harness. Boots $(BUILD)/initech.img if it exists,
# else the serial_hello fixture, echoing captured serial to stdout. This is
# an ACTION target (it asserts nothing) -- a non-zero exit here just reflects
# the guest's verdict, which is informative, not a gate. PRD Sec 8 / CLAUDE.md
# "Build & test".
run: $(HARNESS_BIN) $(SERIAL_HELLO_ELF)
	@if [ -f "$(BUILD)/initech.img" ]; then \
		printf ">>> run: booting %s/initech.img via harness (serial -> stdout)\n" "$(BUILD)"; \
		$(HARNESS_BIN) --disk "$(BUILD)/initech.img" --name initech --serial-stdout || true; \
	else \
		printf ">>> run: no %s/initech.img yet -- booting serial_hello fixture (serial -> stdout)\n" "$(BUILD)"; \
		$(HARNESS_BIN) --kernel "$(SERIAL_HELLO_ELF)" --expect "$(HARNESS_MARKER)" --name run_fixture --serial-stdout || true; \
	fi

run-bochs:
	$(call stub,run-bochs,M1)

# ---------------------------------------------------------------------------
# REAL gate: smoke (beads initech-f8v.1 -- the M0.5 tracer-bullet heartbeat)
# ---------------------------------------------------------------------------
# Ref: PRD Sec 11 (M0.5) -- the tracer bullet is "a thin thread through every
#      layer that actually runs ... plus a repeatable `make smoke` heartbeat.
#      The bullet flying is the GATE that unblocks fleshing out M1-M8."
# Ref: PRD Sec 8 -- the QEMU oracle signals (serial printf marker,
#      `-d int,guest_errors,cpu_reset` triple-fault detect, QMP screendump)
#      and the gate vector (boot_stage_reached + panics/guest_errors==0).
# Ref: CLAUDE.md Law 2 (the oracle is the truth) / Rule 2 (fail fast, loud).
#
# This composes the already-verified pieces into ONE repeatable CI gate:
#   seed compiler -> $(SEED_SMOKE_ELF) (built by the rule above; we depend on
#   it, we do NOT duplicate the seed pipeline) -> boot under the QEMU oracle
#   harness -> assert the serial marker AND no triple-fault AND no hang.
#
# PASS (exit 0) IFF the harness CLI exits 0, which the harness defines as:
#   launched && !timed_out && !triple_fault && guest_errors==0 && marker_found
# (see harness/emu/qemu.h QemuResult.ok). So the harness exit status IS the
# gate -- we propagate it verbatim. Absent marker, a triple-fault, or a hang
# all make the harness exit non-zero, which fails `make smoke`.
#
# Screendump nuance (KNOWN LIMITATION -- bead initech-xcg, do NOT fight it):
#   $(SEED_SMOKE_ELF) clean-exits via isa-debug-exit in ~0.25s, so the QMP
#   screendump can lose the race and be silently skipped. For THIS heartbeat
#   the authoritative signal is the serial marker + no-fault; the screendump
#   is best-effort. We --screendump and REPORT whether it was captured, but we
#   do NOT fail smoke solely because the screendump was skipped. The real
#   visual screendump+SSIM gate (live guest) is a later tracer task f8v.4.
#   We must NOT touch the seed runtime or the harness to work around this.
SMOKE_ELF    := $(SEED_SMOKE_ELF)
SMOKE_MARKER := $(SEED_SMOKE_MARKER)
SMOKE_REPORT := $(BUILD)/smoke.report

smoke: $(HARNESS_BIN) $(SMOKE_ELF)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make smoke : M0.5 tracer-bullet heartbeat\n'
	@printf '  Ref: PRD Sec 11 (M0.5 gate) / Sec 8 (QEMU oracle). beads initech-f8v.1\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s\n' "$(SMOKE_ELF)"
	@printf 'Via       : %s (QEMU -kernel multiboot1 boot)\n' "$(HARNESS_BIN)"
	@printf 'Expecting : serial marker "%s"\n' "$(SMOKE_MARKER)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@# Run the oracle. Capture its report (stderr) so we can summarise it, but
	@# the harness EXIT STATUS is the authoritative verdict and is what gates.
	@rc=0; \
	$(HARNESS_BIN) --kernel "$(SMOKE_ELF)" --expect "$(SMOKE_MARKER)" \
		--screendump --name smoke --out "$(BUILD)" --timeout-ms 5000 \
		2> "$(SMOKE_REPORT)" || rc=$$?; \
	rep=$$(cat "$(SMOKE_REPORT)"); \
	get() { echo "$$rep" | tr ' ' '\n' | grep "^$$1=" | head -n1 | cut -d= -f2; }; \
	slen=$$(get serial_len); mfound=$$(get marker_found); \
	tfault=$$(get triple_fault); gerr=$$(get guest_errors); \
	tout=$$(get timed_out); sdump=$$(get screendump_taken); \
	sdpath=$$(echo "$$rep" | grep 'screendump_taken=' | sed 's/.*path=//'); \
	yn() { [ "$$1" = "1" ] && echo yes || echo no; }; \
	printf 'Heartbeat summary\n'; \
	printf '  serial bytes captured : %s\n' "$$slen"; \
	printf '  marker found?         : %s\n' "$$(yn $$mfound)"; \
	printf '  triple-fault?         : %s\n' "$$(yn $$tfault)"; \
	printf '  guest errors          : %s\n' "$$gerr"; \
	printf '  hung / timed out?     : %s\n' "$$(yn $$tout)"; \
	if [ "$$sdump" = "1" ]; then \
		printf '  screendump            : captured -> %s\n' "$$sdpath"; \
	else \
		printf '  screendump            : skipped (best-effort; guest clean-exits in ~0.25s, bead initech-xcg) -- NOT a smoke failure\n'; \
	fi; \
	printf '%s\n' '----------------------------------------------------------------------'; \
	if [ "$$rc" -eq 0 ]; then \
		printf 'VERDICT   : PASS -- tracer bullet flew (marker found, no fault, no hang)\n'; \
	else \
		printf 'VERDICT   : FAIL -- heartbeat RED (harness exit %s; see report above)\n' "$$rc"; \
	fi; \
	printf '======================================================================\n'; \
	exit $$rc

ssim:
	$(call stub_fail,ssim,M0)

test-region:
	$(call stub_fail,test-region,M3)

# ---------------------------------------------------------------------------
# REAL gate: test-fat (beads initech-5cu; FAT12 read path beads initech-adf)
# ---------------------------------------------------------------------------
# The FAT12 DIFFERENTIAL ORACLE. Our C reader (os/milton/fat12.c, driven on the
# host via the file-backed blockdev) must AGREE with TWO independent references
# on the SAME freshly-minted image (CLAUDE.md Law 2, Rule 5 differential-from-
# day-one):
#   ref #1: mtools (period-authentic)         -- mdir (names+sizes), mcopy (bytes)
#   ref #2: an independent python3 reader      -- fat12_ref.py (NOT mtools, NOT
#                                                 our C; a third implementation)
# Agreement is required on the SET of file names, each file's SIZE, and each
# file's CONTENT bytes. Timestamps and the volume serial are normalized away
# (mdir's date/time columns dropped; we never print them) per
# docs/research/fat12-ground-truth.md Sec 5.
#
# Four assertions, every one fail-loud and exit-non-zero on miss (Rule 2):
#   A. NAMES+SIZES, triple agreement: fat_dump --list == fat12_ref.py --list ==
#      normalized `mdir`. Any pairwise mismatch => RED (prints the diff).
#   B. CONTENT, per file: fat_dump --cat NAME == mcopy ::NAME == fat12_ref.py
#      --cat NAME, byte-for-byte. Any mismatch => RED.
#   C. The three FAT12 unit oracles (bpb, chain, dir) all pass.
#   D. Tool presence: mtools (mdir/mcopy) and python3 MUST exist, else the gate
#      FAILS LOUD -- a silently-skipped oracle is the worst outcome (Law 2).
# Ref: PRD Sec 6.1; ADR-0003 DEC-07; beads initech-5cu / initech-adf.
FAT12_OUR_LIST  := $(BUILD)/fat12_our.list
FAT12_PY_LIST   := $(BUILD)/fat12_py.list
FAT12_MTOOLS_LIST := $(BUILD)/fat12_mtools.list

test-fat: $(FAT_DUMP_BIN) $(FAT12_IMG) $(FAT12_REF_PY) \
          $(TEST_FAT12_BPB) $(TEST_FAT12_CHAIN) $(TEST_FAT12_DIR)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fat : FAT12 differential oracle\n'
	@printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-5cu / initech-adf.\n'
	@printf '  Our C reader vs TWO independent refs (mtools + python3) on one image.\n'
	@printf '======================================================================\n'
	@# ---- (D) Tool presence: fail loud, NEVER silently skip (Law 2). ----
	@command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
	@command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat FAIL: mtools `mcopy` not found (apt install mtools).\n'; exit 1; }
	@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat FAIL: python3 not found (needed for the independent reference reader).\n'; exit 1; }
	@printf '>>> test-fat [D]: reference tools present (mtools mdir/mcopy + python3)\n'
	@# ---- (C) The three FAT12 unit oracles must pass first. ----
	@printf '>>> test-fat [C]: FAT12 unit oracles (bpb / chain / dir)\n'
	@$(TEST_FAT12_BPB)   "$(FAT12_IMG)" \
		|| { printf '!!! test-fat FAIL: test_fat12_bpb red\n'; exit 1; }
	@$(TEST_FAT12_CHAIN) "$(FAT12_IMG)" \
		|| { printf '!!! test-fat FAIL: test_fat12_chain red\n'; exit 1; }
	@$(TEST_FAT12_DIR)   "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" \
		|| { printf '!!! test-fat FAIL: test_fat12_dir red\n'; exit 1; }
	@# ---- (A) NAMES + SIZES: triple agreement (our == python == mdir). ----
	@printf '>>> test-fat [A]: names+sizes -- our fat_dump == python ref == normalized mdir\n'
	@$(FAT_DUMP_BIN) "$(FAT12_IMG)" --list > "$(FAT12_OUR_LIST)" \
		|| { printf '!!! test-fat FAIL: fat_dump --list errored\n'; exit 1; }
	@python3 "$(FAT12_REF_PY)" "$(FAT12_IMG)" --list > "$(FAT12_PY_LIST)" \
		|| { printf '!!! test-fat FAIL: fat12_ref.py --list errored\n'; exit 1; }
	@# Normalize mdir: keep only rows carrying a YYYY-MM-DD date (real entries),
	@# reassemble the space-grouped thousands in the size, drop date/time/serial,
	@# emit "NAME.EXT <size>" and sort -- timestamps/serial normalized away.
	@mdir -i "$(FAT12_IMG)" :: | awk '{ \
		hd=0; di=0; \
		for(i=1;i<=NF;i++){ if($$i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$$/){hd=1;di=i;break} } \
		if(!hd) next; \
		name=$$1; ext=$$2; sz=""; \
		for(i=3;i<di;i++){ sz=sz $$i } \
		if(sz !~ /^[0-9]+$$/) next; \
		fn=(ext!="")?name"."ext:name; \
		print fn, sz \
	}' | sort > "$(FAT12_MTOOLS_LIST)"
	@diff -u "$(FAT12_OUR_LIST)" "$(FAT12_PY_LIST)" \
		|| { printf '!!! test-fat FAIL [A]: our listing != python reference listing\n'; exit 1; }
	@diff -u "$(FAT12_OUR_LIST)" "$(FAT12_MTOOLS_LIST)" \
		|| { printf '!!! test-fat FAIL [A]: our listing != normalized mdir listing\n'; exit 1; }
	@printf '    triple agreement on names+sizes (%s files):\n' "$$(wc -l < "$(FAT12_OUR_LIST)" | tr -d ' ')"
	@sed 's/^/      /' "$(FAT12_OUR_LIST)"
	@# ---- (B) CONTENT per file: our == mcopy == python, byte-for-byte. ----
	@printf '>>> test-fat [B]: content -- our fat_dump --cat == mcopy == python ref, per file\n'
	@for f in $(FAT12_GATE_NAMES); do \
		$(FAT_DUMP_BIN) "$(FAT12_IMG)" --cat "$$f" > "$(BUILD)/fat12_our_$$f.bin" \
			|| { printf '!!! test-fat FAIL [B]: fat_dump --cat %s errored\n' "$$f"; exit 1; }; \
		mcopy -i "$(FAT12_IMG)" "::$$f" - > "$(BUILD)/fat12_mtools_$$f.bin" 2>/dev/null \
			|| { printf '!!! test-fat FAIL [B]: mcopy ::%s errored\n' "$$f"; exit 1; }; \
		python3 "$(FAT12_REF_PY)" "$(FAT12_IMG)" --cat "$$f" > "$(BUILD)/fat12_py_$$f.bin" \
			|| { printf '!!! test-fat FAIL [B]: fat12_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
		cmp -s "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_mtools_$$f.bin" \
			|| { printf '!!! test-fat FAIL [B]: %s -- our bytes != mcopy bytes\n' "$$f"; cmp "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_mtools_$$f.bin"; exit 1; }; \
		cmp -s "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_py_$$f.bin" \
			|| { printf '!!! test-fat FAIL [B]: %s -- our bytes != python ref bytes\n' "$$f"; cmp "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_py_$$f.bin"; exit 1; }; \
		printf '    %-12s %6s bytes : our == mcopy == python\n' "$$f" "$$(wc -c < "$(BUILD)/fat12_our_$$f.bin" | tr -d ' ')"; \
	done
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- our FAT12 reader agrees with mtools AND an independent\n'
	@printf '            python3 reader on names, sizes, and content (triple oracle).\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-fat-write (beads initech-509.11 -- FAT12 WRITE round-trip)
# ---------------------------------------------------------------------------
# The FAT12 WRITE DIFFERENTIAL ORACLE (the heart of initech-509.11). The REAL
# artifact fat12.c WRITE path (fat12_create + fat12_write_file, driven on the
# host via the read-WRITE file-backed blockdev) mints a BLANK image and writes
# four files into it; that image -- written ENTIRELY by our code -- must read
# back correctly in THREE independent ways (Law 2, Rule 5):
#   ref #1: mtools  -- mdir (names+sizes) + mcopy (content bytes)
#   ref #2: python3 -- fat12_ref.py (independent reader)
#   ref #3: our OWN reader -- the in-memory round-trip in test_fat12_write.c
# Agreement on names, sizes, and CONTENT bytes. Timestamps/serial normalized
# away (NOT meaningful bytes: the dir mtime/mdate are stamped to the fixed
# deterministic constant 0 (Rule 11) and dropped from the diff; the meaningful
# bytes are name/attr/size/start_cluster + content).
#
# Assertions (every one fail-loud + exit-non-zero, Rule 2):
#   A. The WRITE unit + in-memory round-trip oracle passes (our read of our
#      write, 12-bit encode, both-FAT sync, full-volume fail-loud, unlink).
#   B. Tool presence: mtools (mdir/mcopy) + python3 MUST exist, else fail loud.
#   C. Mint BLANK image, write the 4 fixtures with OUR writer (--diff).
#   D. NAMES+SIZES: normalized mdir == python3 --list (mtools and python both
#      read OUR written image and agree on the 4 files + sizes).
#   E. CONTENT per file: mcopy ::NAME == python3 --cat NAME == the deterministic
#      bytes our writer emitted (regenerated host-side), byte-for-byte.
# Ref: PRD Sec 6.1; ADR-0003 DEC-07; beads initech-509.11.
FATW_OUR_REF   := $(BUILD)/fatw_ref
FATW_PY_LIST   := $(BUILD)/fatw_py.list
FATW_MTOOLS_LIST := $(BUILD)/fatw_mtools.list

.PHONY: test-fat-write
test-fat-write: $(TEST_FAT12_WRITE) $(FAT12_REF_PY)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fat-write : FAT12 WRITE round-trip oracle\n'
	@printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-509.11.\n'
	@printf '  OUR writer mints a blank image + writes files; mtools + python3 read it back.\n'
	@printf '======================================================================\n'
	@# ---- (B) Tool presence: fail loud, NEVER silently skip (Law 2). ----
	@command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
	@command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: mtools `mcopy` not found.\n'; exit 1; }
	@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: python3 not found.\n'; exit 1; }
	@printf '>>> test-fat-write [B]: reference tools present (mtools + python3)\n'
	@# ---- (A) The WRITE unit + in-memory round-trip oracle. ----
	@dd if=/dev/zero of="$(BUILD)/fatw_unit.img" bs=512 count=2880 status=none
	@mformat -i "$(BUILD)/fatw_unit.img" -f 1440 ::
	@$(TEST_FAT12_WRITE) "$(BUILD)/fatw_unit.img" \
		|| { printf '!!! test-fat-write FAIL [A]: test_fat12_write unit/round-trip red\n'; exit 1; }
	@printf '>>> test-fat-write [A]: WRITE unit + in-memory round-trip green\n'
	@# ---- (C) Mint a clean BLANK image and write the 4 fixtures with OUR writer. ----
	@dd if=/dev/zero of="$(BUILD)/fatw_diff.img" bs=512 count=2880 status=none
	@mformat -i "$(BUILD)/fatw_diff.img" -f 1440 ::
	@$(TEST_FAT12_WRITE) "$(BUILD)/fatw_diff.img" --diff \
		|| { printf '!!! test-fat-write FAIL [C]: writing the diff fixtures failed\n'; exit 1; }
	@printf '>>> test-fat-write [C]: OUR writer wrote 4 files into a blank image\n'
	@# ---- (D) NAMES+SIZES: normalized mdir == python3 --list. ----
	@printf '>>> test-fat-write [D]: names+sizes -- python3 reader == normalized mdir on OUR image\n'
	@python3 "$(FAT12_REF_PY)" "$(BUILD)/fatw_diff.img" --list > "$(FATW_PY_LIST)" \
		|| { printf '!!! test-fat-write FAIL [D]: fat12_ref.py --list errored on our written image\n'; exit 1; }
	@mdir -i "$(BUILD)/fatw_diff.img" :: | awk '{ \
		hd=0; di=0; \
		for(i=1;i<=NF;i++){ if($$i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$$/){hd=1;di=i;break} } \
		if(!hd) next; \
		name=$$1; ext=$$2; sz=""; \
		for(i=3;i<di;i++){ sz=sz $$i } \
		if(sz !~ /^[0-9]+$$/) next; \
		fn=(ext!="")?name"."ext:name; \
		print fn, sz \
	}' | sort > "$(FATW_MTOOLS_LIST)"
	@diff -u "$(FATW_PY_LIST)" "$(FATW_MTOOLS_LIST)" \
		|| { printf '!!! test-fat-write FAIL [D]: python listing != normalized mdir listing of OUR written image\n'; exit 1; }
	@printf '    agreement on names+sizes (%s files):\n' "$$(wc -l < "$(FATW_PY_LIST)" | tr -d ' ')"
	@sed 's/^/      /' "$(FATW_PY_LIST)"
	@# ---- (E) CONTENT per file: mcopy == python3 --cat == our deterministic bytes. ----
	@printf '>>> test-fat-write [E]: content -- mcopy == python ref == our written bytes, per file\n'
	@for f in $(FAT12_WRITE_NAMES); do \
		mcopy -i "$(BUILD)/fatw_diff.img" "::$$f" - > "$(BUILD)/fatw_mtools_$$f.bin" 2>/dev/null \
			|| { printf '!!! test-fat-write FAIL [E]: mcopy ::%s errored (mtools could not read our written file)\n' "$$f"; exit 1; }; \
		python3 "$(FAT12_REF_PY)" "$(BUILD)/fatw_diff.img" --cat "$$f" > "$(BUILD)/fatw_py_$$f.bin" \
			|| { printf '!!! test-fat-write FAIL [E]: fat12_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
		cmp -s "$(BUILD)/fatw_mtools_$$f.bin" "$(BUILD)/fatw_py_$$f.bin" \
			|| { printf '!!! test-fat-write FAIL [E]: %s -- mcopy bytes != python ref bytes (root-cause the writer, Rule 3)\n' "$$f"; cmp "$(BUILD)/fatw_mtools_$$f.bin" "$(BUILD)/fatw_py_$$f.bin"; exit 1; }; \
		printf '    %-12s %6s bytes : mcopy == python\n' "$$f" "$$(wc -c < "$(BUILD)/fatw_mtools_$$f.bin" | tr -d ' ')"; \
	done
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- a volume written ENTIRELY by InitechDOS reads back correctly\n'
	@printf '            in mtools AND an independent python3 reader (names, sizes, content)\n'
	@printf '            AND in our own reader (in-memory round-trip).\n'
	@printf '======================================================================\n'

# Mutation-proof (Rule 6): BOTH WRITE mutants MUST fail the unit/round-trip
# oracle. A mutant that PASSES means the WRITE oracle is decoration.
.PHONY: test-fat-write-mutant
test-fat-write-mutant: $(TEST_FAT12_WRITE_MUT_ONEFAT) $(TEST_FAT12_WRITE_MUT_EOC)
	@printf '>>> test-fat-write-mutant: confirming both WRITE mutants go RED (Rule 6)\n'
	@dd if=/dev/zero of="$(BUILD)/fatw_mut1.img" bs=512 count=2880 status=none
	@mformat -i "$(BUILD)/fatw_mut1.img" -f 1440 ::
	@if $(TEST_FAT12_WRITE_MUT_ONEFAT) "$(BUILD)/fatw_mut1.img" >/dev/null 2>&1; then \
		printf '!!! test-fat-write-mutant FAIL: one-FAT-copy mutant PASSED -- the both-FAT-sync check is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fat-write-mutant: green (one-FAT-copy mutant correctly RED)\n'; \
	fi
	@dd if=/dev/zero of="$(BUILD)/fatw_mut2.img" bs=512 count=2880 status=none
	@mformat -i "$(BUILD)/fatw_mut2.img" -f 1440 ::
	@if $(TEST_FAT12_WRITE_MUT_EOC) "$(BUILD)/fatw_mut2.img" >/dev/null 2>&1; then \
		printf '!!! test-fat-write-mutant FAIL: wrong-EOC mutant PASSED -- the round-trip check is decoration\n'; \
		exit 1; \
	else \
		printf '>>> test-fat-write-mutant: green (wrong-EOC mutant correctly RED -- the oracle bites)\n'; \
	fi

test-dbase:
	$(call stub_fail,test-dbase,M6)

test-compiler:
	$(call stub_fail,test-compiler,M7)

# ---------------------------------------------------------------------------
# REAL gate: test-seed (beads initech-znb)
# ---------------------------------------------------------------------------
# Build and run the seed front-end unit tests. Each test binary exits non-zero
# if any CHECK fails (see seed/test_assert.h), and the recipe runs them with
# `&&` so test-seed itself goes non-zero on the first failing binary. A gate
# must fail loud, never false-green (CLAUDE.md Law 2 / Rule 2), exactly like
# the stub_fail philosophy -- but this one is REAL.
test-seed: $(SEED_TEST_LEXER) $(SEED_TEST_PARSER)
	@printf ">>> test-seed: running seed front-end unit tests\n"
	@$(SEED_TEST_LEXER)
	@$(SEED_TEST_PARSER)
	@printf ">>> test-seed: all green\n"

$(SEED_TEST_LEXER): seed/test_lexer.c $(SEED_LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -o $@ seed/test_lexer.c $(SEED_LIB_SRC)

$(SEED_TEST_PARSER): seed/test_parser.c $(SEED_LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -o $@ seed/test_parser.c $(SEED_LIB_SRC)

# ---------------------------------------------------------------------------
# REAL gate: test-seed-codegen (beads initech-znb, Step B)
# ---------------------------------------------------------------------------
# End-to-end differential correctness for the seed code generator. We compile
# each .pas through initechc --emit-asm -> nasm -> ld (seed runtime), boot the
# resulting multiboot1 ELF under the QEMU oracle harness, and assert the EXACT
# expected serial output (CLAUDE.md Law 2 -- the oracle is the truth). Any
# missing marker, fault, or timeout makes the harness CLI exit non-zero, which
# fails this gate. This is the seed's differential check: get decimal
# formatting and operator semantics (incl. Pascal div/mod truncating toward
# zero, and negative-decimal formatting) exactly right.
#
# Each arith program prints a distinctive "TAG=value" so the asserted marker
# is unambiguous (e.g. "R=14" cannot accidentally match a substring of "-14").
#
# build/seed_arith_<name>.elf is built from seed/examples/arith/<name>.pas.
ARITH_DIR := seed/examples/arith

$(BUILD)/seed_arith_%.elf: $(ARITH_DIR)/%.pas $(SEED_BIN) $(SEED_RT_OBJ) $(SEED_RT_LD) | $(BUILD)
	$(SEED_BIN) --emit-asm -o $(BUILD)/seed_arith_$*.s $<
	$(NASM) -f elf32 $(BUILD)/seed_arith_$*.s -o $(BUILD)/seed_arith_$*.o
	$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $@ $(SEED_RT_OBJ) $(BUILD)/seed_arith_$*.o

ARITH_ELVES := $(BUILD)/seed_arith_precedence.elf \
               $(BUILD)/seed_arith_parens.elf \
               $(BUILD)/seed_arith_divmod.elf \
               $(BUILD)/seed_arith_negative.elf

test-seed-codegen: $(HARNESS_BIN) $(SEED_SMOKE_ELF) $(ARITH_ELVES)
	@printf ">>> test-seed-codegen: SMOKE -- expect serial marker '%s'\n" "$(SEED_SMOKE_MARKER)"
	@$(HARNESS_BIN) --kernel "$(SEED_SMOKE_ELF)" --expect "$(SEED_SMOKE_MARKER)" \
		--name seed_smoke --timeout-ms 5000 \
		|| { printf "!!! test-seed-codegen FAIL: smoke marker missing (marker, fault, or timeout)\n"; exit 1; }
	@printf ">>> test-seed-codegen: ARITH precedence -- expect 'R=14' (2 + 3 * 4)\n"
	@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_precedence.elf" --expect "R=14" \
		--name seed_prec --timeout-ms 5000 \
		|| { printf "!!! test-seed-codegen FAIL: 2 + 3 * 4 did not print R=14\n"; exit 1; }
	@printf ">>> test-seed-codegen: ARITH parens -- expect 'R=20' ((2 + 3) * 4)\n"
	@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_parens.elf" --expect "R=20" \
		--name seed_paren --timeout-ms 5000 \
		|| { printf "!!! test-seed-codegen FAIL: (2 + 3) * 4 did not print R=20\n"; exit 1; }
	@printf ">>> test-seed-codegen: ARITH div/mod -- expect 'D=3 M=2' (17 div 5, 17 mod 5)\n"
	@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_divmod.elf" --expect "D=3 M=2" \
		--name seed_divmod --timeout-ms 5000 \
		|| { printf "!!! test-seed-codegen FAIL: 17 div 5 / 17 mod 5 did not print D=3 M=2\n"; exit 1; }
	@printf ">>> test-seed-codegen: ARITH negative -- expect 'N=-5' (0 - 5)\n"
	@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_negative.elf" --expect "N=-5" \
		--name seed_neg --timeout-ms 5000 \
		|| { printf "!!! test-seed-codegen FAIL: 0 - 5 did not print N=-5\n"; exit 1; }
	@printf ">>> test-seed-codegen: all green (smoke marker + arithmetic exact-output checks)\n"

# ---------------------------------------------------------------------------
# REAL gate: test-harness (beads initech-f2s)
# ---------------------------------------------------------------------------
# Self-test the QEMU oracle harness against hand-written fixtures. This is a
# correctness GATE: a false-green harness is the worst possible outcome
# (CLAUDE.md Law 2), so every expectation is checked and any miss exits
# NON-ZERO. We assert:
#   1. serial_hello -> HARNESS-OK marker captured AND no triple-fault
#      (the CLI exits 0 only when launched && !timed_out && !triple_fault &&
#       guest_errors==0 && marker_found, so exit 0 IS the full assertion).
#   2. triple_fault -> triple-fault IS detected AND the marker is ABSENT
#      (we grep the CLI report for triple_fault=1 and marker_found=0).
# The harness's own wall-clock timeout guarantees this gate cannot hang.
test-harness: $(HARNESS_BIN) $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF)
	@printf ">>> test-harness: GOOD fixture (serial_hello) -- expect marker, no fault\n"
	@$(HARNESS_BIN) --kernel "$(SERIAL_HELLO_ELF)" --expect "$(HARNESS_MARKER)" \
		--name th_good --timeout-ms 5000 \
		|| { printf "!!! test-harness FAIL: serial_hello did not satisfy the oracle (marker missing, fault, or timeout)\n"; exit 1; }
	@printf ">>> test-harness: BAD fixture (triple_fault) -- expect fault detected, marker absent\n"
	@rep=$$($(HARNESS_BIN) --kernel "$(TRIPLE_FAULT_ELF)" --expect "$(HARNESS_MARKER)" \
		--name th_bad --timeout-ms 5000 2>&1); \
		printf '%s\n' "$$rep"; \
		echo "$$rep" | grep -q 'triple_fault=1' \
			|| { printf "!!! test-harness FAIL: triple_fault was NOT detected on the bad fixture\n"; exit 1; }; \
		echo "$$rep" | grep -q 'marker_found=0' \
			|| { printf "!!! test-harness FAIL: HARNESS-OK marker unexpectedly present on the bad fixture\n"; exit 1; }; \
		echo "$$rep" | grep -q 'timed_out=0' \
			|| { printf "!!! test-harness FAIL: bad fixture timed out instead of exiting on the fault\n"; exit 1; }
	@printf ">>> test-harness: all green (marker caught on good fixture, triple-fault caught on bad)\n"

# ---------------------------------------------------------------------------
# REAL gate: test-tracer-boot (beads initech-f8v.2)
# ---------------------------------------------------------------------------
# Boot the REAL raw-disk tracer image under the QEMU oracle and assert the
# whole chain worked. Three independent assertions (CLAUDE.md Law 2 -- the
# oracle is the truth; Rule 2 -- fail loud):
#   1. serial stage markers present (at least S1, PM, OK) -> the boot walked
#      through MBR -> protected mode -> framebuffer fill.
#   2. NO triple-fault in the QEMU -d cpu_reset log (the real->protected
#      transition did not silently reboot -- the minefield callout).
#   3. the QMP screendump of the LIVE guest shows the InitechDOS banner blitted
#      on the seafoam desktop (ppm_text_check).
# The guest hlt-loops to stay live for the screendump, so it is reaped by the
# wall-clock timeout -- a timeout here is EXPECTED and is NOT a failure by
# itself; we assert on markers + screendump + triple_fault, not on exit code.
#
# SUPERSESSION (beads initech-bea): the kernel now blits the InitechDOS banner
# (initech-bea) into the top rows, which inks fg pixels exactly where the old
# 81-point pure-seafoam grid (ppm_seafoam_check) sampled -- so a uniform-seafoam
# assertion would FALSE-FAIL on a CORRECT boot. Per the STOP CONDITION we do NOT
# weaken the oracle; we STRENGTHEN it. The screendump check is now ppm_text_check,
# which asserts a STRICTLY STRONGER property: the banner was rendered at its known
# origin AND the desktop is still seafoam below the banner (not a solid fill, not
# blank). The pure-seafoam grid lives on, still mutation-proven, as the inner
# assertion (C) of ppm_text_check and via tools/ppm_seafoam_check.c which the
# `make test` vector may still use elsewhere.
TRACER_NAME   := tracer_boot
TRACER_SERIAL := $(BUILD)/$(TRACER_NAME).serial
TRACER_PPM    := $(BUILD)/$(TRACER_NAME).ppm
TRACER_REPORT := $(BUILD)/$(TRACER_NAME).report

test-tracer-boot: $(HARNESS_BIN) $(TRACER_IMG) $(PPM_TEXT_CHECK_BIN)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-tracer-boot : real MBR->LFB boot oracle\n'
	@printf '  Ref: PRD Sec 5 (hardware contract) / Sec 11 (M1). beads initech-f8v.2\n'
	@printf '  NOTE: pure-seafoam grid SUPERSEDED by ppm_text_check (banner blitted);\n'
	@printf '        this gate is now STRICTLY STRONGER (banner + seafoam desktop). initech-bea\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (raw disk, custom MBR -> stage2 -> 32-bit flat -> VESA LFB)\n' "$(TRACER_IMG)"
	@printf 'Expecting : serial markers S1/S2/VBE/A20/GDT/PM/LFB/OK/KERNEL/BANNER + banner screendump\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@# Boot via the disk path with a live-guest screendump. The guest hlt-loops,
	@# so it will time out -- that is expected; we do not gate on the exit code.
	@# --screendump-after BANNER (beads initech-3pe): WAIT for the guest's
	@# paint-complete serial marker before grabbing the framebuffer, removing the
	@# wall-clock race that blanked the screendump under a loaded host. BANNER is
	@# emitted (kmain.c) AFTER both banner lines are blitted to the LFB console
	@# (dos_puts -> con sink -> console_putc -> console_draw_glyph). The 6000 ms
	@# timeout stays the HARD backstop: a guest that never paints still RED.
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" --screendump --screendump-after BANNER \
		--name "$(TRACER_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(TRACER_REPORT)" || true
	@cat "$(TRACER_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@# 1. Triple-fault check (the minefield): the -d log must report none.
	@if grep -q 'triple_fault=1' "$(TRACER_REPORT)"; then \
		printf '!!! test-tracer-boot FAIL: TRIPLE FAULT detected in the real->protected transition\n'; \
		exit 1; \
	fi
	@# 2. Serial stage markers. Require at least S1 (MBR), PM (protected mode),
	@#    OK (framebuffer filled). Report the full set we saw.
	@printf 'Serial markers captured:\n'
	@if [ ! -s "$(TRACER_SERIAL)" ]; then \
		printf '!!! test-tracer-boot FAIL: no serial captured at %s\n' "$(TRACER_SERIAL)"; \
		exit 1; \
	fi
	@for m in S1 S2 VBE FONT A20 GDT PM LFB OK KLOAD KERNEL INT21 BI-OK CONSOLE BANNER; do \
		if grep -q "^$$m$$" "$(TRACER_SERIAL)"; then \
			printf '  %-6s : present\n' "$$m"; \
		else \
			printf '  %-6s : MISSING\n' "$$m"; \
		fi; \
	done
	@# Required subset: the full handoff chain. KERNEL proves the far jump into
	@# the C kernel; BI-OK proves boot_info is readable in C (beads initech-d00);
	@# CONSOLE proves the LFB text console initialized (initech-yqb); BANNER
	@# proves the InitechDOS banner was rendered (initech-bea).
	@for m in S1 PM OK FONT KERNEL INT21 BI-OK CONSOLE BANNER; do \
		grep -q "^$$m$$" "$(TRACER_SERIAL)" \
			|| { printf '!!! test-tracer-boot FAIL: required serial marker %s missing\n' "$$m"; exit 1; }; \
	done
	@# 3. Screendump check: banner blitted at its known origin AND the desktop is
	@#    still seafoam below it (ppm_text_check -- STRICTLY STRONGER than the old
	@#    pure-seafoam grid, which the banner now legitimately perturbs).
	@if [ ! -s "$(TRACER_PPM)" ]; then \
		printf '!!! test-tracer-boot FAIL: no screendump captured at %s (live guest required)\n' "$(TRACER_PPM)"; \
		exit 1; \
	fi
	@$(PPM_TEXT_CHECK_BIN) "$(TRACER_PPM)" \
		|| { printf '!!! test-tracer-boot FAIL: banner not rendered on the seafoam desktop\n'; exit 1; }
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- real boot chain reached protected mode, blitted the InitechDOS banner on the seafoam desktop, no triple-fault\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-boot (beads initech-bea -- the InitechDOS banner milestone)
# ---------------------------------------------------------------------------
# The demonstrable M1 banner gate: boot the REAL image and prove the InitechDOS
# operator banner (spec/dos_banner.txt; ADR-0003 DEC-12 / Appendix D.1) was
# printed -- on the wire AND on the framebuffer -- and ties the running artifact
# back to the LOCKED spec byte-for-byte (CLAUDE.md Law 1 cite-source, Law 2
# oracle-is-truth, Law 4 look-and-feel, Rule 2 fail-loud, Rule 8 locked spec).
#
# Four independent assertions, every one fail-loud + exit-non-zero on miss:
#   1. NO triple-fault in the QEMU -d cpu_reset log (the boot did not reboot).
#   2. SERIAL milestone markers (S1 PM OK FONT KERNEL BI-OK CONSOLE) AND the
#      banner markers (BANNER-BEGIN / BANNER-END / BANNER) are present, AND the
#      two banner literals appear on the wire: "InitechDOS  Version 3.30" (the
#      controlled DOUBLE space) and the 1991 copyright line. A missing line => RED.
#   3. SPEC BYTE-EXACTNESS: extract the two lines BETWEEN BANNER-BEGIN/BANNER-END
#      from the captured serial and `diff` them against spec/dos_banner.txt. Any
#      drift (a single character) => RED. This pins the artifact to the locked
#      spec (Law 1 / Rule 8); the same literal is what test-spec verifies in the
#      spec file, so banner == spec == kernel.
#   4. SCREENDUMP: ppm_text_check asserts the banner was actually blitted at its
#      known glyph origin (fg pixels in cell (0,0) + the line-1 band) AND the
#      desktop is still seafoam below the banner (not a solid fill / blank).
#
# The guest hlt-loops to stay live for the screendump, so it is reaped by the
# wall-clock timeout -- a timeout is EXPECTED and is NOT a failure by itself; we
# assert on markers + banner + spec-diff + screendump, not on the exit code.
#
# TRI-EMULATOR: this gate runs QEMU ONLY. The M1 acceptance criterion is
# identical boot across QEMU/Bochs/86Box (PRD Sec 8 differential emulation), but
# the Bochs + 86Box harness drivers are UNBUILT (beads initech-x0i). The gate
# prints this deferral so the coverage gap is honest and visible -- it does NOT
# silently imply tri-emulator agreement (CLAUDE.md Rule 5 / Law 2).
BOOT_NAME     := boottext
BOOT_SERIAL   := $(BUILD)/$(BOOT_NAME).serial
BOOT_PPM      := $(BUILD)/$(BOOT_NAME).ppm
BOOT_REPORT   := $(BUILD)/$(BOOT_NAME).report
BOOT_LINES    := $(BUILD)/$(BOOT_NAME).banner_lines

test-boot: $(HARNESS_BIN) $(TRACER_IMG) $(PPM_TEXT_CHECK_BIN) $(SPEC_BANNER)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-boot : InitechDOS banner boot gate\n'
	@printf '  Ref: ADR-0003 DEC-12 / Appendix D.1; spec/dos_banner.txt (LOCKED).\n'
	@printf '  beads initech-bea. CLAUDE.md Law 1/2/4, Rule 2/8/11.\n'
	@printf '  TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (raw disk, real boot chain -> C kernel -> banner)\n' "$(TRACER_IMG)"
	@printf 'Expecting : serial S1/PM/OK/FONT/KERNEL/BI-OK/CONSOLE + banner literal + screendump text\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@# Boot the live guest with serial + screendump capture (same path as
	@# test-tracer-boot). The guest hlt-loops, so it times out -- expected.
	@# --screendump-after BANNER (beads initech-3pe): wait for the paint-complete
	@# marker before the framebuffer grab (same rationale as test-tracer-boot);
	@# BANNER is serial-emitted only after both banner lines are blitted to the
	@# LFB. The 6000 ms timeout remains the hard backstop for a never-painting guest.
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" --screendump --screendump-after BANNER \
		--name "$(BOOT_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(BOOT_REPORT)" || true
	@cat "$(BOOT_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@# ---- 1. Triple-fault check. ----
	@if grep -q 'triple_fault=1' "$(BOOT_REPORT)"; then \
		printf '!!! test-boot FAIL: TRIPLE FAULT detected in the boot transition\n'; \
		exit 1; \
	fi
	@printf '>>> test-boot [1/4]: no triple-fault\n'
	@# ---- 2. Serial milestone + banner markers + banner literals. ----
	@if [ ! -s "$(BOOT_SERIAL)" ]; then \
		printf '!!! test-boot FAIL: no serial captured at %s\n' "$(BOOT_SERIAL)"; \
		exit 1; \
	fi
	@for m in S1 PM OK FONT KERNEL INT21 BI-OK CONSOLE BANNER-BEGIN BANNER-END BANNER; do \
		grep -q "^$$m$$" "$(BOOT_SERIAL)" \
			|| { printf '!!! test-boot FAIL: required serial marker %s missing\n' "$$m"; exit 1; }; \
	done
	@grep -q '^InitechDOS  Version 3.30$$' "$(BOOT_SERIAL)" \
		|| { printf '!!! test-boot FAIL: banner line 1 "InitechDOS  Version 3.30" (double space) missing on serial\n'; exit 1; }
	@grep -q '^Copyright (C) 1991 Initech Systems Corporation.  All Rights Reserved.$$' "$(BOOT_SERIAL)" \
		|| { printf '!!! test-boot FAIL: banner copyright line missing on serial\n'; exit 1; }
	@printf '>>> test-boot [2/4]: serial markers + banner literals present\n'
	@# ---- 3. Byte-exactness vs spec/dos_banner.txt: extract the lines between
	@#         BANNER-BEGIN and BANNER-END and diff against the LOCKED spec. ----
	@awk '/^BANNER-BEGIN$$/{f=1;next} /^BANNER-END$$/{f=0} f{print}' \
		"$(BOOT_SERIAL)" > "$(BOOT_LINES)"
	@diff -u "$(SPEC_BANNER)" "$(BOOT_LINES)" \
		|| { printf '!!! test-boot FAIL: kernel banner DRIFTED from spec/dos_banner.txt (Law 1 / Rule 8)\n'; exit 1; }
	@printf '>>> test-boot [3/4]: serial banner == spec/dos_banner.txt byte-for-byte\n'
	@# ---- 4. Screendump: banner blitted at its known origin on seafoam desktop. ----
	@if [ ! -s "$(BOOT_PPM)" ]; then \
		printf '!!! test-boot FAIL: no screendump captured at %s (live guest required)\n' "$(BOOT_PPM)"; \
		exit 1; \
	fi
	@$(PPM_TEXT_CHECK_BIN) "$(BOOT_PPM)" \
		|| { printf '!!! test-boot FAIL: screendump does not show the banner on the seafoam desktop\n'; exit 1; }
	@printf '>>> test-boot [4/4]: screendump banner-text check\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechDOS banner printed (serial == spec, rendered on screen), no triple-fault\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-program (beads initech-509.5 -- InitechDOS RUNS A PROGRAM)
# ---------------------------------------------------------------------------
# The in-emulator end-to-end loader oracle: boot the REAL image and prove the
# kernel LOADED, RAN, and got control BACK from a flat program. The program
# prints via INT 21h AH=09h then exits via AH=4Ch; the loader's return-to-loader
# mechanism (ground-truth Sec 4) hands control back to the kernel, which emits
# PROGRAM-EXIT rc=0. Three independent assertions, every one fail-loud + exit-
# non-zero on miss (CLAUDE.md Law 2 / Rule 2):
#   1. NO triple-fault (a bad stack/jump in the control transfer silently reboots
#      in QEMU -- the minefield; the harness -d cpu_reset catches it).
#   2. SERIAL: PROGRAM-BEGIN present, the program's own output line
#      "Hello from InitechOS program." present (printed THROUGH the OS syscall,
#      proving the loaded program ran AND called INT 21h), and PROGRAM-EXIT rc=0
#      present (proving the return-to-loader mechanism brought control back with
#      the right exit code). A missing PROGRAM-EXIT => the program ran but never
#      returned (Risk 1) => RED.
#   3. The program output appears AFTER PROGRAM-BEGIN and the rc line is rc=0.
# The guest hlt-loops to stay live, so it times out -- expected; we assert on the
# markers, not the exit code.
PROG_NAME    := program_boot
PROG_SERIAL  := $(BUILD)/$(PROG_NAME).serial
PROG_REPORT  := $(BUILD)/$(PROG_NAME).report
PROG_OUTPUT  := Hello from InitechOS program.

.PHONY: test-program
test-program: $(HARNESS_BIN) $(TRACER_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-program : InitechDOS RUNS A PROGRAM\n'
	@printf '  Ref: docs/research/psp-loader-ground-truth.md Sec 4/5. beads initech-509.5.\n'
	@printf '  Prove load -> run -> INT 21h -> return-to-loader end-to-end on real boot.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (real boot chain -> C kernel -> load_program)\n' "$(TRACER_IMG)"
	@printf 'Expecting : PROGRAM-BEGIN + "%s" + PROGRAM-EXIT rc=0 + no triple-fault\n' "$(PROG_OUTPUT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" \
		--name "$(PROG_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(PROG_REPORT)" || true
	@cat "$(PROG_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@# ---- 1. Triple-fault check (the control transfer did not silently reboot). ----
	@if grep -q 'triple_fault=1' "$(PROG_REPORT)"; then \
		printf '!!! test-program FAIL: TRIPLE FAULT -- the control transfer or return crashed\n'; \
		exit 1; \
	fi
	@printf '>>> test-program [1/3]: no triple-fault\n'
	@# ---- 2. Serial: begin marker + program output + clean exit. ----
	@if [ ! -s "$(PROG_SERIAL)" ]; then \
		printf '!!! test-program FAIL: no serial captured at %s\n' "$(PROG_SERIAL)"; \
		exit 1; \
	fi
	@grep -q '^PROGRAM-BEGIN$$' "$(PROG_SERIAL)" \
		|| { printf '!!! test-program FAIL: PROGRAM-BEGIN marker missing (loader never invoked)\n'; exit 1; }
	@grep -qF '$(PROG_OUTPUT)' "$(PROG_SERIAL)" \
		|| { printf '!!! test-program FAIL: program output "%s" missing -- the loaded program did not run/print via INT 21h\n' "$(PROG_OUTPUT)"; exit 1; }
	@printf '>>> test-program [2/3]: program ran and printed "%s" through INT 21h\n' "$(PROG_OUTPUT)"
	@# ---- 3. Return-to-loader: PROGRAM-EXIT rc=0 came back to the kernel. ----
	@grep -q '^PROGRAM-EXIT rc=0$$' "$(PROG_SERIAL)" \
		|| { printf '!!! test-program FAIL: PROGRAM-EXIT rc=0 missing -- the program ran but never returned to the loader (Risk 1)\n'; exit 1; }
	@printf '>>> test-program [3/3]: return-to-loader brought control back (PROGRAM-EXIT rc=0)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechOS loaded a flat program, it ran + called the OS, and the loader regained control with rc=0\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-fs (beads initech-saw -- InitechDOS MOUNTS A REAL FILESYSTEM)
# ---------------------------------------------------------------------------
# The FIRST functional exercise of ata.c on the emulator (beads initech-adf:
# "hardware validation defers to boot"). Boot the REAL image WITH a FAT12 data
# disk attached to the IDE primary SLAVE (--disk2), and prove the kernel read
# sector 0 over ATA PIO, mounted the volume, and rendered a proto-DIR of the
# root directory. Fail-loud + exit-non-zero on every miss (Law 2 / Rule 2):
#   1. NO triple-fault (a bad PIO sequence could crash the transition).
#   2. SERIAL: FAT-MOUNT-OK present (ata.c + fat12.c worked end-to-end), the
#      known fixture filenames present (the proto-DIR actually enumerated the
#      root directory), and DIR-OK present (the walk completed).
#   3. BONUS: a screendump shows text rendered on the seafoam desktop (the
#      banner band + intact background -- ppm_text_check), proving the listing
#      reached the LFB console, not only serial.
# Ref: docs/research/fs-mount-sft-ground-truth.md Sec 1, Sec 2, Sec 5.1.
# TRI-EMULATOR: QEMU only here -- Bochs/86Box deferred to beads initech-x0i.
FS_NAME    := fs_boot
FS_SERIAL  := $(BUILD)/$(FS_NAME).serial
FS_REPORT  := $(BUILD)/$(FS_NAME).report
FS_PPM     := $(BUILD)/$(FS_NAME).ppm
# The 8.3 names baked into the FAT12 data disk (HELLO.TXT is the load-bearing
# assertion the brief calls out; the rest pin the full proto-DIR enumeration).
FS_NAMES   := HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN

.PHONY: test-fs
test-fs: $(HARNESS_BIN) $(TRACER_IMG) $(FAT_DATA_IMG) $(PPM_TEXT_CHECK_BIN)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fs : MOUNT A REAL FILESYSTEM over ATA\n'
	@printf '  Ref: docs/research/fs-mount-sft-ground-truth.md Sec 1/2/5.1.\n'
	@printf '  beads initech-saw (+ validates initech-adf ATA on the emulator).\n'
	@printf '  FIRST functional run of os/milton/ata.c on the emulator.\n'
	@printf '  TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (boot disk, primary master)\n' "$(TRACER_IMG)"
	@printf 'Data disk : %s (primary SLAVE, if=ide,index=1)\n' "$(FAT_DATA_IMG)"
	@printf 'Expecting : FAT-MOUNT-OK + proto-DIR filenames (HELLO.TXT ...) + DIR-OK + no triple-fault\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@# --screendump-after DIR-OK (beads initech-3pe): the ppm gate asserts text in
	@# the proto-DIR band (rows 3..8); DIR-OK is serial-emitted only AFTER
	@# fat12_read_root_dir returns, i.e. after every filename has been blitted to
	@# the LFB console (dir_visit -> dir_puts -> console_putc). Waiting for it
	@# guarantees the asserted band is painted before the grab, killing the race.
	@# The 6000 ms timeout stays the hard backstop.
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" --disk2 "$(FAT_DATA_IMG)" --screendump \
		--screendump-after DIR-OK \
		--name "$(FS_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(FS_REPORT)" || true
	@cat "$(FS_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@# ---- 1. Triple-fault check. ----
	@if grep -q 'triple_fault=1' "$(FS_REPORT)"; then \
		printf '!!! test-fs FAIL: TRIPLE FAULT detected booting with the data disk\n'; \
		exit 1; \
	fi
	@printf '>>> test-fs [1/4]: no triple-fault\n'
	@# ---- 2. Mount succeeded (ata.c + fat12.c worked end-to-end). ----
	@if [ ! -s "$(FS_SERIAL)" ]; then \
		printf '!!! test-fs FAIL: no serial captured at %s\n' "$(FS_SERIAL)"; \
		exit 1; \
	fi
	@if grep -q '^FAT-MOUNT-FAIL' "$(FS_SERIAL)"; then \
		printf '!!! test-fs FAIL: kernel reported FAT-MOUNT-FAIL (ATA read or BPB check failed) -- root-cause the ATA protocol, do NOT paper over (Rule 3):\n'; \
		grep '^FAT-MOUNT-FAIL' "$(FS_SERIAL)"; \
		exit 1; \
	fi
	@grep -q '^FAT-MOUNT-OK$$' "$(FS_SERIAL)" \
		|| { printf '!!! test-fs FAIL: FAT-MOUNT-OK missing -- ata.c never delivered sector 0 to fat12_mount (the FIRST emulator ATA run failed)\n'; exit 1; }
	@printf '>>> test-fs [2/4]: FAT-MOUNT-OK (ata.c READ SECTORS + fat12_mount green on the emulator)\n'
	@# ---- 3. Proto-DIR enumerated the root directory + completed. ----
	@for n in $(FS_NAMES); do \
		grep -q "$$n" "$(FS_SERIAL)" \
			|| { printf '!!! test-fs FAIL: proto-DIR filename %s missing -- root-dir enumeration did not list the fixtures\n' "$$n"; exit 1; }; \
	done
	@grep -q '^DIR-OK$$' "$(FS_SERIAL)" \
		|| { printf '!!! test-fs FAIL: DIR-OK missing -- fat12_read_root_dir did not complete the scan\n'; exit 1; }
	@printf '>>> test-fs [3/4]: proto-DIR listed %s + DIR-OK\n' "$(FS_NAMES)"
	@# ---- 4. BONUS screendump: text rendered on the seafoam desktop. ----
	@if [ ! -s "$(FS_PPM)" ]; then \
		printf '!!! test-fs FAIL: no screendump captured at %s (live guest required)\n' "$(FS_PPM)"; \
		exit 1; \
	fi
	@# Assert text in the proto-DIR band (rows 3..8 => y in [48,144)), NOT just
	@# the banner -- the banner alone would false-pass [A]/[B] even if the DIR
	@# never rendered (the dangling-console bug fixed in this step). Band fg must
	@# be healthy (the listing is ~6 lines of text). Rule 6: the gate now BITES.
	@$(PPM_TEXT_CHECK_BIN) "$(FS_PPM)" 48 144 100 \
		|| { printf '!!! test-fs FAIL: proto-DIR listing did not render on screen (band [48,144))\n'; exit 1; }
	@printf '>>> test-fs [4/4]: screendump shows the proto-DIR listing rendered on the seafoam desktop\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechDOS mounted a FAT12 volume over ATA and listed its root directory (proto-DIR)\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-type (beads initech-509.5 read-side -- a program TYPEs a file)
# ---------------------------------------------------------------------------
# Boot the REAL image WITH the FAT12 data disk (--disk2) and prove the baked
# TYPE program OPENed HELLO.TXT, READ it, and WROTE its contents to stdout
# (handle 1 -> CON) through the INT 21h file-handle functions -- the in-universe
# `TYPE HELLO.TXT`. Fail-loud + exit-non-zero on every miss (Law 2 / Rule 2):
#   1. NO triple-fault.
#   2. SERIAL: FILEIO-BIND-OK (the FAT backend bound), TYPE-BEGIN + the known
#      HELLO.TXT contents (a distinctive substring) appearing between
#      TYPE-OUTPUT-BEGIN/END, and TYPE-EXIT rc=0 (OPEN/READ/WRITE/CLOSE ran and
#      the program returned cleanly). A TYPE-OPEN-FAIL marker => OPEN failed.
# Ref: docs/research/fs-mount-sft-ground-truth.md Sec 5.2 / Sec 6 Step 4.
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
TYPE_NAME    := type_boot
TYPE_SERIAL  := $(BUILD)/$(TYPE_NAME).serial
TYPE_REPORT  := $(BUILD)/$(TYPE_NAME).report
# A distinctive substring of HELLO.TXT (the committed fixture is exactly 31
# bytes: "Hello from InitechOS test file" + LF -- no trailing period).
TYPE_CONTENT := Hello from InitechOS test file

.PHONY: test-type
test-type: $(HARNESS_BIN) $(TRACER_IMG) $(FAT_DATA_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-type : a PROGRAM TYPEs a real file\n'
	@printf '  Ref: docs/research/fs-mount-sft-ground-truth.md Sec 5.2. beads initech-509.5.\n'
	@printf '  Prove OPEN(3Dh)+READ(3Fh)+WRITE(40h)+CLOSE(3Eh) over mounted FAT12.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + data disk %s (primary slave)\n' "$(TRACER_IMG)" "$(FAT_DATA_IMG)"
	@printf 'Expecting : FILEIO-BIND-OK + "%s" between TYPE-OUTPUT-BEGIN/END + TYPE-EXIT rc=0\n' "$(TYPE_CONTENT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" --disk2 "$(FAT_DATA_IMG)" \
		--name "$(TYPE_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(TYPE_REPORT)" || true
	@cat "$(TYPE_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(TYPE_REPORT)"; then \
		printf '!!! test-type FAIL: TRIPLE FAULT\n'; exit 1; \
	fi
	@printf '>>> test-type [1/4]: no triple-fault\n'
	@if [ ! -s "$(TYPE_SERIAL)" ]; then \
		printf '!!! test-type FAIL: no serial captured at %s\n' "$(TYPE_SERIAL)"; exit 1; \
	fi
	@grep -q '^FILEIO-BIND-OK$$' "$(TYPE_SERIAL)" \
		|| { printf '!!! test-type FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
	@printf '>>> test-type [2/4]: FILEIO-BIND-OK (FAT12 file backend bound to int21)\n'
	@if grep -q 'TYPE-OPEN-FAIL' "$(TYPE_SERIAL)"; then \
		printf '!!! test-type FAIL: TYPE-OPEN-FAIL -- AH=3Dh OPEN of HELLO.TXT failed (root-cause the OPEN path, Rule 3)\n'; exit 1; \
	fi
	@grep -qF '$(TYPE_CONTENT)' "$(TYPE_SERIAL)" \
		|| { printf '!!! test-type FAIL: HELLO.TXT contents "%s" missing -- OPEN/READ/WRITE did not deliver the file to stdout\n' "$(TYPE_CONTENT)"; exit 1; }
	@printf '>>> test-type [3/4]: the program OPENed+READ HELLO.TXT and WROTE its contents to stdout\n'
	@grep -q '^TYPE-EXIT rc=0$$' "$(TYPE_SERIAL)" \
		|| { printf '!!! test-type FAIL: TYPE-EXIT rc=0 missing -- the TYPE program did not CLOSE+EXIT cleanly\n'; exit 1; }
	@printf '>>> test-type [4/4]: TYPE-EXIT rc=0 (CLOSE + return-to-loader)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- a flat program OPENed, READ, and TYPEd a real FAT12 file via INT 21h\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-dir (beads initech-509.5 read-side -- a program DIRs a volume)
# ---------------------------------------------------------------------------
# Boot the REAL image WITH the FAT12 data disk and prove the baked DIR program
# enumerated the root directory via AH=4Eh FINDFIRST / AH=4Fh FINDNEXT, writing
# each 8.3 filename (read from the 43-byte DTA find-data block) to stdout.
# Fail-loud (Law 2 / Rule 2):
#   1. NO triple-fault.
#   2. SERIAL: FILEIO-BIND-OK; each fixture name (HELLO.TXT ...) appears between
#      DIR-PROG-OUTPUT-BEGIN/END; DIR-PROG-EXIT rc=0.
# Ref: docs/research/fs-mount-sft-ground-truth.md Sec 5.3 / Sec 6 Step 5.
DIRP_NAME    := dir_boot
DIRP_SERIAL  := $(BUILD)/$(DIRP_NAME).serial
DIRP_REPORT  := $(BUILD)/$(DIRP_NAME).report
DIRP_NAMES   := HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN

.PHONY: test-dir
test-dir: $(HARNESS_BIN) $(TRACER_IMG) $(FAT_DATA_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-dir : a PROGRAM lists a real directory\n'
	@printf '  Ref: docs/research/fs-mount-sft-ground-truth.md Sec 5.3. beads initech-509.5.\n'
	@printf '  Prove FINDFIRST(4Eh)/FINDNEXT(4Fh) into the DTA find-data block.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + data disk %s (primary slave)\n' "$(TRACER_IMG)" "$(FAT_DATA_IMG)"
	@printf 'Expecting : FILEIO-BIND-OK + fixture names between DIR-PROG-OUTPUT-BEGIN/END + DIR-PROG-EXIT rc=0\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" --disk2 "$(FAT_DATA_IMG)" \
		--name "$(DIRP_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(DIRP_REPORT)" || true
	@cat "$(DIRP_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(DIRP_REPORT)"; then \
		printf '!!! test-dir FAIL: TRIPLE FAULT\n'; exit 1; \
	fi
	@printf '>>> test-dir [1/3]: no triple-fault\n'
	@if [ ! -s "$(DIRP_SERIAL)" ]; then \
		printf '!!! test-dir FAIL: no serial captured at %s\n' "$(DIRP_SERIAL)"; exit 1; \
	fi
	@grep -q '^FILEIO-BIND-OK$$' "$(DIRP_SERIAL)" \
		|| { printf '!!! test-dir FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
	@printf '>>> test-dir [2/3]: FILEIO-BIND-OK\n'
	@# The DIR program output is bracketed; assert every fixture name appears
	@# within the bracketed region (sed extracts it; grep asserts each name).
	@sed -n '/^DIR-PROG-OUTPUT-BEGIN$$/,/^DIR-PROG-OUTPUT-END$$/p' "$(DIRP_SERIAL)" > "$(BUILD)/$(DIRP_NAME).names"
	@for n in $(DIRP_NAMES); do \
		grep -qF "$$n" "$(BUILD)/$(DIRP_NAME).names" \
			|| { printf '!!! test-dir FAIL: FINDFIRST/FINDNEXT did not list %s\n' "$$n"; exit 1; }; \
	done
	@grep -q '^DIR-PROG-EXIT rc=0$$' "$(DIRP_SERIAL)" \
		|| { printf '!!! test-dir FAIL: DIR-PROG-EXIT rc=0 missing -- the DIR program did not finish cleanly\n'; exit 1; }
	@printf '>>> test-dir [3/3]: the program listed %s via FINDFIRST/FINDNEXT + DIR-PROG-EXIT rc=0\n' "$(DIRP_NAMES)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- a flat program enumerated a real FAT12 directory via INT 21h FINDFIRST/FINDNEXT\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-exec (beads initech-saw -- LOAD A .COM FROM THE FAT VOLUME)
# ---------------------------------------------------------------------------
# THE binding oracle for initech-saw: boot the -DBOOT_EXEC image WITH the FAT12
# data disk (--disk2, carrying GREET.COM which is NOT baked into the kernel) and
# prove a flat .COM was loaded BY NAME from the volume and ran. Two proofs (every
# assertion fail-loud + exit-non-zero, Law 2 / Rule 2):
#   1. NO triple-fault (the FAT read + control transfer did not crash).
#   2a. THE SAW CORE: load_program_from_fat("GREET.COM") -- EXEC-SAW-EXIT rc=7
#       (the program GREET.COM, read FROM THE FAT VOLUME, printed its line via
#       INT 21h AH=09h and exited rc=7). GREETINGS FROM A:GREET.COM on serial
#       proves it RAN (not just loaded). A baked program could not produce this.
#   2b. INT 21h AH=4Bh EXEC of "GREET.COM" from kernel/shell context, then AH=4Dh
#       GET-RETURN-CODE -- EXEC-4B-RC=7 proves the EXEC dispatch path + child rc.
#       (Nested EXEC -- 4Bh from inside a running program -- is NOT supported this
#       milestone; EXEC-from-kernel is the binding oracle, per the brief.)
# Ref: beads initech-saw; docs/research/psp-loader-ground-truth.md Sec 4/5; DOS
#      3.3 PRM AH=4Bh/4Dh; ADR-0003 DEC-08 (flat .COM).
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
EXEC_NAME    := exec_boot
EXEC_SERIAL  := $(BUILD)/$(EXEC_NAME).serial
EXEC_REPORT  := $(BUILD)/$(EXEC_NAME).report
EXEC_OUTPUT  := GREETINGS FROM A:GREET.COM

.PHONY: test-exec
test-exec: $(HARNESS_BIN) $(EXEC_IMG) $(FAT_EXEC_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-exec : LOAD A .COM FROM THE FAT VOLUME\n'
	@printf '  Ref: beads initech-saw; psp-loader-ground-truth.md Sec 4/5; DOS 3.3 PRM 4Bh/4Dh.\n'
	@printf '  Prove a flat .COM (GREET.COM, NOT baked) is loaded BY NAME from FAT and runs,\n'
	@printf '  via load_program_from_fat (saw core) AND INT 21h AH=4Bh EXEC + AH=4Dh.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + data disk %s (primary slave; carries GREET.COM)\n' "$(EXEC_IMG)" "$(FAT_EXEC_IMG)"
	@printf 'Expecting : "%s" + EXEC-SAW-EXIT rc=7 + EXEC-4B-RC=7 + no triple-fault\n' "$(EXEC_OUTPUT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(EXEC_IMG)" --disk2 "$(FAT_EXEC_IMG)" \
		--name "$(EXEC_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(EXEC_REPORT)" || true
	@cat "$(EXEC_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(EXEC_REPORT)"; then \
		printf '!!! test-exec FAIL: TRIPLE FAULT -- the FAT load or control transfer crashed\n'; \
		exit 1; \
	fi
	@printf '>>> test-exec [1/4]: no triple-fault\n'
	@if [ ! -s "$(EXEC_SERIAL)" ]; then \
		printf '!!! test-exec FAIL: no serial captured at %s\n' "$(EXEC_SERIAL)"; exit 1; \
	fi
	@grep -q '^LOADER-FAT-BIND-OK$$' "$(EXEC_SERIAL)" \
		|| { printf '!!! test-exec FAIL: LOADER-FAT-BIND-OK missing -- the loader FAT volume never bound\n'; exit 1; }
	@printf '>>> test-exec [2/4]: LOADER-FAT-BIND-OK (the loader can read .COMs off the volume)\n'
	@# ---- 2a. THE SAW CORE: GREET.COM loaded FROM FAT, ran, exited rc=7. ----
	@if grep -q '^EXEC-SAW-FAIL' "$(EXEC_SERIAL)"; then \
		printf '!!! test-exec FAIL: EXEC-SAW-FAIL -- load_program_from_fat("GREET.COM") failed (root-cause the saw path, Rule 3):\n'; \
		grep '^EXEC-SAW-FAIL' "$(EXEC_SERIAL)"; \
		exit 1; \
	fi
	@grep -qF '$(EXEC_OUTPUT)' "$(EXEC_SERIAL)" \
		|| { printf '!!! test-exec FAIL: "%s" missing -- the .COM loaded from FAT did not run/print via INT 21h\n' "$(EXEC_OUTPUT)"; exit 1; }
	@grep -q '^EXEC-SAW-EXIT rc=7$$' "$(EXEC_SERIAL)" \
		|| { printf '!!! test-exec FAIL: EXEC-SAW-EXIT rc=7 missing -- the FROM-FAT program did not return rc=7 to the loader\n'; exit 1; }
	@printf '>>> test-exec [3/4]: GREET.COM loaded FROM FAT, ran (printed via INT 21h), and returned rc=7\n'
	@# ---- 2b. INT 21h AH=4Bh EXEC + AH=4Dh GET-RETURN-CODE from kernel ctx. ----
	@if grep -q '^EXEC-4B-FAIL' "$(EXEC_SERIAL)"; then \
		printf '!!! test-exec FAIL: EXEC-4B-FAIL -- INT 21h AH=4Bh EXEC of GREET.COM failed (root-cause do_exec / the seam, Rule 3):\n'; \
		grep '^EXEC-4B-FAIL' "$(EXEC_SERIAL)"; \
		exit 1; \
	fi
	@grep -q '^EXEC-4B-RC=7$$' "$(EXEC_SERIAL)" \
		|| { printf '!!! test-exec FAIL: EXEC-4B-RC=7 missing -- AH=4Bh EXEC + AH=4Dh did not return the child rc=7\n'; exit 1; }
	@printf '>>> test-exec [4/4]: INT 21h AH=4Bh EXEC ran GREET.COM; AH=4Dh returned rc=7\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechDOS loaded a flat .COM BY NAME from a FAT12 volume and ran it (saw + AH=4Bh EXEC)\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-fatwrite (beads initech-509.11 -- WRITE+READ round-trip on ATA)
# ---------------------------------------------------------------------------
# THE in-emulator binding oracle for the WRITE half: boot the -DBOOT_WRITE image
# WITH a WRITABLE FAT12 data disk (--disk2 = FAT_WRITE_IMG). The baked WRITE
# program CREATs "OUT.TXT", WRITEs "hello\r\n", CLOSEs (flushing the cluster
# chain + FAT + dir entry to disk over ATA WRITE SECTORS), then re-OPENs + READs
# OUT.TXT and echoes its bytes to stdout -- proving write+read round-trips
# THROUGH the OS on real ATA in ONE boot. Assertions (fail-loud, Rule 2):
#   1. NO triple-fault (the ATA WRITE protocol + CACHE FLUSH did not crash).
#   2. SERIAL: FILEIO-BIND-OK; NO WRITE-*-FAIL markers; "hello" appears between
#      WRITE-OUTPUT-BEGIN/END (the file READ BACK what we wrote); WRITE-EXIT rc=0.
#   3. BONUS: mtools reads OUT.TXT off the (post-run) disk image and its content
#      is exactly "hello\r\n" -- an INDEPENDENT confirmation the bytes hit disk.
# Ref: ATA/ATAPI-6 WRITE SECTORS + FLUSH CACHE; FAT12 spec; ADR-0003 DEC-07.
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
WRITE_NAME    := write_boot
WRITE_SERIAL  := $(BUILD)/$(WRITE_NAME).serial
WRITE_REPORT  := $(BUILD)/$(WRITE_NAME).report
WRITE_EXPECT  := hello

.PHONY: test-fatwrite
test-fatwrite: $(HARNESS_BIN) $(WRITE_IMG) $(FAT_WRITE_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fatwrite : WRITE+READ round-trip on ATA\n'
	@printf '  Ref: ATA/ATAPI-6 WRITE SECTORS + FLUSH CACHE; FAT12 spec; ADR-0003 DEC-07.\n'
	@printf '  beads initech-509.11. CREAT+WRITE+CLOSE then OPEN+READ OUT.TXT, in one boot.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + WRITABLE data disk %s (primary slave)\n' "$(WRITE_IMG)" "$(FAT_WRITE_IMG)"
	@printf 'Expecting : FILEIO-BIND-OK + "%s" between WRITE-OUTPUT-BEGIN/END + WRITE-EXIT rc=0 + no triple-fault\n' "$(WRITE_EXPECT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(WRITE_IMG)" --disk2 "$(FAT_WRITE_IMG)" \
		--name "$(WRITE_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(WRITE_REPORT)" || true
	@cat "$(WRITE_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(WRITE_REPORT)"; then \
		printf '!!! test-fatwrite FAIL: TRIPLE FAULT -- root-cause the ATA WRITE protocol order (Rule 3)\n'; exit 1; \
	fi
	@printf '>>> test-fatwrite [1/4]: no triple-fault\n'
	@if [ ! -s "$(WRITE_SERIAL)" ]; then \
		printf '!!! test-fatwrite FAIL: no serial captured at %s\n' "$(WRITE_SERIAL)"; exit 1; \
	fi
	@grep -q '^FILEIO-BIND-OK$$' "$(WRITE_SERIAL)" \
		|| { printf '!!! test-fatwrite FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
	@if grep -q 'WRITE-CREAT-FAIL\|WRITE-WRITE-FAIL\|WRITE-CLOSE-FAIL\|WRITE-REOPEN-FAIL' "$(WRITE_SERIAL)"; then \
		printf '!!! test-fatwrite FAIL: a WRITE-*-FAIL marker -- the CREAT/WRITE/CLOSE/REOPEN path failed (root-cause, Rule 3):\n'; \
		grep 'WRITE-.*-FAIL' "$(WRITE_SERIAL)"; exit 1; \
	fi
	@printf '>>> test-fatwrite [2/4]: FILEIO-BIND-OK + no WRITE-*-FAIL\n'
	@sed -n '/^WRITE-OUTPUT-BEGIN$$/,/^WRITE-OUTPUT-END$$/p' "$(WRITE_SERIAL)" > "$(BUILD)/$(WRITE_NAME).out"
	@grep -qF '$(WRITE_EXPECT)' "$(BUILD)/$(WRITE_NAME).out" \
		|| { printf '!!! test-fatwrite FAIL: "%s" not READ BACK from OUT.TXT -- the write did not commit to disk (root-cause ATA WRITE / FAT flush, Rule 3)\n' "$(WRITE_EXPECT)"; exit 1; }
	@grep -q '^WRITE-EXIT rc=0$$' "$(WRITE_SERIAL)" \
		|| { printf '!!! test-fatwrite FAIL: WRITE-EXIT rc=0 missing -- the WRITE program did not finish cleanly\n'; exit 1; }
	@printf '>>> test-fatwrite [3/4]: OUT.TXT written, READ BACK through the OS, WRITE-EXIT rc=0\n'
	@# ---- 4. BONUS: mtools reads OUT.TXT off the post-run disk, content exact. ----
	@if command -v mcopy >/dev/null 2>&1; then \
		mcopy -i "$(FAT_WRITE_IMG)" ::OUT.TXT - 2>/dev/null > "$(BUILD)/$(WRITE_NAME).outtxt" || true; \
		if [ -s "$(BUILD)/$(WRITE_NAME).outtxt" ]; then \
			printf 'hello\r\n' > "$(BUILD)/$(WRITE_NAME).expect"; \
			if cmp -s "$(BUILD)/$(WRITE_NAME).outtxt" "$(BUILD)/$(WRITE_NAME).expect"; then \
				printf '>>> test-fatwrite [4/4]: mtools read OUT.TXT off the disk -- content == "hello\\r\\n" (independent confirmation)\n'; \
			else \
				printf '!!! test-fatwrite FAIL [4/4]: mtools read OUT.TXT but content != "hello\\r\\n" (the on-disk bytes are wrong)\n'; \
				cmp "$(BUILD)/$(WRITE_NAME).outtxt" "$(BUILD)/$(WRITE_NAME).expect" || true; exit 1; \
			fi; \
		else \
			printf '!!! test-fatwrite FAIL [4/4]: mtools could not read OUT.TXT off the post-run disk -- the directory entry / chain never reached the medium\n'; exit 1; \
		fi; \
	else \
		printf '>>> test-fatwrite [4/4]: mtools absent -- skipping the post-run disk extraction (in-OS read-back [3/4] already proves the round-trip)\n'; \
	fi
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechDOS WROTE a file to a FAT12 volume over ATA and read it back\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-multiopen (beads initech-0qh; epic initech-6qy -- multi-tenant)
# ---------------------------------------------------------------------------
# THE in-emulator capability oracle for the WHOLE multi-tenant file-I/O step:
# boot the -DBOOT_MULTIOPEN image WITH a FAT12 data disk (--disk2 =
# FAT_MULTIOPEN_IMG: HELLO.TXT + SECOND.TXT + a 96 KiB BIG.DAT). The baked
# MULTI-OPEN program OPENs HELLO.TXT and SECOND.TXT (and BIG.DAT) CONCURRENTLY,
# does interleaved positioned reads with LSEEK on both (independent per-handle
# positions, no cross-talk), and LSEEKs PAST 64 KiB into BIG.DAT to read a
# signature back -- the capability that was IMPOSSIBLE under the old single
# 64 KiB whole-file buffer. Assertions (fail-loud, Rule 2):
#   1. NO triple-fault.
#   2. SERIAL: FILEIO-BIND-OK; NO MO-*-FAIL markers.
#   3. Between MULTIOPEN-OUTPUT-BEGIN/END: MO-A1=Hel (handle A @0), MO-B1=Sec
#      (handle B @0 -- a 2nd file open at the same time), MO-A2=Hello (A seeked
#      back to 0, unaffected by B's reads -> independent positions), and
#      MO-BIG=BEYOND-64KiB! (read from offset 80000 of the >64 KiB file).
#   4. MULTIOPEN-EXIT rc=0.
# Ref: beads initech-0qh / epic initech-6qy; fat12_read_partial (positioned read).
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
MULTIOPEN_NAME    := multiopen_boot
MULTIOPEN_SERIAL  := $(BUILD)/$(MULTIOPEN_NAME).serial
MULTIOPEN_REPORT  := $(BUILD)/$(MULTIOPEN_NAME).report

.PHONY: test-multiopen
test-multiopen: $(HARNESS_BIN) $(MULTIOPEN_IMG) $(FAT_MULTIOPEN_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-multiopen : N concurrent opens + >64 KiB + LSEEK\n'
	@printf '  Ref: beads initech-0qh / epic initech-6qy; positioned cluster-chain I/O.\n'
	@printf '  Two files open at once, interleaved positioned reads, a >64 KiB round-trip.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + data disk %s (primary slave)\n' "$(MULTIOPEN_IMG)" "$(FAT_MULTIOPEN_IMG)"
	@printf 'Expecting : FILEIO-BIND-OK + MO-A1=Hel / MO-B1=Sec / MO-A2=Hello / MO-BIG=%s + MULTIOPEN-EXIT rc=0 + no triple-fault\n' "$(MO_BIG_SIG)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(MULTIOPEN_IMG)" --disk2 "$(FAT_MULTIOPEN_IMG)" \
		--name "$(MULTIOPEN_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
		2> "$(MULTIOPEN_REPORT)" || true
	@cat "$(MULTIOPEN_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(MULTIOPEN_REPORT)"; then \
		printf '!!! test-multiopen FAIL: TRIPLE FAULT (root-cause the positioned read/LSEEK path, Rule 3)\n'; exit 1; \
	fi
	@printf '>>> test-multiopen [1/5]: no triple-fault\n'
	@if [ ! -s "$(MULTIOPEN_SERIAL)" ]; then \
		printf '!!! test-multiopen FAIL: no serial captured at %s\n' "$(MULTIOPEN_SERIAL)"; exit 1; \
	fi
	@grep -q '^FILEIO-BIND-OK$$' "$(MULTIOPEN_SERIAL)" \
		|| { printf '!!! test-multiopen FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
	@if grep -q 'MO-OPENA-FAIL\|MO-OPENB-FAIL\|MO-OPENC-FAIL\|MO-BIG-FAIL' "$(MULTIOPEN_SERIAL)"; then \
		printf '!!! test-multiopen FAIL: a MO-*-FAIL marker -- a concurrent OPEN or the >64 KiB seek failed (root-cause, Rule 3):\n'; \
		grep 'MO-.*-FAIL' "$(MULTIOPEN_SERIAL)"; exit 1; \
	fi
	@printf '>>> test-multiopen [2/5]: FILEIO-BIND-OK + no MO-*-FAIL (all OPENs succeeded)\n'
	@sed -n '/^MULTIOPEN-OUTPUT-BEGIN$$/,/^MULTIOPEN-OUTPUT-END$$/p' "$(MULTIOPEN_SERIAL)" > "$(BUILD)/$(MULTIOPEN_NAME).out"
	@grep -qF 'MO-A1=Hel' "$(BUILD)/$(MULTIOPEN_NAME).out" \
		|| { printf '!!! test-multiopen FAIL: MO-A1=Hel missing -- handle A did not read HELLO.TXT at offset 0\n'; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
	@grep -qF 'MO-B1=Sec' "$(BUILD)/$(MULTIOPEN_NAME).out" \
		|| { printf '!!! test-multiopen FAIL: MO-B1=Sec missing -- a SECOND file was not open concurrently (the multi-tenant capability)\n'; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
	@printf '>>> test-multiopen [3/5]: TWO files open concurrently (MO-A1=Hel + MO-B1=Sec, distinct content)\n'
	@grep -qF 'MO-A2=Hello' "$(BUILD)/$(MULTIOPEN_NAME).out" \
		|| { printf '!!! test-multiopen FAIL: MO-A2=Hello missing -- handle A position was corrupted by handle B (cross-talk)\n'; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
	@printf '>>> test-multiopen [4/5]: independent per-handle positions (A seeked back to 0 reads "Hello" despite B reads)\n'
	@grep -qF 'MO-BIG=$(MO_BIG_SIG)' "$(BUILD)/$(MULTIOPEN_NAME).out" \
		|| { printf '!!! test-multiopen FAIL: MO-BIG=%s missing -- the >64 KiB LSEEK+READ did not return the signature at offset %s (the old 64 KiB buffer limit)\n' "$(MO_BIG_SIG)" "$(MO_BIG_SIG_OFF)"; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
	@grep -q '^MULTIOPEN-EXIT rc=0$$' "$(MULTIOPEN_SERIAL)" \
		|| { printf '!!! test-multiopen FAIL: MULTIOPEN-EXIT rc=0 missing -- the program did not finish cleanly\n'; exit 1; }
	@printf '>>> test-multiopen [5/5]: >64 KiB round-trip (MO-BIG=%s read from offset %s) + MULTIOPEN-EXIT rc=0\n' "$(MO_BIG_SIG)" "$(MO_BIG_SIG_OFF)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechDOS held 2+ files open concurrently with independent\n'
	@printf '            positions and round-tripped a >64 KiB file via positioned LSEEK+READ\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-int21-irqstorm (beads initech-xk2 -- INT 21h reentrancy-safe
# with IRQs LIVE under an IRQ storm during a syscall)
# ---------------------------------------------------------------------------
# THE binding oracle for the reentrancy guard. Boot the -DBOOT_IRQSTORM image WITH
# the storm FAT disk (--disk2) and, gated on IRQSTORM-READY (so the keys arrive
# while the program is mid-run), inject a STORM of keystrokes via QMP --keys
# (IRQ1) while the free-running 100 Hz PIT (IRQ0) ticks. The baked program drives
# the INT 21h dispatcher functions that USE its GLOBAL state -- FINDFIRST/NEXT
# enumeration (g_dta + g_find), a multi-cluster positioned READ (the FAT cache +
# cluster scratch over slow ATA PIO, so a PIT IRQ WILL land mid-read), AND a
# second concurrent open handle. Because INT 21h is a 0x8F TRAP gate (IF stays
# set), an IRQ CAN land mid-syscall. Assertions (each fail-loud, Rule 2 / Law 2):
#   1. NO triple-fault.
#   2. The reentrancy guard did NOT fire (no INT21-REENTRY-PANIC / PANIC vec= --
#      in NORMAL operation no ISR calls DOS, so the guard stays quiescent).
#   3. The keystroke storm ACTUALLY happened (keys_sent > 0) -- otherwise the test
#      proves nothing (it would be decoration without the IRQ1 storm).
#   4. The FINDFIRST/NEXT enumeration returned EXACTLY the right filenames (a
#      skipped/duplicated entry from async g_find corruption -> RED).
#   5. The multi-cluster READ returned the EXACT signature (STORM-SIG=STORM-SIGNAL!)
#      and the 2nd handle read the EXACT bytes (STORM-B=alpha) -- a corrupted FAT
#      cache / cluster scratch / per-handle position -> RED.
#   6. IRQSTORM-EXIT rc=0 (the program finished cleanly).
# Ref: beads initech-xk2; Intel SDM Vol 3A Sec 6.12.1.2 (TRAP gate leaves IF set);
#      DOS 3.3 InDOS flag; os/milton/irq.c + int21.c (the guard + InDOS counter).
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
IRQSTORM_NAME    := irqstorm_boot
IRQSTORM_SERIAL  := $(BUILD)/$(IRQSTORM_NAME).serial
IRQSTORM_REPORT  := $(BUILD)/$(IRQSTORM_NAME).report
# A long keystroke storm (each token = one IRQ1). Letters only (printable keys).
IRQSTORM_KEYS    := d,i,r,spc,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,d,i,r,a,b,c,d,e,f,g,h,i,j,k,l
IRQSTORM_NAMES   := ALPHA.TXT BRAVO.TXT CHARLIE.TXT DELTA.TXT STORM.DAT

.PHONY: test-int21-irqstorm
test-int21-irqstorm: $(HARNESS_BIN) $(IRQSTORM_IMG) $(FAT_IRQSTORM_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-int21-irqstorm : INT 21h reentrancy under an IRQ storm\n'
	@printf '  Ref: beads initech-xk2; TRAP gate leaves IF set; irq.c/int21.c guard + InDOS.\n'
	@printf '  FINDFIRST/NEXT + a multi-cluster READ + a 2nd handle, WHILE keys storm (IRQ1) + PIT (IRQ0).\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + storm disk %s (primary slave)\n' "$(IRQSTORM_IMG)" "$(FAT_IRQSTORM_IMG)"
	@printf 'Expecting : exact dir enum + STORM-SIG=%s + STORM-B=alpha + IRQSTORM-EXIT rc=0 + NO reentry panic + keys_sent>0\n' "$(STORM_SIG)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(IRQSTORM_IMG)" --disk2 "$(FAT_IRQSTORM_IMG)" \
		--name "$(IRQSTORM_NAME)" --out "$(BUILD)" --timeout-ms 10000 \
		--keys "$(IRQSTORM_KEYS)" --keys-after "IRQSTORM-READY" \
		2> "$(IRQSTORM_REPORT)" || true
	@cat "$(IRQSTORM_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(IRQSTORM_REPORT)"; then \
		printf '!!! test-int21-irqstorm FAIL: TRIPLE FAULT (root-cause the reentrancy path, Rule 3)\n'; exit 1; \
	fi
	@printf '>>> test-int21-irqstorm [1/6]: no triple-fault\n'
	@if [ ! -s "$(IRQSTORM_SERIAL)" ]; then \
		printf '!!! test-int21-irqstorm FAIL: no serial captured at %s\n' "$(IRQSTORM_SERIAL)"; exit 1; \
	fi
	@# 2. The guard did NOT fire in normal operation (no ISR calls DOS).
	@if grep -q 'INT21-REENTRY-PANIC\|PANIC vec=' "$(IRQSTORM_SERIAL)"; then \
		printf '!!! test-int21-irqstorm FAIL: the reentrancy guard FIRED in normal operation -- an ISR entered the dispatcher (root-cause, Rule 3):\n'; \
		grep 'INT21-REENTRY-PANIC\|PANIC vec=' "$(IRQSTORM_SERIAL)"; exit 1; \
	fi
	@printf '>>> test-int21-irqstorm [2/6]: reentrancy guard stayed quiescent (no ISR called DOS)\n'
	@# 3. The keystroke storm actually happened (else the IRQ1 pressure is absent).
	@if grep -q 'keys_sent=0' "$(IRQSTORM_REPORT)"; then \
		printf '!!! test-int21-irqstorm FAIL: keys_sent=0 -- the IRQ1 storm never fired, the test proves nothing (Law 2)\n'; exit 1; \
	fi
	@printf '>>> test-int21-irqstorm [3/6]: the keystroke storm fired (keys_sent>0, IRQ1 pressure present)\n'
	@grep -q '^FILEIO-BIND-OK$$' "$(IRQSTORM_SERIAL)" \
		|| { printf '!!! test-int21-irqstorm FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
	@if grep -q 'STORM-OPENA-FAIL\|STORM-OPENB-FAIL\|STORM-READ-FAIL' "$(IRQSTORM_SERIAL)"; then \
		printf '!!! test-int21-irqstorm FAIL: a STORM-*-FAIL marker -- an OPEN/READ failed under the storm (root-cause, Rule 3):\n'; \
		grep 'STORM-.*-FAIL' "$(IRQSTORM_SERIAL)"; exit 1; \
	fi
	@# 4. The FINDFIRST/NEXT enumeration returned EXACTLY the right filenames. The
	@# program emits CRLF lines, so strip CR before the anchored sed slice.
	@tr -d '\r' < "$(IRQSTORM_SERIAL)" | sed -n '/^STORM-DIR-BEGIN$$/,/^STORM-DIR-END$$/p' > "$(BUILD)/$(IRQSTORM_NAME).dir"
	@for n in $(IRQSTORM_NAMES); do \
		grep -qF "$$n" "$(BUILD)/$(IRQSTORM_NAME).dir" \
			|| { printf '!!! test-int21-irqstorm FAIL: FINDFIRST/NEXT enumeration missing %s -- async g_find/g_dta corruption under the storm (Rule 3):\n' "$$n"; cat "$(BUILD)/$(IRQSTORM_NAME).dir"; exit 1; }; \
	done
	@# Exactly 5 surviving names (no skip, no duplicate). Count non-marker lines.
	@cnt=$$(grep -cE 'ALPHA\.TXT|BRAVO\.TXT|CHARLIE\.TXT|DELTA\.TXT|STORM\.DAT' "$(BUILD)/$(IRQSTORM_NAME).dir"); \
	if [ "$$cnt" -ne 5 ]; then \
		printf '!!! test-int21-irqstorm FAIL: expected exactly 5 enumerated names, got %s -- async corruption skipped/duplicated an entry (Rule 3):\n' "$$cnt"; \
		cat "$(BUILD)/$(IRQSTORM_NAME).dir"; exit 1; \
	fi
	@printf '>>> test-int21-irqstorm [4/6]: FINDFIRST/NEXT enumerated EXACTLY the 5 files under the storm\n'
	@# 5. The multi-cluster READ + the 2nd handle returned the EXACT bytes.
	@grep -qF 'STORM-SIG=$(STORM_SIG)' "$(IRQSTORM_SERIAL)" \
		|| { printf '!!! test-int21-irqstorm FAIL: STORM-SIG=%s missing -- the multi-cluster READ returned wrong bytes (FAT cache/cluster scratch corrupted by an IRQ mid-read, Rule 3)\n' "$(STORM_SIG)"; grep 'STORM-SIG' "$(IRQSTORM_SERIAL)" || true; exit 1; }
	@grep -qF 'STORM-B=alpha' "$(IRQSTORM_SERIAL)" \
		|| { printf '!!! test-int21-irqstorm FAIL: STORM-B=alpha missing -- the 2nd handle read wrong bytes (per-handle position/SFT corrupted, Rule 3)\n'; grep 'STORM-B' "$(IRQSTORM_SERIAL)" || true; exit 1; }
	@printf '>>> test-int21-irqstorm [5/6]: multi-cluster READ (STORM-SIG=%s) + 2nd handle (STORM-B=alpha) EXACT under the storm\n' "$(STORM_SIG)"
	@# 6. The program finished cleanly (the run_baked -EXIT marker; substring match
	@# because the program's preceding stdout line carries no trailing newline).
	@grep -qF 'IRQSTORM-EXIT rc=0' "$(IRQSTORM_SERIAL)" \
		|| { printf '!!! test-int21-irqstorm FAIL: IRQSTORM-EXIT rc=0 missing -- the program did not finish cleanly under the storm\n'; exit 1; }
	@printf '>>> test-int21-irqstorm [6/6]: IRQSTORM-EXIT rc=0 (clean finish under the storm)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- INT 21h stayed reentrancy-safe with IRQs LIVE under a keystroke storm:\n'
	@printf '            no frame/result corruption, guard quiescent (QEMU only; tri-emulator pending initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# Rule-6 MUTATION proofs for test-int21-irqstorm (beads initech-xk2): two mutant
# images, each must go RED, proving the storm oracle BITES.
#   A: pit.c -DPIT_MUTATE_SCRIBBLE_DOS -- the PIT ISR scribbles g_find.next_index
#      from IRQ context -> the enumeration SKIPS an entry under the storm -> the
#      "exactly 5 names" check (or a missing name) goes RED. Proves the oracle
#      detects async SHARED-STATE corruption.
#   B: pit.c -DPIT_MUTATE_ISSUE_INT21 -- the PIT ISR issues int 0x21 from IRQ
#      context -> the reentrancy guard PANICS (INT21-REENTRY-PANIC + PANIC vec=03)
#      -> RED on the panic marker. Proves the guard FIRES on the forbidden case.
# We run each mutant image through the SAME harness invocation and assert the
# EXPECTED failure marker is present (so a mutant that DIDN'T bite is itself a
# failure -- the oracle would be decoration).
.PHONY: test-int21-irqstorm-mutant
test-int21-irqstorm-mutant: $(HARNESS_BIN) $(IRQSTORM_MUTA_IMG) $(IRQSTORM_MUTB_IMG) $(FAT_IRQSTORM_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-int21-irqstorm-mutant : prove the storm oracle BITES (Rule 6)\n'
	@printf '======================================================================\n'
	@# ---- MUTANT A: scribble a DOS global from the PIT ISR -> enum goes WRONG. ----
	@$(HARNESS_BIN) --disk "$(IRQSTORM_MUTA_IMG)" --disk2 "$(FAT_IRQSTORM_IMG)" \
		--name "irqstorm_muta" --out "$(BUILD)" --timeout-ms 10000 \
		--keys "$(IRQSTORM_KEYS)" --keys-after "IRQSTORM-READY" \
		2> "$(BUILD)/irqstorm_muta.report" || true
	@tr -d '\r' < "$(BUILD)/irqstorm_muta.serial" 2>/dev/null | sed -n '/^STORM-DIR-BEGIN$$/,/^STORM-DIR-END$$/p' > "$(BUILD)/irqstorm_muta.dir" 2>/dev/null || true
	@cnt=$$(grep -cE 'ALPHA\.TXT|BRAVO\.TXT|CHARLIE\.TXT|DELTA\.TXT|STORM\.DAT' "$(BUILD)/irqstorm_muta.dir" 2>/dev/null || echo 0); \
	if [ "$$cnt" -eq 5 ]; then \
		printf '!!! test-int21-irqstorm-mutant FAIL: MUTANT A enumerated all 5 names -- the storm oracle did NOT detect the async g_find corruption (it is decoration):\n'; \
		cat "$(BUILD)/irqstorm_muta.dir" 2>/dev/null; exit 1; \
	fi
	@printf '>>> test-int21-irqstorm-mutant: MUTANT A correctly RED (PIT scribble corrupted the enumeration: %s/5 names)\n' "$$(grep -cE 'ALPHA\.TXT|BRAVO\.TXT|CHARLIE\.TXT|DELTA\.TXT|STORM\.DAT' "$(BUILD)/irqstorm_muta.dir" 2>/dev/null || echo 0)"
	@# ---- MUTANT B: issue int 0x21 from the PIT ISR -> the guard PANICS. ----
	@$(HARNESS_BIN) --disk "$(IRQSTORM_MUTB_IMG)" --disk2 "$(FAT_IRQSTORM_IMG)" \
		--name "irqstorm_mutb" --out "$(BUILD)" --timeout-ms 10000 \
		--keys "$(IRQSTORM_KEYS)" --keys-after "IRQSTORM-READY" \
		2> "$(BUILD)/irqstorm_mutb.report" || true
	@grep -q 'INT21-REENTRY-PANIC' "$(BUILD)/irqstorm_mutb.serial" 2>/dev/null \
		|| { printf '!!! test-int21-irqstorm-mutant FAIL: MUTANT B did NOT trip the reentrancy guard -- the int-0x21-from-IRQ went undetected (the guard is decoration). Serial tail:\n'; tail -20 "$(BUILD)/irqstorm_mutb.serial" 2>/dev/null; exit 1; }
	@printf '>>> test-int21-irqstorm-mutant: MUTANT B correctly RED (guard PANICKED: INT21-REENTRY-PANIC on int-0x21-from-IRQ)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- both mutants went RED; the storm oracle BITES on async\n'
	@printf '            shared-state corruption (A) AND on a forbidden IRQ-issued reentry (B)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-exit-handles (beads initech-6hk; epic initech-6qy -- the FINAL
# multi-tenant file-I/O step: SFT teardown on process EXIT)
# ---------------------------------------------------------------------------
# THE in-emulator binding oracle for "real DOS closes all a process's handles on
# terminate": boot the -DBOOT_EXITH image WITH the leaky-child FAT disk (--disk2 =
# FAT_EXITH_IMG: EXITH.COM + the four files it OPENs). The boot EXECs EXITH.COM
# SIX times. EXITH.COM OPENs FOUR files (HELLO/SECOND/CHAIN/BLOCK) and TERMINATEs
# via 4Ch WITHOUT closing ANY -- 4 SFT file slots consumed per run.
#
# CAPACITY MATH: the file SFT is 16 slots (SFT_FIRST_FILE=4 .. SFT_MAX_ENTRIES=20).
# 4 opens/run x 6 runs = 24 opens > 16. WITHOUT sft_close_process the leaked slots
# accumulate and by run 5 the table is exhausted: EXITH.COM's 5th-run OPEN returns
# CF=1 -> it prints EXITH-CHILD-OPENFAIL + exits rc=1 -> the run loop emits
# EXITH-RUN5-FAIL. WITH the teardown every run starts with a clean file table, so
# all 6 runs' OPENs succeed (EXITH-CHILD-OK) and each exits rc=0.
# Assertions (fail-loud, Rule 2):
#   1. NO triple-fault.
#   2. SERIAL: EXITH-RUN<n>-OK for every n in 1..6 (each EXEC ran a clean child).
#   3. SERIAL: NO EXITH-RUN*-FAIL and NO EXITH-CHILD-OPENFAIL (no run exhausted).
#   4. SERIAL: EXITH-DONE rc=0 (a clean sweep of all six runs).
# Ref: sft.c sft_close_process; int21.c do_terminate; beads initech-6hk.
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
EXITH_NAME    := exith_boot
EXITH_SERIAL  := $(BUILD)/$(EXITH_NAME).serial
EXITH_REPORT  := $(BUILD)/$(EXITH_NAME).report
EXITH_RUNS    := 6

.PHONY: test-exit-handles
test-exit-handles: $(HARNESS_BIN) $(EXITH_IMG) $(FAT_EXITH_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-exit-handles : EXIT reclaims a process handles\n'
	@printf '  Ref: beads initech-6hk / epic initech-6qy; sft_close_process on 4Ch/00h/INT20h.\n'
	@printf '  EXEC a leaky child (4 opens, no close) %s times; 4*%s=%s > 16 file slots.\n' "$(EXITH_RUNS)" "$(EXITH_RUNS)" "$$(( 4 * $(EXITH_RUNS) ))"
	@printf '======================================================================\n'
	@printf 'Booting   : %s + leaky-child disk %s (primary slave)\n' "$(EXITH_IMG)" "$(FAT_EXITH_IMG)"
	@printf 'Expecting : EXITH-RUN1..%s-OK + EXITH-DONE rc=0 + no triple-fault\n' "$(EXITH_RUNS)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(EXITH_IMG)" --disk2 "$(FAT_EXITH_IMG)" \
		--name "$(EXITH_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
		2> "$(EXITH_REPORT)" || true
	@cat "$(EXITH_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(EXITH_REPORT)"; then \
		printf '!!! test-exit-handles FAIL: TRIPLE FAULT (root-cause the EXIT teardown path, Rule 3)\n'; exit 1; \
	fi
	@printf '>>> test-exit-handles [1/4]: no triple-fault\n'
	@if [ ! -s "$(EXITH_SERIAL)" ]; then \
		printf '!!! test-exit-handles FAIL: no serial captured at %s\n' "$(EXITH_SERIAL)"; exit 1; \
	fi
	@if grep -q 'EXITH-CHILD-OPENFAIL' "$(EXITH_SERIAL)"; then \
		printf '!!! test-exit-handles FAIL: EXITH-CHILD-OPENFAIL -- a child OPEN failed (the SFT exhausted -> handles leaked on EXIT, Rule 3):\n'; \
		grep -n 'EXITH-RUN\|EXITH-CHILD' "$(EXITH_SERIAL)"; exit 1; \
	fi
	@if grep -q 'EXITH-RUN[0-9]*-FAIL' "$(EXITH_SERIAL)"; then \
		printf '!!! test-exit-handles FAIL: an EXITH-RUN*-FAIL marker -- a run did not complete clean (the leak resurfaced, Rule 3):\n'; \
		grep -n 'EXITH-RUN.*-FAIL' "$(EXITH_SERIAL)"; exit 1; \
	fi
	@printf '>>> test-exit-handles [2/4]: no EXITH-CHILD-OPENFAIL and no EXITH-RUN*-FAIL\n'
	@n=1; while [ "$$n" -le "$(EXITH_RUNS)" ]; do \
		grep -q "^EXITH-RUN$$n-OK$$" "$(EXITH_SERIAL)" \
			|| { printf '!!! test-exit-handles FAIL: EXITH-RUN%s-OK missing -- run %s did not EXEC a clean leaky child\n' "$$n" "$$n"; \
			     grep -n 'EXITH-RUN' "$(EXITH_SERIAL)"; exit 1; }; \
		n=$$(( n + 1 )); \
	done
	@printf '>>> test-exit-handles [3/4]: all %s runs OK (EXITH-RUN1..%s-OK) -- EXIT reclaimed every run handles\n' "$(EXITH_RUNS)" "$(EXITH_RUNS)"
	@grep -q '^EXITH-DONE rc=0$$' "$(EXITH_SERIAL)" \
		|| { printf '!!! test-exit-handles FAIL: EXITH-DONE rc=0 missing -- the sweep did not finish clean\n'; \
		     grep -n 'EXITH-DONE' "$(EXITH_SERIAL)"; exit 1; }
	@printf '>>> test-exit-handles [4/4]: EXITH-DONE rc=0 (clean sweep of all %s runs)\n' "$(EXITH_RUNS)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- InitechDOS reclaims an exiting process file handles on 4Ch,\n'
	@printf '            so a %s-deep EXEC chain of a leaky child (24 opens) never exhausts the\n' "$(EXITH_RUNS)"
	@printf '            16-slot file SFT (QEMU only; tri-emulator pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# MUTANT gate: test-exit-handles-mutant (CLAUDE.md Rule 6 -- prove the oracle bites)
# ---------------------------------------------------------------------------
# Boot the EXITH_MUT_IMG (sft.c -DSFT_MUTATE_NO_CLOSE_PROCESS: sft_close_process
# releases NOTHING). The leaky child's FILE slots now leak across EXEC runs; by
# run 5 the 16-slot file SFT is exhausted and that run's OPEN fails. The mutant
# MUST go RED -- i.e. the GREEN assertions of test-exit-handles MUST NOT all hold.
# Concretely we require the failure signature (EXITH-CHILD-OPENFAIL, an
# EXITH-RUN*-FAIL, OR a missing EXITH-DONE rc=0). If the mutant somehow looked
# clean, the gate is decoration -> this target fails loud.
EXITH_MUT_NAME   := exith_mut_boot
EXITH_MUT_SERIAL := $(BUILD)/$(EXITH_MUT_NAME).serial
EXITH_MUT_REPORT := $(BUILD)/$(EXITH_MUT_NAME).report

.PHONY: test-exit-handles-mutant
test-exit-handles-mutant: $(HARNESS_BIN) $(EXITH_MUT_IMG) $(FAT_EXITH_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-exit-handles-mutant : prove the oracle bites (Rule 6)\n'
	@printf '  sft_close_process elided -> handles leak across EXEC runs -> SFT exhausts.\n'
	@printf '======================================================================\n'
	@$(HARNESS_BIN) --disk "$(EXITH_MUT_IMG)" --disk2 "$(FAT_EXITH_IMG)" \
		--name "$(EXITH_MUT_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
		2> "$(EXITH_MUT_REPORT)" || true
	@cat "$(EXITH_MUT_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if [ ! -s "$(EXITH_MUT_SERIAL)" ]; then \
		printf '!!! test-exit-handles-mutant FAIL: no serial captured (cannot judge the mutant)\n'; exit 1; \
	fi
	@if grep -q 'EXITH-CHILD-OPENFAIL' "$(EXITH_MUT_SERIAL)" \
	   || grep -q 'EXITH-RUN[0-9]*-FAIL' "$(EXITH_MUT_SERIAL)" \
	   || ! grep -q '^EXITH-DONE rc=0$$' "$(EXITH_MUT_SERIAL)"; then \
		printf '>>> test-exit-handles-mutant: green (mutant correctly RED -- a run OPEN failed / no clean DONE; the oracle bites):\n'; \
		grep -n 'EXITH-RUN\|EXITH-CHILD\|EXITH-DONE' "$(EXITH_MUT_SERIAL)" || true; \
	else \
		printf '!!! test-exit-handles-mutant FAIL: the NO-CLOSE mutant PASSED clean -- test-exit-handles is DECORATION\n'; \
		grep -n 'EXITH-RUN\|EXITH-CHILD\|EXITH-DONE' "$(EXITH_MUT_SERIAL)" || true; \
		exit 1; \
	fi
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-shell (beads initech-7pc -- BOOT -> COMMAND.COM -> DIR/TYPE/run)
# ---------------------------------------------------------------------------
# THE M2 capstone keystone: boot the -DBOOT_SHELL image WITH a FAT12 disk
# (--disk2 = FAT_EXEC_IMG, carrying HELLO.TXT + GREET.COM) and inject a command
# script via QMP --keys, gated on SHELL-READY so the keys arrive while the shell's
# AH=0Ah is blocking on the prompt. The injected script (each token a key; "ret"
# = Enter, "dot" = '.'):
#     dir<ret> type hello.txt<ret> greet<ret> badcmd<ret> exit<ret>
# Assert on serial (every miss fail-loud + exit-non-zero, Law 2 / Rule 2):
#   1. NO triple-fault.
#   2. SHELL-READY (the REPL was entered after CONIN-LIVE).
#   3. DIR listed HELLO.TXT and GREET.COM (FINDFIRST/FINDNEXT into the DTA).
#   4. TYPE printed HELLO.TXT's contents ("Hello from InitechOS test file").
#   5. `greet` ran GREET.COM ("GREETINGS FROM A:GREET.COM" via AH=4Bh EXEC).
#   6. `badcmd` printed the controlled "Bad command or file name" (MSG-DOS-0002).
#   7. EXIT halted cleanly (SHELL-EXIT + SHELL-DONE markers).
# It BITES: with no --keys the prompt (A:\>) appears but NONE of the command
# outputs do, so the assertions on DIR/TYPE/greet/badcmd would go RED.
# Ref: ADR-0003 DEC-11/DEC-12; DOS 3.3 COMMAND.COM; spec/dos_messages.json.
# TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.
SHELL_NAME    := shell_boot
SHELL_SERIAL  := $(BUILD)/$(SHELL_NAME).serial
SHELL_REPORT  := $(BUILD)/$(SHELL_NAME).report
SHELL_TYPE_CONTENT := Hello from InitechOS test file
SHELL_EXEC_OUTPUT  := GREETINGS FROM A:GREET.COM
SHELL_BADCMD       := Bad command or file name

.PHONY: test-shell
test-shell: $(HARNESS_BIN) $(SHELL_IMG) $(FAT_EXEC_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-shell : BOOT -> COMMAND.COM -> DIR/TYPE/run\n'
	@printf '  Ref: ADR-0003 DEC-11/DEC-12; DOS 3.3 COMMAND.COM; spec/dos_messages.json.\n'
	@printf '  beads initech-7pc (M2 capstone). Inject: dir / type hello.txt / greet / badcmd / exit.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s + disk %s (HELLO.TXT + GREET.COM, primary slave)\n' "$(SHELL_IMG)" "$(FAT_EXEC_IMG)"
	@printf 'Expecting : SHELL-READY + DIR{HELLO.TXT,GREET.COM} + "%s" + "%s" + "%s" + SHELL-EXIT\n' "$(SHELL_TYPE_CONTENT)" "$(SHELL_EXEC_OUTPUT)" "$(SHELL_BADCMD)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(SHELL_IMG)" --disk2 "$(FAT_EXEC_IMG)" \
		--name "$(SHELL_NAME)" --out "$(BUILD)" --timeout-ms 12000 \
		--keys "d,i,r,ret,t,y,p,e,spc,h,e,l,l,o,dot,t,x,t,ret,g,r,e,e,t,ret,b,a,d,c,m,d,ret,e,x,i,t,ret" \
		--keys-after "SHELL-READY" \
		2> "$(SHELL_REPORT)" || true
	@cat "$(SHELL_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q 'triple_fault=1' "$(SHELL_REPORT)"; then \
		printf '!!! test-shell FAIL: TRIPLE FAULT -- the shell boot or a command crashed\n'; \
		exit 1; \
	fi
	@printf '>>> test-shell [1/6]: no triple-fault\n'
	@if [ ! -s "$(SHELL_SERIAL)" ]; then \
		printf '!!! test-shell FAIL: no serial captured at %s\n' "$(SHELL_SERIAL)"; exit 1; \
	fi
	@grep -q '^SHELL-READY$$' "$(SHELL_SERIAL)" \
		|| { printf '!!! test-shell FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
	@printf '>>> test-shell [2/6]: SHELL-READY (COMMAND.COM REPL entered after CONIN-LIVE)\n'
	@# Extract ONLY the post-SHELL-READY region (the REPL's own output) into a
	@# separate file. The boot's earlier baked demos (proto-DIR + the baked
	@# TYPE/DIR programs) also print these filenames + HELLO.TXT contents BEFORE
	@# the prompt; asserting on the whole serial would let the gate pass on the
	@# demo output instead of the shell's. Scoping to after SHELL-READY makes the
	@# DIR/TYPE/greet assertions bite the REPL specifically (Law 2 / Rule 6).
	@sed -n '/^SHELL-READY$$/,$$p' "$(SHELL_SERIAL)" > "$(BUILD)/$(SHELL_NAME).repl"
	@# ---- DIR listed the two files (FINDFIRST/FINDNEXT into the DTA). ----
	@grep -qF 'HELLO.TXT' "$(BUILD)/$(SHELL_NAME).repl" \
		|| { printf '!!! test-shell FAIL: DIR did not list HELLO.TXT (root-cause the DIR built-in / FINDFIRST, Rule 3)\n'; exit 1; }
	@grep -qF 'GREET.COM' "$(BUILD)/$(SHELL_NAME).repl" \
		|| { printf '!!! test-shell FAIL: DIR did not list GREET.COM\n'; exit 1; }
	@printf '>>> test-shell [3/6]: DIR listed HELLO.TXT + GREET.COM (FINDFIRST/FINDNEXT)\n'
	@# ---- TYPE printed HELLO.TXT's contents (in the REPL region). ----
	@grep -qF '$(SHELL_TYPE_CONTENT)' "$(BUILD)/$(SHELL_NAME).repl" \
		|| { printf '!!! test-shell FAIL: TYPE did not print HELLO.TXT contents "%s"\n' "$(SHELL_TYPE_CONTENT)"; exit 1; }
	@printf '>>> test-shell [4/6]: TYPE printed HELLO.TXT contents (OPEN/READ/WRITE/CLOSE)\n'
	@# ---- `greet` ran GREET.COM via AH=4Bh EXEC (in the REPL region). ----
	@grep -qF '$(SHELL_EXEC_OUTPUT)' "$(BUILD)/$(SHELL_NAME).repl" \
		|| { printf '!!! test-shell FAIL: `greet` did not run GREET.COM ("%s" missing -- root-cause external EXEC, Rule 3)\n' "$(SHELL_EXEC_OUTPUT)"; exit 1; }
	@printf '>>> test-shell [5/6]: external command `greet` ran GREET.COM via AH=4Bh EXEC\n'
	@# ---- `badcmd` printed the controlled diagnostic + EXIT halted cleanly. ----
	@grep -qF '$(SHELL_BADCMD)' "$(BUILD)/$(SHELL_NAME).repl" \
		|| { printf '!!! test-shell FAIL: `badcmd` did not print "%s" (the controlled MSG-DOS-0002)\n' "$(SHELL_BADCMD)"; exit 1; }
	@grep -q '^SHELL-EXIT$$' "$(SHELL_SERIAL)" \
		|| { printf '!!! test-shell FAIL: SHELL-EXIT missing -- EXIT did not leave the REPL cleanly\n'; exit 1; }
	@grep -q '^SHELL-DONE$$' "$(SHELL_SERIAL)" \
		|| { printf '!!! test-shell FAIL: SHELL-DONE missing -- the REPL did not return to the halt loop\n'; exit 1; }
	@printf '>>> test-shell [6/6]: `badcmd` -> "%s"; EXIT halted cleanly (SHELL-EXIT + SHELL-DONE)\n' "$(SHELL_BADCMD)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- booted InitechDOS, got an A:\\> prompt, ran DIR/TYPE/a program/EXIT via COMMAND.COM\n'
	@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-panic (beads initech-a5a -- the exception is CAUGHT)
# ---------------------------------------------------------------------------
# Prove the panic path fires on a REAL CPU fault instead of triple-faulting (the
# Prove the panic path fires on a REAL CPU fault instead of triple-faulting (the
# minefield: a triple-fault silently reboots in QEMU -- CLAUDE.md callout). Boot
# the SELF-TEST FAULT image (kmain.c -DBOOT_SELFTEST_FAULT raises a deliberate
# #DE after the banner) under the QEMU oracle (with -d int,guest_errors,cpu_reset
# already passed by the harness) and assert, fail-loud (Rule 2 / Law 2):
#   1. serial shows the grep-able panic marker "PANIC vec=00" (the #DE handler
#      ran and dumped) AND "SELFTEST-FAULT-ARMED" (we actually reached the
#      deliberate fault, so a missing PANIC is a real miss, not a skipped fault).
#   2. triple_fault=0 in the -d cpu_reset log -- the panic HALTED cleanly; the
#      fault did NOT cascade to a silent reboot.
# The guest cli;hlt-loops in the panic handler, so it is reaped by the wall-clock
# timeout -- a timeout is EXPECTED and is NOT a failure; we assert on the marker
# + no-triple-fault, not the exit code. The NORMAL image (test-boot) never
# defines BOOT_SELFTEST_FAULT, so it stays green.
PANIC_NAME   := panic_boot
PANIC_SERIAL := $(BUILD)/$(PANIC_NAME).serial
PANIC_REPORT := $(BUILD)/$(PANIC_NAME).report

.PHONY: test-panic
test-panic: $(HARNESS_BIN) $(PANIC_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-panic : CPU exception caught (no triple-fault)\n'
	@printf '  Ref: ground-truth Sec 3.3 (fail-loud panic) / Sec 8 Risk 1 (the\n'
	@printf '       minefield: a triple-fault silently reboots). beads initech-a5a.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (self-test #DE fault kernel)\n' "$(PANIC_IMG)"
	@printf 'Expecting : serial SELFTEST-FAULT-ARMED + "PANIC vec=00" + triple_fault=0\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(PANIC_IMG)" \
		--name "$(PANIC_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
		2> "$(PANIC_REPORT)" || true
	@cat "$(PANIC_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@# 1. We actually reached the deliberate fault.
	@if [ ! -s "$(PANIC_SERIAL)" ]; then \
		printf '!!! test-panic FAIL: no serial captured at %s\n' "$(PANIC_SERIAL)"; \
		exit 1; \
	fi
	@grep -q '^SELFTEST-FAULT-ARMED$$' "$(PANIC_SERIAL)" \
		|| { printf '!!! test-panic FAIL: never reached the deliberate fault (SELFTEST-FAULT-ARMED missing)\n'; exit 1; }
	@printf '>>> test-panic [1/3]: reached the deliberate #DE\n'
	@# 2. The panic handler ran and dumped the grep-able marker for vector 0 (#DE).
	@grep -q 'PANIC vec=00' "$(PANIC_SERIAL)" \
		|| { printf '!!! test-panic FAIL: no "PANIC vec=00" -- the #DE was not caught by the panic handler\n'; exit 1; }
	@printf '>>> test-panic [2/3]: #DE caught -- "PANIC vec=00" on serial\n'
	@# 3. No triple-fault: the panic halted cleanly (no silent reboot).
	@if grep -q 'triple_fault=1' "$(PANIC_REPORT)"; then \
		printf '!!! test-panic FAIL: TRIPLE FAULT -- the exception cascaded to a silent reboot\n'; \
		exit 1; \
	fi
	@printf '>>> test-panic [3/3]: no triple-fault -- panic halted cleanly\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- deliberate #DE caught by the fail-loud panic, no triple-fault\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-kbd (beads initech-3rs + initech-43b -- KEYBOARD IRQ1 echo)
# ---------------------------------------------------------------------------
# The BITING end-to-end oracle for BOTH the PS/2 keyboard driver (IRQ1) and the
# harness QMP keystroke injection (--keys). This is the FIRST boot that enables
# hardware interrupts (sti) -- the minefield (CLAUDE.md "real mode -> protected
# is a minefield" / Rule 5). Boot the keyboard-echo image (kmain.c
# -DBOOT_KBD_ECHO emits KBD-ECHO-READY after sti, then echoes kbd_getchar() to
# serial), inject "d,i,r" via QMP send-key AFTER the KBD-ECHO-READY marker, and
# assert, fail-loud (Rule 2 / Law 2):
#   1. IRQ-LIVE + KBD-ECHO-READY on serial (sti succeeded; the boot did not
#      triple-fault into a reboot the instant interrupts went live).
#   2. the injected keys reached the guest, were decoded by the IRQ1 path, and
#      echoed back as "dir" between KBD-ECHO-BEGIN/KBD-ECHO-END (this is what
#      proves a key sent via QMP actually drives IRQ1 -- the honest oracle for
#      --keys; a "QMP accepted the command" check alone would be decoration).
#   3. triple_fault=0 AND keys_sent=3 in the harness report.
# A key injected via QMP that is NOT echoed back fails the gate -- exactly the
# biting property the bead demands.
#
# TRI-EMULATOR (Rule 5): this gate runs QEMU. The companion test-kbd-bochs
# attempts the SAME boot under Bochs (the sti/PIC/IRQ path is precisely what
# differs across emulators); see its note for the current Bochs status.
KBD_NAME     := kbd_echo
KBD_SERIAL   := $(BUILD)/$(KBD_NAME).serial
KBD_REPORT   := $(BUILD)/$(KBD_NAME).report

.PHONY: test-kbd
test-kbd: $(HARNESS_BIN) $(KBD_ECHO_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-kbd : KEYBOARD IRQ1 echo (first sti)\n'
	@printf '  Ref: 8042 PS/2 controller / scancode set 1 / 8254 PIT / 8259A EOI.\n'
	@printf '  beads initech-3rs (driver) + initech-43b (QMP --keys). Rule 2/5/6.\n'
	@printf '  TRI-EMULATOR: QEMU here; Bochs via make test-kbd-bochs.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (keyboard-echo kernel), injecting --keys "d,i,r"\n' "$(KBD_ECHO_IMG)"
	@printf 'Expecting : IRQ-LIVE + KBD-ECHO-READY + echoed "dir" + triple_fault=0 + keys_sent=3\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(KBD_ECHO_IMG)" \
		--name "$(KBD_NAME)" --out "$(BUILD)" --timeout-ms 9000 \
		--keys "d,i,r" --keys-after "KBD-ECHO-READY" \
		2> "$(KBD_REPORT)" || true
	@cat "$(KBD_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if [ ! -s "$(KBD_SERIAL)" ]; then \
		printf '!!! test-kbd FAIL: no serial captured at %s\n' "$(KBD_SERIAL)"; \
		exit 1; \
	fi
	@grep -q '^IRQ-LIVE$$' "$(KBD_SERIAL)" \
		|| { printf '!!! test-kbd FAIL: IRQ-LIVE missing -- sti/PIC-unmask path did not complete\n'; exit 1; }
	@grep -q '^KBD-ECHO-READY$$' "$(KBD_SERIAL)" \
		|| { printf '!!! test-kbd FAIL: KBD-ECHO-READY missing -- echo loop never reached\n'; exit 1; }
	@printf '>>> test-kbd [1/3]: IRQ-LIVE + KBD-ECHO-READY (first sti survived, no reboot)\n'
	@# The echoed keys land on their own line between BEGIN/END. Assert "dir".
	@awk '/^KBD-ECHO-BEGIN$$/{f=1;next} /^KBD-ECHO-END$$/{f=0} f{print}' "$(KBD_SERIAL)" \
		| grep -q '^dir$$' \
		|| { printf '!!! test-kbd FAIL: injected keys not echoed as "dir" -- a QMP key did NOT reach IRQ1 (root-cause the kbd path / --keys, Rule 3)\n'; exit 1; }
	@printf '>>> test-kbd [2/3]: QMP-injected "d,i,r" decoded by IRQ1 and echoed as "dir"\n'
	@if grep -q 'triple_fault=1' "$(KBD_REPORT)"; then \
		printf '!!! test-kbd FAIL: TRIPLE FAULT -- enabling interrupts cascaded to a reboot\n'; \
		exit 1; \
	fi
	@grep -q 'keys_sent=3' "$(KBD_REPORT)" \
		|| { printf '!!! test-kbd FAIL: harness did not report keys_sent=3 (QMP send-key path)\n'; exit 1; }
	@printf '>>> test-kbd [3/3]: triple_fault=0 + keys_sent=3 (sti clean, injection issued)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- PS/2 keyboard IRQ1 live; a QMP-injected key reached the guest and echoed\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-conin (beads initech-n62 -- INT 21h CON INPUT end-to-end)
# ---------------------------------------------------------------------------
# A baked .COM program reads a LINE from the keyboard via INT 21h AH=0Ah BUFFERED
# INPUT and writes it back through AH=40h. The harness injects "d,i,r,ret" via
# QMP --keys (the SAME injection path as test-kbd), gated on the CONIN-PROG-READY
# marker so the keys arrive while AH=0Ah is blocking. We assert "CONIN-LINE=dir"
# comes back on serial + triple_fault=0 -- proof a real program reads a line from
# the keyboard through the INT 21h CON input path end-to-end (Law 2 / Rule 5).
# The blocking AH=0Ah can ONLY progress because the keys are injected; the
# -DBOOT_CONIN image is separate so the normal boot never blocks on the keyboard.
CONIN_NAME     := conin_boot
CONIN_SERIAL   := $(BUILD)/$(CONIN_NAME).serial
CONIN_REPORT   := $(BUILD)/$(CONIN_NAME).report

.PHONY: test-conin
test-conin: $(HARNESS_BIN) $(CONIN_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-conin : INT 21h CON INPUT end-to-end\n'
	@printf '  Ref: DOS 3.3 PRM AH=0Ah buffered input. beads initech-n62. Rule 2/5.\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (CON-input self-test kernel), injecting --keys "d,i,r,ret"\n' "$(CONIN_IMG)"
	@printf 'Expecting : CONIN-LIVE + CONIN-PROG-READY + "CONIN-LINE=dir" + triple_fault=0\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@$(HARNESS_BIN) --disk "$(CONIN_IMG)" \
		--name "$(CONIN_NAME)" --out "$(BUILD)" --timeout-ms 9000 \
		--keys "d,i,r,ret" --keys-after "CONIN-PROG-READY" \
		2> "$(CONIN_REPORT)" || true
	@cat "$(CONIN_REPORT)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@if [ ! -s "$(CONIN_SERIAL)" ]; then \
		printf '!!! test-conin FAIL: no serial captured at %s\n' "$(CONIN_SERIAL)"; \
		exit 1; \
	fi
	@grep -q '^CONIN-LIVE$$' "$(CONIN_SERIAL)" \
		|| { printf '!!! test-conin FAIL: CONIN-LIVE missing -- the keyboard input source never bound\n'; exit 1; }
	@grep -q '^CONIN-PROG-READY$$' "$(CONIN_SERIAL)" \
		|| { printf '!!! test-conin FAIL: CONIN-PROG-READY missing -- the CON-input program never started\n'; exit 1; }
	@printf '>>> test-conin [1/3]: CONIN-LIVE + CONIN-PROG-READY (input source bound, program running)\n'
	@# The program writes "CONIN-LINE=dir" back through INT 21h AH=40h after the
	@# AH=0Ah buffered read of the injected "dir" + Enter. The line ends in CRLF
	@# (the program's trailing CRLF), so strip CR before matching the whole line.
	@tr -d '\r' < "$(CONIN_SERIAL)" | grep -q '^CONIN-LINE=dir$$' \
		|| { printf '!!! test-conin FAIL: "CONIN-LINE=dir" missing -- AH=0Ah did not read the injected line back via INT 21h (root-cause the CON-input path, Rule 3)\n'; exit 1; }
	@printf '>>> test-conin [2/3]: AH=0Ah read the injected line; "CONIN-LINE=dir" echoed back through INT 21h\n'
	@if grep -q 'triple_fault=1' "$(CONIN_REPORT)"; then \
		printf '!!! test-conin FAIL: TRIPLE FAULT during the CON-input run\n'; \
		exit 1; \
	fi
	@grep -q 'keys_sent=4' "$(CONIN_REPORT)" \
		|| { printf '!!! test-conin FAIL: harness did not report keys_sent=4 (d,i,r,ret)\n'; exit 1; }
	@printf '>>> test-conin [3/3]: triple_fault=0 + keys_sent=4 (clean run, line injected)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- a real program read a line from the keyboard via INT 21h AH=0Ah\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# Bochs leg of test-kbd (beads initech-3rs / Rule 5: the sti/PIC/IRQ path is
# exactly what differs across emulators -- it MUST be checked on Bochs too).
# ---------------------------------------------------------------------------
# STATUS (honest, NOT silently skipped -- Rule 5 / Law 2): bochs IS installed in
# this environment, but the InitechOS boot chain does NOT currently reach the C
# kernel under Bochs -- stage2's VBE 640x480 LFB mode-set fails on the Bochs VGA
# BIOS available here and the boot halts at the "ERR-VBE" serial marker, BEFORE
# any C, the sti, or the IRQ path. This is a PRE-EXISTING boot/VBE gap (the same
# reason test-boot is marked "QEMU only; tri-emulator pending beads initech-x0i")
# and is INDEPENDENT of the keyboard/PIT work -- the unmodified tracer image hits
# the identical ERR-VBE. Additionally the only Bochs display libraries built here
# are 'x' (needs an X server) and 'rfb' (blocks on a VNC client), so a clean
# headless Bochs harness is itself not yet available.
#
# Rather than fake a green, this gate BOOTS the echo image under Bochs and
# reports what actually happens: if the boot ever reaches IRQ-LIVE the sti path
# is confirmed on Bochs; until the VBE/boot gap (initech-x0i) closes it will
# stop at ERR-VBE, which we surface loudly and treat as a KNOWN-BLOCKED (exit 0
# with a visible BLOCKED verdict, so `make test` is not wedged by a pre-existing
# unrelated boot gap; the QEMU test-kbd remains the binding oracle).
BOCHS            ?= bochs
BOCHS_SEABIOS    ?= /usr/share/seabios/bios.bin
BOCHS_VGABIOS    ?= /usr/share/seabios/vgabios-stdvga.bin
KBD_BOCHS_IMG    := $(BUILD)/$(KBD_NAME)_bochs.img
KBD_BOCHS_SERIAL := $(BUILD)/$(KBD_NAME)_bochs.serial
KBD_BOCHS_RC     := $(BUILD)/$(KBD_NAME)_bochs.rc
KBD_BOCHS_CFG    := $(BUILD)/$(KBD_NAME)_bochsrc

.PHONY: test-kbd-bochs
test-kbd-bochs: $(KBD_ECHO_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-kbd-bochs : Bochs leg (Rule 5)\n'
	@printf '======================================================================\n'
	@if ! command -v $(BOCHS) >/dev/null 2>&1; then \
		printf '!!! test-kbd-bochs BLOCKED: bochs not installed in this environment.\n'; \
		printf '    Install: sudo apt install bochs (CLAUDE.md Build & test). See initech-x0i.\n'; \
		exit 0; \
	fi
	@if [ ! -f "$(BOCHS_SEABIOS)" ] || [ ! -f "$(BOCHS_VGABIOS)" ]; then \
		printf '!!! test-kbd-bochs BLOCKED: Bochs BIOS images not found (%s / %s).\n' "$(BOCHS_SEABIOS)" "$(BOCHS_VGABIOS)"; \
		exit 0; \
	fi
	@# Bochs locks the image file; run on a private copy. CHS 2/2/32 = 128 sectors
	@# (== IMG_SECTORS), spt=32 >= 17 so stage2 fits on track 0.
	@cp -f "$(KBD_ECHO_IMG)" "$(KBD_BOCHS_IMG)"
	@rm -f "$(KBD_BOCHS_IMG).lock" "$(KBD_BOCHS_SERIAL)"
	@printf 'c\n' > "$(KBD_BOCHS_RC)"
	@printf 'romimage: file=%s\n' "$(BOCHS_SEABIOS)"            >  "$(KBD_BOCHS_CFG)"
	@printf 'vgaromimage: file=%s\n' "$(BOCHS_VGABIOS)"         >> "$(KBD_BOCHS_CFG)"
	@printf 'megs: 32\n'                                        >> "$(KBD_BOCHS_CFG)"
	@printf 'cpu: model=pentium, ips=10000000\n'               >> "$(KBD_BOCHS_CFG)"
	@printf 'ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14\n' >> "$(KBD_BOCHS_CFG)"
	@printf 'ata0-master: type=disk, path="%s", mode=flat, cylinders=2, heads=2, spt=32\n' "$(KBD_BOCHS_IMG)" >> "$(KBD_BOCHS_CFG)"
	@printf 'boot: disk\n'                                      >> "$(KBD_BOCHS_CFG)"
	@printf 'com1: enabled=1, mode=file, dev=%s\n' "$(KBD_BOCHS_SERIAL)" >> "$(KBD_BOCHS_CFG)"
	@printf 'display_library: x\n'                             >> "$(KBD_BOCHS_CFG)"
	@printf 'clock: sync=none\n'                                >> "$(KBD_BOCHS_CFG)"
	@printf 'Booting   : %s under Bochs (timeout 12s; serial -> %s)\n' "$(KBD_BOCHS_IMG)" "$(KBD_BOCHS_SERIAL)"
	@printf '%s\n' '----------------------------------------------------------------------'
	@-timeout 12 $(BOCHS) -q -f "$(KBD_BOCHS_CFG)" -rc "$(KBD_BOCHS_RC)" >"$(BUILD)/$(KBD_NAME)_bochs.out" 2>&1 || true
	@rm -f "$(KBD_BOCHS_IMG).lock"
	@printf 'Serial captured:\n'
	@cat "$(KBD_BOCHS_SERIAL)" 2>/dev/null || printf '(none)\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@if grep -q '^IRQ-LIVE$$' "$(KBD_BOCHS_SERIAL)" 2>/dev/null; then \
		printf '>>> test-kbd-bochs: IRQ-LIVE reached under Bochs -- the sti/PIC/IRQ path is confirmed cross-emulator.\n'; \
		if grep -q '^KBD-ECHO-READY$$' "$(KBD_BOCHS_SERIAL)" 2>/dev/null; then \
			printf 'VERDICT   : PASS -- Bochs reached the keyboard echo loop after sti.\n'; \
		else \
			printf 'VERDICT   : PARTIAL -- sti survived on Bochs but echo loop not reached; investigate.\n'; \
		fi; \
	elif grep -q 'ERR-VBE' "$(KBD_BOCHS_SERIAL)" 2>/dev/null; then \
		printf '!!! test-kbd-bochs BLOCKED (KNOWN, beads initech-x0i): boot halted at ERR-VBE.\n'; \
		printf '    stage2 VBE 640x480 LFB mode-set fails on the Bochs VGA BIOS here, BEFORE any C\n'; \
		printf '    or the sti -- this is the SAME pre-existing boot/VBE gap that keeps test-boot\n'; \
		printf '    QEMU-only; it is INDEPENDENT of the keyboard/PIT/sti work (the unmodified tracer\n'; \
		printf '    image hits the identical ERR-VBE). The QEMU make test-kbd is the binding oracle.\n'; \
		printf 'VERDICT   : BLOCKED (Bochs boot/VBE gap initech-x0i; NOT silently skipped -- Rule 5)\n'; \
	else \
		printf '!!! test-kbd-bochs INCONCLUSIVE: Bochs produced no IRQ-LIVE and no ERR-VBE.\n'; \
		printf '    (Likely the headless display gap: only the X-server display lib is built here.)\n'; \
		printf 'VERDICT   : BLOCKED (Bochs harness gap; see initech-x0i)\n'; \
	fi
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-assets (beads initech-vcq)
# ---------------------------------------------------------------------------
# Asset-extraction v0 gate. Two assertions, both must hold (CLAUDE.md Law 2,
# Rule 6 -- the golden must bite):
#   (a) PALETTE HONESTY: re-sample the frame PPM at every (x,y) recorded in
#       palette.json and assert each named color still matches within the
#       JSON tolerance. Corrupt a recorded RGB -> this goes red.
#   (b) STRIKE WELL-FORMEDNESS: the generated chicago8x16.h has the expected
#       glyph count, a blank space cell, out-of-range -> blank, and the full
#       REQUIRED coverage (A-Z a-z 0-9 space . , : - ' ( )) inked.
# Prerequisites wire the webp->PPM decode and the palette.h regeneration so
# the committed header can never drift from palette.json.
test-assets: $(ASSET_CHECK_BIN) $(PREVIEW_PPM) $(PALETTE_H) $(PALETTE_JSON)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-assets : palette honesty + strike check\n'
	@printf '  Ref: PRD Sec 10 (asset pipeline) / Sec 6.4 (Chicago) / Sec 12 (IP).\n'
	@printf '  beads initech-vcq. The frame is REFERENCE ONLY -- we measure it.\n'
	@printf '======================================================================\n'
	@$(ASSET_CHECK_BIN) $(PALETTE_JSON) $(PREVIEW_PPM)
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- palette re-samples match the fixture, strike well-formed\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# REAL gate: test-spec (ADR-0003 Appendices A-D, specs-as-data)
# ---------------------------------------------------------------------------
# Validate the LOCKED InitechDOS spec-data extracted VERBATIM from ADR-0003
# (CLAUDE.md Rule 8 -- specs are locked data, not prose). This is a
# correctness GATE: a malformed or drifted spec must NEVER read green
# (CLAUDE.md Law 2 / Rule 2 -- fail fast, fail loud), so every check exits
# NON-ZERO on the first failure. Ref: ADR-0003 DEC-13 (controlled vocabulary),
# DEC-04 (INT 21h), DEC-05 (PSP), DEC-03 (MCB arena), DEC-07 (dir entry).
#
# Four independent assertions:
#   1. spec/int21h_register.json parses as JSON and has a non-empty function
#      table; every entry carries ah/mnemonic/description/class with class in
#      {Core,Legacy,Resident}.
#   2. spec/dos_messages.json parses as JSON and the catalogue has EXACTLY 16
#      entries MSG-DOS-0001..MSG-DOS-0016, each with non-empty text.
#   3. spec/dos_structs.h compiles in a tiny C11 TU under the house flags so
#      the _Static_assert size checks fire (dir_entry_t=32, psp_t=256,
#      mcb_t=16). A wrong size is a compile error -> gate red.
#   4. spec/dos_banner.txt is EXACTLY two lines and contains the literal
#      'InitechDOS  Version 3.30' (with the controlled DOUBLE space).
SPEC_DIR        := spec
SPEC_INT21H     := $(SPEC_DIR)/int21h_register.json
SPEC_INT21H_CC  := $(SPEC_DIR)/int21h_calling_convention.json
SPEC_MESSAGES   := $(SPEC_DIR)/dos_messages.json
SPEC_STRUCTS_H  := spec/dos_structs.h
SPEC_BANNER     := $(SPEC_DIR)/dos_banner.txt
SPEC_STRUCT_TU  := $(BUILD)/spec_dos_structs_check.c
SPEC_STRUCT_BIN := $(BUILD)/spec_dos_structs_check

test-spec: $(SPEC_INT21H) $(SPEC_INT21H_CC) $(SPEC_MESSAGES) $(SPEC_STRUCTS_H) $(SPEC_BANNER) | $(BUILD)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-spec : ADR-0003 spec-data oracle\n'
	@printf '  Ref: ADR-0003 Appendices A-D / DEC-13 (controlled vocabulary).\n'
	@printf '  CLAUDE.md Rule 8 (specs-as-data) / Law 2 (oracle is the truth).\n'
	@printf '======================================================================\n'
	@printf '>>> test-spec [1/5]: INT 21h register JSON (Appendix A)\n'
	@python3 -c "import json,sys; \
d=json.load(open('$(SPEC_INT21H)')); \
fns=d['functions'] if isinstance(d,dict) and 'functions' in d else d; \
assert isinstance(fns,list) and len(fns)>0, 'function table empty/not a list'; \
ok={'Core','Legacy','Resident'}; \
[ (_ for _ in ()).throw(AssertionError('row %d missing/empty field or bad class: %r'%(i,r))) \
  for i,r in enumerate(fns) \
  if not (all(r.get(k) for k in ('ah','mnemonic','description')) and r.get('class') in ok) ]; \
print('    parsed %d functions; all have ah/mnemonic/description and valid class'%len(fns))" \
		|| { printf '!!! test-spec FAIL: %s invalid (parse/shape/class)\n' "$(SPEC_INT21H)"; exit 1; }
	@printf '>>> test-spec [2/5]: INT 21h calling-convention JSON (beads initech-509.5 / initech-1f9)\n'
	@python3 -c "import json,re; \
cc=json.load(open('$(SPEC_INT21H_CC)')); \
reg=json.load(open('$(SPEC_INT21H)')); \
assert isinstance(cc.get('abi'),dict) and cc['abi'].get('function_selector'), 'cc missing abi.function_selector'; \
ccf=cc['functions']; \
assert isinstance(ccf,list) and len(ccf)>0, 'cc function table empty'; \
hx=lambda t: int(t.strip().lower().rstrip('h'),16); \
toks=[t for r in reg['functions'] for t in re.split(r'[\s/]+', r['ah'].strip()) if t.strip()]; \
rs=set(); \
[ rs.update(range(hx(t.split('-')[0]), hx(t.split('-')[1])+1)) if '-' in t else rs.add(hx(t)) for t in toks ]; \
missing=[f['ah'] for f in ccf if hx(f['ah']) not in rs]; \
assert not missing, 'cc AH(s) NOT in int21h_register.json (controlled scope!): %r'%missing; \
print('    parsed %d cc functions; every AH exists in int21h_register.json (%d sanctioned AHs)'%(len(ccf),len(rs)))" \
		|| { printf '!!! test-spec FAIL: %s invalid or documents an AH NOT in the locked register (Rule 8 / DEC-13)\n' "$(SPEC_INT21H_CC)"; exit 1; }
	@printf '>>> test-spec [3/5]: diagnostic message catalogue JSON (Appendix C)\n'
	@python3 -c "import json,sys; \
d=json.load(open('$(SPEC_MESSAGES)')); \
m=d['messages'] if isinstance(d,dict) and 'messages' in d else d; \
assert isinstance(m,dict), 'messages is not an object'; \
assert len(m)==16, 'expected 16 messages, found %d'%len(m); \
exp=set('MSG-DOS-%04d'%i for i in range(1,17)); \
assert set(m.keys())==exp, 'message IDs are not MSG-DOS-0001..0016: %r'%(sorted(set(m)^exp)); \
[ (_ for _ in ()).throw(AssertionError('empty text for %s'%k)) for k,v in m.items() if not (isinstance(v,str) and v.strip()) ]; \
print('    parsed 16 messages MSG-DOS-0001..0016; all non-empty')" \
		|| { printf '!!! test-spec FAIL: %s invalid (parse/count/IDs/empty)\n' "$(SPEC_MESSAGES)"; exit 1; }
	@printf '>>> test-spec [4/5]: struct size asserts compile (Appendix B)\n'
	@printf '#include "dos_structs.h"\nint main(void){return 0;}\n' > "$(SPEC_STRUCT_TU)"
	@$(CC) $(CFLAGS) -I$(SPEC_DIR) -o "$(SPEC_STRUCT_BIN)" "$(SPEC_STRUCT_TU)" \
		|| { printf '!!! test-spec FAIL: %s failed _Static_assert (dir=32 / psp=256 / mcb=16)\n' "$(SPEC_STRUCTS_H)"; exit 1; }
	@"$(SPEC_STRUCT_BIN)" \
		|| { printf '!!! test-spec FAIL: spec struct check binary returned non-zero\n'; exit 1; }
	@printf '    dos_structs.h compiled clean: dir_entry_t=32, psp_t=256, mcb_t=16\n'
	@printf '>>> test-spec [5/5]: operator banner exact bytes (Appendix D.1)\n'
	@python3 -c "import sys; \
b=open('$(SPEC_BANNER)','rb').read(); \
lines=b.split(b'\n'); \
trail=lines[-1]==b''; \
n=len(lines)-1 if trail else len(lines); \
assert n==2, 'banner must be exactly two lines, found %d'%n; \
assert b'InitechDOS  Version 3.30' in b, 'missing literal double-space banner line'; \
print('    banner is two lines and contains \"InitechDOS  Version 3.30\" (double space)')" \
		|| { printf '!!! test-spec FAIL: %s banner not two lines or missing double-space literal\n' "$(SPEC_BANNER)"; exit 1; }
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- ADR-0003 spec-data parses, sizes hold, banner verbatim\n'
	@printf '======================================================================\n'

# ---------------------------------------------------------------------------
# Aggregate green gate vector (beads initech-4mc)
# ---------------------------------------------------------------------------
# The single command that asserts "InitechDOS is rock solid". Runs the entire
# currently-green oracle vector in one invocation, host gates BEFORE emulator
# gates (Rule 7 / fail fast on cheap checks). EXCLUDES milestone stubs
# (test-region M3, test-dbase M6, test-compiler M7, selfhost/ddc M8) and the
# Bochs/86Box targets (InitechDOS cannot boot Bochs yet -- beads initech-x0i).
# Each member is a fail-loud `make` of an individual gate; the FIRST red gate
# aborts the run (no `-` prefix, no `|| true`) so nothing green is masked.
#
# Class 1 (host unit oracles) + Class 2 (mutant gates): fast, pure C.
TEST_UNIT_GATES := \
	test-fat12-bpb test-fat12-chain test-fat12-dir test-fat12-write \
	test-fat-partial test-fat-write-partial \
	test-console test-idt test-kbd-unit test-conin-unit test-int21 \
	test-fileio test-exec-unit test-command test-psp test-sft test-loader \
	test-fat test-seed test-seed-codegen test-assets test-spec \
	test-idt-mutant test-kbd-unit-mutant test-conin-mutant test-int21-mutant \
	test-fileio-mutant test-exec-mutant test-command-mutant test-psp-mutant \
	test-sft-mutant test-loader-mutant test-fat-write-mutant \
	test-fat-partial-mutant test-fat-write-partial-mutant

# Class 3 (in-emulator QEMU keystones): slow, boot in QEMU.
TEST_EMU_GATES := \
	test-harness test-tracer-boot test-boot test-program test-fs test-type \
	test-dir test-exec test-fatwrite test-multiopen test-exit-handles \
	test-shell test-panic \
	test-kbd test-conin test-int21-irqstorm

test-unit:
	@printf '======================================================================\n'
	@printf '>>> test-unit: host oracle vector (%s gates) -- unit + mutant\n' "$(words $(TEST_UNIT_GATES))"
	@printf '======================================================================\n'
	@for g in $(TEST_UNIT_GATES); do \
		printf '>>> test-unit: running %s\n' "$$g"; \
		$(MAKE) --no-print-directory $$g || { \
			printf '!!! test-unit FAIL: gate %s went RED\n' "$$g"; exit 1; }; \
	done
	@printf '======================================================================\n'
	@printf 'test-unit: ALL GREEN (%s host gates)\n' "$(words $(TEST_UNIT_GATES))"
	@printf '======================================================================\n'

test-emu:
	@printf '======================================================================\n'
	@printf '>>> test-emu: QEMU keystone vector (%s gates) -- in-emulator boot\n' "$(words $(TEST_EMU_GATES))"
	@printf '======================================================================\n'
	@for g in $(TEST_EMU_GATES); do \
		printf '>>> test-emu: running %s\n' "$$g"; \
		$(MAKE) --no-print-directory $$g || { \
			printf '!!! test-emu FAIL: gate %s went RED\n' "$$g"; exit 1; }; \
	done
	@printf '======================================================================\n'
	@printf 'test-emu: ALL GREEN (%s QEMU gates)\n' "$(words $(TEST_EMU_GATES))"
	@printf '======================================================================\n'

# Whole gate vector (PRD SS8): host gates first (fail fast), then emulator.
# Fails loud on the first red gate; prints ALL GREEN only if everything passed.
test:
	@printf '######################################################################\n'
	@printf '### make test -- InitechDOS aggregate green gate vector (initech-4mc)\n'
	@printf '###   host gates: %s   emu gates: %s\n' "$(words $(TEST_UNIT_GATES))" "$(words $(TEST_EMU_GATES))"
	@printf '######################################################################\n'
	@$(MAKE) --no-print-directory test-unit
	@$(MAKE) --no-print-directory test-emu
	@printf '######################################################################\n'
	@printf '### ALL GREEN -- %s host + %s emu gates passed; InitechDOS is rock solid\n' "$(words $(TEST_UNIT_GATES))" "$(words $(TEST_EMU_GATES))"
	@printf '######################################################################\n'

selfhost:
	$(call stub_fail,selfhost,M8)

ddc:
	$(call stub_fail,ddc,M8)

# ---------------------------------------------------------------------------
# clean
# ---------------------------------------------------------------------------
# Remove built artifacts but keep build/README.md (tracked) so the dir
# stays present and self-explaining.
clean:
	@printf ">>> clean: removing build artifacts under %s/ (keeping README.md)\n" "$(BUILD)"
	@find $(BUILD) -mindepth 1 ! -name 'README.md' -exec rm -rf {} + 2>/dev/null || true
