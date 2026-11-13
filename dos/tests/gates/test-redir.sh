set -eu
printf ">>> test-redir: AH=02h/09h/06h output via redirectable STDOUT handle 1 (default CON / DUP2(file,1) / no-PSP fallback) -- beads initech-k36g\n"
build/test_redir
printf ">>> test-redir: green\n"
