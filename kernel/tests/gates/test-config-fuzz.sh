set -eu
printf ">>> test-config-fuzz: CONFIG.SYS parser generative fuzzer over seeds %s..%s (no overflow / no crash / clamp / lenient)\n" "1" "${CONFIG_SEEDS}"
build/test_config_sys_fuzz --sweep 1 "${CONFIG_SEEDS}" --ops 24 \
	|| { printf '!!! test-config-fuzz FAIL: a seed broke an invariant (replay above) -- root-cause it (the repository contract)\n'; exit 1; }
printf ">>> test-config-fuzz: green\n"
