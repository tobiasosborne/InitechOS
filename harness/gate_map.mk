# Locked diff-to-gate contract (initech-fw3x0); PRD Sec 8/9/14.
# Include after ALL TEST_*_GATES assignments, before gate_pairs.mk.
GATE_MAP ?= spec/gate_map.json
GATE_MAP_TOOL := $(BUILD)/gate_map
GATE_MAP_INVENTORY := $(BUILD)/gate_inventory.txt
GATE_MAP_MUT ?=
GATE_MAP_JSON = $(if $(GATE_MAP_MUT),$(BUILD)/gate-map-$(GATE_MAP_MUT).json,$(GATE_MAP))
MAKEFILES ?=
GATE_MAP_MAKEFILE := $(firstword $(filter-out $(MAKEFILES),$(MAKEFILE_LIST)))
DIFF ?=
FILES ?=
export DIFF FILES

# Included-file generators can run before the requested target on a cold tree.
ifneq ($(filter gates-for,$(MAKECMDGOALS)),)
.SILENT:
endif

$(GATE_MAP_TOOL): tools/gate_map.c | $(BUILD)
	@$(CC) $(CFLAGS) -o $@ $< >&2

.PHONY: gate-map-force gate-map-inventory-db
gate-map-force:
gate-map-inventory-db:
# -q exposes the database without executing even recursive recipe lines.
# Restrict to the Files section, excluding unevaluated define/pattern rows.
$(GATE_MAP_INVENTORY): gate-map-force | $(BUILD)
	@rc=0; $(MAKE) --no-print-directory -f $(GATE_MAP_MAKEFILE) -pq gate-map-inventory-db SKIP_BOCHS=0 > $@.db || rc=$$?; \
	 [ "$$rc" -le 1 ] || { printf 'gate-map RED: make database failed (%s)\n' "$$rc" >&2; exit 1; }; \
	 awk '/^# Files$$/{files=1} files && /^test-[^[:space:]:=$$%]+:/ {sub(/:$$/, "", $$1); print $$1}' $@.db | LC_ALL=C sort -u > $@.tmp; \
	 test -s $@.tmp || { printf 'gate-map RED: empty make inventory\n' >&2; exit 1; }; \
	 mv -f $@.tmp $@

.PHONY: gates-for test-for test-gate-map test-gate-map-mutant
gates-for: $(GATE_MAP_TOOL) $(GATE_MAP_INVENTORY) $(GATE_MAP_JSON)
	@set -eu; tmp=$$(mktemp $(BUILD)/gate-map-paths.XXXXXX); trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	 if [ -n "$$DIFF" ] && [ -n "$$FILES" ]; then printf 'gate-map RED: use DIFF or FILES, not both\n' >&2; exit 1; \
	 elif [ -n "$$DIFF" ]; then git -c core.quotePath=false diff --no-ext-diff --no-renames --name-only "$$DIFF" -- > "$$tmp"; \
	 elif [ -n "$$FILES" ]; then set -f; for p in $$FILES; do printf '%s\n' "$$p"; done > "$$tmp"; \
	 else printf 'gate-map RED: set DIFF=<range> or FILES=<space-separated paths>\n' >&2; exit 1; fi; \
	 $(GATE_MAP_TOOL) --resolve $(GATE_MAP_JSON) $(GATE_MAP_INVENTORY) < "$$tmp"

# Completion targets run only after their gate succeeds. Failed prerequisites
# therefore count as red gates too, without guessing from human log wording.
# One -k make sees the entire list, retaining gate_pairs ordering. Force -j16
# so the selected kernel/FLAIR/overlay reproducibility gates exercise that mode.
test-for: $(GATE_MAP_TOOL) $(GATE_MAP_INVENTORY)
	@set -eu; d=$$(mktemp -d $(BUILD)/gate-map-run.XXXXXX); \
	 $(MAKE) --no-print-directory -f $(GATE_MAP_MAKEFILE) -s gates-for > "$$d/gates"; \
	 : > "$$d/green"; \
	 awk -v green="$$d/green" '{print ".PHONY: gate-map-done-" $$0; print "gate-map-done-" $$0 ": " $$0; print "\t@printf '\''%s\\n'\'' '\''" $$0 "'\'' >> " green}' "$$d/gates" > "$$d/done.mk"; \
	 gates=$$(cat "$$d/gates"); done_gates=$$(sed 's/^/gate-map-done-/' "$$d/gates"); rc=0; \
	 $(MAKE) --no-print-directory -k -j16 -f $(GATE_MAP_MAKEFILE) -f "$$d/done.mk" $$gates $$done_gates || rc=$$?; \
	 n=$$(wc -l < "$$d/gates"); green=$$(wc -l < "$$d/green"); red=$$((n - green)); \
	 printf '>>> test-for: %s gates, %s red\n' "$$n" "$$red"; \
	 [ "$$rc" -eq 0 ] && [ "$$red" -eq 0 ]

test-gate-map: $(GATE_MAP_TOOL) $(GATE_MAP_INVENTORY) $(GATE_MAP_JSON)
	@set -eu; p=$$(mktemp $(BUILD)/gate-map-tracked.XXXXXX); trap 'rm -f "$$p"' EXIT HUP INT TERM; \
	 git -c core.quotePath=false ls-files > "$$p"; \
	 $(GATE_MAP_TOOL) --audit $(GATE_MAP_JSON) $(GATE_MAP_INVENTORY) < "$$p"; \
	 $(GATE_MAP_TOOL) --audit-consumers $(GATE_MAP_JSON) $(GATE_MAP_INVENTORY) $(GATE_MAP_INVENTORY).db < "$$p"

$(BUILD)/gate-map-%.json: $(GATE_MAP_TOOL) $(GATE_MAP)
	@$(GATE_MAP_TOOL) --mutate $* $(GATE_MAP) > $@.tmp && mv -f $@.tmp $@

test-gate-map-mutant: test-gate-map
	@set -eu; for m in drop-family duplicate-gate drop-consumer pending-active; do \
	 if $(MAKE) --no-print-directory -f $(GATE_MAP_MAKEFILE) test-gate-map GATE_MAP_MUT=$$m > $(BUILD)/gate-map-$$m.log 2>&1; then \
	 printf 'gate-map RED: %s mutant survived\n' "$$m"; exit 1; fi; \
	 case $$m in pending-active) reason='active gate test-boot in opt-in pending family';; drop-family) reason='unmapped gate';; duplicate-gate) reason='listed twice';; \
	 drop-consumer) reason='consumer harness/proptest/serial_trace_fixture.c -> test-serial-trace-pack (family harness not selected)'; \
	   git -c core.quotePath=false ls-files | $(GATE_MAP_TOOL) --audit $(BUILD)/gate-map-$$m.json $(GATE_MAP_INVENTORY);; esac; \
	 grep -m1 "gate-map RED: .*$$reason" $(BUILD)/gate-map-$$m.log || { cat $(BUILD)/gate-map-$$m.log; exit 1; }; \
	 done; printf '>>> test-gate-map-mutant: green (drop-family, duplicate-gate, drop-consumer and pending-active correctly RED; drop-consumer passes structural audit)\n'
TEST_UNIT_GATES += test-gate-map test-gate-map-mutant
