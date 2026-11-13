set -eu
printf ">>> test-batch-exec: IF [NOT] ERRORLEVEL/EXIST/str==str + FOR parse/tokenize/subst\n"
build/test_batch_exec
printf ">>> test-batch-exec: green\n"
