#!/bin/sh
# Rule 6: the SAME extraction tooth used by test-dosmsg must reject each rule
# violation. Never edit shipped source or a shared generated header here.
set -efu
scan=$1 header=$2 allow=$3 baseline=$4
shift 4
sources="$*" # Locked Makefile source paths have no whitespace or glob tokens.
out=build/dosmsg_source_mutants
mkdir -p "$out"
for mutant in NEW_DIAGNOSTIC INLINE_CATALOGUE SPLIT_CATALOGUE UNKNOWN_MACRO ALLOW_GROWTH STALE_ALLOW NASM_DIAGNOSTIC; do
    cp -f "$allow" "$out/allow"
    cp -f "$baseline" "$out/baseline"
    target=os/milton/command.c
    case "$mutant" in
        NEW_DIAGNOSTIC) replacement='"Directory name rejected"'; why='unlisted literal "Directory name rejected"' ;;
        INLINE_CATALOGUE) replacement='"Invalid directory"'; why='inline MSG_DOS_0018' ;;
        SPLIT_CATALOGUE) replacement='"Invalid " "directory"'; why='inline MSG_DOS_0018' ;;
        UNKNOWN_MACRO) replacement='MSG_DOS_9999'; why='unknown macro MSG_DOS_9999' ;;
        ALLOW_GROWTH) replacement='MSG_DOS_0018'; why='allow-list growth beyond frozen baseline'
            printf 'os/milton/command.c\t"Unapproved UI"\t2026-10-17\tUI: unauthorized new exception\n' >> "$out/allow" ;;
        STALE_ALLOW) replacement='MSG_DOS_0018'; why='stale allow row'
            printf 'os/milton/command.c\t"Removed UI"\t2026-10-17\tUI: obsolete exception must retire\n' >> "$out/allow"
            printf 'os/milton/command.c\t"Removed UI"\n' >> "$out/baseline" ;;
        NASM_DIAGNOSTIC) target=os/milton/find_program.asm
            replacement=''; why='unlisted literal "FIND: broken parameter"' ;;
    esac
    # Change the real CD failure call, not a comment containing a macro name.
    if [ "$mutant" = NASM_DIAGNOSTIC ]; then
        sed 's/^msg_param: db MSG_DOS_[0-9]*/msg_param: db "FIND: broken parameter"/' "$target" > "$out/source"
    else
        awk -v r="$replacement" '
        !done && /dos_print\(MSG_DOS_0018 / {sub(/MSG_DOS_0018/,r);done=1}
        {print} END {if(!done)exit 2}' "$target" > "$out/source"
    fi
    # Retain the source identity, so its exact UI exceptions still apply.
    log=$out/$mutant.log
    set -- "$scan" "$header" "$out/allow" "$out/baseline"
    for source in $sources; do
        set -- "$@" "$source"
        if [ "$source" = "$target" ]; then set -- "$@" --replace "$target" "$out/source"; fi
    done
    if "$@" > "$log" 2>&1; then
        printf 'FAIL dosmsg mutant %s survived\n' "$mutant"; exit 1
    fi
    grep -F "$why" "$log" || { cat "$log"; exit 1; }
    printf 'GREEN dosmsg mutant %s: named assertion RED\n' "$mutant"
done
