set -eu
printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat-write : FAT12 WRITE round-trip oracle\n'
printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-509.11.\n'
printf '  OUR writer mints a blank image + writes files; mtools + "${PYTHON_FAT_REFERENCE:-python3}" read it back.\n'
printf '======================================================================\n'
# ---- (B) Tool presence: fail loud, NEVER silently skip (the repository contract). ----
command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: mtools `mcopy` not found.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found.\n'; exit 1; }
printf '>>> test-fat-write [B]: reference tools present (mtools + "${PYTHON_FAT_REFERENCE:-python3}")\n'
# ---- (A) The WRITE unit + in-memory round-trip oracle. ----
dd if=/dev/zero of="build/fatw_unit.img" bs=512 count=2880 status=none
mformat -i "build/fatw_unit.img" -f 1440 ::
build/test_fat12_write "build/fatw_unit.img" \
	|| { printf '!!! test-fat-write FAIL [A]: test_fat12_write unit/round-trip red\n'; exit 1; }
printf '>>> test-fat-write [A]: WRITE unit + in-memory round-trip green\n'
# ---- (C) Mint a clean BLANK image and write the 4 fixtures with OUR writer. ----
dd if=/dev/zero of="build/fatw_diff.img" bs=512 count=2880 status=none
mformat -i "build/fatw_diff.img" -f 1440 ::
build/test_fat12_write "build/fatw_diff.img" --diff \
	|| { printf '!!! test-fat-write FAIL [C]: writing the diff fixtures failed\n'; exit 1; }
printf '>>> test-fat-write [C]: OUR writer wrote 4 files into a blank image\n'
# ---- (D) NAMES+SIZES: normalized mdir == "${PYTHON_FAT_REFERENCE:-python3}" --list. ----
printf '>>> test-fat-write [D]: names+sizes -- "${PYTHON_FAT_REFERENCE:-python3}" reader == normalized mdir on OUR image\n'
"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fatw_diff.img" --list > "build/fatw_py.list" \
	|| { printf '!!! test-fat-write FAIL [D]: fat12_ref.py --list errored on our written image\n'; exit 1; }
mdir -i "build/fatw_diff.img" :: | awk '{ \
	hd=0; di=0; \
	for(i=1;i<=NF;i++){ if($i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$/){hd=1;di=i;break} } \
	if(!hd) next; \
	name=$1; ext=$2; sz=""; \
	for(i=3;i<di;i++){ sz=sz $i } \
	if(sz !~ /^[0-9]+$/) next; \
	fn=(ext!="")?name"."ext:name; \
	print fn, sz \
}' | sort > "build/fatw_mtools.list"
diff -u "build/fatw_py.list" "build/fatw_mtools.list" \
	|| { printf '!!! test-fat-write FAIL [D]: python listing != normalized mdir listing of OUR written image\n'; exit 1; }
printf '    agreement on names+sizes (%s files):\n' "$(wc -l < "build/fatw_py.list" | tr -d ' ')"
sed 's/^/      /' "build/fatw_py.list"
# ---- (E) CONTENT per file: mcopy == "${PYTHON_FAT_REFERENCE:-python3}" --cat == our deterministic bytes. ----
printf '>>> test-fat-write [E]: content -- mcopy == python ref == our written bytes, per file\n'
for f in SHORT.TXT MULTI.DAT EXACT.BIN EMPTY.NEW; do \
	mcopy -i "build/fatw_diff.img" "::$f" - > "build/fatw_mtools_$f.bin" 2>/dev/null \
		|| { printf '!!! test-fat-write FAIL [E]: mcopy ::%s errored (mtools could not read our written file)\n' "$f"; exit 1; }; \
	"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fatw_diff.img" --cat "$f" > "build/fatw_py_$f.bin" \
		|| { printf '!!! test-fat-write FAIL [E]: fat12_ref.py --cat %s errored\n' "$f"; exit 1; }; \
	cmp -s "build/fatw_mtools_$f.bin" "build/fatw_py_$f.bin" \
		|| { printf '!!! test-fat-write FAIL [E]: %s -- mcopy bytes != python ref bytes (root-cause the writer, the repository contract)\n' "$f"; cmp "build/fatw_mtools_$f.bin" "build/fatw_py_$f.bin"; exit 1; }; \
	printf '    %-12s %6s bytes : mcopy == python\n' "$f" "$(wc -c < "build/fatw_mtools_$f.bin" | tr -d ' ')"; \
done
printf '%s\n' '----------------------------------------------------------------------'
printf 'VERDICT   : PASS -- a volume written ENTIRELY by InitechDOS reads back correctly\n'
printf '            in mtools AND an independent "${PYTHON_FAT_REFERENCE:-python3}" reader (names, sizes, content)\n'
printf '            AND in our own reader (in-memory round-trip).\n'
printf '======================================================================\n'
