#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. factory/image/test-media.sh
legacy=${INITECH_LEGACY:-${LEGACY_TREE:-$HOME/Projects/InitechOS-legacy}}
mkdir -p build/parity
out=$(mktemp -d "$PWD/build/parity/run.XXXXXX")
echo "Parity evidence: $out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C SOURCE_DATE_EPOCH=315532800
build/factory/keys < factory/session.txt > "$out/keys"
finish=$(printf '\nsmoke-complete\r\n')
for variant in legacy product; do
 run=$out/$variant
 mkdir "$run"
 if [ "$variant" = legacy ]; then
  # The frozen tracer runs its original shell from A:. Its utility binaries
  # live in DOS on flair_data.img, selected by that disk's original AUTOEXEC.
  media_boot=$run/master.img media_data=$run/slave.img
  cp "$legacy/build/tracer_boot.img" "$media_boot"
  cp "$legacy/build/flair_data.img" "$media_data"
  media_system=A media_work=$media_data program='DOS\CHKDSK.COM'
 else
  media_prepare "${FACTORY_MEDIA_PAIR:-canonical}" with-data "$run"
  program=CHKDSK.COM
  if [ "$media_system" = A ]; then
   # Preserve the legacy A-system prompt for this text-mode session.
   printf '%s\r\n' '@ECHO OFF' 'PATH A:\;A:\APPS' > "$run/autoexec"
   mcopy -o -i "$media_work" "$run/autoexec" ::AUTOEXEC.BAT
  fi
 fi
 printf 'delete-me\r\n' > "$run/deleteme"
 mcopy -i "$media_work" "$run/deleteme" ::DELETEME.TXT
 mmd -i "$media_work" ::REMOVED
 media_readback qemu /dev/null
 cp "$media_volume" "$run/before.fat"
 media_run qemu build/factory/qemu_harness \
  --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect "$finish" \
  --quit-after "$finish" --timeout-ms 150000 --name run --out "$run" \
  > "$run/stdout" 2> "$run/report"
 media_readback qemu "$run/report"
 export PYTHONPATH=kernel/tests/disk
 python3 -B factory/tests/parity-fat.py dir "$run/before.fat" > "$run/dir"
 python3 -B factory/tests/parity-fat.py chkdsk "$media_volume" "$program" > "$run/chkdsk"
 awk -v drive="$media_system" -f factory/tests/parity-session.awk \
  "$run/dir" "$run/chkdsk" "$run/run.serial" > "$run/session.txt"
 for file in CHECK/PROOF.TXT CHECK/SAVED.TXT CHECK/RENAMED.TXT SORTED.TXT MORE.TXT FOUND.TXT; do
  mcopy -i "$media_volume" "::/$file" "$run/${file##*/}"
 done
done
diff -u "$out/legacy/session.txt" "$out/product/session.txt" > "$out/session.diff" || { cat "$out/session.diff"; exit 1; }
for name in PROOF SAVED RENAMED SORTED MORE FOUND; do
 cmp "$out/legacy/$name.TXT" "$out/product/$name.TXT"
done
echo 'PARITY: complete sessions match after independent DIR and CHKDSK checks; six persisted files match'
