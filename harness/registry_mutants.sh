#!/bin/sh
# Ref: initech-q8j, CLAUDE.md Rule 6. Mutate source fixtures, never the checkout.
set -eu
checker=$(realpath "$1")
work=$(mktemp -d "${TMPDIR:-/tmp}/initech-registries-XXXXXX")
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM
setdir=$work/os/samir/cmd
mint=$work/harness/diff/dbf_diff/xbase_prog_diff/fns-mint
mkdir -p "$setdir" "$mint/accepted" "$mint/pending" "$mint/evidence"
cp -f .gitattributes "$work/.gitattributes"
cp -f os/samir/cmd/set_options.def "$work/base.def"
cp -f "$work/base.def" "$setdir/set_options.def"
: > "$mint/accepted/ACCEPTED"
: > "$mint/pending/PENDING"
"$checker" "$work"
expect_red() {
    label=$1; diagnostic=$2; rc=0
    "$checker" "$work" > "$work/check.log" 2>&1 || rc=$?
    [ "$rc" -eq 1 ] && grep -qF "$diagnostic" "$work/check.log" || {
        cat "$work/check.log"
        printf '!!! registry mutant survived or failed infrastructure: %s (exit %s)\n' "$label" "$rc" >&2
        exit 1
    }
    printf '>>> registry mutant %s correctly RED: ' "$label"
    cat "$work/check.log"
}
# Reproduce the actual conflict resolution: both valid additions survive union.
cp -f "$work/base.def" "$work/ours.def"
cp -f "$work/base.def" "$work/theirs.def"
printf 'SET_OPT("DEMO", set_opt_exact)\n' >> "$work/ours.def"
printf 'SET_OPT("DEMO", set_opt_safety)\n' >> "$work/theirs.def"
git merge-file --union -p "$work/ours.def" "$work/base.def" "$work/theirs.def" > "$setdir/set_options.def"
expect_red union-DEMO 'duplicate SET key DEMO'
cp -f "$work/base.def" "$setdir/set_options.def"
for row in 'SET_OPT(DEMO, set_opt_exact)' \
    'SET_OPT("DEMO", set_opt_exact) trailing' \
    'SET_OPT("DEMO", set_opt_exact) SET_OPT("OTHER", set_opt_safety)' \
    'SET_OPT("DEMO\0OTHER", set_opt_exact)' \
    'SET_OPT("D/*comment*/EMO", set_opt_exact)' \
    'SET_OPT("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEF", set_opt_exact)' \
    '#include "other.def"'; do
    cp -f "$work/base.def" "$setdir/set_options.def"
    printf '%s\n' "$row" >> "$setdir/set_options.def"
    expect_red malformed-row 'malformed SET_OPT row'
done
cp -f "$work/base.def" "$setdir/set_options.def"
printf '/* unterminated\n' >> "$setdir/set_options.def"
expect_red comment 'unterminated comment'
cp -f "$work/base.def" "$setdir/set_options.def"
printf '\000hidden suffix\n' >> "$setdir/set_options.def"
expect_red NUL 'invalid source byte'
cp -f "$work/base.def" "$setdir/set_options.def"
: > "$mint/accepted/PENDING"
expect_red both-markers 'both ACCEPTED and PENDING markers'
rm -f "$mint/accepted/PENDING"
printf 'not a marker\n' > "$mint/pending/PENDING"
expect_red nonempty-marker 'marker must be a regular empty file'
rm -f "$mint/pending/PENDING"
mkdir "$mint/pending/PENDING"
expect_red directory-marker 'marker must be a regular empty file'
rmdir "$mint/pending/PENDING"
ln -s ../accepted/ACCEPTED "$mint/pending/PENDING"
expect_red symlink-marker 'marker must be a regular empty file'
rm -f "$mint/pending/PENDING"
: > "$mint/pending/PENDING"
mkdir "$work/linked-case"
: > "$work/linked-case/ACCEPTED"
: > "$work/linked-case/PENDING"
ln -s "$work/linked-case" "$mint/linked"
expect_red symlink-case 'case directory must not be a symlink'
rm -f "$mint/linked"
printf 'future/*.def merge=union\n' >> "$work/.gitattributes"
expect_red unsupported-union 'union path has no registry validator'
cp -f .gitattributes "$work/.gitattributes"
"$checker" "$work"
printf '>>> test-samir-registries-mutant: green (union duplicate, malformed rows, markers, unknown union schema)\n'
