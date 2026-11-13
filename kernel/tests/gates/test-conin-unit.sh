set -eu
printf ">>> test-conin-unit: CON input 01h/06h/07h/08h/0Ah(+BS,+clamp)/0Bh/0Ch via mock source\n"
build/test_conin
printf ">>> test-conin-unit: green\n"
