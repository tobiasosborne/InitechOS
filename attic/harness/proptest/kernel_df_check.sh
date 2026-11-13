#!/bin/sh
# Intel SDM Vol. 1 Sec. 3.4.3.2 (DF flag): C entry needs forward strings.
# Static fallback: inspect the real linked code, not a recompiled reference.
set -eu
objdump -d --no-show-raw-insn "$1" > "$2"
awk '
BEGIN {
    n=split("isr_common int13_entry int20_entry int21_entry int22_entry int23_entry int24_entry int25_entry int26_entry int27_entry tsr_screen_entry irq0_entry irq1_entry irq12_entry isr_selftest tbx_gate_entry", names)
    for (i=1; i<=n; i++) required[names[i]]=1
}
/^[0-9a-f]+ </ {
    fn=$0; sub(/^.*</,"",fn); sub(/>:.*/,"",fn)
    active=(fn in required)
    if (active) seen[fn]++
    loader=(fn ~ /^loader_run_plan/)
}
/^[ \t]+[0-9a-f]+:/ {
    addr=$1; sub(/:$/,"",addr); sub(/^0+/,"",addr)
    instruction[addr]=$2
    if (active && $2=="cld") cleared[fn]=1
    if (active && $2 ~ /^call/ && !cleared[fn]) {
        print "!!! kernel-df FAIL: " fn " calls C before cld"; bad=1
    }
    # The absolute LEA in the transfer saves the non-local resume label.
    if (loader && $2=="lea" && $3 ~ /^0x[0-9a-f]+,%eax$/) {
        resume=$3; sub(/^0x/,"",resume); sub(/,%eax$/,"",resume)
        resumes++
    }
}
END {
    for (i=1; i<=n; i++) {
        fn=names[i]
        if (seen[fn]!=1 || !cleared[fn]) {
            print "!!! kernel-df FAIL: missing/duplicate stub or cld: " fn; bad=1
        }
    }
    if (resumes!=1 || instruction[resume]!="cld") {
        print "!!! kernel-df FAIL: loader resume must start with cld"; bad=1
    }
    if (bad) exit 1
    print ">>> kernel-df: green (16 C entry stubs clear DF before calls; loader resume starts with cld)"
}' "$2"
