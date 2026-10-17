# WL-0143 -- R4.4 Note Pad and Scrapbook (initech-tdnl.18)

The lane adds NOTEPAD.EXE and SCRAPBK.EXE as disk tenants, using Calculator's
Apple-menu launch table and InitechText's shared TextEdit, desk TEXT Scrap,
DOS-backed file verbs and Platinum scrollbar. No new kernel mechanism is
introduced. Note Pad has eight pages; Scrapbook has 32 TEXT clip slots.
PICT and sound clips are unsupported. Up/Down scroll a long Scrapbook TEXT
clip; Left/Right (or the shared page scrollbar) select clips.

Rule 8: spec/notepad/store.json and store.h deliberately lock the new private
fixed-record format, distinct NPAD/SBK1 magics, version/count/current/slot
header, 1024-byte single-byte TEXT records (extended bytes and tabs retained,
CR line ends), zero padding and FNV1a checksum. Page turns
and close save through the existing file gate. Malformed files yield empty
pages and DA-CORRUPT through DOS AUX, the existing serial-device seam. A save
failure keeps the window and page open and shows the shared alert.

Ground truth is docs/research/period-gui-suite.md Sec 4 and the user's R4.4
contract. The pixel gate reads the independent original Mac OS 8.1 captures
s8_doc_window_active.png, s8_doc_window_inactive.png and
s8_simpletext_empty.png under system7-decomp/goldens/captures. Scrollbar RGB
crops are also proven against that corpus by test-scrollbar-look. The app
never reads those golden expectations. Host tests run the actual tenant over
the real TextEdit and real Scrap Manager with the InitechText recording-mock
pattern; the emulator disk oracle reads files extracted independently by
mtools and checks full fixed-record bytes and zero padding.

Missing native captures are explicit: Mac OS 8.1 Note Pad idle window,
page-counter placement, dog-ear idle/pressed/turn animation, and Scrapbook
idle window, n-of-m placement and page scrollbar states. The authorized
fallback composes the existing documentProc, SimpleText text surface and
Platinum bar. The corner folds and counter placement are functional
adaptations with no native-pixel-fidelity claim. Page turns are immediate;
no native dog-ear animation is claimed. Text uses the owned Geneva9 strike.
As in InitechText, the fixed text layout does not reflow on window growth.

RED evidence: the initial host routing suite reported 30 failures, the
emulator Apple row could not launch NOTEPAD.EXE, and the independent pixel
gate reported 4034 mismatches. Named mutations cover page-turn off-by-one,
skipped close save, wrong-page paste and accepting a corrupt checksum. The
emulator twins use unchanged locked traces and swapped tenant executables.
A restart leg uses Special Restart and the second LIVE-READY, then reopens
the stopped data volume and checks both distinct page payloads. The Scrap
round-trip gate copies Note Pad -> InitechText -> Note Pad and checks the
persisted page bytes independently.

APPS appends the two executables, preserving all existing launch cells.
The damaged-volume minter already puts stranded TRASH at entry 0, so that
trace stays fixed. Locked purge becomes 22 removed / 2 refused; Empty Trash
clip marker becomes 24 (README, APPS, 22 APPS leaves). Fixture contracts and
the Apple-menu row-count gate include both additions.

Rule 14: note_pad and scrapbook RECORD_SCRIPTS blocks are before record-flair.
The PM records and judges both clips on main after merge. This lane leaves
changes uncommitted and does not modify beads, CLAUDE.md or HANDOFF.md.

Acceptance: the final host routing suites pass 40 Note Pad and 22 Scrapbook
checks, including single-byte TEXT / tab preservation and mixed CRLF/LF
normalization across Scrap and persistence. All four named host mutations
are caught (Scrapbook uses the three applicable ones). All seven new QEMU
gates pass, with the three requested Note Pad emulator mutations and the
Scrapbook wrong-page mutation caught by the independent disk oracle.

make image, locked purge, Finder service, app launch (+mutant), Empty Trash,
Calculator DA (+mutant), Apple menu (+mutant), InitechText (+mutant), and
InitechText scroll (+mutant) are green. Calculator, Apple-menu, TextEdit
surface, InitechText host families and fixture-volume contracts also pass.
The kernel-headroom gate measures 203 kernels: flagship 180376 loaded bytes
(16232 headroom), largest kernel_flairtenants_db_write_skip.elf 181112 loaded
bytes (15496 headroom), against a 4096-byte floor. Source ASCII scan is clean.
No make test, Bochs, git, commit, push, or beads writes were performed.
Final logs are preserved under build/notepad-lane-logs/.
