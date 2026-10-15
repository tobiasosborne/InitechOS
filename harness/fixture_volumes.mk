# Ref: PRD Sec 8; CLAUDE.md Rule 2/6; initech-tflp.
# Explicit contracts for seeded data-volume recipes. Included LAST so this
# list cannot cause a read-before-definition (test-makefile-vars, tdnl.81).
# Default mcopy flags are A; mtools lists read-only as A R and hidden as A H.
FLAIR_FIXTURES := README.TXT APPS/TENANTFX.EXE APPS/CTENANT.EXE APPS/123.EXE APPS/WORD.EXE APPS/123.MNU
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
FLAIR_FIXTURE_IMAGES := $(FLAIR_DATA_IMG) $(FLAIR_DATA_TFX_NOREG_IMG) $(FLAIR_DATA_TFX_CRASH_IMG) $(FLAIR_DATA_TFX_BADMBAR_IMG) $(FLAIR_DATA_I123_ARROWS_IMG) $(FLAIR_DATA_I123_NORECALC_IMG) $(FLAIR_DATA_IWORD_NOWRAP_IMG) $(FLAIR_DATA_IWORD_NOSAVE_IMG) $(FLAIR_DATA_FONTS_IMG) $(BUILD)/flair_i123_menu_stale.img $(FLAIR_DATA_IWORD_NOFACE_IMG) $(FLAIR_DATA_IWORD_NOD4_IMG)
$(foreach img,$(FLAIR_FIXTURE_IMAGES),$(eval $(call fixture-contract,$(img),$(FLAIR_FIXTURES))))
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
