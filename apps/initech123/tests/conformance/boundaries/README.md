# Retrieved formula boundary probes

Real R2.2 on the same initialized DOS33/QEMU 486/8MiB runtime as the study.
Independent C WK1 writers are formula-boundary.c, binary-boundary.c and
string-boundary.c; they do not use any product codec. Each directory retains
its exact CASE.WK1, input steps, raw VGA, transcription and monitor log.
BOUNDARIES.sha256 pins the bytes. Reproduce with the study capture.sh.

| RPN expression | Accepted | `Formula too long` |
|---|---|---|
| INT(1) wrapped in PAREN | 63,64,118–124 groups | 125–128 groups |
| CONST(1.0) wrapped in PAREN | 118,119,124 groups | 125 groups |
| INT(-1) wrapped in PAREN | 124 groups | not separately probed |
| INT(1) INT(2) ADD wrapped in PAREN | 120,121,123 groups | 124 groups |
| literal string | 247–249 characters | not separately probed |

The non-string cases locate a 250-character rendered-expression boundary:
249 and 250 fit, 251 fails. This is independent of encoded byte count,
constant encoding and parenthesis depth (the binary expression shifts the
boundary by one group). The artificial leading entry '+' is not part of the
expression. Literal strings have different behavior; these probes do not
justify imposing the same limit on them. Do not infer a bytecode-size limit.
These are Retrieve limits, distinct from the manual's 240 typed characters.
