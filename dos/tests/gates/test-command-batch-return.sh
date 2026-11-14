set -eu
ref=dos/command/tests/compat33/expected/BATCH
gzip -dc "$ref/data.img.gz" > build/actual.img
mtype -i build/actual.img ::BATCH.BAT > build/captured-batch
sed -n '1p; /^command \/c CHILD.BAT/p; /^echo AFTER-COMMAND/p' build/captured-batch > build/script
mcopy -o -i build/actual.img build/script ::BATCH.BAT
# The child's captured commands must overwrite the parent's saved offset;
# a shorter child would leave the old parent tail in memory and hide the bug.
sed -n '1p; /^for .*echo TOKEN=/p; /^for .*echo FILE=/p' build/captured-batch > build/child
mcopy -o -i build/actual.img build/child ::CHILD.BAT
sed -n '/^TOKEN=/p; /^FILE=/p; /^AFTER-COMMAND/p' "$ref/files/TRACE.TXT" > build/expected
build/batch_fat build/actual.img BATCH.BAT '' > build/actual
cmp build/expected build/actual || { echo 'RED secondary COMMAND: parent batch did not resume' >&2;exit 1; }
echo 'GREEN secondary COMMAND: captured child and resumed-parent bytes'
