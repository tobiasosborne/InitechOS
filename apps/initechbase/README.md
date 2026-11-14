# InitechBase

InitechBase provides a dBASE III PLUS 1.1 command interpreter with DBF tables,
DBT memos, NDX indexes, ten work areas, command programs and a dot prompt.

`make` builds INITBASE.EXE and INITBASE.OVL. `make test` runs 77 independent
host suites with a 30-second execution budget, excluding compilation, and
validates the real target pair. `make test THOROUGH=1` adds the live genuine
III+ differential. Missing references fail. `make pending CASE=...` exposes
unfinished captures; it is outside the passing selection.

`make smoke` drives the one product image on QEMU and Bochs at 8 MiB, reads
files back with mtools/independent readers and reopens them with genuine III+.
It passes 50 checks. Scenarios and the selected application pair live on a
separate disposable TEST disk; no test scenario is compiled into the kernel.
`make native-check` runs the DOS console, file, program-return and overlay
oracles. `make mutants` runs 250 host patches and 19 target patches, including
three static semantic rejections. All 28 original pending faults are qualified.
The test controls themselves must receive zero kills.
Host kills require completed assertion counts and identical failure evidence
on two runs. Crashes, timeouts and missing fixtures are test errors. Recursive
source growth is measured after normal completion, without counting a crash.

Core, command and codec sources include only this component's headers. The
PAL vtable is the platform interface; pal_dos uses INT 21h and CF/error returns
for DOS services. Its console-page binding still uses private INT 39h; issue
initech-qu7rx tracks its replacement when the kernel's public INT 21h service
is available. The migration preserves the 4002-byte page payload and exact
screen behavior.
Pure libraries linked into INITBASE.EXE or INITBASE.OVL: none.
The host binding is never linked into the product. Dependencies:
kernel/include/memory_map.h and initechmz_extended.h for the DOS PAL and
packaging contract; factory/mz for mzlink/mzcheck; factory/testlib and emulator
drivers for judging. No SDK or Toolbox dependency and no build input from
attic. Python is used only for independent foreign-format readers.

## Loader and overlays

The resident image, six overlays, BSS, PAL heap tail and fixed slot are checked
by the linker, overlay packer, independent MZ reader and 50 pair checks.
`make package-check` reports current geometry; artifact sizes are not frozen.
IBASE_LOADER_RESERVE follows the public 262144-byte module ceiling, covering
the slot, alignment, BSS and PAL tail. tools/ibase_memory_ld.c generates both
linker symbols and descriptor arguments. The old 65536-byte reserve was too
small; the kernel cannot infer this runtime tail from serialized module bytes.

A module may call resident code but cannot name another module. Resident code
cannot retain module code/data, and a live module cannot evict its return
address. The target linker and relocation call graph enforce static boundaries;
runtime guards reject escaped pointers before they outlive the slot. Modules
are stamp/CRC/geometry/relocation checked. Missing/corrupt media receive the
documented retry/abort refusal. Its text follows the real executable's string;
exact missing-OVL runtime parity with III+ is not claimed.

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
DO supports disk files, procedure libraries and WITH arguments. Program and
statement storage grows within available memory; captured 8192-byte programs,
513 statements and 255-byte source lines execute. Trailing semicolons continue
logical statements. The 20-level DO cap remains; memory exhaustion can refuse
earlier. Disabling that cap is not supported.

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
live-date century transport. SET INDEX preconditions,
mutation NEXT scopes and bounded LOCATE misses still need independent capture
qualification. Accepted captures stay unchanged. The smoke's LIST and INDEX/
SEEK lines match real III+; its Y2K report uses the authored canon golden:
CTOD('01/31/00') is 1900, A1001 ages -36477 days, and overdue total is 0.00.
The period rollover bug is enforced.

## Write preservation

The operator's guarantee governs reported failures; this is not crash atomicity.
DBF flush stages the complete image, including +2-to-+1 header normalization.
Incremental NDX changes and DBT append/flush also stage complete images.
COPY FILE, SAVE TO, REPORT, LABEL, ALTERNATE and index builds retain predecessors
through their last fallible validation. Open-table/memo/index output aliases are
refused. ALTERNATE publishes on close and reports write/close failures. Real
III+ full-disk COPY/SAVE captures are destructive; that behavior is not copied.
Sources and detailed fault coverage are in tests/write-safety/README.md.

REPLACE, APPEND BLANK and each APPEND FROM row retain the DBF and every index
predecessor until that record is complete. Earlier completed source rows remain
if a later import fails. PACK holds the table and all indexes through rebuilding
and final navigation. Key extraction reads physical rows without consulting the
obsolete old sequence. REINDEX likewise discards stale navigation only after
validation. The genuine C21 repair compares both rows, both seeks and recnos.

Refused DELETE, RECALL, ZAP and PACK discard rejected arena edits, restoring
record count/cursor and invalidating navigation. Recovery can grow PACK's arena
back to its pre-compaction size. An unreadable predecessor latches further
flushes until a reload succeeds. Fault tests check the next unrelated write,
not just the file snapshot at the reported error.

DOS 3.30 offers no atomic replace through INT 21h. If installation and restoration
both fail, the operator's explicit review ruling applies: InitechBase prints
`Previous file retained as: <exact pathname>` and deletes nothing. The old BAK,
new stage and installed replacement remain available; the original name can be
absent. Kernel initech-7mb66's primitive is not an INT 21h service and is not
called. This metadata double-failure residual remains explicit.

## Independent checks and remaining work

Tier 0 compares real period captures, literal original file bytes and independent
format/property checks. DBF comparisons normalize only volatile header date and
reserved bytes. dbsafe's exhaustive write/short-write/seek/close/rename/open/read/
allocation matrices cover restored handles, retries, multiple indexes and
completed import prefixes. Case selection is tests/conformance/accepted.cases.
Thirty-five conformance captures pass. C33's disk-DO file-budget repair remains
blocked by kernel initech-kyrcw (CLOSE of standard JFT slot 3 falsely succeeds);
its genuine PSP/JFT capture and app checkpoint are recorded in the phase report.
Its expected limit is not changed to fit the kernel defect.

The genuine-III+ leg of smoke reopens target DBF/NDX, C21's repaired pair, a
host-written memo pair and the +2 table preserved by an injected flush failure.
It checks exact values, counts and seeks. W4 editor/file comparators remain
self-tests, not qualification of unimplemented editors or GET/READ.

Native console checks compare all 4002 character/attribute/cursor bytes against
real supported TE/Enter screens and grade literal redirected SAY cells. The
QMP observer first proves its copied ELF matches the booted system binary.
Color checks use the documented CGA/HELP.DBS properties and do not claim a real
SET COLOR VGA capture. File checks grade exported C21 data; stream checks retain
every internal CR/LF from the complete genuine depth-with transcript. The DOS
return marker must be an executed ECHO, not its input echo. The stack fault emits
the correct full transcript but fails normal return with UD6/HALTED.

Mutation faults live only in patches. Host kills compile and fail the same
independent runtime suite twice. Target static faults must compile every object
and receive the specific linker/call-graph rejection twice. Native faults require
the same independently observed rejection twice. Corrupt/missing media and
retained pointers have explicit format/lifetime oracles. Generic compiler errors,
incomplete guests and timeouts alone earn no kill. No-fault, compiler-error and
native no-fault controls must receive zero kills. Seventy-two active faults were
refreshed contextually after source refactoring and the complete sweep reproved.
See tests/native and mutants/target for the exact target contracts.

Unclaimed legacy emulator journeys remain with the InitechBase owner in
tests/SMOKES.md. The imported helpers build only here; nothing builds from attic.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: dos
Interface migration: base-console39 initech-qu7rx
Pure libraries: none. The PAL console-page INT 39h site alone is pending the
published INT 21h replacement, preserving the 4002-byte payload.
Interface use-records: kernel/include/initechmz_extended.h
Interface use-records: kernel/include/memory_map.h
Interface compiler-runtime: __divdi3
<!-- end-interface-declarations -->
