# Refuted findings (PM, 2026-10-23)

- L002 "F2 followed by Return truncates a full-length label" -- NOT A BUG.
  Real 1-2-3 R2.2 does the same: the EDIT line counts the label prefix
  inside its 240 characters, so F2 on a 240-character label shows prefix +
  239 and Enter stores 239. Golden: fixtures/i123-keys/T240.WK1 (minted
  from the real R2.2, bead initech-69mz): LABEL record length 246 = 5
  header bytes + prefix + 239 characters + NUL. Locked by test-i123-keys
  K22. The audit's byte-equality expectation cites manual pp. 1-16/1-17/
  1-22, which do not cover this case. Bead initech-pmw9 closed by-design.

- Triage B03 (initech-3rqz, "format supported high decimal counts without a
  scaled-integer ceiling") -- NOT A BUG. Lane cx-i123num minted real Lotus
  R2.2 goldens (fixtures/i123-numeric/, README there): Fixed/Currency/comma
  with 13-14 decimals fit, 15 shows stars; Percent 11-12 fit, 13-15 stars;
  @STRING(1234.5678912345,15) is ERR in Lotus itself. Initech 123 matches
  the real product at those bounds. Bead closed by-design.
