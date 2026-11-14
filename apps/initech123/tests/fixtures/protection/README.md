# Lotus R2.2 protection captures

Real initialized Lotus 1-2-3 R2.2 under Packard Bell OEM MS-DOS 3.30,
QEMU 486, 8 MiB, fixed RTC. Provenance, runtime/boot hashes and capture
method are in `goldens/dataloss/README.md` and `goldens/dataloss/inputs/`.
No application binary or operating-system image is included here.

`Bnn.WK1` is the saved worksheet immediately before the command; `Ann.WK1`
is the real worksheet saved after it. `Bnn` and `Rnn` PNG/VGA/text files
show the before and result screens. `input.steps`, `extra.steps` and the
compressed HMP logs preserve the actual inputs and captures. `edge.steps`
captures the last-column case. SHA256SUMS
pins these immutable inputs. The test compares actual cell records,
named-range records and PROTEC bytes against these WK1 files, and compares
READY/ERROR and diagnostics against the result screen. It does not compute
expected cells from product code or reproduce an interpretation in a fixture.
Other WK1 records describe printer, graph and window state outside this test.

The base contains integers A1=11, B1=12, A2=21, B2=22 and destination
D1=31, E1=32, D2=41, E2=42, all initially protected, global protection on.
The result screens can still display the old worksheet while an error is
pending: the saved bytes after dismissal are authoritative for the prefix.

| Case | Command / special input | Observation |
|---|---|---|
| 01 | Copy A1:A2 to D1, all protected | Refuses unchanged. |
| 02 | Same, D1 unprotected | Writes D1, refuses at D2. |
| 03 | Move, source unprotected, target protected | Refuses unchanged. |
| 04 | Move, source protected, D1:D2 unprotected | Succeeds; source protection does not block a move. |
| 05 | Value A1:A2 to D1, only D1 unprotected | Writes D1, refuses at D2. |
| 06 | Trans A1:B1 to D1, only D1 unprotected | Writes D1, refuses at D2. |
| 07 | Erase A1:A2, only A1 unprotected | Erases A1 with its format retained; refuses at A2. |
| 08 | Fixed 2 format A1:A2, protected | Succeeds. |
| 09 | Unprotect A1:A2, global protection enabled | Succeeds. |
| 10 | Enter into existing protected A1 | Refuses unchanged. |
| 11 | Enter into absent H1 | Refuses unchanged. |
| 12 | F2 edit existing protected A1 | Refuses unchanged. |
| 13 | Worksheet Erase Yes | Clears protected worksheet and resets global protection. |
| 14 | Copy A1:B2 to D1, D1:E1 unprotected | Writes D1, refuses D2 before visiting E1: column order. |
| 15 | Move same rectangle/unprotection | Refuses with no prefix: checks the complete destination. |
| 16 | Copy absent H1:H2 to D1, only D1 unprotected | Deletes D1, then refuses D2. |
| 17 | Tile A1 to D1:D2, only D1 unprotected | Writes D1, refuses D2. |
| 18 | Trans A1:B2 to D1, D1:E1 unprotected | Writes D1 and E1, refuses D2: transposed traversal. |
| 19 | Range Label Right A1:A2, A1 label unprotected, A2 numeric protected | Changes A1, succeeds. The setup types alpha then beta in A1; Return does not move the pointer. |
| 20 | Same command, both cells numeric | Succeeds with unchanged cells. |
| 21 | Overlapping Move A1:A2 to A2, only A2 unprotected | Refuses unchanged, including the writable first destination. |
| 22 | Range Label Right A1:A2, both labels, only A1 unprotected | Succeeds and changes BOTH labels. Protection does not block presentation. |
| 23 | Name Table TOTAL=A1 at A10, A10:B10 and A11 unprotected | Writes table and erases A11; refuses at absent protected B11. |
| 24 | Name Table at absent protected A10 | Refuses unchanged. |
| 25 | Name Table with no names at protected A10 | Still refuses: terminator erasure checks protection. |
| 26 | Two names, ZZZ=A1:B2 and AAA=B1, A10:B12 unprotected | Alphabetical output, singleton B1, range A1..B2, formatted terminators retained. |
| 27 | Name Table at IU8192, last output row unprotected | Writes both labels, then errors `Invalid cell or range address` for terminator beyond sheet. |
| 28 | Move protected A1:A2 to unprotected D1:D2 with names and G1=+A1+A2 | Succeeds; formula/name references follow. |
| 29 | Same, only D1 unprotected | Refuses unchanged, including formulas/names. |
| 30 | Value A1:B2 to D1, D1:E1 unprotected | Writes D1 then refuses D2, column order. |
| 31 | Erase A1:B2, A1:B1 unprotected | Erases A1 then refuses A2, before B1. |
| 32 | Range Label Right all four labels, only first row unprotected | Changes all four, including protected labels. |
| 33 | Name Table at IV10, IV10:IV11 unprotected | Refuses with `Invalid cell or range address`, without writing a prefix. |

Reproduce from the repository root, with fresh absolute scratch directories:

```
sh apps/initech123/tests/capture-protection.sh /tmp/123-protection
sh apps/initech123/tests/capture-protection-extra.sh /tmp/123-extra \
  /tmp/123-protection/runtime/runtime.img \
  /tmp/123-protection/runtime/lotus-boot.img
```

The second command consumes the stopped first runtime, which contains BASE.
It now includes case 33 and waits one second after the final Save before
stopping QEMU, allowing the guest to finish that save under host load.
The first last-column experiment stopped too soon and yielded a zero-length
save; it was discarded and repeated with the recorded `edge.steps` barrier.
These scripts use the existing `goldens/dataloss/` C driver and runtime
packer. They do not run during tier 0. Clock/settings metadata can differ on
replay; the retained bytes are the oracle, not regenerated expectations.
