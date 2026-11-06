#!/bin/sh
# oneshot/run.sh <app> [SAMIRX.EXE] -- EXPERIMENT initech-7cbu8 acceptance leg.
# Boots build/tracer_secondary.img in QEMU with a data volume holding SAMIRX.EXE
# (copied as SAMIR.EXE/.OVL) + oneshot/<app>/SETUP/*, starts SAMIR at the DOS
# prompt, replays oneshot/<app>/KEYS (one qemu_harness --mouse token per line:
# k<qcode> keystroke e.g. ka kret kspc kup kdown kleft kright kpgup kpgdn kesc
# ktab kbackspace khome kend kdelete kinsert kctrl-w kctrl-end kshift-a ;
# w<ms> wait ; D<name> dump screen to OUT/<name>.ppm ; T<text> types text,
# lowercase a-z0-9 space . only, expanded here), then copies every file of the
# data volume back to OUT/files/ and runs oneshot/<app>/CHECK (sh, cwd = OUT,
# exit 0 = data effect as expected) if present.
# OUT = build/oneshot/<app>. Requires: make samirx-exe build/screen_capture
# build/qemu_harness build/tracer_secondary.img build/samir_list.img
set -u
app=$1; exe=${2:-build/SAMIRX.EXE}; ovl=${exe%.EXE}.OVL
here=harness/diff/dbf_diff/xbase_prog_diff/oneshot
src=$here/$app; out=build/oneshot/$app
[ -f "$src/KEYS" ] || { echo "RED: $src/KEYS missing"; exit 2; }
mkdir -p "$out"; find "$out" -mindepth 1 -delete
cp -f build/samir_list.img "$out/data.img"
mcopy -o -i "$out/data.img" "$exe" ::SAMIR.EXE && mcopy -o -i "$out/data.img" "$ovl" ::SAMIR.OVL || exit 2
for f in "$src"/SETUP/*; do [ -f "$f" ] || continue; mcopy -o -i "$out/data.img" "$f" ::"$(basename "$f")" || exit 2; done
spec='ks,ka,km,ki,kr,kret,w2000'
while IFS= read -r tok; do
    case $tok in ''|'#'*) continue;; esac
    case $tok in
    T*) t=${tok#T}; i=0
        while [ $i -lt ${#t} ]; do i=$((i+1)); c=$(printf '%s' "$t" | cut -c$i)
            case $c in ' '|'') spec="$spec,kspc";; .) spec="$spec,kdot";; *) spec="$spec,k$c";; esac; done ;;
    D*) spec="$spec,w600,D$out/${tok#D}.ppm" ;;
    *) spec="$spec,$tok" ;;
    esac
done < "$src/KEYS"
spec="$spec,w800"
INITECH_QMP_STDIO=1 INITECH_TEXT_SCREEN_TOOL=build/screen_capture \
INITECH_TEXT_SCREEN_ELF=build/kernel_shell_secondary.elf \
build/qemu_harness --disk build/tracer_secondary.img --disk2 "$out/data.img" \
    --name live --out "$out" --timeout-ms "${ONESHOT_TIMEOUT_MS:-90000}" \
    --mouse "$spec" --keys-after SHELL-READY --legacy-quit > "$out/stdout" 2> "$out/report"
mkdir -p "$out/files"; mcopy -n -o -s -i "$out/data.img" '::*' "$out/files/" 2>/dev/null
rc=0
if grep -q 'TRIPLE FAULT\|PANIC' "$out/report" "$out/live.serial" 2>/dev/null; then echo "RED: $app fault"; rc=1; fi
for p in "$out"/*.ppm; do [ -f "$p" ] || continue; echo "dump: $p"
    if [ -s "$p.screen" ] && build/screen_tool decode "$p.screen" "$p.raw" >/dev/null 2>&1; then
        python3 "$here/screen2txt.py" "$p.raw" > "${p%.ppm}.txt"; echo "  text: ${p%.ppm}.txt"; fi; done
if [ -f "$src/CHECK" ]; then ( cd "$out" && sh "$OLDPWD/$src/CHECK" ) || { echo "RED: $app CHECK"; rc=1; }; fi
[ $rc = 0 ] && echo "GREEN: oneshot $app" || echo "RED: oneshot $app"
exit $rc
