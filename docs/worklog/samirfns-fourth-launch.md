# SAMIR functions lane, fourth launch: incomplete certificate

Work was performed alone. No commit, push, beads edit, full make test or
Bochs run. Every QEMU make invocation exported INITECH_QMP_STDIO=1. RAM and
disk checks showed 17 GiB available and at least 112 GiB free. The requested
fetch of the PM's main succeeded; the requested merge failed before applying
changes because .git/ORIG_HEAD is on a read-only filesystem.

## Changes retained

- SET INDEX opens the selected area's NDX list, supplies the NDX extension,
  makes the first index master, positions at its beginning, and closes the
  selected area's indexes for an empty list. The old text accessors remain.
- LIST OFF consumes the leading OFF before parsing the scope and expression
  list. A trailing OFF still works.
- RECSIZE reads the selected DBF's physical record length, including its
  deletion flag; it returns zero without a table. The evaluator uses a cursor
  callback, keeping the filesystem dependency out of the expression core.
- TYPE's already present expression-string implementation is now exercised
  against a fresh real transcript and a named always-C mutant.
- samir-fns-goldens pins the four inputs and independent real captures and is
  added to TEST_UNIT_GATES. The per-case host/emulator gates and mutant recipes
  were already present in the retained tree and in both gate vectors.
- The INDEX DRAFT marker and LIST OFF limitation were removed from the books;
  RECSIZE was documented. Three example sessions were added. make pubs-examples
  re-minted 258 transcripts. Unrelated regenerated examples were archived under
  build/samirfns/fourth/pubs-full-remint and restored. The LIST entry's existing
  c-list-2 transcript and the three new sessions remain regenerated.

Changed files: Makefile; os/samir/cmd/{query,set,workarea}.c;
os/samir/core/fn_builtins.c; os/samir/include/samir/eval.h;
harness/diff/dbf_diff/xbase_prog_diff/fns-accepted.sha256;
pubs/manuals/initechbase-quick-reference/{qr-functions,qr-set}.tex;
pubs/manuals/initechbase-volume-one/{u5-entries-b,u5-entries-c,u6-entries}.tex;
that volume's examples/c-list-2.txt and new examples/{c-list-off,c-setindex,
fn-recsize}.{dot,txt}; this report.

## Exact focused acceptance

For each row, both baseline recipes ended with the displayed summary and
`>>> <exact target>: green`. Both mutant recipes returned success after their
negative process exited 1 with the transcript assertion. Logs are
build/samirfns/fourth/{initial-host,initial-emu,red-host-next,red-emu-next,
accepted-host,accepted-emu}.log.

| Host target | Host tail | QEMU target | QEMU tail |
| --- | --- | --- | --- |
| test-samir-fns-index | test-samir-fns: 3 checks, 0 failures | test-samir-fns-index-emu | test-samir-fns: 9 checks, 0 failures |
| test-samir-fns-listoff | test-samir-fns: 3 checks, 0 failures | test-samir-fns-listoff-emu | test-samir-fns: 9 checks, 0 failures |
| test-samir-fns-recsize | test-samir-fns: 3 checks, 0 failures | test-samir-fns-recsize-emu | test-samir-fns: 9 checks, 0 failures |
| test-samir-fns-type | test-samir-fns: 3 checks, 0 failures | test-samir-fns-type-emu | test-samir-fns: 9 checks, 0 failures |

Named mutant targets, all GREEN at the recipe level:

- test-samir-fns-index-mutant / test-samir-fns-index-emu-mutant
- test-samir-fns-listoff-mutant / test-samir-fns-listoff-emu-mutant
- test-samir-fns-recsize-mutant / test-samir-fns-recsize-emu-mutant
- test-samir-fns-type-mutant / test-samir-fns-type-emu-mutant

Every host and guest mutant emitted the actual RED line:

    FAIL harness/diff/dbf_diff/test_samir_fns.c:81: FNS transcript byte-identical to real III+

Host witnesses (SAMIR_FNS_MUTANT_ prefix):

| Mutant | Actual first difference |
| --- | --- |
| index | byte 13, got 0x50, real 0x4c; lengths 24/62 |
| listoff | byte 18, got 0x20, real 0x0d; lengths 212/118 |
| recsize | byte 11, got 0x1a, real 0x0d; lengths 12/46 |
| type | byte 13, got 0x43, real 0x4e; lengths 35/35 |

The injected QEMU traces return normally, send all nine keys, and have no
triple fault. mtools provisions/extracts the FAT12 images; fsck.fat runs before
and after each guest session. All four behavior-mutant COM images build;
sizes are 98075, 98555, 98491 and 98555 bytes respectively.

The new target ends:

    >>> samir-fns-goldens: green (SF-index/listoff/recsize/type real III+ captures pinned)

## Ground truth and mint ledger

Read CLAUDE.md in full, PRD 6.6/8, WL-0148, samirdml-acceptance.md,
samirdml-mint.md, ADR-0009, WL-0127/0128/0152 and spec/samir/README.md.
bd show initech-ipu5.8.1 finds no live issue; the supplied brief and local
JSONL are the read-only issue record. ADR-0003 Appendix A remains the DOS
handle/process API contract.

Manual root: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus
1.1 (1986) Manuals/Using dBase III Plus.pdf. Printed pages rendered at 110 dpi
and visually read include U5-144 (PDF 256), U5-239 (351), U5-253 (365),
U5-249 (361), U5-261 (373), U5-190 (302), U5-111 (223), U6-18 (414),
U6-24 (420), U6-61/62/63 (457/458/459), U6-69 (465), U6-86/87 (482/483),
U6-91 (487), and DATE U6-20 (416). Implemented behavior cites U5-239,
U5-144, U6-69 and U6-86/87 respectively. IBM DOS 3.30 Technical Reference,
printed 6-200 (PDF 323), was rendered and viewed for the existing AH=4Ch
normal-return gate; file is dos33/80X0945_DOS_3.30_Technical_Reference_Apr87.pdf.

Real III PLUS 1.1 binaries came from /home/tobias/Projects/dbase3-decomp/mint/work,
copied into build only. fns_mint.sh produced the four complete captures with
both markers, no MINT-ERROR, and DOS EOF. The manifest pins their source PRGs
and complete GOLD.OUT bytes. These are fresh ledger rows, not authored goldens:

| Row | Capture under build/samirfns/golden | GOLD.OUT SHA-256 |
| --- | --- | --- |
| SF-index | index/GOLD.OUT | 02965a01b1325f3c712f511fbd399a9899da8ab2457a0975484371ad224e127c |
| SF-listoff | listoff/GOLD.OUT | 8bd874f565a88f5134d4788684ffd5e16103cd0cec6f1a0491840ae690b29f9c |
| SF-recsize | recsize/GOLD.OUT | d2fc6c9b5db0551dd1e773b46acc9894e863feabd3678adb174eab41d10b851a |
| SF-type | type/GOLD.OUT | d4735936cc949eeba0ca2d19d5ca9741cfa1b441ab8f30a7c70d4e7ee2f19e7d |

Other exploratory mints are not acceptance. Overflow required a normal-core
retry and then flushed a full real capture (SHA-256
42b26019830347a3a8880dfbbab994453b76618240c72085b00e8709e2d51e04).
It is still unimplemented. WITH's retained probe needs SET PROCEDURE to expose
its routines to real III+; its MINT-ERROR capture is not a golden. The separate
publication-data mint produced zero bytes with both auto and normal CPU cores;
the new book transcripts are required pubs-examples output, not independent
real-dBASE acceptance. No mint-impossible exemption is claimed for feature work.

## Stop: date reference contradicts the independent interpreter

CLAUDE.md's stop condition says a reference that fails its oracle is not to be
fixed autonomously. The focused real control is retained at
build/samirfns/fourth/datecontract/{FNSTEST.PRG,GOLD.OUT,sha256.txt}.
Its PRG SHA-256 is
918179ba5d0d57f2bcde9a911afaf1c6f545c8f2c4fef52de843c7bb273782b9;
its capture SHA-256 is
4767823771422fa6466055f316b4dde6f3b270b060634f724ecb51bbe72a250e.

With CENTURY ON, real III+ returns `[  /  /    ]` for a bracketed
DTOC(CTOD("")); with CENTURY OFF it returns `[  /  /  ]`.
CTOD("02/30/26"), CTOD("02/31/90"), CTOD("04/31/85") and
CTOD("02/29/85") return 03/02/1926, 03/03/1990, 05/01/1985 and
03/01/1985. The existing tests instead require space-only DTOC and blank
dates for the latter three cases (test_interp_setfmt.c:206/283 and
test_xbase_fn_a.c:252/274-276). The sibling dates-and-century.md:640-646
explicitly acknowledges that its space-only DTOC rule is inferred by analogy
from the later DTOS function. No date behavior, locked source or old assertion
was changed to conceal this conflict. The attempted display refactor was
restored before the retained acceptance runs.

## Size and reproducibility

The 106352-byte linker guard is unchanged. Loaded image 98587 bytes:
text 94354, rodata 4219, alignment 14; BSS 48504. Hard-budget headroom 7765,
but recovery against the recorded 106367-byte arrival is only 7780 bytes,
412 short of 8192. The stricter existing gate is honestly RED:

    RED SAMIR_SIZE: 98587 B > 98175 B reclaim cap / 102256 B headroom cap

No limit was relaxed and no size-mutant success is claimed with a RED baseline.
Two forced clean production builds compare byte for byte:

    >>> test-samir-size-repro: green (two clean builds byte-identical): e92022d889996d5850dcc493b3001038b8f5b9e551d8fb8b484bafda5b69ef39  build/SAMIR.COM

## Regression results and remaining REDs

Logs: build/samirfns/fourth/{regression-host,regression-emu,pubs-final,image-size,
repro,pubs-remint,golden-integrity}.log. GREEN: test-samir; test-dbase and its
round-trip/differential mutant families; test-samir-core (286), core-mutants
(all eleven), core-diff (104), core-diff-mutant; core-emu (18) and its three
mutants. All fifteen implemented DML host/QEMU baseline and mutant recipes
pass. RUN remains RED in both aggregate families, as expressly allowed:
PAL has no EXEC seam; VERSION.TXT is absent.

Other GREEN tails:

- test-samir-boot [5/5]: screendump shows SAMIR's dot prompt + LIST table on the seafoam desktop
- test-samir-write [5/5]: PERSISTED -- independent reader confirms 4 records, rec0 BAL=9999.99, rec3 BAL=5555.55 ON DISK
- test-samir-canon-y2k [4/4]: screendump shows the AR aging report on the seafoam desktop
- test-flair-app-launch: green (all ten legs)
- test-flair-finder-service: green (both volume legs)
- test-pubs-samir-programs: green (the month-end programs printed as the reference programs, with their reference output)
- pubs-examples: 258 transcripts re-minted
- image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)

Unresolved, not waived:

1. Date reference/oracle conflict above. Remaining SET/function/error features
   and the complete twelve-defect certificate are unfinished. No completion
   claim for PROCEDURE, FUNCTION, ESCAPE/BELL/CONFIRM, DEFAULT/PATH, ALTERNATE
   failure/lifetime behavior, ROW/COL/TIME/DISKSPACE/PROW/PCOL/OS/VERSION,
   RELATION/TALK, event traps, WITH, command-level memo, status branding,
   century transport, overflow or the end-to-end disk-full catalog path.
   Existing core memo and typed-write code was regression-tested, not newly
   completed at the command/differential level.
2. test-interp-set:336 still asserts successful inert SET INDEX without an
   active table. The implementation now refuses it. The standalone real
   no-table control did not flush a capture; no arbitrary replacement golden
   or assertion was installed. Full INDEX failure-path acceptance is unfinished.
3. test-samir-size fails the reclaim cap by 412 bytes, as above.
4. The publication legs stop at pubs/class/initech-xbase.sty:83:
   `LaTeX Error: Command \\MakeBackCover already defined.` The other definition
   is in initech-forms.sty. test-pubs-mutant additionally stops on the retained
   CRLF in pubs/examples/samir/salary.txt:
   `!!! test-pubs RED ASCII: pubs/examples/samir/salary.txt: non-printable/non-ASCII byte`.
   Shared classes and that pre-existing transcript were not edited.
5. test-flair-locked-purge reproduces the known stale UNSELECTED oracle:
   `ppm_flair_trash_check: FAIL leg locked-full -- FULL Trash (TRASH_FULL): 617/1024 cells differ from the mint-008 capture`.
   Read main's WL-0158 and confirmed its SELECTED routing fix. The requested
   fetch updates pm/main to 4755d030; `git merge pm/main` fails exactly:
   `fatal: update_ref failed for ref 'ORIG_HEAD': cannot lock ref 'ORIG_HEAD': Unable to create '/home/tobias/Projects/initech-os-samirfns/.git/ORIG_HEAD.lock': Read-only file system`.
   The merge applied no source changes. No tolerance or oracle was loosened.

The working tree remains uncommitted. git diff --check passes; the touched C
sources/header contain no non-ASCII bytes. This is a stopped partial lane,
not the requested final certificate.
