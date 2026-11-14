# Completed assertion summaries, not a failing program name, prove semantics.
/^[ ]*FAIL[ :\[]/ { assertions = assertions $0 "\n"; seen++ }
/^INITECH_CHECKS [0-9]+ failures=[0-9]+$/ {
    checks=$2+0; split($3,a,"="); failures=a[2]+0; summaries++
}
# Retained host drivers predate the canonical API but explicitly finish with
# their assertion counters. Missing references/setup failures have no such end.
match($0, /[0-9]+ checks, [0-9]+ (failures|failed|failure\(s\))/) {
    split(substr($0,RSTART,RLENGTH),a," ")
    checks=a[1]+0; failures=a[3]+0; summaries++
}
END {
    if (summaries < 1 || checks < 1 || failures < 1 || seen < 1) exit 1
    printf "%s", assertions
}
