# Count whole-name occurrences of marker m in a serial transcript, by the
# rule qemu.c's W waits use: a marker edge that is a name character
# (letter, digit, '_', '-', '.') may not continue into another one, so
# app=CALC is not counted in app=CALCULAT. The marker comes from ENVIRON["MARKER"]
# (awk -v would rewrite its backslashes).
BEGIN { m=ENVIRON["MARKER"] }
function name_char(c) { return c ~ /^[A-Za-z0-9_.-]$/ }
{
    line=$0; sub(/\r$/, "", line); rest=line; base=0; len=length(m)
    while((at=index(rest, m)) > 0) {
        start=base+at; before=(start>1) ? substr(line, start-1, 1) : ""
        after=substr(line, start+len, 1)
        if(!(name_char(substr(m,1,1)) && before!="" && name_char(before)) &&
           !(name_char(substr(m,len,1)) && after!="" && name_char(after))) n++
        base=start+len-1; rest=substr(line, base+1)
        if(len==0) break
    }
}
END { print n+0 }
