set -eu
ref=dos/command/tests/compat33/expected/BATCH
gzip -dc "$ref/data.img.gz" > build/actual.img
# Isolate the captured first ECHO; the invocation and argument list are exact.
mtype -i build/actual.img ::BATCH.BAT | sed -n '1,2p' > build/script
mcopy -o -i build/actual.img build/script ::BATCH.BAT
sed -n '/^PARAMS=/p' "$ref/files/TRACE.TXT" > build/expected
tail=$(tr -d '\r' < "$ref/AUTOEXEC.BAT" | sed -n 's/^command \/c BATCH.BAT\(.*\) > TRACE.TXT$/\1/p')
[ -n "$tail" ]
build/batch_fat build/actual.img BATCH.BAT "$tail" > build/actual
cmp build/expected build/actual || { echo 'RED batch invocation: captured %0 differs' >&2;exit 1; }
echo 'GREEN batch invocation: captured %0 and positional parameters'
