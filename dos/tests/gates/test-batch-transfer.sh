set -eu
ref=dos/command/tests/dos33/reference/batch-transfer
fail() { echo "FAIL batch transfer: $*"; echo 'INITECH_CHECKS 1 failures=1'; exit 1; }
for mode in PLAIN TCALL TIF TFOR; do
 gzip -dc "$ref/before-data.img.gz" > build/actual.img
 build/batch_fat build/actual.img "$mode.BAT" '' > build/actual || fail "$mode dispatched an invalid body"
 cmp "$ref/$mode.OUT" build/actual || fail "$mode returned into an abandoned parent"
 mtype -i build/actual.img ::KEEP.TXT > build/kept || fail "$mode deleted source"
 cmp "$ref/KEEP.TXT" build/kept || fail "$mode source bytes"
done
echo 'GREEN batch transfer: captured plain/CALL/IF/FOR child and grandchild flow, mtools source preservation'
echo 'INITECH_CHECKS 4 failures=0'
