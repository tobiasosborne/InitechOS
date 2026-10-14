#!/bin/sh
# Thin shell counterpart of oracle_policy.h; called only for absent references.
set -eu
root=$1
detail=$2
case "$root" in
    SYSTEM7_DECOMP) skip=${SKIP_SYSTEM7_DECOMP:-0} ;;
    WIN31_DECOMP) skip=${SKIP_WIN31_DECOMP:-0} ;;
    DBASE3_DECOMP) skip=${SKIP_DBASE3_DECOMP:-0} ;;
    LOTUS123_DECOMP) skip=${SKIP_LOTUS123_DECOMP:-0} ;;
    WORDPERFECT51_DECOMP) skip=${SKIP_WORDPERFECT51_DECOMP:-0} ;;
    REFERENCE_TOOLS) skip=${SKIP_REFERENCE_TOOLS:-0} ;;
    *) echo "!!! missing_reference FAIL: unknown reference $root" >&2; exit 2 ;;
esac
if [ "$skip" = 1 ]; then
    echo "!!! $root SKIPPED (SKIP_$root=1 opt-out) -- reference NOT graded: $detail"
else
    echo "!!! $root FAIL: $detail; set SKIP_$root=1 to explicitly opt out" >&2
    exit 2
fi
