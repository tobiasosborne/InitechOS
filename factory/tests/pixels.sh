#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
${HOSTCC:-cc} -std=c11 -Wall -Wextra -Werror factory/tests/pixels-fixture.c -o "$work/fixture"
for mode in black white point bad-mask; do "$work/fixture" "$mode" "$work/$mode.ppm"; done
cmp=build/factory/pixels-desktop
"$cmp" "$work/black.ppm" "$work/black.ppm"
"$cmp" "$work/black.ppm" "$work/white.ppm" "$work/black.ppm" > "$work/empty" 2>&1 && exit 1
for file in white point; do
 if "$cmp" "$work/black.ppm" "$work/$file.ppm" > "$work/result" 2>&1; then
  echo "FAIL pixels accepted $file difference"; exit 1
 fi
 grep -F '(ENFORCED)' "$work/result"
done
"$cmp" "$work/black.ppm" "$work/white.ppm" "$work/point.ppm" > "$work/masked" 2>&1 && exit 1
"$cmp" "$work/black.ppm" "$work/point.ppm" "$work/point.ppm" > "$work/masked" 2>&1 && exit 1
"$cmp" "$work/black.ppm" "$work/black.ppm" "$work/bad-mask.ppm" > "$work/masked" 2>&1 && exit 1
grep -F 'mask must be binary RGB' "$work/masked"
"$cmp" --informational "$work/black.ppm" "$work/white.ppm" | grep -F 'INFORMATIONAL:'
printf 'P6\n640 480\n255\n' > "$work/truncated.ppm"
if "$cmp" "$work/black.ppm" "$work/truncated.ppm"; then echo 'FAIL pixels accepted truncated image'; exit 1; fi
echo 'ok pixels: whole-frame mismatch, outside-old-crops pixel, masked mismatch, empty mask, truncation rejected'
