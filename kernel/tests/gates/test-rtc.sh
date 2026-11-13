set -eu
printf ">>> test-rtc: MC146818 BCD/binary/12-24h/century/DOW + SET encode round-trip (initech-yv9)\n"
build/test_rtc
printf ">>> test-rtc: green\n"
