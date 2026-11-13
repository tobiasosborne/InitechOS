set -eu
printf ">>> test-config-sys: CONFIG.SYS parser vs the LOCKED baseline + edge cases (initech-509.2)\n"
build/test_config_sys
printf ">>> test-config-sys: green\n"
