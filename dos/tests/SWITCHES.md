# InitechDOS utility switches

The DOS 3.30 utility audit captures the period behavior. The checks below exercise the shipped parsers and observe their effects.

| Utility | Supported switches and observed effect | Check |
|---|---|---|
| CHKDSK | `/F` repairs FAT12 and refuses FAT16. `/V` lists paths. A bad switch letter prints `Invalid parameter` and the check continues. | `test-chkdsk`, `test-fat16-utilities` |
| FORMAT | `/S` transfers the bootable system set, `/B` reserves its first two entries, and `/V` sets an interactive label. `/1`, `/4`, `/8`, `/T:n`, and `/N:n` change the geometry. Conflicting dimensions are refused before a prompt or write. | `test-formatsys`, `smoke-system-transfer` |
| SYS | Takes a drive and refuses switches. | `test-utility-errors-compat33`, `smoke-system-transfer` |
| FDISK | Refuses a nonblank command tail with `Invalid parameter` before disk access. | `fdisk-running-disk` |
| EDLIN | `/B` keeps embedded Ctrl-Z as data when reading and saving a file. | `test-edlin-host` |
| XCOPY | `/A` selects archived files and `/M` also clears archive. `/D:date` filters by date, `/S` recurses and `/E` includes empty directories with `/S`. `/P` prompts, `/V` reads back the write, and `/W` waits for input. | `test-xcopy-tree`, `smoke-xcopy-buffer` |
| TREE | `/F` includes file rows. Directory operands are refused with `Invalid parameter`. A bare drive is accepted. | `test-xcopy-tree` |
| ATTRIB | `+R`, `-R`, `+A` and `-A` change the indicated attribute. `/S` descends. | `test-attrib` |
| LABEL | Takes no switches. A slash in an attached label is rejected. | `test-label-vol` |
| FIND | `/V` inverts matches, `/C` counts, and `/N` numbers. `/I` and other unrecognized letters report `FIND: Invalid Parameter` and the letter. Matching continues case-sensitively. | `smoke-find-args` |
| SORT | `/R` reverses order and `/+n` chooses a comparison column. An unsupported switch is refused with `Invalid parameter`. | `smoke-sort-columns` |
| MORE | Refuses a nonblank command tail with `Invalid parameter`. | `smoke-more-page` |
| DISKCOPY | `/1` writes only the first side. | `test-formatsys` |
| DISKCOMP | `/1` compares only the first side and `/8` compares the first eight sectors per track. | `test-formatsys` |
