# Standalone component tests verify their own consumed references. The top
# level verifies the full lock once before admitting parallel components.
.PHONY: pinned-corpus
test-build: pinned-corpus
pinned-corpus:
ifndef INITECH_CORPUS_VERIFIED
	+@$(MAKE) --no-print-directory -C $(TOP) corpus CORPORA="$(CORPORA)"
endif
