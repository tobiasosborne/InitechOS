# ---------------------------------------------------------------------------
# Initech Publications toolchain (bead initech-ipu5.5) -- included from the
# Makefile.  The manuals are DOCUMENTS, not factory code: LuaLaTeX is the
# ratified exception (decision D-11, docs/packaging/MANUALS-AND-BOXES-
# RESEARCH.md Sec 9) and is treated as an external tool like nasm.  The two
# generators the manuals need are C (Law 3): harness/console_text.c (screen
# reproductions from QEMU screendumps) and tools/pubs_msgtable.c (the message
# appendix from spec/dos_messages.json).
#
#   make pubs                 typeset every pubs/manuals/*/{main,cover,spine}.tex
#                             into build/pubs/<manual>/<doc>.pdf
#   make test-pubs-build      HOST GATE: every document builds with zero errors;
#                             overfull boxes <= PUBS_OVERFULL_MAX (0); no
#                             undefined references, missing glyphs or missing
#                             index; main.pdf page count a multiple of 4;
#                             ASCII-only sources (Rule 12); DRAFT count shown.
#                             LOUD FAIL if lualatex/latexmk is missing;
#                             SKIP_PUBS=1 is the one documented, shouting
#                             opt-out (same Make-level ifeq as SKIP_FPC /
#                             SKIP_BOCHS; Law 2, initech-iv8c).
#   make test-pubs-honesty    proves the gate is honest: a missing lualatex is
#                             RED and names SKIP_PUBS=1; SKIP_PUBS=1 shouts.
#   make test-pubs-repro      two clean builds must be byte-identical (Rule 11)
#   make test-pubs-repro-mutant  the same check with a build timestamp
#                             injected into every page must go RED (Rule 6)
#   make pubs-screens         re-capture every manual's screens from the
#                             running OS and write them into the source tree
#                             (a deliberate act: review the diff)
#   make test-pubs-screens    EMU GATE: re-capture into build/ and diff against
#                             the committed captures -- the book must match
#                             the shipped OS output byte for byte
# ---------------------------------------------------------------------------
PUBS_DIR          := pubs
PUBS_BUILD        := $(BUILD)/pubs
PUBS_GEN          := $(PUBS_BUILD)/gen
PUBS_SOURCES      := $(sort $(wildcard $(PUBS_DIR)/*.sh $(PUBS_DIR)/*.mk $(PUBS_DIR)/latexmkrc \
                       $(PUBS_DIR)/class/* $(PUBS_DIR)/common/* $(PUBS_DIR)/screens/*.sh $(PUBS_DIR)/screens/disk/* \
                       $(PUBS_DIR)/manuals/*/*.tex $(PUBS_DIR)/manuals/*/screens/*))
PUBS_MANUALS      := $(patsubst $(PUBS_DIR)/manuals/%/main.tex,%,$(wildcard $(PUBS_DIR)/manuals/*/main.tex))
LUALATEX          ?= lualatex
LATEXMK           ?= latexmk
PUBS_OVERFULL_MAX ?= 0
PUBS_VGABIOS      ?= /usr/share/seabios/vgabios-stdvga.bin
CONSOLE_TEXT_BIN  := $(BUILD)/console_text
PUBS_MSGTABLE_BIN := $(BUILD)/pubs_msgtable
PUBS_MSG_TEX      := $(PUBS_GEN)/dos_messages.tex
PUBS_MSG_DEV_TEX  := $(PUBS_GEN)/dos_messages_device.tex
PUBS_MSG_OTH_TEX  := $(PUBS_GEN)/dos_messages_other.tex
export LATEXMK LUALATEX

$(CONSOLE_TEXT_BIN): harness/console_text.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(PUBS_MSGTABLE_BIN): tools/pubs_msgtable.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(PUBS_MSG_TEX): $(PUBS_MSGTABLE_BIN) spec/dos_messages.json
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_MSGTABLE_BIN) spec/dos_messages.json > $@.tmp && mv -f $@.tmp $@
	@$(PUBS_MSGTABLE_BIN) spec/dos_messages.json device > $(PUBS_MSG_DEV_TEX).tmp && mv -f $(PUBS_MSG_DEV_TEX).tmp $(PUBS_MSG_DEV_TEX)
	@$(PUBS_MSGTABLE_BIN) spec/dos_messages.json other > $(PUBS_MSG_OTH_TEX).tmp && mv -f $(PUBS_MSG_OTH_TEX).tmp $(PUBS_MSG_OTH_TEX)

# Build all documents into $(1) (fresh), optional mode $(2) (mutant).
define pubs-build-into
	@rm -rf $(1)
	@sh $(PUBS_DIR)/build.sh $(1) $(PUBS_GEN) $(2)
endef

.PHONY: pubs test-pubs-build test-pubs-honesty test-pubs-repro test-pubs-repro-mutant \
	pubs-screens test-pubs-screens
pubs: $(PUBS_MSG_TEX)
	@command -v $(LUALATEX) >/dev/null 2>&1 && command -v $(LATEXMK) >/dev/null 2>&1 || { \
		printf '!!! pubs FAIL: %s / %s not found (TeX Live: apt install texlive-luatex latexmk texlive-fonts-extra)\n' "$(LUALATEX)" "$(LATEXMK)"; exit 1; }
	@sh $(PUBS_DIR)/build.sh $(PUBS_BUILD) $(PUBS_GEN)

ifeq ($(SKIP_PUBS),1)
test-pubs-build:
	@printf '!!! test-pubs-build SKIPPED (SKIP_PUBS=1 opt-out) -- the Initech manuals were NOT typeset or linted. This is a LOUD, explicit opt-out, not a pass -- unset SKIP_PUBS and re-run before trusting this gate.\n'
else
test-pubs-build: $(PUBS_MSG_TEX)
	@printf '>>> test-pubs-build: every manual typesets clean (initech-ipu5.5; D-11 LuaLaTeX)\n'
	@for t in $(LUALATEX) $(LATEXMK); do command -v $$t >/dev/null 2>&1 || { \
		printf '!!! test-pubs-build FAIL: %s not found (TeX Live is required to typeset the manuals). A skipped oracle is worse than a red one (Law 2). Set SKIP_PUBS=1 to explicitly (and loudly) opt out.\n' "$$t"; exit 1; }; done
	@bad=$$(LC_ALL=C grep -l -P '[^\x00-\x7F]' $(PUBS_SOURCES) harness/console_text.c tools/pubs_msgtable.c || true); \
	if [ -n "$$bad" ]; then printf '!!! test-pubs-build FAIL: non-ASCII bytes in (Rule 12): %s\n' "$$bad"; exit 1; fi
	$(call pubs-build-into,$(PUBS_BUILD)/gate)
	@set -e; for log in $(PUBS_BUILD)/gate/*/*.log; do \
		doc=$${log%.log}; name=$${doc#$(PUBS_BUILD)/gate/}; \
		over=$$(grep -c '^Overfull \\[hv]box' $$log || true); \
		sh harness/pubs/check_log.sh "$$log" "$(PUBS_OVERFULL_MAX)"; \
		if grep -E -q 'There were undefined references|Reference .* undefined|Label\(s\) may have changed' $$log; then \
			printf '!!! test-pubs-build FAIL: %s: undefined or unsettled references\n' "$$name"; exit 1; fi; \
		if grep -q 'Missing character' $$log; then grep 'Missing character' $$log | head -5; \
			printf '!!! test-pubs-build FAIL: %s: glyphs missing from the fonts\n' "$$name"; exit 1; fi; \
		if grep -q 'No index file' $$log; then \
			printf '!!! test-pubs-build FAIL: %s: the index was not generated\n' "$$name"; exit 1; fi; \
		pages=$$(pdfinfo $$doc.pdf | sed -n 's/^Pages: *//p'); \
		drafts=$$(sed -n 's/^initech-manual: \([0-9]*\) DRAFT markers$$/\1/p' $$log); \
		case $$name in */main) \
			[ $$((pages % 4)) -eq 0 ] || { printf '!!! test-pubs-build FAIL: %s: %s pages is not a multiple of 4 (printer signatures)\n' "$$name" "$$pages"; exit 1; } ;; esac; \
		printf '>>> test-pubs-build: %s.pdf %s pages, %s overfull, %s DRAFT markers\n' "$$name" "$$pages" "$$over" "$${drafts:-0}"; \
	done
	@printf '>>> test-pubs-build: green\n'
endif

# Gate honesty for SKIP_PUBS (mirrors harness/test_gate_honesty.sh, iv8c).
test-pubs-honesty:
	@if env -u SKIP_PUBS $(MAKE) --no-print-directory test-pubs-build LUALATEX=/nonexistent/lualatex > $(BUILD)/pubs-honesty-missing.log 2>&1; then \
		cat $(BUILD)/pubs-honesty-missing.log; printf '!!! test-pubs-honesty FAIL: test-pubs-build accepted a missing lualatex\n'; exit 1; fi
	@grep -F 'test-pubs-build FAIL' $(BUILD)/pubs-honesty-missing.log | grep -qF 'SKIP_PUBS=1' || { \
		cat $(BUILD)/pubs-honesty-missing.log; printf '!!! test-pubs-honesty FAIL: the missing-tool failure does not name SKIP_PUBS=1\n'; exit 1; }
	@$(MAKE) --no-print-directory test-pubs-build SKIP_PUBS=1 LUALATEX=/nonexistent/lualatex > $(BUILD)/pubs-honesty-skip.log 2>&1
	@grep -qF 'test-pubs-build SKIPPED (SKIP_PUBS=1 opt-out)' $(BUILD)/pubs-honesty-skip.log || { \
		cat $(BUILD)/pubs-honesty-skip.log; printf '!!! test-pubs-honesty FAIL: the SKIP_PUBS=1 opt-out is not shouting\n'; exit 1; }
	@printf '>>> test-pubs-honesty: green (missing lualatex RED and names SKIP_PUBS=1; SKIP_PUBS=1 explicitly SKIPPED)\n'

# Two independent clean builds must agree byte for byte (Rule 11).
test-pubs-repro: $(PUBS_MSG_TEX)
	$(call pubs-build-into,$(PUBS_BUILD)/repro-a)
	$(call pubs-build-into,$(PUBS_BUILD)/repro-b)
	@set -e; n=0; for a in $(PUBS_BUILD)/repro-a/*/*.pdf; do \
		b=$(PUBS_BUILD)/repro-b/$${a#$(PUBS_BUILD)/repro-a/}; \
		cmp "$$a" "$$b" || { printf '!!! test-pubs-repro FAIL: %s differs between two builds\n' "$${a#$(PUBS_BUILD)/repro-a/}"; exit 1; }; \
		printf '>>> test-pubs-repro: cmp %s == %s (%s bytes, sha256 %s)\n' "$$a" "$$b" "$$(wc -c < $$a)" "$$(sha256sum $$a | cut -c1-16)"; \
		n=$$((n + 1)); done; \
	[ $$n -gt 0 ] || { printf '!!! test-pubs-repro FAIL: no PDFs built\n'; exit 1; }; \
	printf '>>> test-pubs-repro: green (%s PDFs byte-identical across two clean builds)\n' "$$n"

# Rule 6: inject the build time on every page; the cmp above must then bite.
test-pubs-repro-mutant: $(PUBS_MSG_TEX)
	$(call pubs-build-into,$(PUBS_BUILD)/mutant-a,mutant)
	$(call pubs-build-into,$(PUBS_BUILD)/mutant-b,mutant)
	@if cmp -s $(PUBS_BUILD)/mutant-a/initechdos-users-guide/main.pdf $(PUBS_BUILD)/mutant-b/initechdos-users-guide/main.pdf; then \
		printf '!!! test-pubs-repro-mutant FAIL: two timestamped builds are identical -- test-pubs-repro is decoration\n'; exit 1; fi
	@printf '>>> test-pubs-repro-mutant: green (timestamp-injected builds differ: '; \
		cmp $(PUBS_BUILD)/mutant-a/initechdos-users-guide/main.pdf $(PUBS_BUILD)/mutant-b/initechdos-users-guide/main.pdf | tr -d '\n'; printf ')\n'

# Screens: boot the shipped COMMAND.COM image (TRACER_IMG) per screen.  The
# practice diskette (pubs/screens/disk/MANIFEST) carries the shipped external
# utilities in APPS: the SAME binaries flair-data-rules plants in APPS on the
# shipped data volume, staged here under their shipped names (initech-ipu5.7).
PUBS_UTILS_DIR   := $(PUBS_BUILD)/utils
PUBS_UTILS       := CHKDSK.COM=$(CHKDSK_PROG_BIN) ATTRIB.COM=$(ATTRIB_PROG_BIN) LABEL.COM=$(LABEL_PROG_BIN) \
                    XCOPY.COM=$(XCOPY_PROG_BIN) TREE.COM=$(TREE_PROG_BIN) EDLIN.COM=$(EDLIN_PROG_BIN)
PUBS_UTILS_STAMP := $(PUBS_UTILS_DIR)/.staged
$(PUBS_UTILS_STAMP): $(CHKDSK_PROG_BIN) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN) $(EDLIN_PROG_BIN)
	@rm -rf $(PUBS_UTILS_DIR); mkdir -p $(PUBS_UTILS_DIR)
	@for u in $(PUBS_UTILS); do cp $${u#*=} $(PUBS_UTILS_DIR)/$${u%%=*}; done
	@touch $@
PUBS_SCREEN_DEPS := $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_DIR)/screens/capture.sh $(PUBS_DIR)/screens/keys.sh \
                    $(PUBS_UTILS_STAMP)
pubs-screens: $(PUBS_SCREEN_DEPS)
	@set -e; for m in $(PUBS_MANUALS); do \
		d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/SCREENS ] || continue; \
		sh $(PUBS_DIR)/screens/capture.sh $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) \
			$$d/SCREENS $(PUBS_DIR)/screens/disk $(PUBS_BUILD)/screens/$$m $(PUBS_UTILS_DIR); \
		cp -f $(PUBS_BUILD)/screens/$$m/*.txt $$d/; \
		printf '>>> pubs-screens: %s updated -- review `git diff %s`\n' "$$m" "$$d"; done
test-pubs-screens: $(PUBS_SCREEN_DEPS)
	@set -e; n=0; for m in $(PUBS_MANUALS); do \
		d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/SCREENS ] || continue; \
		sh $(PUBS_DIR)/screens/capture.sh $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) \
			$$d/SCREENS $(PUBS_DIR)/screens/disk $(PUBS_BUILD)/screens-check/$$m $(PUBS_UTILS_DIR); \
		for t in $(PUBS_BUILD)/screens-check/$$m/*.txt; do \
			$(PUBS_GATE) same SCREEN-DRIFT $$d/$$(basename $$t) $$t || { diff -u $$d/$$(basename $$t) $$t || true; exit 1; }; \
			n=$$((n + 1)); done; \
		for c in $$d/*.txt; do [ -f $(PUBS_BUILD)/screens-check/$$m/$$(basename $$c) ] || { \
			printf '!!! test-pubs-screens FAIL: %s has no SCREENS entry\n' "$$c"; exit 1; }; done; done; \
	printf '>>> test-pubs-screens: green (%s screens in the manuals == live OS output)\n' "$$n"
	@sh harness/pubs/check_screens.sh $(PUBS_BUILD)/gate

# PUB-REF-0001 GC-01/03/05/06 and PN-01/05 (initech-ipu5.6).
PUBS_GATE := $(BUILD)/pubs_gate
PUBS_INT21 := $(BUILD)/pubs_int21_inventory
PUBS_SURFACE := $(BUILD)/pubs_dos_surface_inventory
PUBS_REPLAY := $(BUILD)/pubs_samir_replay
PUBS_CHECK := $(PUBS_BUILD)/correctness
PUBS_ALLOW := harness/pubs/pilot.allow
PUBS_GATE_INPUTS := $(shell find pubs -type f)

$(PUBS_GATE): harness/pubs/pubs_gate.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(PUBS_INT21): harness/pubs/int21_inventory.c $(KERNEL_INT21_C) $(DOS_MESSAGES_H) | $(BUILD)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -Ispec -Ios/milton -Iseed -Ibuild -Wl,--gc-sections -o $@ $<
$(PUBS_SURFACE): harness/pubs/dos_surface_inventory.c os/milton/batch.c os/milton/batch.h \
		$(KERNEL_CONFIG_SYS_C) os/milton/config_sys.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ios/milton -Iseed -o $@ $<
$(PUBS_REPLAY): harness/pubs/samir_replay.c $(INTERP_LIST_ENG) $(SAMIR_PAL_HOST_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $^

.PHONY: test-pubs test-pubs-commands test-pubs-lint test-pubs-examples test-pubs-mutant pubs-inventory
# Recursive expansion sees FLAIR_FIXTURES, included after this makefile.
# The shipped inventory is NAME|KIND (pubs_gate.c): command.c's dispatch
# table, the INT 21h register, the APPS executables, and the batch
# subcommands / CONFIG.SYS directives the real batch_classify and
# config_sys_parse accept (candidates: the quoted words of their sources).
pubs-inventory: $(PUBS_GATE) $(PUBS_INT21) $(PUBS_SURFACE) $(DOS_MESSAGES_H)
	@mkdir -p $(PUBS_CHECK)
	@$(CC) -E -P -Ispec -Ios/milton -Ibuild os/milton/command.c > $(PUBS_CHECK)/command.i
	@$(PUBS_INT21) > $(PUBS_CHECK)/int21.list
	@grep -oh '"[A-Z][A-Z0-9]*"' os/milton/batch.c $(KERNEL_CONFIG_SYS_C) | tr -d '"' | sort -u | $(PUBS_SURFACE) > $(PUBS_CHECK)/surface.list
	@$(PUBS_GATE) inventory $(PUBS_CHECK)/command.i $(PUBS_CHECK)/int21.list $(PUBS_CHECK)/surface.list $(filter APPS/%,$(FLAIR_FIXTURES)) > $(PUBS_CHECK)/shipped

ifeq ($(SKIP_PUBS),1)
test-pubs test-pubs-mutant:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- publication correctness was NOT checked. This is NOT a pass; unset SKIP_PUBS before trusting this gate.\n'
else
test-pubs: test-pubs-build test-pubs-commands test-pubs-lint test-pubs-examples test-pubs-screens
	@printf '>>> test-pubs: green (command coverage, example replay, source lints, build cap)\n'
# GC-01 over the InitechDOS books together (the Reference documents, the
# User's Guide teaches): their main.cmds lists (initech-syntax.sty, the one
# registration mechanism) against the shipped inventory, both directions,
# and every inline \cmd against the shipped names.  Any other book must
# stay empty until its product has its own shipped inventory (the Initech
# 123 books have theirs: test-pubs-i123 below, bead initech-ipu5.9).
PUBS_DOS_BOOKS := initechdos-reference initechdos-users-guide
test-pubs-commands: pubs-inventory test-pubs-build
	@set -e; for idx in $(PUBS_BUILD)/gate/*/main.cmds $(PUBS_BUILD)/gate/*/main.mentions; do \
		case $$idx in */initechdos-*/*|*/initech123-*/*) continue;; esac; \
		[ -z "$$(grep -v '^#' "$$idx" || true)" ] || { printf '!!! test-pubs RED COMMAND-SURFACE: %s needs its own shipped inventory before it can contain entries\n' "$$idx"; exit 1; }; done
	@$(PUBS_GATE) commands $(PUBS_CHECK)/shipped $(PUBS_ALLOW) $(foreach b,$(PUBS_DOS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.cmds)
	@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(foreach b,$(PUBS_DOS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
	@sh harness/pubs/check_allowlist.sh $(PUBS_GATE) $(PUBS_ALLOW) $(PUBS_CHECK)
test-pubs-lint: $(PUBS_GATE)
	@$(PUBS_GATE) lint pubs/denylist.txt pubs
	@sha256sum -c pubs/fonts.lock > $(BUILD)/pubs-fonts.log
	@printf '>>> test-pubs-fonts: green (all font files match fonts.lock)\n'
test-pubs-examples: test-pubs-build $(PUBS_REPLAY) $(PUBS_GATE)
	@sh harness/pubs/replay.sh $(PUBS_REPLAY) $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK)
test-pubs-mutant: test-pubs $(PUBS_GATE)
	@sh harness/pubs/mutants.sh $(PUBS_GATE) $(PUBS_REPLAY) $(PUBS_CHECK) $(PUBS_BUILD)/gate $(PUBS_ALLOW)
endif

# TeX emits the figure registry, preventing unchecked printed captures.
test-pubs-screens: test-pubs-build $(PUBS_GATE)

.PHONY: test-pubs-families
ifeq ($(SKIP_PUBS),1)
test-pubs-families:
	@printf '!!! test-pubs-families SKIPPED (SKIP_PUBS=1 opt-out) -- family furniture was NOT checked; this is NOT a pass.\n'
else
test-pubs-families:
	@sh harness/pubs/families.sh
endif

# ---------------------------------------------------------------------------
# Initech 123 books (bead initech-ipu5.9; PUB-REF-0001 GC-01/03/04, CP-14,
# CP-15, D-14).  The shipped inventory is OBTAINED by driving the shipped
# application on the host (harness/pubs/i123_face.c: os/i123/app/face_a.c
# over the core and the shipped menu resource, through a recording Toolbox
# mock): every slash path of the menu resource, every @function the formula
# compiler accepts when typed, every key that acts; plus the list of menu
# leaves that answer "Command not implemented".  harness/pubs/i123_gate.c
# grades the books' main.cmds (initech-syntax.sty \RegisterCommand, KINDs
# menu / function / key) against it in both directions, and the state of
# every documented menu path against the build (GATED = works, DRAFT =
# refused).  Printed examples (.i23) are replayed through the same shipped
# application; FLAIR figures are captured from the running system by
# pubs/screens/flair_capture.sh (QEMU, the 123.EXE launch trace of
# spec/flair_i123_traces.mk) and graded pixel for pixel.
#   make pubs-i123-inventory      the inventory + the generated menu trees
#   make test-pubs-i123           GC-01 both directions + state + mentions
#   make test-pubs-i123-examples  GC-03: every printed example replayed
#   make pubs-i123-examples       (re)write the printed .txt (deliberate)
#   make pubs-i123-screens        (re)capture the FLAIR figures (deliberate)
#   test-pubs-screens             also re-captures every FLAIR figure (GC-04)
#   test-pubs-mutant              also runs harness/pubs/i123_mutants.sh
# ---------------------------------------------------------------------------
PUBS_I123_FACE   := $(BUILD)/pubs_i123_face
PUBS_I123_GATE   := $(BUILD)/pubs_i123_gate
PUBS_FLAIR_SHOT  := $(BUILD)/pubs_flair_shot
PUBS_I123_BOOKS  := $(patsubst $(PUBS_DIR)/manuals/%/main.tex,%,$(wildcard $(PUBS_DIR)/manuals/initech123-*/main.tex))
PUBS_I123_TOPS   := Worksheet Range Copy Move File Print Graph Data System Add-In Quit
PUBS_I123_GEN    := $(foreach t,$(PUBS_I123_TOPS),$(PUBS_GEN)/i123-tree-$(t).tex) $(PUBS_GEN)/i123-keymap.tex
PUBS_I123_DATA   := $(PUBS_BUILD)/i123_data.img
PUBS_I123_SAMPLES := $(sort $(wildcard $(PUBS_DIR)/manuals/initech123-*/samples/*.k123))
PUBS_I123_FACE_SRC := harness/pubs/i123_face.c os/i123/app/face_a.c os/apps/tbx/tbx.h spec/toolbox_gate.h \
                    spec/event_model.h $(I123_CORE_SRC) $(I123_CORE_HDR)

$(PUBS_I123_FACE): $(PUBS_I123_FACE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -mfpmath=387 -Ios/apps/tbx $(I123_CORE_INC) -o $@ harness/pubs/i123_face.c $(I123_CORE_SRC)
$(BUILD)/pubs_i123_face_mut_%: $(PUBS_I123_FACE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -mfpmath=387 -D$* -Ios/apps/tbx $(I123_CORE_INC) -o $@ harness/pubs/i123_face.c $(I123_CORE_SRC)
$(PUBS_I123_GATE): harness/pubs/i123_gate.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(PUBS_FLAIR_SHOT): harness/pubs/flair_shot.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lpng
$(PUBS_GEN)/i123-tree-%.tex: $(PUBS_I123_FACE)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_I123_FACE) tree $* > $@.tmp && mv -f $@.tmp $@
# The key map (PK-14 templates, the Quick Reference): one \LotKey line per
# shipped key, in the probe's keyboard order.
$(PUBS_GEN)/i123-keymap.tex: $(PUBS_I123_FACE)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_I123_FACE) inventory | sed -n 's/^\(.*\)|key$$/\\LotKey{\1}/p' > $@.tmp && mv -f $@.tmp $@

.PHONY: pubs-i123-inventory test-pubs-i123 test-pubs-i123-examples pubs-i123-examples pubs-i123-screens
pubs-i123-inventory: $(PUBS_I123_FACE) $(PUBS_I123_GEN)
	@mkdir -p $(PUBS_CHECK)
	@$(PUBS_I123_FACE) inventory > $(PUBS_CHECK)/i123.shipped
	@$(PUBS_I123_FACE) refusals > $(PUBS_CHECK)/i123.refused
	@printf '>>> pubs-i123-inventory: %s menu paths, %s @functions, %s keys shipped; %s menu leaves refuse\n' \
		"$$(grep -c '|menu$$' $(PUBS_CHECK)/i123.shipped)" "$$(grep -c '|function$$' $(PUBS_CHECK)/i123.shipped)" \
		"$$(grep -c '|key$$' $(PUBS_CHECK)/i123.shipped)" "$$(wc -l < $(PUBS_CHECK)/i123.refused)"

# The Reference documents every shipped menu path, @function and key; the
# Quick Reference every @function and key; the other books may only name
# shipped items (or DRAFT ones).
ifeq ($(SKIP_PUBS),1)
test-pubs-i123 test-pubs-i123-examples:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- the Initech 123 books were NOT checked. This is NOT a pass.\n'
else
test-pubs-i123: pubs-i123-inventory $(PUBS_I123_GATE) test-pubs-build
	@$(PUBS_I123_GATE) commands $(PUBS_CHECK)/i123.shipped $(PUBS_CHECK)/i123.refused \
		$(foreach b,$(PUBS_I123_BOOKS),$(if $(filter initech123-reference,$(b)),menu+function+key=,$(if $(filter initech123-quick-reference,$(b)),function+key=,other=))$(PUBS_BUILD)/gate/$(b)/main.cmds)
	@$(PUBS_I123_GATE) mentions $(PUBS_CHECK)/i123.shipped $(foreach b,$(PUBS_I123_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
test-pubs-i123-examples: $(PUBS_I123_FACE) $(PUBS_GATE) test-pubs-build
	@sh harness/pubs/i123_replay.sh check $(PUBS_I123_FACE) $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK)
endif
pubs-i123-examples: $(PUBS_I123_FACE)
	@sh harness/pubs/i123_replay.sh write $(PUBS_I123_FACE) - - $(PUBS_CHECK)

# FLAIR figures: the 500-tick gate image (short traces) and a publications
# image living 6000 ticks (long traces), both the shipped FLAIR desktop
# with only the -D tick budget changed; the data volume is the shipped one
# plus the sample worksheets the samples/*.k123 scripts save when typed
# into the shipped application (i123_face run: the same code as 123.EXE).
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_TEN_TICK_BUDGET=6000,pubs_i123))
PUBS_I123_LONG_IMG := $(BUILD)/flair_tenants_mut_pubs_i123.img
$(PUBS_I123_DATA): $(FLAIR_DATA_IMG) $(PUBS_I123_FACE) $(PUBS_I123_SAMPLES)
	@rm -rf $(PUBS_BUILD)/i123-samples; mkdir -p $(PUBS_BUILD)/i123-samples
	@cp -f $(FLAIR_DATA_IMG) $@.tmp
	@for s in $(PUBS_I123_SAMPLES); do $(PUBS_I123_FACE) run $$s $(PUBS_BUILD)/i123-samples > /dev/null || exit 1; done
	@for f in $(PUBS_BUILD)/i123-samples/*.WK1; do [ -f "$$f" ] || continue; \
		TZ=UTC touch -d @$(FLAIR_DATA_EPOCH) "$$f"; \
		SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -m -i $@.tmp "$$f" ::$$(basename $$f) || exit 1; done
	@mv -f $@.tmp $@
	@printf '>>> pubs: %s (the shipped data volume + %s sample worksheets)\n' "$@" "$$(ls $(PUBS_BUILD)/i123-samples | wc -l)"
PUBS_FLAIR_DEPS := $(HARNESS_BIN) $(FLAIR_I123_FILE_GATE_IMG) $(PUBS_I123_LONG_IMG) $(PUBS_I123_DATA) $(PUBS_I123_FACE) \
                   $(PUBS_FLAIR_SHOT) $(PUBS_DIR)/screens/flair_capture.sh spec/assets/clut.json spec/assets/color_canon.json
pubs-i123-screens: $(PUBS_FLAIR_DEPS)
	@set -e; for m in $(PUBS_I123_BOOKS); do d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/FLAIR ] || continue; \
		sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(FLAIR_I123_FILE_GATE_IMG) $(PUBS_I123_LONG_IMG) $(PUBS_I123_DATA) \
			$(PUBS_I123_FACE) $(PUBS_FLAIR_SHOT) $$d/FLAIR $(PUBS_BUILD)/flair/$$m spec/assets/clut.json spec/assets/color_canon.json; \
		cp -f $(PUBS_BUILD)/flair/$$m/*.png $$d/; printf '>>> pubs-i123-screens: %s updated -- review the figures\n' "$$m"; done
.PHONY: test-pubs-flair-screens
test-pubs-flair-screens: $(PUBS_FLAIR_DEPS) test-pubs-build
	@set -e; n=0; for m in $(PUBS_I123_BOOKS); do d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/FLAIR ] || continue; \
		sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(FLAIR_I123_FILE_GATE_IMG) $(PUBS_I123_LONG_IMG) $(PUBS_I123_DATA) \
			$(PUBS_I123_FACE) $(PUBS_FLAIR_SHOT) $$d/FLAIR $(PUBS_BUILD)/flair-check/$$m spec/assets/clut.json spec/assets/color_canon.json > $(PUBS_BUILD)/flair-check-$$m.log 2>&1 || { tail -5 $(PUBS_BUILD)/flair-check-$$m.log; exit 1; }; \
		for p in $(PUBS_BUILD)/flair-check/$$m/*.png; do $(PUBS_FLAIR_SHOT) same $$d/$$(basename $$p) $$p; n=$$((n + 1)); done; \
		for c in $$d/*.png; do [ -f $(PUBS_BUILD)/flair-check/$$m/$$(basename $$c) ] || { \
			printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s has no FLAIR entry\n' "$$c"; exit 1; }; done; done; \
	printf '>>> test-pubs-flair-screens: green (%s FLAIR figures in the Initech 123 books == the running system, pixel for pixel)\n' "$$n"
test-pubs-screens: test-pubs-flair-screens
test-pubs: test-pubs-i123 test-pubs-i123-examples
pubs: $(PUBS_I123_GEN)
test-pubs-build: $(PUBS_I123_GEN)
test-pubs-repro test-pubs-repro-mutant: $(PUBS_I123_GEN)
test-pubs-mutant: test-pubs-i123-mutant
.PHONY: test-pubs-i123-mutant
test-pubs-i123-mutant: test-pubs-i123 test-pubs-i123-examples test-pubs-flair-screens $(PUBS_I123_GATE) $(PUBS_FLAIR_SHOT) $(BUILD)/pubs_i123_face_mut_I123_MUT_MENU_SPELLING
	@sh harness/pubs/i123_mutants.sh $(PUBS_I123_GATE) $(PUBS_I123_FACE) $(PUBS_GATE) $(PUBS_FLAIR_SHOT) $(PUBS_CHECK) $(PUBS_BUILD)/gate \
		$(BUILD)/pubs_i123_face_mut_I123_MUT_MENU_SPELLING
