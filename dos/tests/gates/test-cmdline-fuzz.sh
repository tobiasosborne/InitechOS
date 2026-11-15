set -eu
printf ">>> test-cmdline-fuzz: cmd-line tokenizer + PSP cmd_tail generative fuzzer over seeds %s..%s (no overflow / clamp to 126/127)\n" "1" "${CONFIG_SEEDS}"
build/test_cmdline_fuzz --sweep 1 "${CONFIG_SEEDS}" --ops 320
printf ">>> test-cmdline-fuzz: green\n"
