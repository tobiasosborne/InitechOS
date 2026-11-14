# Full registry, named receipts (checked separately), and the top-level verdict.
FNR==NR {
    if ($0 !~ /^[a-z][a-z0-9\/]*:([[:space:]].*)?$/) bad=1
    name=$1; if (name in registry) bad=1
    registry[name]=1; total++; next
}
/^registered [0-9]+, run [0-9]+; elapsed [0-9]+s$/ {
    registered=$2+0; run=$4+0; receipts++
}
/^ALL GREEN$/ {green++}
/^SUBSET GREEN|component\(s\) RED/ {bad=1}
END {
    if (bad || !total || receipts!=1 || registered!=total || run!=total || green!=1) {
        print "FAIL cert: host execution did not cover the complete registry"; exit 1
    }
}
