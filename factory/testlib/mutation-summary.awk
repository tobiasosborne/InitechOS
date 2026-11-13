# CLAUDE.md, The oracle is the truth: process success alone proves no kills.
# Count every component summary, including independently scheduled groups.
/mutants,/ {
    if ($1 != component ":" || NF < 9 || $3 != "mutants," ||
        $5 != "killed," || $7 != "survived," || $9 !~ /^errors(;|$)/ ||
        $2 !~ /^[0-9]+$/ || $4 !~ /^[0-9]+$/ ||
        $6 !~ /^[0-9]+$/ || $8 !~ /^[0-9]+$/) {
        print "FAIL " component ": invalid mutation summary: " $0 > "/dev/stderr"
        bad=1; next
    }
    rows++; total+=$2; killed+=$4; survived+=$6; errors+=$8
    if ($2 != $4 || $6 != 0 || $8 != 0) bad=1
}
END {
    if (!rows) {print "FAIL " component ": mutation target reported no counts" > "/dev/stderr";bad=1}
    if (total != expected || killed != expected || survived || errors) {
        print "FAIL " component ": expected " expected " kills; got " total " mutants, " killed " killed, " survived " survived, " errors " errors" > "/dev/stderr"
        bad=1
    }
    if (bad) exit 1
    print total, killed, survived, errors
}
