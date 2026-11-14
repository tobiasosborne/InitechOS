set -eu
printf '>>> test-overlay: AH=4Bh AL=03h core (factor reloc, footprint, raw, refusals) + dispatch + DEC-19.4 ownership\n'
build/test_overlay
printf '>>> test-overlay: green\n'

build/overlay_dos33_check kernel/tests/fixtures/dos33_overlay/factor-different.bin kernel/tests/fixtures/dos33_overlay/factor-equal.bin
if build/overlay_dos33_negative kernel/tests/fixtures/dos33_overlay/factor-different.bin kernel/tests/fixtures/dos33_overlay/factor-equal.bin > build/overlay-negative.log 2>&1; then
 echo 'FAIL: wrong real-DOS overlay expectation survived'; exit 1
fi
grep -qx 'FAIL DOS33 different-factor fixup address' build/overlay-negative.log
printf '%s\n' \
 'd265aa24d1d3dabf87957e2707af63b61af43b94121476a522ff649153431fed  kernel/tests/fixtures/dos33_overlay/factor-different.bin' \
 'e771f09529654bc8f8048668b11df6a95023243e6fdd7c29a76cbf2ba8460317  kernel/tests/fixtures/dos33_overlay/factor-equal.bin' | sha256sum -c -
build/test_overlay_captures kernel/tests/fixtures/dos33_overlay/factor-different.bin kernel/tests/fixtures/dos33_overlay/factor-equal.bin
