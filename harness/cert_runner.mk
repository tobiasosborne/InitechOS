# Factory bookkeeping oracle; docs/design/parallel-certificate.md Sec 3.
# Rule 6 mutates the actual runner in build, never the checkout.
.PHONY: test-cert-runner test-cert-runner-mutant
test-cert-runner:
	@tools/cert-sharded-selftest.sh
test-cert-runner-mutant: | $(BUILD)
	@sed 's/^    *flock 9$$/    : # mutant: queue flock removed/' tools/cert-sharded.sh > $(BUILD)/cert-sharded-nolock.sh
	@chmod +x $(BUILD)/cert-sharded-nolock.sh
	@rc=0; tools/cert-sharded-selftest.sh $(BUILD)/cert-sharded-nolock.sh > $(BUILD)/cert-runner-mutant.log 2>&1 || rc=$$?; \
	if [ "$$rc" != 1 ] || ! grep -q '^RED cert-runner QUEUE-LOCK:' $(BUILD)/cert-runner-mutant.log; then \
		cat $(BUILD)/cert-runner-mutant.log; echo 'FAIL cert-runner: queue flock mutant survived or failed infrastructure'; exit 1; fi
	@cat $(BUILD)/cert-runner-mutant.log
	@echo 'GREEN test-cert-runner-mutant: queue flock removal caught'
TEST_UNIT_GATES += test-cert-runner test-cert-runner-mutant
