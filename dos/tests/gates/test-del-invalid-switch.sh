set -eu
ref=dos/command/tests/compat33/expected/SWITCH
gzip -dc "$ref/data.img.gz" > build/reference.img
mtype -i build/reference.img ::DEL.TXT > build/expected-file
mtype -i build/reference.img ::SWITCH.BAT | sed -n '/^del /p' > build/command
# Ground the command and the preserved bytes in genuine DOS, not a guessed mask.
grep -F 'Invalid parameter' "$ref/result.txt" >/dev/null
truncate -s 1474560 build/actual.img
mformat -i build/actual.img -f 1440 ::
mcopy -i build/actual.img build/expected-file ::DEL.TXT
cp build/actual.img build/before.img
build/shell_files build/actual.img build/command > build/diagnostic
if ! mtype -i build/actual.img ::DEL.TXT > build/actual-file;then
 echo 'RED DEL invalid switch: protected file was deleted' >&2;exit 1
fi
cmp build/actual-file build/expected-file
cmp build/actual.img build/before.img
[ -s build/diagnostic ] || { echo 'RED DEL invalid switch: refusal was silent' >&2;exit 1; }
echo 'GREEN DEL invalid switch: genuine captured operand bytes and whole-volume preservation'
