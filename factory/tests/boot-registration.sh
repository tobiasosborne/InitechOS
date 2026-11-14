#!/bin/sh
# GOLDEN-MASTER item 1 and boot/README.md require boot in both complete registries.
set -eu
cd "${BOOT_REGISTRATION_ROOT:-$(dirname "$0")/../..}"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
bad=0
make -s help > "$work/host"
if ! awk '/^registered components:/ {for(i=3;i<=NF;i++)if($i=="boot")n++} END{exit n!=1}' "$work/host"; then
 echo 'FAIL boot registration: complete host list must contain boot exactly once';bad=1
fi
if make -s smoke-selection SMOKE_COMPONENTS='turbo apps/initechbase' > "$work/omitted" 2>&1; then
 echo 'FAIL boot registration: full smoke accepted omitted boot';bad=1
elif ! grep -F 'FAIL smoke: incomplete components' "$work/omitted" >/dev/null; then
 cat "$work/omitted";bad=1
fi
if ! make -s smoke-selection SMOKE_COMPONENTS='boot turbo apps/initechbase' > "$work/full" 2>&1; then
 echo 'FAIL boot registration: complete smoke list rejected';cat "$work/full";bad=1
fi
if [ "$bad" -eq 0 ]; then echo 'ok boot registration: host includes boot; full smoke requires boot';fi
test "$bad" -eq 0
