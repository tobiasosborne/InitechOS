#!/bin/sh
# Reuse the independent original-file/mtools input oracle. The
# adapter changes only which operation is graded on each extracted real file.
set -eu
out=$2
oracle=$3
mode=${4:-replace-corpus}
mkdir -p "$out"
if [ "$mode" = replace-corpus ]; then
cat > "$out/oracle" <<EOF_ADAPTER
#!/bin/sh
set -eu
"$oracle" "$mode" "\$1" "$out/trials"
exec "${oracle%/*}/iword_replace_ref" "\$1" "$out/trials"
EOF_ADAPTER
else
cat > "$out/oracle" <<EOF_ADAPTER
#!/bin/sh
exec "$oracle" "$mode" "\$1"
EOF_ADAPTER
fi
chmod +x "$out/oracle"
sh tests/diff/search.sh "$1" "$out" "$out/oracle"
printf '>>> IWORD corpus (%s): 32 real documents, independent byte comparisons\n' "$mode"
