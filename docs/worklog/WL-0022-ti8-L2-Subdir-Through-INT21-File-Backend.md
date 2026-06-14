# WL-0022 -- ti8 Layer 2: subdir resolution through the INT 21h file backend

Bead: **initech-mzxa** (CLOSED). Follow-up filed: **initech-zs24** (subdir EXEC + subdir WRITE).
Depends on: initech-ti8 (Layer 1: `fat12_dir_t` + `fat12_read_dir` + `fat12_resolve_path`, READ side).

## Context

Layer 1 (`command-com-default @ 99b84bd`) exported the FAT12 subdir READ primitives
but nothing consumed them: the INT 21h file backend (`int21_file_backend_t`) was purely
root-relative -- OPEN/CREAT/UNLINK/FINDFIRST took only a bare 8.3 name and the four
file/find sites + do_exec REJECTED any `\` / `:` path with AX=0x0003. Layer 2 (mzxa)
threads a resolved containing-directory cluster (a `uint16` start_cluster, 0 == root)
through the vtable so `\SUB\FILE` paths resolve.

## What changed

**Artifact (C):**
- `int21.h` -- `int21_file_backend_t` extended ADDITIVELY: `open()/create()/dir_entry()/
  unlink()` gain a `uint16_t dir_start_cluster` param (0 == root => byte-identical to
  before); a new `resolve()` member (the path->containing-dir seam). New CWD seam:
  `int21_cwd_reset/save/restore` + `int21_cwd_snapshot_t` + `INT21_CWD_MAX`.
- `int21.c` -- `g_cwd_path` + `g_cwd_start_cluster` file-statics (read-side; CWD stays
  root until u6wa's AH=3Bh CHDIR writer). `resolve_dir_path()` strips a leading `X:`
  drive (int21.c owns the drive -- a volume concern) then calls the backend resolve;
  the 4 file/find sites (do_open/do_creat/do_unlink/do_findfirst) resolve a subdir path
  and pass `(leaf, dir_start)` to the backend. do_findfirst builds the search template
  from the LEAF and stores `dir_start` in `g_find` (threaded into dir_entry()). do_getcwd
  reports `g_cwd_path`. do_terminate resets the CWD. AX=0x0003 PATH_NOT_FOUND preserved
  at every rejection site (+ the AH=59h do_geterr mapping). The overlength fail-loud +
  the `INT21_MUTATE_PATHSCAN_NOBOUND` seam are preserved (new `path_overlength()` helper
  shares the bound).
- `fileio_fat.c` (real kernel backend) -- `fat_resolve()` (split leaf, resolve the parent
  chain via `fat12_resolve_path` with a trailing `\`, gate on `DIR_ATTR_DIRECTORY` so a
  file mid-path is rejected); `locate_in_dir()` (root via `fat12_find_slot`, subdir via
  `fat12_read_dir`); `fat_open/fat_dir_entry` honor `dir_start_cluster`. `fat_create/
  fat_unlink` return 0x0003 for a non-root dir (subdir WRITE is out of READ-side scope).
- `loader.c` -- `int21_cwd_reset()` on program launch (beside `int21_mcb_reset`).
- `kmain.c` -- save/restore the CWD around the 5 kernel-context PSP rebinds (the EXEC
  adapter + run_baked + EXEC-SAW/EXITH/SYSINIT blocks) so a child's CWD never leaks.

**do_exec decision (decision 3): KEPT ROOT-ONLY + follow-up filed.** The EXEC source
feeds the loader's `load_program_from_fat -> load_program` path, which locates the .COM by
a ROOT-dir 8.3 name. Threading a resolved cluster into the loader is a riskier
loader-internal change than the clean read-side resolve, so do_exec keeps rejecting subdir
paths (decision 3 explicitly authorized this) and **initech-zs24** tracks subdir EXEC +
subdir WRITE together.

## Oracle (Law 2 / Rule 6)

- **Unit:** `test-fileio` -- the mock now models a nested SUB/NESTED.TXT namespace + the
  resolve seam (107 checks, was 89). Asserts OPEN `\SUB\NESTED.TXT` resolves+reads;
  `\SUB\MISSING.TXT` -> 0x0002 (dir exists, file absent -- DOS-correct, NOT 0x0003);
  `\NODIR\X` + `\SUB\NESTED.TXT\X` -> 0x0003; `A:\SUB\...` drive stripped; FINDFIRST in
  `\SUB`; do_getcwd reports root (buf[0]==0).
- **Integration (decisive, real backend):** `test-mzxa-integration` (new
  `test_fileio_subdir.c`) binds the REAL `fileio_fat.c` over `build/fat12_nested.img` via
  `blockdev_file` and drives int21 OPEN/READ of `\SUB\NESTED.TXT` + `\SUB\DEEP\DEEP.TXT`
  through the WHOLE DOS-API -> resolve -> `fat12_resolve_path` stack, byte-for-byte vs the
  fixtures (45 checks). FEASIBLE and green.
- **Mutants:** `test-mzxa-mutant` -- `INT21_MUTATE_RESOLVE_NODRIVE` (skip the `A:` strip ->
  the drive case fails) and `INT21_MUTATE_RESOLVE_NOTROOT` (force dir_start=0 -> `\SUB`
  looks in root) both drive the unit oracle RED, then restore GREEN.
- Both new gates added to `TEST_UNIT_GATES`. Full `make test` = **65 host + 22 emu = ALL
  GREEN**; `test-kernel-repro` confirms kernel.bin still byte-identical (root==start_cluster
  0 preserved byte-for-byte).

## Frictions

- The brief's "`\SUB\MISSING` -> 0x0003" is DOS-imprecise: a missing FILE in an existing
  dir is 0x0002 (file not found); 0x0003 (path not found) is a missing/non-dir
  intermediate. The oracle asserts the DOS-correct codes (and the real backend agrees),
  covering both cases explicitly.
- `fat12_resolve_path("\SUB\NESTED.TXT\")` resolves to the FILE (attr 0x20), not a dir, so
  `fat_resolve` must gate on `DIR_ATTR_DIRECTORY` to reject `\SUB\NESTED.TXT\X`.
- The NODRIVE mutant only bites the MOCK unit oracle (the real `fat_resolve` defensively
  re-strips the drive). That is intentional layered robustness; the unit oracle is the
  NODRIVE gate.

## Acceptance

`make test` ALL GREEN. Backend signature change compiles across int21.h + fileio_fat.c +
all 3 host mocks in lockstep. Existing test-fs / test-program / file-handle / exit-handles
emu gates green (byte-identical root behavior).

## Pointers

- Resolve seam: `int21.c::resolve_dir_path`, `fileio_fat.c::fat_resolve` / `locate_in_dir`.
- CWD plumbing: `int21.c::int21_cwd_{reset,save,restore}`, `loader.c:296`, `kmain.c` 5 sites.
- Oracles: `os/milton/test_fileio.c` (unit), `os/milton/test_fileio_subdir.c` (integration),
  Makefile `test-mzxa-mutant` / `test-mzxa-integration`.
