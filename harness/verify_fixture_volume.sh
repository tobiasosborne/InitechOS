#!/bin/sh
# Ref: CLAUDE.md Rule 2; PRD Sec 8; initech-tflp.
# Expectations are explicit Makefile contracts, not inferred from an image.
# Each operand is an exact mtools path, optionally =flags (R/H/S/A).
set -eu
export LC_ALL=C
phase=$1
image=$2
shift 2
fail() {
    printf '!!! fixture-volume %s ERROR: %s: %s\n' "$phase" "$image" "$*" >&2
    exit 1
}
[ "$#" -gt 0 ] || fail 'no fixture contract declared'
command -v mdir >/dev/null 2>&1 || fail 'mtools mdir missing'
command -v mattrib >/dev/null 2>&1 || fail 'mtools mattrib missing'
[ -f "$image" ] || fail 'image missing'
for spec do
    path=${spec%%=*}
    flags=${spec#*=}
    [ "$flags" != "$spec" ] || flags=A
    # -a includes hidden/system fixtures. Check each exact path, never a glob
    # that could accept nineteen of twenty files. mattrib's flag column is
    # checked separately: a missing RO bit must fail before the emulator runs.
    mdir -a -b -i "$image" "::$path" >/dev/null 2>&1 \
        || fail "missing fixture ::$path"
    listing=$(mattrib -i "$image" "::$path" 2>&1) \
        || fail "cannot list attributes of ::$path: $listing"
    actual=$(printf '%s\n' "$listing" | sed 's/::.*//' | tr -d '[:space:]')
    [ "$actual" = "$flags" ] \
        || fail "fixture ::$path attributes '$actual', expected '$flags'"
done
