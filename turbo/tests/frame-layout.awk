# 32-bit cdecl frame property (Intel 80386 CALL/PUSH/LEAVE semantics and
# the published runtime ABI): [ebp] is the saved link, [ebp+4] is return PC.
# Generated Pascal local/result storage may never alias either header word.
/\[ebp(\+0|\+4)?\]/ {
    print "generated storage aliases cdecl frame header: " $0
    bad=1
}
END { exit bad ? 1 : 0 }
