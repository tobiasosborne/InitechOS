#!/bin/sh
# oneshot/all.sh -- EXPERIMENT initech-7cbu8: build the four SAMIRX variants
# and run every oneshot acceptance leg against the variant that links its app.
# usage: sh harness/diff/dbf_diff/xbase_prog_diff/oneshot/all.sh   (needs make factory image)
set -u
here=harness/diff/dbf_diff/xbase_prog_diff/oneshot
for v in "A:browse edit" "B:create modcmd" "C:frmdesign lbldesign" "D:assist help"; do
    n=${v%%:*}
    make -s samirx-exe SAMIRX_EXE=build/SAMIRX_$n.EXE SAMIRX_UI="${v#*:}" > build/samirx_$n.log 2>&1 || { cat build/samirx_$n.log; exit 1; }
    grep '^>>>' build/samirx_$n.log || echo ">>> SAMIRX_$n.EXE up to date"
done
fail=0
# Two waves: a parent leg (browse) clears build/oneshot/browse, which holds
# its sub-legs' outputs (browse/edges), so parents run first.
for wave in "kit:A browse:A edit:A append:A create:B modcmd:B report:C label:C assist:D help:D" \
            "browse/edges:A browse/indexed:A browse/arrows:A create/memo:B create/modify:B modcmd/edit:B"; do
    pids=
    for leg in $wave; do
        app=${leg%%:*}; [ -f "$here/$app/KEYS" ] || { echo "SKIP: $app (no KEYS)"; continue; }
        ( sh "$here/run.sh" "$app" "build/SAMIRX_${leg##*:}.EXE" > "build/oneshot-$(echo "$app" | tr / _).log" 2>&1 ) &
        pids="$pids $!"
    done
    for p in $pids; do wait "$p" || fail=1; done
done
grep -h '^GREEN\|^RED' build/oneshot-*.log
exit $fail
