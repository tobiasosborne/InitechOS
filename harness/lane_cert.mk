# Ref: PRD sections 8/9/14; Merge workflow v2 F9 (initech-xses3).
# Private local repositories: no network, project refs, or shared gate scratch.
LANE_CERT_SCRIPT ?= tools/lane-cert.sh

.PHONY: test-lane-cert test-lane-cert-mutant
test-lane-cert:
	+@set -eu; \
	fail() { printf '!!! test-lane-cert FAIL: %s\n' "$$1"; exit 1; }; \
	[ "$(filter test-lane-cert test-lane-cert-mutant,$(GATE_VECTOR))" = 'test-lane-cert test-lane-cert-mutant' ] || fail 'gate vector registration'; \
	cert_tool="$(abspath $(LANE_CERT_SCRIPT))"; \
	check_tool="$(abspath tools/cert-check.sh)"; \
	replay_tool="$(abspath tools/proof-replay.sh)"; \
	for tool in "$$cert_tool" "$$check_tool" "$$replay_tool"; do \
	    [ -f "$$tool" ] || fail "script missing: $${tool##*/}"; \
	done; \
	out="$(abspath $(BUILD))/lane-cert/unit"; \
	if [ -d "$$out/repo/.git" ]; then git -C "$$out/repo" worktree prune; fi; \
	rm -rf "$$out"; mkdir -p "$$out/repo"; \
	git init -q --bare --initial-branch=main "$$out/origin.git"; \
	cd "$$out/repo"; git init -q --initial-branch=main; \
	git config user.name 'Lane fixture'; git config user.email 'lane@example.invalid'; \
	git config commit.gpgSign false; git config core.hooksPath /dev/null; \
	printf '/build/\n' > .gitignore; \
	printf '%s\n' '.PHONY: test-one test-two' \
	    'test-one:' \
	    ' @case "$$$$(cat state)" in green) echo ">>> test-one: green (fixture)";; false-green) echo ">>> test-one: green (fixture)"; exit 1;; red-zero) echo ">>> test-one: red (fixture)";; quiet) echo no-marker;; *) echo ">>> test-one: red (fixture)"; exit 1;; esac' \
	    'test-two:' \
	    ' @echo ">>> test-two: red (intermediate)"' \
	    ' @echo ">>> test-two: green (fixture)"' | sed 's/^ /\t/' > Makefile; \
	printf 'red\n' > state; git add .; git commit -qm 'fixture red base'; \
	base=$$(git rev-parse HEAD); \
	git remote add origin "$$out/origin.git"; git push -q origin main; \
	git switch -qc lane/proof; \
	printf 'green\n' > state; git commit -qam 'fixture green lane'; \
	head=$$(git rev-parse HEAD); cert=build/cert/lane-proof.cert; \
	run_cert() { rc=0; GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; }; \
	refused() { [ "$$rc" -eq 2 ] && grep -F "$$1" "$$out/cert.log" >/dev/null && [ ! -f "$$cert" ]; }; \
	printf 'dirty\n' >> state; run_cert; \
	refused 'dirty tree' || fail 'dirty tree refusal'; git restore state; \
	printf 'untracked\n' > untracked; run_cert; \
	refused 'dirty tree' || fail 'untracked tree refusal'; rm -f untracked; \
	printf 'staged\n' > staged; git add staged; run_cert; \
	refused 'dirty tree' || fail 'staged tree refusal'; git reset -q; rm -f staged; \
	git switch -q main; git commit -q --allow-empty -m 'advance main'; \
	git push -q origin main; git switch -q lane/proof; run_cert; \
	refused 'rebase required' || fail 'unrebased branch refusal'; \
	git switch -q main; git reset -q --hard "$$base"; git push -q --force origin main; \
	git switch -q lane/proof; \
	printf 'red\n' > state; git commit -qam 'fixture mutant red gate'; \
	mutant=$$(git rev-parse HEAD); run_cert; \
	refused 'red gate: test-one' || fail 'red gate refusal (no cert)'; \
	for state in false-green red-zero quiet; do \
	    printf '%s\n' "$$state" > state; git commit -qam "fixture $$state"; run_cert; \
	    refused 'red gate: test-one' || fail "$$state accepted"; \
	done; \
	git reset -q --hard "$$head"; \
	rc=0; env -u GATES MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; \
	refused 'no gate list' || fail 'missing gate list refusal'; \
	run_cert; [ "$$rc" -eq 0 ] && [ -f "$$cert" ] || { cat "$$out/cert.log"; fail 'green certification'; }; \
	for field in "base_sha: $$base" "head_sha: $$head" 'branch: lane/proof' 'gate_count: 2' \
	    'gate test-one >>> test-one: green (fixture)' 'gate test-two >>> test-two: green (fixture)'; do \
	    grep -qxF "$$field" "$$cert" || fail "certificate field: $$field"; \
	done; \
	for field in toolchain host utc; do grep -q "^$$field: ." "$$cert" || fail "certificate field: $$field"; done; \
	for tool in gcc nasm qemu-system-i386 bochs; do grep -q "$$tool=" "$$cert" || fail "toolchain field: $$tool"; done; \
	[ -z "$$(LC_ALL=C tr -d '\011\012\015\040-\176' < "$$cert")" ] || fail 'non-ASCII certificate'; \
	! grep -F "$$HOME" "$$cert" >/dev/null || fail 'home path in certificate'; \
	cp -f "$$cert" "$$out/good.cert"; \
	printf 'red\n' > state; git commit -qam 'revoke previous green'; run_cert; \
	refused 'red gate: test-one' || fail 'old certificate not revoked on red'; \
	git reset -q --hard "$$head"; \
	git switch -q main; sh "$$check_tool" "$$out/good.cert" > "$$out/check.log" 2>&1 || { cat "$$out/check.log"; fail 'fresh certificate check'; }; \
	git commit -q --allow-empty -m 'stale certificate base'; \
	if sh "$$check_tool" "$$out/good.cert" > "$$out/check.log" 2>&1; then fail 'stale base accepted'; fi; \
	grep -F 'stale base_sha' "$$out/check.log" >/dev/null || fail 'stale base reason'; \
	git reset -q --hard "$$base"; \
	printf '%s\n' '.PHONY: gates-for' 'gates-for:' \
	    ' @echo "test-one test-two"' | sed 's/^ /\t/' > gate-map.mk; \
	printf '\ninclude gate-map.mk\n' >> Makefile; \
	git add .; git commit -qm 'fixture gates-for'; \
	mapbase=$$(git rev-parse HEAD); git push -q origin main; git switch -qc lane/mapped; \
	printf 'green\n' > state; git commit -qam 'mapped lane green'; \
	run_cert; [ "$$rc" -eq 0 ] || { cat "$$out/cert.log"; fail 'gates-for certification'; }; \
	mapped=build/cert/lane-mapped.cert; \
	grep -qxF "base_sha: $$mapbase" "$$mapped" || fail 'mapped base'; \
	git switch -q main; sh "$$check_tool" "$$mapped" > "$$out/check.log" 2>&1 || fail 'mapped superset'; \
	sed '/^gate test-two /d; s/^gate_count: 2$$/gate_count: 1/' "$$mapped" > "$$out/missing.cert"; \
	if sh "$$check_tool" "$$out/missing.cert" > "$$out/check.log" 2>&1; then fail 'missing gate accepted'; fi; \
	grep -F 'missing gate: test-two' "$$out/check.log" >/dev/null || fail 'missing gate reason'; \
	git switch -q lane/mapped; \
	printf '%s\n' '.PHONY: gates-for' 'gates-for:' \
	    ' @echo "test-two"' | sed 's/^ /\t/' > gate-map.mk; \
	git commit -qam 'selector overrides GATES'; run_cert; \
	[ "$$rc" -eq 0 ] && grep -qxF 'gate_count: 1' "$$mapped" || fail 'gates-for precedence'; \
	printf '%s\n' '.PHONY: gates-for' 'gates-for:' \
	    ' @echo selector-broken; exit 1' | sed 's/^ /\t/' > gate-map.mk; \
	git commit -qam 'broken selector'; run_cert; \
	[ "$$rc" -eq 2 ] && [ ! -f "$$mapped" ] || fail 'broken selector accepted'; \
	git switch -q main; mkdir -p build/proof; \
	printf 'RED|%s|make -s test-one|>>> test-one: red (fixture)\nGREEN|%s|make -s test-one|>>> test-one: green (fixture)\nMUTANT|%s|make -s test-one|>>> test-one: red (fixture)\n' \
	    "$$base" "$$head" "$$mutant" > build/proof/manifest; \
	sh "$$replay_tool" > "$$out/replay.log" 2>&1 || { cat "$$out/replay.log"; fail 'proof replay'; }; \
	[ "$$(grep -c '^PASS ' "$$out/replay.log")" -eq 3 ] || fail 'proof replay PASS records'; \
	sh "$$replay_tool" > "$$out/replay.log" 2>&1 || fail 'proof worktree reuse'; \
	printf 'GREEN|%s|cat; make -s test-one|>>> test-one: green (fixture)\nGREEN|%s|make -s test-one|>>> test-one: green (fixture)\n' \
	    "$$head" "$$head" > build/proof/manifest; \
	sh "$$replay_tool" > "$$out/replay.log" 2>&1 || fail 'proof stdin isolation'; \
	[ "$$(grep -c '^PASS ' "$$out/replay.log")" -eq 2 ] || fail 'proof stdin consumed records'; \
	printf 'GREEN|%s|make -s test-one|wrong EXPECT\n' "$$head" > build/proof/manifest; \
	if sh "$$replay_tool" > "$$out/replay.log" 2>&1; then fail 'wrong EXPECT accepted'; fi; \
	grep -q '^FAIL ' "$$out/replay.log" || fail 'proof replay FAIL record'; \
	printf 'GREEN|%s|make -s test-one|>>> test-one: red (fixture)\n' "$$base" > build/proof/manifest; \
	if sh "$$replay_tool" > "$$out/replay.log" 2>&1; then fail 'GREEN with red exit accepted'; fi; \
	printf 'RED|%s|make -s test-one|>>> test-one: green (fixture)\n' "$$head" > build/proof/manifest; \
	if sh "$$replay_tool" > "$$out/replay.log" 2>&1; then fail 'RED with green exit accepted'; fi; \
	printf '>>> test-lane-cert: green (dirty, stale base, red, fields, selector, proof replay)\n'

test-lane-cert-mutant:
	+@set -eu; out="$(abspath $(BUILD))/lane-cert/mutants"; mkdir -p "$$out"; \
	sed 's/^if \[ -n "$$dirty" \]; then$$/if false; then # mutant: skip dirty check/' tools/lane-cert.sh > "$$out/dirty.sh"; \
	sed 's/^    if \[ "$$gate_ok" != yes \]; then$$/    if false; then # mutant: certify red gate/' tools/lane-cert.sh > "$$out/red.sh"; \
	! cmp -s tools/lane-cert.sh "$$out/dirty.sh" && ! cmp -s tools/lane-cert.sh "$$out/red.sh"; \
	for mutant in dirty red; do \
	    rc=0; $(MAKE) --no-print-directory test-lane-cert \
	        "LANE_CERT_SCRIPT=$$out/$$mutant.sh" "BUILD=$$out/$$mutant-build" > "$$out/$$mutant.log" 2>&1 || rc=$$?; \
	    case $$mutant in dirty) reason='dirty tree refusal';; red) reason='red gate refusal (no cert)';; esac; \
	    [ "$$rc" -ne 0 ] && grep -qF "!!! test-lane-cert FAIL: $$reason" "$$out/$$mutant.log" || { cat "$$out/$$mutant.log"; exit 1; }; \
	    printf '>>> test-lane-cert-mutant: %s RED (%s)\n' "$$mutant" "$$reason"; \
	done; \
	printf '>>> test-lane-cert-mutant: green (skip-dirty and certify-red killed by fixture)\n'

TEST_UNIT_GATES += test-lane-cert test-lane-cert-mutant
