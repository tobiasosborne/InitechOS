#!/bin/sh
# Captured OEM DOS 3.30 stdout and whole-volume effects; independent mtools.
set -eu
ref=dos/command/tests/dos33/reference
out=build/copy-full
mkdir -p "$out"
fail() { echo "RED copy-full: $*" >&2; exit 1; }
for scenario in concat append appendpartial final self; do
 d=$out/$scenario; mkdir -p "$d"
 gzip -dc "$ref/$scenario/data.img.gz" > "$d/expected.img"
 cp "$d/expected.img" "$d/actual.img"
 if [ "$scenario" != self ]; then
  mcopy -i "$d/actual.img" "$ref/$scenario/files/OLD.BIN" ::TARGET.BIN
 fi
 build/shell_files "$d/actual.img" "$ref/$scenario/COMMAND.TXT" > "$d/stdout" 2> "$d/stderr"
 cmp "$d/stdout" "$ref/$scenario/COPY.OUT" || fail "$scenario stdout differs from real DOS"
 for side in expected actual; do
  mdir -b -i "$d/$side.img" :: | sort > "$d/$side.list"
  mdir -i "$d/$side.img" :: | tail -2 > "$d/$side.free"
 done
 cmp "$d/expected.list" "$d/actual.list" || fail "$scenario surviving names"
 cmp "$d/expected.free" "$d/actual.free" || fail "$scenario allocation/free bytes"
 for f in SOURCE.BIN OLD.BIN FILLER.BIN; do
  mtype -i "$d/actual.img" "::$f" > "$d/got"
  mtype -i "$d/expected.img" "::$f" > "$d/want"
  cmp "$d/got" "$d/want" || fail "$scenario $f bytes"
 done
 done
# Original independent captures: new/overwrite, /B, zero free and /A final EOF.
for scenario in new overwrite newbinary overwritebinary newzero overwritezero ascii asciifinal; do
 r=$SOURCE_ROOT/goldens/dataloss/dos/$scenario
 d=$out/$scenario; mkdir -p "$d"
 gzip -dc "$r/data.img.gz" > "$d/actual.img"
 # Source lives on the reference's A:; put it on our sole mounted volume,
 # exchanging the same number of filler clusters to keep free bytes identical.
 mdel -i "$d/actual.img" ::FILLER.BIN
 source=$SOURCE_ROOT/goldens/dataloss/dos/SOURCE.BIN
 [ "$scenario" != asciifinal ] || source=$r/SHORT.BIN
 mcopy -i "$d/actual.img" "$source" ::SOURCE.BIN
 old=0; case $scenario in overwrite*) old=4096; mcopy -i "$d/actual.img" "$SOURCE_ROOT/goldens/dataloss/dos/OLD.BIN" ::TARGET.BIN;; esac
 free=8192; case $scenario in *zero) free=0;; esac
 size=$(wc -c < "$source")
 truncate -s "$((1457664-size-old-free))" "$d/filler"
 mcopy -i "$d/actual.img" "$d/filler" ::FILLER.BIN
 mode=; case $scenario in *binary) mode=/B;; ascii*) mode=/A;; esac
 printf 'COPY SOURCE.BIN TARGET.BIN %s\n' "$mode" > "$d/command"
 build/shell_files "$d/actual.img" "$d/command" > "$d/stdout" 2> "$d/stderr"
 cmp "$d/stdout" "$r/COPY.OUT" || fail "$scenario stdout differs from pinned capture"
 mdir -i "$d/actual.img" :: | tail -2 > "$d/free"
 tail -2 "$r/after.mdir" > "$d/reference-free"
 cmp "$d/free" "$d/reference-free" || fail "$scenario free clusters differ from capture"
 if mdir -i "$d/actual.img" ::TARGET.BIN >/dev/null 2>&1; then fail "$scenario retained target"; fi
 mtype -i "$d/actual.img" ::SOURCE.BIN > "$d/got"
 cmp "$d/got" "$source" || fail "$scenario source changed"
done
echo 'GREEN copy-full: thirteen DOS 3.30 cases, exact stdout, survivors, bytes and free clusters'
