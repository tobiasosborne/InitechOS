set -eu
printf ">>> test-fileio: OPEN/READ/CLOSE/LSEEK + FINDFIRST/NEXT + SETDTA/GETDTA via mock backend (initech-509.5)\n"
build/test_fileio
printf ">>> test-fileio: green\n"
