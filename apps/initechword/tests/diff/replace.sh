#!/bin/sh
# Reuse the independent original-file/mtools input oracle. The
# adapter changes only which operation is graded on each extracted real file.
set -eu
out=$2
oracle=$3
mode=${4:-replace-corpus}
mkdir -p "$out"
cat > "$out/oracle" <<EOF_ADAPTER
#!/bin/sh
exec "$oracle" "$mode" "\$1"
EOF_ADAPTER
chmod +x "$out/oracle"
sh tests/diff/search.sh "$1" "$out" "$out/oracle"
printf '>>> IWORD corpus (%s): 32 real documents, independent byte comparisons\n' "$mode"
