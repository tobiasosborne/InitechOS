#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${4:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
printf '@echo off\r\necho BEGIN > TRACE.TXT\r\ntime 25:00 >> TRACE.TXT\r\necho AFTER-INVALID >> TRACE.TXT\r\ndate >> TRACE.TXT\r\necho AFTER-DATE >> TRACE.TXT\r\ntime 12:34:10.00 >> TRACE.TXT\r\necho AFTER-VALID >> TRACE.TXT\r\ntime >> TRACE.TXT\r\ndate >> TRACE.TXT\r\necho END >> TRACE.TXT\r\n' > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::DONE.COM
# Keys: blank at the invalid-TIME prompt, a leap date at the DATE prompt,
# blank at the bare TIME prompt, then the transport program.
printf '\n2-29-92\n\n\ndone\n' | build/factory/keys > "$out/keys"
if [ "${DOS_SESSION_EMULATOR:-qemu}" = bochs ]; then
    DOS_SESSION_RTC=1992-03-02T09:30:00 sh dos/tests/runtime/execute.sh \
        "$root" "$boot" "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
        error 'datetime transport did not complete'
else
    . dos/tests/runtime/session-media.sh
    prepare_session_boot "$boot" "$out/data.img" "$out/boot.img"
    timeout 40 build/factory/qemu_harness --disk "$out/boot.img" --disk2 "$out/data.img" \
        --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE \
        --quit-after TRANSPORT-DONE --timeout-ms 15000 --rtc-base 1992-03-02T09:30:00 \
        --rtc-deterministic --out "$out" --name datetime > "$out/driver.log" 2> "$out/report" ||
        error 'datetime transport did not complete'
    transport_pass "$out/report"
fi
mtype -i "$out/data.img" ::TRACE.TXT > "$out/trace" || error 'datetime trace missing'
tr -d '\r' < "$out/trace" > "$out/text"
require 'invalid TIME argument reports Invalid time' grep -F 'Invalid time' "$out/text"
require 'invalid TIME argument prompts for entry' grep -F 'Enter new time: ' "$out/text"
require 'bare DATE prints the captured Current line' grep -E 'Current date is Mon  3-02-1992' "$out/text"
require 'bare TIME prompts for entry too' test "$(grep -c 'Enter new time: ' "$out/text")" -ge 2
require 'bare DATE prompts for entry' grep -F 'Enter new date (mm-dd-yy): ' "$out/text"
require 'valid TIME argument prints nothing before the next marker' \
  awk '/AFTER-DATE/{f=1;next} /AFTER-VALID/{f=0} f&&NF{bad=1} END{exit bad?1:0}' "$out/text"
require 'bare TIME shows the set time and prompts' grep -E 'Current time is 12:34:1[0-9]\.' "$out/text"
require 'entered date was set' grep -E 'Current date is Sat  2-29-1992' "$out/text"
require 'session reached the end marker' grep -F 'END' "$out/text"
echo 'GREEN datetime: captured DATE/TIME entry prompts, invalid reprompt, silent valid set'
