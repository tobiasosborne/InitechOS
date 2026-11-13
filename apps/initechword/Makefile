TOP := $(abspath ../..)
COMPONENT := apps/initechword
include $(TOP)/mk/config.mk

CORE := $(wildcard core/*.c) fs/wp5.c
LOCAL_INC := -Iinclude
HOST_CORE := $(addprefix $(OUT)/host-core/,$(CORE:.c=.o))
# The app interface. The face includes nothing else from outside.
FACE_INC := -I$(TOP)/sdk/include
HOST_INCLUDES := $(LOCAL_INC) -I$(TOP)/factory/testlib
HOST_TESTS := test_iword_core test_iword_codes test_iword_wp5_roundtrip \
 test_iword_reveal_codes test_iword_face test_iword_face_small \
 test_iword_fkeys test_iword_search test_iword_listfiles test_iword_listfiles_ops \
 test_iword_replace test_iword_undo test_iword_setup
FACE_TESTS := face face_small fkeys search listfiles listfiles_ops replace undo setup
$(foreach t,core codes,$(eval test_iword_$(t)_SRCS := tests/test_iword_$(t).c))
$(foreach t,wp5_roundtrip reveal_codes,$(eval test_iword_$(t)_SRCS := tests/diff/test_iword_$(t).c))
$(foreach t,face face_small,$(eval test_iword_$(t)_SRCS := tests/test_iword_face.c))
$(foreach t,fkeys search,$(eval test_iword_$(t)_SRCS := tests/test_iword_p3.c))
$(foreach t,replace undo setup,$(eval test_iword_$(t)_SRCS := tests/test_iword_p3_part2.c))
$(foreach t,listfiles listfiles_ops,$(eval test_iword_$(t)_SRCS := tests/test_iword_$(t).c))
$(foreach t,$(HOST_TESTS),$(eval $(t)_OBJS := $(HOST_CORE)))
$(foreach t,$(FACE_TESTS),$(eval test_iword_$(t)_CFLAGS := $(FACE_INC) -D_DEFAULT_SOURCE))
test_iword_face_small_CFLAGS += -DIWORD_SMALL_BUFFERS
EXTRA_HOST_TESTS := test_iword_search_corpus iword_fkeys_gen
test_iword_search_corpus_SRCS := tests/diff/test_iword_search_corpus.c
test_iword_search_corpus_OBJS := $(HOST_CORE)
iword_fkeys_gen_SRCS := tests/tools/iword_fkeys_gen.c
$(foreach t,$(FACE_TESTS),$(eval test_iword_$(t)_DEPS := app/face_a.c tests/test_iword_face.c tests/run.sh))
test_iword_search_DEPS += $(OUT)/host/test_iword_search_corpus
test_iword_fkeys_DEPS += $(OUT)/host/iword_fkeys_gen
include $(TOP)/mk/host-test.mk

# Keep isolated scratch and multi-command independent post-checks per entry.
define host_test_run
n=$$((n+1)); \
if WORDPERFECT51_DECOMP="$(WORDPERFECT51_DECOMP)" sh tests/run.sh $(HOST_OUT)/$(1) > $(HOST_OUT)/$(1).log 2>&1; then \
  echo "  ok    $(1)"; \
else \
  echo "  FAIL  $(1)   (log: $(HOST_OUT)/$(1).log)"; \
  tail -n 15 $(HOST_OUT)/$(1).log | sed 's/^/        /'; \
  fail=$$((fail+1)); \
fi;
endef

.PHONY: corpus
corpus:
	@sh tests/corpus.sh "$(WORDPERFECT51_DECOMP)"
test-build: corpus

$(OUT)/host-core/%.o: %.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) -MMD -MP -c $< -o $@)
-include $(HOST_CORE:.o=.d)

TARGET_SRCS := $(CORE) app/face_a.c
TARGET_INCLUDES := $(LOCAL_INC)
$(OUT)/app/face_a.o: TARGET_INCLUDES += $(FACE_INC)
include $(TOP)/mk/target.mk
TARGET_ARTIFACTS := $(OUT)/WORD.EXE

smoke:
	@echo 'Tier 1 awaits the booted product and sdk tenant rules; see tests/SMOKES.md' >&2; exit 1
mutants:
	@echo 'Patch conversion deferred: see tests/MUTANTS-TODO.md' >&2; exit 1

TEST_BUDGET_S := 30
CORPORA := WORDPERFECT51_DECOMP
include $(TOP)/mk/corpus.mk
