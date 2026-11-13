set -eu
printf ">>> test-ansi-wire: con_putc -> ansi_feed -> console-ops (gate + cursor + erase + SGR + save/restore + plain-text identity)\n"
build/test_ansi_wire
printf ">>> test-ansi-wire: green\n"
