#!/bin/sh
# PRD 8/9; initech-0spyf/fzsvo. Real local cert/check integration.
# Operator: only test-assets may record the absent copyrighted frame as SKIP.
set -eu
cert_tool=$1
check_tool=$2
out=$3
rm -rf "$out"
mkdir -p "$out/repo/build"
cd "$out/repo"
git init -q --initial-branch=main
git config user.name 'Certificate fixture'
git config user.email 'cert@example.invalid'
git config commit.gpgSign false
git config core.hooksPath /dev/null
printf '/build/\n/spec/assets/preview.webp\n' > .gitignore
cat > Makefile <<'MAKE'
.PHONY: test-one test-assets
test-one:
	@case "$$(cat build/state)" in green) echo '>>> test-one: green (fixture)';; legacy) echo 'GREEN test-one: real host oracle';; count) echo 'test_one: 3 checks, 0 failures';; zero-count) echo 'test_one: 0 checks, 0 failures';; other) echo 'GREEN test-other: wrong target';; silent) echo no-marker;; skip-other) echo '[SKIP] test-one: absent reference';; red-last) echo '>>> test-one: green'; echo 'RED test-one: final failure';; false-green) echo '>>> test-one: green'; exit 1;; esac
test-assets:
	@echo '[SKIP] test-assets: local-only fixture spec/assets/preview.webp absent (gitignored copyrighted frame); palette-honesty check requires it'
MAKE
git add .gitignore Makefile
git commit -qm 'certificate fixture base'
git update-ref refs/remotes/origin/main main
git switch -qc lane/summaries
git commit -q --allow-empty -m 'certificate fixture lane'
cert=build/cert/lane-summaries.cert
run_cert() {
    rc=0
    GATES='test-one test-assets' REQUIRED_GATES='test-one test-assets' \
        sh "$cert_tool" > "$out/cert.log" 2>&1 || rc=$?
}
red() { echo "RED cert-summaries: $*"; cat "$out/cert.log"; exit 1; }
for state in green legacy count; do
    printf '%s\n' "$state" > build/state
    run_cert
    [ "$rc" = 0 ] || red "$state with authorized assets SKIP refused"
    grep -qF 'gate test-assets [SKIP] test-assets: local-only fixture spec/assets/preview.webp absent' "$cert" || red 'SKIP evidence not retained'
    REQUIRED_GATES='test-one test-assets' sh "$check_tool" "$cert" HEAD > "$out/check.log" 2>&1 || { cat "$out/check.log"; red 'checker refused valid evidence'; }
    grep -qF 'cert-check: NOT PROVEN test-assets: SKIP, fixture absent on this host' "$out/check.log" || red 'checker hid NOT PROVEN'
    cp -f "$cert" "$out/good.cert"
done
for state in zero-count other silent skip-other red-last false-green; do
    printf '%s\n' "$state" > build/state
    run_cert
    [ "$rc" = 2 ] && [ ! -f "$cert" ] && grep -qF 'red gate: test-one' "$out/cert.log" || red "$state accepted"
done
printf 'green\n' > build/state
mkdir -p spec/assets
printf 'present\n' > spec/assets/preview.webp
run_cert
[ "$rc" = 2 ] && grep -qF 'red gate: test-assets' "$out/cert.log" || red 'false absent-fixture SKIP accepted'
rm -f spec/assets/preview.webp
sed 's/^gate test-one .*/gate test-one [SKIP] test-one: absent reference/' "$out/good.cert" > "$out/forged.cert"
rc=0
REQUIRED_GATES='test-one test-assets' sh "$check_tool" "$out/forged.cert" HEAD > "$out/check.log" 2>&1 || rc=$?
[ "$rc" = 2 ] && grep -qF 'non-green gate: test-one' "$out/check.log" || red 'checker accepted unrelated SKIP'
echo '>>> test-lane-cert-summaries: green (named GREEN/count; assets-only SKIP; nonzero, wrong target, quiet, zero checks, final RED refused)'
# Local integration lanes can be ahead of the published origin/main. The
# certificate must name the base cert-check compares with local main.
git switch -q main
git commit -q --allow-empty -m 'local integration base'
local_base=$(git rev-parse HEAD)
git switch -qC lane/summaries main
git commit -q --allow-empty -m 'local integration lane'
rc=0
LANE_CERT_BASE=main GATES='test-one test-assets' REQUIRED_GATES='test-one test-assets' \
    sh "$cert_tool" > "$out/cert.log" 2>&1 || rc=$?
[ "$rc" = 0 ] && grep -qxF "base_sha: $local_base" "$cert" || red 'explicit local main base ignored'
REQUIRED_GATES='test-one test-assets' sh "$check_tool" "$cert" HEAD > "$out/check.log" 2>&1 || { cat "$out/check.log"; red 'checker refused local integration base'; }
echo '>>> test-lane-cert-summaries: green (named verdicts, assets-only SKIP, explicit local base)'
