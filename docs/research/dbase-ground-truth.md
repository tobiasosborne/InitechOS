<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->
<!-- Ground-truth brief: InitechBase (SAMIR) — .dbf/.dbt/.ndx/.mdx + the xBase -->
<!-- dot-prompt subset + the differential oracle. Law 1: every claim cites a -->
<!-- local file:line, the locked spec, the ADR/PRD, a bead, or a clearly-stated -->
<!-- external (period / reverse-engineered) reference. NO code ships from this -->
<!-- brief; the locked spec-data + the oracle do. -->

# InitechBase (SAMIR) Ground-Truth Brief — `.dbf`/`.dbt`/`.ndx`/`.mdx`, the xBase Subset, and the Differential Oracle

**Issuing Body:** Initech Systems Corporation — Platform Engineering, Data Services Section (SAMIR)
**Document Class:** Ground-Truth Brief (per-milestone evidence base; supersede in place)
**Programme / Milestone:** InitechOS (STAPLER) — **M6, InitechBase, codename SAMIR** (PRD §6.6, §11)
**Governing Epic:** `initech-586` (M6 — InitechBase). Engine lattice `initech-aul → 7az/ahu/gmo → 0tl/17n → ax9`; coercion `initech-gmo`; oracle `initech-17n`.
**Last Reconciled:** 2026-06-16

> Incoming agent: this brief is the *evidence base* for M6. It does not authorise code. Read `CLAUDE.md` (the Laws & Rules), then PRD §6.6 / §11-M6 / §14, then `bd show initech-586` and the engine beads, then this brief, then mint a golden from real dBASE before you write a byte. The oracle is the truth, not this document (Law 2): where a byte below is flagged **oracle-resolves**, do **not** reason it out — mint it from real dBASE III+/IV under DOSBox-X / 86Box and assert.

---

## 0. Scope, grounding, and the M6 Definition of Done

### 0.1 What SAMIR is (PRD §6.6, line 145–152)

InitechBase (codename SAMIR) is a **dBASE III+/IV-compatible database that really runs** as a **character-mode xBase program in an 80×25 text-console window inside the Mac desktop** — the DOS-box-in-the-GUI metaphor made concrete, with authentic CP437 / Turbo-Vision rendering (`InitechOS-PRD.md:147`). It is a **bundled app for the current release**, therefore **C** (ADR-0002; PRD §14; CLAUDE.md Law 3). It is **emphatically not Pascal** — Pascal is reserved for Turbo Initech (TPS) and the user programs it compiles (PRD §6.7; CLAUDE.md "Two compilers, never conflated"). The SAMIR interpreter is a **C tree-walking interpreter** that *interprets* xBase dot-prompt source; it does not compile and is not the self-host compiler. Its file home is `os/samir/` (CLAUDE.md file map), today a greenfield `.gitkeep`.

Scope per PRD §6.6 (line 149):
- `.dbf` reader/writer (field types **C/N/D/L/M**, the documented header)
- `.dbt` memos
- `.ndx`/`.mdx` **B+-tree** indexes
- a tree-walking interpreter for the dot-prompt subset: `USE`, `APPEND`, `LIST`, `LOCATE`/`SEEK`, `SET INDEX`/`SET ORDER`, `DO WHILE…ENDDO`, `IF…ELSE…ENDIF`, `REPLACE`
- `@ row,col SAY … GET … / READ` full-screen forms

### 0.2 The M6 Definition of Done (PRD §6.6 oracle, §11-M6 line 239, §8 gate vector)

M6 is **green** when both prongs of the PRD §6.6 oracle hold and the §8 `differential_pass_rate` gate reads 100% on the shared corpus:

1. **Interoperable round-trips.** A `.dbf`/`.mdx` SAMIR writes is **read back correctly by real dBASE III+/IV, and vice versa** — compared on the **meaningful bytes** only. Header fields like the last-update date and the reserved / language-driver bytes are **normalized before diffing**, "since exact reproduction of those is neither possible nor the point" (PRD §6.6, line 150; CLAUDE.md callout "`.dbf` round-trips are on *meaningful* bytes"). Reproducible builds (Rule 11) make the last-update stamp un-matchable anyway, which forces the normalization.
2. **Differential program output.** Generate xBase programs over the §6.6 subset, run them in **both** SAMIR and the reference, and **diff** the console transcript + the resulting `.dbf` (meaningful bytes) — gate at **100%** (PRD §8, line 201). The §11-M6 row names the reference explicitly: "Differential suite vs **real dBASE IV** green" (`InitechOS-PRD.md:239`).

The goldens are **mutation-proven** (Rule 6): perturb one branch/constant in the *real source* and confirm the oracle goes RED, then restore. This is the **port-and-verify / differential** TDD shape (CLAUDE.md "TDD shapes" #2), not spec-from-scratch.

### 0.3 Method note (Law 1 honesty — read before trusting any byte below)

The on-disk byte facts are corroborated across ≥2 of five independent reverse-engineered references (Erik Bachmann / clicketyclick.dk; independent-software.com; fileformat.info / Corion; the `cl-db3` and `dBASE.NET` codebases; the NDbfReaderEx Bachmann mirror). The language semantics are grounded against the dBASE language reference (dbase.com) **with a standing caveat**: dbase.com is the modern **dBASE PLUS / dB2K** help; the *relational/coercion core* (SET EXACT, `=` vs `==`, `$`, `+`/`-` on strings/dates) is lineage-stable from III+/IV, but PLUS adds OOP types and **auto-stringification that did NOT exist in III+/IV**. Where the modern doc and the period behavior diverge (notably `C + N`, §4.3), the **III+/IV behavior wins and is proven by fuzzing the period interpreter** — exactly the "do not infer from intuition" trap CLAUDE.md warns of for xBase. Items flagged **oracle-resolves** are NOT settled by this brief; they are minted from real dBASE.

### 0.4 Dependency wall (PRD §11; `bd show initech-586`)

`initech-586` `DEPENDS ON` exactly two upstream epics:
- **`initech-509` (M2 — MILTON), P0** — the `.dbf`/`.dbt`/`.ndx`/`.mdx` files live on the FAT volume and are reached through InitechDOS file I/O. SAMIR is a normal DOS program from the kernel's view: it uses the **handle API** (INT 21h 3Ch/3Dh/3Eh/3Fh/40h/42h), **not** FCB (operator note on `initech-509.9`: "North-star … uses the handle API, not FCB"). M2's FAT12+FAT16 read/write substrate and INT 21h handle surface have substantially landed (HANDOFF §4: WL-0008..0027; FAT16 decode `z01`, in-kernel FAT16 mount tracked as `initech-d27i`).
- **`initech-8oi` (M4 — FLAIR), P3** — the 80×25 text-console window (CP437 glyphs, Turbo-Vision chrome) inside the Mac desktop; the surface `initech-ax9` (the M6 gate) realizes.

Transitively M6 also rides M3 (ATKINSON region engine, for window clipping) and the M0.5 tracer keystone (`initech-f8v.4`), at which every engine bead's chain roots.

---

## 1. The `.dbf` table file — on-disk format

### 1.1 Overall layout

A `.dbf` is three concatenated regions plus a trailer (clicketyclick.dk/databases/xbase/format/dbf.html; corroborated independent-software.com, fileformat.info/Corion):

```
[ 32-byte file header ]
[ N x 32-byte field descriptors ]
[ 0x0D terminator ]              <- ends the header region; header_length points just past it
[ record 0 ][ record 1 ] ...     <- each (1 + sum(field_lengths)) bytes, fixed width
[ 0x1A ]                         <- EOF marker (semi-optional; see Sec 1.6)
```

Two structural invariants the parser MUST assert (fail loud, Rule 2; the validation formula `FileSize = NDR*DRS + HSZ` from fileformat.info/Corion):

- `header_length (off 0x08) == 32 + 32*num_fields + 1` (the `+1` is the `0x0D`).
- `file_size == header_length + record_count * record_length [+ 1 for the 0x1A]`.

If either fails on a file real dBASE wrote, **the parser is wrong, not dBASE** (Law 2). Panic on the mismatch.

> ⚠️ Hallucination guard (period vs dBASE 7). The dbase.com KB page `db7_file_fmt.htm` and the Embarcadero blog describe a **48-byte field descriptor** with a **68-byte header start** — that is the **dBASE 7 / Visual dBASE** format. **SAMIR targets III+/IV: the header prefix is 32 bytes and each field descriptor is 32 bytes.** Use the dBASE 7 page only for cross-checking the shared header bytes, never for the descriptor layout.

### 1.2 The 32-byte file header — byte map

| Off (hex) | Size | Field | Round-trip class |
|---|---|---|---|
| `0x00` | 1 | **Version / flags byte** (dialect + memo bit; see §1.3) | **MEANINGFUL** (low nibble + bit 7) |
| `0x01` | 1 | Last-update **year** (`YY`, actual = `YY+1900`) | **NORMALIZE** |
| `0x02` | 1 | Last-update **month** (1–12) | **NORMALIZE** |
| `0x03` | 1 | Last-update **day** (1–31) | **NORMALIZE** |
| `0x04` | 4 | **Record count** (`uint32` LE) | **MEANINGFUL** |
| `0x08` | 2 | **Header length** (`uint16` LE; offset of first record) | **MEANINGFUL** (derived; assert §1.1) |
| `0x0A` | 2 | **Record length** (`uint16` LE; includes the 1-byte delete flag) | **MEANINGFUL** (derived; assert §1.1) |
| `0x0C` | 2 | Reserved (`0x0000`) | **NORMALIZE** |
| `0x0E` | 1 | Incomplete-transaction flag (dBASE IV) | **NORMALIZE** |
| `0x0F` | 1 | Encryption flag (dBASE IV) | **NORMALIZE** |
| `0x10` | 12 | Multi-user / LAN reserved | **NORMALIZE** |
| `0x1C` | 1 | **Production `.MDX` flag** (`0x01` = a production `.MDX` exists, auto-opened) | **MEANINGFUL** iff SAMIR emits `.MDX` |
| `0x1D` | 1 | **Language driver / code-page ID** | **NORMALIZE** (PRD §6.6 names this explicitly) |
| `0x1E` | 2 | Reserved (`0x0000`) | **NORMALIZE** |

> **dBASE III note.** On classic III the run `0x0C–0x1F` is one undifferentiated reserved block (fileformat.info/Corion). The IV-era meanings of `0x0E`/`0x0F`/`0x1C`/`0x1D` were carved out of that reserve. Consequence: a III+ writer zeros `0x0C–0x1F` except possibly the MDX flag; an IV writer populates them. For III round-trips treat `0x0E–0x1F` as **NORMALIZE**.

### 1.3 Version / flags byte (`0x00`) — value table

Low nibble = format-version code; **bit 7 (`0x80`) = "an associated `.dbt` memo file exists."**

| Value | Meaning |
|---|---|
| `0x03` | dBASE III / III+ **without** memo |
| `0x83` | dBASE III / III+ **with** `.dbt` memo (`0x03 \| 0x80`) |
| `0x04` | dBASE IV without memo (sometimes seen) |
| `0x8B` | dBASE IV **with** memo (the canonical IV-with-memo byte) |
| `0x43`/`0x63`/`0xCB` | dBASE IV SQL-table variants — **OUT OF SCOPE** (reject or treat III-equivalent) |
| `0xF5`/`0x30` | FoxPro / VFP (`.fpt` memos, 4-byte binary memo pointer) — **OUT OF SCOPE** |

**SAMIR target set (PRD §6.6, §11-M6 "dBASE IV"):** read/write `0x03`, `0x83`, and dBASE IV `0x04`/`0x8B`. The memo bit and the low nibble are the two **meaningful** sub-fields — they select the `.dbt` block dialect (§3). **Oracle-resolves:** the exact IV version byte SAMIR should *emit* (`0x8B` with memo, `0x04`/`0x03` without).

### 1.4 The field-descriptor array (32 bytes each)

One descriptor per field, immediately after the header, terminated by a single `0x0D`.

| Off (within desc) | Size | Field | Round-trip class |
|---|---|---|---|
| `0x00` | 11 | **Field name** (ASCII ≤10, NUL-padded; dBASE upper-cases) | **MEANINGFUL** |
| `0x0B` | 1 | **Field type** (ASCII `C N D L M`, `F` in IV) | **MEANINGFUL** |
| `0x0C` | 4 | Field data address (in-memory pointer; garbage on disk) | **NORMALIZE** |
| `0x10` | 1 | **Field length** (binary) | **MEANINGFUL** |
| `0x11` | 1 | **Decimal count** (binary; nonzero only N/F) | **MEANINGFUL** |
| `0x12` | 2 | Reserved (III+ LAN) | **NORMALIZE** |
| `0x14` | 1 | Work-area ID | **NORMALIZE** |
| `0x15` | 2 | Reserved (III+ LAN) | **NORMALIZE** |
| `0x17` | 1 | SET-FIELDS flag | **NORMALIZE** |
| `0x18` | 7 | Reserved | **NORMALIZE** |
| `0x1F` | 1 | Production-`.MDX` field flag (`0x01` if a `.MDX` key field) | **MEANINGFUL** iff `.MDX` |

**Header terminator:** a single **`0x0D`** after the last descriptor is **mandatory and meaningful** — it is how a reader finds the end of the array. Some versions write `0x0D 0x00`; treat the trailing NUL as **NORMALIZE** (oracle-resolves whether the IV target writes it).

### 1.5 Field types — on-disk encoding

All values are **fixed-width, space-padded ASCII inside the record** (the defining property of `.dbf`: even numbers are ASCII digits), except IV-era binary types SAMIR does not target.

| Code | Type | Encoding | Width | Notes |
|---|---|---|---|---|
| **C** | Character | ASCII, **right-padded with `0x20`** | 1–254 (III+) | Trailing spaces not significant data — but see `SET EXACT` (§4.5). |
| **N** | Numeric | ASCII decimal, **right-justified, left-padded `0x20`**; optional `-`/`.`; overflow → `*` fill | ≤ 20 | `decimal count` (desc `0x11`) = places after `.`. Blank ≈ NULL. The salami "sub-cent remainder" (§6.3.2) is the value truncated/rounded to `decimal count`. |
| **D** | Date | **8 ASCII digits `YYYYMMDD`** (no separators) | 8 | Blank (`"        "`) = empty date. **4-digit year on disk** — so the 586.1 Y2K bug is an *application* 2-digit choice, not a property of the D field. |
| **L** | Logical | 1 ASCII char | 1 | `T t Y y` = true; `F f N n` = false; `?`/space = uninitialized. |
| **M** | Memo | block pointer into `.dbt` | 10 | III+/IV store a **10-byte right-justified, space-padded ASCII** block number; all-blank = no memo. (§3) |
| **F** | Float | same as `N` (ASCII) | ≤ 20 | dBASE IV only; storage identical to `N`. |

> **`C` width >254 (oracle-resolves).** III caps `C` at 254. IV (and many clones) encode `C` widths >255 by borrowing the **decimal-count byte (`0x11`) as the high byte** (effective width = `len + decimal*256`). SAMIR should **clamp `C` ≤254** for III+ safety; mint a golden if it ever needs wide chars under IV. Do not invent the rule.
>
> **NULL semantics.** Classic III+/IV has **no per-field NULL bitmap**; "empty" is in-band (spaces for C/N/D/M, `?`/space for L). The dBASE 7 `_NullFlags` pseudo-field is **OUT OF SCOPE**.

### 1.6 The record + the `0x1A` EOF marker

- **Record prefix:** 1 byte — `0x20` (space) = active, `0x2A` (`*`) = deleted. Fields follow packed sequentially, no separators, each in its fixed width.
- **EOF:** a single **`0x1A`** at the physical end. **Semi-optional** — dBASE writes it (III sometimes appends an extra one) but `record_count` is authoritative for record boundaries, not `0x1A`. **Oracle stance:** SAMIR **writes** `0x1A` (period-authentic) and **tolerates its absence on read**; record parsing depends on `record_count`, never on finding `0x1A`. Treat its presence/absence as **NORMALIZE** in round-trips.

---

## 2. The MEANINGFUL-vs-NORMALIZED byte contract (the round-trip core)

This is the operational heart of PRD §6.6's "meaningful bytes" clause and the contract the `initech-17n` oracle enforces. It belongs as **locked spec-data** (proposed `spec/dbf_normalization.json`, §8 bead) so the oracle consumes the mask as data, not prose (Rule 8).

**MEANINGFUL — must round-trip byte-exact:**
- Version byte `0x00` (low nibble + memo bit `0x80`).
- Record count `0x04–0x07`; header length `0x08–0x09`; record length `0x0A–0x0B` (derived, internally consistent — §1.1).
- Production-`.MDX` flag `0x1C` iff SAMIR emits `.MDX`.
- Each field descriptor's name (`0x00–0x0A`), type (`0x0B`), length (`0x10`), decimal count (`0x11`), and production-MDX field flag (`0x1F` iff `.MDX`).
- The `0x0D` header terminator.
- **All record bytes:** the delete flag (`0x20`/`0x2A`) + every field's space-padded ASCII content. This is the data — the whole point.
- `.dbt`: every memo's text content + its block pointer (modulo block-packing/tail normalization, §3).

**NORMALIZE — overwrite to a fixed sentinel before diffing:**
- **Last-update date `0x01–0x03`** — real dBASE stamps "today"; SAMIR must be deterministic (Rule 11) so it cannot match. Normalize both sides to `00 00 00` (also dodges the `YY+1900` 2026→1926 wrap).
- **Language-driver byte `0x1D`** — install-dependent; **preserve-on-rewrite, do not diff** (PRD §6.6 names it).
- **Reserved spans** — `0x0C–0x0D`, `0x0E`, `0x0F`, `0x10–0x1B`, `0x1E–0x1F` (the whole `0x0C–0x1F` on III); per-descriptor data-address `0x0C–0x0F`, work-area `0x14`, SET-FIELDS `0x17`, reserved `0x12–0x13`/`0x15–0x16`/`0x18–0x1E`; a trailing NUL after `0x0D`.
- **Trailing `0x1A`** presence/absence.
- **Deleted-record payload** — when the flag is `0x2A`, the field payload may legitimately be stale; diff the flag, optionally skip the payload (an oracle-policy decision to pin).
- **`.dbt` block tails** — bytes past the memo terminator / length-declared end within a 512-byte block are unused garbage → mask. Block-number *assignment* may differ between SAMIR and dBASE's allocator → compare memo **content keyed by record**, not raw block layout, unless testing the allocator specifically.

**Mechanical recipe (Law 2):** (1) parse both files into a canonical struct (header fields, field defs, records-as-typed-values, memos-as-strings); (2) diff at the **value level** for data; (3) **additionally** do a masked raw `memcmp` (zero the NORMALIZE ranges in both copies, compare) to catch structural drift the value-diff misses; (4) **bidirectional** — SAMIR-writes→dBASE-reads AND dBASE-writes→SAMIR-reads (PRD §6.6 "and vice versa"); (5) **mutation-prove** (Rule 6) — flip one constant (record-length off-by-one; `0x03` where `0x83` is required; the `.dbt` next-block endianness) and confirm RED.

---

## 3. The `.dbt` memo file

Triggered by version-byte bit 7 (`0x80`); same basename, `.dbt` extension. **Two incompatible block formats — III+ and IV — selected by the `.dbf` version byte.**

### 3.1 Block 0 = header block
- **Block size:** III+ = fixed **512 bytes**; IV = per-file (default 512), stored in block 0.
- **Offset 0–3:** next-free / next-available block number (`uint32`).
  - ⚠️ **Oracle-resolves endianness.** `cl-db3` reads this **big-endian**; clicketyclick + independent-software say **little-endian**. This is a real documented contradiction in the wild (some builds wrote it big-endian). **Mint from the reference and assert** — getting it wrong corrupts every memo append.
- **IV block-0 extras:** `0x08–0x0F` = `.dbf` base filename (NUL-padded, no ext); `0x14–0x15` = block length in bytes (`uint16` LE, per dBASE.NET issue #2). For III+, `0x04` onward is reserved/garbage; size implicitly 512.

### 3.2 Memo content blocks
- **dBASE III+ (`0x83`):** a memo starts at a block boundary and runs across consecutive blocks until **two consecutive `0x1A`** (`0x1A 0x1A`). No per-block length header; final-block tail is garbage → **normalize the tail**. (The independent-software page did not confirm the double-`0x1A` explicitly — verify against a real III+ golden `.dbt` when minting, an open item below.)
- **dBASE IV (`0x8B`):** each memo block begins with an **8-byte header** — `0x00–0x03` = signature `FF FF 08 00`, `0x04–0x07` = memo length *including* the 8-byte header (`uint32` LE) — text follows at byte 8. A `0x1A 0x1A` pair may still appear but the **length field is authoritative**; treat `0x1A 0x1A` as redundant.

### 3.3 The pointer in the `.dbf` record
III+ and IV both store the memo's starting block number in the M field as a **10-byte right-justified, space-padded ASCII** string (block 62 → `"        62"`); all-blanks = empty. (FoxPro/VFP store a 4-byte binary `uint32` — that is `.fpt`, OUT OF SCOPE.)

---

## 4. The `.ndx` / `.mdx` index formats

> **Sequencing headline (drives §8 phasing).** The **`.ndx` node layout is fully documented** and corroborated by three independent sources → it is the right, lowest-risk first target. The **`.mdx` header + tag table are documented but the per-block node internals are NOT** (Bachmann is sparse; the one source with detailed node tables had them for FoxPro **`.cdx`, not `.mdx`** — CDX ≠ MDX). MDX write/auto-maintenance is therefore **deferrable** per PRD §6.6 ("start with `.ndx`, defer `.mdx` production-index auto-maintenance"), and when pursued its node layout must be **reversed from a real-dBASE-IV golden**, never inferred (Law 1; CLAUDE.md stop-condition on inferring from intuition / CDX analogy).

### 4.1 `.ndx` (dBASE III+ single index) — header block 0 (512 bytes)

B-tree of fixed 512-byte pages; block byte offset = `page# * 512`; all integers little-endian. (clicketyclick.dk/.../ndx.html; corroborated NDbfReaderEx, piclist/hallikainen Listing 1.)

| Off | Len | Field |
|---|---|---|
| 0–3 | 4 | Starting (root) page number |
| 4–7 | 4 | Total number of pages (next-available/EOF block) |
| 8–11 | 4 | Reserved (**normalize**) |
| 12–13 | 2 | Key length (bytes) |
| 14–15 | 2 | Keys per page |
| 16–17 | 2 | Key type (0 = char; 1 = numeric) |
| 18–21 | 4 | Size of key record (group length): `4 (lower-page ptr) + 4 (dbf recno) + keylen`, padded to a multiple of 4 |
| 22 | 1 | Reserved |
| 23 | 1 | Unique flag (0/1) |
| 24–511 | 488 | Key-expression string (NUL-terminated xBase expression) |

> **Discrepancy (oracle-resolves).** piclist describes the key-expression field as **100 bytes, stored lowercase**; Bachmann gives the span 24–511 (488 bytes). Treat *capacity* as 488 but the *meaningful length* as the NUL-terminated string; let the round-trip oracle (real dBASE reopens it) decide casing/length. Likewise the **entry count** width: Bachmann says a 4-byte long, piclist says a 2-byte short + 2-byte pad (byte-compatible LE for counts < 65536) — pin via round-trip.

### 4.2 `.ndx` index page (node)
4-byte entry count @0; key-entry array @4. Each entry: **lower-page pointer** (`uint32`, `0` ⇒ leaf), **dbf record number** (`uint32`, meaningful at leaf), then **key data** (char = ASCII space-padded; numeric/date = 8-byte IEEE-754 double LE, dates as Julian). Search = classic B-tree descent: scan a node for the first key ≥ target, follow its lower-page pointer; terminate when the pointer is 0 (leaf), harvest the recno. **Assert the leaf/branch invariant on load and panic on violation** (Rule 2) — Bachmann does not state the N-keys/N+1-children placement explicitly, so confirm against a real `.ndx` dump.

### 4.3 `.mdx` (dBASE IV multi-index, up to 47–48 tags)
File header (0–543): version @0; creation date @1–3 (**normalize**); data filename @4–19; block size @20–21; block-size adder @22–23; production flag @24; tag-table entries @25; tag-descriptor length @26; tags-in-use @28–29; pages-in-tagfile @32–35; first-free-page @36–39; available-pages @40–43; **last-update date @44–46 (normalize)**; reserved/garbage @48–543 (**normalize**). Tag table @544: per-tag descriptor (typ. 32 bytes) — header page# @0–3; tag name @4–14; key format @15; forward/backward thread links @16–18; key type @20. Per-tag header page: root page @0–3; file size in pages @4–7; key format @8; key type @9; key length @12–13; max keys/page @14–15; key-item length @18–19; unique flag @23; the tag's key-expression string lives in/after this page. The `.dbf` byte `0x1C` = `0x01` flags a production `.mdx`.

**The documented gap (§4 headline):** MDX **node-body** internals are not byte-specified in any acquired source beyond "reuses the NDX block layout with a **single long per key entry** (child page# in interior nodes, dbf recno in leaves)" plus an undocumented record-number-mask scheme. Garbage-filled directory/header regions (dBASE does not null-clear) must join the round-trip normalization set. **Reverse the node layout from a real dBASE IV `.mdx` golden before trusting it.**

### 4.4 What's required vs deferrable for interoperable round-trips
- **Required (byte-correct, navigation-bearing):** `.ndx` root/total-pages/keylen/keys-per-page/key-type/group-size/unique/key-expr; every node's entry count + each entry's lower-page-ptr/recno/key; correct B-tree ordering so descent terminates. For `.mdx`: header version/block-size/production-flag/tag counts/page counts/free-page ptr; each tag descriptor + tag header; the node bodies.
- **Normalize:** `.mdx` creation/last-update dates + all reserved/garbage; `.ndx` reserved @8–11.
- **MDX is behavioral, not byte-identical, where allocation policy legitimately varies.** Free-space layout, page-allocation order, and split policy may differ between SAMIR and dBASE while both remain valid B-trees. **Prefer byte-identity where deterministic (`.ndx`); fall back to behavioral round-trip for `.mdx`** — real dBASE `USE`s the table, `SET ORDER TO <tag>`, and a `LIST`/`SEEK` sweep returns the exact order/positions, and vice versa. Strengthen, never weaken, the oracle (Law 2; PRD stop-condition).

### 4.5 Index maintenance, keys, collation, SEEK
- **`.ndx`** is **not** auto-maintained unless explicitly open (`SET INDEX TO`); a record edit while a relevant `.ndx` is closed corrupts it → `REINDEX`. **`.mdx` production** is **auto-maintained** — opened with `USE`, all tags updated on every `APPEND`/`REPLACE`/`DELETE`. This auto-maintenance is the chief functional cost and the reason to defer.
- **Index keys are fixed-length per-record xBase expressions** — `INDEX ON UPPER(LASTNAME+FIRSTNAME) TAG name`; mixed types coerced via `DTOS()` (date→sortable `YYYYMMDD`), `STR()` (num→string), `UPPER()` (case-fold). Hard constraint: the key must be **fixed-length per record** — this couples the index workstream tightly to the **expression evaluator + coercion table** (§5).
- **Collation:** default is byte-wise **ASCII / CP437** on the stored space-padded key bytes (matching III+/IV default). Numeric keys sort as IEEE doubles; dates as Julian. Pin CP437 in the coercion spec.
- **SEEK** searches the master/controlling index; `SET EXACT OFF` (default) → prefix match (`SEEK 'S'` finds "Sanders"); `SET ORDER TO n`/`TAG <name>` selects the master order; `SET ORDER TO 0` reverts to physical order. `FOUND()`/`EOF()`/`RECNO()` are the differential-observable outputs.

---

## 5. The xBase dot-prompt subset, the evaluator, and the coercion table

### 5.1 The dot-prompt command subset (PRD §6.6)

Each verb, its runtime effect, and the file/record ops it drives. Sources: dbase.com command reference, piclist/xsharp, the dBASE III+ tutorial.

- **`USE <table> [INDEX …] [ORDER [TAG] …] [ALIAS …]`** — opens a `.dbf` + its `.dbt` + production `.mdx` (auto) in a work area; `ORDER` sets the master index (pointer positioned per its sort order); bare `USE` closes the work area. Drives: open `.dbf` header+records, `.dbt`, `.mdx`/`.ndx`; set master order; `RECNO()=1`.
- **`APPEND` / `APPEND BLANK` / `APPEND FROM`** — `APPEND BLANK` (the programmatic form) adds one empty record (type-default empties), bumps the header record count, updates all open indexes, leaves the pointer on the new record. Canonical idiom: `APPEND BLANK` then `REPLACE … WITH …`.
- **`LIST [<scope>] [FIELDS …] [FOR …] [WHILE …] [OFF]`** — default scope `ALL`; iterates in current order (master-index order if active, else physical), evaluates each field/expression, prints to console; moves the pointer to EOF.
- **`LOCATE FOR <cond> [<scope>] [WHILE …]` / `CONTINUE`** — `LOCATE` searches **sequentially** (not via index) for the first record where `<cond>` is true; sets `FOUND()`. **`CONTINUE` re-applies only the FOR condition, NOT the scope/WHILE of the original LOCATE** (a real gotcha for the corpus); `CONTINUE` with no prior `LOCATE` in the work area is an error.
- **`SEEK <expr>`** — indexed exact-key search of the master index; governed by `SET EXACT` (§5.5); `SET NEAR ON/OFF` decides the miss position and `EOF()`. The `<expr>` type/functions must match the index key expression.
- **`SET INDEX TO …` / `SET ORDER TO n|TAG <name>`** — opens `.ndx`/`.mdx`; selects the master order; `SET ORDER TO 0` reverts to physical order keeping indexes open. dBASE IV limit: 10 `.ndx` + 47 `.mdx` tags.
- **`DO WHILE <lcond> … [LOOP] [EXIT] … ENDDO`** — control flow; the canonical record walk is `DO WHILE .NOT. EOF() … SKIP … ENDDO`. The guard is a coercion-table consumer (must reduce to Logical).
- **`IF <lcond> … [ELSE …] ENDIF`** — guard must be Logical; **dBASE does NOT auto-coerce N/C/D to Logical** (a non-Logical guard is a runtime error, unlike C). `IIF(c,a,b)` is the inline form.
- **`REPLACE <field> WITH <expr> [, …] [<scope>] [FOR …] [WHILE …]`** — **default scope = current record only**; overwrites field(s) with the evaluated expression, which **must share the field's type** (the **assignment-coercion** axis, distinct from operator coercion); auto-updates all open indexes; **replacing a master-key field across records can drift/skip the pointer** as keys re-sort (flag for the corpus). Over-long Character truncates; Numeric overflow → `*` fill.
- **`@ row,col SAY <expr> [PICTURE …] [FUNCTION …]` / `@ row,col [SAY …] GET <var|field> [PICTURE …] [RANGE …] [VALID <lc>] [WHEN <lc>]` / `READ`** — `SAY` displays formatted; `GET` binds an editable field/var with validation; `READ` activates all pending GETs (full-screen edit), writing values back on exit. Drives CP437 text rendering in the 80×25 window, the keyboard input loop, and `.dbf` field writes on `READ`. The InitechCalc `570-` trailing-minus is the SAY `PICTURE` negative-format analogue (Law 4 canon).

### 5.2 The four base types and operator classes

The only types in scope are the **four base types**: **Character (C)**, **Numeric (N)**, **Date (D)**, **Logical (L)**. (`.dbf` Memo → Character in expressions; dBASE IV Float → Numeric — "no conversion needed between these two." OOP types Object/Array/null/funcptr are **PLUS-era, EXCLUDED**.) Four operator classes: mathematical (`+ - * / ** ^ %`), relational (`= == <> # != < > <= >= $`), logical (`.AND. .OR. .NOT.`), string (`+ -`).

### 5.3 Arithmetic / string `+` and `-` — the crux of mixed-type behavior

- **`+`:** `N+N→N`; `C+C→C` (concat, trailing blanks kept in place); `D+N→D` and `N+D→D` (shift by N days; blank date stays blank). **`C+N` in III+/IV is a data-type-mismatch ERROR** — the modern dbase.com `+` page documents auto-stringification that **did NOT exist in III+/IV**; encode the period behavior (error), proven by fuzzing the real interpreter. This is the single biggest hazard in the brief.
- **`-`:** `N-N→N`; `D-N→D`; **`D-D→N`** (number of days between; `date - blankdate = 0`); `C-C→C` with **trailing-blank relocation** (`"AB   " - "CD"` → `"ABCD   "` — same length, gap closed, blanks pushed to the tail). Unary `-` negates Numeric.
- **`* / ** ^ %`:** Numeric only → Numeric (div-by-zero/overflow → error/condition).

### 5.4 Relational / logical operators (result always Logical)

- **`< > <= >=`** — Numeric/Character/Date (and Logical: false < true); **both operands must be the same type** (cross-type is a mismatch error). Character order = collation (CP437 byte order for the period build). **Date special case: a blank date is GREATER THAN any non-blank date.**
- **`=`** — all four types; for Character, governed by `SET EXACT` (§5.5). For N/D/L it is ordinary equality.
- **`==`** — **always** behaves as `SET EXACT ON` (exact, trailing blanks ignored) regardless of the switch; dBASE IV feature. For non-Character types behaves as `=`.
- **`<> # !=`** — mirror `=`, so they **follow `SET EXACT`** for Character ("does not begin with" when EXACT OFF).
- **`$`** — substring containment (`c1 $ c2` → true iff `c1` is a substring of `c2`); Character only; **case-sensitive, trailing blanks significant, UNAFFECTED by `SET EXACT`**; documented edge: `"" $ "ABC"` → **.F.** (empty string is *not* contained — fuzz it).
- **`.AND. .OR. .NOT.`** — operate on two Logical values, return Logical; **no truthiness coercion** from N/C/D (a non-Logical operand is an error). Precedence: `.NOT.` > `.AND.` > `.OR.`. Period syntax requires the dots and `.T.`/`.F.`/`.Y.`/`.N.`.

### 5.5 `SET EXACT` semantics (the string-comparison switch) — definitive

- **`SET EXACT OFF` (default):** `=` is a **"begins with"** test for Character — the **LEFT operand must begin with the RIGHT** (the pattern); comparison runs to the length of the RIGHT operand; trailing blanks ignored. `"Smith" = "S"` → **.T.**; `"S" = "Smith"` → **.F.** (non-commutative / directional). Governs `SEEK` and index partial match.
- **`SET EXACT ON`:** `=` requires exact match **except trailing blanks are ignored** — `"Smith" = "Smith "` → **.T.** (trailing-blank tolerance even in EXACT mode).
- **`==`** ignores the switch (always EXACT-ON for Character); **`<>`/`#`** follow it.

**Weird-case summary the fuzzer MUST hit:** (1) `=` non-commutative for Character (EXACT OFF, begins-with is directional); (2) trailing blanks ignored even EXACT ON; (3) `==` order-independent of the switch; (4) `$` unaffected by EXACT, directional/case-sensitive, empty→.F.; (5) `D-D→N` but `D-N→D`; (6) blank-date "greater than" any real date and `date - blankD = 0`; (7) **`C+N` is an ERROR in III+/IV** (not stringification — reject the PLUS doc).

### 5.6 Proposed `spec/xbase_coercion.json` schema (locked data — `initech-gmo`)

Per PRD §6.6 ("capture it as a data table and fuzz the evaluator against the real interpreter"), §9/§14, and the house JSON conventions (`_comment` provenance block, `status`/ratification line, machine-consumable rows; cf. `spec/int21h_calling_convention.json`). **Two distinct coercion axes** must be captured separately (conflating them is the trap): **operator coercion** `(lhs_type, op, rhs_type) -> result|error|rule_id` and **assignment coercion** `(field_type, expr_type) -> ok|truncate|overflow|error` (drives `REPLACE … WITH`, `@…GET`, `STORE`). Snippet (ASCII-clean per Rule 12 — code stays ASCII even though the doc may use Unicode):

```json
{
  "_comment": [
    "InitechBase (SAMIR) xBase type-coercion table -- LOCKED spec-data (CLAUDE.md Rule 8).",
    "Scope: dBASE III+/IV FOUR base types only (C/N/D/L). Memo->Character in exprs;",
    "dBASE IV Float->Numeric. OOP types (Object/Array/null/funcptr) are PLUS-era; EXCLUDED.",
    "Source of truth at lock time is the REAL period interpreter under DOSBox-X/86Box",
    "(PRD 6.6), fuzzed via harness/diff/. Where dbase.com (PLUS) and the dBASE IV",
    "Language Reference DISAGREE (notably C+N), the III+/IV behavior wins.",
    "Refs: PRD 6.6; dbase.com SET EXACT + Comparison ops + +/- operator pages;",
    "  dBASE IV Language Reference (data types, same-type relational rule).",
    "Bead: initech-gmo (deps initech-aul; blocks initech-17n)."
  ],
  "status": "DRAFT -- pending fuzz-confirmation against real dBASE IV (bead initech-gmo)",
  "schema_version": 1,
  "types": ["C", "N", "D", "L"],
  "modes": {
    "SET_EXACT": { "values": ["OFF","ON"], "default": "OFF",
                   "affects": ["=","<>","#","SEEK"],
                   "note": "== ignores this; $ ignores this" },
    "SET_NEAR":  { "values": ["OFF","ON"], "default": "OFF", "affects": ["SEEK"] },
    "collation": { "value": "ASCII", "note": "CP437 byte order for the period build" }
  },
  "rules": {
    "R_begins_with":   "Character =: LEFT must begin with RIGHT; directional; trailing blanks ignored (EXACT OFF).",
    "R_exact_blankpad":"Character ==/= under EXACT ON: equal except trailing blanks ignored.",
    "R_date_plus_num": "D +/- N -> D (shift by N days); blank date stays blank.",
    "R_date_minus_date":"D - D -> N (#days between); (date - blankdate) = 0.",
    "R_concat_plus":   "C + C -> C; trailing blanks kept in place.",
    "R_concat_minus":  "C - C -> C; trailing blanks of LHS moved to end (length preserved).",
    "R_substr":        "c1 $ c2 -> L; literal substring, case-sensitive, blanks significant; empty -> .F.",
    "R_same_type_rel": "<,>,<=,>= require operands of the SAME base type; else ERROR.",
    "R_blankdate_high":"In ordering, a blank date is GREATER THAN any non-blank date.",
    "R_no_truthiness": ".AND./.OR./.NOT. and IF/DO WHILE guards require L; non-L -> ERROR."
  },
  "operator_coercion": [
    { "lhs": "N", "op": "+",  "rhs": "N", "result": "N" },
    { "lhs": "C", "op": "+",  "rhs": "C", "result": "C", "rule": "R_concat_plus" },
    { "lhs": "D", "op": "+",  "rhs": "N", "result": "D", "rule": "R_date_plus_num" },
    { "lhs": "C", "op": "+",  "rhs": "N", "result": "error",
      "note": "III+/IV mismatch; PLUS auto-stringifies -- EXCLUDED" },
    { "lhs": "D", "op": "-",  "rhs": "D", "result": "N", "rule": "R_date_minus_date" },
    { "lhs": "C", "op": "-",  "rhs": "C", "result": "C", "rule": "R_concat_minus" },
    { "lhs": "C", "op": "=",  "rhs": "C", "result": "L",
      "exact_off": "R_begins_with", "exact_on": "R_exact_blankpad" },
    { "lhs": "C", "op": "==", "rhs": "C", "result": "L", "rule": "R_exact_blankpad",
      "note": "ignores SET EXACT (dBASE IV)" },
    { "lhs": "C", "op": "$",  "rhs": "C", "result": "L", "rule": "R_substr" },
    { "lhs": "D", "op": "<",  "rhs": "D", "result": "L", "rule": "R_blankdate_high" },
    { "lhs": "N", "op": "<",  "rhs": "C", "result": "error", "rule": "R_same_type_rel" },
    { "lhs": "L", "op": ".AND.", "rhs": "L", "result": "L" }
  ],
  "assignment_coercion": [
    { "field": "C", "expr": "C", "result": "ok", "on_too_long": "truncate" },
    { "field": "N", "expr": "N", "result": "ok", "on_overflow": "overflow_stars" },
    { "field": "C", "expr": "N", "result": "error", "note": "no implicit; use STR()" },
    { "field": "N", "expr": "C", "result": "error", "note": "no implicit; use VAL()" },
    { "field": "D", "expr": "C", "result": "error", "note": "no implicit; use CTOD()" }
  ]
}
```

**C-evaluator note:** dispatch is a `switch` on `(lhs_type, op, rhs_type)` returning `result_type`/`error`; the two `SET EXACT`-dependent Character `=` cases pick `exact_off`/`exact_on` at eval time from the live mode. **`==` is dBASE IV only** — gate by version if any corpus targets III+; M6 names IV, so `==` is in scope. The `@…GET` `PICTURE`/`FUNCTION`/`VALID`/`RANGE` template language is a separate mini-spec; if M6 forms need full picture fidelity, lock it as a distinct sub-table (proposed bead, §8).

---

## 6. Runtime model + the deadpan canon

### 6.1 The runtime model and its M2/M4 deps

SAMIR runs as a **C character-mode xBase interpreter** in an **80×25 text-console window** (CP437 / Turbo-Vision) hosted in the FLAIR desktop (PRD §6.6; §6.1 dep). Files reach the FAT volume through MILTON's **handle API** (INT 21h 3Ch/3Dh/3Eh/3Fh/40h/42h), not FCB (`initech-509.9` operator note). It is **cooperative, not preemptive** (CLAUDE.md callout): the interpreter is a guest in the `WaitNextEvent`-style loop; a `DO WHILE` that never yields hangs the desktop — authentic, not a bug. Two hard deps (`bd show initech-586`): **`initech-509` (M2)** for file I/O — substantially landed (FAT12/16 read+write, INT 21h handle surface; in-kernel FAT16 mount tracked as `initech-d27i`) — and **`initech-8oi` (M4)** for the text-console window (the `initech-ax9` gate surface). The **SAMIR↔FLAIR seam** (CP437 glyph buffer, 80×25 cell grid, keyboard event routing from the cooperative loop, and the GUI-exit path the TPS Report Generator also needs) is a real integration bead today implicit in `ax9` (proposed split, §8).

It is **oracle-tractable** because real dBASE III+/IV under DOSBox-X / 86Box mints goldens and runs differential programs (PRD §8, line 192: 86Box "runs real DOS 3.3 + real dBASE to mint golden files").

### 6.2 Canon stance

The canon is **played completely straight — never flagged as a joke** (HANDOFF §2 "the recursive joke"; CLAUDE.md Law 4 "the canonical bugs are canon … enforced, not fixed"). Each artifact is a *real, working* program, and its bug is *load-bearing canon*, like the 116% pie chart and the hourglass-not-wristwatch cursor. The four artifacts form a stack rooted on the `.dbf` store.

### 6.3 The four canon artifacts

**6.3.1 `initech-586.1` — Initech accounting system + canonical Y2K bug (P3, child of 586).**
A real, working accounting/payroll application built on InitechBase. Ships a **deliberate, period-authentic Year-2000 bug: 2-digit year storage + comparison**, so dates roll over incorrectly at 2000 (`00 < 99`). **Mechanizable and application-level:** the dBASE D field stores a **4-digit `YYYYMMDD`** on disk (§1.5), so the bug is the *Initech app's* choice to store/compare 2-digit years (e.g. a `C(2)` year field or a `YY` substring comparison), authentically reproducing how period dBASE applications actually failed. Oracle: a differential xBase program asserting a 1999-vs-2000 comparison sorts/compares wrong, reproducible against real dBASE running the same source. `BLOCKS 586.2`.

**6.3.2 `initech-586.2` — Michael Bolton's salami-slicing virus + canonical rounding-error bug (P3, child of 586).**
The *Office Space* plot device, built for real and played straight: a program that **skims the sub-cent rounding remainder from each transaction** in the Initech accounting app and accumulates it, carrying the **canonical rounding-error bug** that makes it skim far too much, far too fast (the movie's bug). **Mechanizable:** dBASE Numeric fields are ASCII-stored with a fixed `decimal count` (§1.5), so "sub-cent remainder" is precise — the difference between the unrounded computation and the value written back at the field's `decimal count`. Must actually work against the in-universe accounting software inside the emulated OS; a self-contained homage targeting **ONLY** the project's own fictional accounting app. `DEPENDS ON 586.1`; `RELATED initech-bf2` (the FILE COPY "Saving tables to disk…" dialog + `michael_bolton.conf` rate).

**6.3.3 `initech-8479.1` — TPS Report Generator (P4, child of `initech-8479`, NOT 586).**
A real, working CP/M-heritage program that uses **ONLY the legacy FCB calls (INT 21h 0Fh–24h)** — no handle API. The flagship use case for vestigial features and the canonical "feature that looks like a joke but actually works." It **MUST be run from the DOS command prompt — the user must EXIT the FLAIR GUI to run it** (authentic corporate tedium, operator 2026-06-14) and **MUST genuinely require the FCB API**. **Boundary note for synthesis:** 8479.1 sits in **M2's external-utilities epic, not M6/SAMIR**. Its relation to SAMIR is *thematic and contrastive* — where SAMIR is the modern handle-API app *inside* the GUI, the TPS Report Generator is the vestigial FCB-API app you must *exit the GUI* to run. It could plausibly report over the same Initech accounting `.dbf`, but its defining mechanic is the **FCB round-trip + the GUI-exit**, not the SAMIR engine. **Do not re-file it under 586.** `DEPENDS ON initech-509.9`.

**6.3.4 `initech-509.9` — FCB legacy file API 0Fh–24h (P4, vestigial, child of 509).**
The CP/M-derived FCB API (open/close/find-first/find-next/read/write/delete). Stays **REQUIRED** (design stance: the blandest corporate OS keeps every vestigial structure, **implemented in full** — "corporate software never deletes") but **BACKBURNERED** behind kernel-complete + utils + GUI/dBASE; does not block the kernel feature-complete cert (`initech-40oq`). The **thematic core of the recursive joke** ("vestigial dead code that actually works"). `BLOCKS 8479.1`. Acceptance: FCB round-trip + differential vs reference DOS FCB behavior.

---

## 7. The oracle architecture

Three oracles, faithful to the existing FAT differential pattern (CLAUDE.md Law 2; `harness/README.md`, which already names `dbf_diff` "vs real dBASE"). Mirror `harness/diff/fat_diff/` (the proven structure: a host **dumper** that mounts with the *real artifact reader* and emits a deterministic normalized view; an **independent reference reader** = "the third implementation"; a **fuzzer** with a deterministic PRNG, an independent model, three-way agreement, and a **shrinker**; a Makefile gate that **fails loud, never skips** — "a skipped oracle is worse than a red one"; **mutation variants** of the real source asserted RED).

### 7.1 Proposed layout (mirroring `fat_diff/`)
```
harness/diff/dbf_diff/
  blockdev_file.{c,h}    reuse/share the fat_diff host block backend
  dbf_dump.c             ARTIFACT dumper: mounts a .dbf/.dbt with the REAL os/samir reader,
                         emits normalized --schema / --records / --memo / --index manifests
  dbf_ref.py             INDEPENDENT python reader (the "third implementation"): parses
                         dbf/dbt AND ndx/mdx from first principles (Sec 1-4); deterministic, normalized
  dbf_coerce_fuzz.c      C property-test coercion fuzzer (expr generator + shrinker)
  xbase_prog_diff/       shared xBase corpus + driver (differential program output)
  fixtures/              committed deterministic .prg programs + seed schemas
spec/xbase_coercion.json LOCKED coercion table (the contract the evaluator + fuzzer share)
spec/dbf_normalization.json LOCKED meaningful-vs-normalize byte map (Sec 2)
```

### 7.2 Host reference tooling (probed; prefer apt over pip — `externally-managed-environment` blocks pip)
- **`python3-dbf`** (Ethan Furman, apt): **READ + WRITE + memos**, dBASE III/Clipper/FoxPro/VFP — the day-1 reference for **`.dbf`+`.dbt` both directions**. **"Not supported: index files"** → cannot read/write `.ndx`/`.mdx`.
- **`python3-dbfread`** (Ole Martin Bjørndalen, apt): **READ-only** — the **independent second reader** (catches a bug shared by our writer + Furman's reader); recognizes version bytes incl. `0x03`/`0x83`/`0x8B`/`0x43`/`0xF5`.
- **Indexes have NO host library** → author the independent `.ndx`/`.mdx` reader in `dbf_ref.py` (grounded in §4) as the day-1 operator-free gate, and use real dBASE under DOSBox-X (apt) / 86Box (operator-supplied) as the authenticity-tier authority.

### 7.3 The three oracles
- **(a) Round-trip — `test-dbase-roundtrip`.** Direction 1: SAMIR writes `STAPLER.DBF` (+`.DBT`/+`.NDX`/`.MDX`); reference readers A (`python3-dbf`) and B (`dbfread`) read back typed values; the **index leg** uses `dbf_ref.py --index-dump` (every active record once in the leaf chain, keys sorted under the tag collation, `SEEK(k)` resolves to the same recno). Direction 2: `python3-dbf` writes a `.dbf`+`.dbt`; SAMIR reads it via `dbf_dump.c --records`; assert value equality; index produced by real dBASE → SAMIR `SET INDEX`/`SEEK` (authenticity tier). **Meaningful-byte normalization (§2) applied before any header byte-diff;** prefer value-level diffing for data, masked `memcmp` for structure.
- **(b) Differential program output — `test-dbase-diff`.** Mirror `compiler_diff`. A shared-subset xBase corpus (committed `.prg` over the §6.6 verbs) run in SAMIR and in the reference; **two tiers** — day-1 host tier = a small from-scratch reference interpreter for the dot-prompt subset, grounded in the locked coercion table (keeps the gate operator-free + deterministic); authenticity tier = **real dBASE III+/IV under DOSBox-X / 86Box** to mint goldens. Diff normalized stdout + the normalized result `.dbf`. **Gate 100%** (PRD §8). Mutation-prove: flip the `=` prefix direction and confirm the corpus goes RED.
- **(c) Coercion fuzzer — `test-xbase-coercion`.** A plain C property test (PRD §14; the `test_region.c`/`fat12_fuzz.c` idiom): seeded PRNG → random typed literals + operators `{= == <> < > <= >= $ + -}` under `{SET EXACT ON/OFF}`; evaluate in our engine, diff against the reference evaluator driven by `spec/xbase_coercion.json`; on divergence, **shrink** to a minimal expression and print a replayable seed. The locked table is the contract (fuzzer proves the engine matches it); a periodic real-dBASE sweep proves the *table* matches dBASE (do not infer from intuition). Mutation-prove the table itself: flip one cell (make `C+N` succeed, or `=` commutative), confirm RED.

### 7.4 Make targets (mirroring `test-fat`)
`test-dbase` (umbrella M6 gate) → `test-dbase-roundtrip` / `test-dbase-diff` / `test-xbase-coercion`, plus per-Rule-6 mutant gates (`test-dbase-roundtrip-mutant`, `test-xbase-coercion-mutant` — one perturbed constant each, assert RED). Tool-presence guards in each recipe (`python3 -c 'import dbf'` / `import dbfread`, `command -v dosbox-x`), failing loud where an operator image is genuinely required.

---

## 8. Phased build plan → proposed beads

The existing 7-bead engine chain (`aul → 7az/ahu/gmo → 0tl/17n → ax9`) is well-formed but **coarse-grained**; several independently-oracle-able phases are folded into single beads. The proposals below sit **around** the existing beads (do not duplicate them) and are PROPOSALS ONLY — the orchestrator dedupes against existing beads and files them. **Two structural recommendations first:** (R1) re-parent the 7 engine beads as explicit children of `initech-586` so the epic's child tree (today "0/2") reflects reality; (R2) wire `586.1 DEPENDS ON 7az+aul` and `586.2 DEPENDS ON 586.1+7az` so `bd ready` cannot surface the canon apps before the engine exists.

Phase ordering: `.dbf`/`.dbt` (aul + memo split) → expression evaluator → `.ndx` → coercion lock (gmo) → interpreter (7az) → `@SAY/GET` (0tl) → `.mdx` → oracle infra + corpus → 17n → canon apps → ax9 gate. The detailed proposed beads are enumerated in the structured output (`proposedBeads`).

**Phases already covered (do NOT propose):** `.dbf` reader/writer (aul), dot-prompt interpreter core (7az), `@SAY/GET/READ` forms (0tl), coercion-table lock (gmo), the differential oracle proper (17n), the text-console-window host gate (ax9).

---

## 9. Open questions for the operator

1. **Real dBASE image for the authenticity tier.** 86Box is not in apt; the strongest interoperability claim — and the only authoritative `.ndx`/`.mdx` *writer* — needs a licensed/abandonware dBASE III+/IV binary under DOSBox-X (apt) or 86Box. Decide: acquire a dBASE image, and whether 86Box authenticity is required at M6 green or whether DOSBox-X + the python refs + our independent index reader suffice for the gate (Risk Register: "Reference software acquisition: Low — abandonware/DOSBox; document").
2. **`.ndx` vs `.mdx` as the M6 primary.** PRD §6.6/§11-M6 name `.mdx` ("dBASE IV"); the scope line lists both. Recommendation: `.ndx` first (fully documented, deterministic), `.mdx` second/behavioral. Confirm.
3. **Differential reference authority for (b).** Is the day-1 host-side reference interpreter acceptable as the *gate* reference (with real dBASE as periodic authenticity check), or does M6 green require real dBASE in the loop from the start?
4. **dBASE IV version byte SAMIR emits** (`0x8B`/`0x04`/`0x03`); the `.dbt` next-block-pointer **endianness**; the US III+ **language-driver byte**; whether the target writes a **trailing NUL after `0x0D`**; the **`C` width >254** rule — all five are "mint a golden, don't guess" (§1, §3).
5. **`spec/xbase_coercion.json` ratification:** lock against the **dBASE IV Language Reference** (not the modern PLUS doc) and fuzz-confirm `C+N`=error before ratifying. Pin CP437 collation. Does the `@…GET` PICTURE/FUNCTION/VALID/RANGE template need its own locked sub-table for M6?

---

## 10. Risk register

| Risk | Severity | Mitigation |
|---|---|---|
| **`C+N` PLUS-doc divergence** — the authoritative-looking dbase.com `+` page documents auto-stringification absent in III+/IV. | High (silent miscoercion poisons every program diff) | Encode period behavior (error); fuzz the real interpreter; lock from the dBASE IV Language Reference; mutation-prove the cell. |
| **`.mdx` node internals undocumented** — no acquired source gives the byte layout; CDX ≠ MDX. | High (corrupt index, false round-trips) | Defer `.mdx` write/auto-maintenance; reverse the node layout from a real dBASE IV golden; prefer behavioral round-trip for MDX (Law 2). |
| **`.dbt` next-block-pointer endianness contradiction** (big- vs little-endian across sources). | Medium (corrupts every memo append) | Oracle-resolves: mint from real dBASE and assert; mutation-prove. |
| **Reproducible-build vs last-update date** — SAMIR cannot match dBASE's "today" stamp. | Medium (false round-trip RED) | Normalize `0x01–0x03` both sides (§2); never let data diff depend on it (Rule 11). |
| **No host index library** — `.ndx`/`.mdx` round-trip has no `mtools`-equivalent. | Medium (gate gap) | Author the independent `dbf_ref.py` index reader (the "third implementation"); real dBASE as authenticity authority. |
| **86Box / real dBASE acquisition** for authenticity-tier golden minting. | Low | Abandonware under DOSBox-X (apt) for the day-1 gate; 86Box deferred/operator-funded. |
| **Coarse engine beads hide phases** (evaluator, `.dbt`, `.ndx`/`.mdx` split, SAMIR↔FLAIR seam folded in). | Low | The §8 phase split + R1/R2 re-parenting/dep wiring. |
| **`.dbt` `0x1A 0x1A` III+ terminator unconfirmed** by the fetched page. | Low | Verify against a real III+ golden `.dbt` when minting; assert. |
| **Canon-bug regression** — a well-meaning agent "fixes" the Y2K/rounding bug. | Low | Bugs are enforced canon (Law 4); the differential oracle asserts the *wrong* answer matches real dBASE; label `canon`. |

---

## Sources

**Local (Law 1):** `InitechOS-PRD.md` §6.6 (145–152), §8 (186–206), §11-M6 (239), §14 (270–278); `CLAUDE.md` (Laws 1–4, Rules 6/8/11/12, "two compilers"/"meaningful bytes"/"xBase weirdly typed" callouts, TDD shapes, file map); `docs/HANDOFF.md` (§2 recursive joke, §4 M2 substrate, dep wall); `harness/README.md` + `harness/diff/fat_diff/{fat_dump.c,fat12_ref.py,fat12_fuzz.c}` (the FAT differential pattern); `spec/README.md` + `spec/int21h_calling_convention.json` (house JSON conventions); `spec/dos_structs.h`; beads `initech-586/586.1/586.2/8479.1/509.9/aul/7az/ahu/gmo/0tl/17n/ax9`, `initech-d27i` (in-kernel FAT16), `initech-bf2` (FILE COPY).

**External (period / reverse-engineered):**
- Erik Bachmann / clicketyclick.dk — DBF https://www.clicketyclick.dk/databases/xbase/format/dbf.html · data types https://www.clicketyclick.dk/databases/xbase/format/data_types.html · DBT https://www.clicketyclick.dk/databases/xbase/format/dbt.html · NDX https://www.clicketyclick.dk/databases/xbase/format/ndx.html · MDX https://www.clicketyclick.dk/databases/xbase/format/mdx.html (mirror http://www.manmrk.net/tutorials/database/xbase/dbf.html)
- Independent Software — DBF/DBT/FPT http://independent-software.com/dbase-dbf-dbt-file-format.html
- Corion dBASE III (fileformat.info) https://www.fileformat.info/format/corion-dbase-iii.htm
- dBASE Inc. KB — `.DBF` structure (dBASE 7 / 48-byte descriptor; header cross-check only) https://www.dbase.com/Knowledgebase/INT/db7_file_fmt.htm
- Embarcadero — dBASE .DBF structure (byte 0x1C production-MDX flag) https://blogs.embarcadero.com/dbase-dbf-file-structure/
- Working code: cl-db3 https://github.com/dimitri/cl-db3/blob/master/db3.lisp · dBASE.NET issue #2 (IV `.dbt`) https://github.com/henck/dBASE.NET/issues/2 · NDbfReaderEx xbase.txt https://github.com/emelhu/NDbfReaderEx/blob/master/Doc/xbase.txt
- Language driver / code page — http://www.dbase.com/help/Language_issues/IDH_INTLSTUF_DRIVERID.htm · https://blog.codetitans.pl/post/dbf-and-language-code-page/
- Index — piclist/hallikainen NDX http://www.piclist.com/techref/language/dbase/ndxs.htm · DBFree about-indexes https://dbfree.org/webdocs/1-documentation/a-about_indexes.htm · MS ODBC dBASE indexes https://learn.microsoft.com/en-us/sql/odbc/microsoft/dbase-indexes
- Language reference — SET EXACT http://www.dbase.com/help/9_4/Xbase/IDH_XBASE_SET_EXACT.htm · Comparison operators http://www.dbase.com/help/Operators_and_Symbols/IDH_OPS_COMPARISON.htm · `+` https://www.dbase.com/help/2_62/Operators_and_Symbols/IDH_OPS_PLUS.htm · `-` https://www.dbase.com/help/2_62/Operators_and_Symbols/IDH_OPS_MINUS.htm · USE/SEEK/SET INDEX/CONTINUE/FOUND/REPLACE (dbase.com `/help/Xbase/`) · LOCATE https://www.xsharp.eu/help/command_locate.html · Data Types (dBASE IV) https://1library.net/article/data-types-dbase-iv-language-reference.zg257n7y · dBASE IV Language Reference https://www.scribd.com/doc/117872936/dBase-IV-Language-Reference · dBASE Language Handbook https://www.terrellamedia.com/wp-content/uploads/2022/01/dBASE-Language-Handbook-by-David-M-Kalman-Final.pdf
- Host reference libs — Furman `dbf` https://pypi.org/project/dbf/ · `dbfread` https://dbfread.readthedocs.io/en/latest/ (versions https://raw.githubusercontent.com/olemb/dbfread/master/dbfread/dbversions.py)
- Wikipedia `.dbf` (version-byte cross-check) https://en.wikipedia.org/wiki/.dbf

---

*— End of Brief —*

<!-- Tedium certified compliant with NFR-7. The pie chart sums to 116% on purpose; -->
<!-- the Y2K bug and the rounding-error virus are enforced canon, not defects. If -->
<!-- you have received this brief in error, please shred it and notify the Help Desk (ext. 2504). -->
