set -eu
printf ">>> test-exec-unit: AH=4Bh EXEC (path validation + not-found/nested mapping) + AH=4Dh GET-RETURN-CODE via mock backend\n"
build/test_exec
printf ">>> test-exec-unit: green\n"
