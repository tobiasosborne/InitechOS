set -eu
printf ">>> test-idt: IDT gate encode (offset-split) + int_frame_t field offsets\n"
build/test_idt
printf ">>> test-idt: green\n"
