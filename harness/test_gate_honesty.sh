#!/bin/sh
# Ref: CLAUDE.md Law 2; initech-iv8c. Exercise REAL gates, not a policy mock.
set -eu
make_cmd=$1
makefile=$2
out=$3
shift 3
mkdir -p "$out/empty"
# Ignore inherited opt-outs: each negative leg must prove the default policy.
unset SKIP_SYSTEM7_DECOMP SKIP_WIN31_DECOMP SKIP_DBASE3_DECOMP SKIP_LOTUS123_DECOMP SKIP_REFERENCE_TOOLS
for pair in SYSTEM7_DECOMP:test-chicago-metrics WIN31_DECOMP:test-region-gdi DBASE3_DECOMP:test-samir-dbf-ref LOTUS123_DECOMP:test-i123-wk1-roundtrip; do
    root=${pair%%:*}
    gate=${pair#*:}
    skip=SKIP_$root
    log=$out/$root.fail.log
    if "$make_cmd" --no-print-directory -j1 -f "$makefile" "$gate" "BUILD=$out/$root" "$root=$out/empty" "$skip=0" "$@" >"$log" 2>&1; then
        echo "!!! test-gate-honesty FAIL: $gate accepted missing $root"
        cat "$log"
        exit 1
    fi
    grep -F "$root FAIL:" "$log" >/dev/null && grep -F "$skip=1" "$log" >/dev/null || {
        echo "!!! test-gate-honesty FAIL: $gate failed without naming $root and $skip=1"
        cat "$log"
        exit 1
    }
    log=$out/$root.skip.log
    "$make_cmd" --no-print-directory -j1 -f "$makefile" "$gate" "BUILD=$out/$root" "$root=$out/empty" "$skip=1" "$@" >"$log" 2>&1 || {
        cat "$log"; exit 1;
    }
    grep -F "$root SKIPPED ($skip=1 opt-out)" "$log" >/dev/null || {
        echo "!!! test-gate-honesty FAIL: $gate opt-out was not shouting"
        cat "$log"
        exit 1
    }
    echo ">>> test-gate-honesty: $root missing RED; $skip=1 explicitly SKIPPED"
done
echo '>>> test-gate-honesty: green (4 real corpus gates reject missing references; 4 explicit opt-outs shout)'
