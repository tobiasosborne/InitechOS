#!/bin/sh
# L118: stopped-image mtools/fsck judgment, independent of Finder's model.
set -eu
boot=$1
base=$2
trace=$3
name=locked_trash${4:-}
out=build/$name
mode=${5:-stage}
mkdir -p "$out"
fail() { printf 'FAIL locked-trash: %s\n' "$*" >&2; exit 1; }
img=$out/data.img
cp -f "$base" "$img"
if [ "$mode" = stage ]; then
    source=README.TXT
    destination=README.TXT
    pixel=locked
else
    source=APPS/TENANTFX.EXE
    destination=TRASH/APPS/TENANTFX.EXE
    pixel=locked-full
fi
mattrib -i "$img" +r "::$source"
mcopy -i "$img" "::$source" - > "$out/before.txt"
mattrib -i "$img" "::$source" | cut -c1-9 > "$out/before.attr"
build/qemu_harness --disk "$boot" --disk2 "$img" --name "$name" --out build \
    --mouse "$trace" --keys-after FLAIR-LIVE-READY --timeout-ms 15000 \
    --quit-after FINDER-LOCKED-ALERT --screendump --screendump-after FINDER-LOCKED-ALERT \
    > "$out/emu.report" 2>&1 || true
mcopy -i "$img" "::$destination" - > "$out/after.txt" 2>/dev/null || fail 'locked file left source'
cmp -s "$out/before.txt" "$out/after.txt" || fail 'locked bytes changed'
mattrib -i "$img" "::$destination" | cut -c1-9 > "$out/after.attr"
cmp -s "$out/before.attr" "$out/after.attr" || fail 'locked attributes changed'
if [ "$mode" = stage ]; then
    if mdir -i "$img" ::TRASH/README.TXT >/dev/null 2>&1; then fail 'locked file staged'; fi
    grep -qx 'FINDER-MOVE-REFUSED reason=locked name=README.TXT' "build/$name.serial" || fail 'protection-specific refusal missing'
else
    for leaf in README.TXT TRASH/README.TXT TRASH/APPS/CTENANT.EXE TRASH/APPS/123.EXE; do
        if mdir -i "$img" "::$leaf" >/dev/null 2>&1; then fail 'writable purge control survived'; fi
    done
    grep -qx 'FINDER-TRASH-EMPTIED purged=3 refused=2' "build/$name.serial" || fail 'mixed purge counts wrong'
fi
grep -qx 'FINDER-LOCKED-ALERT' "build/$name.serial" || fail 'visible message missing'
grep -q 'OK=1' "$out/emu.report" || fail 'emulator did not complete'
mcopy -i "$img" ::DESKTOP.DB - > "$out/desktop.db"
origins=0
[ "$mode" = stage ] || origins=1
build/dos_safety_fixture origins "$origins" < "$out/desktop.db" || fail 'origin records changed incorrectly'
build/ppm_flair_trash_check "$pixel" "build/$name.ppm" || fail 'locked notice pixels missing'
fsck.fat -n "$img" > "$out/fsck.txt" 2>&1 || fail 'fsck.fat found damage'
printf 'VERDICT: PASS -- test-flair-locked-%s (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)\n' "$mode"
