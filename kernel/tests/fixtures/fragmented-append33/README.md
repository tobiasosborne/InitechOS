# FAT12 append capture

`capture.sh` boots a disposable copy of the hash-pinned Packard Bell
MS-DOS 3.30 disk. `append.asm` checks the DOS version, then opens, seeks
to byte 512, writes one byte and closes B:TARGET.BIN. mtools records
the cluster chains before and after. `chains.txt` holds the after
listings in the order tail, middle, blocked.

The last-cluster case starts at 2848. Both middle cases start at 4.
In the blocked case GUARD.BIN occupies cluster 5. All three writes
take cluster 2. Each extracted file is 512 zero bytes followed by
`X`, SHA-256
`17afe5921dc1beb923980b99c68a400f60146fff3cc23906987af03a5ee038a1`.

Re-capture with `sh capture.sh /absolute/path/to/new-directory` and compare
`tail.after`, `middle.after` and `blocked.after` in that order with
`chains.txt`. The host gate mints fresh disks, reads the resulting bytes
and chains using mtools, and checks allocation ownership and FAT mirrors
with the independent disk reader.
