# GNU make emits recursive children's databases first. Audit only this parent.
/^# Make data base,/ {
    delete lines; n=0; active=1; files=finished=all=test=0; directory=""
}
active {
    lines[++n]=$0
    if ($0 ~ /^CURDIR := /) directory=substr($0,11)
    if ($0=="# Files") files=1
    if ($0 ~ /^all:/) all=1
    if ($0 ~ /^test:/) test=1
    if ($0 ~ /^# Finished Make data base/) finished=1
}
END {
    if (!active || !files || !finished || !all || !test || directory!=expected) {
        print "FAIL dependency: incomplete/wrong make database for " expected > "/dev/stderr"
        exit 1
    }
    for(i=1;i<=n;i++) print lines[i]
}
