set -eu
printf ">>> test-redir-parse: cmd_redir_parse -- '>' truncate / '>>' append / target+clean extraction / no-redir passthrough / last-wins / bound guards (beads initech-hsct)\n"
build/test_redir_parse
printf ">>> test-redir-parse: green\n"
