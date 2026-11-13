#!/bin/sh
# realdos_run.sh -- run DOS programs under a REAL MS-DOS boot diskette in
# dosbox-x and leave the diskette image for mtools to read back (beads
# initech-509.9 / initech-8479.1 golden minting; NOT part of `make test`).
#
#   realdos_run.sh DOS_IMG RUN_IMG AUTOEXEC_LINES_FILE HOSTFILE=DOSNAME ...
#
# RUN_IMG is DOS_IMG with large unused files removed, CONFIG.SYS reduced to
# FILES=30, AUTOEXEC.BAT = "@ECHO OFF" + the given lines + "OFF", and the
# HOSTFILE=DOSNAME pairs copied in. OFF.COM is an APM power-off (INT 15h
# AX=5301h/530Eh/5307h) so dosbox-x exits cleanly and the image is flushed.
set -e
dos_img=$1; run_img=$2; lines=$3; shift 3
dir=$(dirname "$run_img")
mkdir -p "$dir"
printf 'bits 16\norg 0x100\nmov ax,0x5301\nxor bx,bx\nint 0x15\nmov ax,0x530E\nxor bx,bx\nmov cx,0x0102\nint 0x15\nmov ax,0x5307\nmov bx,1\nmov cx,3\nint 0x15\nmov ax,0x4C00\nint 0x21\n' > "$dir/off.asm"
nasm -f bin "$dir/off.asm" -o "$dir/OFF.COM"
cp -f "$dos_img" "$run_img"
mdel -i "$run_img" ::QBASIC.EXE ::EMM386.EXE ::MSCDEX.EXE ::CD2.SYS ::CD3.SYS 2>/dev/null || true
printf 'FILES=30\r\n' > "$dir/CONFIG.SYS"
{ printf '@ECHO OFF\r\n'; sed 's/$/\r/' "$lines"; printf 'OFF\r\n'; } > "$dir/AUTOEXEC.BAT"
mcopy -o -i "$run_img" "$dir/CONFIG.SYS" ::CONFIG.SYS
mcopy -o -i "$run_img" "$dir/AUTOEXEC.BAT" ::AUTOEXEC.BAT
mcopy -o -i "$run_img" "$dir/OFF.COM" ::OFF.COM
for pair in "$@"; do
	mcopy -o -i "$run_img" "${pair%%=*}" "::${pair#*=}"
done
printf '[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=8\nquit warning=false\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[autoexec]\nimgmount a %s -t floppy\nboot a:\n' "$(cd "$dir" && pwd)/$(basename "$run_img")" > "$dir/run.conf"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 120 dosbox-x -conf "$dir/run.conf" -nogui -nomenu > "$dir/dosbox.log" 2>&1 || true
