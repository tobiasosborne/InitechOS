set -eu
dd if=/dev/zero of=build/fat12_fixture.img bs=512 count=2880 status=none
mformat -i build/fat12_fixture.img -f 1440 ::
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/hello.txt ::HELLO.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/second.txt ::SECOND.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/chain.txt ::CHAIN.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/empty.txt ::EMPTY.TXT
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/block.bin ::BLOCK.BIN
mcopy -i build/fat12_fixture.img kernel/tests/fixtures/bigchain.txt ::BIGCHAIN.TXT
printf ">>> fat12: minted %s (1.44MB FAT12, fixtures copied; build intermediate)\n" "build/fat12_fixture.img"
sh kernel/tests/disk/verify_fixture_volume.sh recipe "build/fat12_fixture.img" HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN BIGCHAIN.TXT

printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat : FAT12 differential oracle\n'
printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-5cu / initech-adf.\n'
printf '  Our C reader vs TWO independent refs (mtools + "${PYTHON_FAT_REFERENCE:-python3}") on one image.\n'
printf '======================================================================\n'
# ---- (D) Tool presence: fail loud, NEVER silently skip (the repository contract). ----
command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat FAIL: mtools `mcopy` not found (apt install mtools).\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (needed for the independent reference reader).\n'; exit 1; }
printf '>>> test-fat [D]: reference tools present (mtools mdir/mcopy + "${PYTHON_FAT_REFERENCE:-python3}")\n'
# ---- (C) The three FAT12 unit oracles must pass first. ----
printf '>>> test-fat [C]: FAT12 unit oracles (bpb / chain / dir)\n'
build/test_fat12_bpb   "build/fat12_fixture.img" \
	|| { printf '!!! test-fat FAIL: test_fat12_bpb red\n'; exit 1; }
build/test_fat12_chain "build/fat12_fixture.img" \
	|| { printf '!!! test-fat FAIL: test_fat12_chain red\n'; exit 1; }
build/test_fat12_dir   "build/fat12_fixture.img" "kernel/tests/fixtures" \
	|| { printf '!!! test-fat FAIL: test_fat12_dir red\n'; exit 1; }
# ---- (A) NAMES + SIZES: triple agreement (our == python == mdir). ----
printf '>>> test-fat [A]: names+sizes -- our fat_dump == python ref == normalized mdir\n'
build/fat_dump "build/fat12_fixture.img" --list > "build/fat12_our.list" \
	|| { printf '!!! test-fat FAIL: fat_dump --list errored\n'; exit 1; }
"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fat12_fixture.img" --list > "build/fat12_py.list" \
	|| { printf '!!! test-fat FAIL: fat12_ref.py --list errored\n'; exit 1; }
# Normalize mdir: keep only rows carrying a YYYY-MM-DD date (real entries),
# reassemble the space-grouped thousands in the size, drop date/time/serial,
# emit "NAME.EXT <size>" and sort -- timestamps/serial normalized away.
mdir -i "build/fat12_fixture.img" :: | awk '{ \
	hd=0; di=0; \
	for(i=1;i<=NF;i++){ if($i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$/){hd=1;di=i;break} } \
	if(!hd) next; \
	name=$1; ext=$2; sz=""; \
	for(i=3;i<di;i++){ sz=sz $i } \
	if(sz !~ /^[0-9]+$/) next; \
	fn=(ext!="")?name"."ext:name; \
	print fn, sz \
}' | sort > "build/fat12_mtools.list"
diff -u "build/fat12_our.list" "build/fat12_py.list" \
	|| { printf '!!! test-fat FAIL [A]: our listing != python reference listing\n'; exit 1; }
diff -u "build/fat12_our.list" "build/fat12_mtools.list" \
	|| { printf '!!! test-fat FAIL [A]: our listing != normalized mdir listing\n'; exit 1; }
printf '    triple agreement on names+sizes (%s files):\n' "$(wc -l < "build/fat12_our.list" | tr -d ' ')"
sed 's/^/      /' "build/fat12_our.list"
# ---- (B) CONTENT per file: our == mcopy == python, byte-for-byte. ----
printf '>>> test-fat [B]: content -- our fat_dump --cat == mcopy == python ref, per file\n'
for f in HELLO.TXT SECOND.TXT CHAIN.TXT EMPTY.TXT BLOCK.BIN BIGCHAIN.TXT; do \
	build/fat_dump "build/fat12_fixture.img" --cat "$f" > "build/fat12_our_$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: fat_dump --cat %s errored\n' "$f"; exit 1; }; \
	mcopy -i "build/fat12_fixture.img" "::$f" - > "build/fat12_mtools_$f.bin" 2>/dev/null \
		|| { printf '!!! test-fat FAIL [B]: mcopy ::%s errored\n' "$f"; exit 1; }; \
	"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fat12_fixture.img" --cat "$f" > "build/fat12_py_$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: fat12_ref.py --cat %s errored\n' "$f"; exit 1; }; \
	cmp -s "build/fat12_our_$f.bin" "build/fat12_mtools_$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: %s -- our bytes != mcopy bytes\n' "$f"; cmp "build/fat12_our_$f.bin" "build/fat12_mtools_$f.bin"; exit 1; }; \
	cmp -s "build/fat12_our_$f.bin" "build/fat12_py_$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: %s -- our bytes != python ref bytes\n' "$f"; cmp "build/fat12_our_$f.bin" "build/fat12_py_$f.bin"; exit 1; }; \
	printf '    %-12s %6s bytes : our == mcopy == python\n' "$f" "$(wc -c < "build/fat12_our_$f.bin" | tr -d ' ')"; \
done
printf '%s\n' '----------------------------------------------------------------------'
printf 'VERDICT   : PASS -- our FAT12 reader agrees with mtools AND an independent\n'
printf '            "${PYTHON_FAT_REFERENCE:-python3}" reader on names, sizes, and content (triple oracle).\n'
printf '======================================================================\n'
