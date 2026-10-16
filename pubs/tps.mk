# ---------------------------------------------------------------------------
# Turbo Initech manuals: generated inputs and correctness gates (bead
# initech-ipu5.12; PUB-REF-0001 GC-01, GC-02, GC-03, CP-18, CP-19).  Included
# from pubs/pubs.mk.  The four Turbo Initech books and the Quick Reference
# (pubs/manuals/turbo-initech-*) are checked against an inventory GENERATED
# from the product (os/tps/tps.pas through its own lexer, seed/, seed/rt/):
#
#   make pubs-tps-gen        the shipped inventory, the keyword list for the
#                            program listings, one railroad diagram per rule
#                            of the printed grammar (into build/pubs/gen)
#   make test-pubs-tps       HOST+EMU GATE: documented == shipped both ways
#                            (tps_gate commands), diagnostics verbatim
#                            (msgcheck), inline names shipped (mentions),
#                            InitechDOS commands named in the books shipped,
#                            the printed grammar == the shipped parser both
#                            ways (tps_grammar.sh), every printed program,
#                            output, compiler output, assembly excerpt and
#                            development-system run replayed (tps_replay.sh:
#                            Free Pascal, the seed, Turbo Initech, and one
#                            InitechDOS boot running every example .COM)
#   make test-pubs-tps-mutant   Rule 6: one named mutant per rule goes RED
# test-pubs and test-pubs-mutant depend on these; SKIP_PUBS=1 shouts.
# ---------------------------------------------------------------------------
PUBS_TPS_DIR      := $(PUBS_BUILD)/tps
PUBS_TPS_GATE     := $(BUILD)/pubs_tps_gate
PUBS_RAILROAD     := $(BUILD)/pubs_railroad
PUBS_TPS_GRAMMAR  := $(PUBS_DIR)/manuals/turbo-initech-programmers-guide/grammar/tps.ebnf
PUBS_TPS_LEXICAL  := $(PUBS_DIR)/manuals/turbo-initech-programmers-guide/grammar/lexical.ebnf
PUBS_TPS_SHIPPED  := $(PUBS_TPS_DIR)/shipped
PUBS_TPS_MSGS     := $(PUBS_TPS_DIR)/messages
PUBS_TPS_KW_TEX   := $(PUBS_GEN)/tps_keywords.tex
PUBS_TPS_RR_STAMP := $(PUBS_GEN)/rr-program.tex
PUBS_TPS_RT       := seed/rt/start.asm seed/rt/start_dos.asm seed/rt/fileio.asm
PUBS_TPS_BOOKS     = $(patsubst $(PUBS_DIR)/manuals/%/main.tex,%,$(wildcard $(PUBS_DIR)/manuals/turbo-initech-*/main.tex))
PUBS_TPS_GRAMMAR_N ?= 300
PUBS_TPS_REPLAY_ENV = TPS_BIN=$(TPS_LEX_FPC_BIN) SEED_BIN=$(SEED_BIN) NASM=$(NASM) LD=$(LD) \
	OBJCOPY=$(OBJCOPY) SEED_RT_OBJ=$(SEED_RT_OBJ) SEED_DOS_RT_OBJ=$(SEED_DOS_RT_OBJ) \
	SEED_FILEIO_RT_OBJ=$(SEED_FILEIO_RT_OBJ) SEED_RT_LD=$(SEED_RT_LD) SEED_DOS_LD=$(SEED_DOS_LD) \
	HARNESS_BIN=$(HARNESS_BIN) TRACER_IMG=$(SECONDARY_TRACER_IMG) GATE=$(PUBS_TPS_GATE) MAKE="$(MAKE)"

$(PUBS_TPS_GATE): harness/pubs/tps_gate.c seed/lexer.c seed/token.c seed/lexer.h seed/token.h | $(BUILD)
	$(CC) $(CFLAGS) -Wno-format-truncation -Iseed -o $@ harness/pubs/tps_gate.c seed/lexer.c seed/token.c
$(PUBS_RAILROAD): harness/pubs/railroad.c | $(BUILD)
	$(CC) $(CFLAGS) -Wno-format-truncation -o $@ $<
$(PUBS_TPS_SHIPPED): $(PUBS_TPS_GATE) $(TPS_LEX_FPC_BIN) $(TPS_LEX_SRC) seed/lexer.c seed/initechc.c $(PUBS_TPS_RT)
	@rm -rf $(PUBS_TPS_DIR)/probe; mkdir -p $(PUBS_TPS_DIR)/probe
	@$(PUBS_TPS_GATE) candidates $(TPS_LEX_SRC) seed/lexer.c > $(PUBS_TPS_DIR)/probe/TPSIN.PAS
	@cd $(PUBS_TPS_DIR)/probe && $(abspath $(TPS_LEX_FPC_BIN)) > lex.txt
	@$(PUBS_TPS_GATE) inventory $(TPS_LEX_SRC) $(PUBS_TPS_DIR)/probe/lex.txt seed/initechc.c $(PUBS_TPS_RT) > $@.tmp
	@$(PUBS_TPS_GATE) messages $(TPS_LEX_SRC) seed/rt/fileio.asm > $(PUBS_TPS_MSGS)
	@mv -f $@.tmp $@
	@printf '>>> pubs-tps-gen: shipped inventory %s items (%s)\n' "$$(wc -l < $@)" "$@"
$(PUBS_TPS_KW_TEX): $(PUBS_TPS_SHIPPED)
	@mkdir -p $(PUBS_GEN)
	@printf '%% Generated from the shipped lexer by pubs/tps.mk; do not edit.\n\\def\\TPSKeywordList{%s}\n' \
		"$$(sed -n 's/|reserved$$//p' $< | paste -sd, -)" > $@
$(PUBS_TPS_RR_STAMP): $(PUBS_RAILROAD) $(PUBS_TPS_GRAMMAR) $(PUBS_TPS_LEXICAL)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_RAILROAD) tikz $(PUBS_TPS_GRAMMAR) $(PUBS_GEN)
	@$(PUBS_RAILROAD) tikz-lexical $(PUBS_TPS_LEXICAL) $(PUBS_GEN)

.PHONY: pubs-tps-gen test-pubs-tps test-pubs-tps-mutant
pubs-tps-gen: $(PUBS_TPS_KW_TEX) $(PUBS_TPS_RR_STAMP)

ifeq ($(SKIP_PUBS),1)
test-pubs-tps test-pubs-tps-mutant:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- the Turbo Initech manuals were NOT checked. This is NOT a pass; unset SKIP_PUBS before trusting this gate.\n'
else
pubs test-pubs-build test-pubs-repro test-pubs-repro-mutant: pubs-tps-gen
test-pubs: test-pubs-tps
test-pubs-mutant: test-pubs-tps-mutant
PUBS_TPS_GATE_DEPS := test-pubs-build pubs-inventory $(PUBS_TPS_SHIPPED) $(PUBS_RAILROAD) $(HARNESS_BIN) \
	$(SECONDARY_TRACER_IMG) $(SEED_BIN) $(SEED_RT_OBJ) $(SEED_DOS_RT_OBJ) $(SEED_FILEIO_RT_OBJ)
test-pubs-tps: $(PUBS_TPS_GATE_DEPS)
	@printf '>>> test-pubs-tps: the Turbo Initech manuals against the shipped compiler (initech-ipu5.12)\n'
	@test -n "$(PUBS_TPS_BOOKS)" || { printf '!!! test-pubs RED TPS-BOOKS: no pubs/manuals/turbo-initech-*/main.tex\n'; exit 1; }
	@$(PUBS_TPS_GATE) commands $(PUBS_TPS_SHIPPED) $(foreach b,$(PUBS_TPS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.cmds)
	@$(PUBS_TPS_GATE) msgcheck $(PUBS_TPS_MSGS) $(foreach b,$(PUBS_TPS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.msgs)
	@$(PUBS_TPS_GATE) mentions $(PUBS_TPS_SHIPPED) $(foreach b,$(PUBS_TPS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.tpsmentions)
	@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(foreach b,$(PUBS_TPS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
	@sh harness/pubs/tps_grammar.sh $(PUBS_RAILROAD) $(PUBS_TPS_GRAMMAR) $(TPS_LEX_FPC_BIN) $(PUBS_TPS_DIR)/grammar \
		$(PUBS_TPS_GRAMMAR_N) $(wildcard $(TPS_LEX_FIXTURE_DIR)/gen_*.pas) $(TPS_PARSE_RICH) $(TPS_LEX_SRC) \
		$(filter-out %/err-%.pas,$(wildcard $(PUBS_DIR)/manuals/turbo-initech-*/examples/*.pas))
	@$(PUBS_TPS_REPLAY_ENV) BUILT=$(PUBS_BUILD)/gate OUT=$(PUBS_TPS_DIR)/replay sh harness/pubs/tps_replay.sh
	@printf '>>> test-pubs-tps: green\n'
test-pubs-tps-mutant: test-pubs-tps
	@$(PUBS_TPS_REPLAY_ENV) BUILT=$(PUBS_BUILD)/gate RAILROAD=$(PUBS_RAILROAD) GRAMMAR=$(PUBS_TPS_GRAMMAR) \
		SHIPPED=$(PUBS_TPS_SHIPPED) MSGS=$(PUBS_TPS_MSGS) OUT=$(PUBS_TPS_DIR)/mutants \
		PARSE_RICH=$(TPS_PARSE_RICH) FIXTURES=$(TPS_LEX_FIXTURE_DIR) sh harness/pubs/tps_mutants.sh
endif
