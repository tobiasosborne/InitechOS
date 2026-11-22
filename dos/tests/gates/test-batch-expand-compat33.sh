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

# Interactive ground truth (compat33 INTERACT, genuine DOS 3.30 at the
# prompt): %VAR% and %% are NOT expanded interactively; env expansion is
# the batch path only. The same screen grades IBAT.BAT's unmatched-%
# variants in a batch: each A=..E= line is the genuine batch output for
# the IBAT input recorded in the capture image.
ref=dos/command/tests/compat33/expected/INTERACT
gzip -dc "$ref/data.img.gz" > build/actual.img
mtype -i build/actual.img ::IBAT.BAT | tr -d '\r' | grep '^echo [A-D]=' > build/lines
sed -n 's/^\([A-D]=\[[^]]*\]\).*$/\1/p' "$ref/result.txt" > build/expected
build/batch_fat build/actual.img build/lines > build/actual || fail 'IBAT host run failed'
tr -d '\r' < build/actual > build/text
cmp build/expected build/text || fail 'batch unmatched-percent variants differ from genuine DOS screen'
echo 'GREEN batch expand unmatched variants: IBAT A-D match the genuine DOS capture'

ref=dos/command/tests/dos33/reference/shift-window
truncate -s 1474560 build/shift.img
mformat -i build/shift.img -f 1440 ::
for name in PARENT CHILD; do
    awk '{printf "%s\r\n",$0}' "$ref/$name.txt" > "build/$name.BAT"
    mcopy -i build/shift.img "build/$name.BAT" ::
done
build/batch_fat build/shift.img PARENT.BAT "$(cat "$ref/tail.txt")" > build/actual || fail 'SHIFT batch run failed'
tr -d '\r' < build/actual > build/text
cmp "$ref/trace.txt" build/text || fail 'SHIFT positions differ from the genuine DOS capture'
echo 'GREEN SHIFT: captured late positions, nested CALL and exhaustion'
echo 'INITECH_CHECKS 4 failures=0'
