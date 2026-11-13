#!/bin/sh
# Replay the ORIGINAL audit batch, answering the newly supported question.
set -eu
out=build/del-b023
mkdir -p "$out"
img=$out/data.img
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 -N 338 ::
mmd -i "$img" ::T ::CASES
mcopy -i "$img" docs/audits/2026-10-23-dos/evidence/cases/B023.BAT ::CASES/B023.BAT
mcopy -i "$img" docs/audits/2026-10-23-dos/evidence/extracted/suite2/T/DELDIR/ONE.TXT ::T/ONE.TXT
printf 'SHELL=COMMAND.COM /E:512\r\n' > "$out/config"
mcopy -i "$img" "$out/config" ::CONFIG.SYS
build/qemu_harness --disk "$1" --disk2 "$img" --out "$out" --name run \
 --mouse 'kbsl,kc,ka,ks,ke,ks,kbsl,kb,k0,k2,k3,kdot,kb,ka,kt,kret,WAre you sure (Y/N)?,ky,kret,WAUDIT-DONE-B023,ke,kx,ki,kt,kret' \
 --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 30000 \
 --screendump --screendump-after SHELL-DONE > "$out/stdout" 2> "$out/report" || true
fail() { echo "RED del-b023: $*" >&2; exit 1; }
grep -q SHELL-DONE "$out/run.serial" || fail 'script did not finish'
grep -q AUDIT-DONE-B023 "$out/run.serial" || fail 'original batch sentinel missing'
grep -qF 'Are you sure (Y/N)?' "$out/run.serial" || fail 'directory confirmation missing'
if mdir -i "$img" ::T/DELDIR/ONE.TXT > "$out/deleted" 2>&1; then fail 'child file survived Y'; fi
mdir -i "$img" ::T/DELDIR > "$out/directory" || fail 'directory removed'
mcopy -i "$img" ::T/ONE.TXT - > "$out/root"
cmp "$out/root" docs/audits/2026-10-23-dos/evidence/extracted/suite2/T/DELDIR/ONE.TXT || fail 'source bytes changed'
fsck.fat -n "$img" > "$out/fsck" 2>&1 || fail 'FAT damage'
echo 'GREEN del-b023: original batch, Y deletes immediate file, directory/source retained, fsck.fat'
