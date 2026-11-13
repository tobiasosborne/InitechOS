W4.0 real dBASE III PLUS 1.1 screens; PRD 6.6, ship-plan W4.0; no SAMIR-derived goldens.
Add a case directory containing `SETUP/`, `KEYS`, and `DUMPS`; names are case-sensitive.
`SETUP/` holds DOS 8.3 regular files/directories, including `CONFIG.DB` and `SETUP.PRG`; no links; DOS text uses CRLF.
`SETUP.PRG` initializes and returns; DBASE.*, DBASEINL.*, DUMP*, CAPTURE.OK, SCREEN.COM, EVENTS.BIN, MINT.PRG, AUTOEXEC.BAT and CONFIG.SYS are reserved.
The supplied CLIENTS.DBF is the existing deterministic mint_clients_dbf fixture (three records).
`KEYS`: one directive per line; blank lines and lines beginning `#` are ignored.
`WAIT n` waits n BIOS ticks (about 18.2/second), 1..65535; each key consumes one timer tick.
`TEXT text` injects printable US ASCII; `KEY name` injects a BIOS key, with its scan code.
Keys: ENTER ESC UP DOWN LEFT RIGHT HOME END PGUP PGDN TAB BACKSPACE DELETE CTRL-Y CTRL-END CTRL-HOME F1 F2 F10.
`DUMP name` captures the active 80x25 colour text page and BIOS cursor while III+ is running.
`DUMPS` lists each name once (letters/digits/hyphen/underscore, <=32 bytes); maximum eight dumps.
Every listed dump must execute once; finish with UI cleanup, TEXT QUIT, KEY ENTER, WAIT 18.
Run `make mint-screen SCREEN_CASE=path/to/case SCREEN_OUT=build/my-case`; inspect captures before copying them here.
Requires dosbox-x, nasm, C11 cc, timeout, sha256sum; DBASE3_DECOMP defaults to ../dbase3-decomp (mint/work binaries).
Each `.screen` is UTF-8/LF: `INITECH-SCREEN 1 CP437 80 25`, `TEXT`, exactly 25 rows of 80 Unicode scalars.
TEXT reversibly maps CP437 to Unicode; bytes 00..1F/7F use U+2400..241F/U+2421 control pictures; trailing spaces count.
Then `ATTR`, 25 rows of 160 uppercase hex digits (one CGA byte/cell), `CURSOR rr cc` (zero-based, two digits), `END`.
SHA256SUMS, INPUTS.sha256 and ORACLE.sha256 are enforced pins; the TSR must freshly save CAPTURE.OK with every declared dump bit.
Input mtimes are 1986-07-24 noon, DOS time frozen; 32MiB drive/16MiB free (512/32/2048/1024 geometry), fixed 386/10000 cycles.
Gates: make test-screen-mint test-screen-mint-repro test-screen-mint-samir test-screen-mint-mutants; see docs/screen-mint.md.
