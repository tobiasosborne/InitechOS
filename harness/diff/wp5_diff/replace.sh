#!/bin/sh
# Reuse the independent MILTON/mtools identical-image input oracle. The
# adapter changes only which operation is graded on each extracted real file.
set -eu
out=$2
oracle=$4
mode=${5:-replace-corpus}
mkdir -p "$out"
cat > "$out/oracle" <<EOF
#!/bin/sh
exec "$oracle" "$mode" "\$1"
EOF
chmod +x "$out/oracle"
sh harness/diff/wp5_diff/search.sh "$1" "$out" "$3" "$out/oracle"
printf '>>> IWORD corpus (%s): 32 real documents, independent byte comparisons\n' "$mode"
