#!/bin/sh
# L2 locked replay, DEC-03a.10 / initech-9nzpr. Pattern: screen_forms_emu.sh.
# usage: w4_trace.sh APP [SAMIR.EXE]. Run from repository root, after make.
# KEYS dialect is the existing screen-mint WAIT/TEXT/KEY/DUMP dialect.
# W4_TRACE_ROOT permits isolated harness mutants; normal acceptance unsets it.
set -eu
export LC_ALL=C
app=${1:?app required}; exe=${2:-build/SAMIR.EXE}; ovl=${exe%.EXE}.OVL
case $app in ''|*[!a-z0-9-]*) echo 'RED: unsafe W4 app'; exit 2;; esac
root=$(pwd)
src=${W4_TRACE_ROOT:-harness/diff/dbf_diff/xbase_prog_diff/w4}/$app
out=build/w4/$app
for f in "$src/KEYS" "$src/CHECK" "$exe" "$ovl"; do
    [ -s "$f" ] || { echo "RED: w4 trace missing $f"; exit 2; }
done
src=$(cd "$src" && pwd)
mkdir -p "$out"
# Prevent concurrent invocations of the SAME app corrupting evidence.
mkdir "$out/.lock" 2>/dev/null || { echo "RED: w4 trace $app already running"; exit 2; }
trap 'rmdir "$out/.lock"' EXIT
trap 'exit 130' HUP INT TERM
find "$out" -mindepth 1 -maxdepth 1 ! -name .lock -exec rm -rf {} +
cp -f build/samir_list.img "$out/data.img"
mcopy -o -i "$out/data.img" "$exe" ::SAMIR.EXE
mcopy -o -i "$out/data.img" "$ovl" ::SAMIR.OVL
for f in "$src"/SETUP/*; do
    [ -e "$f" ] || continue
    [ -f "$f" ] && [ ! -L "$f" ] || { echo 'RED: W4 SETUP must contain regular files'; exit 2; }
    name=$(basename "$f" | tr 'a-z' 'A-Z')
    case $name in SAMIR.EXE|SAMIR.OVL) echo 'RED: SETUP cannot replace SAMIR'; exit 2;; esac
    mcopy -o -i "$out/data.img" "$f" ::"$(basename "$f")"
done
mkdir "$out/before" "$out/files"
mcopy -n -o -s -i "$out/data.img" '::*' "$out/before/"
spec='ks,ka,km,ki,kr,kret,w2000'
dumps=
while IFS= read -r line || [ -n "$line" ]; do
    case $line in ''|'#'*) continue;; esac
    verb=${line%% *}; arg=${line#* }
    [ "$arg" != "$line" ] || { echo "RED: bad KEYS: $line"; exit 2; }
    case $verb in
    WAIT) case $arg in ''|*[!0-9]*) echo 'RED: bad WAIT'; exit 2;; esac
        [ "$arg" -ge 1 ] && [ "$arg" -le 65535 ] || exit 2
        spec="$spec,w$((arg*55))" ;;
    TEXT) t=$(printf '%s' "$arg" | tr 'A-Z' 'a-z')
        # Reject unsupported characters rather than silently dropping keys.
        case $t in *[!a-z0-9\ .]*) echo "RED: unsupported TEXT: $arg"; exit 2;; esac
        while [ -n "$t" ]; do c=${t%"${t#?}"}; t=${t#?}
            case $c in ' ') c=spc;; .) c=dot;; esac
            spec="$spec,k$c"
        done ;;
    KEY) case $arg in
        ENTER) k=ret;; ESC) k=esc;; UP) k=up;; DOWN) k=down;; LEFT) k=left;; RIGHT) k=right;;
        HOME) k=home;; END) k=end;; PGUP) k=pgup;; PGDN) k=pgdn;; TAB) k=tab;;
        BACKSPACE) k=backspace;; DELETE) k=delete;; INSERT) k=insert;;
        CTRL-W) k=ctrl-w;; CTRL-Y) k=ctrl-y;; CTRL-END) k=ctrl-end;; CTRL-HOME) k=ctrl-home;;
        F1) k=f1;; F2) k=f2;; F10) k=f10;; *) echo "RED: unsupported KEY $arg"; exit 2;;
        esac; spec="$spec,k$k" ;;
    DUMP) case $arg in ''|*[!a-zA-Z0-9_-]*) echo 'RED: unsafe DUMP'; exit 2;; esac
        case " $dumps " in *" $arg "*) echo 'RED: duplicate DUMP'; exit 2;; esac
        dumps="$dumps $arg"; spec="$spec,D$out/$arg.ppm" ;;
    *) echo "RED: unknown KEYS directive $verb"; exit 2;;
    esac
done < "$src/KEYS"
[ -n "$dumps" ] || { echo 'RED: KEYS has no capture'; exit 2; }
spec="$spec,w800"
INITECH_QMP_STDIO=1 INITECH_TEXT_SCREEN_TOOL=build/screen_capture \
INITECH_TEXT_SCREEN_ELF=build/kernel_shell_secondary.elf \
build/qemu_harness --disk build/tracer_secondary.img --disk2 "$out/data.img" \
    --name live --out "$out" --timeout-ms "${W4_TRACE_TIMEOUT_MS:-90000}" \
    --mouse "$spec" --keys-after SHELL-READY --legacy-quit > "$out/stdout" 2> "$out/report" || {
        cat "$out/report"; echo "RED: w4 trace $app QEMU runner"; exit 1;
    }
if grep -Eq 'TRIPLE FAULT|PANIC|triple_fault=1|timed_out=1|KBD-RING-DROP' "$out/report" "$out/live.serial"; then
    echo "RED: w4 trace $app fault/timeout/key drop"; exit 1
fi
for d in $dumps; do
    [ -s "$out/$d.ppm" ] && [ -s "$out/$d.ppm.screen" ] || { echo "RED: missing capture $d"; exit 1; }
done
mcopy -n -o -s -i "$out/data.img" '::*' "$out/files/"
W4_ROOT=$root W4_CASE=$src W4_OUT=$root/$out
export W4_ROOT W4_CASE W4_OUT
(cd "$out" && sh "$W4_CASE/CHECK") || { echo "RED: w4 trace $app CHECK"; exit 1; }
printf 'GREEN: w4 trace %s: locked KEYS replayed; data disk extracted; CHECK passed\n' "$app"
