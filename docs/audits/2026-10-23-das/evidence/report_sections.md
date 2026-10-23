# Desk accessories, small apps, Control Panels, TPS and film audit

Audit date: 2026-10-23. Supplied image commit: e67b8311092873fe16e6c6d62ad82f894bfb8d7d. This report contains 45 new findings: 9 P1, 23 P2 and 13 P3. New L numbers belong to this report; they are not the L numbers in the older full-drive audit.

The most urgent results are the standalone film panic, the Scrapbook Ctrl-C termination, destructive full-disk text replacement, and TPS claiming success for a truncated report. The normal TPS example is byte-identical to the shipped manual's golden. The ordinary calculator arithmetic tested here agrees with the supplied real-Mac vectors.

All emulators used copied images, QMP on stdio, -display none, -cpu 486 and -m 16. At most one emulator owned by this audit ran at a time. No owner's emulator, X display, source repository file, build, tracker or commit was modified. All guest screenshots are QMP screendumps. Files were extracted with mtools only after their emulator stopped.

## Evidence and replay

- [NITS.tsv](NITS.tsv) is the flat finding index; [findings.json](evidence/findings.json) contains the same structured details.
- [actions.jsonl](evidence/actions.jsonl) records actual guest input with timestamps and serial byte offsets. It includes nested method calls and must not be replayed blindly as a flat command list.
- [replay.py](evidence/replay.py) replays one session's low-level input at its measured timing: `python3 evidence/replay.py copybreak replay-copybreak`. Use a new output label. It starts only one QEMU and stops it in finally. Session JSON files identify the exact input images and emulator arguments.
- [drive.py](evidence/drive.py) is the extended interactive driver; [audit_driver-original.py](evidence/audit_driver-original.py) preserves the supplied driver. The latter still exists at the workspace root as audit_driver.py. The named JSON action suites, prepare scripts and retained prepared disk copies support shorter controlled replays.
- [image-manifest.json](evidence/image-manifest.json), per-session hashes, [cleanup.jsonl](evidence/cleanup.jsonl), and [final-validation.json](evidence/final-validation.json) identify the tested artifacts and cleanup. PIDs can repeat because commands use sandbox PID namespaces; session time and command identify ownership.
- [screenshot-verification.json](evidence/screenshot-verification.json) lists the visually checked finding screenshots. Contact sheets are inspection aids; the PNGs in shots/ retain native 640x480 pixels.
- [harness-notes.txt](evidence/harness-notes.txt) explains mistargeted, early and misnamed captures. Findings use their actual contents, not optimistic filenames.

Coordinates are native guest pixels, with origin at top left. Drag steps give start and delta. Unless otherwise stated, desktop entry is double-click disk (600,64), APPS (123,101), then the executable. Normal APPS launch points are CALC (150,172), TEXT (215,172), PAINT (285,172), CALCULAT (78,224). Apple menu rows are About (90,27), Calculator (90,49), Control Panels (90,65), Note Pad (90,81), Scrapbook (90,97), reached from (20,10). Ctrl is the guest's displayed command-key convention.

Reference paths beginning os/, spec/ or pubs/ are under /home/tobias/Projects/initech-os. Named System 7/Mac OS 8 references are under /home/tobias/Projects/system7-decomp; the Macintosh guide is /home/tobias/Projects/initech-manuals-ref/macos/Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf. Reference copies and inspected manual pages/excerpts are in evidence/references/. Expectations based on memory or ordinary data/UI contracts are identified as such. Source diagnoses are guesses, not substitutes for observed evidence.

## Findings

@@FINDINGS@@

## Still present

These are freshly observed portions of known findings, not new numbers.

| Prior ID | Result in this session | Evidence |
| --- | --- | --- |
| F15, first GUI audit | The upper Special menu still exposes About/Quit instead of Special operations. About is now gray, so the older enabled-About no-op is not claimed here. The film therefore has no reached normal Shut Down path in this menu. | [192](shots/192-film-upper-special-menu.png), filmdata serial |
| M003, look pass 6 | Command equivalents still use a caret, for example ^Q and ^W, rather than the Mac Command glyph. | [128](shots/128-paint-file-actual.png), [024](shots/024-calc-edit-settled.png) |
| M025, look pass 6, partially present | The film worksheet is no longer completely blank, but it still does not reproduce the reference expense table: only three labels, two columns and TODO_FRAME values appear. The photographic expense grid and actual entries remain absent. This residual content gap is not assigned a duplicate new ID. | [186](shots/186-film-data-rest.png), [187](shots/187-film-data-escape.png), [photo](evidence/references/film-preview.webp) |

The one-disk-tenant limit also remains, with a visible generic refusal now. Earlier reports described this limit in coverage, rather than a dedicated finding. L030 covers the new reached same-accessory activation/error behavior. G09's deliberately malformed app/crash fixtures were not rerun, so that ID is not declared fixed on the basis of a different error path.

## Now fixed

| Prior ID | What no longer reproduces | Evidence |
| --- | --- | --- |
| F13, Apple-menu portion only | Both menu display and the listed accessory launches work. About This Computer shows InitechDOS 3.30 and memory figures. Finder Help was not re-audited, so the whole composite F13 is not certified. | [019](shots/019-apple-held.png), [039](shots/039-about.png), main serial |
| F05, Restart/Shut Down no-op baseline | Restart actually reboots, and Shut Down reaches the safe-to-power-off screen with Note Pad, Scrapbook, Text, Paint and Control Panels. Calculator is the new L022 exception. | [071](shots/071-after-restart.png), [237](shots/237-note-shutdown.png), [243](shots/243-text-shutdown-completed.png), [260](shots/260-paint-shutdown-refusal.png), [274](shots/274-scrap-shutdown-real.png), [276](shots/276-cpanels-shutdown-real.png) |
| M024 | With the data disk attached, the film chart is populated rather than white. Its appearance still differs as L038-L040 describe. | [187](shots/187-film-data-escape.png) |
| M026 | The film saving state has the intended hourglass after pointer movement. | [186](shots/186-film-data-rest.png) |

These are bounded retests against the seven previous REPORT/TRIAGE pairs. Other older findings are neither silently refiled nor declared fixed.

## Coverage and limits

| Area | Driven and verified | Limits or unreachable work |
| --- | --- | --- |
| Calculator, CALCULAT | All 18 face keys by mouse; keyboard digits/operators/C; 2+3*4= gives 20; 5== gives 5; 1/0 gives infinity and 0/0 not a number; negative zero gives 0; -5, decimal duplicates/overflow, long digits, 9999999999 squared gives 1E20, 1/3*3 gives 1; Backspace and Escape; held key; Edit menu; Close W; repeated launches; shutdown refusal. Readouts are collected in calculator-results.png. | No CE, percent, square-root or paper-tape control exists on this four-function face. These are not invented defects: the named real Mac reference is also four-function, and Backspace's no-op matches its vectors. No claim of arbitrary-precision certification. |
| Ledger, CALC | Distinguished from the actual Calculator; edit/commit, Return/Down, pie/116%, trailing 570-, Save completion, real bytes copied, second Save, Escape during copy, Quit/reopen, film resize/movement. Rate 0, 100, bad, empty and missing config; empty/missing ledger; a 1000-line ledger; existing target. | It is not a second scientific calculator. LEDGER.TXT and MICHAEL.CFG belong to CALC's copy operation, not TPSREP's table report. The source and live behavior establish that distinction. No invented financial rounding repair or normalization of the canon numbers is proposed. |
| InitechText | Three-line typing; wrapping; 4096-byte document; 32 KB and 64 KB rejection; LF and CRLF, blank lines, high-bit bytes; Ctrl-A/C/X/V/Z; mouse-drag selection and Cut/Paste; Shift-arrows; resize; caret/pointer; 25-line scroll boundary; title repaint; Open, Save, Save As; valid/invalid names; replacement Cancel/confirm; full disk; Quit Save/Cancel/Don't Save; dirty Open Don't Save; New Save/Cancel/Don't Save; shutdown Save and chooser Cancel; exact saved bytes. | Shift-click selection, deleting selected text with every key, read-only permissions, unusual path names, every chooser navigation path, and all possible line/byte limits were not exhaustively exercised. The shared caret/line renderer is not certified pixel-perfect. |
| InitechPaint | Finder launch twice; canvas drag, grow, close box, Ctrl-Q, shutdown. Every File/Edit/Image/Layer/Select/View/Window/Help menu was opened and inspected. | Every command except Quit is gray. No actual tool, pattern, fill, selection, layer, image-resize, open, save or undo workflow could be reached; consequently off-canvas painting, leaky fills, brush edges and image-file roundtrips cannot be graded. No separate bug is invented for each unavailable gray row. |
| Note Pad | Cross-tenant paste from Text; pages 1,2,8; last-page boundary; contents retained through Close/reopen and Restart; long text; Shift-arrows; explicit full-page alert; corrupt checksum; same-app relaunch; Shutdown and reboot persistence. Extracted NPAD file decoded. | Each page's exact 1024-byte boundary was not independently enumerated; the observed over-capacity paste did show a full notice. Cursor animation and every page-corner cancellation gesture were not exhaustively covered. |
| Scrapbook | Text copied between tenants; two clips, first/last boundaries, Cut removing a clip, close/reopen and Restart; long clip keyboard vs mouse scroll; Ctrl-C crash isolated twice; mouse Copy workaround; 32 clips and 33rd refusal; shutdown persistence, SBK1 bytes extracted. | PICT and sound clips could not be produced from the reached apps; Paint has no drawing or Copy path. The app advertises TEXT only. No claim that graphics/audio conversion was tested. Earlier failed capacity attempts are explicitly excluded. |
| Control Panels | All five reached panels: Date & Time, Desktop Patterns, Mouse, Sound, Memory. Every pattern choice; all tracking/double-click radio choices; all sound/length choices and Test; Cancel vs OK; 2000-02-29, 1999 clamp to Feb 28, return to leap day; year cap 2099; clock hide/show; clock advancement in menu; fast mouse produces approximately doubled motion; restart persistence; panel shutdown. CPANELS.DAT extracted and decoded. | No General panel or Font Manager install/remove/list interface exists in the reached panel chooser, Apple menu, or APPS roster. Font installation/removal therefore could not be driven. No seconds field is offered, so exact 23:59:59 entry was unreachable. Date limits at 1980 and all month/hour/minute wrap combinations were not exhaustively enumerated. Sound controls were exercised, but audible output was not captured; no audio-quality verdict is made. Memory figures were inspected, not independently stress-certified. |
| TPS | Real GUI Exit to DOS route; shipped parameters/table; exact golden report and backup; six/eight alignment presses rejected, seven accepted; mismatched report IDs; overlong ID input (documented truncation); wrong memo; cover N; missing table; absent/empty/over-1-KB parameter file; empty/truncated DBF; bad LINES; LINES 0/100; bad numeric PAGES; 1000 records; zero-free-space report. | The 1000-record fixture is an explicit table stress control, separate from the 1000-line ledger copy. Arbitrary malformed DBF layouts, every DOS device failure, every non-ASCII parameter and all full-disk backup permutations remain outside this finite run. Oversize parameters were refused with missing-key feedback; no hang is claimed. |
| Film | Required no-data boot and subsequent controls document a panic. Data-attached control reaches the scene, hourglass, Escape dismissal, editable ledger, chart move/grow, upper Special and lower-bar attempts, Quit. Compared with preview.webp at named regions. | The no-data image cannot reach any interactive controls after panic. The data-attached saving tableau is a staged prop, not a file copy; its static bar is not called a hang or a failed disk write. No usable normal Special Shut Down was reached on that film desktop. Photo/CRT geometry is not an RGB oracle; no misleading native-pixel mismatch percentage is asserted. |
| Cross-app/lifecycle | All assigned small GUI apps launched, accessories from Apple and primary apps from Finder; multiple attempts and close/reopen paths; same-app and second-app refusals; shared TEXT Scrap; settings and page files across Restart; active-app shutdown controls, including edited CALC in the final finderaccess session. | A final finderaccess session launched Note Pad, Scrapbook and Control Panels twice from the scrolled APPS window; the other small apps already had repeated Finder launches. Only one disk app can run at once, blocking simultaneous Text-to-Text windows. No multi-hour soak, real printer, non-16-MB machine or second emulator backend was used. |

Important controls that held up:

- The normal TPS report is exactly 2432 bytes and equals the manual's golden SHA-256 b81305271c56b62f4dcf69e9049da1c9da2b81c9b6631fd6bf2f9c9bb4abf934. The prior report is retained as BAK on a successful second run. See [comparison](evidence/extracted-tps/comparison.json).
- The 1000-record report lists 1000 records, totals 12000 pages, and produces a 67968-byte report. LINES=0 gives 13 detail pages; LINES=100 gives one for the shipped 13 undeleted records. See [case summary](evidence/extracted-tps-cases/summary.json).
- The 1000-line ledger copy at rate 100 writes all 17000 bytes and finishes at 102 ticks. Invalid config preserves the pre-existing TABLES.TXT, despite the missing visible error. A valid config with empty ledger completes with a zero-byte output; a missing ledger refuses.
- CPANELS.DAT is a 16-byte CP01 record in the volume root. After main it encodes pattern 7, normal tracking, slow double-click, Click sound, Full length, clock shown; it survives Restart. Note Pad stores 8224 bytes and Scrapbook 32848 bytes in root files, with checksummed page records. The original files, decoded pages and checks are retained.
- Text shutdown chooser cancellation leaves the desktop running; subsequent Save writes the exact 21-byte UNSAVED SHUTDOWN TEXT and reaches the safe-shutdown screen. Note Pad's SHUTDOWN SENTINEL survives reboot.

## The fifteen fixes I would do first

1. Keep GUI Copy from entering DOS Control-Break (L002).
2. Preserve original text files until replacement succeeds (L003).
3. Check every TPS write and report a nonzero failure status (L004).
4. Make the specified standalone film image boot without panic (L001).
5. Preserve LF-only empty lines (L005).
6. Make ledger Save serialize the document being edited (L006).
7. Guard ledger Quit and close against unsaved changes (L007).
8. Preserve and explain corrupt Note Pad storage (L008).
9. Identify and confirm the ledger copy destination before replacement (L009).
10. Deliver visible Save and capacity errors (L019, L045, L014).
11. Give InitechPaint working tools and picture file operations (L023-L025).
12. Repair Text resizing and scrolling paint correctness (L012, L013).
13. Provide Undo and Shift-arrow selection (L010, L011).
14. Finish the filename field's editing and validation behavior (L016, L017, L035).
15. Make the accessories integrate: Calculator clipboard/shutdown, Scrapbook mouse scrolling, and stable menu bars after preference changes (L021, L022, L026, L027).

The [native scene/reference crop pairs](evidence/film-comparison.json) retain region coordinates without rescaling or pretending the photograph is a framebuffer golden.

The film's chart/body fidelity should then receive a focused pass using L031 and L038-L044 plus the residual M025. These are deliberate-number-preserving appearance repairs, not corrections to 116%, 570-, the teal desktop, the two bars or the hourglass convention.
