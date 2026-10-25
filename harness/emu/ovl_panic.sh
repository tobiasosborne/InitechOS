#!/bin/sh
# ovl_panic.sh -- one leg of the kernel-overlay refusal / context gates
# (ADR-0014 DEC-14-9(b)/(c); harness/proptest/overlay.mk). CLAUDE Rule 2.
#
#   ovl_panic.sh <build> <image> <data> <tag> <REASON> <v-hex|-> <quit-marker> [ready-ok]
#
# [ready-ok]=1 only for the IRQ0 CONTEXT probe, which can run only once the
# pump has enabled interrupts (after FLAIR-TENANTS-READY).
# Boots <image> (+ <data> as the primary slave) and requires the named
# refusal: serial "PANIC FLAIR.OVL <REASON> v=<v>", HALTED, no triple fault,
# NEVER FLAIR-TENANTS-READY, and the PC LOAD LETTER raster with the status
# line "CANNOT LOAD FLAIR.OVL" and the reason as its subject, graded by the
# independent tools/ppm_panic_check.c against the pre-fault ROM font dump of
# test-panic-screen (never against screen pixels the artifact drew). Literal
# expected text here, never the kernel's strings (Law 2).
# Exit 0 = GREEN; 1 = RED (the reason is printed); 2 = harness misuse.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; image=$2; data=$3; tag=$4; why=$5; want_v=$6; quit=$7; ready_ok=${8:-0}
work=$build/ovl-gates
mkdir -p "$work"
red() { echo "RED ovl-panic $tag: $*"; exit 1; }
"$build/qemu_harness" --disk "$image" --disk2 "$data" --out "$work" --name "$tag" \
    --timeout-ms 30000 --quit-after "$quit" \
    --screendump --screendump-after "$quit" > "$work/$tag.stdout" 2> "$work/$tag.report" || true
grep -q 'triple_fault=0' "$work/$tag.report" || red "triple fault or no report"
tr -d '\r' < "$work/$tag.serial" > "$work/$tag.norm"
if [ "$ready_ok" != 1 ] && grep -q '^FLAIR-TENANTS-READY' "$work/$tag.norm"; then red "reached FLAIR-TENANTS-READY (the overlay was accepted)"; fi
line=$(grep '^PANIC FLAIR.OVL ' "$work/$tag.norm" | head -n 1 || true)
[ -n "$line" ] || red "no PANIC FLAIR.OVL line"
case "$line" in
"PANIC FLAIR.OVL $why v="*) : ;;
*) red "wrong refusal: $line (want $why)" ;;
esac
if [ "$want_v" != - ] && [ "$line" != "PANIC FLAIR.OVL $why v=$want_v" ]; then red "wrong value: $line (want v=$want_v)"; fi
grep -qx HALTED "$work/$tag.norm" || red "no HALTED"
echo "GREEN ovl-panic-serial $tag: $line"
"$build/ppm_panic_check" "$work/$tag.ppm" "$build/panic-screen/ud32.font" 640 480 "CANNOT LOAD FLAIR.OVL" "$why" \
    || red "PC LOAD LETTER raster"
echo "GREEN ovl-panic $tag: $why refused by name on serial and on the PC LOAD LETTER screen"
