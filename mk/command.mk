# Compare commands in make: unchanged outputs need no shell or stamp writes.
.PHONY: FORCE
FORCE:
shquote = '$(subst ','"'"',$(1))'
# Commands contain doubled dollars before make passes them to the shell.
command-text = $(subst $$$$,$$,$(or $(1),$(command)))
command-same = $(if $(subst $(call command-text,$(1)),,$(file <$@.cmd))$(subst $(file <$@.cmd),,$(call command-text,$(1))),,1)
define run-command
$(if $(and $(wildcard $@),$(if $(filter-out FORCE,$?),,1),$(call command-same,$(1))),@:,@mkdir -p $(@D); \
printf '%s' $(call shquote,$(or $(1),$(command))) > $@.cmd.tmp; \
echo $(call shquote,$(or $(1),$(command))); \
$(or $(1),$(command)) && mv $@.cmd.tmp $@.cmd)
endef
