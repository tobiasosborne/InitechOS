#!/bin/sh
# Validate captured exact references; retired producer binary hashes are META.
# Validate newly minted cheap exact references and prove all three screen-byte
# classes still bite. Never changes existing W3 captures/gates. DEC-03a.10.
set -eu
. "${INITECH_SHELL_CHECKS:?factory shell-checks.sh}"
. tests/check_status.sh
status_self_test
need build/screen_tool build/w4_screen_check
ref=${DBASE3_DECOMP:?absolute sister repository required}
base=tests/xbase_prog_diff/w4
out=build/w4-retained
mkdir -p "$out"
for app in retained create-retained structure-retained generated-form; do
    src=$base/$app
    need_manifest "$src" SHA256SUMS; need_manifest "$src" INPUTS.sha256; need "$src/ORACLE.sha256" "$src/DUMPS"
    check "retained-screens: $app pins" sh -c 'cd "$1" && sha256sum -c SHA256SUMS && sha256sum -c INPUTS.sha256' sh "$src"
    check "retained-screens: $app events" build/screen_tool compile "$src/KEYS" "$src/DUMPS" "$out/$app.events"
    while read -r expected name; do
        case $name in
        DBASE.EXE|DBASEINL.OVL|DBASE.MSG|DBASE.OVL|HELP.DBS) f=$ref/mint/work/$name;;
        SCREEN.COM|screen_tool|w4_screen_check|dosbox-x|w4_retained_mint.sh) continue;;
        EVENTS.BIN) f=$out/$app.events;;
        CLIENTS.DBF|CLIENTS.FMT) f=$ref/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/$name;;
        *) setup_error "retained-screens: unknown oracle manifest entry $name";;
        esac
        need "$f"
        actual=$(sha256sum < "$f" | cut -d' ' -f1)
        [ "$actual" = "$expected" ] || fail "retained-screens: $app oracle provenance $name"; ok
    done < "$src/ORACLE.sha256"
    while IFS= read -r name; do
        case $name in ''|'#'*) continue;; esac
        golden=$src/$name.screen
        need "$golden" "$src/$name.ASSERTIONS"
        check "retained-screens: $name assertions" build/w4_screen_check "$golden" "$src/$name.ASSERTIONS"
        check "retained-screens: $name self-compare" build/screen_tool compare "$golden" "$golden"
        for class in cell attr cursor; do
            build/screen_tool "mutate-$class" "$golden" "$out/mutant.screen" || setup_error "retained-screens: $name $class mutant not written"
            rc=0
            build/screen_tool compare "$golden" "$out/mutant.screen" > "$out/$name-$class.log" 2>&1 || rc=$?
            [ "$rc" -le 1 ] || { cat "$out/$name-$class.log"; setup_error "retained-screens: compare exit $rc"; }
            [ "$rc" = 1 ] && grep -q '^RED: first difference' "$out/$name-$class.log" || {
                cat "$out/$name-$class.log"; fail 'retained-screens: retained exact-screen mutant survived';
            }; ok
            echo "RED proven: retained $name $class"
        done
    done < "$src/DUMPS"
done
echo 'GREEN: comparator self-test w4-retained: real reference hashes/identity, exact char/attribute/cursor mutants RED'
summary
