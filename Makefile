# InitechOS -- top level. This file only composes components. Each
# component builds and tests itself; see CLAUDE.md.

# Components that have been imported into the new tree, in dependency
# order. Add a component here when its `make test` is green.
COMPONENTS := flair/region sdk apps/initech123

.DEFAULT_GOAL := help
.PHONY: help all test smoke image cert clean

help:
	@echo "make test    tier 0: host tests of every imported component"
	@echo "make image   build the product image        (not yet: needs boot + kernel)"
	@echo "make smoke   tier 1: drive the booted image (not yet)"
	@echo "make cert    tier 2: the full certificate   (not yet)"
	@echo "imported components: $(COMPONENTS)"

all:
	@for c in $(COMPONENTS); do $(MAKE) --no-print-directory -C $$c all || exit 1; done

test:
	@fail=0; for c in $(COMPONENTS); do \
	  $(MAKE) --no-print-directory -C $$c test || fail=$$((fail+1)); \
	done; \
	if [ $$fail -eq 0 ]; then echo "ALL GREEN"; else echo "$$fail component(s) RED"; exit 1; fi

clean:
	rm -rf build
