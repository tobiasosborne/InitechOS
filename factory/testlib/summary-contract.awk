# factory/testlib/README.md, Kill contract: classify a test log's summaries.
# Prints "none" (no completed summary), "failures" (a summary counted a
# failure), "empty" (summaries counted no checks) or "ok".
/^INITECH_CHECKS [0-9]+ failures=[0-9]+$/ {
    split($3, a, "="); seen++; if ($2 + 0 > 0) checks++; if (a[2] + 0 > 0) failed++
}
match($0, /[0-9]+ checks, [0-9]+ (failures|failed|failure\(s\))/) {
    split(substr($0, RSTART, RLENGTH), b, " ")
    seen++; if (b[1] + 0 > 0) checks++; if (b[3] + 0 > 0) failed++
}
END {
    if (!seen) print "none"; else if (failed) print "failures"; else if (!checks) print "empty"; else print "ok"
}
