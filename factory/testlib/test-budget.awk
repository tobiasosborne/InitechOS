# Shell times reports user/system CPU seconds for the recipe and its children.
# Read it from the recipe shell itself: a subshell has different counters.
NF != 2 { bad = 1 }
{
    for (field = 1; field <= NF; field++) {
        if ($field !~ /^[0-9]+m[0-9]+(\.[0-9]+)?s$/)
            bad = 1
        split($field, parts, /[ms]/)
        cpu += 60 * parts[1] + parts[2]
    }
}
END {
    if (bad || NR != 2 || budget !~ /^[0-9]+$/) {
        print "FAIL budget: invalid suite timing or TEST_BUDGET_S"
        exit 1
    }
    printf "%s: %d tests, %d failures (%.2fs CPU; %ds wall; budget %ds CPU)\n", component, count, failed, cpu, wall, budget
    if (cpu > budget) {
        print "FAIL budget: speed the suite up or raise TEST_BUDGET_S with a reason in the component README"
        exit 1
    }
}
