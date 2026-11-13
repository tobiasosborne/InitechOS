# Deferred emulator gates

Names below are from legacy-final Makefile and
harness/diff/wk1_diff/i123_numeric.mk. None is run by this component import.
The normal gate is QEMU unless explicitly labelled Bochs. Each `-mutant` gate
reruns the same action with deliberately wrong code and requires the named red.

| Gate | Checks |
|---|---|
| test-flair-i123 | Launch, type MINT01 row, arrow navigation, serial drawing and pixels |
| test-flair-i123-mutant | Dead arrows must fail the navigation oracle |
| test-flair-i123-bochs | QEMU and Bochs headless CPU row/format/navigation replay |
| test-flair-i123-files | Save/retrieve WK1 bytes through real DOS files; independent reader |
| test-flair-i123-files-mutant | Empty save must fail disk content checks |
| test-flair-i123-file-ui | Slash menu/file prompts, retrieve and escape behaviour |
| test-flair-i123-file-ui-mutant | Misspelled menu and cancelled retrieve clobbering are caught |
| test-flair-i123-quitguard | Dirty close/menu quit, Cancel/Save/Don't Save, desktop/trash pixels |
| test-flair-i123-quitguard-mutant | Close-without-ask, never-dirty and cancel-quits are caught |
| test-flair-i123-formula | Typed formula recalc/F9, text and CIRC indicator |
| test-flair-i123-formula-mutant | Missing recalc must fail formula rows |
| test-flair-i123-entry | 240-character entry and tail scrolling |
| test-flair-i123-entry-mutant | Old 75-character entry cap is caught |
| test-flair-i123-keys | F2 EDIT, F5 GOTO, page and Tab viewport movement, pixels |
| test-flair-i123-keys-mutant | Tab moving one cell is caught |
| test-flair-i123-hit | Global mouse hit testing after Tab and resize, pointer pixels |
| test-flair-i123-hit-mutant | Ignoring scroll in hit testing is caught |
| test-flair-i123-resize | Shrink/grow reflows, follows E1; serial and pixels |
| test-flair-i123-resize-mutant | Fixed geometry is caught on all resize legs |
| test-flair-i123-wide | Width-240 A clipped to viewport, Right/Left/click, pixels |
| test-flair-i123-wide-mutant | Hiding wide columns is caught |
| test-flair-i123-menu | Copy/Move row, unavailable leaves, Quit, 82 menu levels/RI descriptions |
| test-flair-i123-menu-mutant | Move leaving a referrer stale is caught |
| test-flair-i123-copy | POINT rectangle replay, saved SINGLE vs real Lotus, pixels |
| test-flair-i123-copy-cpu | Headless QEMU SINGLE/ROW/OVER/BLOCK saved files vs real Lotus |
| test-flair-i123-copy-bochs | Both CPU replays, same four independent disk comparisons |
| test-flair-i123-copy-mutant | Period-as-text passes saved cells but fails POINT serial/pixels |
| test-flair-i123-numeric | Controlled RTC, currency entry, two NOW saves/F9 and independent file reader |

Shared gates also exercise 123: test-flair-fullsave (safe saves on full disk),
its mutant, and Finder/tenant launch and menu routing. They belong to sdk/factory.
The trace make fragments and i123_copy_smoke_keys.h remain in attic/spec.
The menu/numeric serial checker programs remain in attic/harness/diff/wk1_diff;
they consume emulator evidence, not standalone tier-0 fixtures.
