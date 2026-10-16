# Ref: PRD Sec 8; CLAUDE.md Rule 2/6; initech-tflp.
# Explicit contracts for seeded data-volume recipes. Included LAST so this
# list cannot cause a read-before-definition (test-makefile-vars, tdnl.81).
# Default mcopy flags are A; mtools lists read-only as A R and hidden as A H.
FLAIR_FIXTURES := README.TXT APPS/TENANTFX.EXE APPS/CTENANT.EXE APPS/123.EXE APPS/WORD.EXE APPS/123.MNU APPS/CHKDSK.COM APPS/ATTRIB.COM APPS/LABEL.COM APPS/XCOPY.COM APPS/TREE.COM APPS/CALC.EXE APPS/EDLIN.COM APPS/TEXT.EXE APPS/PAINT.EXE APPS/LEDGER.TXT APPS/MICHAEL.CFG APPS/TPSREP.COM APPS/TPSPARM.DAT APPS/TPS.DBF APPS/CALCULAT.EXE
FIXTURE_VOLUME_NAMES :=
define fixture-contract
FIXTURE_VOLUME_NAMES += $(1)
FIXTURE_CONTENTS_$(1) := $(2)
$(1): harness/verify_fixture_volume.sh harness/fixture_volumes.mk
endef
$(foreach img,$(FAT12_IMG),$(eval $(call fixture-contract,$(img),HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN BIGCHAIN.TXT)))
$(eval $(call fixture-contract,$(FAT16_IMG),HELLO.TXT CHAIN.TXT BLOCK.BIN BIGCHAIN.TXT HIGHCLUS.BIN))
$(eval $(call fixture-contract,$(FAT12_NESTED_IMG),HELLO.TXT SUB/NESTED.TXT SUB/DEEP/DEEP.TXT $(foreach n,01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40,BIGDIR/FILE$(n).TXT)))
$(eval $(call fixture-contract,$(FAT12_RENAME_GOLDEN_IMG),NEW.BAK))
$(eval $(call fixture-contract,$(FAT12_RENAME_SUBDIR_GOLDEN_IMG),SUB/NEW2.BAK))
$(eval $(call fixture-contract,$(FAT_DATA_IMG),HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN))
$(eval $(call fixture-contract,$(FAT_EXEC_IMG),HELLO.TXT GREET.COM))
$(eval $(call fixture-contract,$(FAT_EXITH_IMG),HELLO.TXT SECOND.TXT CHAIN.TXT BLOCK.BIN EXITH.COM))
$(foreach img,$(FAT_SYSI_IMG) $(FAT_SYSI_BIG_IMG),$(eval $(call fixture-contract,$(img),CONFIG.SYS HELLO.TXT SYSI.COM)))
$(eval $(call fixture-contract,$(FAT_WRITE_IMG),HELLO.TXT))
$(eval $(call fixture-contract,$(FAT_MULTIOPEN_IMG),HELLO.TXT SECOND.TXT BIG.DAT))
$(eval $(call fixture-contract,$(FAT_IRQSTORM_IMG),ALPHA.TXT BRAVO.TXT CHARLIE.TXT DELTA.TXT STORM.DAT))
$(foreach img,$(FAT_MZEXEC_IMG) $(FAT_MZEXEC_NORELOC_IMG),$(eval $(call fixture-contract,$(img),HELLO.TXT MZEXEC.EXE)))
$(eval $(call fixture-contract,$(FAT_MZFOREIGN_IMG),HELLO.TXT FOREIGN.EXE))
# Ref: bead initech-a5sr. The FLAIR data-volume contract set is DERIVED, not
# hand-listed: a hand list let three InitechWord mutant volumes merge with no
# contract (6865fc0 / 8080833), caught only at emu-gate time. Every
# FLAIR_DATA_<X>_IMG variable defined when this file is included (it is
# included LAST) gets the stock contract, except:
#   FLAIR_DATA_CORRUPT_IMG      own contract below (DESKTOP.DB=AH)
#   FLAIR_DATA_RUNTIME_IMG      scratch copy of test-flair-data-volume
#   FLAIR_DATA_MUT_RUNTIME_IMG  scratch copy of test-flair-data-volume-mutant
# FLAIR_DATA_IMG does not match the pattern, and flair_i123_menu_stale.img is
# a flair-data recipe image with no variable: both stay explicit entries.
# FLAIR_DAMAGED_DATA_IMG / FLAIR_PURGE_VIEWS_DATA / FLAIR_SCROLL_DATA_IMG do not
# match the pattern either; they keep their own contracts below.
FLAIR_DATA_SPECIAL_VARS := FLAIR_DATA_CORRUPT_IMG
FLAIR_DATA_SCRATCH_VARS := FLAIR_DATA_RUNTIME_IMG FLAIR_DATA_MUT_RUNTIME_IMG
FLAIR_DATA_VARIANT_VARS := $(sort $(filter-out $(FLAIR_DATA_SPECIAL_VARS) $(FLAIR_DATA_SCRATCH_VARS),$(filter FLAIR_DATA_%_IMG,$(.VARIABLES))))
FLAIR_FIXTURE_IMAGES := $(FLAIR_DATA_IMG) $(foreach v,$(FLAIR_DATA_VARIANT_VARS),$($(v))) $(BUILD)/flair_i123_menu_stale.img
ifdef FIXTURE_MUT_HANDLIST
# Mutant (test-fixture-volumes-mutant, HANDLIST_DRIFT): the pre-a5sr hand list
# as it stood before 8080833, i.e. missing the no-block-on InitechWord volume.
FLAIR_FIXTURE_IMAGES := $(FLAIR_DATA_IMG) $(FLAIR_DATA_TFX_NOREG_IMG) $(FLAIR_DATA_TFX_CRASH_IMG) $(FLAIR_DATA_TFX_BADMBAR_IMG) $(FLAIR_DATA_I123_ARROWS_IMG) $(FLAIR_DATA_I123_NORECALC_IMG) $(FLAIR_DATA_IWORD_NOWRAP_IMG) $(FLAIR_DATA_IWORD_NOSAVE_IMG) $(FLAIR_DATA_FONTS_IMG) $(BUILD)/flair_i123_menu_stale.img $(FLAIR_DATA_IWORD_NOFACE_IMG) $(FLAIR_DATA_IWORD_NOD4_IMG)
endif
$(foreach img,$(FLAIR_FIXTURE_IMAGES),$(eval $(call fixture-contract,$(img),$(FLAIR_FIXTURES))))
ifdef FIXTURE_MUT_UNDECLARED_VARIANT
# Mutant (test-fixture-volumes-mutant, UNDECLARED_VARIANT): a flair-data
# variant volume defined after the derivation above, so it has no contract.
FLAIR_DATA_ZZZ_IMG := $(BUILD)/flair_data_zzz.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_ZZZ_IMG),$(TENANTFX_EXE),$(I123_EXE),$(IWORD_EXE),))
endif
$(eval $(call fixture-contract,$(FLAIR_DATA_CORRUPT_IMG),$(FLAIR_FIXTURES) DESKTOP.DB=AH))
$(eval $(call fixture-contract,$(FLAIR_DAMAGED_DATA_IMG),NEWFOLD= TRASH= APPS/TRASH= APPS/TRASH/README.TXT $(filter-out README.TXT,$(FLAIR_FIXTURES))))
$(eval $(call fixture-contract,$(FLAIR_PURGE_VIEWS_DATA),$(FLAIR_FIXTURES) TRASH/A/KEEP.TXT TRASH/A/CHILD/GRAND/NOTE.TXT))
$(eval $(call fixture-contract,$(FLAIR_SCROLL_DATA_IMG),$(FLAIR_FIXTURES) $(foreach n,00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19,F$(n).TXT)))
$(eval $(call fixture-contract,$(BUILD)/record_locked_trash.img,$(filter-out README.TXT,$(FLAIR_FIXTURES)) README.TXT=AR))
$(eval $(call fixture-contract,$(BUILD)/record_locked_purge.img,$(filter-out APPS/TENANTFX.EXE,$(FLAIR_FIXTURES)) APPS/TENANTFX.EXE=AR))
$(foreach img,$(SEED_FILEIO_IMG) $(SEED_FILEIO_MUT_SHORT_IMG) $(SEED_FILEIO_MUT_WRONG_IMG),$(eval $(call fixture-contract,$(img),FILEIO.COM)))
$(foreach img,$(TPS_LEX_IMG) $(TPS_PARSE_IMG) $(TPS_TYPE_IMG) $(TPS_GEN_IMG),$(eval $(call fixture-contract,$(img),TPS.COM TPSIN.PAS)))
$(eval $(call fixture-contract,$(TPS_GEN_MUT_IMG),MTINY.COM))
$(eval $(call fixture-contract,$(TPS_COMPILER_IMG),AUTOEXEC.BAT $(foreach pair,$(TPS_COMPILER_CORPUS),$(word 2,$(subst :, ,$(pair))).COM)))
$(eval $(call fixture-contract,$(TPS_GEN_MUT_VAR_IMG),MVARPAR.COM))
$(eval $(call fixture-contract,$(HARNESS_QA_IMG),$(FIXTURE_CONTENTS_$(TPS_COMPILER_IMG))))
$(foreach img,$(SAMIR_LIST_IMG) $(SAMIR_LIST_MUT_IMG),$(eval $(call fixture-contract,$(img),SAMIR.COM CLIENTS.DBF)))
$(eval $(call fixture-contract,$(SAMIR_REPORT_IMG),SAMIR.COM $(SAMIR_REPORT_FILES)))
$(eval $(call fixture-contract,$(FLAIR_FILE_DATA),$(FLAIR_FIXTURES) KEEP.DAT))
$(eval $(call fixture-contract,$(FLAIR_I123_FILE_DATA),$(FLAIR_FIXTURES) MINT01.WK1 BAD.WK1))
$(eval $(call fixture-contract,$(FLAIR_I123_QUIT_DATA),$(FIXTURE_CONTENTS_$(FLAIR_I123_FILE_DATA)) RO.WK1=AR))
$(eval $(call fixture-contract,$(BUILD)/flair_i123_file_mut_%.img,$(FIXTURE_CONTENTS_$(FLAIR_I123_FILE_DATA))))

# These volumes are minted inline on every gate run, rather than by an image
# target. Protect their declared names too, without inventing build recipes.
define fixture-inline-contract
FIXTURE_VOLUME_NAMES += $(1)
FIXTURE_CONTENTS_$(1) := $(2)
endef
$(eval $(call fixture-inline-contract,$(FAT12_RENAME_ART_IMG),OLD.TXT))
$(eval $(call fixture-inline-contract,$(FAT12_RENAME_SUBDIR_ART_IMG),SUB/OLD2.TXT))
$(eval $(call fixture-inline-contract,$(FAT12_MOVE_DIFF_IMG),MOVEME.TXT SRC/CROSS.TXT SRC/SUBFILE.TXT SRC/REPORT.DBF TRASH/REPORT.DBF SRC/MOVEDIR/INSIDE.TXT COLL.TXT DST/COLL.TXT))
$(eval $(call fixture-inline-contract,$(FAT12_MOVE_STRUCT_IMG),FILEA.TXT KEEPA.TXT LONGBASE.DAT GROWA.TXT GROWB.TXT GROWC.TXT HOLDER/KEEP.TXT))
$(eval $(call fixture-inline-contract,$(OJXN_IMG),TEST.TXT))
$(foreach img,$(UT6D_IMG) $(UT6D_MUT_IMG) $(UT6D_RDNOOP_IMG),$(eval $(call fixture-inline-contract,$(img),HELLO.TXT)))
$(foreach img,$(ZS24EXEC_IMG) $(ZS24EXEC_REJECT_IMG) $(ZS24EXEC_ROOTONLY_IMG),$(eval $(call fixture-inline-contract,$(img),HELLO.TXT SUB/GREET.COM)))
$(foreach img,$(SAMIR_WRITE_IMG) $(SAMIR_WRITE_SCRN_IMG) $(SAMIR_WRITE_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),SAMIR.COM CLIENTS.DBF)))
$(foreach img,$(CY2K_IMG) $(CY2K_SCRN_IMG) $(CY2K_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),SAMIR.COM INVOICE.DBF Y2KACCT.PRG)))
$(foreach img,$(CSAL_IMG) $(CSAL_SCRN_IMG) $(CSAL_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),SAMIR.COM INVOICE.DBF SALAMI.PRG)))
$(foreach img,$(AUTOEXEC_IMG) $(AUTOEXEC_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT SUB.BAT GREET.COM)))
$(foreach img,$(HSCT_REDIR_IMG) $(HSCT_REDIR_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT)))
$(foreach img,$(BSY9_REDIR_IMG) $(BSY9_REDIR_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT GREET.COM)))
$(foreach img,$(BSY7_REDIR_IMG) $(BSY7_REDIR_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT GOBBLE.COM GIN.TXT)))
$(foreach img,$(BSY8_PIPE_IMG) $(BSY8_PIPE_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT GREET.COM GOBBLE.COM)))
$(foreach img,$(SORT_IMG) $(SORT_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT SORT.COM SIN.TXT)))
$(foreach img,$(FIND_IMG) $(FIND_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT FIND.COM FIN.TXT)))
$(foreach img,$(MORE_IMG) $(MORE_MUT_IMG),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT MORE.COM MIN.TXT)))
$(foreach img,$(FCBX_IMG) $(foreach m,$(FCB_MUTANTS),$(BUILD)/fcbx_data_$(m).img),$(eval $(call fixture-inline-contract,$(img),AUTOEXEC.BAT FCBX.COM)))
$(foreach img,$(TPS_IMG_A) $(BUILD)/tps_mut6.img $(BUILD)/tps_mutseq.img,$(eval $(call fixture-inline-contract,$(img),TPSREP.COM TPS.DBF TPSPARM.DAT TPSREPT.TXT TPSREPT.BAK)))
$(eval $(call fixture-inline-contract,$(TPS_IMG_B),TPSREP.COM TPS.DBF AUTOEXEC.BAT))
$(eval $(call fixture-inline-contract,$(TPS_IMG_C),TPSREP.COM TPS.DBF TPSPARM.DAT AUTOEXEC.BAT))

# Ref: bead initech-a5sr, rm-on-failure proof. A tiny dedicated fixture no
# gate consumes, so test-fixture-volumes-mutant can break its contract and
# delete it without racing a real gate under make -j.
FIXTURE_RMPROOF_IMG := $(BUILD)/fixture_rmproof.img
$(FIXTURE_RMPROOF_IMG): $(FAT12_FIXTURE_DIR)/hello.txt | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
	@printf '>>> fixture_rmproof: minted %s (rm-on-failure proof volume; initech-a5sr)\n' "$@"
	@$(call fixture-check,recipe,$@)
$(eval $(call fixture-contract,$(FIXTURE_RMPROOF_IMG),HELLO.TXT))
ifdef FIXTURE_MUT_BROKEN_CONTRACT
# Mutant: the contract names a file the recipe never copies, so the
# end-of-recipe verifier fails and must delete the half-built image.
FIXTURE_CONTENTS_$(FIXTURE_RMPROOF_IMG) += ZZZMUT.TXT
endif

# Lint leg (bead initech-a5sr), run by test-fixture-volumes: every image a
# flair-data recipe mints (FLAIR_DATA_RECIPE_IMAGES, registered by
# flair-data-rules in the Makefile) and every FLAIR_DATA_<X>_IMG value that is
# not a declared scratch copy must carry a non-empty declared contract.
# Recursive, so it is evaluated at recipe time and also sees anything defined
# after the derivation above. The C inventory alone missed this: a recipe that
# runs mformat/mmd was auto-filed as an @unseeded volume, which masked the
# missing contract (see harness/check_fixture_volumes.c).
FLAIR_DATA_LINT_IMAGES = $(sort $(FLAIR_DATA_RECIPE_IMAGES) $(foreach v,$(filter-out $(FLAIR_DATA_SCRATCH_VARS),$(filter FLAIR_DATA_%_IMG,$(.VARIABLES))),$($(v))))
FLAIR_DATA_UNDECLARED = $(strip $(foreach img,$(FLAIR_DATA_LINT_IMAGES),$(if $(strip $(if $(filter $(img),$(FIXTURE_VOLUME_NAMES)),$(FIXTURE_CONTENTS_$(img)))),,$(img))))
.PHONY: fixture-volume-recipe-lint
fixture-volume-recipe-lint:
	@$(if $(FLAIR_DATA_UNDECLARED),printf '!!! test-fixture-volumes FAIL: flair-data image %s has no declared fixture contract (harness/fixture_volumes.mk; initech-a5sr)\n' $(FLAIR_DATA_UNDECLARED); exit 1,:)
	@$(if $(filter $(FLAIR_DATA_IMG),$(FLAIR_DATA_RECIPE_IMAGES)),:,printf '!!! test-fixture-volumes FAIL: FLAIR_DATA_RECIPE_IMAGES does not register $(FLAIR_DATA_IMG); flair-data-rules lost its registration line\n'; exit 1)
	@printf '>>> fixture-volume-recipe-lint: green (%s flair-data images, each with a declared contract)\n' $(words $(FLAIR_DATA_LINT_IMAGES))

# The pattern image carries the same contract as the non-mutant file volume.
fixture-spec = $(if $(filter $(BUILD)/flair_i123_file_mut_%.img,$(1)),$(FIXTURE_CONTENTS_$(FLAIR_I123_FILE_DATA)),$(FIXTURE_CONTENTS_$(1)))
fixture-check = sh harness/verify_fixture_volume.sh $(1) "$(2)" $(if $(strip $(3)),$(3),$(call fixture-spec,$(2)))
# Preflight only declared fixture images; freshly minted inline volumes are
# checked by their minter, and copied scratch volumes by their copy helper.
fixture-preflight = $(if $(call fixture-spec,$(1)),$(call fixture-check,harness,$(1)),:)

.PHONY: verify-fixture-volume
FIXTURE_IMAGE ?= $(FLAIR_I123_QUIT_DATA)
verify-fixture-volume:
	@$(call fixture-check,recipe,$(FIXTURE_IMAGE))
