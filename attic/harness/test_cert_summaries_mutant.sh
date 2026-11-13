#!/bin/sh
# Rule 6: widen the real shared policy to accept every SKIP; it must be caught.
set -eu
out=$1
mkdir -p "$out/tools"
cp -f tools/lane-cert.sh tools/cert-check.sh "$out/tools/"
awk '
    {print}
    /^cert_summary_kind\(\) \{$/ {
        print "    case $2 in \047[SKIP] \047*) printf \047skip\\n\047; return 0;; esac"
    }
' tools/cert-summary.sh > "$out/tools/cert-summary.sh"
rc=0
sh harness/test_cert_summaries.sh "$out/tools/lane-cert.sh" "$out/tools/cert-check.sh" "$out/run" > "$out/mutant.log" 2>&1 || rc=$?
[ "$rc" = 1 ] && grep -qF 'RED cert-summaries: skip-other accepted' "$out/mutant.log" || { cat "$out/mutant.log"; exit 1; }
grep -m1 -F 'RED cert-summaries: skip-other accepted' "$out/mutant.log"
echo '>>> test-lane-cert-summaries-mutant: green (arbitrary SKIP acceptance rejected)'
# Restore the old fixed origin/main base; the local integration case must fail.
cp -f tools/cert-summary.sh "$out/tools/cert-summary.sh"
sed 's|base_ref=${LANE_CERT_BASE:-origin/main}|base_ref=origin/main|' tools/lane-cert.sh > "$out/tools/lane-cert.sh"
rc=0
sh harness/test_cert_summaries.sh "$out/tools/lane-cert.sh" "$out/tools/cert-check.sh" "$out/base-run" > "$out/base-mutant.log" 2>&1 || rc=$?
[ "$rc" = 1 ] && grep -qF 'RED cert-summaries: explicit local main base ignored' "$out/base-mutant.log" || { cat "$out/base-mutant.log"; exit 1; }
grep -m1 -F 'RED cert-summaries: explicit local main base ignored' "$out/base-mutant.log"
echo '>>> test-lane-cert-summaries-mutant: green (arbitrary SKIP and ignored local base rejected)'
