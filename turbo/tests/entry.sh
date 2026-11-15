#!/bin/sh
# Preserve only evidence emitted by actual assertion sites. A completed last
# summary must agree with the exit status; never synthesize one from exit 1.
runner=$1 out=$2 entry=$3 log=$4
rc=0
sh "$runner" "$out" "$entry" > "$log" 2>&1 || rc=$?
case $rc in 0|1) ;; *) exit "$rc";; esac
if ! awk -v status="$rc" '
NF { last=$0 }
END {
    if (last ~ /^[0-9]+ checks, [0-9]+ (failures|failed|failure\(s\))$/) {
        split(last,a," "); checks=a[1]+0; failures=a[3]+0
    } else if (last ~ /^INITECH_CHECKS [0-9]+ failures=[0-9]+$/) {
        split(last,a," "); checks=a[2]+0; split(a[3],b,"="); failures=b[2]+0
    } else exit 1
    if (checks<1 || (status==0 && failures!=0) || (status==1 && failures<1)) exit 1
}' "$log"; then
    echo "ERROR incomplete assertions: $entry (exit $rc)" >> "$log"
    exit 2
fi
exit "$rc"
