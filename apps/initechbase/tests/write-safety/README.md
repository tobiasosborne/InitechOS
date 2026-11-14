# Failed writes and real-software qualification

The operator's Phase 2 failed-save ruling is the preservation authority:
after a reported write failure the preceding file must still open under its
own name. It deliberately strengthens III+ where the reference loses data.
This is reported-error recovery, not power-loss/crash atomicity.

`capture.sh /absolute/fresh/output` uses the tooling in goldens/dataloss and
the same hash-pinned real DOS 3.30 and dBASE III PLUS 1.1 runtime as the
existing corpora. No reference binaries are retained here. SCENARIOS selects
individual cases. It fills a 1.44 MiB FAT12 floppy with mtools before running
real commands on it. Captured runs are in captured/ and SHA256SUMS pins them.
The complete disposable disks and raw monitor evidence from this session
remain under /tmp/ibase-write-capture5 and /tmp/ibase-write-capture6.

Observed in the retained captures:

- `full`: REPLACE fits the existing allocation; repeated APPEND reaches
  `Disk full when writing file: B:BASE.dbf`. This is a write-failure capture,
  not proof of real III+'s abort/reopen contract: both attempted A responses
  left the reference at its recovery prompt. We do not infer the missing result.
- `copy`: failed COPY FILE on full media leaves the previous TARGET.TXT at
  length zero (original was `PREVIOUS-FILE\r\n`).
- `save`: failed SAVE TO on full media leaves TARGET.MEM at length zero.
- `index`: building on full media reports insufficient index disk space and
  then a disk-full recovery prompt. It does not qualify failed REINDEX of an
  existing index. The preservation property, not an invented III+ contract,
  governs that case.
- `alias`: real self-copy completes, but TARGET.TXT is empty afterward.

The retained `running` VGA/PNG captures are sampled at the recovery prompt;
files/listings are extracted only after QEMU stops. Host dates in mdir are
observations, not product bytes. The SCRIPT captures include unsuccessful
abort/reopen attempts so they cannot be mistaken for a positive reopen proof.

The host `test_ibase_dbsafe writes` matrix injects negative and short writes
(zero/one/half/all-but-one), persistent write failure, and individual open,
seek, close and rename faults. Byte equality with independent real III+
inputs is stronger than merely accepting a DBF parser's exit status. It also
exhausts output writes/closes for COPY, SAVE, REPORT, LABEL, ALTERNATE and
index builds. Source-table navigation failures are not claimed by that matrix.
`append` separately qualifies 255-to-256 count publication with +1/+2 inputs.

Same-directory 8.3 stages are closed before replacing a destination; failed
serialization removes only the stage. Commit retains the predecessor until the
caller's final checks succeed. INDEX/REINDEX include reopen, header, attachment
and navigation checks; failure restores old bytes and index metadata. Under the
operator's review ruling, if installation and restoration both fail, the error
prints `Previous file retained as: <exact pathname>` and deletes nothing. The
old BAK, new stage and any installed replacement survive; the original name can
be absent. DOS 3.30 has no atomic replace through INT 21h; kernel-only
initech-7mb66 is not used. Cleanup failure after success can leave a BAK.
No crash guarantee is made.

`test_ibase_say` now executes the captured TA/TB/TD @...TO command prefixes,
saves actual target cell/attribute/cursor dumps, and compares all 4002 bytes
with the real III+ at-to-a captures. TC's traceback/recovery screen remains
outside the implemented recovery UI; no GET/READ or editor work is included.
The retained-screens and w4-files suites are comparator/provenance self-tests,
not target acceptance tests for the unimplemented editor forms.

The smoke's `reopen.sh` boots fresh real III+ and requires exact known records
from the target-written DBF, seeks with its NDX, and exact memo text from a
host-written DBF/DBT. It requires all three independent acceptance markers;
parse success alone is insufficient. The ordinary tier-0 suite uses no emulator.

The real-reader check tests memo output directly: real III+ rejects a memo in
an ordinary character comparison (#34), so the checker requires the exact
50-column `hello` display and no REOPEN-ERROR marker. A completed genuine run
with only a mutated target record is rejected by reopen-red.sh while intact
index and memo checks still pass. `PRESERVED-OK` separately qualifies the +2
TAX table exported by the injected record-region flush failure.

The committed genuine-reader leg also requires `C21-RECORDS-OK` and
`C21-INDEX-OK`: two rows, unchanged A/1, repaired B/9 and exact indexed seeks.
It runs in `make smoke`, not tier 0, because it boots actual III+ under QEMU.
`reopen-red.sh OUT PRODUCT CORE c21` changes only row 2's serialized VALUE
from 9 to 8 and requires a completed real run to reject it.
