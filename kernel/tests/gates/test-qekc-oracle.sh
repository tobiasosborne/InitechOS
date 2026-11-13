# initech-cpd4n: the verdict must reject a helper that never performs WRITE.
set -eu
mv build/test_fileio_subdir build/filetime-original
trap 'rm -f build/test_fileio_subdir; mv build/filetime-original build/test_fileio_subdir' EXIT HUP INT TERM
cat > build/test_fileio_subdir <<'EOF'
#!/bin/sh
if [ "${4:-}" = filetime-persist ]; then
    echo 'injected FILETIME persist failure before WRITE' >&2
    exit 23
fi
exec "$(dirname "$0")/filetime-original" "$@"
EOF
chmod +x build/test_fileio_subdir
rc=0
sh kernel/tests/gates/test-qekc.sh > build/qekc-rejection.log 2>&1 || rc=$?
if [ "$rc" -ne 23 ]; then
    cat build/qekc-rejection.log
    echo "RED qekc oracle accepted a failed WRITE helper: rc=$rc"
    exit 1
fi
grep 'injected FILETIME persist failure' build/qekc-rejection.log
echo 'GREEN qekc oracle rejects failed persist helper and retains diagnostics'
