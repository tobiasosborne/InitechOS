TOP := $(abspath ..)
COMPONENT := turbo
include $(TOP)/mk/config.mk

all test smoke mutants:
	+@for c in seed compiler; do $(MAKE) --no-print-directory -C $$c $@ || exit 1; done

selfhost ddc:
	@echo '$@: not implemented; see ROADMAP.md' >&2; exit 1
