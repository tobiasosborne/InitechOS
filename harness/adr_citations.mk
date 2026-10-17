# test-adr-citations -- bead initech-lk29; WL-0161.
# Ref: CLAUDE.md Law 1 (a governance doc cited by id must exist), Law 2,
#      Rule 2, Rule 6, Rule 8; docs/adr/ADR-0001-*.md and
#      docs/adr/ADR-0002-*.md Appendix R; harness/check_adr_citations.c.
# Host gate, no emulator. The CITING corpus (CLAUDE.md, the PRD, HANDOFF,
# docs/, spec/, os/, harness/, tools/, seed/, the Makefile) is the oracle:
#   (F) every scoped Record that is cited is FILED under docs/adr/;
#   (D) every "ADR-0001 DEC-nn" / "ADR-0002 DEC-nn" citation names a DEC
#       heading in that Record.
# Scope is the two foundational Records plus CDR-0001 (ADR-0002's deviation).
# Other Records do not use DEC headings uniformly (ADR-0004, ADR-0008); widening
# the scope is its own bead, not a loosened grammar here.
ADR_CITATIONS_CHECK := $(BUILD)/check_adr_citations
ADR_CITATIONS_SCOPE := --scope ADR-0001 --scope ADR-0002 --scope CDR-0001
ADR_CITATIONS_MUT := $(BUILD)/adr-citations-mutant

$(ADR_CITATIONS_CHECK): harness/check_adr_citations.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: test-adr-citations test-adr-citations-mutant
test-adr-citations: $(ADR_CITATIONS_CHECK)
	@$(ADR_CITATIONS_CHECK) --root . --adr-dir docs/adr $(ADR_CITATIONS_SCOPE) --inventory $(BUILD)/adr-citations.inventory

# Three named mutants, each on a COPY of docs/adr (the tree is never edited):
#   ADR_UNFILED      ADR-0002 removed -> RED for the lk29 reason (cited, not filed)
#   ADR1_DEC02_RENAMED  ADR-0001 "### DEC-02 --" -> "### DEC-92 --" -> RED naming DEC-02
#   ADR2_DEC03_RENAMED  ADR-0002 "### DEC-03 --" -> "### DEC-93 --" -> RED naming DEC-03
# Each copy is checked to differ from the original first (a no-op mutant is a FAIL).
define adr-citations-mutant
	@rm -rf $(ADR_CITATIONS_MUT)/$(1) && mkdir -p $(ADR_CITATIONS_MUT)/$(1) && cp docs/adr/*.md $(ADR_CITATIONS_MUT)/$(1)/
	@$(2)
	@if diff -rq docs/adr $(ADR_CITATIONS_MUT)/$(1) > /dev/null; then \
	  printf '!!! test-adr-citations-mutant FAIL: mutant $(1) was not applied\n'; exit 1; fi
	@set +e; $(ADR_CITATIONS_CHECK) --root . --adr-dir $(ADR_CITATIONS_MUT)/$(1) $(ADR_CITATIONS_SCOPE) > $(ADR_CITATIONS_MUT)/$(1).log 2>&1; rc=$$?; set -e; \
	 test $$rc -eq 1 && grep -qF '$(3)' $(ADR_CITATIONS_MUT)/$(1).log || { cat $(ADR_CITATIONS_MUT)/$(1).log; \
	   printf '!!! test-adr-citations-mutant FAIL: mutant $(1) survived or went RED for the wrong reason (rc=%s)\n' "$$rc"; exit 1; }; \
	 printf '>>> mutant $(1) RED: '; grep -F '$(3)' $(ADR_CITATIONS_MUT)/$(1).log | head -1
endef

test-adr-citations-mutant: test-adr-citations
	$(call adr-citations-mutant,ADR_UNFILED,rm -f $(ADR_CITATIONS_MUT)/ADR_UNFILED/ADR-0002-*.md,!!! test-adr-citations FAIL: ADR-0002 is cited at)
	$(call adr-citations-mutant,ADR1_DEC02_RENAMED,sed -i 's/^### DEC-02 -- /### DEC-92 -- /' $(ADR_CITATIONS_MUT)/ADR1_DEC02_RENAMED/ADR-0001-*.md,!!! test-adr-citations FAIL: ADR-0001 DEC-02 is cited at)
	$(call adr-citations-mutant,ADR2_DEC03_RENAMED,sed -i 's/^### DEC-03 -- /### DEC-93 -- /' $(ADR_CITATIONS_MUT)/ADR2_DEC03_RENAMED/ADR-0002-*.md,!!! test-adr-citations FAIL: ADR-0002 DEC-03 is cited at)
	@printf '>>> test-adr-citations-mutant: green (3 mutants RED for the named reason)\n'

TEST_UNIT_GATES += test-adr-citations test-adr-citations-mutant
