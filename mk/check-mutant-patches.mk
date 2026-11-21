# CLAUDE.md, The constitution: catch stale patches before running tier 2.
# DOS keeps its patches under the individual commands but applies them at dos/.
mutant_patch_dirs := $(wildcard mutants $(if $(filter dos,$(COMPONENT)),*/mutants))
ifneq ($(mutant_patch_dirs),)
.PHONY: check-mutant-patches
test: check-mutant-patches
check-mutant-patches:
	@find $(mutant_patch_dirs) -type f -name '*.patch' \
	  -exec sh "$(TOP)/mk/check-mutant-patches.sh" "$(TOP)" "$(COMPONENT)" {} +
endif
