set -eu
ref=dos/command/tests/compat33/expected/BATCH
gzip -dc "$ref/data.img.gz" > build/actual.img
mtype -i build/actual.img ::BATCH.BAT | sed -n '/^if exist /p' > build/commands
sed -n '/^WILDCARD-EXISTS/p; /^DIRECTORY-EXISTS/p' "$ref/files/TRACE.TXT" > build/expected
build/batch_fat build/actual.img build/commands > build/actual
cmp build/expected build/actual || {
 echo 'RED IF EXIST: captured wildcard-file or directory decision differs' >&2;exit 1;
}
echo 'GREEN IF EXIST: wildcard matches file; bare directory does not execute body'
