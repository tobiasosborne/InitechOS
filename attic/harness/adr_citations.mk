# test-adr-citations -- beads initech-lk29, initech-0ni0; WL-0161.
# Ref: CLAUDE.md Law 1 (a governance doc cited by id must exist), Law 2,
#      Rule 2, Rule 6, Rule 8; docs/adr/ADR-0001-*.md and
#      docs/adr/ADR-0002-*.md Appendix R; harness/check_adr_citations.c.
# Host gate, no emulator. The CITING corpus (CLAUDE.md, the PRD, HANDOFF,
# docs/, spec/, os/, harness/, tools/, seed/, the Makefile) is the oracle:
#   (F) every scoped Record that is cited is FILED under docs/adr/;
#   (D) every scoped Record/DEC citation names a DEC heading in that Record.
# Ref: WL-0161 Frictions / initech-0ni0: discover every filed ADR/CDR id,
# including amendments and future Records. Derive scope from the real ADR
# directory even when a mutant supplies a scratch --adr-dir, so removing a
# copied Record cannot remove it from the gate's scope. Retain the original
# three explicit ids so unfiling a foundational Record still fails loud.
ADR_CITATIONS_CHECK := $(BUILD)/check_adr_citations
ADR_CITATIONS_RECORDS := $(sort ADR-0001 ADR-0002 CDR-0001 $(foreach f,$(notdir $(wildcard docs/adr/[AC]DR-*.md)),$(word 1,$(subst -, ,$(f)))-$(word 2,$(subst -, ,$(f)))))
ADR_CITATIONS_SCOPE := $(addprefix --scope ,$(ADR_CITATIONS_RECORDS))
ADR_CITATIONS_MUT := $(BUILD)/adr-citations-mutant

$(ADR_CITATIONS_CHECK): harness/check_adr_citations.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: test-adr-citations test-adr-citations-mutant
test-adr-citations: $(ADR_CITATIONS_CHECK)
	@$(ADR_CITATIONS_CHECK) --root . --adr-dir docs/adr $(ADR_CITATIONS_SCOPE) --inventory $(BUILD)/adr-citations.inventory

# Four named mutants, each on a COPY of docs/adr (the tree is never edited):
#   ADR_UNFILED      ADR-0002 removed -> RED for the lk29 reason (cited, not filed)
#   ADR1_DEC02_RENAMED  ADR-0001 "### DEC-02 --" -> "### DEC-92 --" -> RED naming DEC-02
#   ADR2_DEC03_RENAMED  ADR-0002 "### DEC-03 --" -> "### DEC-93 --" -> RED naming DEC-03
#   ADR8_BOGUS_CITATION  inject an undefined DEC for a newly scoped Record;
#                       scan the scratch copy as the citing corpus as well.
# Each copy is checked to differ from the original first (a no-op mutant is a FAIL).
define adr-citations-mutant
	@rm -rf $(ADR_CITATIONS_MUT)/$(1) && mkdir -p $(ADR_CITATIONS_MUT)/$(1) && cp -f docs/adr/*.md $(ADR_CITATIONS_MUT)/$(1)/
	@$(2)
	@if diff -rq docs/adr $(ADR_CITATIONS_MUT)/$(1) > /dev/null; then \
	  printf '!!! test-adr-citations-mutant FAIL: mutant $(1) was not applied\n'; exit 1; fi
	@set +e; $(ADR_CITATIONS_CHECK) --root $(if $(4),$(4),.) --adr-dir $(ADR_CITATIONS_MUT)/$(1) $(ADR_CITATIONS_SCOPE) > $(ADR_CITATIONS_MUT)/$(1).log 2>&1; rc=$$?; set -e; \
	 test $$rc -eq 1 && grep -qF '$(3)' $(ADR_CITATIONS_MUT)/$(1).log || { cat $(ADR_CITATIONS_MUT)/$(1).log; \
	   printf '!!! test-adr-citations-mutant FAIL: mutant $(1) survived or went RED for the wrong reason (rc=%s)\n' "$$rc"; exit 1; }; \
	 printf '>>> mutant $(1) RED: '; grep -F '$(3)' $(ADR_CITATIONS_MUT)/$(1).log | head -1
endef

test-adr-citations-mutant: test-adr-citations
	$(call adr-citations-mutant,ADR_UNFILED,rm -f $(ADR_CITATIONS_MUT)/ADR_UNFILED/ADR-0002-*.md,!!! test-adr-citations FAIL: ADR-0002 is cited at)
	$(call adr-citations-mutant,ADR1_DEC02_RENAMED,sed -i 's/^### DEC-02 -- /### DEC-92 -- /' $(ADR_CITATIONS_MUT)/ADR1_DEC02_RENAMED/ADR-0001-*.md,!!! test-adr-citations FAIL: ADR-0001 DEC-02 is cited at)
	$(call adr-citations-mutant,ADR2_DEC03_RENAMED,sed -i 's/^### DEC-03 -- /### DEC-93 -- /' $(ADR_CITATIONS_MUT)/ADR2_DEC03_RENAMED/ADR-0002-*.md,!!! test-adr-citations FAIL: ADR-0002 DEC-03 is cited at)
	$(call adr-citations-mutant,ADR8_BOGUS_CITATION,printf '%s %s\n' 'ADR-0008' 'DEC-99' > $(ADR_CITATIONS_MUT)/ADR8_BOGUS_CITATION/bogus.md,DEC-99 is cited at 1 site(s),$(ADR_CITATIONS_MUT)/ADR8_BOGUS_CITATION)
	@printf '>>> test-adr-citations-mutant: green (4 mutants RED for the named reason)\n'

TEST_UNIT_GATES += test-adr-citations test-adr-citations-mutant
