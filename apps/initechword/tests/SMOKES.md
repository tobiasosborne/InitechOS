# Deferred emulator gates

No emulator gate is ported or run by this import. All drive the booted product
from outside, using keystrokes, serial drawing, disk reads and pixel checks.
Names are from legacy-final Makefile:32357..32756, the three wp5_diff make
fragments, spec/flair_iword_traces.mk and harness/bochs_legs.mk. Those recipes
and trace literals remain in attic for the tier-1 factory importer.

Each row has both test-flair-iword<suffix> and its -mutant twin:

| Suffix | Evidence graded / named faults |
|---|---|
| (empty) | Launch/register, wrapped typing, F10 save, F7/Shift-F10 retrieve; MEMO bytes and wp5_ref.py, serial/pixels; NO_WRAP and SAVE_NOTHING |
| -codes | Bold/underline/italic runs, Reveal Codes rows, STYLE cell tokens and pixel forms; NO_FACE and kernel underline-placement fault |
| -block | Block/Move/Copy/Delete prompts and highlight, TABS body vs real MINT09, page groups; NO_D4 and NO_BLOCK_ON |
| -format | Format Line screens, spacing, center, Home keys, block attributes; FMT == MINT32, reader, Reveal Codes/pixels; NO_CENTER and BLK_CUR_START |
| -fkeys | Function-key template, gray/unmapped command routing, serial/pixels; FKEY_DROP and FKEY_UNMAPPED |
| -search | Forward/backward search, code queries, match/cancel/not-found cursor and prompts; SEARCH_BACK and SEARCH_NOTFOUND |
| -replace | Confirmation, case-preserving replace, saved REPLACE body/text, pixels; REPLACE_CONFIRM |
| -undo | Delete stack, restore and cursor-motion separation, saved UNDO/MOTION, pixels; UNDO_LEVEL and UNDO_MOVEMENT |
| -setup | Preference menus, third-launch persistence, captured settings bytes, pixels; SETUP_LOAD |
| -listfiles | Real LISTF grids, ordering, Look, disk-free and clock, cells/pixels; LF_COLUMN |
| -listfiles-ops | Selected Delete/Move/Copy, confirmation/collisions, protected/full targets, saved bytes and fsck; LF_WRONG_PATH and LF_TRUNCATE |
| -listfiles-alias | Dot-path alias rejection and target/source preservation; LF_ALIAS |

Search also defines the individual gates test-flair-iword-search_back-mutant
and test-flair-iword-search_notfound-mutant, composed into its -search-mutant.

Bochs wrapper gates are test-flair-iword-bochs and
 test-flair-iword-{codes,block,format,fkeys,search,replace,undo,setup}-bochs.
They replay the corresponding headless CPU traces on both QEMU and Bochs;
pixel-specific QEMU checks still need their own capture legs.

Cross-component coverage remains with SDK/factory: test-flair-fullsave and its
-mutant (Word/Text/123 saves on a full volume preserve old targets), Finder
launch/menu/close dispatch, styled cell/font rasterization and tenant loading.
The host suites retain their serial-grading modes for eventual factory reuse,
but make test does not invoke those modes. tools/ppm_iword_codes_check.c stays
in attic because it needs real emulator pixels, not a tier-0 host substitute.

Publication captures: test-pubs-iword-flair-screens replays the manual's
FLAIRSHOTS traces and compares committed PNGs pixel for pixel, with printed /
unreplayed figure accounting. pubs-iword-flair-screens is its manual capture
command. Both remain with the publications/factory importer (attic/pubs/pubs.mk).
