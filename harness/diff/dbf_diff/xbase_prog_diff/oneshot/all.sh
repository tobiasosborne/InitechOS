#!/bin/sh
# oneshot/all.sh -- EXPERIMENT initech-7cbu8: build the four SAMIRX variants
# and run every oneshot acceptance leg against the variant that links its app.
# usage: sh harness/diff/dbf_diff/xbase_prog_diff/oneshot/all.sh   (needs make factory image)
set -u
here=harness/diff/dbf_diff/xbase_prog_diff/oneshot
for v in "A:browse edit" "B:create modcmd" "C:frmdesign lbldesign" "D:assist help"; do
    n=${v%%:*}
    make -s samirx-exe SAMIRX_EXE=build/SAMIRX_$n.EXE SAMIRX_UI="${v#*:}" 2>&1 | grep -E '^>>>|rror' || exit 1
done
fail=0; pids=
for leg in kit:A browse:A browse/edges:A browse/indexed:A edit:A append:A create:B create/memo:B create/modify:B \
           modcmd:B report:C label:C assist:D help:D; do
    app=${leg%%:*}; [ -f "$here/$app/KEYS" ] || { echo "SKIP: $app (no KEYS)"; continue; }
    ( sh "$here/run.sh" "$app" "build/SAMIRX_${leg##*:}.EXE" > "build/oneshot-$(echo "$app" | tr / _).log" 2>&1 ) &
    pids="$pids $!"
done
for p in $pids; do wait "$p" || fail=1; done
grep -h '^GREEN\|^RED' build/oneshot-*.log
exit $fail
