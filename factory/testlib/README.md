# Host test helpers

Factory-only code; none of this is linked into the product.

- `test_assert.h`: CHECK, CHECK_STR_EQ, TEST_HARNESS and TEST_SUMMARY.
- `expect_abort.h`: forked success, diagnostic-marker and SIGABRT probes.
  Includes follow test_assert.h; the caller supplies stdio declarations.
- `oracle_policy.h`: explicit diagnostics for unavailable reference corpora.
- `run-mutants.sh`: the POSIX shell runner used by `mk/mutants.mk`.

Components add this directory to their test include paths and list these
headers as test prerequisites. Product compile paths do not include it.
The seeded region generators and shrinker remain private to ATKINSON.
