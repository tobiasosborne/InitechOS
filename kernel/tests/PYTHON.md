# Inherited Python dependencies pending operator decision

The import neither ports nor deletes an independent reader. Two make variables
isolate the interpreters: PYTHON_FAT_REFERENCE (FAT reader and inherited inline
byte checks/generators) and PYTHON_MESSAGE_REFERENCE (JSON/catalogue check).
All required tools fail loudly when absent. Python is not linked into a product.

| File | Purpose / independent oracle | C port effort |
|---|---|---|
| kernel/tests/disk/fat12_ref.py | Independent FAT12 reader, chains/directories/content; compares with C and mtools | Medium: bounded sector reader, FAT12 decoding and path traversal, preserve malformed-chain handling |
| kernel/tests/disk/fat16_ref.py | Separate FAT16 reader, including high cluster numbers | Medium: BPB/FAT16/path reader; must remain independent of kernel headers/code |
| kernel/tests/fat12_fuzz.c | Launches PYTHON_FAT_REFERENCE to compare each live file with its independent model | Small adapter change once the separate reader exists |
| kernel/tests/gates/test-dosmsg.sh | Inherited Python JSON/header consistency check over all 63 catalogue rows | Small/medium: independent JSON parser plus C escaping; do not reuse gen_messages.c |
| kernel/tests/disk/fat_spc.sh | FAT12/FAT16 content and grown-directory differentials at six cluster sizes | Small inline checks; reader work as above |
| kernel/tests/gates/test-b53d.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-d27i.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat-fuzz.sh | Tool preflight and model/mtools/reference-reader sweep | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat-move.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat-partial.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat-subdir.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat-write-partial.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat-write.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-fat16.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-m0bp.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-nmpo.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-qekc.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |
| kernel/tests/gates/test-zs24.sh | Inherited inline byte generator/independent byte or directory comparison; some also invoke fat12_ref.py/fat16_ref.py | Small inline checks; reader work as above |

The pure-C host tests also mention Python in comments describing their external
judges. Those mentions do not add runtime dependencies. The claimed mint scripts
under fixtures/dos33_overlay and the real-DOS capture sources are provenance;
tier 0 never runs them. No Python dependency was hidden behind a successful skip.
