#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../.."
. factory/desktop-trace.sh
out=build/apps/initechcalc/smoke
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
for emu in ${EMULATORS:-qemu bochs}; do
 cp build/image/data.img "$out/$emu-data.img"
 if [ "$emu" = qemu ]; then
  trace="WFLAIR-CLOCK 9:00 am,$open_disk,WFINDER-OPEN-VOLUME,$open_apps,WFINDER-OPEN-FOLDER,kc,ka,kl,kc,kdot,ke,kx,ke,kctrl-o,WFLAIR-DISPATCH app=CALC,k7,k3,kret,D$out/calc.ppm,kctrl-s,WTENANT-FILECOPY-DONE,w500,kctrl-q"
  set -- --mouse "$trace"
 else
  set -- --keys "$open_disk,w1000,$open_apps,w1000,kc,ka,kl,kc,kdot,ke,kx,ke,kctrl-o,w1000,k7,k3,kret,kctrl-q,w1000,kret"
 fi
 timeout 60s build/factory/${emu}_harness --disk build/image/boot.img --disk2 "$out/$emu-data.img" \
  --keys-after FLAIR-LIVE-READY "$@" --expect 'TENANT-EXIT rc=0 via=exit' \
  --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 45000 --name "$emu" --out "$out" \
  > "$out/$emu.stdout" 2> "$out/$emu.report"
 grep -F 'FLAIR-DISPATCH app=CALC' "$out/$emu.serial"
 grep -F 'TENANT-FILECOPY-DONE' "$out/$emu.serial"
 grep -F 'triple_fault=0' "$out/$emu.report"
 mcopy -o -i "$out/$emu-data.img" ::APPS/TABLES.TXT "$out/$emu.txt"
 # Literal PRD rows, first label edited to 73; preserve the unknown amounts.
 printf 'INITECH LEDGER\r\n73               TODO_FRAME\r\nMatte Cutters    TODO_FRAME\r\nExacto Blades    TODO_FRAME\r\n570- format      570-\r\nTOTAL            $112,276.87\r\n' > "$out/expected.txt"
 cmp "$out/expected.txt" "$out/$emu.txt"
 mcopy -i "$out/$emu-data.img" ::APPS/LEDGER.TXT - | cmp - apps/initechcalc/data/LEDGER.TXT
 if mdir -b -i "$out/$emu-data.img" ::APPS | grep -E 'TABLE.*\.TMP$'; then exit 1; fi
 fsck.fat -n "$out/$emu-data.img" > "$out/$emu.fsck"
 echo "ok $emu: InitechCalc saved edited worksheet bytes, original ledger intact, stage removed"
done
