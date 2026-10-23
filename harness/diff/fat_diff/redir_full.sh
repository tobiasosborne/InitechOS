#!/bin/sh
# initech-66oc: COMMAND.COM redirection and pipes on a full or protected
# volume, driven on the emulated machine. Every expectation below comes from
# mtools on the same seed or from the MS-DOS 3.3 User's Guide, never from the
# artifact. Audit 2026-10-23-dos L001/L002/L003/L004/L027 are the steps.
#   Guide pp.18-20 (redirection and pipes); App. F printed p.330 ("File
#   creation error" [MS-DOS]: the file exists and is read-only) and p.337
#   ("Insufficient disk space" [MS-DOS]: the disk is full); IBM DOS 3.30
#   Technical Reference pp.6-139/6-140 (AH=40h on a full disk writes fewer
#   bytes) and p.6-215 (AH=5Bh fails when the name exists).
# Usage: redir_full.sh BOOT_IMG SORT_COM [tag]
set -efu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
boot=$1 sort=$2
out=build/redir-full-${3:-green}
mkdir -p "$out"
fail() { echo "RED redir-full: $*"; exit 1; }
# section NAME FILE: the serial lines after the marker NAME, up to the next marker.
section() {
    awk -v m="$1" '$0==m{on=1;next} /^(L0[0-9][0-9]|PIPE-RETURNED|REDIR-FULL-DONE)$/{on=0} on' "$2"
}
free_bytes() {
    mdir -i "$1" :: | sed -n 's/^ *\([0-9 ]*\) bytes free$/\1/p' | tr -d ' '
}
boot_run() {
    build/qemu_harness --disk "$boot" --disk2 "$out/$1.img" --out "$out" --name "$1" \
     --timeout-ms 30000 --keys-after SHELL-READY --keys 'e,x,i,t,ret' --quit-after SHELL-DONE \
     --rtc-base 1987-07-06T13:30:58 --rtc-deterministic 2> "$out/$1.report" || true
    grep -q quit_after_found=1 "$out/$1.report" || fail "$1: shell did not finish"
    if grep -q triple_fault=1 "$out/$1.report"; then fail "$1: triple fault"; fi
    tr -d '\r' < "$out/$1.serial" > "$out/$1.text"
    grep -qx REDIR-FULL-DONE "$out/$1.text" || fail "$1: batch did not finish"
}
printf 'ALPHA\r\n' > "$out/one"
printf 'USER-OWNED-PIPE-FILE\r\n' > "$out/sentinel"
printf 'PROTECTED\r\n' > "$out/protected"

# ---- volume 1: room to spare; a user file already owns PIPE1.$$$ (L001) and
# READONLY.TXT carries the read-only attribute (L027).
cat > "$out/room.bat" <<'BAT'
@echo off
echo L001
type one.txt | sort
echo L027
echo LOST > readonly.txt
echo LOST >> readonly.txt
echo REDIR-FULL-DONE
BAT
dd if=/dev/zero of="$out/room.img" bs=512 count=2880 status=none
mformat -i "$out/room.img" -f 1440 -N 102 ::
mcopy -i "$out/room.img" "$out/room.bat" ::AUTOEXEC.BAT
mcopy -i "$out/room.img" "$sort" ::SORT.COM
mcopy -i "$out/room.img" "$out/one" ::ONE.TXT
mcopy -i "$out/room.img" "$out/sentinel" '::PIPE1.$$$'
mcopy -i "$out/room.img" "$out/protected" ::READONLY.TXT
mattrib -i "$out/room.img" +r ::READONLY.TXT
sh harness/verify_fixture_volume.sh recipe "$out/room.img" AUTOEXEC.BAT SORT.COM ONE.TXT 'PIPE1.$$$' READONLY.TXT=AR
mdir -a -b -i "$out/room.img" :: | sort > "$out/room.before"

# ---- volume 2: zero bytes free (the audit's full0 fixture: a filler takes
# exactly the free space mdir reports).
cat > "$out/full.bat" <<'BAT'
@echo off
echo L002
echo TEST > full.out
echo L003
type one.txt | sort
echo PIPE-RETURNED
echo L004
copy long.txt full.bin
echo REDIR-FULL-DONE
BAT
dd if=/dev/zero of="$out/full.img" bs=512 count=2880 status=none
mformat -i "$out/full.img" -f 1440 -N 102 ::
mcopy -i "$out/full.img" "$out/full.bat" ::AUTOEXEC.BAT
mcopy -i "$out/full.img" "$sort" ::SORT.COM
mcopy -i "$out/full.img" "$out/one" ::ONE.TXT
awk 'BEGIN{for(i=0;i<1200;i++)printf "LONG LINE %05d abcdefghijklmnopqrstuvwxyz0123456789ABCDEF\r\n",i}' > "$out/long"
mcopy -i "$out/full.img" "$out/long" ::LONG.TXT
free=$(free_bytes "$out/full.img")
[ -n "$free" ] && [ "$free" -gt 0 ] || fail 'fixture free-space probe'
head -c "$free" /dev/zero | tr '\0' F > "$out/filler"
mcopy -i "$out/full.img" "$out/filler" ::FILLER.BIN
free=$(free_bytes "$out/full.img")
[ "$free" = 0 ] || fail "full fixture still has $free bytes free"
sh harness/verify_fixture_volume.sh recipe "$out/full.img" AUTOEXEC.BAT SORT.COM ONE.TXT LONG.TXT FILLER.BIN
cp -f "$out/full.img" "$out/full.reference.img"

boot_run room
boot_run full

# ---- L001: the pipe still delivers ALPHA, the user's PIPE1.$$$ survives byte
# for byte, and the pipe leaves no temporary behind (TR p.6-215).
section L001 "$out/room.text" > "$out/l001"
grep -qx ALPHA "$out/l001" || fail 'L001 pipe did not deliver ALPHA'
mtype -i "$out/room.img" '::PIPE1.$$$' > "$out/got" 2>/dev/null || fail 'L001 pipe deleted the user file PIPE1.$$$'
cmp -s "$out/got" "$out/sentinel" || fail 'L001 pipe overwrote the user file PIPE1.$$$'
# ---- L027: a read-only target is refused with the [MS-DOS] App. F p.330
# message, for > and >> alike, and the file keeps its contents.
section L027 "$out/room.text" > "$out/l027"
[ "$(grep -cx 'File creation error' "$out/l027")" = 2 ] || fail 'L027 read-only redirect did not report File creation error twice'
if grep -q 'Bad command or file name' "$out/l027"; then fail 'L027 read-only redirect reports Bad command or file name'; fi
mtype -i "$out/room.img" ::READONLY.TXT > "$out/got"
cmp -s "$out/got" "$out/protected" || fail 'L027 read-only file changed'
mdir -a -b -i "$out/room.img" :: | sort > "$out/room.after"
cmp -s "$out/room.before" "$out/room.after" || fail 'room volume membership changed (pipe temporary leaked)'
# ---- L002: output redirected to a full disk reports the full disk.
section L002 "$out/full.text" > "$out/l002"
grep -qx 'Insufficient disk space' "$out/l002" || fail 'L002 full-disk redirect reported no error (silent empty file)'
# ---- L003: a pipe whose intermediate cannot be written reports it, and the
# shell carries on with the next line.
section L003 "$out/full.text" > "$out/l003"
grep -qx 'Insufficient disk space' "$out/l003" || fail 'L003 full-disk pipe silently lost its input'
grep -qx PIPE-RETURNED "$out/full.text" || fail 'L003 shell did not continue after the pipe'
# ---- L004: COPY onto a full disk says why.
section L004 "$out/full.text" > "$out/l004"
grep -qx 'Insufficient disk space' "$out/l004" || fail 'L004 full-disk COPY did not report Insufficient disk space'
if grep -q 'Bad command or file name' "$out/l004"; then fail 'L004 full-disk COPY reports Bad command or file name'; fi
# The sources are untouched and the full volume stays a consistent FAT.
for f in ONE.TXT LONG.TXT FILLER.BIN; do
    mtype -i "$out/full.img" "::$f" > "$out/got"
    mtype -i "$out/full.reference.img" "::$f" > "$out/want"
    cmp -s "$out/got" "$out/want" || fail "full volume source $f changed"
done
if mdir -a -b -i "$out/full.img" :: | grep -q 'PIPE.*[$][$][$]'; then fail 'full volume pipe temporary leaked'; fi
fsck.fat -n "$out/room.img" > "$out/fsck" 2>&1 || fail 'room volume dosfstools damage'
fsck.fat -n "$out/full.img" >> "$out/fsck" 2>&1 || fail 'full volume dosfstools damage'
echo 'GREEN test-redir-full-emu: L001 pipe keeps a user PIPE1.$$$, L027 File creation error, L002/L003/L004 Insufficient disk space; mtools + fsck.fat'
