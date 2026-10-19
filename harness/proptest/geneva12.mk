# Geneva 12 strike gates (bead initech-uelk): the SimpleText document face.
# Ref: PRD Sec 6.4; ../system7-decomp specs/sys8/simpletext-stdfile.md Sec 1.2
# [MEAS] (Geneva 12, 16-px line); spec/assets/geneva12.art (provenance: real
# Mac OS 8.1 NFNT 12112 METRICS, hand-authored GLYPH ART).
#   make gen-geneva12            regenerate spec/assets/geneva12.h from the .art
#   make test-geneva12-art       the committed header == a fresh transcription
#   make test-geneva12-art-mutant  (a) one flipped pixel changes the header;
#                                (b) art whose ink disagrees with lb is refused
#   make test-geneva12-pack      every kernel-decoded row == the locked strike
#   make test-geneva12-pack-mutant GENEVA12_PACK_MUT_ROW RED
#   make test-geneva12-metrics   advances / ink boxes / vertical metrics vs the
#                                REAL NFNT 12112 read at run time (independent)
#   make test-geneva12-metrics-mutant  a flipped art pixel in 'S' row 2 RED
GENEVA12_ART      := spec/assets/geneva12.art
GENEVA12_H        := spec/assets/geneva12.h
GENEVA12_ART2H    := $(BUILD)/geneva12_art2h
GENEVA12_DIR      := $(BUILD)/geneva12
$(GENEVA12_DIR): | $(BUILD)
	@mkdir -p $@
$(GENEVA12_ART2H): tools/geneva12_art2h.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: gen-geneva12 test-geneva12-art test-geneva12-art-mutant
gen-geneva12: $(GENEVA12_ART2H) $(GENEVA12_ART)
	$(GENEVA12_ART2H) $(GENEVA12_ART) > $(BUILD)/geneva12.gen.h
	mv -f $(BUILD)/geneva12.gen.h $(GENEVA12_H)
test-geneva12-art: $(GENEVA12_ART2H) $(GENEVA12_ART) $(GENEVA12_H) | $(GENEVA12_DIR)
	@$(GENEVA12_ART2H) $(GENEVA12_ART) > $(GENEVA12_DIR)/regen.h
	@cmp -s $(GENEVA12_DIR)/regen.h $(GENEVA12_H) \
		|| { printf '!!! test-geneva12-art FAIL: $(GENEVA12_H) is not the transcription of $(GENEVA12_ART) (run make gen-geneva12)\n'; exit 1; }
	@printf '>>> test-geneva12-art: green -- $(GENEVA12_H) == transcribe($(GENEVA12_ART))\n'
# (a) 'S' row 2 (blank, one row above the cap line) gains one ink pixel at
#     lb: the header must change.
# (b) the whole of 'S' shifted one column right -- leftmost ink != lb: the
#     transcriber must REFUSE it (fail loud).
test-geneva12-art-mutant: $(GENEVA12_ART2H) $(GENEVA12_ART) $(GENEVA12_H) | $(GENEVA12_DIR)
	@awk '/^glyph 0x53 /{g=1;r=0;print;next} /^glyph /{g=0} g&&!/^;/{if(r==2){$$0="." "#" substr($$0,3)} r++} {print}' $(GENEVA12_ART) > $(GENEVA12_DIR)/mut1.art
	@cmp -s $(GENEVA12_DIR)/mut1.art $(GENEVA12_ART) && { printf '!!! test-geneva12-art-mutant FAIL: the pixel flip did not apply\n'; exit 1; } || true
	@$(GENEVA12_ART2H) $(GENEVA12_DIR)/mut1.art > $(GENEVA12_DIR)/mut1.h
	@if cmp -s $(GENEVA12_DIR)/mut1.h $(GENEVA12_H); then printf '!!! test-geneva12-art-mutant FAIL: a flipped pixel left the header unchanged -- the sync check is decoration\n'; exit 1; fi
	@printf '>>> test-geneva12-art-mutant: (a) green -- one flipped pixel in S makes the header differ\n'
	@awk '/^glyph 0x53 /{g=1;r=0;print;next} /^glyph /{g=0} g&&!/^;/{$$0="." substr($$0,1,length($$0)-1); r++} {print}' $(GENEVA12_ART) > $(GENEVA12_DIR)/mut2.art
	@if $(GENEVA12_ART2H) $(GENEVA12_DIR)/mut2.art > $(GENEVA12_DIR)/mut2.h 2> $(GENEVA12_DIR)/mut2.log; then printf '!!! test-geneva12-art-mutant FAIL: art shifted off its NFNT bearing was accepted\n'; exit 1; fi
	@grep -q 'leftmost ink column != lb' $(GENEVA12_DIR)/mut2.log || { cat $(GENEVA12_DIR)/mut2.log; printf '!!! test-geneva12-art-mutant FAIL: refused for the wrong reason\n'; exit 1; }
	@printf '>>> test-geneva12-art-mutant: (b) green -- art whose ink disagrees with the NFNT lb is refused (%s)\n' "$$(cat $(GENEVA12_DIR)/mut2.log)"

# Kernel storage: the Chicago packer, -DPACK_GENEVA12 (same algorithm).
$(BUILD)/geneva12_pack: tools/chicago_pack.c $(GENEVA12_H) spec/assets/font_linkage.h | $(BUILD)
	$(CC) $(CFLAGS) -DPACK_GENEVA12 -Ispec/assets -o $@ $<
$(GENEVA12_PACK_H): $(BUILD)/geneva12_pack $(GENEVA12_H)
	@$< >$@.tmp && mv -f $@.tmp $@
.PHONY: test-geneva12-pack test-geneva12-pack-mutant
test-geneva12-pack: $(GENEVA12_PACK_H) | $(GENEVA12_DIR)
	$(CC) $(CFLAGS) -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_geneva12_pack.c -o $(GENEVA12_DIR)/pack
	@$(GENEVA12_DIR)/pack
test-geneva12-pack-mutant: test-geneva12-pack
	$(CC) $(CFLAGS) -DGENEVA12_PACK_MUT_ROW -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_geneva12_pack.c -o $(GENEVA12_DIR)/pack-mut
	@if $(GENEVA12_DIR)/pack-mut >$(GENEVA12_DIR)/pack-mut.log 2>&1; then \
		printf '!!! test-geneva12-pack-mutant FAIL: packed_row survived\n'; exit 1; fi
	@grep '^!!! geneva12-pack FAIL:' $(GENEVA12_DIR)/pack-mut.log
	@printf '>>> test-geneva12-pack-mutant: green (GENEVA12_PACK_MUT_ROW RED)\n'

# The independent metrics oracle: the REAL NFNT 12112 at run time.
GENEVA12_NFNT := $(SYSTEM7_DECOMP)/mint/work/geneva12/NFNT_12112.bin
G12MET_DEF = -DSYSTEM7_DECOMP=\"$(SYSTEM7_DECOMP)\"
G12MET_DEPS := harness/proptest/test_geneva12_metrics.c os/flair/text.c $(FLAIR_FONT_HDRS) os/flair/surface.c os/flair/surface.h $(ORACLE_POLICY_H) $(CORPUS_CONFIG)
$(GENEVA12_DIR)/metrics: $(G12MET_DEPS) | $(GENEVA12_DIR)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(TEXT_INC) $(G12MET_DEF) -o $@ harness/proptest/test_geneva12_metrics.c os/flair/text.c os/flair/surface.c
.PHONY: test-geneva12-metrics test-geneva12-metrics-mutant
test-geneva12-metrics: $(GENEVA12_DIR)/metrics
	@$<
test-geneva12-metrics-mutant: test-geneva12-metrics test-geneva12-art-mutant $(G12MET_DEPS)
	@mkdir -p $(GENEVA12_DIR)/mutinc && cp -f $(GENEVA12_DIR)/mut1.h $(GENEVA12_DIR)/mutinc/geneva12.h
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -I$(GENEVA12_DIR)/mutinc $(TEXT_INC) $(G12MET_DEF) -o $(GENEVA12_DIR)/metrics-mut harness/proptest/test_geneva12_metrics.c os/flair/text.c os/flair/surface.c
	@if $(GENEVA12_DIR)/metrics-mut > $(GENEVA12_DIR)/metrics-mut.log 2>&1; then printf '!!! test-geneva12-metrics-mutant FAIL: a flipped S pixel PASSED -- the ink-box oracle is decoration\n'; exit 1; fi
	@grep -q "C ink box 'S' (0x53): drawn x 1..6 y 2..11, NFNT 12112 x 1..6 y 3..11" $(GENEVA12_DIR)/metrics-mut.log \
		|| { tail -5 $(GENEVA12_DIR)/metrics-mut.log; printf '!!! test-geneva12-metrics-mutant FAIL: RED, but not for the flipped S pixel\n'; exit 1; }
	@printf '>>> test-geneva12-metrics-mutant: green (S row-2 pixel RED: %s)\n' "$$(grep -m1 "ink box 'S'" $(GENEVA12_DIR)/metrics-mut.log | sed 's/^ *//')"

TEST_UNIT_GATES += test-geneva12-art test-geneva12-art-mutant test-geneva12-pack test-geneva12-pack-mutant test-geneva12-metrics test-geneva12-metrics-mutant
