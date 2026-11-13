TOP := $(abspath ../..)
COMPONENT := apps/initech123
include $(TOP)/mk/config.mk

CORE := $(wildcard core/*.c) fs/wk1.c
HOST_CORE := $(addprefix $(OUT)/host-core/,$(CORE:.c=.o))
X87_CORE := $(addprefix $(OUT)/host-x87/,$(CORE:.c=.o))
LOCAL_INC := -Iinclude
FACE_INC := -I$(TOP)/sdk/include -I$(TOP)/sdk/host
HOST_INCLUDES := $(LOCAL_INC) -Itests
FACE_TESTS := quit range_menu entry clock keys hit resize cells
DIFF_TESTS := wk1_roundtrip formula recalc_order copy_move transfer_baseline menu_tree
HOST_TESTS := $(addprefix test_i123_,$(DIFF_TESTS) $(FACE_TESTS)) \
              test_i123_formula_x87_037F test_i123_formula_x87_027F \
              test_i123_copy_corpus test_i123_copy_tile
$(foreach t,$(DIFF_TESTS),$(eval test_i123_$(t)_SRCS := tests/diff/test_i123_$(t).c $(HOST_CORE)))
$(foreach t,$(FACE_TESTS),$(eval test_i123_$(t)_SRCS := tests/test_i123_$(t).c $(HOST_CORE)))
$(foreach t,$(FACE_TESTS),$(eval test_i123_$(t)_CFLAGS := $(FACE_INC) -D_DEFAULT_SOURCE))
$(foreach cw,037F 027F,$(eval test_i123_formula_x87_$(cw)_SRCS := tests/diff/test_i123_formula.c $(X87_CORE)))
$(foreach cw,037F 027F,$(eval test_i123_formula_x87_$(cw)_CFLAGS := -mfpmath=387 -DI123_TEST_X87_CW=0x$(cw)))
$(foreach mode,corpus tile,$(eval test_i123_copy_$(mode)_SRCS := $(test_i123_copy_move_SRCS)))
test_i123_menu_tree_SRCS := tests/diff/test_i123_menu_tree.c core/menu.c
test_i123_menu_tree_CFLAGS := -DI123_MENU_HOST
TEST_HDRS := $(shell find include core -name '*.h') tests/test_assert.h tests/face_driver.h
TEST_HDRS += $(wildcard $(TOP)/sdk/include/*.h) $(TOP)/sdk/host/tbx_host.h
$(foreach t,$(FACE_TESTS),$(eval test_i123_$(t)_DEPS := $(TOP)/build/sdk/host/libtbx_host.a app/face_a.c tests/test_i123_quit.c tests/run.sh))
$(foreach t,$(FACE_TESTS),$(eval test_i123_$(t)_LIBS := $(TOP)/build/sdk/host/libtbx_host.a))
include $(TOP)/mk/host-test.mk

# The suites also need fresh scratch directories and independent post-checks.
# Keep the shared runner's status/summary format, wrapping each invocation.
define host_test_run
n=$$((n+1)); \
if LOTUS123_DECOMP="$(LOTUS123_DECOMP)" sh tests/run.sh $(HOST_OUT)/$(1) > $(HOST_OUT)/$(1).log 2>&1; then \
  echo "  ok    $(1)"; \
else \
  echo "  FAIL  $(1)   (log: $(HOST_OUT)/$(1).log)"; \
  tail -n 15 $(HOST_OUT)/$(1).log | sed 's/^/        /'; \
  fail=$$((fail+1)); \
fi;
endef

.PHONY: corpus
corpus:
	@test -f "$(LOTUS123_DECOMP)/tools/wk1_ref.py" && test -d "$(LOTUS123_DECOMP)/goldens/minted" && test -f "$(LOTUS123_DECOMP)/goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI" || { echo 'FAIL: Lotus corpus missing; set LOTUS123_DECOMP to the lotus123-decomp directory' >&2; exit 1; }
	@command -v python3 >/dev/null || { echo 'FAIL: python3 required for the independent Lotus reader' >&2; exit 1; }
test: corpus

$(TOP)/build/sdk/host/libtbx_host.a: $(wildcard $(TOP)/sdk/host/* $(TOP)/sdk/include/*.h) $(TOP)/sdk/Makefile $(TOP)/mk/config.mk
	+$(MAKE) -C $(TOP)/sdk host

$(OUT)/host-core/%.o: %.c $(filter include/% core/%,$(TEST_HDRS)) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) -MMD -MP -c $< -o $@
$(OUT)/host-x87/%.o: %.c $(filter include/% core/%,$(TEST_HDRS)) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) -mfpmath=387 -MMD -MP -c $< -o $@
-include $(HOST_CORE:.o=.d) $(X87_CORE:.o=.d)

TENANT := 123
TENANT_SRCS := $(CORE) app/face_a.c
TENANT_EXE := 123.EXE
TENANT_CPPFLAGS := -DI123_MENU_DISK $(LOCAL_INC)
TENANT_RESOURCES := $(OUT)/123.MNU
include $(TOP)/mk/tenant.mk
$(OUT)/123.MNU: $(HOST_OUT)/test_i123_menu_tree
	$< --resource > $@

smoke:
	@echo 'Tier 1 awaits the booted desktop; see tests/SMOKES.md' >&2; exit 1
