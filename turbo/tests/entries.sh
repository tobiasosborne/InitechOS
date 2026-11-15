#!/bin/sh
# Run turbo/seed or turbo/compiler entries under the shared host-test status
# protocol (mk/host-test.mk), so factory/testlib/run-mutants.sh can grade
# their mutants (initech-fytwe). Usage: entries.sh COMPONENT OUT RUNNER ENTRY...
set -u
component=$1 out=$2 runner=$3
shift 3
[ "$#" -gt 0 ] || { echo "FAIL $component: empty test registration/selection"; exit 1; }
mkdir -p "$out/host"
rm -f "$out"/host/*.result "$out"/host/*.assertion
start=$(date +%s)
n=0 failed=0
for entry do
    n=$((n+1))
    sh "$TOP/factory/testlib/run-test.sh" "$entry" "${TEST_TIMEOUT_S:-20}" '' "$out/host/$entry" \
        sh "$TOP/turbo/tests/entry.sh" "$runner" "$out" "$entry" "$out/host/$entry.log" || failed=$((failed+1))
done
elapsed=$(($(date +%s)-start))
echo "INITECH_TEST_RUN $n"
echo "$component: $n tests, $failed failures (${elapsed}s; budget ${TEST_BUDGET_S:-30}s)"
if [ "$elapsed" -gt "${TEST_BUDGET_S:-30}" ]; then
    echo 'FAIL budget: speed the suite up or raise TEST_BUDGET_S with a reason in the component README'
    failed=$((failed+1))
fi
test "$failed" -eq 0
