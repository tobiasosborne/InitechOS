#!/bin/sh
# Build each registered application's real artifacts before minting the disk.
# This keeps app additions out of shared Makefile dependency lists.
set -eu
cd "$(dirname "$0")/.."
while read -r dir rest; do
 case "$dir" in ''|\#*) continue;; esac
 test -z "$rest"
 test -f "$dir/journey.sh" && test -f "$dir/verify.sh"
 app=${dir%/tests/smoke}
 test "$app" != "$dir"
 "${MAKE:-make}" -C "$app" all
done < factory/desktop-journeys.list
