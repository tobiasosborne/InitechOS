#!/bin/sh
set -eu
out=$1 root=${TURBO_TESTS_ROOT:?}
. "$root/checks.sh"
work=$out/progress-plan
setup mkdir -p "$work"
printf 'ret,ret,r,u,n,ret\n' > "$work/base"
cat > "$work/cases" <<'EOF'
P001 tiny seed
P002 tiny hosted
P003 tiny resident
EOF
printf 'ret,ret,r,u,n,ret,Wend-P001,Wend-P002,Wend-P003,Wturbo-execution-complete\n' > "$work/expected"
plan() { sh "$root/execution/keys.sh" "$work/base" "$1" > "$work/actual" 2> "$work/error"; }
assert 'accept completed-case plan' plan "$work/cases"
assert 'wait for each exact completion in order' cmp "$work/expected" "$work/actual"
: > "$work/empty"
assert_not 'reject empty coverage' plan "$work/empty"
printf 'P001 tiny seed\nP003 tiny hosted\n' > "$work/gap"
assert_not 'reject skipped case' plan "$work/gap"
printf 'P001 tiny unknown\n' > "$work/bad-path"
assert_not 'reject unknown path' plan "$work/bad-path"
finish
