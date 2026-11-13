set -eu
dd if=/dev/zero of=build/fat12_fuzz_blank.img bs=512 count=2880 status=none
mformat -i build/fat12_fuzz_blank.img -f 1440 ::
printf ">>> fat12: minted BLANK %s (1.44MB FAT12, no files; generative fuzzer template)\n" "build/fat12_fuzz_blank.img"

cp -f build/fat12_fuzz_blank.img build/test-fat-corrupt-fuzz.fuzzblank.img
printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-fat-corrupt-fuzz : FAT12 corruption fuzzer (deterministic seed sweep)\n'
printf '======================================================================\n'
printf '>>> test-fat-corrupt-fuzz: malformed FAT12 inputs over seeds %s..%s (fail-loud + bounded)\n' "1" "${CORRUPT_SEEDS}"
timeout 120 build/fat12_corrupt_fuzz --sweep 1 "${CORRUPT_SEEDS}" --ops 40 "build/test-fat-corrupt-fuzz.fuzzblank.img" \
	|| { printf '!!! test-fat-corrupt-fuzz FAIL: a malformed input hung, went OOB, or was NOT rejected fail-loud -- root-cause it (the repository contract)\n'; rm -f "build/test-fat-corrupt-fuzz.fuzzblank.img".cseed*.scratch; exit 1; }
rm -f "build/test-fat-corrupt-fuzz.fuzzblank.img".cseed*.scratch
printf '>>> test-fat-corrupt-fuzz: green (every malformed FAT12 input failed loud + bounded -- no hang, no OOB)\n'
