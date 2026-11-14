#!/bin/sh
# A real product layout perturbation must break the old positional journey,
# while name-based navigation still reaches the requested EXE on both CPUs.
# The native independent row-placement oracle kills the wrong layout itself;
# surviving navigation is a resilience property, not a fidelity certificate.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build/factory/navigation-mutants
work=$(mktemp -d "$root/build/factory/navigation-mutants/run.XXXXXX")
fail() {
 echo "FAIL navigation: $* (evidence: $work)" >&2
 echo 'factory: 1 mutants, 0 killed, 0 survived, 1 errors'
 exit 1
}
probe() {
 cpu=$1 style=$2 boot=$3 volume=$4 dir=$5 target=$6 expected=$7
 mkdir -p "$dir"
 cp "$volume" "$dir/data.img"
 app_entries=$(mdir -b -i "$volume" ::APPS/)
 markers=$dir/markers
 : > "$markers"
 trace=$(
  . factory/desktop-trace.sh
  inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
  inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
  inject m22:20
  if [ "$style" = position ]; then
   # Literal first APPS cell, before the one-slot shift. The complete disk
   # listing independently establishes that this is initially 123.EXE.
   inject 'l1,l0,kctrl-o'
  else launch_app "$target"; fi
 )
 trace=${trace%,}
 if [ "$style" = name ]; then trace=${trace%,W*}; fi
 printf '%s\n' "$trace" > "$dir/trace"
 if [ "$cpu" = qemu ]; then
  set -- build/factory/qemu_harness --mouse "$trace"
 else
  set -- build/factory/desktop-bochs --keys "$trace" --rtc-time0 699440400
 fi
 DESKTOP_SERIAL="$dir/run.serial" INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 "$@" --disk "$boot" --disk2 "$dir/data.img" \
  --keys-after FLAIR-LIVE-READY --expect "$expected" --quit-after "$expected" \
  --timeout-ms "${NAVIGATION_TIMEOUT_MS:-45000}" --name run --out "$dir" \
  > "$dir/stdout" 2> "$dir/report"
}
cp build/image/boot.img "$work/pristine-boot.img"
cp build/image/data.img "$work/pristine-data.img"
first=$(mdir -b -i "$work/pristine-data.img" ::APPS/ | sed -n '1p')
[ "$first" = '::/APPS/123.EXE' ] || fail 'positional control requires 123.EXE in the first cell'
probe qemu position "$work/pristine-boot.img" "$work/pristine-data.img" \
 "$work/pristine-position" 123.EXE 'FLAIR-DISPATCH app=123' || fail 'pristine positional baseline'
scratch=$work/source
mkdir -p "$scratch"
tar -cf - --exclude=./build --exclude=./.git --exclude=./.beads --exclude=./attic . | tar -xf - -C "$scratch"
mkdir -p "$scratch/build"
tar -cf - --exclude=./factory/gate-mutants --exclude=./factory/smoke-mutants \
 --exclude=./factory/navigation-mutants --exclude=./factory/mutants \
 --exclude=./factory-gates --exclude=./smoke* --exclude=./test -C build . | tar -xf - -C "$scratch/build"
patch --batch --fuzz=0 -p1 -d "$scratch" < factory/mutants/navigation/finder-slot-shift.patch \
 > "$work/patch.log" 2>&1 || fail 'slot-shift patch'
(cd "$scratch" && make -j2 -C flair/desktop test-build TESTS=test_finder_windows && make -j2 image) \
 > "$work/build.log" 2>&1 || fail 'shifted product build'
for pass in 1 2; do
 rc=0
 "$scratch/build/flair/desktop/host/test_finder_windows" > "$work/geometry-$pass.log" 2>&1 || rc=$?
 [ "$rc" -ne 0 ] && grep -F 'no hole where TRASH was' "$work/geometry-$pass.log" >/dev/null \
  || fail 'independent row-placement oracle did not reject the shifted product'
done
rc=0
probe qemu position "$scratch/build/image/boot.img" "$scratch/build/image/data.img" \
 "$work/shift-position" 123.EXE 'FLAIR-DISPATCH app=123' || rc=$?
[ "$rc" -ne 0 ] && grep -F 'marker_found=0 (expect=FLAIR-DISPATCH app=123)' "$work/shift-position/report" >/dev/null \
 || fail 'positional journey did not fail at its requested app marker'
for cpu in qemu bochs; do
 probe "$cpu" name "$scratch/build/image/boot.img" "$scratch/build/image/data.img" \
  "$work/shift-$cpu-123" 123.EXE 'FLAIR-DISPATCH app=123' || fail "$cpu shifted 123 by name"
 probe "$cpu" name "$scratch/build/image/boot.img" "$scratch/build/image/data.img" \
  "$work/shift-$cpu-word" WORD.EXE 'FLAIR-DISPATCH app=WORD' || fail "$cpu shifted offscreen Word by name"
 probe "$cpu" name "$work/pristine-boot.img" "$work/pristine-data.img" \
  "$work/pristine-$cpu-word" WORD.EXE 'FLAIR-DISPATCH app=WORD' || fail "$cpu pristine offscreen Word by name"
done
echo 'ok navigation: positional control RED after one-slot shift; names GREEN on QEMU + Bochs, including offscreen Word'
echo "factory: 1 mutants, 1 killed, 0 survived, 0 errors; navigation evidence: $work"
