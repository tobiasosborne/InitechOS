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
#   make pubs-<manual>        typeset one manual through the same build route
#   make test-pubs-aux        stale class/style auxiliaries must be discarded;
#                             an unchanged build must retain its directory
#   make test-pubs-aux-mutant  disabling that cleanup must go RED (Rule 6)
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
#   make test-pubs-repro-single  the same oracle for the small installation card
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
                       $(PUBS_DIR)/manuals/*/*.tex $(PUBS_DIR)/manuals/*/entries/*/*.tex $(PUBS_DIR)/manuals/*/screens/*))
PUBS_MANUALS      := $(patsubst $(PUBS_DIR)/manuals/%/main.tex,%,$(wildcard $(PUBS_DIR)/manuals/*/main.tex))
PUBS_BOOK_TARGETS := $(addprefix pubs-,$(PUBS_MANUALS))
PUBS_STYLE_SOURCES := $(wildcard $(PUBS_DIR)/*.cls $(PUBS_DIR)/*.sty $(PUBS_DIR)/preamble.tex \
                       $(PUBS_DIR)/class/*.cls $(PUBS_DIR)/class/*.sty $(PUBS_DIR)/common/*.tex)
LUALATEX          ?= lualatex
LATEXMK           ?= latexmk
PUBS_OVERFULL_MAX ?= 0
PUBS_VGABIOS      ?= /usr/share/seabios/vgabios-stdvga.bin
CONSOLE_TEXT_BIN  := $(BUILD)/console_text
PUBS_MSGTABLE_BIN := $(BUILD)/pubs_msgtable
PUBS_MSG_TEX      := $(PUBS_GEN)/dos_messages.tex
PUBS_MSG_DEV_TEX  := $(PUBS_GEN)/dos_messages_device.tex
PUBS_MSG_OTH_TEX  := $(PUBS_GEN)/dos_messages_other.tex
# The TPS Report Generator's message table (bead initech-ipu5.14): generated
# from the locked spec/tps_ritual.json by harness/pubs/tps_inventory.c, so
# the book prints every program message from the shipped data (GC-02).
PUBS_TPS_INV_BIN  := $(BUILD)/pubs_tps_inventory
PUBS_TPS_TEX      := $(PUBS_GEN)/tps_messages.tex
PUBS_TPS_BOOK     := tps-report-generator
PUBS_GENERATED    := $(PUBS_MSG_TEX) $(PUBS_TPS_TEX)
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

$(PUBS_TPS_INV_BIN): harness/pubs/tps_inventory.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(PUBS_TPS_TEX): $(PUBS_TPS_INV_BIN) spec/tps_ritual.json
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_TPS_INV_BIN) tex spec/tps_ritual.json > $@.tmp && mv -f $@.tmp $@
# The InitechBase tables (bead initech-ipu5.8): the Limitations table from
# the shipped engine's capacity constants, the coercion appendix from
# spec/samir/xbase_coercion.json (harness/pubs/samir_tables.c).
PUBS_SAMIR_TABLES   := $(BUILD)/pubs_samir_tables
PUBS_SAMIR_GEN      := $(PUBS_GEN)/samir_limits.tex $(PUBS_GEN)/samir_coercion.tex $(PUBS_GEN)/samir_formats.tex \
                       $(PUBS_GEN)/samir_messages.tex
PUBS_SAMIR_TBL_LINK  = $(filter-out $(SAMIR_FLOW_SRC) $(SAMIR_PROC_SRC) $(SAMIR_REPORT_SRC),$(INTERP_PROC_ENG)) $(SAMIR_PAL_HOST_SRC)
$(PUBS_SAMIR_TABLES): harness/pubs/samir_tables.c $(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) $(SAMIR_HEADERS) spec/samir/dbf_format.h spec/samir/ndx_format.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -Ispec/samir -o $@ harness/pubs/samir_tables.c $(PUBS_SAMIR_TBL_LINK)
$(PUBS_GEN)/samir_limits.tex: $(PUBS_SAMIR_TABLES)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_SAMIR_TABLES) limits > $@.tmp && mv -f $@.tmp $@
$(PUBS_GEN)/samir_messages.tex: $(PUBS_SAMIR_TABLES)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_SAMIR_TABLES) messages > $@.tmp && mv -f $@.tmp $@
$(PUBS_GEN)/samir_formats.tex: $(PUBS_SAMIR_TABLES)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_SAMIR_TABLES) formats > $@.tmp && mv -f $@.tmp $@
$(PUBS_GEN)/samir_coercion.tex: $(PUBS_SAMIR_TABLES) spec/samir/xbase_coercion.json
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_SAMIR_TABLES) coercion spec/samir/xbase_coercion.json > $@.tmp && mv -f $@.tmp $@
PUBS_GENERATED += $(PUBS_SAMIR_GEN)

# F4 (initech-0wox2): one file per manual entry.  Each directory
# pubs/manuals/<book>/entries/<group>/ becomes a generated, sorted \input index
# $(PUBS_GEN)/entries-<book>-<group>.tex (pubs/entry-index.sh); a chapter says
# \input{entries-<book>-<group>}.  Adding an entry = adding one file; the
# directory itself is a prerequisite so adds and removals both regenerate.
PUBS_ENTRY_DIRS  := $(sort $(patsubst %/,%,$(wildcard $(PUBS_DIR)/manuals/*/entries/*/)))
PUBS_ENTRY_IDX   := $(foreach d,$(PUBS_ENTRY_DIRS),$(PUBS_GEN)/entries-$(word 3,$(subst /, ,$(d)))-$(word 5,$(subst /, ,$(d))).tex)
define pubs-entry-index
$$(PUBS_GEN)/entries-$(word 3,$(subst /, ,$(1)))-$(word 5,$(subst /, ,$(1))).tex: $(PUBS_DIR)/entry-index.sh $(1) $$(wildcard $(1)/*.tex)
	@mkdir -p $$(PUBS_GEN)
	@sh $(PUBS_DIR)/entry-index.sh $(1) > $$@.tmp && mv -f $$@.tmp $$@
endef
$(foreach d,$(PUBS_ENTRY_DIRS),$(eval $(call pubs-entry-index,$(d))))
PUBS_GENERATED += $(PUBS_ENTRY_IDX)

.PHONY: test-pubs-entry-index
test-pubs-entry-index:
	+@MAKE="$(MAKE)" sh harness/pubs/entry_index_check.sh
test-pubs-build: test-pubs-entry-index

# Build into $(1) (fresh), optional mode $(2) (mutant) and manual $(3).
define pubs-build-into
	@rm -rf $(1)
	@sh $(PUBS_DIR)/build.sh $(1) $(PUBS_GEN) "$(2)" "$(3)"
endef

# Ref: initech-46u4 (minitoc -> titletoc leaves incompatible aux/maf),
# PUB-REF-0001 Sec 11.2 GC-07, and test-pubs-repro's clean-build route.
# Check timestamps with shell builtins; only inspect latexmk's recorded INPUT
# dependencies when a source is newer. A missing recorder is conservative.
# Clean all stale books before typesetting so a partner's master index is not
# removed after the other volume has already been typeset (build.sh pass 2).
define pubs-clean-stale
	@set -e; for m in $(1); do \
		d="$(PUBS_BUILD)/$$m"; [ -d "$$d" ] || continue; \
		for style in $(abspath $(PUBS_STYLE_SOURCES)); do \
			if [ "$(PUBS_MUT_KEEP_AUX)" != 1 ] && [ "$$style" -nt "$$d/main.aux" ]; then \
				if [ -f "$$d/main.fls" ] && ! grep -qFx "INPUT $$style" "$$d"/*.fls; then continue; fi; \
				rm -rf "$$d"; \
				printf '>>> pubs: cleaned %s (newer %s)\n' "$$m" "$$style"; \
				break; \
			fi; \
		done; \
	done
endef

.PHONY: pubs test-pubs-build test-pubs-honesty test-pubs-repro test-pubs-repro-mutant \
	pubs-screens test-pubs-screens $(PUBS_BOOK_TARGETS) test-pubs-aux test-pubs-aux-mutant \
	test-pubs-repro-single
pubs: $(PUBS_GENERATED)
pubs: $(PUBS_MSG_TEX) $(PUBS_SAMIR_GEN)
	@command -v $(LUALATEX) >/dev/null 2>&1 && command -v $(LATEXMK) >/dev/null 2>&1 || { \
		printf '!!! pubs FAIL: %s / %s not found (TeX Live: apt install texlive-luatex latexmk texlive-fonts-extra)\n' "$(LUALATEX)" "$(LATEXMK)"; exit 1; }
	$(call pubs-clean-stale,$(PUBS_MANUALS))
	@sh $(PUBS_DIR)/build.sh $(PUBS_BUILD) $(PUBS_GEN)

# Ref: initech-46u4's single-document oracle; PUB-REF-0001 GC-07.
$(PUBS_BOOK_TARGETS): $(PUBS_GENERATED) pubs-turbo-gen
	$(call pubs-clean-stale,$(patsubst pubs-%,%,$@))
	@sh $(PUBS_DIR)/build.sh $(PUBS_BUILD) $(PUBS_GEN) '' $(patsubst pubs-%,%,$@)

test-pubs-aux:
	+@MAKE="$(MAKE)" sh harness/pubs/aux_check.sh $(PUBS_BUILD)

# Rule 6: one branch skips the cleanup; the real LuaLaTeX aux reader must bite.
test-pubs-aux-mutant:
	@mkdir -p $(PUBS_BUILD)
	+@if $(MAKE) --no-print-directory -j3 test-pubs-aux PUBS_MUT_KEEP_AUX=1 > $(PUBS_BUILD)/aux-mutant.log 2>&1; then \
		cat $(PUBS_BUILD)/aux-mutant.log; printf '!!! test-pubs-aux-mutant FAIL: stale-aux mutant survived\n'; exit 1; fi
	@grep -F 'Undefined control sequence' $(PUBS_BUILD)/aux-mutant.log && \
		grep -F '!!! test-pubs-aux RED: newer dependency left an incompatible main.aux' $(PUBS_BUILD)/aux-mutant.log || { \
		cat $(PUBS_BUILD)/aux-mutant.log; printf '!!! test-pubs-aux-mutant FAIL: wrong failure\n'; exit 1; }
	@rm -rf $(PUBS_BUILD)/initechword-install
	+@$(MAKE) --no-print-directory -j3 pubs-initechword-install
	@printf '>>> test-pubs-aux-mutant: green (cleanup-disabled branch rejected by the stale-aux oracle)\n'

ifeq ($(SKIP_PUBS),1)
test-pubs-build:
	@printf '!!! test-pubs-build SKIPPED (SKIP_PUBS=1 opt-out) -- the Initech manuals were NOT typeset or linted. This is a LOUD, explicit opt-out, not a pass -- unset SKIP_PUBS and re-run before trusting this gate.\n'
else
test-pubs-build: $(PUBS_GENERATED)
test-pubs-build: $(PUBS_MSG_TEX) $(PUBS_SAMIR_GEN)
	@printf '>>> test-pubs-build: every manual typesets clean (initech-ipu5.5; D-11 LuaLaTeX)\n'
	@for t in $(LUALATEX) $(LATEXMK); do command -v $$t >/dev/null 2>&1 || { \
		printf '!!! test-pubs-build FAIL: %s not found (TeX Live is required to typeset the manuals). A skipped oracle is worse than a red one (Law 2). Set SKIP_PUBS=1 to explicitly (and loudly) opt out.\n' "$$t"; exit 1; }; done
	@bad=$$(LC_ALL=C grep -l -P '[^\x00-\x7F]' $(PUBS_SOURCES) harness/console_text.c tools/pubs_msgtable.c harness/pubs/tps_inventory.c \
		harness/pubs/pubs_gate.c harness/pubs/srclockstep.h harness/pubs/slip_places.sh harness/pubs/slip_mutants.sh spec/pubs_inconsistencies.json || true); \
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
# The single-document route retains the identical cmp oracle (initech-46u4).
test-pubs-repro-single: PUBS_REPRO_MANUAL = initechword-install
test-pubs-repro-single: test-pubs-repro
	@printf '>>> test-pubs-repro-single: green (initechword-install; two clean builds compared byte for byte)\n'
test-pubs-repro: $(PUBS_GENERATED)
test-pubs-repro: $(PUBS_MSG_TEX) $(PUBS_SAMIR_GEN)
	$(call pubs-build-into,$(PUBS_BUILD)/repro-a,,$(PUBS_REPRO_MANUAL))
	$(call pubs-build-into,$(PUBS_BUILD)/repro-b,,$(PUBS_REPRO_MANUAL))
	@set -e; n=0; for a in $(PUBS_BUILD)/repro-a/*/*.pdf; do \
		b=$(PUBS_BUILD)/repro-b/$${a#$(PUBS_BUILD)/repro-a/}; \
		cmp "$$a" "$$b" || { printf '!!! test-pubs-repro FAIL: %s differs between two builds\n' "$${a#$(PUBS_BUILD)/repro-a/}"; exit 1; }; \
		printf '>>> test-pubs-repro: cmp %s == %s (%s bytes, sha256 %s)\n' "$$a" "$$b" "$$(wc -c < $$a)" "$$(sha256sum $$a | cut -c1-16)"; \
		n=$$((n + 1)); done; \
	[ $$n -gt 0 ] || { printf '!!! test-pubs-repro FAIL: no PDFs built\n'; exit 1; }; \
	printf '>>> test-pubs-repro: green (%s PDFs byte-identical across two clean builds)\n' "$$n"

# Rule 6: inject the build time on every page; the cmp above must then bite.
test-pubs-repro-mutant: $(PUBS_GENERATED)
test-pubs-repro-mutant: $(PUBS_MSG_TEX) $(PUBS_SAMIR_GEN)
	$(call pubs-build-into,$(PUBS_BUILD)/mutant-a,mutant)
	$(call pubs-build-into,$(PUBS_BUILD)/mutant-b,mutant)
	@if cmp -s $(PUBS_BUILD)/mutant-a/initechdos-users-guide/main.pdf $(PUBS_BUILD)/mutant-b/initechdos-users-guide/main.pdf; then \
		printf '!!! test-pubs-repro-mutant FAIL: two timestamped builds are identical -- test-pubs-repro is decoration\n'; exit 1; fi
	@printf '>>> test-pubs-repro-mutant: green (timestamp-injected builds differ: '; \
		cmp $(PUBS_BUILD)/mutant-a/initechdos-users-guide/main.pdf $(PUBS_BUILD)/mutant-b/initechdos-users-guide/main.pdf | tr -d '\n'; printf ')\n'

# Screens: boot the shipped COMMAND.COM image (TRACER_IMG) per screen.  The
# practice diskette (pubs/screens/disk/MANIFEST) carries the shipped external
# utilities in APPS: the SAME binaries flair-data-rules plants in DOS on the
# shipped data volume, staged here under their shipped names (initech-ipu5.7).
PUBS_UTILS_DIR   := $(PUBS_BUILD)/utils
PUBS_UTILS       := CHKDSK.COM=$(CHKDSK_PROG_BIN) ATTRIB.COM=$(ATTRIB_PROG_BIN) LABEL.COM=$(LABEL_PROG_BIN) \
                    XCOPY.COM=$(XCOPY_PROG_BIN) TREE.COM=$(TREE_PROG_BIN) EDLIN.COM=$(EDLIN_PROG_BIN)
PUBS_UTILS_STAMP := $(PUBS_UTILS_DIR)/.staged
$(PUBS_UTILS_STAMP): $(CHKDSK_PROG_BIN) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN) $(EDLIN_PROG_BIN)
	@rm -rf $(PUBS_UTILS_DIR); mkdir -p $(PUBS_UTILS_DIR)
	@for u in $(PUBS_UTILS); do cp $${u#*=} $(PUBS_UTILS_DIR)/$${u%%=*}; done
	@touch $@
# The InitechBase screens (bead initech-ipu5.8) boot with SAMIR.EXE + SAMIR.OVL and the
# Sample Programs and Utilities diskette on the second drive: capture.sh's
# `samir` disk, minted from this staging directory.
PUBS_SAMIR_CAPTURE       := $(PUBS_BUILD)/samir-capture
PUBS_SAMIR_CAPTURE_STAMP := $(PUBS_SAMIR_CAPTURE)/.staged
$(PUBS_SAMIR_CAPTURE_STAMP): $(SAMIR_EXE) $(SAMIR_OVL) $(PUBS_BUILD)/samir-disk/.minted
	@rm -rf $(PUBS_SAMIR_CAPTURE); mkdir -p $(PUBS_SAMIR_CAPTURE)
	@cp $(SAMIR_EXE) $(PUBS_SAMIR_CAPTURE)/SAMIR.EXE
	@cp $(basename $(SAMIR_EXE)).OVL $(PUBS_SAMIR_CAPTURE)/SAMIR.OVL
	@for f in $(PUBS_BUILD)/samir-disk/*; do cp $$f $(PUBS_SAMIR_CAPTURE)/; done
	@touch $@
PUBS_SCREEN_DEPS := $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_DIR)/screens/capture.sh $(PUBS_DIR)/screens/keys.sh \
                    $(PUBS_UTILS_STAMP) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(PUBS_SAMIR_CAPTURE_STAMP)
# The journey route (disk field `flair`, bead initech-ipu5.14): the flagship
# image and data volume, left through Special > Exit to InitechDOS by the
# LOCKED trace of test-flair-exit-to-dos leg 3 (spec/flair_exit_dos_traces.mk
# FLAIR_EXIT_DOS_SPEC) up to its W SHELL-READY; its own DIR keys are cut off
# here and the screen's .keys are typed instead.  Since the \DOS folder
# (WL-0144) the flagship volume's AUTOEXEC.BAT skips the date and time
# prompts, so the trace types DIR directly after SHELL-READY; capture.sh
# fails loud if the cut leaves anything after W SHELL-READY.
PUBS_FLAIR_EXIT := $(patsubst %$(comma)kd$(comma)ki$(comma)kr$(comma)kret,%,$(FLAIR_EXIT_DOS_SPEC))
PUBS_FLAIR_ENV  := PUBS_FLAIR_IMG=$(FLAIRTENANTS_IMG) PUBS_FLAIR_DATA=$(FLAIR_DATA_IMG) PUBS_FLAIR_EXIT='$(PUBS_FLAIR_EXIT)'

pubs-screens: $(PUBS_SCREEN_DEPS)
	@set -e; for m in $(PUBS_MANUALS); do \
		d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/SCREENS ] || continue; \
		$(PUBS_FLAIR_ENV) sh $(PUBS_DIR)/screens/capture.sh $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) \
			$$d/SCREENS $(PUBS_DIR)/screens/disk $(PUBS_BUILD)/screens/$$m $(PUBS_UTILS_DIR) $(PUBS_SAMIR_CAPTURE); \
		cp -f $(PUBS_BUILD)/screens/$$m/*.txt $$d/; \
		printf '>>> pubs-screens: %s updated -- review `git diff %s`\n' "$$m" "$$d"; done
test-pubs-screens: $(PUBS_SCREEN_DEPS)
	@set -e; n=0; for m in $(PUBS_MANUALS); do \
		d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/SCREENS ] || continue; \
		$(PUBS_FLAIR_ENV) sh $(PUBS_DIR)/screens/capture.sh $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) \
			$$d/SCREENS $(PUBS_DIR)/screens/disk $(PUBS_BUILD)/screens-check/$$m $(PUBS_UTILS_DIR) $(PUBS_SAMIR_CAPTURE); \
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

$(PUBS_GATE): harness/pubs/pubs_gate.c harness/pubs/srclockstep.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(PUBS_INT21): harness/pubs/int21_inventory.c $(KERNEL_INT21_C) $(DOS_MESSAGES_H) | $(BUILD)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -Ispec -Ios/milton -Iseed -Ibuild -Wl,--gc-sections -o $@ $<
$(PUBS_SURFACE): harness/pubs/dos_surface_inventory.c os/milton/batch.c os/milton/batch.h \
		$(KERNEL_CONFIG_SYS_C) os/milton/config_sys.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ios/milton -Iseed -o $@ $<
# The replay runner links the WHOLE shipped dot prompt (samir_main.c and every
# module SAMIR.EXE registers) since bead initech-ipu5.8: the InitechBase
# manuals replay their examples through samir_repl (the session route).
$(PUBS_REPLAY): harness/pubs/samir_replay.c $(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $^

# ---- InitechBase (bead initech-ipu5.8; PUB-REF-0001 GC-01/02/03, CP-11/12) ----
# The shipped InitechBase surface, asked of the shipped dot prompt
# (harness/pubs/samir_inventory.c): NAME|command, NAME|function, NAME|set for
# every candidate word of the SAMIR sources the REPL accepts, and the message
# catalogue n|text.  The candidates are every one- or two-word upper-case
# element of a string literal of the sources the dispatcher compiles, \0
# separating the elements of a packed grammar table ("TYPE\0RENAME\0...",
# rt_kw_index; harness/pubs/samir_candidates.sh, initech-u01i); a two-word
# element such as "CLEAR MEMORY" gives both its words.  The Sample Programs
# and Utilities diskette is minted from pubs/examples/samir/disk through the
# shipped writers
# (harness/pubs/samir_sample_disk.c); every InitechBase example replays
# against a fresh copy of it.
PUBS_SAMIR_INV      := $(BUILD)/pubs_samir_inventory
PUBS_SAMIR_DISKTOOL := $(BUILD)/pubs_samir_sample_disk
PUBS_SAMIR_DISK     := $(PUBS_BUILD)/samir-disk
PUBS_SAMIR_DISK_SRC := $(sort $(wildcard $(PUBS_DIR)/examples/samir/disk/*))
PUBS_SAMIR_BOOKS    := initechbase-volume-one initechbase-volume-two initechbase-quick-reference
PUBS_SAMIR_ALLOW    := harness/pubs/samir.allow
PUBS_SAMIR_CAND     := harness/pubs/samir_candidates.sh
PUBS_SAMIR_CAND_SRC  = $(SAMIR_MAIN_SRC) $(wildcard $(SAMIR_CMD_DIR)/*.c $(SAMIR_DIR)/core/*.c) $(SAMIR_HEADERS)
$(PUBS_SAMIR_INV): harness/pubs/samir_inventory.c $(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) $(SAMIR_HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ harness/pubs/samir_inventory.c $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC)
$(PUBS_SAMIR_DISKTOOL): harness/pubs/samir_sample_disk.c $(SAMIR_DBF_SRC) $(SAMIR_DBT_SRC) $(SAMIR_NDX_SRC) $(SAMIR_VALUE_SRC) $(SAMIR_RT_SRC) $(SAMIR_PAL_HOST_SRC) $(SAMIR_HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ harness/pubs/samir_sample_disk.c $(SAMIR_DBF_SRC) $(SAMIR_DBT_SRC) $(SAMIR_NDX_SRC) $(SAMIR_VALUE_SRC) $(SAMIR_RT_SRC) $(SAMIR_PAL_HOST_SRC)
$(PUBS_SAMIR_DISK)/.minted: $(PUBS_SAMIR_DISKTOOL) $(PUBS_SAMIR_DISK_SRC)
	@rm -rf $(PUBS_SAMIR_DISK) && mkdir -p $(PUBS_SAMIR_DISK)
	@$(PUBS_SAMIR_DISKTOOL) $(PUBS_DIR)/examples/samir/disk $(PUBS_SAMIR_DISK)
	@touch $@
.PHONY: pubs-samir-inventory pubs-examples test-pubs-samir-disk test-pubs-samir-programs
# A real file target (bead initech-7opiw): the inventory is re-probed only
# when the inventory tool, the candidate scanner or the SAMIR sources change,
# so the per-merge test-pubs-lockstep-src is not charged for it every time.
pubs-samir-inventory: $(PUBS_CHECK)/samir.shipped
$(PUBS_CHECK)/samir.shipped: $(PUBS_SAMIR_INV) $(PUBS_SAMIR_CAND) $(PUBS_SAMIR_CAND_SRC)
	@mkdir -p $(PUBS_CHECK)/samir-probe
	@sh $(PUBS_SAMIR_CAND) $(PUBS_SAMIR_CAND_SRC) > $(PUBS_CHECK)/samir.candidates
	@cd $(PUBS_CHECK)/samir-probe && $(abspath $(PUBS_SAMIR_INV)) < ../samir.candidates | LC_ALL=C sort > ../samir.shipped
	@$(PUBS_SAMIR_INV) messages > $(PUBS_CHECK)/samir.catalogue
	@[ -z "$$(ls -A $(PUBS_CHECK)/samir-probe)" ] || { printf '!!! test-pubs RED SAMIR-INVENTORY: a probe left files behind\n'; exit 1; }
	@printf '>>> pubs-samir-inventory: %s shipped InitechBase names (%s commands, %s functions, %s SET options), %s catalogue messages\n' \
		"$$(wc -l < $(PUBS_CHECK)/samir.shipped)" "$$(grep -c '|command$$' $(PUBS_CHECK)/samir.shipped)" \
		"$$(grep -c '|function$$' $(PUBS_CHECK)/samir.shipped)" "$$(grep -c '|set$$' $(PUBS_CHECK)/samir.shipped)" \
		"$$(wc -l < $(PUBS_CHECK)/samir.catalogue)"
# Re-mint every InitechBase example transcript from the shipped dot prompt
# into the source tree (a deliberate act, like pubs-screens: review the diff).
pubs-examples: $(PUBS_REPLAY) $(PUBS_SAMIR_DISK)/.minted
	@set -e; n=0; for m in $(PUBS_SAMIR_BOOKS); do \
		for d in $(PUBS_DIR)/manuals/$$m/examples/*.dot; do [ -f "$$d" ] || continue; \
			w=$(PUBS_BUILD)/examples-work; rm -rf $$w; cp -r $(PUBS_SAMIR_DISK) $$w; \
			$(PUBS_REPLAY) session $$w $$d > $${d%.dot}.txt; n=$$((n + 1)); done; done; \
	printf '>>> pubs-examples: %s transcripts re-minted -- review `git diff pubs/manuals`\n' "$$n"
# CP-12: every file the books name on the sample diskette exists there, and
# the diskette has the files its MANIFEST lists.
test-pubs-samir-disk: $(PUBS_SAMIR_DISK)/.minted
	@set -e; while IFS='|' read -r name src; do case $$name in ''|'#'*) continue;; esac; \
		[ -f $(PUBS_SAMIR_DISK)/$$name ] || { printf '!!! test-pubs RED SAMPLE-DISK: %s missing\n' "$$name"; exit 1; }; \
	done < $(PUBS_DIR)/examples/samir/disk/MANIFEST
	@printf '>>> test-pubs-samir-disk: green (%s files on the Sample Programs and Utilities diskette)\n' "$$(ls $(PUBS_SAMIR_DISK) | wc -l)"

# ---- CADENCE (operator ruling 2026-10-25; bead initech-7opiw, F1+F6) -------
# Typesetting runs once per SESSION, never per merge.
#   per merge   : test-pubs-lockstep-src (+ -mutant): the InitechBase manuals
#                 in lockstep with the shipped inventory, read from the .tex
#                 SOURCES by `pubs_gate srclockstep` -- no LaTeX, about 1 s.
#                 In TEST_UNIT_GATES.
#   per session : make test-pubs (test-pubs-build typesets every document,
#                 then the LaTeX-derived tps/commands/examples/screens/repro
#                 gates).  pubs/build.sh runs pass 1 in parallel (PUBS_JOBS,
#                 default nproc) and pass 2 (master index) after it.
# test-pubs-tps and test-pubs-examples still depend on test-pubs-build, so
# they are session gates, not merge gates.  No gate was removed from test-pubs.
.PHONY: test-pubs-lockstep-src test-pubs-lockstep-src-mutant
PUBS_SAMIR_ENTRY_IDX := $(filter $(foreach b,$(PUBS_SAMIR_BOOKS),$(PUBS_GEN)/entries-$(b)-%),$(PUBS_ENTRY_IDX))
test-pubs-lockstep-src: $(PUBS_GATE) $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_GEN) $(PUBS_SAMIR_ENTRY_IDX)
	@$(PUBS_GATE) srclockstep $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) \
		$(PUBS_DIR)/manuals/initechbase-volume-one $(PUBS_DIR)/manuals/initechbase-quick-reference \
		$(PUBS_DIR)/manuals/initechbase-volume-two $(PUBS_GEN)
test-pubs-lockstep-src-mutant: test-pubs-lockstep-src
	@sh harness/pubs/srclockstep_mutants.sh $(PUBS_GATE) $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) \
		$(PUBS_DIR)/manuals $(PUBS_BUILD)/lockstep-src-mutants $(PUBS_GEN)

.PHONY: test-pubs test-pubs-commands test-pubs-tps test-pubs-lint test-pubs-examples test-pubs-mutant pubs-inventory \
	test-pubs-inconsistency
# initech-77am; MS-DOS 3.3 LABEL pp.86-87, GC-01/03/04. The normal screen
# gate also replays labelset on the shipped OS and compares every byte.
PUBS_LABEL_ENTRY := $(PUBS_DIR)/manuals/initechdos-reference/ch2-entries-b.tex
PUBS_LABEL_SCREEN := $(PUBS_DIR)/manuals/initechdos-reference/screens/labelset
.PHONY: test-pubs-label test-pubs-label-mutant
test-pubs-label:
	@sh harness/pubs/label_check.sh $(PUBS_LABEL_ENTRY) $(PUBS_LABEL_SCREEN).keys $(PUBS_LABEL_SCREEN).txt
test-pubs-label-mutant: test-pubs-label
	@sh harness/pubs/label_mutants.sh harness/pubs/label_check.sh $(PUBS_LABEL_ENTRY) $(PUBS_LABEL_SCREEN).keys $(PUBS_LABEL_SCREEN).txt $(PUBS_CHECK)/label-mutants
# VO-30 (PUB-REF-0001 Sec 9.3; bead initech-ipu5.16; WL-0150): the house
# register of tolerated inconsistencies, LOCKED in spec/pubs_inconsistencies.json.
# Every book listed here carries its budget of \Slip{ID}{text} slips (just
# enough to pass QA): `pubs_gate inconsistency` reads the register, the
# books' sources and the .slips/.toc/.log their build wrote; slip_places.sh
# proves the class reports each printed place.  A book joins the register
# by being added to PUBS_SLIP_BOOKS with its slips placed.
PUBS_SLIP_REGISTER := spec/pubs_inconsistencies.json
PUBS_SLIP_BOOKS    := initechdos-reference initechdos-users-guide $(PUBS_TPS_BOOK)
# Recursive expansion sees FLAIR_FIXTURES, included after this makefile.
# The shipped inventory is NAME|KIND (pubs_gate.c): command.c's dispatch
# table, the INT 21h register, the APPS executables, and the batch
# subcommands / CONFIG.SYS directives the real batch_classify and
# config_sys_parse accept (candidates: the quoted words of their sources).
pubs-inventory: $(PUBS_GATE) $(PUBS_INT21) $(PUBS_SURFACE) $(DOS_MESSAGES_H) pubs-samir-inventory
	@mkdir -p $(PUBS_CHECK)
	@$(CC) -E -P -Ispec -Ios/milton -Ibuild os/milton/command.c > $(PUBS_CHECK)/command.i
	@$(PUBS_INT21) > $(PUBS_CHECK)/int21.list
	@grep -oh '"[A-Z][A-Z0-9]*"' os/milton/batch.c $(KERNEL_CONFIG_SYS_C) | tr -d '"' | sort -u | $(PUBS_SURFACE) > $(PUBS_CHECK)/surface.list
	@$(PUBS_GATE) inventory $(PUBS_CHECK)/command.i $(PUBS_CHECK)/int21.list $(PUBS_CHECK)/surface.list $(filter APPS/% DOS/%,$(FLAIR_FIXTURES)) > $(PUBS_CHECK)/shipped

ifeq ($(SKIP_PUBS),1)
test-pubs test-pubs-mutant test-pubs-tps test-pubs-inconsistency:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- publication correctness was NOT checked. This is NOT a pass; unset SKIP_PUBS before trusting this gate.\n'
else
test-pubs: test-pubs-build test-pubs-commands test-pubs-tps test-pubs-lint test-pubs-examples test-pubs-screens test-pubs-samir-programs test-pubs-iword \
	test-pubs-inconsistency test-pubs-label
	@printf '>>> test-pubs: green (command coverage, TPS message inventory, InitechBase programs, InitechWord surface and replay, example replay, source lints, build cap, house register)\n'
test-pubs-inconsistency: test-pubs-build $(PUBS_GATE)
	@$(PUBS_GATE) inconsistency $(PUBS_SLIP_REGISTER) $(PUBS_DIR)/class $(PUBS_DIR)/manuals $(PUBS_BUILD)/gate $(PUBS_SLIP_BOOKS)
	@sh harness/pubs/slip_places.sh $(PUBS_BUILD)
# GC-01 over the InitechDOS books together (the Reference documents, the
# User's Guide teaches): their main.cmds lists (initech-syntax.sty, the one
# registration mechanism) against the shipped inventory, both directions,
# and every inline \cmd against the shipped names.  Any other book must
# stay empty until its product has its own shipped inventory.  The
# InitechWord books have theirs (harness/pubs/iword_probe.c): their .cmds
# are graded by test-pubs-iword below; their inline \cmd list stays empty.
PUBS_DOS_BOOKS := initechdos-reference initechdos-users-guide
test-pubs-commands: pubs-inventory test-pubs-build
	@set -e; for idx in $(PUBS_BUILD)/gate/*/main.cmds $(PUBS_BUILD)/gate/*/main.mentions; do \
		case $$idx in */initechdos-*/*|*/$(PUBS_TPS_BOOK)/*|*/initechbase-*/*|*/turbo-initech-*/*|*/initechword-*/main.cmds) continue;; esac; \
		[ -z "$$(grep -v '^#' "$$idx" || true)" ] || { printf '!!! test-pubs RED COMMAND-SURFACE: %s needs its own shipped inventory before it can contain entries\n' "$$idx"; exit 1; }; done
	@$(PUBS_GATE) commands $(PUBS_CHECK)/shipped $(PUBS_ALLOW) $(foreach b,$(PUBS_DOS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.cmds)
	@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(foreach b,$(PUBS_DOS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
	@sh harness/pubs/check_allowlist.sh $(PUBS_GATE) $(PUBS_ALLOW) $(PUBS_CHECK)
# GC-01/GC-02 for the TPS Report Generator manual (bead initech-ipu5.14), the
# second product with its own shipped inventory (test-pubs-commands skips it
# for that reason only): its main.cmds against the inventory GENERATED from
# spec/tps_ritual.json, both directions (every program message documented;
# nothing documented that the program lacks, unless DRAFT); TPSREP|external
# documented and in the DOS shipped inventory; the alignment count written
# in its four required places (bead initech-5igf); every inline \cmd a
# shipped command.  DRAFT transcripts (decided, not yet shipped) are listed
# on every run and must exist with their .keys.
PUBS_TPS_GATE_DIR := $(PUBS_BUILD)/gate/$(PUBS_TPS_BOOK)
# The files the book prints with \ReportLines (paths as the book names them,
# from its own directory), each compared with the running program by a gate:
# tpsrept_expected.txt == TPSREPT.TXT written on the flagship and under real
# MS-DOS 6.22 (test-flair-tps-journey, test-tps-report leg A); tpsparm.dat is
# the very file the Makefile plants as APPS\TPSPARM.DAT (FLAIR_TPSPARM_DAT)
# and the one test-tps-report runs with; dos_banner.txt == the banner after
# Exit to InitechDOS (test-flair-exit-to-dos leg 3).
PUBS_TPS_PROVED := ../../../harness/diff/tps/tpsrept_expected.txt ../../../$(FLAIR_TPSPARM_DAT) ../../../spec/dos_banner.txt
test-pubs-tps: test-pubs-build pubs-inventory $(PUBS_TPS_INV_BIN) $(PUBS_GATE)
	@$(PUBS_TPS_INV_BIN) inventory spec/tps_ritual.json > $(PUBS_CHECK)/tps.inventory
	@$(PUBS_TPS_INV_BIN) check $(PUBS_CHECK)/tps.inventory $(PUBS_TPS_GATE_DIR)/main.cmds $(PUBS_CHECK)/shipped
	@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(PUBS_TPS_GATE_DIR)/main.mentions
	@$(PUBS_TPS_INV_BIN) reports $(PUBS_TPS_GATE_DIR)/main.reports $(PUBS_TPS_PROVED)
	@set -e; n=0; while IFS='|' read -r f why; do [ -n "$$f" ] || continue; \
		d=$(PUBS_DIR)/manuals/$(PUBS_TPS_BOOK); \
		[ -f "$$d/$$f" ] && [ -f "$$d/$${f%.draft}.keys" ] || { printf '!!! test-pubs RED DRAFT-SCREEN-PAIR: %s/%s\n' "$$d" "$$f"; exit 1; }; \
		printf '!!! test-pubs DRAFT-SCREEN: %s NOT replayed (%s) -- re-capture after the change ships\n' "$$f" "$$why"; n=$$((n + 1)); \
	done < $(PUBS_TPS_GATE_DIR)/main.draftscreens; \
	printf '>>> test-pubs-tps: green (%s DRAFT transcripts listed above)\n' "$$n"
	@# InitechBase (initech-ipu5.8): Using InitechBase (Volume One) and the
	@# Quick Reference Guide each document the whole shipped surface, both
	@# directions, by NAME|KIND; Volume Two teaches and only mentions.  Every
	@# printed message is the catalogue text; Volume One has one entry per
	@# catalogue message (GC-02).
	@$(PUBS_GATE) commands $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) $(PUBS_BUILD)/gate/initechbase-volume-one/main.cmds
	@$(PUBS_GATE) commands $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) $(PUBS_BUILD)/gate/initechbase-quick-reference/main.cmds
	@[ -z "$$(grep -v '^#' $(PUBS_BUILD)/gate/initechbase-volume-two/main.cmds || true)" ] || { \
		printf '!!! test-pubs RED COMMAND-SURFACE: Volume Two teaches; its commands are documented in Using InitechBase\n'; exit 1; }
	@$(PUBS_GATE) samirmentions $(PUBS_CHECK)/samir.shipped $(foreach b,$(PUBS_SAMIR_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
	@$(PUBS_GATE) messages $(PUBS_CHECK)/samir.catalogue $(foreach b,$(PUBS_SAMIR_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.msgs)
	@sh harness/pubs/check_allowlist.sh $(PUBS_GATE) $(PUBS_SAMIR_ALLOW) $(PUBS_CHECK)
PUBS_GENERATED += $(PUBS_SAMIR_GEN)
test-pubs-lint: $(PUBS_GATE)
	@$(PUBS_GATE) lint pubs/denylist.txt pubs
	@sha256sum -c pubs/fonts.lock > $(BUILD)/pubs-fonts.log
	@printf '>>> test-pubs-fonts: green (all font files match fonts.lock)\n'
test-pubs-examples: test-pubs-build $(PUBS_REPLAY) $(PUBS_GATE) test-pubs-samir-disk
	@sh harness/pubs/replay.sh $(PUBS_REPLAY) $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK) $(PUBS_SAMIR_DISK)
# GC-11 for InitechBase: the month-end programs and their output as the
# reference programs of harness/diff/dbf_diff run them.
test-pubs-samir-programs:
	@sh harness/pubs/samir_programs.sh $(PUBS_DIR)/examples/samir/disk $(CANON_DIR) $(PUBS_DIR)/manuals/initechbase-volume-two
# One recipe (two used to be merged in, and make ran only the last: the TPS
# manual's mutants were silently skipped).
test-pubs-mutant: test-pubs test-pubs-label-mutant $(PUBS_GATE) $(PUBS_SAMIR_INV) $(PUBS_TPS_INV_BIN)
	@sh harness/pubs/mutants.sh $(PUBS_GATE) $(PUBS_REPLAY) $(PUBS_CHECK) $(PUBS_BUILD)/gate $(PUBS_ALLOW) $(PUBS_SAMIR_DISK) $(PUBS_SAMIR_INV) \
		$(PUBS_SAMIR_CAND) $(PUBS_SAMIR_CAND_SRC)
	@sh harness/pubs/tps_mutants.sh $(PUBS_TPS_INV_BIN) $(PUBS_GATE) $(PUBS_CHECK) $(PUBS_BUILD)/gate/$(PUBS_TPS_BOOK) $(PUBS_GEN)
	@sh harness/pubs/slip_mutants.sh $(PUBS_GATE) $(PUBS_SLIP_REGISTER) $(PUBS_DIR)/class $(PUBS_DIR)/manuals $(PUBS_BUILD)/gate $(PUBS_BUILD) $(PUBS_SLIP_BOOKS)
	@if sh harness/pubs/slip_places.sh $(PUBS_BUILD) mutant > $(PUBS_BUILD)/slip-places-mutant.log 2>&1; then \
		cat $(PUBS_BUILD)/slip-places-mutant.log; printf '!!! test-pubs-mutant FAIL: the class place proof stayed green with the running-head place switched off\n'; exit 1; fi
	@grep -q 'RED SLIP-PLACES: the class reports' $(PUBS_BUILD)/slip-places-mutant.log || { cat $(PUBS_BUILD)/slip-places-mutant.log; \
		printf '!!! test-pubs-mutant FAIL: the class place mutant went RED for another reason\n'; exit 1; }
	@printf '>>> test-pubs-mutant: VO-30 class place proof RED as required (running-head place switched off: %s)\n' \
		"$$(grep '^+VO30' $(PUBS_BUILD)/slip-places-mutant.log | head -1)"
	@$(MAKE) --no-print-directory test-pubs-iword-mutant
PUBS_GENERATED += $(PUBS_SAMIR_GEN)
endif

# TeX emits the figure registry, preventing unchecked printed captures.
test-pubs-screens: test-pubs-build $(PUBS_GATE)

.PHONY: test-pubs-families
# The identity leg (PUB-REF-0001 D-8 revised, PK-01/02/03/05; WL-0132) grades
# rendered covers with cover_check (C) against the measured Initech Cyan.
PUBS_COVER_CHECK := $(BUILD)/pubs_cover_check
$(PUBS_COVER_CHECK): harness/pubs/cover_check.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
ifeq ($(SKIP_PUBS),1)
test-pubs-families:
	@printf '!!! test-pubs-families SKIPPED (SKIP_PUBS=1 opt-out) -- family furniture was NOT checked; this is NOT a pass.\n'
else
test-pubs-families: $(PUBS_COVER_CHECK)
	@command -v pdftoppm >/dev/null 2>&1 || { printf '!!! test-pubs-families FAIL: pdftoppm (poppler-utils) not found; the identity leg renders the covers with it\n'; exit 1; }
	@COVER_CHECK=$(PUBS_COVER_CHECK) sh harness/pubs/families.sh
endif

# The Turbo Initech manuals carry their own shipped inventory (bead
# initech-ipu5.12): pubs/turbo.mk.
include pubs/turbo.mk
# ---------------------------------------------------------------------------
# The InitechWord manuals (bead initech-ipu5.10; PUB-REF-0001 GC-01..04,
# CP-16, CP-17, PK-14).  The shipped inventory is MEASURED from WORD.EXE's
# own code: harness/pubs/iword_probe.c compiles os/iword/app/face_a.c over
# the real document core with a recording Toolbox mock and drives it --
# every key, menu choice, pull-down item and message that does something,
# every Reveal Codes label of the locked spec/iword/reveal_codes.h.  The same
# program replays every keystroke sequence the books print and the gate
# diffs the screens (harness/pubs/iword_check.sh).  The FLAIR figures are
# pixels from the booted desktop (pubs/screens/flair_capture.sh +
# harness/pubs/ppm_grey_png.c), re-captured by test-pubs-screens; being
# binary they are committed under fixtures/pubs/<book>/ (pubs/ is ASCII only,
# GC-05), their traces under pubs/manuals/<book>/flair/.
#   make test-pubs-iword          the gate (in test-pubs)
#   make test-pubs-iword-mutant   its named mutants (in test-pubs-mutant)
#   make pubs-iword-screens       re-replay and write the replayed screens
#                                 into pubs/manuals (review the diff)
#   make pubs-flair-screens       re-capture the FLAIR figures into the tree
#   make test-pubs-flair-screens  EMU GATE: re-capture and cmp (in
#                                 test-pubs-screens)
# ---------------------------------------------------------------------------
PUBS_IWORD_PROBE := $(BUILD)/pubs_iword_probe
PUBS_IWORD_ALLOW := harness/pubs/iword.allow
PUBS_PPM_GREY    := $(BUILD)/pubs_ppm_grey_png
PUBS_IWORD_SRC    = harness/pubs/iword_probe.c os/iword/app/face_a.c $(IWORD_CORE_SRC) $(IWORD_CORE_HDR) \
                    os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/iword/settings.h
PUBS_IWORD_CC     = $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ios/apps/tbx $(IWORD_CORE_INC)
$(PUBS_IWORD_PROBE): $(PUBS_IWORD_SRC) | $(BUILD)
	$(PUBS_IWORD_CC) -o $@ harness/pubs/iword_probe.c $(IWORD_CORE_SRC)
# Rule 6: a WORD.EXE that never shows "Block on" (face_a.c's own mutant knob)
$(BUILD)/pubs_iword_probe_noblk: $(PUBS_IWORD_SRC) | $(BUILD)
	$(PUBS_IWORD_CC) -DIWORD_MUT_NO_BLOCK_ON -o $@ harness/pubs/iword_probe.c $(IWORD_CORE_SRC)
# Rule 6: a WORD.EXE whose plain F9 shows no grey End Field (the P3 lane's
# IWORD_MUT_FKEY_DROP knob, initech-qvgd)
$(BUILD)/pubs_iword_probe_fkdrop: $(PUBS_IWORD_SRC) | $(BUILD)
	$(PUBS_IWORD_CC) -DIWORD_MUT_FKEY_DROP -o $@ harness/pubs/iword_probe.c $(IWORD_CORE_SRC)
$(PUBS_PPM_GREY): harness/pubs/ppm_grey_png.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
PUBS_IWORD_MUTANT_DEPS = $(PUBS_IWORD_PROBE) $(BUILD)/pubs_iword_probe_noblk $(BUILD)/pubs_iword_probe_fkdrop
PUBS_IWORD_BOOKS := initechword-reference initechword-workbook initechword-install

.PHONY: test-pubs-iword test-pubs-iword-mutant pubs-iword-screens pubs-flair-screens test-pubs-flair-screens
ifeq ($(SKIP_PUBS),1)
test-pubs-iword test-pubs-iword-mutant test-pubs-flair-screens:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- the InitechWord manuals were NOT checked. This is NOT a pass.\n'
else
test-pubs-iword: test-pubs-build $(PUBS_IWORD_PROBE) $(PUBS_GATE)
	@sh harness/pubs/iword_check.sh check $(PUBS_IWORD_PROBE) $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK) $(PUBS_IWORD_ALLOW)
test-pubs-iword-mutant: test-pubs-iword $(PUBS_IWORD_MUTANT_DEPS) $(PUBS_GATE)
	@sh harness/pubs/iword_mutants.sh $(PUBS_IWORD_PROBE) $(BUILD)/pubs_iword_probe_noblk $(BUILD)/pubs_iword_probe_fkdrop $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK) $(PUBS_IWORD_ALLOW)
test-pubs-flair-screens: test-pubs-build $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(PUBS_PPM_GREY) $(PUBS_GATE)
	@set -e; n=0; for m in $(PUBS_IWORD_BOOKS); do d=$(PUBS_DIR)/manuals/$$m/flair; f=fixtures/pubs/$$m; [ -f $$d/FLAIRSHOTS ] || continue; \
		sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(PUBS_PPM_GREY) \
			$$d/FLAIRSHOTS $(PUBS_BUILD)/flair-check/$$m spec/flair_iword_traces.mk FLAIR_IWORD_TYPE_SPEC; \
		for p in $(PUBS_BUILD)/flair-check/$$m/*.png; do \
			$(PUBS_GATE) same SCREEN-DRIFT $$f/$$(basename $$p) $$p; n=$$((n + 1)); done; \
		for c in $$f/*.png; do [ -f $(PUBS_BUILD)/flair-check/$$m/$$(basename $$c) ] || { \
			printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s has no FLAIRSHOTS row\n' "$$c"; exit 1; }; \
			grep -qxF "$$(basename $$c .png)" $(PUBS_BUILD)/gate/$$m/main.flairshots || { \
			printf '!!! test-pubs RED SCREEN-UNPRINTED: %s\n' "$$c"; exit 1; }; done; \
		while IFS= read -r x; do [ -f $$f/$$x.png ] || { printf '!!! test-pubs RED SCREEN-PAIR: %s/%s.png is printed but not committed\n' "$$f" "$$x"; exit 1; }; \
			done < $(PUBS_BUILD)/gate/$$m/main.flairshots; done; \
	printf '>>> test-pubs-flair-screens: green (%s FLAIR figures in the InitechWord manuals == the booted desktop, pixel for pixel)\n' "$$n"
endif
test-pubs-screens: test-pubs-flair-screens
pubs-iword-screens: $(PUBS_IWORD_PROBE) $(PUBS_GATE) pubs
	@sh harness/pubs/iword_check.sh update $(PUBS_IWORD_PROBE) $(PUBS_GATE) $(PUBS_BUILD) $(PUBS_CHECK) $(PUBS_IWORD_ALLOW)
pubs-flair-screens: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(PUBS_PPM_GREY)
	@set -e; for m in $(PUBS_IWORD_BOOKS); do d=$(PUBS_DIR)/manuals/$$m/flair; [ -f $$d/FLAIRSHOTS ] || continue; \
		sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(PUBS_PPM_GREY) \
			$$d/FLAIRSHOTS $(PUBS_BUILD)/flair/$$m spec/flair_iword_traces.mk FLAIR_IWORD_TYPE_SPEC; \
		mkdir -p fixtures/pubs/$$m; cp -f $(PUBS_BUILD)/flair/$$m/*.png fixtures/pubs/$$m/; \
		printf '>>> pubs-flair-screens: fixtures/pubs/%s updated -- review the figures\n' "$$m"; done
