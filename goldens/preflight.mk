# Shared defaults/overrides also used by every component.
TOP := $(abspath .)
COMPONENT := goldens
include $(TOP)/mk/config.mk
export SYSTEM7_DECOMP WIN31_DECOMP DBASE3_DECOMP LOTUS123_DECOMP WORDPERFECT51_DECOMP TURBOPASCAL_DECOMP
.PHONY: verify verifier
verifier: $(OUT)/stat-files
$(OUT)/stat-files: goldens/stat-files.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
verify: verifier
	@CORPORA="$(CORPORA)" CORPUS_CACHE="$(OUT)" sh $(TOP)/goldens/verify.sh --ready
