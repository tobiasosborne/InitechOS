#!/bin/sh
# Real R2.2 EDIT keys; uses goldens' DOS33/QEMU driver, no product code.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../../.." && pwd)
out=${1:?absolute scratch output}; runtime=${2:?runtime directory}
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
cp "$runtime/runtime.img" "$out/runtime.img"
s=$out/input.steps
printf 'sleep 4\nkey esc\n' > "$s"
row=0
while IFS='|' read -r name keys; do
 row=$((row+1))
 printf 'key f5\ntype A%s\nkey ret\ntype ABCDEFGHIJ\nkey ret\nkey f2\n' "$row" >> "$s"
 printf '%s\n' "$keys" | tr ',' '\n' >> "$s"
 printf 'shot %s\nkey ret\n' "$name" >> "$s"
done <<'CASES'
left|key left,type X
right|key left,key left,key right,type X
home|key home,type X
end|key home,key end,type X
btab|key shift-tab,type X
tab|key home,key tab,type X
ctrlleft|key ctrl-left,type X
ctrlright|key home,key ctrl-right,type X
delete|key left,key delete
backspace|key left,key backspace
overwrite|key left,key left,key insert,type X
insert|key home,key insert,key insert,type X
enddelete|key end,key delete,type X
startbackspace|key home,key backspace,type X
clampleft|key home,key shift-tab,key left,type X
clampright|key end,key tab,key right,type X
CASES
printf 'type /fsEDITKEYS\nkey ret\nsleep 0.5\nshot saved\nend\n' >> "$s"
"$out/drive" "$out" "$runtime/lotus-boot.img" - "$out/runtime.img" "$s"
mcopy -i "$out/runtime.img@@32256" ::LOTUS/EDITKEYS.WK1 "$out/EDITKEYS.WK1"
lotus=${LOTUS123_DECOMP:-"$HOME/Projects/lotus123-decomp"}
python3 "$lotus/tools/wk1_ref.py" "$out/EDITKEYS.WK1" > "$out/EDITKEYS.dump"
