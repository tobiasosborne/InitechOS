#!/bin/sh
# Run actual Lotus R2.2 through the goldens/ HMP/VGA capture tooling.
# Runtime provenance and DOS environment: goldens/dataloss/README.md.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
out=${1:?absolute scratch output directory}
mkdir -p "$out"
sh "$root/goldens/dataloss/runtime.sh" "$out/runtime"
cc -std=c11 -O2 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
s=$out/input.steps
cat > "$s" <<'STEPS'
sleep 4
key esc
type 123
key ret
type 11
key right
type 12
key down
type 22
key left
type 21
key ret
key f5
type D1
key ret
type 31
key right
type 32
key down
type 42
key left
type 41
key ret
type /wgpe
type /fsBASE
key ret
STEPS
# Each case starts from the same real, saved worksheet. Bnn/Ann are its
# before/after bytes. All source/destination format bits start protected.
case_steps() {
 id=$1; unlock=$2; command=$3
 printf 'type /frBASE\nkey ret\n' >> "$s"
 if [ -n "${4:-}" ]; then printf '%s\n' "$4" >> "$s"; fi
 if [ -n "$unlock" ]; then printf 'type /ru%s\nkey ret\n' "$unlock" >> "$s"; fi
 printf 'type /fsB%s\nkey ret\nshot B%s\n' "$id" "$id" >> "$s"
 printf '%s\n' "$command" >> "$s"
 printf 'shot R%s\nkey esc\ntype /fsA%s\nkey ret\n' "$id" "$id" >> "$s"
}
case_steps 01 '' 'type /cA1..A2
key ret
type D1
key ret'
case_steps 02 'D1' 'type /cA1..A2
key ret
type D1
key ret'
case_steps 03 'A1..A2' 'type /mA1..A2
key ret
type D1
key ret'
case_steps 04 'D1..D2' 'type /mA1..A2
key ret
type D1
key ret'
case_steps 05 'D1' 'type /rvA1..A2
key ret
type D1
key ret'
case_steps 06 'D1' 'type /rtA1..B1
key ret
type D1
key ret'
case_steps 07 'A1' 'type /reA1..A2
key ret'
case_steps 08 '' 'type /rff2
key ret
type A1..A2
key ret'
case_steps 09 '' 'type /ruA1..A2
key ret'
case_steps 10 '' 'key f5
type A1
key ret
type 99
key ret'
case_steps 11 '' 'key f5
type H1
key ret
type 99
key ret'
case_steps 12 '' 'key f5
type A1
key ret
key f2
key backspace
type 9
key ret'
# Labels for alignment and Name Labels are established by the earlier,
# retained dataloss/lotus/labels capture; this case erases the whole sheet.
case_steps 13 '' 'type /wey'
case_steps 14 'D1..E1' 'type /cA1..B2
key ret
type D1
key ret'
case_steps 15 'D1..E1' 'type /mA1..B2
key ret
type D1
key ret'
case_steps 16 'D1' 'type /cH1..H2
key ret
type D1
key ret'
case_steps 17 'D1' 'type /cA1
key ret
type D1..D2
key ret'
case_steps 18 'D1..E1' 'type /rtA1..B2
key ret
type D1
key ret'
case_steps 19 'A1' 'type /rlrA1..A2
key ret' 'type /wgpd
key f5
type A1
key ret
type '\''alpha
key ret
type '\''beta
key ret
type /wgpe'
# Range Label on numeric cells: does a protected non-label stop it?
case_steps 20 'A1' 'type /rlrA1..A2
key ret'
# Move onto an overlapping destination: protection checks use the original
# destination formats, even when an earlier write/erase touches the source.
case_steps 21 'A2' 'type /mA1..A2
key ret
type A2
key ret'
printf 'sleep 1\nend\n' >> "$s"
"$out/drive" "$out" "$out/runtime/lotus-boot.img" - "$out/runtime/runtime.img" "$s"
for id in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21; do
 for prefix in B A; do
  mcopy -i "$out/runtime/runtime.img@@32256" "::LOTUS/$prefix$id.WK1" "$out/$prefix$id.WK1"
 done
done
for f in "$out"/*.ppm; do convert "$f" "${f%.ppm}.png"; done
