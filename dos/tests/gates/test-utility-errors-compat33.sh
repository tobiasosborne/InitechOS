set -eu
status=0
ref=dos/command/tests/compat33/expected
gzip -dc "$ref/U04/data.img.gz" > build/actual.img
cp build/actual.img build/before.img
# Raw core return codes are recorded, not asserted as period exit-code goldens:
# those need an AH=4Dh capture. Setup errors still fail the adapter.
CHKDSK_REPORT_ONLY=1 build/chkdsk_host build/actual.img /Z > build/chkdsk
head -3 "$ref/U04/files/UTIL.OUT" > build/expected
head -3 build/chkdsk > build/actual
if ! cmp build/expected build/actual;then echo 'RED CHKDSK captured invalid-switch prefix' >&2;status=1;fi
if ! grep -F 'bytes total disk space' build/chkdsk >/dev/null;then echo 'RED CHKDSK stopped before its report' >&2;status=1;fi
if grep -E '^ *0 bytes in 0 hidden files' build/chkdsk >/dev/null;then echo 'RED CHKDSK uncaptured zero-hidden row' >&2;status=1;fi
for pair in 2:U09 3:U10 1:U08;do
 kind=${pair%:*};capture=${pair#*:}
 DISKUTIL_RAW_OUTPUT=1 DISKUTIL_REPORT_ONLY=1 build/formatsys_host build/actual.img build/actual.img "$kind" /Z '' 0 > build/actual
 if ! cmp "$ref/$capture/files/UTIL.OUT" build/actual;then echo "RED $capture captured utility refusal bytes" >&2;status=1;fi
done
rc=0;build/dosutil_host build/actual.img attrib /Z > build/actual 2> build/console || rc=$?
case $rc in 0|1) ;; *) echo 'ERROR ATTRIB adapter did not complete' >&2;exit 2;;esac
if ! cmp "$ref/U13/files/UTIL.OUT" build/actual;then echo 'RED ATTRIB diagnostic leaked to stdout' >&2;status=1;fi
tag=dos/command/tests/dos33/reference/attrib-error
awk '/^ATTRIB-BEGIN/{p=1;next} /^ATTRIB-END/{p=0} p{sub(/ *$/, "");print}' "$tag/result.txt" > build/expected
tr -d '\r' < build/console > build/actual-console
if ! cmp build/expected build/actual-console;then echo 'RED ATTRIB captured CON blank line and diagnostic' >&2;status=1;fi
cmp build/actual.img build/before.img
[ "$status" -eq 0 ] || exit 1
echo 'GREEN utility refusals: genuine raw bytes, report continuation, CON framing and unchanged volume'
