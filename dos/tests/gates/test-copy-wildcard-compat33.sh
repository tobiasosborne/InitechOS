set -eu
# Genuine DOS 3.30 captures (capture-copywild{,2}.sh): each case replays the
# captured COPY line on a copy of the captured input disk and compares the
# resulting destination bytes and console text with the captured output.
fail() { echo "FAIL copy-wildcard $*" >&2; echo "INITECH_CHECKS 1 failures=1"; exit 1; }
for spec in 'copywild-deleted ALL.TXT' 'copywild-plain ALL.TXT' \
            'copywild-existing ALL.TXT' 'copywild2-outside ALL.DAT' \
            'copywild2-inside ALL.TXT' 'copywild2-plus ALL.TXT'; do
 case=${spec%% *}; outname=${spec##* }
 ref=dos/command/tests/dos33/reference/$case
 gzip -dc "$ref/before-data.img.gz" > build/actual.img
 tr -d '\r' < "$ref/CPYCASE.BAT" | sed -n '/^copy /p' > build/command
 build/shell_files build/actual.img build/command > build/actual-output
 mtype -i build/actual.img "::$outname" > build/actual-file 2>/dev/null || fail "$case: destination missing"
 cmp "$ref/ALL-OUT.TXT" build/actual-file || fail "$case: destination bytes"
 grep -v 'COPYWILD-DONE' "$ref/TRACE.TXT" > build/expected-output
 cmp build/expected-output build/actual-output || { diff build/expected-output build/actual-output >&2 || true; fail "$case: console text"; }
 fsck.fat -n build/actual.img > build/fsck 2>&1 || { cat build/fsck; fail "$case: fsck"; }
done
echo 'GREEN COPY wildcard: captured destination bytes and console text for deleted/plain/existing/outside/inside/plus layouts'
echo "INITECH_CHECKS 6 failures=0"
