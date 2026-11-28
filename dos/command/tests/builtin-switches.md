# COMMAND switch coverage

| Switch | Observable effect | Check |
|---|---|---|
| `DIR /P` | Waits at the page boundary, then continues or aborts on Ctrl-C. | `tests/runtime/dirbreak.sh`, `command/tests/dos_parity.sh` |
| `DIR /W` | Lists names in five columns. | `make smoke-dir-wide` |
| `COPY /A` | Treats Ctrl-Z as text EOF when reading, and writes a final Ctrl-Z to a text destination. | `test-shell-files-host`, DOS 3.30 concatenation captures |
| `COPY /B` | Copies the source bytes through Ctrl-Z and omits a final Ctrl-Z on a binary destination. | `test-shell-files-host`, DOS 3.30 concatenation captures |
| `COPY /V` | Reads written bytes back and reports a failure if they differ. | `test-shell-files-host` with a corrupt readback |
| `COMMAND /C` | Starts a secondary with its own environment and returns after executing the tail. | `make smoke-batch-return`, `tests/runtime/session.sh` |
| `COMMAND /P` | Runs the current drive startup batch and keeps the resulting prompt after EXIT. | `make smoke-shell-fidelity` |
| `COMMAND /E:n` | Sets the secondary environment capacity within the installed arena. The 160-byte child refuses a SET that fits in the 512-byte child. | `make smoke-shell-fidelity` |

The COMMAND audit's DOS 3.30 switch sweep shows refusal of /A through /Z
and /? for MD, RD, TYPE, CLS, VER and EXIT. Its hands-on comparison also
shows refusal of /Z for VOL and BREAK. The native replay checks the exact
diagnostic, the surviving directory and the line after EXIT in a secondary.

DEL, REN and CD refuse switches through their existing diagnostics checks.
ECHO and PROMPT take text, and SET takes assignment data. A slash in that
text or data is used as part of the operand.
