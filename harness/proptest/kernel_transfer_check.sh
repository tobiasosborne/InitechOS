#!/bin/sh
# Round-2 Sec 4 / loader_context_t offset assertions. Check the actual linked
# instructions, including LTO clones. This is a structural contract; the
# injected EXEC/MZ/nested/DOS traces remain the semantic proof.
set -eu
objdump -d --no-show-raw-insn "$1" > "$2"
awk '
BEGIN {
    save[1]="mov[ \t]+%esp,"; save[2]="mov[ \t]+%ebp,";
    save[3]="mov[ \t]+%ebx,"; save[4]="lea[ \t]+.*,%eax$";
    save[5]="mov[ \t]+%eax,"; save[6]="mov[ \t]+%esi,";
    save[7]="mov[ \t]+%edi,"; save[8]="mov[ \t]+.*,%eax$";
    save[9]="mov[ \t]+.*,%ebx$"; save[10]="mov[ \t]+.*,%esp$";
    save[11]="jmp[ \t]+\\*%eax$";
    restore[1]="mov[ \t]+0x8\\(%eax\\),%ebx$";
    restore[2]="mov[ \t]+0xc\\(%eax\\),%ebp$";
    restore[3]="mov[ \t]+0x10\\(%eax\\),%esi$";
    restore[4]="mov[ \t]+0x14\\(%eax\\),%edi$";
    restore[5]="mov[ \t]+\\(%eax\\),%esp$";
    restore[6]="jmp[ \t]+\\*0x4\\(%eax\\)$";
}
/^[0-9a-f]+ </ {
    entry=($0 ~ /<loader_run_plan[^>]*>:/);
    exit_hook=($0 ~ /<loader_exit_hook>:/);
}
{
    if (entry) {
        if (s || $0 ~ save[1]) {
            s++;
            if ($0 !~ save[s]) { print "!!! kernel-transfer FAIL: save step " s; bad=1; exit 1 }
            if (s==11) { saves++; s=0 }
        }
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
    if (!saves || !restores || s || r) {
        print "!!! kernel-transfer FAIL: incomplete context/stack sequence"; exit 1
    }
    print ">>> kernel-transfer: green (all callee-saved GPRs; ESP switched last; register entry jump; full restore)"
}' "$2"
