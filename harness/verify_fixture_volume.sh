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
    # initech-a5sr: in the recipe phase the image is the make target being
    # built. Leaving it on disk let the NEXT make run call a half-built,
    # contract-less volume up to date (a mutant gate went GREEN). GNU make's
    # .DELETE_ON_ERROR cannot be scoped to these recipes, so delete here.
    if [ "$phase" = recipe ] && [ -e "$image" ]; then
        rm -f -- "$image"
        printf '!!! fixture-volume recipe: removed %s so the next make rebuilds it\n' "$image" >&2
    fi
    exit 1
}
[ "$#" -gt 0 ] || fail 'no fixture contract declared'
command -v mdir >/dev/null 2>&1 || fail 'mtools mdir missing'
command -v mattrib >/dev/null 2>&1 || fail 'mtools mattrib missing'
[ -f "$image" ] || fail 'image missing'
# A boot-partition fixture carries its FAT at a byte offset (DEC-07a: the
# flagship boot volume at LBA 63 = 32256); the contract names it first as
# an @@offset operand (mtools -i image@@offset syntax). initech-e9sx.
offset=
for spec do
    case $spec in @@*) offset=$spec; continue;; esac
    path=${spec%%=*}
    flags=${spec#*=}
    [ "$flags" != "$spec" ] || flags=A
    # -a includes hidden/system fixtures. Check each exact path, never a glob
    # that could accept nineteen of twenty files. mattrib's flag column is
    # checked separately: a missing RO bit must fail before the emulator runs.
    mdir -a -b -i "$image$offset" "::$path" >/dev/null 2>&1 \
        || fail "missing fixture ::$path"
    listing=$(mattrib -i "$image$offset" "::$path" 2>&1) \
        || fail "cannot list attributes of ::$path: $listing"
    actual=$(printf '%s\n' "$listing" | sed 's/::.*//' | tr -d '[:space:]')
    [ "$actual" = "$flags" ] \
        || fail "fixture ::$path attributes '$actual', expected '$flags'"
done
