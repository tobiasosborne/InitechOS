set -eu
dd if=/dev/zero of=build/fat12_blank_wp.img bs=512 count=2880 status=none
mformat -i build/fat12_blank_wp.img -f 1440 ::
printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; positioned-WRITE oracle target)\n" "build/fat12_blank_wp.img"

printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat-write-partial : FAT12 positioned-WRITE oracle\n'
printf '======================================================================\n'
command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-write-partial FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
command -v "${PYTHON_FAT_REFERENCE:-python3}" >/dev/null 2>&1 || { printf '!!! test-fat-write-partial FAIL: "${PYTHON_FAT_REFERENCE:-python3}" not found (needed for the independent reference).\n'; exit 1; }
printf '>>> test-fat-write-partial [A]: in-process matrix -- read_file/read_partial == byte model (+ disk-full rollback)\n'
cp -f "build/fat12_blank_wp.img" "build/fatwp_unit.img"
build/test_fat12_write_partial "build/fatwp_unit.img" \
	|| { printf '!!! test-fat-write-partial FAIL [A]: in-process matrix red\n'; exit 1; }
printf '>>> test-fat-write-partial [A]: green\n'
printf '>>> test-fat-write-partial [B]: OUR writer writes the files into a fresh blank image\n'
cp -f "build/fat12_blank_wp.img" "build/fatwp_diff.img"
build/test_fat12_write_partial "build/fatwp_diff.img" --diff \
	|| { printf '!!! test-fat-write-partial FAIL [B]: writing the diff fixtures failed\n'; exit 1; }
printf '>>> test-fat-write-partial [C]: content -- mcopy == "${PYTHON_FAT_REFERENCE:-python3}" ref, per positioned-written file\n'
set -e; \
for f in OVR.BIN APP.BIN HOLE.BIN FRESH.BIN BIG.BIN; do \
	mcopy -n -i "build/fatwp_diff.img" "::$f" "build/fatwp_mtools_$f.bin" \
		|| { printf '!!! test-fat-write-partial FAIL [C]: mcopy ::%s errored (mtools could not read our written file)\n' "$f"; exit 1; }; \
	"${PYTHON_FAT_REFERENCE:-python3}" "kernel/tests/disk/fat12_ref.py" "build/fatwp_diff.img" --cat "$f" > "build/fatwp_py_$f.bin" \
		|| { printf '!!! test-fat-write-partial FAIL [C]: fat12_ref.py --cat %s errored\n' "$f"; exit 1; }; \
	cmp -s "build/fatwp_mtools_$f.bin" "build/fatwp_py_$f.bin" \
		|| { printf '!!! test-fat-write-partial FAIL [C]: %s -- mcopy bytes != python ref bytes (root-cause the writer, the repository contract)\n' "$f"; \
		     cmp "build/fatwp_mtools_$f.bin" "build/fatwp_py_$f.bin"; exit 1; }; \
done
printf '>>> test-fat-write-partial [C]: green (mcopy + "${PYTHON_FAT_REFERENCE:-python3}" agree on every positioned-written file)\n'
printf '>>> test-fat-write-partial: green\n'
