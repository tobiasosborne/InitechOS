# HOST_TESTS / EXTRA_HOST_TESTS register programs; TESTS selects the run.
# <name>_{SRCS,OBJS,CFLAGS,LIBS,ARGS,DEPS,ENV} and HOST_INCLUDES unchanged.
# TEST_HDRS is an optional explicit dependency list (no directory scan).
# TEST_TIMEOUT_S=20, <name>_TIMEOUT_S overrides; TEST_BUDGET_S=30 (run only).
# <name>_COUNT_PATTERN: ERE with one numeric capture; auto for shared header.
# host_test_run may still be overridden (n/fail shell counters and per-test log).
HOST_OUT := $(OUT)/host
TESTS ?= $(HOST_TESTS)
TEST_TIMEOUT_S ?= 20
TEST_BUDGET_S ?= 30
ifneq ($(filter-out $(HOST_TESTS) $(EXTRA_HOST_TESTS),$(TESTS)),)
$(error unknown TESTS: $(filter-out $(HOST_TESTS) $(EXTRA_HOST_TESTS),$(TESTS)))
endif

define host_source_rule
$(HOST_OUT)/$(1).objs/$(2:.c=.o): private command = $$(HOSTCC) $$(HOST_CFLAGS) $$(HOST_INCLUDES) $$($(1)_CFLAGS) -MMD -MP -c $$< -o $$@
$(HOST_OUT)/$(1).objs/$(2:.c=.o): $(2) $$($(1)_DEPS) $$(TEST_HDRS) FORCE
	$$(run-command)
endef
define host_test_rule
$(1)_C_OBJS := $(addprefix $(HOST_OUT)/$(1).objs/,$(patsubst %.c,%.o,$(filter %.c,$($(1)_SRCS))))
$(HOST_OUT)/$(1): private command = $$(HOSTCC) $$(HOST_CFLAGS) $$(HOST_INCLUDES) $$($(1)_CFLAGS) -o $$@ $$($(1)_C_OBJS) $(filter-out %.c,$($(1)_SRCS)) $$($(1)_OBJS) $$($(1)_LIBS)
$(HOST_OUT)/$(1): $$($(1)_C_OBJS) $(filter-out %.c,$($(1)_SRCS)) $$($(1)_OBJS) $$($(1)_DEPS) $$(TEST_HDRS) FORCE
	$$(run-command)
endef
$(foreach t,$(sort $(HOST_TESTS) $(EXTRA_HOST_TESTS)),$(foreach s,$(filter %.c,$($(t)_SRCS)),$(eval $(call host_source_rule,$(t),$(s))))$(eval $(call host_test_rule,$(t))))
# Read dep rules without adding generated files to legacy MAKEFILE_LIST lists.
$(foreach d,$(wildcard $(foreach t,$(HOST_TESTS) $(EXTRA_HOST_TESTS),$($(t)_C_OBJS:.o=.d))),$(eval $(file <$(d))))
HOST_TEST_BINS := $(addprefix $(HOST_OUT)/,$(TESTS))

define host_test_run
$($(1)_ENV) $(HOST_OUT)/$(1) $($(1)_ARGS) > $(HOST_OUT)/$(1).log 2>&1 || fail=1;
endef
# Isolate inherited runners under the same timeout/count/timing wrapper.
define timed_test
if sh $(TOP)/factory/testlib/run-test.sh $(1) $(or $($(1)_TIMEOUT_S),$(TEST_TIMEOUT_S)) $(call shquote,$($(1)_COUNT_PATTERN)) $(HOST_OUT)/$(1) sh -c $(call shquote,n=0; fail=0; $(call host_test_run,$(1)) test $$fail -eq 0); then :; else fail=$$((fail+1)); fi;
endef
.PHONY: test test-build test-run
test-build: $(HOST_TEST_BINS)
test: test-run
test-run: test-build
	@start=$$(date +%s); fail=0; ulimit -c 0; \
	$(foreach t,$(TESTS),$(call timed_test,$(t))) \
	elapsed=$$(($$(date +%s)-start)); \
	echo "$(COMPONENT): $(words $(TESTS)) tests, $$fail failures ($${elapsed}s; budget $(TEST_BUDGET_S)s)"; \
	if [ $$elapsed -gt $(TEST_BUDGET_S) ]; then \
	  echo 'FAIL budget: speed the suite up or raise TEST_BUDGET_S with a reason in the component README'; fail=1; \
	fi; test $$fail -eq 0
