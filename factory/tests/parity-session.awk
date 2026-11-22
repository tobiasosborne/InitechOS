# Replace each complete, independently rendered FAT block exactly once.
# Every other line is compared, including the two commands and their headers.
FILENAME == ARGV[1] { dir = dir $0 "\n"; next }
FILENAME == ARGV[2] { chkdsk = chkdsk $0 "\n"; next }
{
 sub(/\r$/, "")
 if ($0 == "SHELL-READY") ready = 1
 if (!ready) next
 if (index($0, drive ":\\") == 1 && index($0, ">")) $0 = "WORK" substr($0, 2)
 if (index($0, " Volume in drive " drive " is ") == 1) sub(/drive [AC] is/, "drive WORK is")
 if ($0 == " Directory of " drive ":\\") $0 = " Directory of WORK:\\"
 session = session $0 "\n"
}
function verify(block, name,    n, tail) {
 n = index(session, "\n" block)
 tail = substr(session, n + 1 + length(block))
 if (!length(block) || !n || index(tail, "\n" block)) {
  print "FAIL parity: expected one " name " block from FAT in " FILENAME > "/dev/stderr"
  exit 1
 }
 session = substr(session, 1, n) "[" name " verified against FAT]\n" tail
}
END {
 verify(dir, "DIR")
 verify(chkdsk, "CHKDSK")
 printf "%s", session
}
