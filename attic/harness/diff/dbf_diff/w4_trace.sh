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
for f in "$src/KEYS" "$src/CHECK" "$src/SETUP/SETUP.PRG" "$exe" "$ovl"; do
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
    case $name in SAMIR.EXE|SAMIR.OVL|W4INIT.PRG|W4TOKEN.TXT|W4READY.TXT)
        echo 'RED: SETUP cannot replace SAMIR or W4 prelude evidence'; exit 2;; esac
    mcopy -o -i "$out/data.img" "$f" ::"$(basename "$f")"
done
# Same DO SETUP as w4_retained_mint.sh's MINT.PRG. COPY FILE marks return
# without changing any SET option, memory variable, selected area or cursor.
# A 512-byte token avoids COPY FILE's real III+ block-rounding ambiguity.
if mdir -b -i "$out/data.img" ::W4READY.TXT > "$out/stale-marker.log" 2>&1; then
    echo 'RED: stale W4 setup completion marker'; exit 2
fi
printf 'DO SETUP\r\nCOPY FILE W4TOKEN.TXT TO W4READY.TXT\r\nRETURN\r\n' > "$out/W4INIT.PRG"
dd if=/dev/zero of="$out/W4TOKEN.TXT" bs=512 count=1 2>/dev/null
printf 'W4-SETUP-OK\r\n' | dd of="$out/W4TOKEN.TXT" conv=notrunc 2>/dev/null
for f in W4INIT.PRG W4TOKEN.TXT; do mcopy -o -i "$out/data.img" "$out/$f" ::"$f"; done
mkdir "$out/before" "$out/files"
mcopy -n -o -s -i "$out/data.img" '::*' "$out/before/"
spec='ks,ka,km,ki,kr,kret,w2000,kd,ko,kspc,kw,k4,ki,kn,ki,kt,kret,w800'
dumps=
while IFS= read -r line || [ -n "$line" ]; do
    case $line in ''|'#'*) continue;; esac
    verb=${line%% *}; arg=${line#* }
    [ "$arg" != "$line" ] || { echo "RED: bad KEYS: $line"; exit 2; }
    case $verb in
    WAIT) case $arg in ''|*[!0-9]*) echo 'RED: bad WAIT'; exit 2;; esac
        [ "$arg" -ge 1 ] && [ "$arg" -le 65535 ] || exit 2
        spec="$spec,w$((arg*55))" ;;
    TEXT) t=$arg
        # Reject unsupported characters rather than silently dropping keys.
        while [ -n "$t" ]; do c=${t%"${t#?}"}; t=${t#?}
            case $c in ' ') c=spc;; .) c=dot;; '"') c=shift-apostrophe;;
                [A-Z]) c=shift-$(printf '%s' "$c" | tr 'A-Z' 'a-z');;
                [a-z0-9]) ;; *) echo "RED: unsupported TEXT: $arg"; exit 2;; esac
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
[ "${#spec}" -lt 4096 ] || { echo 'RED: W4 key stream exceeds injector capacity'; exit 2; }
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
if ! mcopy -o -i "$out/data.img" ::W4READY.TXT "$out/setup.ok" 2>/dev/null ||
   ! cmp -s "$out/W4TOKEN.TXT" "$out/setup.ok"; then
    echo 'RED: W4 setup completion marker missing/invalid'; exit 1
fi
echo 'GREEN: W4 DO SETUP returned before KEYS (fresh completion marker)'
# Keep harness-only completion evidence beside captures, outside app files.
mdel -i "$out/data.img" ::W4READY.TXT
mcopy -n -o -s -i "$out/data.img" '::*' "$out/files/"
W4_ROOT=$root W4_CASE=$src W4_OUT=$root/$out
export W4_ROOT W4_CASE W4_OUT
(cd "$out" && sh "$W4_CASE/CHECK") || { echo "RED: w4 trace $app CHECK"; exit 1; }
printf 'GREEN: w4 trace %s: locked KEYS replayed; data disk extracted; CHECK passed\n' "$app"
