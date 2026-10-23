# Refuted findings (PM, 2026-10-23)

- L002 "F2 followed by Return truncates a full-length label" -- NOT A BUG.
  Real 1-2-3 R2.2 does the same: the EDIT line counts the label prefix
  inside its 240 characters, so F2 on a 240-character label shows prefix +
  239 and Enter stores 239. Golden: fixtures/i123-keys/T240.WK1 (minted
  from the real R2.2, bead initech-69mz): LABEL record length 246 = 5
  header bytes + prefix + 239 characters + NUL. Locked by test-i123-keys
  K22. The audit's byte-equality expectation cites manual pp. 1-16/1-17/
  1-22, which do not cover this case. Bead initech-pmw9 closed by-design.
