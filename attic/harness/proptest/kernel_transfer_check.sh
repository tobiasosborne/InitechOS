#!/bin/sh
# Round-2 Sec 4 / loader_context_t offset assertions. Check the actual linked
# instructions, including LTO clones. This is a structural contract; the
# injected EXEC/MZ/nested/DOS traces remain the semantic proof.
set -eu
objdump -d --no-show-raw-insn "$1" > "$2"
awk '
BEGIN {
    frame="(-?0x[0-9a-f]+)?\\(%(esp|ebp)\\)";
    save[1]="mov[ \t]+%esp," frame "$"; save[2]="mov[ \t]+%ebp,";
    save[3]="mov[ \t]+%ebx,"; save[4]="lea[ \t]+.*,%eax$";
    save[5]="mov[ \t]+%eax,"; save[6]="mov[ \t]+%esi,";
    save[7]="mov[ \t]+%edi,"; save[8]="mov[ \t]+.*,%eax$";
    save[9]="mov[ \t]+" frame ",%esp$";
    save[10]="jmp[ \t]+\\*%eax$";
    restore[1]="mov[ \t]+0x8\\(%eax\\),%ebx$";
    restore[2]="mov[ \t]+0xc\\(%eax\\),%ebp$";
    restore[3]="mov[ \t]+0x10\\(%eax\\),%esi$";
    restore[4]="mov[ \t]+0x14\\(%eax\\),%edi$";
    restore[5]="mov[ \t]+\\(%eax\\),%esp$";
    restore[6]="jmp[ \t]+\\*0x4\\(%eax\\)$";
}
/^[0-9a-f]+ </ {
    exit_hook=($0 ~ /<loader_exit_hook>:/);
}
/^[ \t]+[0-9a-f]+:/ {
    # Scan the entire ELF, including unnamed/inlined copies outside the
    # loader_run_plan symbol. Exactly one complete transfer is supported.
    if (s || $0 ~ save[1]) {
        s++;
        if ($0 !~ save[s]) { print "!!! kernel-transfer FAIL: save step " s; bad=1; exit 1 }
        if (s==10) { saves++; s=0 }
    }
    if (exit_hook) {
        if (r || $0 ~ restore[1]) {
            r++;
            if ($0 !~ restore[r]) { print "!!! kernel-transfer FAIL: restore step " r; bad=1; exit 1 }
            if (r==6) { restores++; r=0 }
        }
    }
}
END {
    if (bad) exit 1;
    if (saves!=1 || restores!=1 || s || r) {
        print "!!! kernel-transfer FAIL: expected exactly one transfer and restore; got " saves+0 "/" restores+0; exit 1
    }
    print ">>> kernel-transfer: green (one transfer; all callee-saved GPRs; ESP/EBP-based stack load last; full restore)"
}' "$2"
