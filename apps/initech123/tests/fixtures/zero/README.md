Real Lotus 1-2-3 R2.2 Global Zero captures (Reference 12-33..35).

capture.sh uses goldens/dataloss/drive.c on the hash-pinned DOS 3.30/Lotus
runtime made by goldens/dataloss/runtime.sh. input.steps records exact input;
.vga is the raw 80x25 character/attribute buffer, .txt its character view.
The seven matrix cases and long-steady are READY captures. Fast long-label
input was discarded and repeated with one key per driver command.

No/Yes/Label always opens with No selected. Yes substitutes an empty right
label. Label starts empty after No, with a double quote after Yes, or with
the current encoded label. Plain input gains a right prefix. The 240-character
input reopens with a prefix and 239 characters (protocol, 239 backspaces).
Esc clears a nonempty working field; a second Esc returns to the Zero menu
with Label selected, preserving the published replacement. The replacement
never spills into another cell, even a long left label with empty neighbours.

Only actual numeric zeros are affected: integer and formula zero, numeric
Text, Time and Date; Hidden remains hidden. A label '0, formula Text +A1,
rounded nonzero 0.05, NA and ERR are unaffected. Before captures also establish
that Date1 serial zero is 00-Jan-00. Retrieve and Worksheet Erase reset Zero.
The current cell's control-panel value is unchanged. All paired before/after
WK1 files are byte-identical; changing Zero after Save quits without an
unsaved-changes warning. The view setting has no WK1 or dirty-state bit.
