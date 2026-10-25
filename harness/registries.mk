# Ref: F4 review (initech-q8j); PRD 6.6; CLAUDE.md Rules 2/6/11.
REGISTRY_CHECK := $(BUILD)/check_registries
$(REGISTRY_CHECK): harness/check_registries.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: test-samir-registries test-samir-registries-mutant test-samir-def-deps
test-samir-registries: $(REGISTRY_CHECK)
	@$(REGISTRY_CHECK) .
test-samir-registries-mutant: test-samir-registries
	@sh harness/registry_mutants.sh $(REGISTRY_CHECK)
test-samir-def-deps:
	+@MAKE="$(MAKE)" sh harness/def_dependency_check.sh

# Phony on purpose: marker additions, removals and union attributes must be
# inspected on every merge-vector run, independently of timestamp caching.
test-samir: test-samir-registries test-samir-registries-mutant test-samir-def-deps
