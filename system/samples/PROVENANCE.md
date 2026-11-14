# Installed examples

These are Initech's own content, never vendor demonstration databases.
`system` owns the installed examples and the deterministic factory producer
`samples.c` / `install-inputs.sh`. No reference program or test driver ships.
All manifest entries have ordinary archive attributes, no H/S/R attributes.
Installed locations: the Base/TPS data and program are in root SAMPLES;
the compiler, its full source and Hello inputs are in root TURBO. The operating
directions are root README.TXT. APPS retains its original fourteen entries,
so every existing desktop application remains in the default visible grid.
The image builder fixes FAT dates to 1980-01-01. DBF update dates generated
here are 1993-07-19; the recovered TPS table retains 1989-10-16.

| Installed input | Producer and provenance |
|---|---|
| CLIENTS.DBF | `samples.c`; the Initech-authored NAME C(10), CITY C(12), BAL N(8,2) sample in `apps/initechbase/tests/mint_clients_dbf.c`. PESTON/HONOLULU/1234.50, WADDAMS/AUSTIN/-42.00, LUMBERGH/DALLAS/7777.77. Real III+ LIST capture: `apps/initechbase/tests/xbase_prog_diff/screen-mint/boot-list-real/list.screen`. |
| PARTS.DBF | `samples.c`; Initech-authored CODE C(3), QTY N(5), OK L sample in `apps/initechbase/tests/xbase_prog_diff/prog_diff.c`. AAA/100/T, BBB/200/F, CCC/100/T, DDD/300/T; real III+ report `mint/prog-mint/query/REAL.OUT` in that test directory. |
| INVOICE.DBF | `samples.c`; schema and values from `apps/initechbase/tests/test_canon_y2k.c` and `tests/smoke/fixtures.c`. The first account is spelled INITECH in this distribution; amounts/due dates/paid flags remain the canon inputs. The stored 1900 dates are intentional. |
| Y2KACCT.PRG | `install-inputs.sh` converts this directory's source to CRLF. Body preserved from `apps/initechbase/tests/canon/y2k_accounting.prg`; initial `STORE CTOD('01/31/00') TO ASOF` from the existing standalone driver. Only the period comment banner is new. `USE INVOICE` then `DO Y2KACCT` must retain the negative ages and 0.00 overdue total in `y2k_accounting.out`. No test commentary ships. |
| TPS.DBF / TPSPARM.DAT | Recovered byte-for-byte from read-only `/home/tobias/Projects/InitechOS-legacy/harness/diff/tps/{tps.dbf,tpsparm.dat}`, `legacy-final` c5d18538. Table originally created by real dBASE III PLUS 1.1 using `tpsmk.prg`; original mint normalized the non-meaningful header year to 1989. Parameters are TITLE=TESTING PROCEDURE SPECIFICATION, DIVISION=SOFTWARE ENGINEERING, MEMO=88-114, LINES=6. The same recovered bytes survive in `dos/tpsrep/tests/reference/`. SHA256SUMS pins both imported inputs; the build never reads legacy or attic. |
| HELLO.PAS / initial TURBOIN.PAS | New Initech introductory program, `Writeln('Hello from Turbo Initech.')`; CRLF conversion by `install-inputs.sh`. Initial input is an exact copy. This follows Turbo's fixed input/output protocol, documented in `turbo/compiler/turbo.pas` and `turbo/tests/smoke/compile.sh`. |
| TURBO.PAS | Exact copy of `turbo/compiler/turbo.pas`, the complete single-file compiler including embedded runtime tables; no include dependencies. Turbo owns the source and its compiler artifact, `build/turbo/compiler/TURBO.COM` (`make -C turbo`). No host compiler ships. |
| README.TXT | New Initech operating directions, CRLF conversion by `install-inputs.sh`; the commands above and existing TPS `keys_ok.txt` ritual. |
| CONFIG.SYS / AUTOEXEC.BAT / HELLO.TXT | `install-inputs.sh`; preserves the existing factory/image/build.sh startup and greeting, adding root and APPS to PATH so programs resolve from SAMPLES. C: consolidation remains the delivery/factory owners' work. |

The first three DBFs use the documented dBASE III byte layout in
`apps/initechbase/spec/ibase/dbf.md`, independently read by
`system/tests/read-samples.py`. None has a memo field or opens an index;
therefore no DBT/NDX companion belongs to these examples.

`dos` owns COMMAND.COM and the 15 explicitly listed utilities (`make -C dos`).
The existing APPS entries retain their component owners/producers, identified
by their `build/apps/<component>/` paths. Boot IO.SYS and composed INITDOS.SYS
remain on the separately built boot volume, not in the working-volume list.

The TPS report oracle is the retained REALDOS/MS-DOS 6.22 output
`dos/tpsrep/tests/reference/tpsrept_expected.txt` (SHA-256
b81305271c56b62f4dcf69e9049da1c9da2b81c9b6631fd6bf2f9c9bb4abf934).
It establishes historical DOS compatibility of the same report formatter,
not an independent formatter implementation. A shipped golden report would
mask the missing-input failure, so reports are generated only by the user.
