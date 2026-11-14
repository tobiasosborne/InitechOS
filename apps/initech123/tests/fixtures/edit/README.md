# EDIT key reference

Real Lotus 1-2-3 R2.2, initialized pinned runtime, on DOS 3.30 / QEMU 486,
8 MiB. `capture.sh OUT RUNTIME` uses `goldens/dataloss/drive.c` and a runtime
prepared by `goldens/dataloss/runtime.sh`. Exact inputs are `input.steps`;
VGA bytes and transcriptions retain every EDIT result and the saved screen.
`EDITKEYS.WK1` is extracted from the stopped reference disk. SHA256SUMS pins
these independent observations. The Reference Manual pp. 1-22/1-23 (scan
pages 38/39) supplies the key table.

The prefix participates in cursor movement: Home is before the apostrophe.
End reaches the append point. Tab/Ctrl-Right advance five characters;
Shift-Tab/Ctrl-Left retreat five. Movement clamps at either end. Delete at
End and Backspace at Home do nothing. Insert toggles overwrite, which
replaces a character without deleting its successor. The next EDIT session
starts in insert mode. These conclusions come from the captured edit lines
and independently decoded saved LABELs, not from the application under test.
