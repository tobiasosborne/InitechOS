set -eu
ref=dos/command/tests/compat33/expected/BATCH
gzip -dc "$ref/data.img.gz" > build/actual.img
mtype -i build/actual.img ::BATCH.BAT > build/captured-batch
status=0
for name in TOKEN FILE WILD;do
 sed -n "/^for .*echo $name=/p" build/captured-batch > build/commands
 sed -n "/^$name=/p" "$ref/files/TRACE.TXT" > build/expected
 build/batch_fat build/actual.img build/commands > build/actual
 if ! cmp build/expected build/actual;then
  echo "FAIL FOR $name: captured output differs" >&2;status=1
 fi
done
# A wildcard without matches must still run zero bodies.
sed -n '/^for .*UNMATCHED=/p' build/captured-batch > build/commands
build/batch_fat build/actual.img build/commands > build/actual
[ ! -s build/actual ] || { echo 'FAIL FOR no-match: body ran with no wildcard match' >&2; status=1; }
[ "$status" = 0 ] || { echo 'INITECH_CHECKS 1 failures=1'; exit 1; }
echo 'GREEN FOR: captured separators, no-match and wildcard continuation'
echo 'INITECH_CHECKS 1 failures=0'
