set -eu
dd if=/dev/zero of=build/absdisk_scratch.img bs=512 count=2880 status=none
mformat -i build/absdisk_scratch.img -f 1440 ::
printf ">>> absdisk: minted BLANK %s (1.44MB FAT12, no files; INT 25h/26h scratch target)\n" "build/absdisk_scratch.img"

printf '======================================================================\n'
printf 'InitechOS (InitechOS) -- make test-absdisk : INT 25h/26h ABSOLUTE DISK (DEC-15)\n'
printf '  Ref: kernel/tests/data/absdisk_int2526.json (LOCKED); ADR-0003 Amendment DEC-15.\n'
printf '  beads initech-4mq7. the repository contract (oracle is the truth).\n'
printf '======================================================================\n'
cp -f "build/absdisk_scratch.img" "build/absdisk_run.img"
build/test_absdisk "build/absdisk_run.img"
printf ">>> test-absdisk: green (round-trip + cross-check + non-corruption + error legs)\n"
