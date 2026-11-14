# InitechBase

InitechBase provides a dBASE III PLUS 1.1 command interpreter with DBF tables,
DBT memos, NDX indexes, ten work areas, command programs and a dot prompt.
This import adds packaging and boot validation; it adds no language features.

`make` builds INITBASE.EXE and INITBASE.OVL. `make test` runs the independent
host corpus (76 suites, 30-second execution budget, excluding compilation).
`make test THOROUGH=1` adds the live real-dBASE DOSBox differential. Missing
references fail. `make pending CASE=...` exposes unfinished captures honestly;
it is not part of the passing selection. `make pair-test EXE=/absolute/file.EXE`
runs the 50 container/overlay checks. `make smoke` builds the product image,
then drives the DOS session on QEMU and Bochs at 8 MiB. Test fixtures live on a
separate TEST disk; the product working volume contains only the EXE/OVL pair.
Logs and mtools readbacks are under build/apps/initechbase/smoke.

Core, command and codec sources include only this component's headers. The
PAL vtable is the only platform interface; pal_dos uses INT 21h, flat pointers
and CF/error returns. The host binding is never linked into the product.
Dependencies: kernel/include/memory_map.h and initechmz_extended.h for the DOS
PAL and packaging contract; factory/mz for the generic mzlink/mzcheck tools;
factory/testlib and emu drivers for judging. No SDK or Toolbox dependency and
no build input from attic. Host tests retain the inherited independent Python
DBF/DBT/NDX readers; new orchestration is C, make and POSIX shell.

## Loader and overlay contract

The imported baseline resident module is 107744 bytes with 792 relocations; INITBASE.EXE is
110976 bytes, including its relocation header, and INITBASE.OVL is 39424 bytes.
Above the resident bytes, the link needs a 13968-byte overlay slot, 16 bytes
of BSS alignment, 47680 bytes of BSS, and a 3952-byte PAL heap region: 65616
bytes total. Startup zeroes BSS; the PAL initializes that fixed first region
and obtains its separate 32912-byte scratch/codec allocation through AH=48h.

The old app-owned 65536-byte reserve was 80 bytes short. App policy now gives
the runtime tail the same budget as a maximum module: IBASE_LOADER_RESERVE
follows INITECHMZ_EXT_MODULE_MAX (262144 bytes), with 196528 bytes of tail
headroom. It leaves 1657616 usable bytes in the DOS arena (1624704 above the
required minimum). tools/ibase_memory_ld.c generates both the linker symbols
and the generic MZ descriptor arguments from include/ibase/memory_contract.h.
boot/ibase.ld checks the actual tail, module ceiling and complete footprint
against the high-memory guard and minimum heap. factory/mz/mzcheck checks the
container independently. No kernel change or size-squeezing flags are needed.

Ground truth: kernel/mz.c reads reserve_bytes from the extended descriptor;
kernel/loader_validation.c:loader_high_plan places the allocation arena at
ALIGN(EXT_APP_BASE + module length + reserve_bytes,16). It cannot infer BSS.
The reserve protects our entire runtime tail from that arena, independently
of LOAD_STAGING_MAX, PROGRAM_BSS_RESERVE and graphical tenant limits.

Six overlay entries reuse one fixed slot; on-disk overlay bytes do not live
in BSS. Startup validates the matching file, and the manager checks lifetime,
stamp, CRC and relocations when loading it. It is separate from FLAIR. Removing
it would require resident dispatch and new lifetime/packaging proofs, outside
this import. Target flags retain soft-float, section isolation and the shipped
FLOW_MAX_REGISTRY=1/FLOW_MEMVAR_ARENA=6000 profile with the default C ABI.

## Frozen feature inventory

This is the present subset, not a promise of complete III+ compatibility.
Sources: cmd/*_verbs.def, cmd/set_options.def, core/fn_builtins.c:fn_names,
the accepted host capture lists, and attic/triage/triage.md's Base findings.

54 dispatch spellings (including aliases and punctuation):

- Query: LIST, DISPLAY, ?, ??, LOCATE, CONTINUE, SEEK, FIND, GO, GOTO, SKIP,
  @ (SAY, not GET), EJECT, CLEAR.
- Files/data: TYPE, RENAME, ERASE, COPY (FILE), RUN, !, SORT, TOTAL, INDEX,
  REINDEX, REPLACE, APPEND (BLANK/FROM), DELETE, RECALL, PACK, ZAP.
- Variables/areas: SAVE, RESTORE, STORE, SELECT, RELEASE; USE, CLOSE.
- Programs/I/O: DO, PROCEDURE, PARAMETERS, PARAMETER, RETURN, RETRY, CANCEL,
  QUIT, PUBLIC, PRIVATE, ACCEPT, INPUT, WAIT, ON (ERROR/ESCAPE); SET, REPORT,
  LABEL. ON KEY is accepted but inert, a known defect.

The dot prompt also handles EXIT and RESUME directly (RESUME without a
suspended program reports #101, `Not suspended.`).

Assignments, macro substitution, comments, IF/ELSE/ENDIF, DO WHILE/ENDDO,
DO CASE/CASE/OTHERWISE/ENDCASE, LOOP and EXIT are structural program syntax.
DO supports disk files, procedure libraries and WITH arguments. Program limits
include 8191 bytes, 512 parsed lines and a 20-level DO cap; memory exhaustion
can refuse earlier. The cap is production behavior; disabling it is not supported.

68 registered functions:

RECSIZE ERROR MESSAGE UPPER LOWER TRIM RTRIM LTRIM SUBSTR LEN SPACE CHR ASC
STR VAL CTOD DTOC DATE DAY MONTH YEAR TYPE ABS INT MOD ROUND MAX MIN CDOW CMONTH
DOW RECNO RECCOUNT EOF BOF FOUND DELETED FIELD DBF FILE LEFT RIGHT STUFF REPLICATE
AT ISALPHA ISUPPER ISLOWER TRANSFORM EXP LOG SQRT TIME LUPDATE NDX ROW COL PROW
PCOL INKEY READKEY LASTKEY ISCOLOR FKLABEL OS VERSION GETENV DISKSPACE.

28 SET spellings (27 distinct options because HEADING/HEADINGS are aliases):

ALTERNATE EXACT DECIMALS FIXED DATE CENTURY ORDER INDEX DELETED FILTER RELATION
TALK SAFETY ESCAPE HEADING HEADINGS CONSOLE STATUS SCOREBOARD COLOR DEVICE
UNIQUE MARGIN PRINT MEMOWIDTH HISTORY DOHISTORY PROCEDURE.

RELATION and TALK currently store intent without the corresponding runtime
behavior. The other 26 spellings have implemented effects, within the captured
subset; pending history, numeric formatting and memo layout cases limit parity.

Omissions and current refusals (the catalogue text is retained verbatim):

- CALL, LOAD, IMPORT, EXPORT, CREATE, MODIFY, EDIT, BROWSE, ASSIST, READ,
  COUNT, SUM, AVERAGE, JOIN, UPDATE, INSERT, TEXT, DIR: #16,
  `*** Unrecognized command verb.` These have no command rows. The message
  is generic, not yet an individual named refusal.
- SET DEFAULT, PATH, FIELDS, VIEW and other unregistered SET options: #10,
  `Syntax error.` Unknown function names: #31, `Invalid function name.`
- COPY TO is outside the COPY FILE subset; unrecognised supported-verb
  clauses use #36, `Unrecognized phrase/keyword in command.`, or #10 depending
  on the parser. @ GET/READ forms and interactive table/schema editors do not
  exist. CLEAR FIELDS/GETS are accepted with nothing to clear.
- Unhandled program errors return to the dot prompt; the real III+ traceback
  and Cancel/Ignore/Suspend recovery dialog are absent.

Known parity limits, not silently promoted pending captures: STR scientific
fallback/range/precision and rounding, exponent pictures, HISTORY growth and
recording/paging, MEMOWIDTH wrapping/column padding, multi-area DISPLAY pauses,
empty-table BOF, unique-index incremental duplicate handling, error routing,
live-date century transport, and memo-reader refresh. SET INDEX preconditions,
mutation NEXT scopes and bounded LOCATE misses still need independent capture
qualification. Accepted captures stay unchanged. The smoke's LIST and INDEX/
SEEK lines match real III+; its Y2K report uses the authored canon golden:
CTOD('01/31/00') is 1900, A1001 ages -36477 days, and overdue total is 0.00.
The period rollover bug is enforced.

## Mutation and migration evidence

Product faults live only as zero-fuzz patches under mutants/. `make mutants`
groups identical host selections and invokes the shared strict factory runner
with four workers (MUTANT_JOBS overrides; MUTANT_SERIAL=1 uses mk/mutants.mk
alone). Where available, ccache caches sandbox host compiles without debug
information, keeping scratch paths out of the cache key. Product flags and the
ordinary host oracle keep their normal settings. Each selection has one passing
baseline; every kill must compile and
fail the same runtime suite twice. Pending patches are excluded, with their
specific missing oracle recorded in mutants/pending/README.md. No survivor is
reported as killed.

`sh tests/smoke/red.sh` drops REPLACE's write, boots the same session, requires
the independent DBF readback to reject it, and restores/rebuilds the product.
The EXE and overlay hashes are compared across mutation-hook removal.
The remaining legacy emulator scenarios are listed in tests/SMOKES.md;
tests/IMPORT.md records host provenance and unfinished migration work.

Qualification: 258 original fault names (including nine aliases), plus the
write-loss control: 231 active patches killed, 28 pending. The four-worker
`make mutants` sweep takes 208 seconds with the local cache. Product bytes
before and after hook removal are identical:

- INITBASE.EXE: `5a713213ab16795e8f9efd24ad0ce9121db34fb58ae544a2b25983aadd0f7501`
- INITBASE.OVL: `015fd792d0b0d458761c956024250a41a620c53e6dc1fff1e9cad11b8929db1d`

The product smoke passes 44 checks. The top-level parallel test run is ALL
GREEN (11 components, 89 seconds); InitechBase executes its 75 suites in nine
seconds. `make status` reports apps/initechbase as imported. This establishes
the import conditions and the named DOS journey, not the deferred parity and
screen qualification listed above.

## Write preservation and reference qualification

DBF flush and committed APPEND serialize to a separate file, including +2 to
+1 header normalization. COPY FILE (including self-copy), SAVE TO, REPORT,
LABEL, ALTERNATE and index builds likewise close a completed stage before
replacing their destination. An output alias of an open table, memo or index
is refused. ALTERNATE publishes on close and reports output/close failures.
The policy is the operator's failed-save guarantee; the real III+ full-disk
COPY/SAVE captures are destructive. Sources, precise fault coverage and the
remaining repeated-metadata-failure limitation are in
[tests/write-safety/README.md](tests/write-safety/README.md).

The host SAY suite executes three captured supported @...TO workflows and
compares target character, attribute and cursor dumps to real III+. W4's
retained editor screens and file comparators are explicitly self-tests. They
still do not qualify the unimplemented editors or traceback UI. The smoke
also starts genuine III+ to reopen target DBF/NDX, a host-written memo pair,
and the real +2 table preserved by an injected flush failure. It checks exact
values, record counts and seeks. GET/READ is unchanged.

The artifact sizes and hashes earlier in this README describe the imported
baseline, before this change. `make package-check` reports the current loader
geometry and validates the actual EXE/OVL pair; no size or hash above is a
claim about the current build. The 28 pending patches remain unproved.

The C19/C20/C21 conformance repair selection is now part of tier 0 (76 suites).
Its real III+ fixtures come unchanged from the conformance study. An old NDX
sequence is sized by its own entry count, including removed DBF records; explicit
GO BOTTOM on the captured missing record raises #4 and leaves blank fields at
EOF. REINDEX obtains keys in physical DBF order without consulting that old
sequence, then invalidates navigation state. C21's transcript, the edited row 2
and the rebuilt two-entry NDX match real III+; its booted smoke readback checks
the same three results on both emulators. The study's other fifteen defects
and all 28 pending patches remain assigned to the next owner.

INDEX and REINDEX retain each predecessor through their final reopen, header,
attachment and navigation checks. An error restores the old file and index
metadata; successful validation alone releases the backup. Fault tests exercise
both the public REINDEX API and the commands, including full attachment slots
and allocation failures. Incremental NDX updates and DBT saves remain outside
this proof; initech-hls50 and initech-4qwvm belong to the second owner.

DOS 3.30 cannot atomically replace files through INT 21h. Under the operator's
review ruling, if installation and restoration both fail, InitechBase prints
`Previous file retained as: <exact pathname>` for the surviving predecessor.
It deletes nothing in that double-failure case: the old BAK, new stage and any
installed replacement remain available. The original name can be absent; this
is the explicit recovery residual until suitable DOS support exists. The kernel
primitive tracked as initech-7mb66 is not an INT 21h service and is not called.

The genuine-III+ leg of `make smoke` also reopens C21's REPAIRED.DBF and
STALE.NDX, checking both physical rows (A/1 and B/9), count two, and both
indexed seeks with exact record numbers/values. `reopen-red.sh ... c21` corrupts
only the serialized repaired row and requires a completed genuine run to reject
it. This emulator coverage belongs to tier 1; tier 0 remains emulator-free.
