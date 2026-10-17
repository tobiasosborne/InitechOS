# ===========================================================================
# harness/tbx/flair_pascal.mk -- THE TURBO INITECH FLAIR BINDING (beads
# initech-tdnl.22 R5.2, initech-tdnl.23 R5.3). Included from the Makefile
# after TEST_UNIT_GATES / TEST_EMU_GATES exist (it appends to both).
#
# Ref: ADR-0007 DEC-09/DEC-10 as reconciled in
#      docs/design/GUI-remediation-ADR-reconciliation.md Part B (the binding:
#      zero subset growth, hand-assembled INT 81h stubs in the start_dos/
#      fileio lineage, the named event handler in the link-synthesized
#      record; the contract = the ordered trap sequence, graded seed vs fpc
#      host print harness vs Turbo Initech); D2.1-D2.7 of
#      docs/design/GUI-remediation-D1-D2-D3-design.md; spec/toolbox_gate.h.
#
# THE PIECES
#   os/tps/FLAIR.PAS        the binding: the head of every FLAIR program
#   seed/rt/flairstub.asm   the stubs + entry + record + event buffer
#   tools/tbxgate_inc.c     spec/toolbox_gate.h -> build/flairgate/
#                           flairgate.inc (the stubs' every number; Rule 8)
#   harness/tbx/            the host oracles: tbxmodel.c (the gate model),
#                           tbx32_rt.c + tbxhost32.c (a REAL InitechMZ tenant
#                           run in an i386 process, its real int 0x81 caught),
#                           test_flairstub.c (the per-verb marshalling
#                           oracle), tbxhost64.c (the fpc print harness),
#                           pascal_tenant.sh / pascal_fpc.sh (the rails).
#
# GATES (host, TEST_UNIT_GATES)
#   test-flairstub          every stub vs the header: AX, arity, block,
#                           strings, menu graph, record, trampoline,
#                           callee-saved registers, local refusals; every
#                           FLAIR.PAS constant vs the headers.
#   test-flairstub-mutant   Rule 6: one FLAIRSTUB_MUT_* per verb class, each
#                           RED on its own class and on no other.
#   test-flairstub-smoke    SMOKETPS (FLAIR.PAS + os/tps/SMOKE.PAS, every
#                           binding) built by the SEED and by TURBO INITECH
#                           into real .EXEs, run under tbxhost32, and by FPC
#                           under the print harness: all three traces ==
#                           harness/tbx/smoke.golden (hand-checked against
#                           SMOKE.PAS, line by line).
#   test-flairstub-smoke-mutant  SMOKE_MUT_SKIP_CLICK (the D2.7 mutant): the
#                           click is ignored -> the trace leaves the golden.
# ===========================================================================
FLAIR_PAS_SRC       := os/tps/FLAIR.PAS
FLAIRSTUB_ASM       := seed/rt/flairstub.asm
FLAIRGATE_DIR       := $(BUILD)/flairgate
TBXGATE_INC_BIN     := $(FLAIRGATE_DIR)/tbxgate_inc
FLAIRGATE_INC       := $(FLAIRGATE_DIR)/flairgate.inc
FLAIRSTUB_OBJ       := $(FLAIRGATE_DIR)/flairstub.o
FLAIRSTUB_MUTANTS   := LIFECYCLE WINDOW DRAW MENU ALERT SCRAP EVENT
TBX_HARNESS_DIR     := harness/tbx
TBX32_CFLAGS        := -m32 -std=c11 -Wall -Wextra -Werror -O2 -ffreestanding -fno-builtin \
                       -nostdlib -static -fno-pie -no-pie -fno-stack-protector \
                       -fno-asynchronous-unwind-tables -z noexecstack -Ispec -I$(TBX_HARNESS_DIR)
TBX32_RT_SRC        := $(TBX_HARNESS_DIR)/tbx32_rt.c $(TBX_HARNESS_DIR)/tbxmodel.c
TBX32_HDRS          := $(TBX_HARNESS_DIR)/tbx32_rt.h $(TBX_HARNESS_DIR)/tbxmodel.h spec/toolbox_gate.h \
                       spec/cp_dialog.h spec/event_model.h
TBXHOST32_BIN       := $(FLAIRGATE_DIR)/tbxhost32
TEST_FLAIRSTUB_BIN  := $(FLAIRGATE_DIR)/test_flairstub
TBX64_CFLAGS        := -std=c11 -Wall -Wextra -Werror -O1 -ffreestanding -fno-builtin \
                       -fno-stack-protector -fno-pie -Ispec -I$(TBX_HARNESS_DIR)
TBXHOST64_OBJS      := $(FLAIRGATE_DIR)/tbxhost64.o $(FLAIRGATE_DIR)/tbxmodel64.o
PASCAL_TENANT_SH    := $(TBX_HARNESS_DIR)/pascal_tenant.sh
PASCAL_FPC_SH       := $(TBX_HARNESS_DIR)/pascal_fpc.sh
PASCAL_TENANT_ENV    = SEED_BIN=$(abspath $(SEED_BIN)) TPS_BIN=$(abspath $(TPS_LEX_FPC_BIN)) \
                       NASM=$(NASM) LD=$(LD) OBJCOPY=$(OBJCOPY) NM=$(NM) \
                       MZLINK=$(abspath $(MZLINK_BIN)) TENANT_LD=$(abspath $(TENANT_LD)) \
                       TENANT_PROBE=$(TENANT_PROBE) FLAIRSTUB_OBJ=$(abspath $(FLAIRSTUB_OBJ))
PASCAL_TENANT_DEPS  := $(PASCAL_TENANT_SH) $(SEED_BIN) $(TPS_LEX_FPC_BIN) $(MZLINK_BIN) $(TENANT_LD) \
                       $(FLAIRSTUB_OBJ)
PASCAL_FPC_ENV       = FPC=fpc TBXHOST64_OBJS="$(abspath $(TBXHOST64_OBJS))"

$(FLAIRGATE_DIR): | $(BUILD)
	@mkdir -p $@

$(TBXGATE_INC_BIN): tools/tbxgate_inc.c spec/toolbox_gate.h spec/cp_dialog.h | $(FLAIRGATE_DIR)
	$(CC) $(CFLAGS) -Ispec -o $@ $<
$(FLAIRGATE_INC): $(TBXGATE_INC_BIN)
	$< > $@.tmp && mv -f $@.tmp $@
$(FLAIRSTUB_OBJ): $(FLAIRSTUB_ASM) $(FLAIRGATE_INC)
	$(NASM) -f elf32 -I$(FLAIRGATE_DIR)/ $< -o $@
$(FLAIRGATE_DIR)/flairstub_mut_%.o: $(FLAIRSTUB_ASM) $(FLAIRGATE_INC)
	$(NASM) -f elf32 -I$(FLAIRGATE_DIR)/ -DFLAIRSTUB_MUT_$* $< -o $@

$(TBXHOST32_BIN): $(TBX_HARNESS_DIR)/tbxhost32.c $(TBX32_RT_SRC) $(TBX32_HDRS) | $(FLAIRGATE_DIR)
	$(CC) $(TBX32_CFLAGS) -o $@ $(TBX_HARNESS_DIR)/tbxhost32.c $(TBX32_RT_SRC)
# The direct oracle links the stub into its own image: __image_start is the
# executable's start and __image_len a sentinel the record must carry.
TEST_FLAIRSTUB_LINK := -Wl,--defsym=__image_start=__executable_start -Wl,--defsym=__image_len=0x13572468
$(TEST_FLAIRSTUB_BIN): $(TBX_HARNESS_DIR)/test_flairstub.c $(TBX32_RT_SRC) $(TBX32_HDRS) $(FLAIRSTUB_OBJ)
	$(CC) $(TBX32_CFLAGS) $(TEST_FLAIRSTUB_LINK) -o $@ $< $(TBX32_RT_SRC) $(FLAIRSTUB_OBJ)
$(FLAIRGATE_DIR)/test_flairstub_mut_%: $(TBX_HARNESS_DIR)/test_flairstub.c $(TBX32_RT_SRC) $(TBX32_HDRS) $(FLAIRGATE_DIR)/flairstub_mut_%.o
	$(CC) $(TBX32_CFLAGS) $(TEST_FLAIRSTUB_LINK) -o $@ $< $(TBX32_RT_SRC) $(FLAIRGATE_DIR)/flairstub_mut_$*.o
$(FLAIRGATE_DIR)/tbxhost64.o: $(TBX_HARNESS_DIR)/tbxhost64.c $(TBX32_HDRS) | $(FLAIRGATE_DIR)
	$(CC) $(TBX64_CFLAGS) -c $< -o $@
$(FLAIRGATE_DIR)/tbxmodel64.o: $(TBX_HARNESS_DIR)/tbxmodel.c $(TBX32_HDRS) | $(FLAIRGATE_DIR)
	$(CC) $(TBX64_CFLAGS) -c $< -o $@

.PHONY: test-flairstub test-flairstub-mutant
test-flairstub: $(TEST_FLAIRSTUB_BIN)
	@$< $(FLAIR_PAS_SRC) > $(FLAIRGATE_DIR)/test_flairstub.log; rc=$$?; \
	 grep -c '^PASS' $(FLAIRGATE_DIR)/test_flairstub.log | sed 's/^/>>> test-flairstub: PASS lines /'; \
	 grep '^FAIL' $(FLAIRGATE_DIR)/test_flairstub.log; \
	 [ $$rc -eq 0 ] && ! grep -q '^FAIL' $(FLAIRGATE_DIR)/test_flairstub.log \
	 || { printf '!!! test-flairstub RED (log %s)\n' '$(FLAIRGATE_DIR)/test_flairstub.log'; exit 1; }
	@printf '>>> test-flairstub: green (every FLAIR.PAS stub vs spec/toolbox_gate.h: AX, arity, block, strings, menu graph, record, trampoline, registers, refusals; every constant)\n'
test-flairstub-mutant: test-flairstub $(foreach m,$(FLAIRSTUB_MUTANTS),$(FLAIRGATE_DIR)/test_flairstub_mut_$(m))
	@for m in $(FLAIRSTUB_MUTANTS); do \
	   log=$(FLAIRGATE_DIR)/test_flairstub_mut_$$m.log; \
	   if $(FLAIRGATE_DIR)/test_flairstub_mut_$$m $(FLAIR_PAS_SRC) > $$log 2>&1; then \
	     printf '!!! test-flairstub-mutant: FLAIRSTUB_MUT_%s survived -- decoration\n' "$$m"; exit 1; fi; \
	   grep -q "^FAIL $$m " $$log || { cat $$log; printf '!!! FLAIRSTUB_MUT_%s RED, but not on its class\n' "$$m"; exit 1; }; \
	   other=$$(grep '^FAIL' $$log | awk '{print $$2}' | sort -u | grep -vx "$$m" || true); \
	   [ -z "$$other" ] || { printf '!!! FLAIRSTUB_MUT_%s also RED on %s (not a one-class mutant)\n' "$$m" "$$other"; exit 1; }; \
	   printf '>>> test-flairstub-mutant: FLAIRSTUB_MUT_%s correctly RED (%s)\n' "$$m" "$$(grep -m1 "^FAIL $$m " $$log)"; \
	 done
	@printf '>>> test-flairstub-mutant: green (seven verb classes, each mutant RED on its own class alone)\n'
TEST_UNIT_GATES += test-flairstub test-flairstub-mutant

# ---- SMOKETPS: the three-way trace differential (D2.7) ----------------------
SMOKE_PAS_SRC    := os/tps/SMOKE.PAS
SMOKE_EVENTS     := $(TBX_HARNESS_DIR)/smoke.events
SMOKE_GOLDEN     := $(TBX_HARNESS_DIR)/smoke.golden
SMOKE_DIR        := $(BUILD)/pascal/smoke
SMOKE_TPSIN      := $(SMOKE_DIR)/TPSIN.PAS
SMOKE_SEED_EXE   := $(SMOKE_DIR)/seed/SMOKETPS.EXE
SMOKE_TPS_EXE    := $(SMOKE_DIR)/tps/SMOKETPS.EXE
SMOKE_MUT_TPSIN  := $(SMOKE_DIR)/mut/TPSIN.PAS
SMOKE_MUT_EXE    := $(SMOKE_DIR)/mut/SMOKETPS.EXE

# COPY /B FLAIR.PAS+SMOKE.PAS TPSIN.PAS, on the host
$(SMOKE_TPSIN): $(FLAIR_PAS_SRC) $(SMOKE_PAS_SRC)
	@mkdir -p $(@D)
	cat $(FLAIR_PAS_SRC) $(SMOKE_PAS_SRC) > $@
$(SMOKE_SEED_EXE): $(SMOKE_TPSIN) $(PASCAL_TENANT_DEPS)
	$(PASCAL_TENANT_ENV) sh $(PASCAL_TENANT_SH) seed $< $@ $(SMOKE_DIR)/seed
$(SMOKE_TPS_EXE): $(SMOKE_TPSIN) $(PASCAL_TENANT_DEPS)
	$(PASCAL_TENANT_ENV) sh $(PASCAL_TENANT_SH) tps $(abspath $<) $(abspath $@) $(abspath $(SMOKE_DIR)/tps)
# SMOKE_MUT_SKIP_CLICK: the anchored click count is dropped (D2.7 mutant)
$(SMOKE_MUT_TPSIN): $(SMOKE_TPSIN)
	@mkdir -p $(@D)
	@test "$$(grep -Fc '  Clicks := Clicks + 1 { SMOKE_MUT_SKIP_CLICK }' $<)" -eq 1 \
		|| { printf '!!! SMOKE_MUT_SKIP_CLICK anchor must match exactly once\n'; exit 1; }
	sed 's@^  Clicks := Clicks + 1 { SMOKE_MUT_SKIP_CLICK }$$@  Clicks := Clicks { SMOKE_MUT_SKIP_CLICK }@' $< > $@
	@! cmp -s $< $@ || { printf '!!! SMOKE_MUT_SKIP_CLICK sed was a no-op\n'; exit 1; }
$(SMOKE_MUT_EXE): $(SMOKE_MUT_TPSIN) $(PASCAL_TENANT_DEPS)
	$(PASCAL_TENANT_ENV) sh $(PASCAL_TENANT_SH) seed $< $@ $(SMOKE_DIR)/mut

.PHONY: test-flairstub-smoke test-flairstub-smoke-mutant
test-flairstub-smoke: $(SMOKE_SEED_EXE) $(SMOKE_TPS_EXE) $(TBXHOST32_BIN) $(TBXHOST64_OBJS) $(SMOKE_EVENTS) $(SMOKE_GOLDEN)
	@command -v fpc >/dev/null 2>&1 || { printf '!!! test-flairstub-smoke FAIL (loud, Law 2): fpc is not installed -- the third rail cannot run\n'; exit 1; }
	@$(TBXHOST32_BIN) $(SMOKE_SEED_EXE) $(SMOKE_EVENTS) > $(SMOKE_DIR)/seed.trace
	@$(TBXHOST32_BIN) $(SMOKE_TPS_EXE) $(SMOKE_EVENTS) > $(SMOKE_DIR)/tps.trace
	@$(PASCAL_FPC_ENV) sh $(PASCAL_FPC_SH) $(SMOKE_TPSIN) $(SMOKE_DIR)/fpc $(abspath $(SMOKE_EVENTS)) $(SMOKE_DIR)/fpc.trace
	@for r in seed tps fpc; do \
	   diff -u $(SMOKE_GOLDEN) $(SMOKE_DIR)/$$r.trace > $(SMOKE_DIR)/$$r.diff \
	   || { cat $(SMOKE_DIR)/$$r.diff; printf '!!! test-flairstub-smoke FAIL: the %s rail trace differs from smoke.golden\n' "$$r"; exit 1; }; \
	   printf '>>> test-flairstub-smoke: %s rail == smoke.golden (%s lines)\n' "$$r" "$$(wc -l < $(SMOKE_DIR)/$$r.trace)"; \
	 done
	@printf '>>> test-flairstub-smoke: green (seed == Turbo Initech == fpc print harness == the hand-checked golden; every binding, both directions)\n'
test-flairstub-smoke-mutant: test-flairstub-smoke $(SMOKE_MUT_EXE)
	@$(TBXHOST32_BIN) $(SMOKE_MUT_EXE) $(SMOKE_EVENTS) > $(SMOKE_DIR)/mut.trace
	@if cmp -s $(SMOKE_GOLDEN) $(SMOKE_DIR)/mut.trace; then \
	   printf '!!! test-flairstub-smoke-mutant: SMOKE_MUT_SKIP_CLICK matched the golden -- decoration\n'; exit 1; fi
	@diff $(SMOKE_GOLDEN) $(SMOKE_DIR)/mut.trace | grep -q '^> TBX DRAWCELLS win=1 x=8 y=80 s="1" ' \
	 || { diff $(SMOKE_GOLDEN) $(SMOKE_DIR)/mut.trace; printf '!!! SMOKE_MUT_SKIP_CLICK RED for the wrong reason\n'; exit 1; }
	@printf '>>> test-flairstub-smoke-mutant: SMOKE_MUT_SKIP_CLICK correctly RED (the repaint counts 1, not 101)\n'
TEST_UNIT_GATES += test-flairstub-smoke test-flairstub-smoke-mutant
