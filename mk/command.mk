# One stamp per output; FORCE checks commands without touching unchanged files.
.PHONY: FORCE
FORCE:
shquote = '$(subst ','"'"',$(1))'
define run-command
@mkdir -p $(@D); \
printf '%s\n' $(call shquote,$(or $(1),$(command))) > $@.cmd.tmp; \
if ! cmp -s $@.cmd.tmp $@.cmd || [ -n "$(filter-out FORCE,$?)" ] || [ ! -e $@ ]; then \
  echo $(call shquote,$(or $(1),$(command))); \
  $(or $(1),$(command)) && mv $@.cmd.tmp $@.cmd; \
else rm -f $@.cmd.tmp; fi
endef
