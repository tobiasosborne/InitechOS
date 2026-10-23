# dBASE III PLUS 1.1 `.ndx` single-index B-tree file format

**Status / confidence.** Header layout, node layout, grouplen/keys-per-page
formulas, char/numeric/date key encodings, the 4-byte alignment/filler, and the
B-tree high-key ordering invariant are all **byte-verified** against 11 real
III PLUS 1.1 golden `.ndx` files paired to their `.dbf` files (see Sources).
The Bachmann/piclist `index_header` struct is the correct on-disk layout; the
clicketyclick ASCII diagram has the right *fields* but a *wrong* sub-layout of
bytes 18-23 (it makes "size of key record" a 4-byte long and `unique` a single
byte at 23) -- the real bytes contradict it; see Open questions. The format is
single-file `.ndx` only (one index per file); dBASE IV `.mdx` and Clipper `.ntx`
are different and appear here only as labeled cross-references.

TARGET is dBASE III PLUS 1.1 (1986). All on-disk facts below are from III+
fixtures. `.mdx`/`.ntx`/`.idx` notes are lineage/contrast only and are tagged.

---

## 1. File geometry

* The file is a sequence of **512-byte pages** ("blocks"/"nodes").
  [verified: piclist `BLOCK_SIZE 512` + clicketyclick + all fixture sizes are
  multiples of 512: 512/1024/2048/3072/3584].
* **Page 0** = the header ("anchor node"). All other pages are B-tree nodes
  (root / branch / leaf). A page number `P` is at byte offset `P * 512`.
  [verified: clicketyclick note 1 + byte-checked: CNAMES.NDX header says
  `root=6`, and offset `6*512=3072` is a valid branch node].
* It is a **paged B-tree** (often called B+ tree). Every node is exactly one
  page. Total pages = header field at offset 4. [verified: piclist +
  clicketyclick + fixtures: header `total` always equals filesize/512].
* dBASE does **not** zero new pages before use; unused bytes (page tails, the
  trailing key slot, the header `dummy` field, key-entry filler) routinely
  contain leftover garbage. A byte-exact *writer* need not reproduce specific
  garbage, but a *reader* MUST ignore it. [verified: piclist para 6 + byte-
  checked: `dummy` at off 20 = 0x0010/0x0008/0x000F in AVAL_FLT/CUSTOMER/
  FLT_NO; trailing slots hold stale keys, e.g. CUSTOMER.NDX e5 = "ATLKKK"
  recno=6 when only 5 records exist].

---

## 2. Header (page 0, 512 bytes)

All integers little-endian. Validated against **CNAMES.NDX** (and cross-checked
against all 11 fixtures).

| off (hex) | size | field        | type   | notes |
|-----------|------|--------------|--------|-------|
| 0x00      | 4    | root_page    | uint32 | page number of the current root node. Updated on reindex. CNAMES=6. |
| 0x04      | 4    | total_pages  | uint32 | total page count incl. header (= next-page-to-allocate / EOF page). CNAMES=7. Max 0x7FFFFF. |
| 0x08      | 4    | reserved     | uint32 | always 0x00000000 in III+ fixtures. piclist calls it `version` ("uncertain"). Treat as reserved=0. |
| 0x0C      | 2    | key_length   | uint16 | length in bytes of the key data within each entry. CNAMES=40. |
| 0x0E      | 2    | keys_per_page| uint16 | MAX keys per node = floor((512-4)/group_length). CNAMES=10. (A node's live count may be lower.) |
| 0x10      | 2    | key_type     | uint16 | 0 = character; 1 = numeric OR date (both stored as 8-byte double). CNAMES=0, CHKNO=1, TOURDATE=1. |
| 0x12      | 2    | group_length | uint16 | bytes per key entry = ceil4(key_length + 8). CNAMES=48. |
| 0x14      | 2    | dummy        | uint16 | unused/uninitialized; holds garbage. AVAL_FLT=0x0010, CUSTOMER=0x0008, FLT_NO=0x000F, others 0. |
| 0x16      | 2    | unique_flag  | uint16 | 0 = not unique, 1 = unique index. All fixtures = 0. |
| 0x18      | 100  | key_expr     | char[] | NUL-terminated dBASE index expression, stored AS TYPED (NOT lowercased -- see below). Span 0x18..0x7B. |
| 0x7C      | 388  | (unused tail)| --     | to end of 512-byte page; garbage/zeros. |

`ceil4(n) = ((n + 3) / 4) * 4`. **Verified across all 11 fixtures**: every file
has `group_length == ceil4(key_length + 8)` and
`keys_per_page == (512 - 4) / group_length` (integer division). Examples:
key_length 5 -> group 16, kpp 31 (ZIPCODE); 14 -> 24, 21 (AVAL_FLT); 40 -> 48,
10 (CNAMES); 3 -> 12, 42 (FLT_NO). [verified: byte-check of all fixtures +
piclist note 3].

### 2.1 Key-expression byte span / casing (RESOLVED from bytes)

* **Span:** the expression field starts at **offset 0x18 (24)** and the
  documented capacity is **100 bytes** (0x18..0x7B). [verified: piclist
  `char expression[100]` + byte-check: in CNAMES.NDX bytes 0x00..0x17 are the
  10 header fields above and the ASCII "LASTNAME + FIRSTNAME " begins exactly at
  0x18]. The contradictory clicketyclick numbers ("24..511") and the Bachmann
  prose "up to 100 characters ... stored in lowercase" are partly right (start
  24, cap 100) and partly wrong (casing -- see next).
* **Termination:** a single `0x00` terminates the string; bytes after the NUL up
  to 0x7B are garbage. [verified: byte-check; e.g. CHKNO.NDX has NUL at expr
  offset 6 followed by binary garbage].
* **Casing -- piclist is WRONG for III+:** the expression is stored **verbatim
  as the user typed `INDEX ON ...`**, preserving case, NOT lowercased.
  [verified: byte-check]. Concrete counter-examples to "stored in lowercase":
  `CNAMES.NDX` = `"LASTNAME + FIRSTNAME "` (uppercase field names);
  `CHKNO.NDX` = `"Chkno "` (mixed case; the DBF field is `CHKNO`);
  `TOURDATE.NDX` = `"DEPARTURE "`; `CUSTOMER.NDX` = `"CDES_CITY+CNAME "`.
  Lowercase examples also exist (`FLT_NO.NDX` = `"aflt_no "`,
  `AVAL_FLT.NDX` = `"adep_city+ades_city+dtoc(adate) "`), confirming the bytes
  simply echo the source text. The expression's printed length is unrelated to
  `key_length` (e.g. CNAMES expr is 21 chars but key_length is 40).

### 2.2 Header validated bytes (CNAMES.NDX, offsets 0x00..0x2F)

```
0x00: 06 00 00 00   root_page    = 6
0x04: 07 00 00 00   total_pages  = 7
0x08: 00 00 00 00   reserved     = 0
0x0C: 28 00         key_length   = 40
0x0E: 0A 00         keys_per_page= 10
0x10: 00 00         key_type     = 0 (char)
0x12: 30 00         group_length = 48
0x14: 00 00         dummy        = 0
0x16: 00 00         unique_flag  = 0
0x18: 4C 41 53 54 4E 41 4D 45 20 2B 20 46 49 52 53 54   "LASTNAME + FIRST"
0x28: 4E 41 4D 45 20 00 ...                              "NAME " 0x00 ...
```

---

## 3. B-tree node (any page 1..total_pages-1)

Each node is a 512-byte page: a 4-byte node header followed by an array of
fixed-size key entries (group_length bytes each). Validated against
**CUSTOMER.NDX** (single-page tree) and **CNAMES.NDX** (multi-page tree).

| off (hex) | size | field      | type   | notes |
|-----------|------|------------|--------|-------|
| 0x00      | 2    | entry_count| uint16 | number of LIVE key entries in this node ("nkeys"). 0..keys_per_page. |
| 0x02      | 2    | filler     | uint16 | pad to 4-byte boundary; uninitialized garbage. CUSTOMER=0x5543, CNAMES=0. |
| 0x04      | g*N  | entries    | group[]| N = entry_count live entries, EACH group_length bytes (see 3.1). |

* `entry_count` is a **2-byte** count at offset 0, with **2 bytes of filler** at
  offset 2 (NOT a 4-byte count). [verified: piclist "count (two bytes) ... and
  two bytes of padding" + byte-check: CUSTOMER entry_count=5, filler=0x5543
  garbage; CNAMES root entry_count=4, filler=0]. This 4-byte node header is why
  `keys_per_page = (512 - 4) / group_length`.

### 3.1 Key entry ("group") layout

Each group is `group_length = ceil4(key_length + 8)` bytes:

| off (hex) | size | field       | type   | notes |
|-----------|------|-------------|--------|-------|
| 0x00      | 4    | child_page  | uint32 | lower-level page number. **0 => this entry is in a LEAF** (no child). |
| 0x04      | 4    | dbf_recno   | uint32 | DBF record number (1-based). Meaningful in LEAF entries; 0 (ignored) in branch entries. |
| 0x08      | KL   | key_data    | bytes  | the key (KL = key_length); see section 4. |
| 0x08+KL   | pad  | filler      | bytes  | 0..3 bytes to reach group_length (4-byte align). Garbage or 0. |

[verified: piclist `key_group` struct + byte-check across CUSTOMER (leaf),
CNAMES (branch+leaf), TOURDATE (numeric)].

* Filler width = `group_length - 8 - key_length`. Examples: AVAL_FLT KL=14 ->
  group 24 -> 2 filler bytes (observed `0x0000`); CUSTOMER KL=28 -> group 36 ->
  0 filler; CHKNO KL=8 -> group 16 -> 0 filler. The worst case is KL = 1 over a
  multiple of 4 (e.g. 29) -> 3 filler bytes. [verified: piclist para 7 + byte-
  check of AVAL_FLT filler bytes].

### 3.2 The trailing (N+1th) child pointer

A node physically has room for one entry beyond `entry_count`: group index
`entry_count` (offset `4 + entry_count*group_length`).

* In a **branch** node this slot's first 4 bytes are the **rightmost child
  pointer** (Bachmann's `last_block_num`): the subtree for keys greater than the
  last separator key. Its `dbf_recno`/`key_data` are meaningless garbage.
  [verified: byte-check -- CNAMES root entry_count=4 with child pointers
  1,2,3,4 in entries 0..3 and child=5 in the trailing entry 4; ZIPCODE root
  entry_count=1 with children 1 (entry0) and 2 (trailing entry1)].
* In a **leaf** node this trailing slot is unused garbage (often a stale copy of
  a key that used to live there). [verified: byte-check -- CUSTOMER leaf
  trailing entry = "ATLKKK" recno=6 (no rec 6 exists); CNAMES leaf page 5
  trailing entry = stale "Plimpton Daniel" recno=12].
* **Overflow caveat:** when a leaf is FULL (`entry_count == keys_per_page`) the
  trailing group does not fully fit in 512 bytes; only its leading bytes exist.
  Its `child_page` still reads 0. Example: CNAMES leaf page 1 has
  entry_count=10, groups occupy `4 + 10*48 = 484` bytes, leaving only 28 of the
  48 needed for the trailing group; bytes 484..492 are 0. [verified: byte-check
  CNAMES page 1]. Readers MUST NOT read a trailing branch child pointer past
  offset 512; branch nodes always keep `entry_count` small enough that all
  `entry_count + 1` child pointers fit (the tree balances to ensure this)
  [inferred from the overflow geometry + observed branch fill levels].

---

## 4. Key data encoding

`key_type` (header offset 0x10) selects the encoding of `key_data`:

### 4.1 Character keys (`key_type == 0`)

* `key_data` is `key_length` bytes of **space-padded ASCII** (OEM/CP437), left-
  justified, padded on the right with 0x20. [verified: byte-check CUSTOMER,
  CNAMES, ZIPCODE, AVAL_FLT, TRIPS].
* For a **multi-field / expression** key the value is the runtime
  **concatenation** of the expression's parts, each rendered to its fixed width,
  then the whole truncated/padded to `key_length`. Examples:
  - CNAMES `LASTNAME + FIRSTNAME`: LASTNAME(20 chars, space-padded) + FIRSTNAME
    (20) = 40 = key_length. e.g. rec15 -> `"Adams"+15sp + "Nathan"+14sp`.
    [verified: byte-check leaf key == CLIENTS.DBF rec15 fields].
  - CUSTOMER `CDES_CITY+CNAME`: CDES_CITY(3) + CNAME(25) = 28. rec1 ->
    `"ATL"+"LOUIS"+20sp`. [verified: byte-check vs CUSTOMER.DBF].
  - AVAL_FLT `adep_city+ades_city+dtoc(adate)`: 3+3+8 = 14. A **date rendered as
    a char key** uses `DTOC()` -> `"MM/DD/YY"` (default SET DATE AMERICAN), so
    rec2 -> `"ATL"+"BOS"+"12/03/85"`. [verified: byte-check]. (Contrast 4.2:
    indexing a bare D field gives a numeric key, not a char key.)

### 4.2 Numeric and Date keys (`key_type == 1`)

* `key_data` is an **8-byte IEEE-754 double, little-endian** (`key_length == 8`,
  `group_length == 16`). [verified: piclist "dBASE ... stores dates as a
  floating point double (8-byte) value and numbers in Intel (little-endian)
  format" + byte-check TOURDATE].
* **Numeric (N field / numeric expression):** the field's numeric value as a
  **raw** IEEE-754 double -- **no scaling, no sign-flip, no order-preserving bit
  transform**. Confirmed byte-for-byte against a minted index with negatives and
  decimals (`mint/work/NCOST.NDX`, `INDEX ON UNITCOST`): `-123.45` ->
  `cd cc cc cc cc dc 5e c0`, `-1.0` -> `00 00 00 00 00 00 f0 bf`, `0.0` ->
  `00 00 00 00 00 00 00 00`, `279.0` -> `00 00 00 00 00 70 71 40`, stored in true
  numeric order. [verified: minted NCOST.NDX 2026-06-16; see re/mint-results-001.md]
  (Historic note: the original CHKNO.NDX fixture was empty (CHECKS.DBF nrec=0), so
  this was minted fresh to confirm the negative/decimal path.)
* **Date (D field):** stored as the **chronological Julian Day Number** of the
  date, as a double. [verified: byte-check TOURDATE.NDX vs TOURS.DBF].
  Worked example (TOURDATE rec1, DEPARTURE = `19850805`):
  ```
  JDN(1985-08-05) = 2446283
  double(2446283.0) little-endian = 00 00 00 80 E5 A9 42 41
  TOURDATE.NDX leaf entry 0 key_data = 00 00 00 80 E5 A9 42 41   (match)
  ```
  Additional confirmations: `19850907` -> JDN 2446316 -> `00 00 00 00 F6 A9 42 41`;
  `19850923` -> 2446332 -> `00 00 00 00 FE A9 42 41`. The standard Gregorian-to-
  JDN formula reproduces the bytes exactly. Equal dates produce identical key
  bytes (ties broken by tree position, see section 5).
* **Key comparison is ARITHMETIC, not byte-wise**, for `key_type == 1`. Raw
  little-endian IEEE-754 doubles do NOT byte-sort correctly across the sign
  boundary (negatives have the high bit set), yet the minted NCOST.NDX leaf is in
  true numeric order (`-123.45 < -1 < 0 < 279`). Therefore a reader/writer MUST
  decode the 8 bytes to a `double` and compare numerically -- do not memcmp numeric
  keys. (For `key_type == 0` character keys, comparison IS byte-wise, section 4.1.)
  [verified: minted NCOST.NDX 2026-06-16]

---

## 5. B-tree ordering invariant and search

* **Ordering:** keys are stored in **ascending order** by the collation in
  section 6, both within a node and across the whole tree (in-order traversal
  yields a fully sorted sequence). [verified: byte-check -- CNAMES leaves run
  Adams < Anderson < ... < Zambini across pages 1..5; ZIPCODE 72450 < ... <
  97401 across pages 1..2; TOURDATE JDNs monotonic non-decreasing].
* **Branch separator = HIGH key of subtree.** In a branch node, the `key_data`
  of entry `i` (for `i < entry_count`) is the **largest key in the subtree
  rooted at `child_page[i]`**. The rightmost child (trailing slot, section 3.2)
  holds keys greater than the last separator. [verified: byte-check -- CNAMES
  root entry0 key "Collins Sara" == last key of child page 1; ZIPCODE root
  entry0 key "91306" == last key of child page 1, and page 2 (rightmost child)
  starts at "91316" > "91306"].
* **Search** (find key K): start at `root_page`. In each node, scan entries in
  order until you find an entry whose key >= K. If that node is internal
  (`child_page != 0`), descend into that entry's `child_page` (or the rightmost
  child if K exceeds all separators) and repeat. When you reach a leaf
  (`child_page == 0`), an exact key match returns `dbf_recno`. [documented:
  clicketyclick "Search algorithm" + consistent with the byte-verified
  high-key layout].
* **Duplicates** (when `unique_flag == 0`): multiple entries may share a key
  value; each carries its own `dbf_recno`. [verified: byte-check -- TOURDATE
  recnos 9 and 10 both have JDN 2446363; AVAL_FLT recnos 12 and 13 both
  "LAXSFO12/24/85"]. Index order among equal keys is by insertion/tree position,
  not by recno [inferred].

---

## 6. Key collation

* **Character keys** collate by **unsigned byte value** (ASCII/OEM CP437),
  left-to-right, space-padded. This is plain ASCII order: uppercase letters
  (0x41..) sort before lowercase (0x61..), digits (0x30..) before letters.
  [verified: byte-check -- CNAMES "DeBello" (0x44 0x65 0x42...) sorts before
  "Dean" (0x44 0x65 0x61...) because 'B'(0x42) < 'a'(0x61); i.e. it is byte
  order, NOT case-insensitive]. This is the default III+ collation (SET to the
  installed code page; the pristine fixtures are plain ASCII).
* **Numeric / date keys** collate by the `double` value (section 4.2).
* No record-number tiebreak is encoded in the key; equal keys are simply
  adjacent (section 5).

---

## 7. Cross-references / lineage (NOT III+ on-disk facts)

These are labeled contrasts; do not treat as III+ byte authority.

* **dBASE IV `.mdx`** [lineage]: multi-index container (up to 47 tags, default
  1024-byte blocks). Reuses the NDX B-tree node shape but stores only ONE long
  (block# OR recno) per entry, so its entry layout differs. III+ uses
  single-file `.ndx` only. [documented: piclist para "dBASE IV supports..." +
  msodbc-dbase-indexes].
* **Clipper `.ntx`** [contrast]: 1024-byte pages; header key-expr capacity 256;
  the node begins with an **array of 2-byte offsets** indexing the unordered
  key entries (NDX keeps entries physically sorted, no offset array); NTX uses
  **no filler/padding** and stores numeric/date keys as **ASCII** not doubles.
  Both branch and leaf can hold recnos. [documented: clicketyclick ntx.md +
  piclist Listing 2; structure confirmed by oracle
  `oracles/harbour/src/rdd/dbfntx/dbfntx1.c:1466`
  `MaxKeys = (NTXBLOCKSIZE - 2)/(uiKeyLen + 10) - 1` -- note the per-entry
  overhead 10 = 2 longs + 2-byte offset, vs NDX's 8 + 4-byte-aligned filler with
  a 4-byte node header]. Cousin, NOT byte-identical.
* **Fox `.idx`** [contrast]: 512-byte pages, node-type flag, single long per
  entry, recnos stored **big-endian**, doubly-linked leaves, no padding.
  [documented: piclist Listing 3]. Not III+.

---

## 8. Implementation checklist (byte-exact reader)

1. Read page 0: parse the 10 header fields (section 2). Recompute
   `group_length = ceil4(key_length+8)` and
   `keys_per_page = (512-4)/group_length`; if they disagree with the stored
   values, trust the stored values but flag corruption.
2. Read the NUL-terminated `key_expr` from 0x18 (cap 100).
3. To enumerate in sorted order: in-order traverse from `root_page`. For each
   node read `entry_count` (off 0); for `i in 0..entry_count-1` read group at
   `4 + i*group_length`: `child_page` (u32), `dbf_recno` (u32), `key_data`
   (key_length bytes). If `child_page != 0` recurse into it before emitting the
   entry's key (branch); else emit `(key_data, dbf_recno)` (leaf). After the
   last entry, if internal, recurse into the trailing child pointer at
   `4 + entry_count*group_length` (rightmost child).
4. Decode `key_data` per `key_type` (section 4). Ignore filler bytes.
5. Never read past offset 512 in a page; ignore the trailing slot in leaves.

---

## Open questions

* **[RESOLVED 2026-06-16 — minted]** Live numeric (N-field) key bytes, incl.
  negatives + decimals, are **raw LE IEEE-754 doubles with no transform**, compared
  ARITHMETICALLY (section 4.2). Minted `NCOST.NDX` (`INDEX ON UNITCOST` over a table
  seeded with `-123.45,-1,0`) confirmed it byte-for-byte. The old CHKNO.NDX fixture
  was empty (CHECKS.DBF nrec=0); this was minted fresh. See re/mint-results-001.md.
* **[oracle-resolves] `dummy` field at offset 0x14 semantics.** It holds
  nonzero garbage (0x0010, 0x0008, 0x000F) in some files and 0 in others, with
  no correlation to key_length, key_type, or unique. piclist's struct lists it
  as `short dummy;` with no meaning. Confirmed unused-for-reading, but whether
  the writer ever sets it deliberately is unresolved -- check DBASE.EXE.
* **[oracle-resolves] `reserved`/`version` at offset 0x08.** Always 0 in III+
  fixtures; piclist guesses "version (uncertain)". Whether III+ ever writes
  nonzero here (e.g. on a specific code page or after a specific operation) is
  unconfirmed; assume 0 for III+.
* **[oracle-resolves] clicketyclick header sub-layout contradicts the bytes.**
  clicketyclick's diagram puts "Size of key record" as a 4-byte long at bytes
  18-21 and `unique` as a single byte at 23, with reserved at 22. The real
  bytes show group_length is 2 bytes @0x12, dummy 2 bytes @0x14, unique 2 bytes
  @0x16 (Bachmann/piclist layout). The bytes win; clicketyclick is wrong on
  18-23. Recorded here so no one re-imports the bad layout.
* **[RESOLVED 2026-06-16 — minted]** `unique_flag` @0x16/0x17: `INDEX ... UNIQUE`
  sets it to **1** (minted `NUNIQ.NDX`); non-unique = 0. III+ `INDEX` is ascending
  only (no DESCENDING; that is a IV feature) -- unchanged. See re/mint-results-001.md.
* **[RESOLVED 2026-06-16 — minted]** Multi-field key length: `INDEX ON A+B` stores a
  concatenated key of length = sum of parts (`LASTNAME+FIRSTNAME` of two C20 ->
  key_length 40, group 48), key_expr stored verbatim. Minted `NMULTI.NDX`.
* **[RESOLVED 2026-06-16 — minted (bulk build)]** A bulk `INDEX ON` / `REINDEX`
  **packs each leaf to capacity (`keys_per_page`) in key order, left-to-right,
  with the remainder in the final leaf** — NOT 50/50 splitting. Minted BIGIDX.NDX
  (245 records, key_len 20, kpp 18): 14 leaves = 13 full (18/18) + 1 last (11),
  one root branch with **13 separator keys for 14 children** (confirms the
  N-keys/N+1-children branch structure: separators = the first key of leaves
  2..N). A byte-exact writer reproduces `INDEX ON` output by packing leaves then
  building parents over the leaves' first keys. See re/mint-results-003.md.
  *(Still open, finer: the INCREMENTAL-insert split — APPEND one record into an
  already-open `.ndx`, forcing a mid-leaf split — may use a true 50/50 split;
  not yet minted. Lower priority; REINDEX is the primary path.)*

---

## Sources

Read and cited (local-only; no WebFetch needed):

* `archive/piclist/ndxs.md` -- Bachmann's "Xbase index file formats" article;
  `index_header` / `key_group` / `index_block` C structs (Listing 1). Primary
  on-disk layout source. (Casing claim shown WRONG by fixtures.)
* `archive/clicketyclick/ndx.md` -- ASCII header/node diagrams + search
  algorithm + notes 1-8. (Header bytes 18-23 sub-layout shown WRONG by
  fixtures.)
* `archive/clicketyclick/ntx.md` -- Clipper NTX header/node diagram (contrast).
* `archive/misc/msodbc-dbase-indexes.md` -- ODBC dBASE index limits (per-column
  <100 bytes, multi-column must be text, unique-index definition); III vs IV
  `.ndx`/`.mdx` distinction.
* `oracles/harbour/src/rdd/dbfntx/dbfntx1.c` -- NTX RDD (semantic structural
  cousin only): `MaxKeys = (NTXBLOCKSIZE-2)/(uiKeyLen+10)-1` @line 1466,
  `NTX_MAX_KEY`. Used for structure analogy, NOT byte authority.

Golden fixtures byte-inspected with python3/xxd (paths under
`goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/`):

* `CNAMES.NDX` + `CLIENTS.DBF` -- char multi-field key `LASTNAME + FIRSTNAME`
  (KL=40), multi-page tree (root=6 branch, 5 leaves); header layout, branch
  high-key invariant, leaf recnos resolved to CLIENTS records, full-leaf
  trailing-slot overflow.
* `NAMES.NDX`, `TNAMES.NDX` -- identical structure to CNAMES (header parsed).
* `ZIPCODE.NDX` + `CLIENTS.DBF` -- char key `ZIPCODE` (KL=5), 2-level tree;
  branch separator = high key of left leaf confirmed.
* `CUSTOMER.NDX` + `CUSTOMER.DBF` -- char key `CDES_CITY+CNAME` (KL=28),
  single-page leaf; entry_count, filler garbage, sorted keys, recnos resolved.
* `CHKNO.NDX` + `CHECKS.DBF` -- numeric key flag (key_type=1) over N field;
  DBF empty (0 recs) so node bytes are garbage -- documented as Open question.
* `TOURDATE.NDX` + `TOURS.DBF` -- date key (key_type=1), 30 entries; JDN double
  encoding byte-verified.
* `AVAL_FLT.NDX` + `AVAL_FLT.DBF` -- char key with `dtoc(adate)` (KL=14);
  filler bytes and 3-field concatenation verified.
* `FLT_NO.NDX`, `LOCATION.NDX`, `TRIPS.NDX` -- header layout + grouplen/kpp
  formulas; TRIPS root branch (3 separators, 4 children) verified.

---

## Verification

Adversarial byte-level re-derivation performed 2026-06-16 with python3 `struct`
against all 11 golden `.ndx` files (and their paired `.dbf`). Every offset,
size, encoding, and formula claim in this document was re-derived independently
from the raw bytes; **no errors were found** and every checked claim matched.

**Fixtures byte-checked (all 11):** `CNAMES.NDX`+`CLIENTS.DBF`, `NAMES.NDX`,
`TNAMES.NDX`, `ZIPCODE.NDX`+`CLIENTS.DBF`, `CUSTOMER.NDX`+`CUSTOMER.DBF`,
`CHKNO.NDX`+`CHECKS.DBF`, `TOURDATE.NDX`+`TOURS.DBF`, `AVAL_FLT.NDX`+`AVAL_FLT.DBF`,
`FLT_NO.NDX`, `LOCATION.NDX`, `TRIPS.NDX`.

**Confirmed (all by re-derivation, not by re-citing the source docs):**

* All 10 header fields parsed for all 11 files: `root_page`, `total_pages`
  (`== filesize/512` for every file), `reserved` (`== 0` for all 11),
  `key_length`, `keys_per_page`, `key_type`, `group_length`, `dummy`,
  `unique_flag` (`== 0` for all 11), `key_expr`.
* Both formulas hold for all 11: `group_length == ceil4(key_length+8)` and
  `keys_per_page == (512-4)//group_length`. Spot-checked edges:
  KL 2->grp 12/kpp 42 (LOCATION), KL 3->12/42 (FLT_NO), KL 5->16/31 (ZIPCODE),
  KL 14->24/21 (AVAL_FLT), KL 28->36/14 (CUSTOMER), KL 40->48/10 (CNAMES).
* CNAMES header byte dump (0x00..0x2D in section 2.2) reproduced byte-for-byte,
  including `"LASTNAME + FIRSTNAME "` starting exactly at 0x18 and NUL at 0x2D.
* `dummy` values reproduced exactly: AVAL_FLT=0x0010, CUSTOMER=0x0008,
  FLT_NO=0x000F, all others 0x0000 (no correlation to KL/type/unique).
* Node header = 2-byte `entry_count` + 2-byte `filler` (CUSTOMER count=5,
  filler=0x5543 garbage; CNAMES root count=4, filler=0; TRIPS root count=3).
* Group layout child_page(u32)/dbf_recno(u32)/key_data(KL)/pad confirmed; branch
  entries carry recno=0 and child!=0, leaf entries child=0; AVAL_FLT 2-byte
  filler observed as 0x0000.
* Branch high-key invariant (separator i == high key of subtree child[i]):
  CNAMES root e0 `"Collins...Sara"` == leaf page1 last live key; ZIPCODE root e0
  `"91306"` == page1 last key and page2 (rightmost child) starts `"91316"`;
  LOCATION root e0 `"NM"` == child page1 last key.
* Trailing (N+1th) child pointer: CNAMES root trailing child=5; ZIPCODE trailing
  child=2; TRIPS root 3 separators (children 1,2,3) + trailing child 4.
* Full-leaf overflow: CNAMES page1 entry_count=10==kpp, groups occupy
  4+10*48=484 bytes, bytes 484..492 (trailing child slot) all 0x00.
* Stale trailing leaf slots: CUSTOMER `"ATLKKK"` recno=6 (only 5 recs);
  CNAMES page5 trailing `"Plimpton...Daniel"` recno=12 (page5 last live key is
  `"Zambini...Rick"`).
* Char concatenation keys resolved against the DBF: CNAMES rec15 ==
  CLIENTS LASTNAME `"Adams"`+FIRSTNAME `"Nathan"` (each 20, space-padded);
  CUSTOMER rec1 == CDES_CITY `"ATL"`+CNAME `"LOUIS"` (28); AVAL_FLT rec2 ==
  ADEP `"ATL"`+ADES `"BOS"`+DTOC(ADATE 19851203)=`"12/03/85"`.
* Date/JDN double encoding byte-exact for all three worked examples:
  19850805->JDN 2446283->`00 00 00 80 E5 A9 42 41` (TOURDATE leaf e0, recno=1);
  19850907->2446316->`00 00 00 00 F6 A9 42 41`; 19850923->2446332->
  `00 00 00 00 FE A9 42 41`. TOURDATE key_type=1, KL=8, grp=16, 30 live entries.
* Casing verbatim (piclist "lowercase" refuted): `LASTNAME + FIRSTNAME `,
  `Chkno ` (NUL at expr offset 6 then garbage), `DEPARTURE `, `CDES_CITY+CNAME `
  (mixed/upper) coexist with `aflt_no `, `adep_city+ades_city+dtoc(adate) `
  (lower) -- bytes echo the source text.
* Collation = unsigned byte order: CNAMES `"DeBello"` (0x42) sorts before
  `"Dean"` (0x61); full in-order traversal of CNAMES yields 49 leaf recnos
  (1..49) in sorted key order, first `"Adams Nathan"`, last `"Zambini Rick"`;
  ZIPCODE in-order yields 50 sorted keys `"72450".."97401"`.
* Duplicate keys (unique=0): TOURDATE recnos 9,10 both JDN 2446363; AVAL_FLT
  recnos 12,13 both `"LAXSFO12/24/85"`, and 10,11 both `"SEASFO12/20/85"`.
* NAMES.NDX and TNAMES.NDX header bytes 0x00..0x13 + expr are byte-identical to
  CNAMES.NDX (same structure, as claimed).
* CHKNO empty-DBF case: CHECKS.DBF nrec=0; CHKNO page1 entry_count=0 with binary
  garbage -- confirms the Open question (no live numeric key bytes available).
* III+ vs IV/7 sanity: no 48-byte dBASE-7 field descriptor or IV-only rule is
  present; key_expr is at 0x18 with 100-byte capacity (not a IV/`.mdx`
  container); the 512-byte page geometry and 4-byte node header are III+.
* Cross-reference oracle spot-checked: `oracles/harbour/src/rdd/dbfntx/dbfntx1.c`
  line 1466 is verbatim `pTag->MaxKeys = ( NTXBLOCKSIZE - 2 ) / ( uiKeyLen + 10 )
  - 1;` (contrast/cousin only, correctly labeled non-III+).
* Document is ASCII-clean (0 non-ASCII bytes re-confirmed).

**Fixes applied:** none. No byte claim was found to be wrong; all confidence
tags already match what the bytes support, so none were weakened or upgraded.

**Residual [oracle-resolves] gaps (unchanged, all confirmed still open):**

* Live numeric N-field key bytes still unconfirmed (CHECKS.DBF is empty; only
  the date/JDN double path is byte-confirmed). Decimal/negative numeric handling
  is inferred-by-shared-encoding, not dumped.
* `dummy`@0x14 semantics: holds nonzero garbage (0x0010/0x0008/0x000F) with no
  derivable meaning; writer intent unresolved.
* `reserved`/`version`@0x08: always 0 in III+ fixtures; whether III+ ever writes
  nonzero is unconfirmed.
* `unique_flag`=1 exact value: no unique fixture available (all 11 are 0).
* Writer-side split/rebalance/min-fill policy: not derivable from static
  fixtures.
