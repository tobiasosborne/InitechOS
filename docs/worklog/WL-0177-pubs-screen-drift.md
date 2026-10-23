# WL-0177 -- DOS publication totals after drivext

## Context and ground truth

Beads initech-64yp (seven Reference screens) and initech-ug8l (suspected
volume-label misclassification). Base: 75a49fe2, including drivext merge
c51ee50a. No spec, capture trace, comparator or tolerance is changed.
The following adjudication was recorded before replacing any publication
capture. Diagnostic captures go only to build/cx-screendrift.

Read the original MS-DOS 3.3 User's Guide and User's Reference, July 1987:
Reference p.43/PDF128 (CHKDSK categories), p.57/PDF142 (DIR, wildcard and
wide listings), p.88/PDF173 (MD), p.104/PDF189 (REN), p.163/PDF248 (EDLIN
preserves the original as .BAK). These pages were rendered and inspected.
Local reference: initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf.
The manual defines behavior, not immutable byte totals for our diskette.
FAT allocation is independently judged by mtools on the exact disk bytes
(PRD 6.1; ADR-0003 B.1). The native memory arena is ADR-0003 DEC-03/B.3;
the DOS manual's example memory figures are not an Initech flat-32 ABI.

## ug8l: preserve the volume-label count

IBM DOS Reference p.7-50 explicitly counts the label with the hidden files.
More decisively, a fresh genuine DOS 3.30 mint agrees: no label/no hidden
file omits the category; label only is 0 bytes/1 file; a hidden 9-byte file
alone is 512 bytes/1 file; both are 512 bytes/2 files. The label has no
data chain. `VER`, the original media and CHKDSK hashes, reproducible mint
script and fixed reports are documented in
harness/diff/fat_diff/chkdsk-label-golden/README.md. The gate compares with
those reports; it never mints expected values using InitechDOS.

Thus `0 bytes in 1 hidden files` is correct, stays unchanged, and explains
none of the drift. The artifact change adds only a source citation and a
LABEL mutant seam; normal CHKDSK.COM remains byte-identical at 8,529 bytes.

## Explain every changed number before refreshing

Recompiled c51ee50a's first parent with the current toolchain in an isolated
build directory. No prior binaries or captured totals were assumed correct.

| Utility | Before drivext | Merged build | Allocated clusters before -> after |
|---|---:|---:|---:|
| CHKDSK.COM | 8241 | 8529 | 17 -> 17 |
| ATTRIB.COM | 2368 | 2368 | 5 -> 5 |
| LABEL.COM | 1885 | 1937 | 4 -> 4 |
| XCOPY.COM | 6052 | 6312 | 12 -> 13 |
| TREE.COM | 6052 | 6312 | 12 -> 13 |
| EDLIN.COM | 8209 | 8209 | 17 -> 17 |

The practice diskette has 512-byte clusters. Only XCOPY and TREE cross an
allocation boundary: one extra cluster each, 1024 bytes together. This is
the practice data disk's APPS folder, not the separate tracer boot volume.
`mdu -a`, `mshowfat` and `mdir` independently confirm the merged allocation:
80 user clusters (4 root files, 69 utility clusters, 7 sample clusters),
2 directory clusters, 2847 total data clusters. Therefore user bytes are
40960, directory bytes 1024, and free bytes 1457664-40960-1024=1415680.
The label is a root-directory entry and allocates no cluster.

| Reference capture | Exact numeric changes | Judgment against DOS 3.3 |
|---|---|---|
| chkdsk | user 39936 -> 40960; available 1416704 -> 1415680; total memory and free memory each 118448 -> 118160 | p.43: report allocation and memory; utility allocation +1024, native image +288 |
| chkfile | same four changes as chkdsk | p.43: specifying a file adds its report, still reports the same whole disk |
| dir | free 1416704 -> 1415680 | p.57: listing leaves allocation intact; mdir agrees |
| dirargs | free 1416704 -> 1415680 in BOTH listings | p.57: wildcard and /W change listing selection, not whole-volume free space |
| edlin | free 1416192 -> 1415168 | p.163: original PHONE.TXT becomes PHONE.BAK, new 212-byte PHONE.TXT needs one additional 512-byte cluster; mdir verifies both files |
| md | free 1416192 -> 1415168 | p.88: new REPORTS directory owns one 512-byte cluster holding dot entries; mdir verifies it |
| ren | free 1416704 -> 1415680 | p.104: rename changes the name, keeps the data and allocation; mdir verifies it |

All seven changes are correct. The memory delta follows independently from
the linked image sizes: arena base round16(0x40100+image_bytes+0x10000)
moves from 0x52140 to 0x52260, a 288-byte increase; available memory is
0x6f000-base-16, hence 118160. The image's on-disk cluster count stays 17.
File counts (17 users, 2 directories, one hidden label), volume date,
1457664 total disk bytes, file lengths and timestamps do not change.

The User's Guide uses the same practice diskette. Its matching CHKDSK,
DIR, REN and CDSUB totals have the identical explanation. COPY adds one
cluster; PATHS makes a directory and copies a file (two clusters); BACKUP
makes a directory and copies three .TXT files (four clusters). Each free-space
number must therefore decrease by the same 1024-byte utility growth,
without any trace or command-semantics change.

| Additional User's Guide capture | Exact changes |
|---|---|
| chkdsk | same four changes as Reference chkdsk |
| dir, ren, cdsub | 1416704 -> 1415680 free |
| copy | 1416192 -> 1415168 free |
| paths | 1415680 -> 1414656 free |
| backup | 1414656 -> 1413632 free |

## Three other stale captures exposed by the aggregate

The first complete test-pubs-screens run stopped at InitechBase gs-dir,
before reaching the DOS books. These figures were adjudicated separately
before refreshing them, using the same DOS 3.3 DIR p.57 whole-volume rule.

* InitechBase Volume One gs-dir: 1310720 -> 1307648 free (-3072).
  Rebuilt its last capture revision 4f1221fe: SAMIR.COM was 95514 bytes,
  187 clusters; merged SAMIR.COM is 98810 bytes, 193 clusters. All sample
  disk files rebuilt at that revision compare byte-for-byte with today's
  files. Six extra executable clusters explain the full delta. `mdu`
  independently counts 293 user clusters today, with no subdirectories:
  1457664-293*512=1307648. The four listed DBFs and their sizes stay fixed.
* TPS tpsdir: 1162752 -> 1161216 free (-1536). Rebuilt the complete data
  volume at the last TPS capture revision 80a1d7ac. `mdu` finds exactly
  three added clusters today: DOS/XCOPY.COM 12->13, DOS/TREE.COM 12->13,
  and APPS/TPSREP.COM 15->16 (7348->7800 bytes, tpsrepfull). Every other
  file's allocation is unchanged. Desktop startup adds DESKTOP.DB and
  TRASH (one cluster each), as before. `mdir` on the actual journey image
  reports 1161216; the whole disk, not just the TPS* selection, is counted.
* TPS tpsbak: 1157632 -> 1156096 free, the same -1536. The two report files
  still consume ten clusters (5120 bytes) together; 1161216-5120=1156096.
  Their file names, report content and retained-backup behavior stay fixed.

## Acceptance

The first test-pubs aggregate passed, including all 82 live text screens,
9 graphical figures and 277 example replays. test-pubs-mutant then exposed
an obsolete one-fault fixture in harness/pubs/tps_mutants.sh: it renamed
TPS-008 to TPS-009, which tpsrepfull now legitimately ships. The inventory
parser rejected the duplicate ID as SPEC before the intended coverage
oracle ran. The fixture now uses the already-established synthetic TPS-099
ID, with a guard that it is unshipped. Its required DOC-NOT-SHIPPED failure
is unchanged. All seven TPS-manual mutants pass directly; the locked
catalogue, parser, comparator and diagnostic expectations are unchanged.

The later InitechWord mutant leg also had a stale application-site match:
it looked for File Not Found with the old M_GET_NAME return-mode argument.
The shared retrieve path now passes `back`. The guard correctly failed
with "the message mutant did not apply". The fixture now mutates only the
error-message literal, independently of that argument. All 14 InitechWord
mutants then passed directly, still demanding MESSAGE-UNPROBED and the
other original diagnostic tags. No WORD.EXE source or golden was changed.
The final mutant aggregate passed (exit 0), resumed with
`-o test-pubs -o test-pubs-build` to reuse the unchanged baseline already
verified twice; all mutant cases were rerun, including the 14 InitechWord
cases. Exact results and commands are recorded in REPORT.md.

The initial byte comparator rejects all seven named captures. In
particular chkdsk.txt byte 163 is ASCII 51 -> 52, exactly the reported
3 -> 4 change (39936 -> 40960), not a hidden-file count change.
The label-count mutant drops the label increment and is rejected by both
the focused genuine-DOS host comparison and the existing emulator report
oracle. The normal binary is unchanged. Full gate results and handoff are
recorded in the root REPORT.md.
