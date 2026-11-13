set -eu
printf '>>> test-overlay: AH=4Bh AL=03h core (factor reloc, footprint, raw, refusals) + dispatch + DEC-19.4 ownership\n'
build/test_overlay
printf '>>> test-overlay: green\n'

build/overlay_dos33_check kernel/tests/fixtures/dos33_overlay/factor-different.bin kernel/tests/fixtures/dos33_overlay/factor-equal.bin
if build/overlay_dos33_negative kernel/tests/fixtures/dos33_overlay/factor-different.bin kernel/tests/fixtures/dos33_overlay/factor-equal.bin > build/overlay-negative.log 2>&1; then
 echo 'FAIL: wrong real-DOS overlay expectation survived'; exit 1
fi
grep -qx 'FAIL DOS33 different-factor fixup address' build/overlay-negative.log
