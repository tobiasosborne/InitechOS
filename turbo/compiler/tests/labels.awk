# NASM local labels are scoped to the previous non-local label. Uniqueness
# is checked before invoking the assembler, independently of its exit code.
/^[A-Za-z_.][A-Za-z_0-9.]*:/ {
    label=$1; sub(/:.*$/, "", label)
    if (label !~ /^\./) scope=label
    key=scope ":" label
    if (seen[key]++) { print "duplicate generated label: " key; bad=1 }
}
END { exit bad ? 1 : 0 }
