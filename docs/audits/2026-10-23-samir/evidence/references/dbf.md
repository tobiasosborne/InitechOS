# DBF — dBASE III PLUS 1.1 table file (.dbf)

**Status / confidence:** Byte-validated against six III+ 1.1 golden fixtures
(BANK, CLIENTS, TOURS, TRAVEL, TAX, UNIVERSD). The 32-byte header, the 32-byte
field-descriptor array, the `0x0D` terminator, the C/N/D/L/M on-disk encodings,
the delete flag, and both structural invariants are all confirmed against real
bytes. Two genuine findings the documents miss are flagged loudly below:
(1) **two header-length conventions coexist** in real III+ 1.1 output
(`+1` vs `+2` past the terminator), and (2) **the `0x1A` EOF byte is optional**
(present in some III+ files, absent in others). These are recorded as Open
questions for the binary to settle.

TARGET = dBASE III PLUS 1.1 (1986). dBASE IV / dBASE 7 facts appear only as
clearly-labelled contrast. This file describes the .dbf table only; the memo
(.dbt) format is a separate spec — the memo *pointer* encoding inside a record
is covered here, the .dbt block layout is cross-referenced, not specified.

---

## 1. Overall layout

A `.dbf` file is three regions, contiguous, no padding between them in III+:

```
+--------------------------------------+  offset 0
| 32-byte file header                  |
+--------------------------------------+  offset 32 (0x20)
| field-descriptor array               |  32 bytes per field
|   (1..128 descriptors)               |
+--------------------------------------+
| 0x0D  field-array terminator         |  (followed by 0 or 1 extra 0x00 byte;
|                                      |   see section 4 and Open questions)
+--------------------------------------+  offset = header_length
| record 0   (record_length bytes)     |  delete-flag byte + packed fixed fields
| record 1                             |
|   ...                                |
| record (nrec-1)                      |
+--------------------------------------+  offset = header_length + nrec*record_length
| 0x1A  EOF marker (OPTIONAL in III+)  |  present in some files, absent in others
+--------------------------------------+
```

The header binary is little-endian. Record bodies are ASCII text (every III+
field type C/N/D/L/M stores printable ASCII; there is **no binary numeric type
and no NULL bitmap** in III+ — those are dBASE IV / VFP / dBASE-7 additions).

[verified: clicketyclick/dbf.md DBF_STRUCT diagram + byte-checked CLIENTS.DBF,
TRAVEL.DBF (records start exactly at header_length, no gap)]

---

## 2. The 32-byte file header

Validated against **CLIENTS.DBF** (header_length 0x101=257, 7 fields) and the
memo file **TRAVEL.DBF** (0x83, 11 fields). Little-endian; offsets in hex.

| off (hex) | size | field | type | notes |
|-----------|------|-------|------|-------|
| 00 | 1 | version / flags byte | u8 | III+: `0x03` (no memo) or `0x83` (memo). High bit (0x80) = "has .dbt memo file". Low nibble `3` = dBASE III/III+ table. See section 3. |
| 01 | 1 | last-update year | u8 | year minus 1900. e.g. `0x55`=85 -> 1985. Range 0..255 -> 1900..2155. |
| 02 | 1 | last-update month | u8 | 1..12 |
| 03 | 1 | last-update day | u8 | 1..31 |
| 04 | 4 | number of records (nrec) | u32 LE | count of logical records in the data area |
| 08 | 2 | header length | u16 LE | byte offset where record data begins (= size of header + field array + terminator[+extra]) |
| 0A | 2 | record length | u16 LE | bytes per record = 1 (delete flag) + sum of all field lengths |
| 0C | 2 | reserved | — | `0x00 0x00` in all III+ fixtures. (dBASE IV: unused/0.) |
| 0E | 1 | reserved (IV: incomplete-transaction) | u8 | `0x00` in III+ fixtures. IV-only meaning; do not use in III+. |
| 0F | 1 | reserved (IV: encryption flag) | u8 | `0x00` in III+ fixtures. IV-only meaning. |
| 10 | 12 | reserved for multi-user / LAN | — | all `0x00` in III+ fixtures. dBASE III+ doc labels 0x14-0x1B "reserved for multi-user dBASE". |
| 1C | 1 | reserved (IV: production .mdx flag) | u8 | `0x00` in III+ fixtures. III+ has no MDX; do not import the IV meaning. |
| 1D | 1 | language driver / code-page id (LDID) | u8 | `0x00` in all III+ fixtures (= "no codepage defined"). See section 7. |
| 1E | 2 | reserved | — | `0x00 0x00` in III+ fixtures |

Worked example — **CLIENTS.DBF** bytes 0x00-0x1F:
```
03 55 0A 1E 31 00 00 00 01 01 6A 00 00 00 00 00  ...1......j.....
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
```
Decoded: ver=0x03; date 0x55/0x0A/0x1E = 1985-10-30; nrec=0x00000031=49;
header_length=0x0101=257; record_length=0x006A=106; bytes 0x0C-0x1F all zero.
[verified: byte-checked CLIENTS.DBF + clicketyclick/dbf.md + oracle
oracles/harbour/include/hbdbf.h struct `_DBFHEADER` (bYear@1,bMonth@2,bDay@3,
ulRecCount@4,uiHeaderLen@8,uiRecordLen@10,bCodePage@29)]

### Date byte order is YY,MM,DD — NOT MM,DD,YY

The fileformat-info / Corion mirror (`archive/fileformat-info/corion-dbase-iii.md`)
claims offset 1=month, 2=day, 3=year. **The fixtures prove it wrong.** In every
fixture bytes 1-3 decode sensibly only as year,month,day:

| fixture | bytes 1-3 | as YY,MM,DD | as MM,DD,YY (Corion) |
|---------|-----------|-------------|----------------------|
| CLIENTS | 55 0A 1E | 1985-10-30 (valid) | month=0x55=85 (invalid) |
| TOURS   | 55 0A 1E | 1985-10-30 (valid) | month=85 (invalid) |
| TRAVEL  | 55 0B 0E | 1985-11-14 (valid) | month=85 (invalid) |
| TAX     | 55 06 09 | 1985-06-09 (valid) | month=85 (invalid) |
| BANK    | 55 08 0E | 1985-08-14 (valid) | month=85 (invalid) |

[verified: byte-checked all 5 Sample fixtures + clicketyclick/dbf.md note 21
("YYMMDD") + wikipedia-dbf.md ("YYMMDD") + harbour struct (bYear,bMonth,bDay).
Corion's M/D/Y ordering is WRONG for III+.]

### Header bytes 0x0C-0x1F are NOT "blank-filled"

clicketyclick warns "several sources claim dBASE clears the header with blanks
(0x20)... but I've seen data in the reserved areas." In all six III+ 1.1
fixtures these bytes are `0x00`, not `0x20`. A byte-exact writer should emit
`0x00` for III+ reserved header bytes.
[verified: byte-checked all 6 fixtures]

---

## 3. Version / flags byte (offset 0x00)

dBASE III PLUS 1.1 writes exactly two values:

| value | bits | meaning (III+) | fixtures |
|-------|------|----------------|----------|
| `0x03` | `0000 0011` | dBASE III/III+ table, **no** memo (.dbt) file | BANK, CLIENTS, TAX, UNIVERSD |
| `0x83` | `1000 0011` | dBASE III+ table **with** memo (.dbt) file | TOURS, TRAVEL |

The single distinguishing bit is **bit 7 (0x80) = "has memo file"**. In the
fixtures it tracks the presence of an `M`-type field / a sibling `.DBT` file
with 100% correlation:

```
BANK.DBF    ver=0x03 memobit=0 has_M_field=no  DBT=no
CLIENTS.DBF ver=0x03 memobit=0 has_M_field=no  DBT=no
TAX.DBF     ver=0x03 memobit=0 has_M_field=no  DBT=no
TOURS.DBF   ver=0x83 memobit=1 has_M_field=yes DBT=TOURS.DBT
TRAVEL.DBF  ver=0x83 memobit=1 has_M_field=yes DBT=TRAVEL.DBT
```
[verified: byte-checked all fixtures + clicketyclick/dbf.md note 1 (0x83 =
"dBASE III+ with memo file") + independent-software (0x83 = "FoxBASE 2.x /
dBASE III with memo file") + loc/fdd000325 (0x83 = "FoxBASE+/dBASE III PLUS,
with memo")]

The low 3 bits `011` (=3) are the dBASE-III-family version number. The
"bits 4-6 = SQL table" / "bit 7 = any memo" decomposition that Wikipedia and
the dBASE-7 / Level-5 docs give is a **later (IV/5/7) reinterpretation**;
independent-software explicitly notes it "does not seem to be the case" for
III/IV. For III+ 1.1, treat the byte as a literal enum: only `0x03` and `0x83`
are produced.

**III-vs-IV / lineage contrast (do NOT use for III+):** `0x8B` = dBASE IV with
memo (different .dbt block format, the `0x8B`-style memo); `0x04`/`0x05` =
IV/V without memo; `0x30`/`0x31`/`0x32` = Visual FoxPro; `0xF5` = FoxPro 2 with
memo; `0xE5` = Clipper/HiPer-Six SMT. These never appear in genuine III+ 1.1
output. [documented: clicketyclick/dbf.md note 1, loc/fdd000325 adoption list]

---

## 4. Field-descriptor array + terminator

Immediately after the 32-byte header (offset 0x20) comes an array of 32-byte
field descriptors, one per field, terminated by a `0x0D` byte. III+ supports
up to 128 fields. [documented: clicketyclick/dbf.md note 19 "Max 128 fields"]

### 32-byte field descriptor

Validated against **CLIENTS.DBF** FIRSTNAME (offset 0x20) and **BANK.DBF**
DATE/AMT (offsets 0x20/0x40). Little-endian.

| off (hex) | size | field | type | notes |
|-----------|------|-------|------|-------|
| 00 | 11 | field name | ASCII, 0x00-terminated, 0x00-padded | up to 10 usable chars + terminator. Name = bytes up to the first `0x00`. Unused trailing bytes may hold garbage (see below). |
| 0B | 1 | field type | ASCII char | one of `C N D L M` in III+ (section 5) |
| 0C | 4 | field data address (in memory) | u32 | **meaningless on disk** — a RAM pointer left over from the running program. Ignore on read; a writer may emit anything (real III+ leaves a live address). |
| 10 | 1 | field length | u8 | bytes the field occupies in each record (1..255). For N: includes sign + decimal point. |
| 11 | 1 | decimal count | u8 | number of digits after the decimal point; meaningful for N only (0 for C/D/L/M). |
| 12 | 2 | reserved (multi-user/LAN) | — | `0x00 0x00` in fixtures |
| 14 | 1 | work-area id | u8 | `0x00` or `0x01` in III+ fixtures (see correlation in section 9). clicketyclick note 16 claims "01h in all dBASE III files", but BANK/TAX show `0x00` here — the note is too strong. (Modern Harbour reuses this byte as an auto-increment counter, an IV/VFP meaning; do NOT import it for III+.) |
| 15 | 2 | reserved (multi-user/LAN) | — | `0x00 0x00` in fixtures |
| 17 | 1 | SET FIELDS flag | u8 | `0x00` in all fixtures |
| 18 | 8 | reserved | — | `0x00` in fixtures |
| 1F | 1 | (IV: index-field flag) | u8 | `0x00` in III+ fixtures; IV-only meaning |

Worked example — **CLIENTS.DBF** descriptor 0 at 0x20 (FIRSTNAME, C, len 20):
```
46 49 52 53 54 4E 41 4D 45 00 00  "FIRSTNAME\0\0"
43                                 type 'C'
23 00 2C 40                        data addr 0x402C0023 (RAM, ignore)
14                                 length = 20
00                                 dec = 0
00 00                              reserved
01                                 work-area id = 1
00 00 00 00 00 00 00 00 00 00 00   reserved
```
[verified: fixture CLIENTS @0x20 (FIRSTNAME C len 20 dec 0 wa 1 addr 0x402C0023),
fixture BANK @0x20 (DATE D len 8), @0x40 (AMT N len 10 dec 2); descriptor stride
re-derived = 32 bytes in all 6 fixtures (term offset minus 32 is an exact
multiple of 32 — confirms the III+ 32-byte descriptor, NOT the dBASE-7 48-byte
form) + clicketyclick/dbf.md field-descriptor diagram + independent-software
field-descriptor table + corion-dbase-iii.md + oracle harbour `_DBFFIELD`
(bName[11]@0, bType@11, reserved[4]@12, bLen@16, bDec@17)]

### Field name: read to first NUL; ignore trailing garbage

The 11-byte name field is fixed-size and NUL-padded, **but unused bytes after
the terminating `0x00` can contain stale bytes**. In CLIENTS.DBF the ADDRESS
descriptor name is `41 44 44 52 45 53 53 00 48 4F 00` = `"ADDRESS\0HO\0"` — the
real name is `ADDRESS` (7 chars), and `HO` is leftover garbage in the unused
tail. A correct reader takes the name as bytes `[0 .. first 0x00)`.
[verified: byte-checked CLIENTS.DBF descriptor 2; same pattern in TOURS PERCENT
(`50 45 52 43 45 4E 54 00 4F 4D`) and TRAVEL COST/PAID]

### Terminator: `0x0D`, optionally followed by an extra `0x00`

After the last descriptor there is a `0x0D` byte. In III+ 1.1 there are **two
conventions in the wild**, and `header_length` always points to the byte *after*
the whole terminator region:

| fixture | work-area id | term offset | byte after term | header_length | header_length - (32+32*n) |
|---------|--------------|-------------|-----------------|---------------|----------------------------|
| CLIENTS | 0x01 | 0x100 | `0x20` (first record) | 257 | **1** (just `0x0D`) |
| TOURS   | 0x01 | 0x100 | `0x20` | 257 | **1** |
| TRAVEL  | 0x01 | 0x180 | `0x20` | 385 | **1** |
| UNIVERSD| 0x01 | 0x40  | `0x20` | 65  | **1** |
| BANK    | 0x00 | 0xC0  | `0x00` then 0x1A | 194 | **2** (`0x0D 0x00`) |
| TAX     | 0x00 | 0x60  | `0x00` then data | 98  | **2** (`0x0D 0x00`) |

So:
- **Canonical / "+1" form** (CLIENTS, TOURS, TRAVEL, UNIVERSD): header ends with
  a lone `0x0D`; `header_length == 32 + 32*nfields + 1`.
- **"+2" form** (BANK, TAX): header ends with `0x0D 0x00`; `header_length ==
  32 + 32*nfields + 2`.

The `+2` form matches what the Harbour oracle writes on create
(`nSize = nfields*sizeof(DBFFIELD) + 2`, i.e. terminator + one extra byte;
`oracles/harbour/src/rdd/dbf1.c` ~line 3194). The `+1` form is what the other
four III+ files use. Both are produced by genuine III+ 1.1 tooling in this
corpus. A byte-exact reader must therefore **trust `header_length`** for where
records begin and must NOT assume a fixed `+1`; a byte-exact *writer* must
choose the right convention to reproduce a given file (see Open questions).
[verified: byte-checked all 6 fixtures + oracle dbf1.c create path]

---

## 5. Field types (III+: C, N, D, L, M)

On-disk every III+ field is stored as fixed-width printable ASCII inside the
record body. Confirmed encodings from the fixtures:

| code | type | width rule | on-disk encoding | empty / uninitialised |
|------|------|------------|------------------|------------------------|
| `C` | Character | 1..254 (len byte; III+ practical max 254) | left-justified, **space-padded (0x20)** to field length | all spaces |
| `N` | Numeric | up to 19 (sign + digits + `.`); dec count at desc 0x11 | **right-justified**, space-padded; ASCII digits `0-9`, optional leading `-`, single `.` | all spaces (blank). Overflow -> all `*` (asterisks); see below |
| `D` | Date | always 8 | 8 ASCII chars `YYYYMMDD` (with century, unlike the header date) | 8 spaces |
| `L` | Logical | always 1 | one ASCII char: `T`/`t`/`Y`/`y` (true), `F`/`f`/`N`/`n` (false), `?` or space (uninitialised) | `?` or space |
| `M` | Memo | always 10 | 10-char ASCII, **right-justified** decimal block number into the .dbt; `0` (or blanks) = no memo | right-justified `0`, or 10 spaces |

### C — Character
Left-justified, trailing spaces. CLIENTS rec 1 FIRSTNAME(20) =
`"Claire              "`, ZIPCODE(5)=`"93034"`, STATE(2)=`"CA"`.
[verified: byte-checked CLIENTS.DBF records 1-2]

### N — Numeric
Right-justified, space-padded, plain ASCII. TOURS rec 1 UNITCOST(10,dec2) =
`"   1378.00"` (3 leading spaces); PERCENT(6,dec3)=`"10.000"`; SEATS_OPEN(4,dec0)
=`"  25"`. TRAVEL COST(10,dec2) rec1 = `"   1199.00"`. Negative values carry a
leading `-` inside the field width (BANK ghost data shows `"    -44.00"`,
`"     -5.00"`). The `.` and any sign count toward the field width.
[verified: byte-checked TOURS.DBF, TRAVEL.DBF records; negatives seen in
BANK.DBF residual data 0xC2+]

**Overflow-to-asterisk rule:** when a numeric value will not fit in the field
width (e.g. an arithmetic result wider than the column), III+ fills the field
with asterisks `*` (0x2A) across the whole width. *No fixture in this corpus
exhibits an overflowed field* (scanned all 5 Sample tables: 0 asterisk-filled
numerics), so this is documented, not fixture-confirmed.
[documented: standard dBASE numeric-overflow behaviour; oracle transform code
in oracles/harbour produces `*`-fill on width overflow — NOT byte-confirmed
here. Flagged in Open questions.]

### D — Date
8 ASCII chars `YYYYMMDD`, full century. TOURS DEPARTURE rec1 = `"19850805"`;
TRAVEL RESERVDATE = `"19850715"`. (Contrast: the header last-update date at
offset 1-3 is 2-digit `YYMMDD` binary, not ASCII — see section 2.)
A blank/empty date is 8 spaces. `00000000` is not a valid date.
[verified: byte-checked TOURS.DBF, TRAVEL.DBF + clicketyclick/data_types.md
(D = YYYYMMDD, "0000-00-00 is NOT valid")]

### L — Logical
1 byte. Values seen: `T` (0x54), `F` (0x46). TRAVEL PAID rec1-3 = `T`, rec4 =
`F`. dBASE accepts `Y/y/N/n/T/t/F/f` on input and stores the case as given;
uninitialised is `?` (0x3F) or a space. (clicketyclick: "logical fields are
always *displayed* using T/F/?" — display, not storage.)
[verified: byte-checked TRAVEL.DBF PAID column (`T`,`F`) +
clicketyclick/data_types.md legal-value list]

### M — Memo (pointer only; .dbt is a separate spec)
A 10-byte field holding the **decimal block number** in the sibling `.dbt`,
stored **right-justified ASCII, space-padded** (NOT a binary uint32 in III+).
- TOURS NOTE: every record = `"         0"` (block 0 = no memo).
- TRAVEL NOTES: `"         1"`, `"         2"`, `"         0"` observed
  (block 1, block 2, and 0=none).

```
NOTES rec1 = 20 20 20 20 20 20 20 20 20 31   -> "         1"  (memo block 1)
NOTES rec3 = 20 20 20 20 20 20 20 20 20 30   -> "         0"  (no memo)
```
A block value of `0` means "no memo for this record". (clicketyclick note 17
says an empty memo is "10 blanks"; in these III+ fixtures the empty/no-memo
encoding is right-justified `0`, not all-blanks — both forms mean "no memo";
the fixtures show `0`.)

The 10-byte ASCII pointer is the III+ form. **Contrast:** dBASE IV / FoxPro can
store a 4-byte *binary* uint32 memo pointer (distinguished by field length = 4);
III+ always uses width 10 ASCII. The .dbt block layout, 512-byte blocks, and the
next-free-block header word are specified in the memo (.dbt) spec, not here.
(Cross-check: TOURS.DBT header word at offset 0 reads `01 00 00 00` = 1 as
little-endian, which is the III+ next-block convention; the independent-software
"BIG ENDIAN" claim describes a different lineage and is settled by the .dbt
fixture in that spec.)
[verified: byte-checked TOURS.DBF, TRAVEL.DBF memo columns +
independent-software memo-index section (ASCII vs binary by width) +
clicketyclick/data_types.md (M = 10-digit pointer)]

**Not present in III+:** `F` (float), `B`/`G`/`P`/`Y`/`I`/`T`/`@`/`O`/`+`/`V`.
These are dBASE IV / V / FoxPro / Clipper / Flagship additions. Reject or treat
as unknown in a III+ reader. [documented: clicketyclick/data_types.md type table
which marks each as "dBASE IV and later" / "FoxPro" / "Clipper" etc.]

### No NULL bitmap in III+
There is no null-flags field and no `_NullFlags` system column in III+ (that is
a Visual FoxPro 0x30 / dBASE-7 feature). "Empty" is represented in-band: spaces
for C/N/D/M, `?`/space for L. [verified: byte-checked all fixtures — record
length always equals 1 + sum(field lengths) with no extra null-flag byte; see
section 8]

---

## 6. Record layout

```
+-------+----------------+----------------+-----+
| del   | field 0 bytes  | field 1 bytes  | ... |   total = record_length
| flag  | (len f0)       | (len f1)       |     |
+-------+----------------+----------------+-----+
  1 byte
```

- Records are packed back-to-back with **no separators**, starting at byte
  offset `header_length`. Record *k* begins at `header_length + k*record_length`.
- The first byte of each record is the **delete flag**:
  - `0x20` (space) = active/live record,
  - `0x2A` (`*`) = deleted record (still physically present until PACK).
- The remaining `record_length - 1` bytes are the fixed-width fields in
  descriptor order, each exactly its `field length`, no padding between them.

All fixture records observed carry delete flag `0x20`; none are deleted, so the
`0x2A` value is documented (from clicketyclick/independent-software), not
fixture-confirmed. A deleted record keeps all its field bytes; only the flag
changes. [verified: byte-checked delete flag = 0x20 on every record of CLIENTS,
TOURS, TRAVEL, TAX, UNIVERSD + independent-software (0x20 live / 0x2A deleted)
+ clicketyclick/db2_dbf.md note 10]

Worked record — TOURS.DBF rec 1 (record_length 83), fields packed:
```
20                                  delete flag (live)
"AV10"                              TRAVELCODE  C(4)
"10-night Alaska/Vancouver Cruise        "  TRAVELPLAN C(40)
"19850805"                          DEPARTURE   D(8)
"   1378.00"                        UNITCOST    N(10,2)
"10.000"                            PERCENT     N(6,3)
"  25"                              SEATS_OPEN  N(4,0)
"         0"                        NOTE        M(10) -> no memo
```
[verified: byte-checked TOURS.DBF record 1]

---

## 7. Language-driver / code-page byte (offset 0x1D)

One byte (also called LDID). **`0x00` in every III+ 1.1 fixture in this corpus**
= "no codepage defined" (the table relies on whatever the runtime SET to;
III+ on US DOS effectively means CP437). The byte became actively used by FoxPro
and dBASE IV+; the value table (for readers that need it):

| value | code page | value | code page |
|-------|-----------|-------|-----------|
| 0x00 | none defined | 0x65 | CP866 Russian DOS |
| 0x01 | CP437 US DOS | 0x66 | CP865 Nordic DOS |
| 0x02 | CP850 Intl DOS | 0x67 | CP861 Icelandic DOS |
| 0x03 | CP1252 Windows ANSI | 0x68 | CP895 Kamenicky (Czech) DOS |
| 0x04 | CP10000 Mac Roman | 0x69 | CP620 Mazovia (Polish) DOS |
| 0x64 | CP852 EE DOS | 0x6B | CP857 Turkish DOS |
| | | 0xC8 | CP1250 EE Windows |

(codetitans-codepage and clicketyclick give slightly different CP numbers for
0x65/0x66 — codetitans: 0x65=866, 0x66=865; clicketyclick: 0x65=865, 0x66=866.
The DOS-Nordic/DOS-Russian pair is swapped between sources; irrelevant for III+
where the byte is `0x00`, but flagged for completeness.)

The modern dBASE Plus `LDRIVER()`/`CHARSET()` functions read this region and map
it to driver strings like `"DB437DE0"` / charset `"DOS:437"`
(`archive/dbase-com-help/DRIVERID.md`). For a III+ 1.1 byte-exact writer, emit
`0x00` unless a driver was explicitly set.
[verified: byte-checked LDID=0x00 in all 6 fixtures + codetitans-codepage.md
table + clicketyclick/dbf.md note 5 + DRIVERID.md (LDID region semantics).
clicketyclick/codetitans disagree on 0x65/0x66 ordering — noted.]

---

## 8. Structural invariants (CHECKED)

### Invariant 1 — header length vs field count
Canonical: `header_length == 32 + 32*nfields + 1`.
III+ also produces the `+2` variant (`0x0D 0x00`); see section 4.

| fixture | nfields | 32+32*n+1 | actual header_length | result |
|---------|---------|-----------|----------------------|--------|
| CLIENTS | 7  | 257 | 257 | OK (+1) |
| TOURS   | 7  | 257 | 257 | OK (+1) |
| TRAVEL  | 11 | 385 | 385 | OK (+1) |
| UNIVERSD| 1  | 65  | 65  | OK (+1) |
| BANK    | 5  | 193 | 194 | +1 OFF -> matches 32+32*n+**2** |
| TAX     | 2  | 97  | 98  | +1 OFF -> matches 32+32*n+**2** |

So the safe check is: `header_length - (32 + 32*nfields)` is **1 or 2**, and
the number of field descriptors = `(header_length - 32 - 1) // 32` works for
the `+1` form but gives the right count for both because the trailing 0x0D/0x00
is consumed before the next 32-byte boundary (the descriptor scan stops at the
`0x0D` byte, which is the authoritative way to find the field count).
**A reader must locate the field count by scanning descriptors until the `0x0D`
terminator, not by arithmetic on header_length.**
[verified: byte-checked all 6 fixtures]

### Invariant 1b — record length vs field lengths
`record_length == 1 + sum(field lengths)`. Holds exactly for all 6 fixtures
(this is how the absence of a null bitmap is confirmed):

| fixture | sum(field len) | +1 | record_length | result |
|---------|----------------|----|---------------|--------|
| CLIENTS | 105 | 106 | 106 | OK |
| TOURS   | 82  | 83  | 83  | OK |
| TRAVEL  | 136 | 137 | 137 | OK |
| TAX     | 19  | 20  | 20  | OK |
| BANK    | 23  | 24  | 24  | OK |
| UNIVERSD| 34  | 35  | 35  | OK |
[verified: byte-checked all 6 fixtures]

### Invariant 2 — file size vs records
`file_size == header_length + nrec*record_length [+ 1 EOF byte]`.

| fixture | header_length | nrec | record_length | body=h+nrec*r | file_size | EOF byte present? |
|---------|---------------|------|---------------|---------------|-----------|-------------------|
| CLIENTS | 257 | 49 | 106 | 5451 | 5451 | **no** (file == body) |
| TOURS   | 257 | 30 | 83  | 2747 | 2747 | **no** |
| TRAVEL  | 385 | 49 | 137 | 7098 | 7098 | **no** |
| TAX     | 98  | 9  | 20  | 278  | 279  | **yes** (0x1A at end) |
| UNIVERSD| 65  | 1  | 35  | 100  | 101  | **yes** (0x1A at end) |
| BANK    | 194 | 0  | 24  | 194  | 512  | special (see below) |

The corion-dbase-iii.md validation rule `FileSize = NDR*DRS + HSZ` matches the
"no EOF byte" files exactly and is off by 1 for the files that carry the EOF
byte. **The `0x1A` EOF marker is therefore optional in III+ 1.1** — present in
TAX and UNIVERSD, absent in CLIENTS/TOURS/TRAVEL. A reader must not require it
and must use `header_length + nrec*record_length` (or the header `nrec`) to find
the record-area end; the EOF byte, if present, is the single byte immediately
after the last record. independent-software confirms "this marker is *not*
present" in some files.
[verified: byte-checked all 6 fixtures; last byte = 0x1A for TAX/UNIVERSD,
non-0x1A for CLIENTS/TOURS/TRAVEL]

**BANK.DBF special case** — an *empty* table that was not truncated. Header says
`nrec=0`, so the logical record area is empty; at offset `header_length`=194
(0xC2) sits the `0x1A` EOF marker, and the 317 bytes after it are **ghost data**
from a previous, larger version of the file (visible numeric records like
`19850612 ...`). This is exactly the behaviour clicketyclick warns about: dBASE
does not shrink the physical file on delete/PACK, so bytes after the logical EOF
are garbage. A correct reader uses `nrec=0` and ignores everything past 0xC2.
[verified: byte-checked BANK.DBF: nrec=0, byte@0xC2=0x1A, residual ASCII numeric
data follows + clicketyclick/dbf.md ("file is not truncated... garbage after the
EOF mark") + db2_dbf.md PACK diagram]

---

## 9. Notes for implementers

- **Trust `header_length` and `record_length` from the header**, not arithmetic
  assumptions, for navigation. Get the field count by scanning 32-byte
  descriptors until the `0x0D` terminator.
- **Record count:** the header `nrec` (offset 4, u32 LE) is authoritative for
  reading. Harbour additionally recomputes it from file size as
  `(file_size - header_length) / record_length` when a table is opened shared
  (`oracles/harbour/src/rdd/dbf1.c` `hb_dbfCalcRecCount`); for a single-writer
  byte-exact tool, write and trust the header `nrec`.
- **work-area id (descriptor 0x14) correlation:** in this corpus the value
  perfectly predicts the terminator convention — `0x00` -> `+2` header
  (BANK, TAX); `0x01` -> `+1` header (CLIENTS, TOURS, TRAVEL, UNIVERSD). Whether
  this is causal or coincidental (e.g. files created by different III+ utilities
  or different III+ point releases) is unresolved; see Open questions.
- **Field data address (descriptor 0x0C, 4 bytes):** a live RAM pointer at the
  time the file was written; meaningless and non-deterministic on disk. Do not
  validate it; for byte-exact reproduction of a specific file you must replay
  the captured bytes.
- **ASCII storage everywhere:** C/N/D/L/M are all printable ASCII. No
  little-endian binary field values exist in III+ (those are IV/VFP `I`,`Y`,`T`).

---

## Open questions

These are tagged `[oracle-resolves]` — the documents/fixtures conflict or are
silent, and the III+ 1.1 binary (`goldens/.../DBASE.EXE` / `DBASE.OVL`) should
settle them. (To be promoted into GAPS.md.)

1. **Header-length `+1` vs `+2` terminator convention.** Genuine III+ 1.1 output
   in this corpus uses *both* `header_length = 32+32n+1` (lone `0x0D`) and
   `= 32+32n+2` (`0x0D 0x00`). Which does DBASE.EXE 1.1's CREATE/COPY/APPEND
   write? Is the `+2`/work-area-id-0 form produced by a different code path
   (e.g. CREATE vs COPY, or an older dBASE III file later opened by III+)? A
   byte-exact *writer* must know which to emit. `[oracle-resolves]`

2. **`0x1A` EOF byte: when is it written?** Present in TAX and UNIVERSD, absent
   in CLIENTS/TOURS/TRAVEL — all III+ 1.1 files. Does III+ always append `0x1A`
   on APPEND/normal close but omit it after certain operations (PACK, COPY,
   ZAP)? Need the binary to decide whether a byte-exact writer emits it. `[oracle-resolves]`

3. **Work-area-id (descriptor byte 0x14) = 0 vs 1.** The value (0 in BANK/TAX,
   1 in the rest) correlates with the terminator convention. Is it the dBASE
   work-area number baked in at write time, and does it actually drive the
   header-length choice, or is the correlation incidental? `[oracle-resolves]`

4. **Numeric overflow-to-asterisk:** no fixture demonstrates `*`-filled numeric
   fields. Confirm via the binary (or a generated overflow case) that III+ 1.1
   fills the entire field width with `0x2A` on width overflow and the exact
   trigger condition. `[oracle-resolves]`

5. **Deleted-record byte `0x2A`:** no fixture contains a deleted record; the
   `0x20`/`0x2A` semantics are documented but not fixture-confirmed for III+ 1.1.
   `[oracle-resolves]`

6. **Code-page values 0x65/0x66:** clicketyclick and codetitans disagree on
   whether 0x65=CP865/CP866 (Nordic vs Russian). Irrelevant for III+ (byte is
   0x00) but unresolved if a non-zero LDID is ever encountered. `[oracle-resolves]`

---

## Sources

Read for this document (all under `/home/tobias/Projects/dbase3-decomp/`):

- `archive/clicketyclick/dbf.md` — primary header + field-descriptor + record
  diagram, version-byte table (note 1), date note 21, EOF note 11, codepage
  note 5, "not truncated/garbage after EOF" warning.
- `archive/clicketyclick/data_types.md` — C/N/D/L/M widths and semantics, memo
  pointer = 10-digit, logical legal values, date YYYYMMDD.
- `archive/clicketyclick/db2_dbf.md` — dBASE II contrast, PACK behaviour,
  delete flag 0x20/0x2A.
- `archive/independent-software/dbf-dbt-fpt.md` — header + descriptor tables,
  version byte, delete marker 0x20/0x2A, "EOF marker may be absent", memo
  pointer ASCII-vs-binary by width.
- `archive/fileformat-info/corion-dbase-iii.md` — Corion header struct
  (its M/D/Y date ordering is WRONG vs fixtures; its `FileSize=NDR*DRS+HSZ`
  rule matches no-EOF files).
- `archive/loc/fdd000325-dbf.md` — version-byte adoption list (0x03/0x83
  labels), format history/lineage.
- `archive/misc/wikipedia-dbf.md` — Level-5 header table (YYMMDD confirmation;
  the bit-7=memo decomposition is a later reinterpretation).
- `archive/misc/codetitans-codepage.md` — LDID code-page value table.
- `archive/dbase-com-help/DRIVERID.md` — LDID region read via LDRIVER()/CHARSET().

Golden fixtures byte-inspected with python3/xxd:
- `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/BANK.DBF`
  (0x03, nrec=0, +2 header, ghost data after EOF)
- `.../CLIENTS.DBF` (0x03, 49 recs, 7 C fields, +1 header, no EOF byte)
- `.../TOURS.DBF` (0x83, 30 recs, memo, +1 header, no EOF byte)
- `.../TRAVEL.DBF` (0x83, 49 recs, 11 fields incl. L+M, +1 header, no EOF byte)
- `.../TAX.DBF` (0x03, 9 recs, +2 header, EOF byte present)
- `goldens/dbase-iii-plus-1.1/extracted/disk2/UNIVERSD.DBF`
  (0x03, 1 rec, +1 header, EOF byte present)
- `.../TOURS.DBT` (header word, cross-reference only; .dbt is a separate spec)

Semantic oracle (NOT a byte authority):
- `oracles/harbour/include/hbdbf.h` — `_DBFHEADER` / `_DBFFIELD` struct offsets
  (confirm date byte order, header field positions).
- `oracles/harbour/src/rdd/dbf1.c` — create-path header sizing (`nfields*32 + 2`),
  `hb_dbfCalcRecCount`, record read/write offset math.

---

## Verification

Adversarial byte-level re-derivation, 2026-06-16, with python3 `struct`/`xxd`.
Every offset/size/encoding claim was re-derived independently from the raw bytes;
results below are reproduced, not taken on faith.

**Fixtures byte-checked (all 6):**
- `goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/BANK.DBF`
- `.../CLIENTS.DBF`
- `.../TOURS.DBF`
- `.../TRAVEL.DBF`
- `.../TAX.DBF`
- `goldens/dbase-iii-plus-1.1/extracted/disk2/UNIVERSD.DBF`

**Confirmed (independently re-derived from the bytes):**
- Header layout (offsets 0x00-0x1F): version, YY/MM/DD date, nrec u32 LE,
  header_length u16 LE, record_length u16 LE all decode exactly as the section-2
  table states for all 6 fixtures. CLIENTS worked-example dump
  `03 55 0A 1E 31 00 00 00 01 01 6A 00 00 ...` matches byte-for-byte.
  [CLIENTS @0x00: ver 0x03, 1985-10-30, nrec 49, hlen 257, rlen 106.]
- **Date order YY,MM,DD** confirmed: byte 1 is the year offset (0x55=85) in all 5
  Sample fixtures; Corion's M/D/Y would require month=85 (invalid). Corion wrong.
- Reserved bytes 0x0C-0x1F: `0x00` (never `0x20`) in all 6 fixtures.
- **Version byte / memo bit:** 0x03 (BANK/CLIENTS/TAX/UNIVERSD), 0x83 (TOURS/TRAVEL);
  bit 0x80 correlates with M-field AND a sibling .DBT on disk at 100% (re-checked
  filesystem: TOURS.DBT and TRAVEL.DBT exist, the four 0x03 files have no .DBT).
- **Field descriptor (32 bytes):** name@0, type@0x0B, addr@0x0C (RAM, e.g.
  0x402C0023), len@0x10, dec@0x11, work-area-id@0x14 all re-derived. Descriptor
  stride is **32** (term offset minus 32 is an exact multiple of 32 in every
  fixture) — this is the III+ form, NOT the dBASE-7 48-byte descriptor.
- Field-name trailing garbage: CLIENTS ADDRESS = `41 44 44 52 45 53 53 00 48 4F 00`
  (`"ADDRESS\0HO\0"`), TOURS PERCENT = `...00 4F 4D` confirmed; name = bytes to
  first NUL.
- **Terminator +1/+2 conventions** confirmed: delta = header_length-(32+32n) is
  **1** for CLIENTS/TOURS/TRAVEL/UNIVERSD (lone 0x0D, byte-after-term 0x20) and
  **2** for BANK/TAX (`0x0D 0x00`). Correlates exactly with descriptor-0
  work-area-id (1 → +1, 0 → +2). Both produced by genuine III+ 1.1 tooling.
- **On-disk encodings:** C left-justified space-padded (CLIENTS records); N
  right-justified space-padded (`"   1378.00"`, `"10.000"`, `"  25"`); D = 8-char
  `YYYYMMDD` (`"19850805"`); L = `T`/`F` (TRAVEL PAID rec1-3 T, rec4 F); M =
  10-char right-justified ASCII block number (TRAVEL NOTES rec1
  `20*9 31` = "1", rec3 `20*9 30` = "0"). All byte-exact.
- **Invariant 1b** `record_length == 1 + sum(field len)` holds for all 6 (proves
  no III+ null bitmap; descriptor stride 32 leaves no room for one either).
- **Invariant 2 / optional EOF:** file_size == header_length + nrec*record_length
  with NO trailing byte for CLIENTS/TOURS/TRAVEL (last byte 0x37/0x30/0x30); +1
  trailing `0x1A` for TAX/UNIVERSD. EOF marker confirmed optional.
- **BANK special case:** nrec=0, byte@0xC2 = 0x1A, then 317 ghost bytes containing
  ASCII numeric records including `"-44.00"` and `"-5.00"` (negatives carry an
  in-width leading `-`, as claimed).
- **Language driver (0x1D) = 0x00** in all 6 fixtures. Codepage value table
  cross-checked against `archive/misc/codetitans-codepage.md` — every listed value
  (0x01=437, 0x02=850, 0x03=1252, 0x04=10000, 0x64=852, 0x65/0x66, 0x67=861,
  0x68=895, 0x69=620, 0x6B=857, 0xC8=1250) matches the source.
- **Delete flag = 0x20** on every record of CLIENTS/TOURS/TRAVEL/TAX/UNIVERSD
  (re-scanned all records; no 0x2A present anywhere).
- **Asterisk-overflow scan:** 0 asterisk-filled numeric fields across all Sample
  tables — so the `*`-overflow rule remains documented-only, as the spec states.
- **ASCII purity of code blocks:** scanned the whole document; the only non-ASCII
  character is the em-dash (U+2014, 46 occurrences) and it appears **exclusively in
  prose, never inside a fenced code/offset block**. Author's claim confirmed.
- III-vs-IV hygiene: no 48-byte descriptor, no binary numeric type, no null-flags
  column, no IV-only meanings smuggled into a III+ assertion. The memo pointer is
  the III+ 10-byte ASCII form (width 10, not the IV 4-byte binary).

**Fixed during verification:**
- Section 2 descriptor table, offset 0x14: added that clicketyclick note 16
  ("01h in all dBASE III files") is contradicted by BANK/TAX (`0x00`), and that
  modern Harbour reuses this byte as an IV/VFP auto-increment counter — a meaning
  that must NOT be imported for III+.
- Strengthened the field-descriptor `[verified]` tag with exact fixture offsets
  and the re-derived 32-byte stride (rules out the dBASE-7 48-byte descriptor).

**No factual byte errors found.** Every offset, size, and encoding claim in the
document matched the fixtures on independent re-derivation. The two flagged
discoveries (+1/+2 header convention; optional 0x1A EOF) are real and reproduced.

**Residual `[oracle-resolves]` gaps (unchanged — no fixture can settle them):**
- Numeric overflow-to-asterisk (Open question 4): not exhibited by any fixture.
- Deleted-record byte 0x2A (Open question 5): no fixture contains a deleted record.
- +1/+2 terminator and work-area-id 0/1 causality (Open questions 1, 3): the
  correlation is byte-confirmed but the *cause* needs DBASE.EXE.
- 0x1A EOF write-trigger (Open question 2): optionality confirmed, write rule not.
- Code-page 0x65/0x66 ordering (Open question 6): sources disagree; irrelevant
  while LDID is 0x00.
