#!/bin/sh
# Independent real DOS 3.30 nested behavior is in batch_break_mint/. QEMU
# drives the shipped interpreter, then mtools grades stopped-image bytes.
set -eu
boot=$1
tag=${2:-clean}
only=${3:-all}
fail() { echo "RED batch-break-emu: $mode $*" >&2; exit 1; }
for mode in pause-n pause-y invalid loop-n loop-y nested-n nested-y redirect-n redirect-y exec-n exec-y; do
    [ "$only" = all ] || [ "$only" = "$mode" ] || continue
    out=build/batch-break-$tag-$mode
    mkdir -p "$out"
    img=$out/data.img
    dd if=/dev/zero of="$img" bs=512 count=2880 status=none
    mformat -i "$img" -f 1440 -N 338 ::
    printf 'SHELL=COMMAND.COM /E:512\r\n' > "$out/config"
    mcopy -i "$img" "$out/config" ::CONFIG.SYS
    printf 'INPUT-SENTINEL\r\n' > "$out/input"
    mcopy -i "$img" "$out/input" ::INPUT.TXT
    printf '@ECHO OFF\r\nECHO BB-BEGIN\r\n' > "$out/autoexec"
    printf '@ECHO OFF\r\nPAUSE\r\nECHO CHILD_AFTER\r\n' > "$out/child"
    printf '@ECHO OFF\r\nCALL CHILD\r\nECHO PARENT_AFTER\r\n' > "$out/parent"
    trigger='Strike a key'
    case "$mode" in
    loop-*)
        cat docs/audits/2026-10-23-dos/evidence/resume-fixtures/LOOP.BAT > "$out/child"
        printf 'CALL CHILD\r\n' >> "$out/autoexec"
        trigger=LOOPING;;
    nested-*) printf 'CALL PARENT\r\n' >> "$out/autoexec";;
    redirect-*)
        printf 'CALL PARENT < INPUT.TXT > OUT.TXT\r\n' >> "$out/autoexec"
        trigger=BB-BEGIN;;
    exec-*)
        mcopy -i "$img" build/batch_break_child.bin ::BRKCHILD.COM
        printf '@ECHO OFF\r\nBRKCHILD < INPUT.TXT > OUT.TXT\r\nECHO CHILD_AFTER\r\n' > "$out/child"
        printf 'CALL CHILD\r\n' >> "$out/autoexec"
        trigger=BB-BEGIN;;
    *) printf 'CALL CHILD\r\n' >> "$out/autoexec";;
    esac
    printf 'ECHO MAIN_AFTER\r\n' >> "$out/autoexec"
    mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
    mcopy -i "$img" "$out/parent" ::PARENT.BAT
    mcopy -i "$img" "$out/child" ::CHILD.BAT
    case "$mode" in
    *-y) answer='ky';;
    invalid) answer='kx,w100,kn';;
    loop-n) answer='kn,w100,kctrl-c,w100,ky';;
    *) answer='kn';;
    esac
    delay=''
    case "$mode" in redirect-*|exec-*) delay='w100,';; esac
    build/qemu_harness --disk "$boot" --disk2 "$img" --out "$out" --name run \
        --mouse "${delay}kctrl-c,WTerminate batch job (Y/N)?,${answer},WA:\>,ke,kc,kh,ko,kspc,kc,kl,ke,ka,kn,kspc,kshift-dot,kspc,kc,kl,ke,ka,kn,kdot,kt,kx,kt,kret,ke,kc,kh,ko,kspc,kr,ke,ks,ku,km,ke,kd,kret,ke,kx,ki,kt,kret" \
        --keys-after "$trigger" --quit-after SHELL-DONE --timeout-ms 25000 > "$out/stdout" 2> "$out/report" || true
    [ -f "$out/run.serial" ] || fail 'missing serial'
    tr -d '\r' < "$out/run.serial" > "$out/text"
    grep -q SHELL-DONE "$out/text" || fail 'no usable prompt after answer'
    grep -q '^resumed$' "$out/text" || fail 'stdout not restored'
    grep -qF 'Terminate batch job (Y/N)?' "$out/text" || fail 'catalogue question missing'
    case "$mode" in exec-*)
        grep -qx INT23-BREAK "$out/text" || fail 'EXEC child never terminated on Ctrl-C'
        [ "$(grep -cF 'Terminate batch job (Y/N)?' "$out/text")" = 1 ] || fail 'EXEC child question not exactly once'
        mcopy -i "$img" ::OUT.TXT - > "$out/redirected"
        printf 'EXEC-CHILD-READY\r\n' > "$out/expected"
        cmp "$out/expected" "$out/redirected" || fail 'EXEC child survived or question leaked to stdout';;
    esac
    case "$mode" in exec-*)
        ref=harness/diff/fat_diff/batch_break_mint/$mode.txt
        grep -qF 'Terminate batch job (Y/N)?' "$ref" || fail 'real DOS EXEC golden question missing'
        continued=0;grep -q EXEC_AFTER "$ref" && continued=1
        actual=0;grep -qx CHILD_AFTER "$out/text" && actual=1
        [ "$actual" = "$continued" ] || fail 'EXEC continuation differs from real DOS golden';;
    esac
    case "$mode" in
    *-y|loop-n)
        if grep -q '_AFTER' "$out/text"; then fail 'Y did not cancel all batch sentinels'; fi;;
    redirect-n) ;;
    *)
        [ "$(grep -c '^CHILD_AFTER$' "$out/text")" = 1 ] || fail 'N child sentinel not exactly once'
        [ "$(grep -c '^MAIN_AFTER$' "$out/text")" = 1 ] || fail 'N outer sentinel not exactly once';;
    esac
    case "$mode" in
    nested-n) [ "$(grep -c '^PARENT_AFTER$' "$out/text")" = 1 ] || fail 'N parent sentinel not exactly once';;
    invalid) [ "$(grep -cF 'Terminate batch job (Y/N)?' "$out/text")" = 2 ] || fail 'invalid answer did not reprompt';;
    loop-n)
        [ "$(grep -cF 'Terminate batch job (Y/N)?' "$out/text")" = 2 ] || fail 'loop N did not resume to second break'
        sed -n '/Terminate batch job (Y\/N)? n/,/Terminate batch job (Y\/N)? y/p' "$out/text" > "$out/continued"
        grep -q '^LOOPING$' "$out/continued" || fail 'N did not continue LOOP';;
    redirect-*)
        mcopy -i "$img" ::OUT.TXT - > "$out/redirected"
        if grep -q 'Terminate batch job' "$out/redirected"; then fail 'question leaked to redirected stdout'; fi
        if [ "$mode" = redirect-n ]; then
            printf 'Strike a key when ready . . .\r\nCHILD_AFTER\r\nPARENT_AFTER\r\n' > "$out/expected"
            cmp "$out/expected" "$out/redirected" || fail 'N redirected sentinels not exactly once'
            grep -qx MAIN_AFTER "$out/text" || fail 'N outer sentinel missing'
        else
            printf 'Strike a key when ready . . .\r\n' > "$out/expected"
            cmp "$out/expected" "$out/redirected" || fail 'Y left redirected post-question bytes'
        fi;;
    esac
    mcopy -i "$img" ::INPUT.TXT - > "$out/actual"
    cmp "$out/input" "$out/actual" || fail 'redirected input changed'
    mcopy -i "$img" ::CLEAN.TXT - > "$out/actual"
    printf 'clean\r\n' > "$out/expected"
    cmp "$out/expected" "$out/actual" || fail 'redirection handles not restored'
    fsck.fat -n "$img" > "$out/fsck" 2>&1 || fail 'FAT damage'
    echo "GREEN batch-break-emu $mode: question, continuation/cancellation sentinels, input/output cleanup, fsck.fat"
done
