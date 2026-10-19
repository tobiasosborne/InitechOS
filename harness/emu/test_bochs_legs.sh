#!/bin/sh
# Factory make-wiring differential. Mutate actual factory recipes in build,
# then parse them with GNU make; never edit the checkout during mutation.
set -eu
export LC_ALL=C
build=$1
mode=$2
work=$build/bochs2
mkdir -p "$work"
database() {
    rc=0
    make --no-print-directory -rR -qp -f "$1" > "$2" 2> "$2.err" || rc=$?
    if [ "$rc" -gt 1 ]; then cat "$2.err"; exit "$rc"; fi
}
database Makefile "$work/legs.make-db"
"$build/test_bochs_legs" "$work/legs.make-db"
[ "$mode" = mutants ] || exit 0
for rule in MISSING VECTOR IMAGE BOUNDARY VIDEO DAC; do
    case $rule in
    MISSING) sed '/call bochs-flair-leg,command-file-emu,/d' harness/bochs_legs.mk ;;
    VECTOR) sed '/^TEST_EMU_GATES += test-$(1)-bochs$/d' harness/bochs_legs.mk ;;
    IMAGE) sed 's@command-file-emu,$(COMMAND_FILE_IMG)/fragment.img@command-file-emu,$(FLAIRTENANTS_IMG)@' harness/bochs_legs.mk ;;
    BOUNDARY) sed 's/^BOCHS_LEG_END_shell := SHELL-READY$/BOCHS_LEG_END_shell := HALTED/' harness/bochs_legs.mk ;;
    # The forced mode-13h legs silently fall back to the stock VBE card.
    VIDEO) sed 's/^BOCHS_LEG_VIDEO_vga13 := .*$/BOCHS_LEG_VIDEO_vga13 :=/' harness/bochs_legs.mk ;;
    # The VESA8 flagship leg stops grading the 648-pixel padded scanline.
    DAC) sed 's/,$(FLAIR_DATA_IMG),flair,vbe101pad))$/,$(FLAIR_DATA_IMG),flair))/' harness/bochs_legs.mk ;;
    esac > "$work/$rule.mk"
    sed "s@include harness/bochs_legs.mk@include $work/$rule.mk@" Makefile > "$work/$rule.Makefile"
    database "$work/$rule.Makefile" "$work/$rule.make-db"
    rc=0
    "$build/test_bochs_legs" "$work/$rule.make-db" > "$work/$rule.log" 2>&1 || rc=$?
    test "$rc" = 1 && grep -q "^RED bochs-legs $rule:" "$work/$rule.log" || {
        cat "$work/$rule.log"; echo "FAIL BOCHS_LEGS_$rule survived or failed infrastructure"; exit 1;
    }
    printf 'BOCHS_LEGS_%s: ' "$rule"
    cat "$work/$rule.log"
done
echo 'GREEN test-bochs-legs-mutant: all six factory wiring mutants caught'
