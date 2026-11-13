set -eu
printf ">>> test-devices: chain + attr bits + NUL/CON/PRN/AUX/CLOCK$ dispatch\n"
build/test_devices
printf ">>> test-devices: green\n"
