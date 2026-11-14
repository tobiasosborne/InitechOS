set -eu
ref=dos/command/tests/compat33/expected/NAMES
gzip -dc "$ref/data.img.gz" > build/reference.img
mtype -i build/reference.img ::A.TXT > build/source
mtype -i build/reference.img ::LONGFILE.LON > build/expected-file
mtype -i build/reference.img ::NAMES.BAT | sed -n '/^copy /p' > build/command
sed -n '/File(s) copied/p' "$ref/files/TRACE.TXT" > build/expected-output
truncate -s 1474560 build/actual.img
mformat -i build/actual.img -f 1440 ::
mcopy -i build/actual.img build/source ::A.TXT
build/shell_files build/actual.img build/command > build/actual-output
if ! mtype -i build/actual.img ::LONGFILE.LON > build/actual-file;then
 echo 'RED COPY name: captured truncated destination is missing' >&2;exit 1
fi
cmp build/expected-file build/actual-file
cmp build/expected-output build/actual-output
mtype -i build/actual.img ::A.TXT > build/actual-source
cmp build/source build/actual-source
echo 'GREEN COPY name: captured LONGFILE.LON bytes, source and success count'
