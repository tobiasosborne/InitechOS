set -eu
# initech-xfg8: expansion semantics graded against genuine DOS 3.30 output
# (compat33 captures), never against this implementation's own output.
fail() { echo "FAIL batch expand compat33: $*"; echo 'INITECH_CHECKS 1 failures=1'; exit 1; }

# Overflow: DOS executes the expansion truncated to its 127-character
# command buffer instead of skipping the line (compat33 PARAMS: expanded
# ECHO is 176 chars; captured output payload is 122 = 127 - "echo ").
ref=dos/command/tests/compat33/expected/PARAMS
gzip -dc "$ref/data.img.gz" > build/actual.img
mtype -i build/actual.img ::PARAMS.BAT > build/PARAMS.BAT
tail=$(tr -d '\r' < build/PARAMS.BAT | sed -n 's/^command \/c PARAM.BAT\(.*\)$/\1/p')
[ -n "$tail" ] || fail 'captured PARAMS invocation not found'
sed -n '2,3p' "$ref/files/TRACE.TXT" > build/expected
build/batch_fat build/actual.img PARAM.BAT "$tail" > build/actual || fail 'PARAM.BAT host run failed'
cmp build/expected build/actual || fail 'overexpanded line differs from captured truncated execution'
echo 'GREEN batch expand overflow: captured truncated ECHO, then SHIFT and next ECHO'

# Unmatched percent: captured `echo PERCENT=[%%][trailing%]` prints
# `PERCENT=[%][trailing]` in genuine DOS; the unmatched % contributes
# no bytes (compat33 BATCH TRACE.TXT).
ref=dos/command/tests/compat33/expected/BATCH
gzip -dc "$ref/data.img.gz" > build/actual.img
mtype -i build/actual.img ::BATCH.BAT | tr -d '\r' | sed -n '/^echo PERCENT=/p' > build/lines
[ -s build/lines ] || fail 'captured PERCENT line not found'
sed -n '/^PERCENT=/p' "$ref/files/TRACE.TXT" > build/expected
build/batch_fat build/actual.img build/lines > build/actual || fail 'PERCENT host run failed'
cmp build/expected build/actual || fail 'unmatched percent differs from captured output'
echo 'GREEN batch expand percent: %% collapses and unmatched % contributes no bytes'
echo 'INITECH_CHECKS 2 failures=0'
