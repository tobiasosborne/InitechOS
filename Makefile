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
# Total raw image: keep small + deterministic. 64 sectors (32 KiB) is ample.
IMG_SECTORS     := 64

# PPM seafoam checker (factory C tool, tools/).
PPM_CHECK_SRC   := tools/ppm_seafoam_check.c
PPM_CHECK_BIN   := $(BUILD)/ppm_seafoam_check

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
        test-harness test-tracer-boot test-assets test-spec selfhost ddc clean

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
	@printf '  test-dbase     InitechBase differential + round-trip vs real dBASE.\n'
	@printf '  test-compiler  Turbo Initech vs Free Pascal on the shared corpus.\n'
	@printf '  test-seed      Seed front-end unit tests (lexer + parser). REAL: fails non-zero on any check.\n'
	@printf '  test-seed-codegen  Seed codegen end-to-end: compile .pas, boot ELF in QEMU, assert exact serial. REAL.\n'
	@printf '  test-harness   QEMU oracle harness self-test: serial marker caught on good fixture, triple-fault caught on bad. REAL.\n'
	@printf '  test-tracer-boot   Real MBR->stage2->32-bit/flat->VESA LFB boot: assert serial stage markers + seafoam screendump + no triple-fault. REAL.\n'
	@printf '  test-assets    Asset v0: re-sample palette.json anchors vs the frame fixture + validate the Chicago strike header. REAL.\n'
	@printf '  test-spec      InitechDOS spec-data (ADR-0003 Appendices A-D): JSON parse + 16 messages + struct size asserts + banner double-space. REAL.\n'
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
$(TRACER_IMG): $(MBR_BIN) $(STAGE2_BIN) | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@printf ">>> tracer image: %s (MBR@s0, stage2@s1..%d, %d sectors total)\n" \
		"$@" "$(STAGE2_SECTORS)" "$(IMG_SECTORS)"

$(PPM_CHECK_BIN): $(PPM_CHECK_SRC) | $(BUILD)
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

test-fat:
	$(call stub_fail,test-fat,M2)

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
#   3. the QMP screendump of the LIVE guest is seafoam (the framebuffer was
#      actually written with the right color/pitch/bpp).
# The guest hlt-loops to stay live for the screendump, so it is reaped by the
# wall-clock timeout -- a timeout here is EXPECTED and is NOT a failure by
# itself; we assert on markers + screendump + triple_fault, not on exit code.
TRACER_NAME   := tracer_boot
TRACER_SERIAL := $(BUILD)/$(TRACER_NAME).serial
TRACER_PPM    := $(BUILD)/$(TRACER_NAME).ppm
TRACER_REPORT := $(BUILD)/$(TRACER_NAME).report

test-tracer-boot: $(HARNESS_BIN) $(TRACER_IMG) $(PPM_CHECK_BIN)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-tracer-boot : real MBR->LFB boot oracle\n'
	@printf '  Ref: PRD Sec 5 (hardware contract) / Sec 11 (M1). beads initech-f8v.2\n'
	@printf '======================================================================\n'
	@printf 'Booting   : %s (raw disk, custom MBR -> stage2 -> 32-bit flat -> VESA LFB)\n' "$(TRACER_IMG)"
	@printf 'Expecting : serial markers S1/S2/VBE/A20/GDT/PM/LFB/OK + seafoam screendump\n'
	@printf '%s\n' '----------------------------------------------------------------------'
	@# Boot via the disk path with a live-guest screendump. The guest hlt-loops,
	@# so it will time out -- that is expected; we do not gate on the exit code.
	@$(HARNESS_BIN) --disk "$(TRACER_IMG)" --screendump \
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
	@for m in S1 S2 VBE A20 GDT PM LFB OK; do \
		if grep -q "^$$m$$" "$(TRACER_SERIAL)"; then \
			printf '  %-4s : present\n' "$$m"; \
		else \
			printf '  %-4s : MISSING\n' "$$m"; \
		fi; \
	done
	@for m in S1 PM OK; do \
		grep -q "^$$m$$" "$(TRACER_SERIAL)" \
			|| { printf '!!! test-tracer-boot FAIL: required serial marker %s missing\n' "$$m"; exit 1; }; \
	done
	@# 3. Seafoam screendump check.
	@if [ ! -s "$(TRACER_PPM)" ]; then \
		printf '!!! test-tracer-boot FAIL: no screendump captured at %s (live guest required)\n' "$(TRACER_PPM)"; \
		exit 1; \
	fi
	@$(PPM_CHECK_BIN) "$(TRACER_PPM)" \
		|| { printf '!!! test-tracer-boot FAIL: framebuffer is not seafoam\n'; exit 1; }
	@printf '%s\n' '----------------------------------------------------------------------'
	@printf 'VERDICT   : PASS -- real boot chain reached protected mode, filled the LFB seafoam, no triple-fault\n'
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
SPEC_MESSAGES   := $(SPEC_DIR)/dos_messages.json
SPEC_STRUCTS_H  := $(SPEC_DIR)/dos_structs.h
SPEC_BANNER     := $(SPEC_DIR)/dos_banner.txt
SPEC_STRUCT_TU  := $(BUILD)/spec_dos_structs_check.c
SPEC_STRUCT_BIN := $(BUILD)/spec_dos_structs_check

test-spec: $(SPEC_INT21H) $(SPEC_MESSAGES) $(SPEC_STRUCTS_H) $(SPEC_BANNER) | $(BUILD)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-spec : ADR-0003 spec-data oracle\n'
	@printf '  Ref: ADR-0003 Appendices A-D / DEC-13 (controlled vocabulary).\n'
	@printf '  CLAUDE.md Rule 8 (specs-as-data) / Law 2 (oracle is the truth).\n'
	@printf '======================================================================\n'
	@printf '>>> test-spec [1/4]: INT 21h register JSON (Appendix A)\n'
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
	@printf '>>> test-spec [2/4]: diagnostic message catalogue JSON (Appendix C)\n'
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
	@printf '>>> test-spec [3/4]: struct size asserts compile (Appendix B)\n'
	@printf '#include "dos_structs.h"\nint main(void){return 0;}\n' > "$(SPEC_STRUCT_TU)"
	@$(CC) $(CFLAGS) -I$(SPEC_DIR) -o "$(SPEC_STRUCT_BIN)" "$(SPEC_STRUCT_TU)" \
		|| { printf '!!! test-spec FAIL: %s failed _Static_assert (dir=32 / psp=256 / mcb=16)\n' "$(SPEC_STRUCTS_H)"; exit 1; }
	@"$(SPEC_STRUCT_BIN)" \
		|| { printf '!!! test-spec FAIL: spec struct check binary returned non-zero\n'; exit 1; }
	@printf '    dos_structs.h compiled clean: dir_entry_t=32, psp_t=256, mcb_t=16\n'
	@printf '>>> test-spec [4/4]: operator banner exact bytes (Appendix D.1)\n'
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

test:
	$(call stub_fail,test,M0)

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
