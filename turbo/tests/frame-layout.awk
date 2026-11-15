# 32-bit cdecl frame property (Intel 80386 CALL/PUSH/LEAVE semantics and
# the runtime ABI): [ebp] is saved EBP; [ebp+4] is the return PC. Generated
# direct scalar/result stores must not overwrite either header word.
# Reads and address formation are not graded by this bounded property.
/^[ \t]*(mov|add|sub|and|or|xor|inc|dec|pop)[ \t]+((byte|word|dword)[ \t]+)?\[ebp(\+0|\+4)?\]([ \t]*,|[ \t]*$)/ {
    print "generated store overwrites cdecl frame header: " $0
    bad=1
}
END { exit bad ? 1 : 0 }
