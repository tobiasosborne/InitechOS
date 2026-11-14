# Compare the independently queried complete registry with every status row.
FNR==NR {
    if ($0 !~ /^[a-z][a-z0-9\/]*:([[:space:]].*)?$/) { bad=1; next }
    name=$1; sub(/:$/, "", name)
    if (name in expected) bad=1
    expected[name]=1; total++; next
}
{
    name=$1; sub(/:$/, "", name)
    if (name in expected || $0 ~ /^[a-z][a-z0-9\/]*: (imported|host-green|red)$/) {
        if (!(name in expected) || ++seen[name]!=1 || NF!=2 || $2!="imported") {
            print "FAIL cert import: " $0; bad=1
        }
    }
}
END {
    if (!total) { print "FAIL cert: empty component registry"; bad=1 }
    for (name in expected) if (seen[name]!=1) {
        print "FAIL cert: missing/duplicate status for " name; bad=1
    }
    exit bad
}
