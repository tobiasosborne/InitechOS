#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$root/build/dos-datetime-entry-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
# No AUTOEXEC.BAT: genuine COMMAND.COM asks for date and time itself.
# (Plain media has none; del in case a future media copy carries one.)
mdel -i "$d/boot.img" ::AUTOEXEC.BAT 2>/dev/null || true
cat > "$d/steps" <<'EOF'
wait Enter new date
shot dateprompt
type 13-40-90
key ret
sleep 1
shot dateinvalid
type 2-29-92
key ret
wait Enter new time
shot timeprompt
type 25:00
key ret
sleep 1
shot timeinvalid
type 13:14:15.99
key ret
wait A>
type date
key ret
wait Enter new date
shot datecmd
key ret
type time
key ret
wait Enter new time
shot timecmd
type 9:08:07.06
key ret
type time
key ret
wait Enter new time
shot timeagain
key ret
type ver
key ret
sleep 1
shot final
end
EOF
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" - - "$d/steps" > "$d/driver.log" 2>&1
for f in dateprompt dateinvalid timeprompt timeinvalid datecmd timecmd timeagain final; do echo "=== $f"; cat "$d/$f.txt"; done
