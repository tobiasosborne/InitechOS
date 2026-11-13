# mk/host-test.mk -- tier 0: host tests for one component.
#
# Before including, set:
#   HOST_TESTS    := names of test programs
#   <name>_SRCS   := C sources of that program (test file + code under test)
# and optionally:
#   <name>_CFLAGS := extra compiler flags (-I, -D)
#   <name>_LIBS   := extra link flags (-lm)
#   <name>_ARGS   := arguments passed when the test runs
#   <name>_DEPS   := extra files that should trigger a rebuild
#   <name>_OBJS   := separately compiled objects to link (with their own rules)
#   <name>_ENV    := environment assignments for the test run
#   EXTRA_HOST_TESTS := optional programs, excluded from the default run
#   TESTS        := programs to build/run (default: HOST_TESTS)
#   HOST_INCLUDES := -I flags shared by every test of the component
#   TEST_HDRS     := headers every test depends on (default: all *.h here)
#
# Each test is one compiler invocation, built into build/<component>/host/.
# A test passes when it exits 0. `make test` runs every test, prints one
# line per test and a summary, and fails if any test failed.

HOST_OUT  := $(OUT)/host
TEST_HDRS ?= $(shell find . -name '*.h' 2>/dev/null)
TESTS ?= $(HOST_TESTS)
ifneq ($(filter-out $(HOST_TESTS) $(EXTRA_HOST_TESTS),$(TESTS)),)
$(error unknown TESTS: $(filter-out $(HOST_TESTS) $(EXTRA_HOST_TESTS),$(TESTS)))
endif

$(HOST_OUT):
	@mkdir -p $@

define host_test_rule
$(HOST_OUT)/$(1): $$($(1)_SRCS) $$($(1)_OBJS) $$($(1)_DEPS) $$(TEST_HDRS) $$(MAKEFILE_LIST) | $(HOST_OUT)
	$$(HOSTCC) $$(HOST_CFLAGS) $$(HOST_INCLUDES) $$($(1)_CFLAGS) -o $$@ $$($(1)_SRCS) $$($(1)_OBJS) $$($(1)_LIBS)
endef
$(foreach t,$(sort $(HOST_TESTS) $(EXTRA_HOST_TESTS)),$(eval $(call host_test_rule,$(t))))

HOST_TEST_BINS := $(addprefix $(HOST_OUT)/,$(TESTS))

define host_test_run
n=$$((n+1)); \
if $($(1)_ENV) $(HOST_OUT)/$(1) $($(1)_ARGS) > $(HOST_OUT)/$(1).log 2>&1; then \
  echo "  ok    $(1)"; \
else \
  echo "  FAIL  $(1)   (log: $(HOST_OUT)/$(1).log)"; \
  tail -n 15 $(HOST_OUT)/$(1).log | sed 's/^/        /'; \
  fail=$$((fail+1)); \
fi;
endef

.PHONY: test-build test-run
all test-build: $(HOST_TEST_BINS)
test: test-run
test-run: test-build
	@n=0; fail=0; \
	ulimit -c 0; \
	$(foreach t,$(TESTS),$(call host_test_run,$(t))) \
	echo "$(COMPONENT): $$n tests, $$fail failures"; \
	test $$fail -eq 0
