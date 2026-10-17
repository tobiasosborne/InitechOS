# ---------------------------------------------------------------------------
# Turbo Initech manuals: generated inputs and correctness gates (bead
# initech-ipu5.12; PUB-REF-0001 GC-01, GC-02, GC-03, CP-18, CP-19).  Included
# from pubs/pubs.mk.  The four Turbo Initech books and the Quick Reference
# (pubs/manuals/turbo-initech-*) are checked against an inventory GENERATED
# from the product (os/tps/tps.pas through its own lexer, seed/, seed/rt/):
#
#   make pubs-turbo-gen        the shipped inventory, the keyword list for the
#                            program listings, one railroad diagram per rule
#                            of the printed grammar (into build/pubs/gen)
#   make test-pubs-turbo       HOST+EMU GATE: documented == shipped both ways
#                            (tps_gate commands), diagnostics verbatim
#                            (msgcheck), inline names shipped (mentions),
#                            InitechDOS commands named in the books shipped,
#                            every cited gate a real target (GC-09),
#                            the printed grammar == the shipped parser both
#                            ways (turbo_grammar.sh), every printed program,
#                            output, compiler output, assembly excerpt and
#                            development-system run replayed (turbo_replay.sh:
#                            Free Pascal, the seed, Turbo Initech, and one
#                            InitechDOS boot running every example .COM)
#   make test-pubs-turbo-mutant   Rule 6: one named mutant per rule goes RED
# test-pubs and test-pubs-mutant depend on these; SKIP_PUBS=1 shouts.
# ---------------------------------------------------------------------------
PUBS_TURBO_DIR      := $(PUBS_BUILD)/tps
PUBS_TURBO_GATE     := $(BUILD)/pubs_tps_gate
PUBS_RAILROAD     := $(BUILD)/pubs_railroad
PUBS_TURBO_GRAMMAR  := $(PUBS_DIR)/manuals/turbo-initech-programmers-guide/grammar/tps.ebnf
PUBS_TURBO_LEXICAL  := $(PUBS_DIR)/manuals/turbo-initech-programmers-guide/grammar/lexical.ebnf
PUBS_TURBO_SHIPPED  := $(PUBS_TURBO_DIR)/shipped
PUBS_TURBO_MSGS     := $(PUBS_TURBO_DIR)/messages
PUBS_TURBO_KW_TEX   := $(PUBS_GEN)/tps_keywords.tex
PUBS_TURBO_RR_STAMP := $(PUBS_GEN)/rr-program.tex
PUBS_TURBO_RT       := seed/rt/start.asm seed/rt/start_dos.asm seed/rt/fileio.asm
PUBS_TURBO_BOOKS     = $(patsubst $(PUBS_DIR)/manuals/%/main.tex,%,$(wildcard $(PUBS_DIR)/manuals/turbo-initech-*/main.tex))
PUBS_TURBO_GRAMMAR_N ?= 300
PUBS_TURBO_REPLAY_ENV = TPS_BIN=$(TPS_LEX_FPC_BIN) SEED_BIN=$(SEED_BIN) NASM=$(NASM) LD=$(LD) \
	OBJCOPY=$(OBJCOPY) SEED_RT_OBJ=$(SEED_RT_OBJ) SEED_DOS_RT_OBJ=$(SEED_DOS_RT_OBJ) \
	SEED_FILEIO_RT_OBJ=$(SEED_FILEIO_RT_OBJ) SEED_RT_LD=$(SEED_RT_LD) SEED_DOS_LD=$(SEED_DOS_LD) \
	HARNESS_BIN=$(HARNESS_BIN) TRACER_IMG=$(SECONDARY_TRACER_IMG) GATE=$(PUBS_TURBO_GATE) MAKE="$(MAKE)"

$(PUBS_TURBO_GATE): harness/pubs/turbo_gate.c seed/lexer.c seed/token.c seed/lexer.h seed/token.h | $(BUILD)
	$(CC) $(CFLAGS) -Wno-format-truncation -Iseed -o $@ harness/pubs/turbo_gate.c seed/lexer.c seed/token.c
$(PUBS_RAILROAD): harness/pubs/railroad.c | $(BUILD)
	$(CC) $(CFLAGS) -Wno-format-truncation -o $@ $<
$(PUBS_TURBO_SHIPPED): $(PUBS_TURBO_GATE) $(TPS_LEX_FPC_BIN) $(TPS_LEX_SRC) seed/lexer.c seed/initechc.c $(PUBS_TURBO_RT)
	@rm -rf $(PUBS_TURBO_DIR)/probe; mkdir -p $(PUBS_TURBO_DIR)/probe
	@$(PUBS_TURBO_GATE) candidates $(TPS_LEX_SRC) seed/lexer.c > $(PUBS_TURBO_DIR)/probe/TPSIN.PAS
	@cd $(PUBS_TURBO_DIR)/probe && $(abspath $(TPS_LEX_FPC_BIN)) > lex.txt
	@$(PUBS_TURBO_GATE) inventory $(TPS_LEX_SRC) $(PUBS_TURBO_DIR)/probe/lex.txt seed/initechc.c $(PUBS_TURBO_RT) > $@.tmp
	@$(PUBS_TURBO_GATE) messages $(TPS_LEX_SRC) seed/rt/fileio.asm > $(PUBS_TURBO_MSGS)
	@mv -f $@.tmp $@
	@printf '>>> pubs-turbo-gen: shipped inventory %s items (%s)\n' "$$(wc -l < $@)" "$@"
$(PUBS_TURBO_KW_TEX): $(PUBS_TURBO_SHIPPED)
	@mkdir -p $(PUBS_GEN)
	@printf '%% Generated from the shipped lexer by pubs/turbo.mk; do not edit.\n\\def\\TPSKeywordList{%s}\n' \
		"$$(sed -n 's/|reserved$$//p' $< | paste -sd, -)" > $@
$(PUBS_TURBO_RR_STAMP): $(PUBS_RAILROAD) $(PUBS_TURBO_GRAMMAR) $(PUBS_TURBO_LEXICAL)
	@mkdir -p $(PUBS_GEN)
	@$(PUBS_RAILROAD) tikz $(PUBS_TURBO_GRAMMAR) $(PUBS_GEN)
	@$(PUBS_RAILROAD) tikz-lexical $(PUBS_TURBO_LEXICAL) $(PUBS_GEN)

.PHONY: pubs-turbo-gen test-pubs-turbo test-pubs-turbo-mutant
pubs-turbo-gen: $(PUBS_TURBO_KW_TEX) $(PUBS_TURBO_RR_STAMP)

ifeq ($(SKIP_PUBS),1)
test-pubs-turbo test-pubs-turbo-mutant:
	@printf '!!! $@ SKIPPED (SKIP_PUBS=1 opt-out) -- the Turbo Initech manuals were NOT checked. This is NOT a pass; unset SKIP_PUBS before trusting this gate.\n'
else
pubs test-pubs-build test-pubs-repro test-pubs-repro-mutant: pubs-turbo-gen
test-pubs: test-pubs-turbo
test-pubs-mutant: test-pubs-turbo-mutant
PUBS_TURBO_GATE_DEPS := test-pubs-build pubs-inventory $(PUBS_TURBO_SHIPPED) $(PUBS_RAILROAD) $(HARNESS_BIN) \
	$(SECONDARY_TRACER_IMG) $(SEED_BIN) $(SEED_RT_OBJ) $(SEED_DOS_RT_OBJ) $(SEED_FILEIO_RT_OBJ)
test-pubs-turbo: $(PUBS_TURBO_GATE_DEPS)
	@printf '>>> test-pubs-turbo: the Turbo Initech manuals against the shipped compiler (initech-ipu5.12)\n'
	@test -n "$(PUBS_TURBO_BOOKS)" || { printf '!!! test-pubs RED TPS-BOOKS: no pubs/manuals/turbo-initech-*/main.tex\n'; exit 1; }
	@$(PUBS_TURBO_GATE) commands $(PUBS_TURBO_SHIPPED) $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.cmds)
	@$(PUBS_TURBO_GATE) msgcheck $(PUBS_TURBO_MSGS) $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.msgs)
	@$(PUBS_TURBO_GATE) mentions $(PUBS_TURBO_SHIPPED) $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.tpsmentions)
	@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
	@sh harness/pubs/turbo_gates_cited.sh $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.gates)
	@sh harness/pubs/turbo_grammar.sh $(PUBS_RAILROAD) $(PUBS_TURBO_GRAMMAR) $(TPS_LEX_FPC_BIN) $(PUBS_TURBO_DIR)/grammar \
		$(PUBS_TURBO_GRAMMAR_N) $(wildcard $(TPS_LEX_FIXTURE_DIR)/gen_*.pas) $(TPS_PARSE_RICH) $(TPS_LEX_SRC) \
		$(foreach f,$(wildcard $(PUBS_DIR)/manuals/turbo-initech-*/examples/*.pas),$(if $(findstring /err-,$(f)),,$(f)))
	@$(PUBS_TURBO_REPLAY_ENV) BUILT=$(PUBS_BUILD)/gate OUT=$(PUBS_TURBO_DIR)/replay sh harness/pubs/turbo_replay.sh
	@printf '>>> test-pubs-turbo: green\n'
test-pubs-turbo-mutant: test-pubs-turbo
	@$(PUBS_TURBO_REPLAY_ENV) BUILT=$(PUBS_BUILD)/gate RAILROAD=$(PUBS_RAILROAD) GRAMMAR=$(PUBS_TURBO_GRAMMAR) \
		SHIPPED=$(PUBS_TURBO_SHIPPED) MSGS=$(PUBS_TURBO_MSGS) OUT=$(PUBS_TURBO_DIR)/mutants \
		PARSE_RICH=$(TPS_PARSE_RICH) FIXTURES=$(TPS_LEX_FIXTURE_DIR) sh harness/pubs/turbo_mutants.sh
endif
