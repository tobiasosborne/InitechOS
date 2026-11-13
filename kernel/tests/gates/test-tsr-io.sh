set -eu
build/test_tsr_io
printf '>>> test-tsr-io: green (BIOS flag byte Ctrl+Alt=0Ch; text page save/restore == pixel snapshot + cursor)\n'
