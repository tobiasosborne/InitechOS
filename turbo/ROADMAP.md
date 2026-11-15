# Turbo Initech: remaining golden-master work

The seed is C on the host; Turbo Initech and its programs are Pascal. One
TURBO.COM opens the minimal blue IDE without arguments and compiles a source
filename with an argument. The IDE edits, saves through published DOS calls,
compiles, locates the first error, runs a freshly compiled flat program through
EXEC, returns to the editor and preserves the user screen for Alt-F5.
MINT06/07/08 real TP 5.5 captures specify its screens and saved bytes.

Mechanical checks:

- Tier 0 runs 8 seed, 44 compiler and 15 IDE entries plus assertion controls.
  Nineteen flat and twenty extended native containers match independently
  assembled/linked output. Runtime tables and intrinsic offsets match NASM/ld.
  CLI ordinary/dump mode, invalid diagnostics, DOS-saved compiler source,
  output-name alias refusal, paged I/O failures and save conservation are checked.
- Product smoke runs 19 corpus programs plus 19 regressions through five paths
  on QEMU and Bochs: 380 executions with independent FPC stdout/file oracles.
  IDE journeys compare 52 captured screens per emulator and exact source/backup
  bytes. The native screen reader checks actual DOS page cells independently;
  injected keys exercise no-argument startup and the integrated commands.
- Selfhost compares complete K2 and K3 containers from fresh OS boots on both
  emulators. DDC uses an FPC-built bootstrap and requires both in-OS D2 results
  to equal K2/K3 across emulators. The parent README records the current proved
  source, container size, SHA-256 and timings. Corruption, changed-source and
  deadline controls reject their intended failures. Ordinary compile and proof
  runs create no factory trace or assembly dumps.

Remaining item-7 requirements are tracked explicitly:

- initech-311t2 (kernel): supply the DOS 3 executable-name environment trailer
  required by ParamStr(0), including nested EXEC. The independent raw-PSP probe
  exposes the missing trailer. Other command-tail and EXEC values agree with
  MINT07. No new interrupt function is requested.
- initech-wkp2p stays open until that runtime requirement is green. The compiler
  and IDE portion is implemented; the standalone runtime probe remains honest
  about the external failure.
- initech-cd2yi: runtime-encoder authorship remains separate from the current
  checked-in, independently regenerated NASM tables.
- initech-ry6ge: complete the final product boot-to-IDE-to-self-reproduction
  acceptance session and coordinate the delivered compiler, exact proved source
  and demonstration program with the system owner. Existing proof and IDE
  journeys do not claim this final combined delivery session.

Evidence remains under build/turbo/selfhost, build/turbo/ddc and
build/turbo/smoke. Compiler/NATIVE.md describes native encodings and trust.
The historical resident lex/parse/type protocol journeys are inventoried in
compiler/tests/SMOKES.md; the new native proofs do not claim those old imports.

Semantics remain bounded: signed 32-bit integers, booleans, chars, ShortStrings,
static arrays, named scalar records and untyped byte files. No pointers/heap,
reals, units, sets, objects or general Pascal compatibility is claimed.
The loader supplies a separate stack and guard; current checks enforce placement
limits without claiming a static worst-case stack bound. Scheduling is cooperative.
