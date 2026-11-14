set -eu
ref=dos/command/tests/compat33/expected/SWITCH
gzip -dc "$ref/data.img.gz" > build/reference.img
mtype -i build/reference.img ::B.TXT > build/expected-file
mtype -i build/reference.img ::SWITCH.BAT | sed -n '/^ren /p' > build/command
grep -F 'Invalid parameter' "$ref/result.txt" >/dev/null
truncate -s 1474560 build/actual.img
mformat -i build/actual.img -f 1440 ::
mcopy -i build/actual.img build/expected-file ::B.TXT
cp build/actual.img build/before.img
build/shell_files build/actual.img build/command > build/diagnostic
if ! mtype -i build/actual.img ::B.TXT > build/actual-file;then
 echo 'RED REN invalid switch: source was renamed' >&2;exit 1
fi
cmp build/actual-file build/expected-file
cmp build/actual.img build/before.img
[ -s build/diagnostic ] || { echo 'RED REN invalid switch: refusal was silent' >&2;exit 1; }
echo 'GREEN REN invalid switch: captured source bytes and whole-volume preservation'
