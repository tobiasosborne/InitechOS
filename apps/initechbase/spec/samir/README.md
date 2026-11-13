# spec/samir/ -- Provenance Ledger

LOCKED spec-data for SAMIR (InitechBase). Authored on S0.5 lock (bead initech-586.5.4).
Per CLAUDE.md Rule 8: changing these files is a deliberate act requiring an issue +
worklog note. Do NOT silently edit to make a test pass.

---

## File provenance

| File | Exact corpus source | Notes |
|---|---|---|
| `dbf_format.h` | `../dbase3-decomp/specs/file-formats/dbf.md` sections 2, 4, 6, 8 | Byte-verified against 6 III+ 1.1 golden fixtures (BANK, CLIENTS, TOURS, TRAVEL, TAX, UNIVERSD). Every offset, size, and note cites the section. |
| `ndx_format.h` | `../dbase3-decomp/specs/file-formats/ndx.md` sections 1, 2, 3, 3.1, 4 | Byte-verified against 11 III+ 1.1 golden .ndx files. Minted NCOST.NDX / NUNIQ.NDX / NMULTI.NDX used for Open-question resolutions (re/mint-results-001.md). |
| `xbase_coercion.json` | `../dbase3-decomp/specs/language/coercion-table.md` section 6 | Verbatim basis from the III+-only corpus table. `not_in_iii_plus` explicitly lists `"=="`. No `==` operator row anywhere in `operator_coercion`. Period-grounded on HELP.DBS.strings.txt + DBASE.MSG.strings.txt. |
| `dbf_normalization.json` | `../dbase3-decomp/specs/file-formats/dbf.md` section 2 (header table) and section 4 (field-descriptor table) | III+-only classification. Header byte `0x1C` (MDX flag) and per-descriptor byte `0x1F` (index-field flag) are both NORMALIZE for III+ (0x00 in 100% of III+ fixtures). |
| `dbase_msg_codes.tsv` | `../dbase3-decomp/archive/golden-mined/DBASE_MSG_codes.tsv` | Mined from DBASE.MSG binary (1-based ordinal, 151 codes). Codes 32, 40, 69, 135, 151 have empty messages in the source; preserved as empty here. Line 150 had binary garbage after NUL terminator; truncated at NUL. |

---

## Superseded source

`docs/research/dbase-ground-truth.md` is **SUPERSEDED for III+ codec and coercion
provenance** (per ADR-0008 DEC-06, ratified 2026-06-17).

That research brief straddles III+ and dBASE IV in several places:

- Its coercion table draft (section 5.6) contained a `==` operator row and commented
  "DRAFT pending dBASE IV". The III+-only corpus table (`coercion-table.md` section 6)
  has NO `==` row and explicitly lists `==` in `not_in_iii_plus`. The corpus table is
  the authoritative source for `xbase_coercion.json`.

- Its normalization table marked header byte `0x1C` and per-descriptor byte `0x1F` as
  "MEANINGFUL iff .MDX" -- the IV meaning. For III+, both are 0x00 in 100% of III+ 1.1
  fixtures and carry no III+ meaning. The III+-only corpus table (`dbf.md` section 2/4)
  correctly classifies them as reserved/0x00. These are NORMALIZE for III+.

For any future III+ codec or coercion work, use the corpus files above, not the brief.
The brief remains useful as background context for the human engineer but is NOT a
primary source for locked spec-data.

---

## GATED items (not locked)

Per ADR-0008 DEC-06, the `numfn-1..8` cells (MOD/INT/ROUND/LOG/SQRT edge cases) in
`xbase_coercion.json` are GATED pending MINT against the real DBASE.EXE. They are
loud-skipped in the oracle, not locked. The rest of the coercion table is minted-confirmed.

---

*Provenance locked 2026-06-17. Governing bead: initech-586.5.4 (S0.5).*
