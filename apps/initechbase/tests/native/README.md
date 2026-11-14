# Native InitechBase oracles

Tier 2 only: `make native-check` or `make target-mutants`. Both build the one
product image. A separate disposable TEST disk contains the selected EXE/OVL
and ordinary PRGs; no test scenario is compiled into the kernel.

The observer reads the booted system ELF symbols through QMP while QEMU is
stopped. It compares the boot volume's exact INITDOS.SYS with the ELF's binary
before reading anything; a stale observer is an error. The page contains all
2000 character/attribute pairs plus cursor row/column. The reader uses no
kernel header and the target uses only INT 21h.

- forms compares all 4002 bytes after the captured supported TE drawing prefix
  to genuine III+ at-to-b/TE.screen, including off-page cursor (25,0).
- enter compares all bytes to the new genuine prompt-tail/entered.screen.
- controls projects the literal Z, CR, Q, LF, R cells and (24,5) cursor from
  at-say-edge-controls/controls.screen with stdout redirected.
- color and color-state check the native 1F attribute from III+ HELP.DBS
  1390-1398 and test_ibase_set_color.c. These are documented CGA properties,
  not a claim of a real SET COLOR VGA capture.
- keys checks the IBM/DOS 00,scan extended-key pair: Left is navigation and
  must not become printable K. The dot prompt and cursor remain intact.
- continuation checks completion of a full 80-cell span after off-page SAY.

The drawing prefix ends at a silent `WAIT ""`, avoiding an INKEY busy loop
that can exhaust the emulator log cap. This changes no asserted page bytes;
no GET/READ is used. The driver must complete every requested step and exact
capture without emulator faults. A missing capture, timeout, log cap, stale
ELF or ordinary compilation failure is an error, never a kill. Each mutant
must produce the same semantic screen rejection twice; ordinary compiler-error
and no-fault controls must receive zero kills.

The file/stream cases require an executed DOS ECHO line after process return,
not just the command's input echo. Ordered input waits for the program's END
marker and allows child teardown before typing the ECHO. io writes the captured
C21 B/9 row and uses the independent DBF reader against real SAVED.DBF. stream
compares every internal CR/LF and character of the complete depth-with capture.
Input programs/results are hash-checked before running.

stack runs the same captured 20-level WITH program and verifies the complete
correct transcript plus normal QUIT return to DOS. Its fault completes that
transcript, then raises UD6 and HALTED instead of returning. This specific exit
violation counts only with the full correct transcript, exact exception and
missing executed DOS completion line; an incomplete program, arbitrary crash
or timeout alone earns no kill. The return failure is not described as a
wrong nesting transcript or an observed stack-guard diagnostic.

screen_capture.c and test_console_review.c were claimed with git mv from the
legacy harness. The remaining unclaimed emulator inventory stays with the
InitechBase owner under tests/SMOKES.md; these helpers build only here.
