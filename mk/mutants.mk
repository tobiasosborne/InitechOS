# Patch paths are component-relative: a/region.c -> b/region.c, patch -p1.
# Include after config.mk and host-test.mk. Defaults need no component wiring.
# MUTANT_PATCHES overrides the patch list. Optional '# test: name ...' selects
# registered host programs (TESTS); otherwise run the component's HOST_TESTS.
# Each patch gets a fresh copy under OUT/mutants. TOP points at the run's
# private tree for make fragments, test helpers and dependency builds.
# Baselines and compilation must pass; only a runtime failure kills a mutant.
MUTANT_PATCHES ?= $(wildcard mutants/*.patch)

.PHONY: mutants
mutants:
	+@sh $(TOP)/factory/testlib/run-mutants.sh "$(abspath $(TOP))" "$(COMPONENT)" \
	  "$(abspath $(OUT))" "$(MAKE)" $(MUTANT_PATCHES)
