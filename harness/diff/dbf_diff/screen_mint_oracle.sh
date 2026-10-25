#!/bin/sh
# Recompute the current independent oracle; never trust a recorded hash alone.
set -eu
export LC_ALL=C TZ=UTC
root=$(pwd)
mode=check
if [ "${1:-}" = --write ]; then mode=write; shift; fi
case_dir=$(cd "${1:?case directory required}" && pwd)
work=$(mktemp -d "$root/build/screen-oracle.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
ref=$(cd "${DBASE3_DECOMP:-../dbase3-decomp}" && pwd)
"$root/build/screen_tool" compile "$case_dir/KEYS" "$case_dir/DUMPS" "$work/EVENTS.BIN"
(
    for f in DBASE.EXE DBASEINL.OVL DBASE.MSG DBASE.OVL; do
        sha256sum < "$ref/mint/work/$f" | sed "s/  -$/  $f/"
    done
    sha256sum < "$root/build/SCREEN.COM" | sed 's/  -$/  SCREEN.COM/'
    (cd "$work" && sha256sum EVENTS.BIN)
    sha256sum < "$(command -v dosbox-x)" | sed 's/  -$/  dosbox-x/'
    sha256sum < "$root/build/screen_tool" | sed 's/  -$/  screen_tool/'
    sha256sum < "$root/harness/diff/dbf_diff/screen_mint.sh" | sed 's/  -$/  screen_mint.sh/'
    sha256sum < "$root/harness/diff/dbf_diff/screen_mint_oracle.sh" | sed 's/  -$/  screen_mint_oracle.sh/'
) > "$work/ORACLE.sha256"
if [ "$mode" = write ]; then
    cp -f "$work/ORACLE.sha256" "${2:?output manifest required}"
elif ! cmp -s "$work/ORACLE.sha256" "$case_dir/ORACLE.sha256"; then
    printf 'RED: oracle provenance mismatch: %s\n' "$(basename "$case_dir")" >&2
    diff -u "$case_dir/ORACLE.sha256" "$work/ORACLE.sha256" >&2 || :
    exit 1
fi
