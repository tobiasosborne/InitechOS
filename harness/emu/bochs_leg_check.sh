#!/bin/sh
# WL-0170: the ONE Bochs boot-leg serial verdict (Rule 5; PRD Sec 8). Every
# Bochs leg used to hand-roll the "VBE-ENOMODE + VGA13 + 640x480 guard" check.
# Since initech-2gva IO.SYS takes VESA 0101h (640x480x8 + LFB) and the 8-bit
# DAC on Bochs 2.7: its rfb display caps VBE at 8 bpp (gui/rfb.cc
# get_capabilities; iodev/display/vga.cc VBE_DISPI_GETCAPS), so the LGPL
# vgabios lists only 0100h/0101h at 640 wide, with the dispi LFB at
# 0xE0000000 (vga.h VBE_DISPI_LFB_PHYSICAL_ADDRESS). The mode-0x13 floor is
# graded on a period card that really lacks a VBE LFB (test-boot-vga13-bochs).
#
# usage: bochs_leg_check.sh NAME REPORT SERIAL PATH [CHECK...]
#   PATH  vbe101 | vga13 (Cirrus ISA: VBE without LFB) | e00 (no VBE at all)
#   CHECK MARKER   a complete exact serial line that must be present
#         !PANIC   no serial line may start with PANIC
set -eu
name=$1; report=$2; serial=$3; path=$4; shift 4
fail() { printf '!!! %s FAIL: %s\n' "$name" "$*"; exit 1; }
grep -q 'rfb_unblocked=1' "$report" || fail 'RFB unblock failed -- Bochs did not run the guest'
if grep -q 'triple_fault=1' "$report"; then fail 'TRIPLE FAULT under Bochs'; fi
[ -s "$serial" ] || fail "no serial captured at $serial"
norm=$serial.norm
tr -d '\r' < "$serial" > "$norm"
has() { grep -qxF -- "$1" "$norm"; }
case $path in
 vbe101) want='VBE-MODE=0101
VBE
VIDEO 640x480 bpp=8 pitch=640 lfb=0xE0000000 dac=8'
        bad='VBE-ENOMODE VGA13' ;;
 vga13)  want='VBE-ENOMODE
VGA13
VIDEO 320x200 bpp=8 pitch=320 lfb=0x000A0000 dac=6'
        bad='VBE VBE-E00' ;;
 e00)    want='VBE-E00
VGA13
VIDEO 320x200 bpp=8 pitch=320 lfb=0x000A0000 dac=6'
        bad='VBE VBE-ENOMODE' ;;
 *) fail "unknown video path $path" ;;
esac
printf 'Serial markers captured (%s video path):\n' "$path"
printf '%s\n' "$want" > "$norm.want"
for m in S1 PM KERNEL CONSOLE BANNER; do printf '%s\n' "$m" >> "$norm.want"; done
for c in "$@"; do [ "$c" = '!PANIC' ] || printf '%s\n' "$c" >> "$norm.want"; done
missing=
while IFS= read -r m; do
    if has "$m"; then printf '  %-52s : present\n' "$m"
    else printf '  %-52s : MISSING\n' "$m"; missing=${missing:-$m}; fi
done < "$norm.want"
[ -z "$missing" ] || fail "marker '$missing' missing"
for m in $bad; do
    if has "$m"; then fail "'$m' printed: stage2 took a different video path than $path"; fi
done
for c in "$@"; do
    if [ "$c" = '!PANIC' ] && grep -q '^PANIC' "$norm"; then
        fail "unexpected $(grep -m1 '^PANIC' "$norm")"
    fi
done
printf '>>> %s: green (Bochs %s video path + shared kernel milestones' "$name" "$path"
[ $# -eq 0 ] || printf ' + %d leg checks' $#
printf '; no triple fault)\n'
