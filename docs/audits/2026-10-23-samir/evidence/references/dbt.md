# DBT — dBASE III PLUS 1.1 memo file (.dbt)

**Status / confidence:** Byte-validated against three III+ 1.1 golden fixtures
(TOURS.DBT, TRAVEL.DBT, UNIVERS.DBT) plus their matching tables (TOURS.DBF,
TRAVEL.DBF, UNIVERSD.DBF). The fixed 512-byte block size, the little-endian
next-available-block pointer at offset 0, the `0x1A 0x1A` memo terminator, and
the 10-byte right-justified space-padded ASCII block pointer in the `.dbf` M
field are all confirmed against real bytes. **Two source contradictions are
RESOLVED here by the golden master, against what the brief expected:**

1. **Next-block endianness is LITTLE-endian** in III+ `.dbt`. The sister brief
   said "cl-db3 reads big-endian"; on inspection cl-db3's `read-uint32` is in
   fact little-endian (and so is its `.dbt` read). independent-software.com's
   "BIG ENDIAN" annotation is wrong for `.dbt`; big-endian belongs to FoxPro
   `.fpt`, a different file (contrast only, out of scope). All three goldens
   confirm little-endian. [verified: TOURS/TRAVEL/UNIVERS.DBT @0x00]
   [oracle-resolves: settled — see Open questions for residual notes]

2. **A pointer value of "0" means NO memo**, not "memo at block 0." Block 0 is
   the header; no record ever points into it. Records with no memo store the
   right-justified ASCII string `"         0"` (nine spaces + `0`), *not*
   all-blanks, in these fixtures. [verified: TRAVEL.DBF records 3-49,
   all TOURS.DBF records]

TARGET = dBASE III PLUS 1.1 (1986). dBASE IV and FoxPro `.fpt` facts appear only
as clearly-labelled **LINEAGE / CONTRAST** and are **out of scope** for III+.

Cross-refs: the M-field pointer is also described in
[`dbf.md`](dbf.md) (record encoding); the FoxPro `.fpt` contrast cites
`archive/clicketyclick/fpt.md`.

---

## 1. What a .dbt is, and when it exists

A `.dbt` ("DataBase Text") holds the variable-length text of every **memo
(`M`) field** in the companion `.dbf`. The `.dbf` itself stores only a fixed
10-byte pointer per memo field; the text lives in the `.dbt`. A `.dbt` exists
iff the `.dbf` version byte has bit 7 set: `0x83` = III/III+ with memo.
[verified: TOURS.DBF/TRAVEL.DBF version byte = 0x83 @0x00; UNIVERSD.DBF =
0x03, has no M field and its sibling UNIVERS.DBT is an orphan]
[documented: archive/clicketyclick/dbf.md, archive/independent-software/dbf-dbt-fpt.md]

The two files share a basename and differ only in extension (`TRAVEL.DBF` +
`TRAVEL.DBT`). The `.dbt` is **append-only**: rewriting or deleting a memo never
reclaims blocks; the only way to compact is `COPY`.
[documented: archive/clicketyclick/dbt.md]

The memo file tells you nothing about its own use — you MUST read the `.dbf` to
know which blocks are live. There is no in-file deleted-block marking.
[documented: archive/clicketyclick/dbt.md]

---

## 2. Block model

The `.dbt` is a sequence of **fixed 512-byte blocks**, numbered from 0:

```
+-----------------------------------------------+  file offset 0x000
| BLOCK 0  (header)                             |  512 bytes
|   off 0..3 = next-available block (uint32 LE) |
|   off 4..511 = reserved / leftover garbage    |
+-----------------------------------------------+  file offset 0x200
| BLOCK 1  (first memo, or unused)              |  512 bytes
+-----------------------------------------------+  file offset 0x400
| BLOCK 2                                        |  512 bytes
+-----------------------------------------------+
|   ...                                         |
+-----------------------------------------------+
```

Block size is a **hard-coded 512 in III+** (it is NOT stored in the file; the
reader assumes 512 because the `.dbf` version byte is `0x83`). The byte offset
of block `n` is `n * 512`. [verified: TRAVEL.DBT block boundaries at 0x000 /
0x200 / 0x400; cl-db3 db3.lisp:368-369 hard-codes 512 for version 0x83]
[documented: archive/independent-software/dbf-dbt-fpt.md "generally 512 bytes"]

A memo **starts on a block boundary** and runs forward across as many
consecutive blocks as it needs, terminated by `0x1A 0x1A` (two consecutive
SUB / EOF bytes). The remainder of the final block after the terminator is
**unused garbage** (left-over bytes from a previous memo write — see section 6).
[verified: TRAVEL.DBT blocks 1 and 2, terminator confirmed below]

### 2.1 Block 0 — header

| off (hex) | size | field | type | notes |
|---|---|---|---|---|
| `0x00` | 4 | next-available (next-free) block number | uint32 **LE** | block at which the next appended memo would start = count of 512-byte blocks the file logically occupies; see section 3 (caution: not a clean `filesize/512` — a stray trailing byte can make filesize 513) |
| `0x04` | 508 | reserved | bytes | III+ leaves leftover memo text / zeros here ("garbage"); **not** parsed |

dBASE III+ does **not** store a block size, a `.dbf` filename, or a version
number in any place we can confirm in these fixtures. (clicketyclick's III+
diagram claims a version `0x03` at byte 16 and "reserved" at 4..15, but the
real bytes at offset 4 onward in our goldens are simply leftover memo text or
zeros — there is no reliable structured field there in III+ 1.1.)
[verified: TOURS/TRAVEL/UNIVERS.DBT @0x04..0x1F are unstructured]
[documented (and partially contradicted): archive/clicketyclick/dbt.md]
Validating fixtures: TOURS.DBT, TRAVEL.DBT, UNIVERS.DBT.

### 2.2 Memo content blocks (block 1+)

In III+ there is **no per-block header**: the memo text begins at byte 0 of its
starting block and runs raw until `0x1A 0x1A`. (This is the key III-vs-IV delta;
see section 5.) [verified: TRAVEL.DBT block 1 starts directly with memo text;
cl-db3 db3.lisp:391-400 "dbase III+ memo record only have the data, no type, no
size"]

Within memo text, the dBASE memo editor uses:
- `0x0D 0x0A` (CR LF) for a **hard** paragraph break (user pressed Enter), and
- `0x8D 0x0A` for a **soft** word-wrap line break (auto-inserted at the right
  margin; `0x8D` = `0x0D | 0x80`).

[verified: TRAVEL.DBT block 2 contains both `0d 0a` and `8d 0a`]
[documented: archive/clicketyclick/dbt.md note *2 (attributed there to dBASE IV,
but it is already present in III+ 1.1 memo text)]

---

## 3. The next-available-block pointer (offset 0) — endianness RESOLVED

This is the field the brief flagged as contradicted across sources. The golden
master settles it: **little-endian**.

| fixture | bytes @0x00 | LE value | BE value | file size | live memo blocks | matches |
|---|---|---|---|---|---|---|
| TOURS.DBT  | `01 00 00 00` | 1 | 16,777,216 | 513 | none (all M ptrs = "0") | **LE** (next free = block 1; only block 0 exists) |
| TRAVEL.DBT | `03 00 00 00` | 3 | 50,331,648 | 1111 | blocks 1, 2 | **LE** (next free = block 3) |
| UNIVERS.DBT| `01 00 00 00` | 1 | 16,777,216 | 513 | none (orphan; DBF has no M field) | **LE** (empty memo file, next free = 1) |

For TRAVEL: records 1 and 2 carry memos (pointers "1" and "2"), so blocks 1 and
2 are consumed and the next free block is **3** — exactly the little-endian
reading. The big-endian reading (50,331,648) is absurd for an 1111-byte file.
[verified: TRAVEL.DBT @0x00 + TRAVEL.DBF memo pointers + file size]

The pointer equals the count of 512-byte blocks the file logically occupies —
i.e. the index of the first block past the live data — which is the logical end
of file (the file is append-only). **It is NOT a clean `filesize / 512`:**

- TOURS = 513 bytes, next-free = 1: only block 0 is live; the 513th byte is a
  single stray `0x1A` (not a real block). `floor(513/512)=1` happens to match.
- UNIVERS = 513 bytes, next-free = 1: same stray-byte situation (orphan/empty).
- TRAVEL = 1111 bytes, next-free = 3: blocks 0,1,2 are live (block 2 holds 87
  bytes of memo + terminator); `floor(1111/512)=2` does NOT match — only
  `ceil(1111/512)=3` matches.

So no single floor-or-ceil-of-filesize formula fits all three; the pointer
tracks the **logical block count**, with the trailing partial/stray byte in
TOURS/UNIVERS NOT counted as an extra block. [verified: all three fixtures,
exact arithmetic above] [documented: archive/clicketyclick/dbt.md "Next
available block will always point the end of the file"]

**Oracle agreement.** cl-db3 reads this field with `read-uint32`
(`db3.lisp:188-193`), which accumulates byte0 at bit 0, byte1 at bit 8, ... =
**little-endian**, and applies it directly to `.dbt` (`db3.lisp:367`). So cl-db3
agrees with the golden master; the brief's "cl-db3 reads big-endian" was a
misreading. clicketyclick and independent-software's prose are split, but the
bytes win (Law 2). [verified: oracles/cl-db3/db3.lisp:188-193, 367]

---

## 4. The memo pointer inside the .dbf record (the M field)

Each `M` field occupies **10 bytes** in every `.dbf` record. III+ stores the
starting block number as a **right-justified, space-padded ASCII decimal
string**.

| value on disk (10 bytes) | meaning |
|---|---|
| `20 20 20 20 20 20 20 20 20 30` = `"         0"` | **no memo** (block 0 is the header; never a target) |
| `20 20 20 20 20 20 20 20 20 31` = `"         1"` | memo starts at block 1 (file offset `1*512 = 0x200`) |
| `20 20 20 20 20 20 20 20 20 32` = `"         2"` | memo starts at block 2 (file offset `2*512 = 0x400`) |
| `20 ... 20` = `"          "` (all spaces) | also "no memo" (see note) |

To read a memo: take the 10 bytes, `trim()` the leading spaces, parse the
decimal integer `n`; if `n == 0` or the field is blank, there is no memo;
otherwise seek to `n * 512` in the `.dbt` and read until `0x1A 0x1A`.
[verified: TRAVEL.DBF NOTES field @ record-offset 127, len 10: rec1="1",
rec2="2", rec3..49="0"; TOURS.DBF NOTE field @ record-offset 73, len 10: all
records "0"] [documented: archive/independent-software/dbf-dbt-fpt.md "stored
as a string padded with spaces"; oracles/cl-db3/db3.lisp:373-400]

**"No memo" sentinel — note for round-trips.** The brief said "all-blank = no
memo." Our III+ 1.1 fixtures instead write a right-justified ASCII **`0`** for
the empty case. Both forms parse to "no memo" (trim→`""`→nothing, or
trim→`"0"`→block 0 = header = nothing). A robust reader MUST treat *both*
all-blank and a parsed value of 0 as "no memo," and a III+-faithful writer
should emit `"         0"` to match these goldens. [verified: fixtures above]
[oracle-resolves: which exact sentinel DBASE.EXE emits on every code path — see
Open questions]

### 4.1 Worked example (TRAVEL record 1 -> block 1)

```
TRAVEL.DBF record 1, NOTES bytes        = 20 20 20 20 20 20 20 20 20 31  ("1")
  -> block 1 -> .dbt offset 1*512       = 0x200
TRAVEL.DBT @0x200 .. until 0x1A 0x1A (@0x27F):
  "\r\nPaid on 7/15/85    Visa 9999-111-999-999\r\n\r\n
   Name on the Card: Claire M. Buckman\r\n\r\n
   Expiration Date: 5/86\r\n\r\nApproval: JK \r\n\r\n"
```

[verified: TRAVEL.DBT block 1, `0x1A 0x1A` at file offset 0x27F]

---

## 5. The 0x1A 0x1A terminator

A III+ memo ends at the **first `0x1A 0x1A`** (two consecutive SUB bytes)
encountered from its starting block. There is no length field; the terminator
is the only end marker.

| fixture | block | `0x1A 0x1A` at block-relative offset | file offset |
|---|---|---|---|
| TRAVEL.DBT | 1 | 127 | 0x27F |
| TRAVEL.DBT | 2 | 85 | 0x455 |
| TOURS.DBT  | 0 (leftover) | 103 | 0x067 |

[verified: TRAVEL.DBT block 1 terminator @0x27F (block-rel 127), block 2 @0x455
(block-rel 85); TOURS.DBT block-0 leftover terminator @0x67 (block-rel 103)]
cl-db3 searches for `#(#x1a #x1a)` and
**errors** if absent (`db3.lisp:396-400`), confirming the double-SUB is the
contract, not a single SUB. (clicketyclick note *1 muses that "only one field
terminator (1Ah)" may suffice for FoxPro — that is not III+; III+ writes and
expects two.) [verified: oracles/cl-db3/db3.lisp:396-400 + TRAVEL.DBT bytes]
[documented: archive/clicketyclick/dbt.md]

Note: a long memo that exactly fills its blocks still terminates with
`0x1A 0x1A` in the next available bytes; the terminator can fall anywhere inside
a block, and everything after it in that block is unused.

---

## 6. Append-only behaviour and block-tail garbage (round-trip normalization)

dBASE III+ **appends** on every memo write — it never overwrites in place and
never reclaims freed blocks. Consequences visible in the goldens:

- **Block 0 carries leftover text.** TOURS.DBT block 0 (after the 4-byte header)
  and TRAVEL.DBT block 0 both contain stale memo text ("...Claire M.
  Buckman...Approval: JK...") from earlier edits. This is never read (no record
  points to block 0). In TOURS the leftover ASCII begins right at `0x04`; in
  TRAVEL offsets `0x04..0x19` hold unstructured binary leftovers and the ASCII
  leftover (`"99-111-999-999"...`) begins at `0x1A`. [verified: TOURS.DBT @0x04
  = `39 39 2d 31 31 31...`; TRAVEL.DBT @0x1A = `39 39 2d 31 31 31...`, both
  contain "Claire M. Buckman" and "Approval: JK"]
- **Final-block tails are garbage.** Bytes after `0x1A 0x1A` within a block are
  meaningless. TOURS.DBT's block-0 tail (offsets ~0xE0..0x1FF) holds random
  non-zero bytes left from a prior write. [verified: TOURS.DBT @0xE0..0x1FF]
- **Odd trailing byte.** TOURS.DBT and UNIVERS.DBT are **513 bytes** = one full
  512-byte block + a single stray `0x1A` at offset 0x200. The trailing partial
  byte is not a second block. [verified: TOURS.DBT/UNIVERS.DBT size 513,
  byte[512] = 0x1A]

**Round-trip rule (matches the sister brief, section 2 "NORMALIZE"):** compare
memo **content keyed by record**, not raw `.dbt` bytes. Mask (a) block-0 bytes
4..511, (b) every block's bytes after its `0x1A 0x1A` terminator, and (c) any
trailing partial block. Block-number *assignment* may also legitimately differ
between two writers, so prefer value-level comparison of the decoded memo text
over byte-identical `.dbt` layout unless the allocator itself is under test.
[verified: fixtures above] [documented: brief section 2 / 3.2]

---

## 7. LINEAGE / CONTRAST (out of scope for III+)

These are recorded so a reader does not import a later rule into a III+ claim.
**None of this applies to the `0x83` III+ format above.**

### 7.1 dBASE IV memo (`.dbt`, version byte `0x8B`) — CONTRAST, not III+

dBASE IV adds an **8-byte per-memo header** at the start of each memo's first
block:

| off (hex) | size | field | type | notes |
|---|---|---|---|---|
| `0x00` | 4 | signature | bytes | `FF FF 08 00` |
| `0x04` | 4 | memo length **including** the 8-byte header | uint32 LE | authoritative; text follows at byte 8 |

IV block 0 also stores the `.dbf` basename (off 8..15) and a per-file block size
(off 20..21, uint16 LE), so IV block size is NOT fixed at 512. The `0x1A 0x1A`
pair may still appear but the length field wins. **III+ has none of this.**
[documented: archive/clicketyclick/dbt.md "dBASE IV" diagram; cross-checked
oracles/cl-db3/db3.lisp:402-407 (#x8b branch reads `FF FF 08 00` then a 4-byte
size)] [verified: NOT present in any III+ golden — III+ blocks begin with raw
text]

### 7.2 FoxPro `.fpt` — CONTRAST, different file, out of scope

FoxPro 2.x+ uses a `.fpt` file (not `.dbt`). Its header pointer at offset 0 IS
**big-endian** (high byte first), and each memo block has a 4-byte big-endian
record type + 4-byte big-endian length. The memo *pointer* in the `.dbf` record
is a **4-byte binary uint32**, not a 10-byte ASCII string. This big-endian
`.fpt` is almost certainly the origin of the "big-endian" claim that leaked into
`.dbt` documentation. [documented: archive/clicketyclick/fpt.md notes *1, *2;
oracles/harbour/src/rdd/dbffpt/dbffpt1.c is an `.fpt`/FoxPro RDD] **Do not apply
to III+ `.dbt`.**

---

## 8. Reader / writer recipe (III+ 0x83)

**Read a memo for a record:**
1. Confirm `.dbf` version byte = `0x83` and an M field exists.
2. Read the 10-byte M field; `n = atoi(trim(field))`. If blank or `n == 0`,
   memo is empty -> stop.
3. Seek `.dbt` to `n * 512`.
4. Read forward (across consecutive 512-byte blocks) until `0x1A 0x1A`; the
   bytes before the terminator are the memo text (CP437; `0x0D 0x0A` = hard
   break, `0x8D 0x0A` = soft wrap).

**Append a memo:**
1. `start = next_available_block` (uint32 LE @ `.dbt` offset 0).
2. Write the text at `start * 512`, then `0x1A 0x1A`, padding the final block's
   tail to a 512-byte boundary (tail content is don't-care).
3. `next_available_block += ceil((len(text)+2) / 512)`; write it back LE @0.
4. Store `start` as a right-justified, space-padded ASCII decimal in the
   record's 10-byte M field; for "no memo" store `"         0"`.

[verified end-to-end: TRAVEL.DBF rec1 "1" -> TRAVEL.DBT @0x200 -> text -> 0x1A
0x1A @0x27F; next-free=3 consistent with two used blocks]

---

## Open questions

- **[oracle-resolves] Empty-memo sentinel emitted by DBASE.EXE.** Our III+ 1.1
  goldens write `"         0"` (ASCII zero) for "no memo," whereas the
  documentation describes "all-blank." Does `DBASE.EXE` ever emit all-blank on
  some code path (e.g. a never-edited memo vs. an edited-then-cleared memo)?
  Settle by minting both cases from real dBASE III+ 1.1. A reader must accept
  both regardless. (Goldens consulted: TOURS.DBF, TRAVEL.DBF.)
- **[oracle-resolves] Block-0 bytes 4..15 / 16.** clicketyclick's III+ diagram
  claims a version byte `0x03` at offset 16 and reserved bytes at 4..15. Our
  goldens show only leftover text/zeros there. Confirm whether a *freshly
  created* (never-appended) III+ `.dbt` writes any structured field at 4..16, or
  whether it is reserved/garbage from creation. The DBT in our set are all
  post-edit. Mint a virgin `.dbt` from `DBASE.EXE` to decide.
- **[inferred] Exact block-count rounding on append.** The
  `next_available_block += ceil((len+2)/512)` formula is inferred from the
  packed-file invariant (`next_free == filesize/512`); it has not been observed
  across a multi-append sequence. Verify by appending several memos in real
  dBASE and watching the offset-0 pointer step.
- **[oracle-resolves] Single vs double `0x1A` tolerance on read.** III+ writes
  `0x1A 0x1A` (verified) and cl-db3 requires it, but it is unconfirmed whether
  `DBASE.EXE`'s *reader* would stop at a lone `0x1A`. Out of caution a writer
  emits two; a reader keying off `record_count`/M-pointer is unaffected.

---

## Sources

- `archive/clicketyclick/dbt.md` — Bachmann/clicketyclick `.dbt` structure
  (III+ and IV diagrams, append-only note, terminator note). Primary doc.
- `archive/independent-software/dbf-dbt-fpt.md` — memo file header + block
  structure; M-field "string-or-binary" rule. Note: its header table marks the
  offset-0 pointer "BIG ENDIAN" — **contradicted by the golden master**
  (little-endian); see section 3.
- `archive/clicketyclick/fpt.md` — FoxPro `.fpt` (CONTRAST / out of scope);
  source of the genuinely big-endian header that confused `.dbt` docs.
- `oracles/cl-db3/db3.lisp` — Common Lisp dBASE III reader. `read-uint32`
  (lines 188-193, little-endian); `load-memo-header` (353-371, `.dbt` next-block
  via `read-uint32` = LE, block size 512 for `0x83`); `load-memo-record`
  (373-407, III+ = raw data terminated by `0x1A 0x1A`, IV = `FF FF 08 00` +
  length). Cross-check oracle only, NOT byte authority.
- `oracles/harbour/src/rdd/dbffpt/dbffpt1.c` — Harbour FoxPro `.fpt` RDD
  (CONTRAST). Confirms `.fpt` is the FoxPro lineage, not III+ `.dbt`.
- `../initech-os/docs/research/dbase-ground-truth.md` section 3 — vetted summary
  incl. the endianness contradiction (now resolved here) and the round-trip
  normalization contract (section 6 above).
- Golden fixtures (byte authority, Law 2):
  - `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TOURS.DBT` (513 b) + `TOURS.DBF` (v=0x83, 30 recs, NOTE:M @ rec-off 73)
  - `.../TRAVEL.DBT` (1111 b) + `TRAVEL.DBF` (v=0x83, 49 recs, NOTES:M @ rec-off 127)
  - `goldens/dbase-iii-plus-1.1/extracted/disk2/UNIVERS.DBT` (513 b) + `UNIVERSD.DBF` (v=0x03, no M field; orphan memo file)

---

## Verification

Adversarial byte-level re-derivation performed 2026-06-16 with python3/struct against
all three goldens and the cl-db3 oracle source. **Verdict: spec is accurate; all
core byte/offset/endianness claims independently reproduced. Two imprecisions
fixed (one cosmetic offset, one misleading `filesize/512` formula).**

**Fixtures checked (byte authority):**
- `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TOURS.DBT` (513 b) + `TOURS.DBF` (30 recs, recsize 83, hdr 257)
- `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/TRAVEL.DBT` (1111 b) + `TRAVEL.DBF` (49 recs, recsize 137, hdr 385)
- `goldens/dbase-iii-plus-1.1/extracted/disk2/UNIVERS.DBT` (513 b) + `UNIVERSD.DBF` (101 b)
- `oracles/cl-db3/db3.lisp` (cross-check oracle)

**Claims independently CONFIRMED:**
1. **Endianness = little-endian.** Bytes@0x00: TOURS `01 00 00 00`, TRAVEL
   `03 00 00 00`, UNIVERS `01 00 00 00`. LE = 1/3/1 (all sane); BE = 16777216 /
   50331648 / 16777216 (absurd for 513/1111/513-byte files). Settled by bytes.
2. **512-byte block; block n at offset n*512.** TRAVEL boundaries at 0x000 /
   0x200 / 0x400 confirmed by memo text starting exactly at 0x200 and 0x400.
3. **No III+ per-block header (III-vs-IV delta).** `FF FF 08 00` signature
   absent from all three goldens (substring search). TRAVEL block 1 @0x200 =
   `0d 0a "Paid o..."`, block 2 @0x400 = `0d 0a "Paymen..."` — raw text, byte 0.
4. **`0x1A 0x1A` terminator.** TRAVEL block 1 @0x27F (block-rel 127), block 2
   @0x455 (block-rel 85), TOURS block-0 leftover @0x67 (block-rel 103). All match
   section 5 table exactly.
5. **Worked example (TRAVEL rec1→block1).** Decoded block-1 text matches the
   section 4.1 listing **byte-for-byte** (cp437).
6. **Soft/hard breaks.** TRAVEL block 2 contains both `0d 0a` (hard) and
   `8d 0a` (soft, the `ì` = 0x8d char at block-rel 57).
7. **M-field pointers.** TOURS NOTE:M @ rec-offset 73, len 10 = `"         0"`
   for all 30 records. TRAVEL NOTES:M @ rec-offset 127, len 10 = `"         1"`
   (rec 1), `"         2"` (rec 2), `"         0"` (recs 3-49, i.e. 47 records).
8. **Version bytes / orphan.** TOURS.DBF & TRAVEL.DBF = 0x83 (bit 7 set, memo).
   UNIVERSD.DBF = 0x03 (no memo), 1 record, single field V:C(34), no M field;
   and its basename (UNIVERSD) differs from UNIVERS.DBT, so the `.dbt` is indeed
   an orphan with no `UNIVERS.DBF` partner.
9. **cl-db3 oracle.** `read-uint32` (188-193) accumulates byte0@bit0 = LE;
   `load-memo-header` (367-371) reads next-block via `read-uint32` and hard-codes
   512 for 0x83, per-file (offset 20) for 0x8b; `load-memo-record` 0x83 branch
   (391-400) reads raw + searches `#(#x1a #x1a)` and errors if absent; 0x8b
   branch (402-407) reads `FF FF 08 00` + 4-byte size. All as cited.
10. **Block-0 / tail garbage.** TOURS block-0 ASCII leftover begins at 0x04;
    "Claire M. Buckman" and "Approval: JK" present in both TOURS and TRAVEL block
    0. TOURS @0xE0..0x1FF has nonzero garbage. TOURS/UNIVERS byte[512] = 0x1A
    (single stray byte, 513-byte files).

**Fixes applied:**
- **Section 6 (cosmetic offset):** the TRAVEL block-0 leftover-ASCII offset was
  cited as `@0x18`; the ASCII leftover (`"99-111..."`) actually begins at `0x1A`
  (0x04..0x19 are unstructured binary, 0x18..0x19 = `00 00`). Corrected and
  expanded with the exact bytes for both files.
- **Sections 2.1 + 3 (misleading formula):** the "`= filesize/512` for a packed
  file" claim does NOT hold uniformly — TOURS/UNIVERS match `floor(513/512)=1`
  but TRAVEL needs `ceil(1111/512)=3` (`floor` gives 2, wrong). Rewrote to state
  the pointer = logical 512-byte block count (first block past live data), with
  the stray trailing byte in TOURS/UNIVERS NOT counted, and showed the exact
  arithmetic for all three fixtures.
- **Section 5 (tag strengthened):** the terminator-table verified tag now lists
  the TOURS block-0 terminator (@0x67) that I byte-checked, not just TRAVEL.

**Residual gaps (unchanged, correctly flagged in Open questions):**
- All-blank vs `"         0"` empty-memo sentinel: only `"         0"` observed;
  all-blank path unconfirmed (needs minted virgin/edited-then-cleared fixtures).
- Block-0 bytes 4..16 structured-field claim (clicketyclick): goldens show only
  leftover/binary garbage; a never-appended virgin `.dbt` is needed to settle.
- `ceil((len+2)/512)` append-rounding: consistent with TRAVEL but not observed
  across a multi-append sequence.
- Single-vs-double `0x1A` reader tolerance in DBASE.EXE: unconfirmed.
