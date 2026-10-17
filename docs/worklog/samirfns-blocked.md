# SAMIR SET/functions/errors lane: blocked at the flat-image memory guard

This is an incomplete lane, not acceptance. No beads, Git metadata, CLAUDE.md,
HANDOFF.md, commits or pushes were touched. Work was performed alone. No full
make test or Bochs was run. All QEMU invocations used INITECH_QMP_STDIO=1.

## Blocking evidence

The current source fails `make samir-com` with:

```
/usr/bin/ld: SAMIR image leaves insufficient PAL memory
collect2: error: ld returned 1 exit status
make: *** [Makefile:27474: build/SAMIR.COM] Error 1
```

`os/samir/boot/samir.ld:68` enforces
`_samir_load_end + 0x10000 + 0x4F70 + 0x20 <= 0x6F000`.
With PROGRAM_IMAGE=0x40100 this limits loaded bytes to 106352. The last
successful link, before retaining numeric descriptor width/places in field
expression results, was 106335 bytes, with zero x87 instructions: 17 bytes of
headroom. The subsequent correct field-presentation change trips that guard.
The guard, loader, memory_map and PAL heap sizes were not changed. Continuing
this lane needs a program-memory/packaging decision or a separately validated
size reduction; weakening the guard would risk the live PAL arena/environment.
The earlier COM is stale and was moved to
build/samirfns/SAMIR-before-presentation.COM after recording its size.

Log: build/samirfns/freestanding-limit.log. The earlier successful size is in
build/samirfns/freestanding.log. Feature work stopped at this failure, as the
lane brief requires for a genuine blocker.

## Changes left for review

- Makefile: secondary tracer kmain object now uses the adjacent shell target's
  kernel-main-compile helper. Its previous `$<` was SERIAL_TRACE_PACK, so GCC
  compiled build/serial_trace_pack and produced no kmain_shell_secondary.o.
  This repair allowed the initial injected QEMU RED gate to run normally.
- New fns_mint.sh, test_samir_fns.c and twenty proposed PRGs under
  harness/diff/dbf_diff/xbase_prog_diff/corpus/fns. The factory compares a
  transcript slice byte for byte against real III PLUS 1.1; host and QEMU run
  the same PRG. The QEMU harness also checks every injected key, normal return,
  triple_fault=0 and fsck.fat before/after the FAT image. No new FAT implementation
  was made; existing mtools/dosfstools provision/check the guest disk.
- lex.c / parse.c / eval.h: -> token and qualified identifier source span.
  workarea.c resolves the named open area without changing its selection or
  cursor, and retains field numeric width/places for expression output.
- set.c / set.h / eval.h / workarea.c / samir_main.c: owned ALTERNATE handle,
  ON/OFF capture, DOS EOF on close, and output mirroring before CONSOLE filtering.
  This is only the bounded positive transcript case; alternate failure/lifetime
  semantics are not accepted. A short write currently stops mirroring and sets
  error_number; propagation through the command executor is unfinished.
- set.c also has an ungraded unknown-SET catalog correction. proc.c has an
  ungraded copy of character WITH arguments out of the temporary expression
  buffer. Their named mutant hooks exist, but these two features have not had
  their required independent per-item RED/GREEN cycle. They are not accepted.

The initial alias host gate was RED (4 checks, 1 failure) and the initial guest
gate was RED (11 checks, 1 failure). Both reported:
`FNS transcript differs at byte 11 (got 0x31, real 0x0d; lengths 37/66)`.
The original probe additionally printed DBF(), which exposed the existing
path-vs-alias difference unrelated to alias qualification. Before acceptance,
that probe was narrowed to field values and RECNO(), then independently
re-minted; the current expected transcript was never hand-edited.

## Exact targets and actual results

Aggregate targets added: mint-samir-fns, test-samir-fns,
test-samir-fns-mutants, test-samir-fns-emu, test-samir-fns-emu-mutants.

For each CASE below the Makefile adds the four exact name expansions:
`test-samir-fns-CASE`, `test-samir-fns-CASE-mutant`,
`test-samir-fns-CASE-emu`, `test-samir-fns-CASE-emu-mutant`.
They are registered in TEST_UNIT_GATES / TEST_EMU_GATES.

```
alias alternate century console default diskspace index listoff os overflow
path procedure recsize relation talk time type unknown version with
```

These vectors are scaffolding, not twenty completed features. Only alias and
alternate have completed host positive/mutant runs. The remaining mutant hooks
are not all implemented. No aggregate is claimed GREEN.

| Exact GREEN host target | Tail |
| --- | --- |
| test-samir-fns-alias | test-samir-fns: 4 checks, 0 failures; >>> test-samir-fns-alias: green |
| test-samir-fns-alternate | test-samir-fns: 3 checks, 0 failures; >>> test-samir-fns-alternate: green |
| test-samir-fns-alias-mutant | >>> SAMIR_FNS_MUTANT_alias RED: FAIL harness/diff/dbf_diff/test_samir_fns.c:81: FNS transcript byte-identical to real III+ |
| test-samir-fns-alternate-mutant | >>> SAMIR_FNS_MUTANT_alternate RED: FAIL harness/diff/dbf_diff/test_samir_fns.c:81: FNS transcript byte-identical to real III+ |

The two mutant recipes returned success only after their mutant process exited
1 with that assertion. Alias first differs at byte 11 (got 0x1a, real 0x0d;
lengths 12/55). Alternate first differs at byte 0 (got 0x00, real 0x5b;
lengths 0/31). Logs: focused-host.log, focused-mutants.log, mut-alias.log,
mut-alternate.log in build/samirfns.

The initial QEMU alias run had keys_sent=9, triple_fault=0, timed_out=0,
quit_after_found=1. No updated guest GREEN is possible while the current COM
fails to link. The new emulator mutants were not run.

## Mint ledger and Law-1 sources

Read first: CLAUDE.md in full, PRD 6.6, WL-0148, samirdml-acceptance.md,
samirdml-mint.md, spec/samir/README.md and the requested JSONL bead records.
The live bd database lacks the lane IDs; the ipu5.8.1 brief was authoritative.

Reference installation: /home/tobias/Projects/dbase3-decomp/mint/work, copied
read-only into build/samirfns/golden. Real dBASE III PLUS 1.1 under dosbox-x
2024.03.01. Inputs/captures have per-run sha256.txt files. Only these four
captures completed with both markers and DOS EOF:

| Row | Capture | Printed source | SHA-256 of GOLD.OUT |
| --- | --- | --- | --- |
| SF-alias | build/samirfns/golden/alias/GOLD.OUT | Using III+ U5-190; local work-areas-relations-and-limits.md section 2; real qualified-field bytes | afb004683f7685e12506f14c43f6ad427c35f0c0085cd9d0f64f78bcb1187948 |
| SF-alternate | build/samirfns/golden/alternate/GOLD.OUT | Using III+ U5-193 | bdbb79cbc33d54fcb469bb43e5f6e5c4aa8bc3027c292a90851aa742705ea049 |
| SF-century | build/samirfns/golden/century/GOLD.OUT | Real III+ DATE/CTOD transcript; no date fix made | 8541345da065a3f211c8d62900e23d66f2e56acfafdb3e87de379ac5c5c85b78 |
| SF-console | build/samirfns/golden/console/GOLD.OUT | Existing U5-210 implementation; no console fix made | 806925cf8efce65d604c178648ccc3984a46c6728871de74317899271e92fba8 |

Manual: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1
(1986) Manuals/Using dBase III Plus.pdf. Rendered at 110 dpi and visually read:
U5-111 (DO WITH, PDF 223), U5-144 (LIST OFF, PDF 256), U5-190 (PDF 302),
U5-193 (PDF 305), U5-214 (PDF 326), U5-239 (PDF 351), U5-249 (PDF 361),
U5-253 (PDF 365), U5-255 (PDF 367), U5-261 (PDF 373), U6-18 (PDF 414),
U6-24 (PDF 420), U6-61/62/63 (PDF 457/458/459), U6-69 (PDF 465).
Additional pages were rendered but not all visually inspected; no behavior is
claimed from those pages.

Important unfinished factory issue: DEFAULT's proposed PRG uses a directory
argument. The inspected U5-214 syntax is a drive argument. Its zero-byte
capture is NOT a golden and must not be accepted or implemented from this
probe. The mint stopped at this case; later cases were not minted. The initial
normal-core/dummy-launch failures are superseded by the successful auto-core
control; there is no general reference-emulator blocker. Using timeout
--foreground kept the mint shell alive across retained DOSBox windows.

## Not done

No lane item is fully accepted on host plus emulator. The remaining SET,
function, input/event, memo command-level, LIST OFF, status-label, century,
overflow and FAT/INT21/PAL/DBF typed-error work is unfinished. The unknown-SET
and WITH code mentioned above is ungraded. ALTERNATE failure propagation and
resource cleanup beyond the successful REPL close case are unfinished.

No book DRAFT markers were removed, no examples re-minted with pubs-examples,
and none of test-pubs, test-samir, test-dbase*, test-samir-core*,
test-samir-dml*, test-samir-boot/write/canon-y2k, image, locked-purge or
finder-service was run after the blocker. Existing memo and TYPE fixes from
the prior lanes were read, not claimed as new work. ASCII scan of touched code
and the proposed corpus had zero matches.
