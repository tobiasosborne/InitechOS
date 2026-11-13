#!/bin/sh
# W3 forms emu oracle (initech-p3nn0): replay a real III+ screen-mint case's KEYS
# against SAMIR under QEMU and compare each gated dump with screen_tool compare
# (no trimming or masking: docs/screen-mint.md). Replay the same paused states;
# QEMU needs shorter settling waits than the real 386 reference. Polling INKEY
# for full BIOS-tick waits would exhaust the unchanged QEMU debug-log cap.
# usage: screen_forms_emu.sh [SAMIR.EXE] ; any differing dump is RED.
set -u
exe=${1:-build/SAMIR.EXE}
ovl=${exe%.EXE}.OVL
tag=$(basename "$exe" .EXE)
list=${SCREEN_FORMS_GATED:-harness/diff/dbf_diff/screen_forms_gated.txt}
suite=${SCREEN_FORMS_SUITE:-forms-emu}
root=build/samir-$suite/$tag
mint=harness/diff/dbf_diff/xbase_prog_diff/screen-mint
scale=${SCREEN_FORMS_WAIT_MS:-5}
run_case() {
    case=$1; shift; gated=" $* "; out=$root/$case
    rm -rf "$out"; mkdir -p "$out"
    cp -f build/samir_list.img "$out/data.img"
    mcopy -o -i "$out/data.img" "$exe" ::SAMIR.EXE && mcopy -o -i "$out/data.img" "$ovl" ::SAMIR.OVL || return 2
    for f in "$mint/$case"/SETUP/*.PRG "$mint/$case"/SETUP/*.DBF; do [ -f "$f" ] || continue; mcopy -o -i "$out/data.img" "$f" ::"$(basename "$f")" || return 2; done
    spec='ks,ka,km,ki,kr,kret,w1500,kd,ko,kspc,ks,ke,kt,ku,kp,kret,w800'
    while IFS=' ' read -r verb arg; do
        case $verb in
        WAIT) ms=$((arg * scale)); [ "$ms" -gt 3000 ] && ms=3000; spec="$spec,w$ms" ;;
        KEY) [ "$arg" = ENTER ] || { echo "RED: unsupported key $arg"; return 2; }; spec="$spec,kret" ;;
        TEXT) t=$arg
              i=0; while [ $i -lt ${#t} ]; do i=$((i+1)); c=$(printf '%s' "$t" | cut -c$i)
                  case $c in
                  ' ') k=spc;; '@') k=shift-2;; ',') k=comma;; '"') k=shift-apostrophe;;
                  '?') k=shift-slash;; '=') k=equal;; '.') k=dot;; '-') k=minus;;
                  [A-Z]) k=shift-$(printf '%s' "$c" | tr 'A-Z' 'a-z');;
                  [a-z0-9]) k=$c;; *) echo "RED: unsupported text $arg"; return 2;;
                  esac
                  spec="$spec,k$k"; done ;;
        DUMP) case $gated in *" $arg "*) spec="$spec,D$out/$arg.ppm" ;; esac ;;
        esac
    done < "$mint/$case/KEYS"
    # --legacy-quit: every asserted capture is a D token inside the stream, taken
    # before the last input; a host too slow to reach one is a missing dump = RED.
    if ! INITECH_QMP_STDIO=1 INITECH_TEXT_SCREEN_TOOL=build/screen_capture \
    INITECH_TEXT_SCREEN_ELF=build/kernel_shell_secondary.elf \
    build/qemu_harness --disk build/tracer_secondary.img --disk2 "$out/data.img" \
        --name live --out "$out" --timeout-ms "${SCREEN_FORMS_TIMEOUT_MS:-60000}" \
        --mouse "$spec" --keys-after SHELL-READY --legacy-quit > "$out/stdout" 2> "$out/report"; then
        echo "RED: $case capture run failed"; cat "$out/report"; return 1
    fi
    if ! grep -q '^\[harness\] OK=1$' "$out/report"; then
        echo "RED: $case capture run incomplete"; cat "$out/report"; return 1
    fi
    rc=0
    for d in "$@"; do
        if [ ! -s "$out/$d.ppm.screen" ]; then echo "RED: $case/$d not captured"; rc=1; continue; fi
        if build/screen_tool compare "$mint/$case/$d.screen" "$out/$d.ppm.screen" > "$out/$d.verdict" 2>&1; then
            echo "GREEN: $case/$d"
        else echo "RED: $case/$d: $(head -2 "$out/$d.verdict" | tr '\n' ' ')"; rc=1; fi
    done
    if grep -q 'TRIPLE FAULT\|PANIC' "$out/report" "$out/live.serial" 2>/dev/null; then echo "RED: $case fault"; rc=1; fi
    return $rc
}
rm -rf "$root"; mkdir -p "$root"
pids=
while IFS= read -r line; do
    case $line in ''|'#'*) continue;; esac
    ( run_case $line ) > "$root/$(echo "$line" | cut -d' ' -f1).log" 2>&1 &
    pids="$pids $!"
done < "$list"
fail=0
for p in $pids; do wait "$p" || fail=1; done
cat "$root"/*.log
if [ "$fail" != 0 ]; then printf 'RED: test-samir-%s (%s)\n' "$suite" "$tag"; exit 1; fi
printf '>>> test-samir-%s: green (%s; every gated real III+ dump exact)\n' "$suite" "$tag"
