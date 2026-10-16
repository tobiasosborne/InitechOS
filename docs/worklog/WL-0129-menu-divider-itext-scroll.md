# Menu divider endpoints and InitechText scroll acceptance

Context: initech-r78f and initech-ehq4, scoped by the PM's coding-lane brief.
Neither ID exists in this clone's beads database or `.beads/issues.jsonl`;
tracking is left untouched as instructed.

The independent `system7-decomp/goldens/captures/s8_menu_dropped.png`
has black frame columns x33/230 and separator rows y102/103 spanning
**x34..229**, across both inner bevels. The existing corpus prose describes
only its narrower x35..228 scan. This is a deliberate r78f re-key of the
factory-only `spec/menu_look_golden.h` expectations, not a chrome-metric or
asset change. The host verifies every pixel of both historical rows, then
grades both rendering depths including both endpoints. The Finder emu gate
grades the entire translated/resized row, x33..183 at y106/107, with no
column exemption. The renderer paints the one-pixel-inset groove after the
side bevels so they cannot erase its ends. Effective clipping remains in use.

RED before implementation: `test-menu-look` failed ML10 at both depths;
`test-flair-menu-look` reported `FAIL menu-look ML10 whole divider rows:
3/302 differ`. Both gates are now green. The named relapse
`MENU_LOOK_MUT_OLD_DIVIDER_INSET` reproduces that exact emu failure and fails
ML10 on the host. The existing menu-look mutants remain red as intended.

The new `test-flair-itext-scroll` reuses the real TEXT.EXE, Toolbox scroll
service, `appl-boot`, and `ppm_flair_scroll_look_check`. Its locked trace
copies the exact cell12 launch prefix and cursor return from
`FLAIR_ITEXT_SPEC`, then types forty paced Returns. Independent expectations:
41 Geneva9 lines at 11px plus the SimpleText insets give document459,
view389 and range/value70. The host routing suite checks those literal
values and the visible caret. QEMU checks the serial range/value, every
vertical-bar pixel, the disabled horizontal interior and the whole empty
content field with its one-pixel caret at global x14,y440..450. The enabled
thumb and arrow crops are the existing PNG-verified independent matrices
from `s8_doc_window_inactive.png`; well/disabled anatomy is from that
capture and `s8_doc_window_active.png`. Caret appearance is sourced from
`s8_simpletext_doc_caret.png`, with the documented Geneva9 line-height
substitution. Expected pixels do not read artifact geometry or palette tables.

`ITEXT_MUT_SCROLL_NEVER_ENABLED` substitutes only the gate-local TEXT.EXE.
The host reports `RED-REASON scroll: 40 Returns must enable range70:
len=389 value=0`. QEMU reports `FAIL scroll-look crop thumb-v: 205/210 pixels
at(568,425)`; the app still loads, registers and handles all forty Returns.
The mutant target asserts both serial and pixel rejection.

`record-flair SCRIPT=itext_scroll` is registered before the record target,
uses the same locked input, the existing extra-long record image and the
shipped data volume, and requires the final SETSCROLL marker. Recording and
reproving the Rule-14 clip are the PM's work on main after merge. No clip
was recorded in this lane. The existing `menu_look` clip exercises the
corrected divider.

The normal TEXT.EXE remains byte-identical to its pre-change build (9268
bytes); only a compile-time mutant seam was added. The shipped APPS inventory
is unchanged, so there is no data-volume growth or purge-count re-key.
Host routing, menu behavior/mutation, Makefile ordering and volume-contract
checks pass. All QEMU runs use INITECH_QMP_STDIO=1. No full test vector or
Bochs run was invoked. Changes are left uncommitted for PM review.

Acceptance runs: `test-menu-look` (21 checks), `test-menu-look-mutant`,
`test-menu` (83 checks), `test-menu-mutant`, `test-itext-route` (44 checks),
`test-itext-route-mutant`, `test-scrollbar-look` (10 checks),
`test-flair-menu-look`, `test-flair-menu-look-mutant`,
`test-flair-itext-scroll`, `test-flair-itext-scroll-mutant`,
`test-flair-itext`, `test-flair-itext-mutant`, `test-makefile-vars`,
`fixture-volume-recipe-lint`, `image`, and `test-kernel-headroom` are green.
The headroom gate measured 192 kernels: flagship loaded171112 / free25496
bytes; binding `kernel_flairtenants_db_write_skip.elf` loaded171880 /
free24728 bytes, above the locked4096-byte floor in the196608-byte window.
