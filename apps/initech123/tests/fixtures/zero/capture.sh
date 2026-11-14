#!/bin/sh
# Real R2.2 Global Zero view-state probes, using only the independent driver.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../../.." && pwd)
runtime=${1:?runtime}; out=${2:?new absolute output}; scenario=${3:?scenario}
mkdir -p "$out"; cp "$runtime/runtime.img" "$out/runtime.img"
cc -std=c11 -O2 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
s=$out/input.steps
printf 'sleep 4\nkey esc\ntype /wgc12\nkey ret\n' > "$s"
row=0
while IFS= read -r value; do
 row=$((row+1));printf 'key f5\ntype A%s\nkey ret\n' "$row" >> "$s"
 case "$value" in @*) printf 'key shift-2\ntype %s\n' "${value#@}" >> "$s" ;; *) printf 'type %s\n' "$value" >> "$s" ;; esac
 printf 'key ret\n' >> "$s"
done <<'VALUES'
0
+A1
'0
0.05
0
+A1
0
0
0
@NA
@ERR
VALUES
cat >> "$s" <<'EOF'
type /rff0
key ret
type A4
key ret
type /rfhA5
key ret
type /rftA6..A7
key ret
type /rfdt1A8
key ret
type /rfd1A9
key ret
type /fsBEFORE
key ret
sleep 1
shot before
type /wgz
shot choices
EOF
case "$scenario" in
 yes) printf 'type y\n' >> "$s" ;;
 left|right|center|repeat|plain|empty|long)
  printf 'type l\nshot label_prompt\nkey backspace\n' >> "$s"
  case "$scenario" in
   left) printf "type 'NIL\n" >> "$s" ;;
   right) printf 'key shift-apostrophe\ntype NIL\n' >> "$s" ;;
   center) printf 'key shift-6\ntype NIL\n' >> "$s" ;;
   repeat) printf 'type \\ab\n' >> "$s" ;;
   plain) printf 'type NIL\n' >> "$s" ;;
   empty) : ;;
   long) printf "type 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOP\n" >> "$s" ;;
  esac
  printf 'key ret\n' >> "$s" ;;
 *) exit 2 ;;
esac
cat >> "$s" <<'EOF'
sleep 0.8
shot result
key esc
key esc
key esc
type /fsAFTER
key ret
sleep 1
type /wgzn
sleep 0.8
shot no_again
type /wgzl
sleep 0.8
shot reopened
key esc
sleep 0.8
shot cancel
end
EOF
"$out/drive" "$out" "$runtime/lotus-boot.img" - "$out/runtime.img" "$s"
for f in BEFORE AFTER; do mcopy -i "$out/runtime.img@@32256" "::LOTUS/$f.WK1" "$out/$f.WK1"; done
cmp "$out/BEFORE.WK1" "$out/AFTER.WK1"
