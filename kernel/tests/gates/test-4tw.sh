set -eu
printf ">>> test-4tw: CON-input ^C (0x03) on 01h/08h/0Ah -> INT 23h (Fork A; 07h/06h deliver raw)\n"
build/test_conin
printf ">>> test-4tw: green\n"
