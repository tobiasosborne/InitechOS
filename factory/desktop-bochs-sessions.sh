#!/bin/sh
# A bounded ordinary Bochs boot per group, one shared persisted working volume.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
boot=$1 data=$2 out=$3
shift 3
mkdir -p "$out"
out=$(cd "$out" && pwd)
fail() { echo "FAIL Bochs desktop sessions: $* ($out)" >&2; exit 1; }
session_timeout=${DESKTOP_SMOKE_TIMEOUT_MS:-90000}
case $session_timeout in *[!0-9]*|0) fail "invalid session timeout";; esac
[ "$session_timeout" -gt 0 ] 2>/dev/null || fail "invalid session timeout"
[ "$session_timeout" -le 2147483647 ] 2>/dev/null || fail "session timeout overflow"
journeys=$*
registered=$(sed '/^[[:space:]]*#/d; /^[[:space:]]*$/d' factory/desktop-journeys.list)
sh factory/smoke-selection.sh desktop-journeys "$journeys" "$registered"
sh factory/desktop-sessions.sh "$@" > "$out/bochs-sessions.list"
printf '%s\n' "$@" > "$out/selected.list"
awk '{for(i=1;i<=NF;i++)print $i}' "$out/bochs-sessions.list" > "$out/planned.list"
sort "$out/selected.list" > "$out/selected.sorted"
sort "$out/planned.list" > "$out/planned.sorted"
cmp -s "$out/selected.sorted" "$out/planned.sorted" || fail 'missing or repeated journey in session plan'
cp "$data" "$out/bochs-data.img"
for file in serial stdout report markers; do : > "$out/bochs.$file"; done
printf 'session elapsed_s limit_ms exits journeys\n' > "$out/bochs-times.txt"
. factory/desktop-trace.sh
emulator=bochs
: > "$out/executed.list"
index=0 total_exits=0
while IFS= read -r group; do
 [ -n "$group" ] || fail 'empty group'
 index=$((index+1))
 run=$out/bochs-sessions/$index
 mkdir -p "$run"
 markers=$run/bochs.markers
 : > "$markers"
 exits=0
 for dir in $group; do
  [ -f "$dir/journey.sh" ] && [ -f "$dir/verify.sh" ] || fail "incomplete journey $dir"
  count=$(journey_exits=1; . "./$dir/journey.sh"; printf '%s' "$journey_exits")
  exits=$((exits+count))
 done
 [ "$exits" -gt 0 ] || fail 'empty exit receipt'
 app_entries=$(mdir -b -i "$out/bochs-data.img" ::APPS/)
 trace=$(
  inject "$open_disk"; await_marker FINDER-OPEN-VOLUME
  inject m-100:0; inject m100:0
  inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
  inject m22:20
  for dir in $group; do ( . "./$dir/journey.sh"; journey ); done
 )
 trace=${trace%,}
 trace=${trace%,WTENANT-EXIT rc=0 via=exit}
 printf '%s\n' "$trace" > "$run/trace.txt"
 cp "$out/bochs-data.img" "$run/data.img"
 begun=$(date +%s)
 rc=0
 DESKTOP_SERIAL="$run/bochs.serial" INITECH_TEST_RAM_MB=8 \
 "$root/build/factory/desktop-bochs" --disk "$boot" --disk2 "$run/data.img" \
 --keys-after FLAIR-LIVE-READY --rtc-time0 699440400 --allow-reboot --keys "$trace" \
 --expect 'TENANT-EXIT rc=0 via=exit' --quit-after 'TENANT-EXIT rc=0 via=exit' \
 --quit-after-count "$exits" --timeout-ms "$session_timeout" \
 --name bochs --out "$run" > "$run/bochs.stdout" 2> "$run/bochs.report" || rc=$?
 took=$(($(date +%s)-begun))
 printf '%s %s %s %s %s\n' "$index" "$took" "$session_timeout" "$exits" "$group" >> "$out/bochs-times.txt"
 for file in serial stdout report markers; do
  [ ! -f "$run/bochs.$file" ] || cat "$run/bochs.$file" >> "$out/bochs.$file"
 done
 [ "$rc" -eq 0 ] || { cat "$run/bochs.report"; fail "session $index exit $rc"; }
 grep -Fx VBE-MODE=0101 "$run/bochs.serial" >/dev/null || fail "session $index VGA mode"
 grep -Fx '[bochs] triple_fault=0' "$run/bochs.report" >/dev/null || fail "session $index CPU fault"
 sort "$markers" > "$run/markers.sorted"
 uniq -c "$run/markers.sorted" > "$run/marker-counts"
 while read -r count marker; do
  seen=$(grep -Fc "$marker" "$run/bochs.serial" || :)
  [ "$seen" -ge "$count" ] || fail "session $index missing $marker ($seen/$count)"
 done < "$run/marker-counts"
 cp "$run/data.img" "$out/bochs-data.img"
 for dir in $group; do printf "%s\n" "$dir" >> "$out/executed.list"; done
 total_exits=$((total_exits+exits))
 echo "ok Bochs desktop session $index: ${took}s, $exits normal exits, $group"
done < "$out/bochs-sessions.list"
[ "$index" -gt 0 ] || fail "no sessions executed"
sort "$out/executed.list" > "$out/executed.sorted"
cmp -s "$out/selected.sorted" "$out/executed.sorted" || fail "incomplete executed journey receipts"
echo "Bochs desktop sessions: $index completed, $total_exits normal exits; hard limit ${session_timeout}ms per boot"
