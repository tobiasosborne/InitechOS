<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0012 — Programme Engineering Work Record (PEWR)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Engineering Work Record (Worklog)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | PE-WL-0012 |
| Title | Diagnostic-Message Catalogue Enforcement (ADR-0003 DEC-13): codegen single-source-of-truth + the `test-dosmsg` oracle |
| Version | 1.0 |
| Status | Issued |
| Classification | Internal Use Only |
| Period Covered | 2026-06-09 |
| Recording Function | Build Orchestration (supervised multi-agent; orchestrator re-ran every oracle + independently mutation-proved, Law 2 / Rule 4 / Rule 6) |
| Related | WL-0011. beads CLOSED: 509.1. Unblocked: 509.8, 509.10. Follow-up OPEN: 2nv (uncontrolled subdir notice). |

---

## 1. Purpose & headline

Closes **`509.1` — Diagnostic-message catalogue enforcement (DEC-13)**, the last
P0 in the M2 personality epic blocking the INT 22/23/24 cluster (`509.8`) and the
Appendix-D baseline (`509.10`). DEC-13 mandates a 16-message controlled vocabulary
(`MSG-DOS-0001..0016`) that no kernel code may add to, reword, recapitalize, or
repunctuate — and that all user-facing diagnostic output must reference, not inline.

**State at close:** `make test` = **43 host + 18 emu = 61 gates** (was 41+18=59),
deterministic, green (`triple_fault=0`), re-verified by the orchestrator under load.
The two new host gates are `test-dosmsg` + `test-dosmsg-mutant`. Working tree clean.

## 2. What landed

1. **Single source of truth via build-time codegen.** `spec/dos_messages.json`
   (the LOCKED catalogue, Rule 8) is now the *only* place the message text lives.
   A deterministic Makefile codegen rule (`$(DOS_MESSAGES_H) := build/dos_messages.h`,
   inline `python3` matching the `test-spec` house pattern) emits a generated C
   header `#define MSG_DOS_0001 .. MSG_DOS_0016`, iterating `i in 1..16` explicitly
   (byte-deterministic, Rule 11), C-escaping defensively, ASCII-only (Rule 12), with
   a GENERATED/DO-NOT-EDIT banner citing ADR-0003 Appendix C. No timestamps/paths.
   The header is a `build/` artifact (auto-gitignored), wired as a prerequisite of
   every rule that compiles `command.c` (the kernel `command.o` **and** the four
   host `test_command` builds) with `-Ibuild` on the include path.

2. **Kernel sweep to references.** `os/milton/command.c` — the only TU that emits
   controlled messages — now `#include "dos_messages.h"` and references
   `MSG_DOS_0002 / MSG_DOS_0003 / MSG_DOS_0011` (the 3 of 16 currently emitted).
   The two local literal-bearing `#define`s and one raw inline literal
   (`"Required parameter missing"`) were removed. **Emitted bytes are identical**
   (pure refactor; C adjacent-string-literal concatenation with the `"\r\n$"`
   sentinel is unchanged) — proven by `test-shell` still asserting MSG-DOS-0002.

3. **The `test-dosmsg` oracle (3 teeth; the bead's described oracle made mechanical).**
   - **Consistency:** `build/dos_messages.h` encodes each of the 16 spec texts
     verbatim (header == spec).
   - **Image presence (centerpiece):** the *referenced set* R is derived by
     grepping `MSG_DOS_00NN` in the **comment-stripped** (`gcc -fpreprocessed -E -P`)
     non-test `os/milton/*.c`, then asserting each referenced message's text appears
     verbatim in the extracted printable strings of `build/kernel_shell.bin`. R
     auto-grows as more messages are wired. `%c` messages are matched on the literal
     prefix before the substitution point.
   - **Source purity (no inline literals):** comment-stripped non-test sources must
     contain none of the 16 texts as a quoted literal. Comment-stripping is what
     stops the legitimate prose mention at `command.c:582` from false-positiving.

4. **`test-dosmsg-mutant` (Rule 6).** Mutant A reintroduces an inline literal →
   purity tooth bites; Mutant B rewords the header (not the spec) and rebuilds →
   presence tooth finds the canonical text absent from the image → bites. Restores
   the tree byte-clean.

## 3. Orchestration & verification (Law 2 / Rule 4 in action)

Decomposed into a serial pipeline (A scope → B codegen → C sweep → D oracle →
E verify), delegated step-by-step; the orchestrator re-ran every gate itself rather
than trust subagent "green." Two findings worth recording:

- **Integration gap caught by re-running.** The codegen step wired `-Ibuild` only
  into the *kernel* `command.o` rule; the host `test_command` oracle (+3 mutant
  builds) also compiles `command.c` and went RED with `dos_messages.h: No such
  file`. Fixed by adding `$(DOS_MESSAGES_H)` to the shared `TEST_COMMAND_HDRS` and
  `-Ibuild` to all four recipes. A subagent reporting only its own scoped build
  green would have shipped a broken aggregate.
- **Independent mutation-proof, not just the scripted one.** Beyond the mutant
  gate, the orchestrator hand-perturbed the **real** sources: (a) reverted a
  `MSG_DOS_0011` reference to a raw literal in `command.c` → purity tooth RED
  (exit 2); (b) reworded MSG-DOS-0002 in the real `build/dos_messages.h` →
  consistency tooth RED (exit 2). Both restored identical. The oracle bites real
  regressions, not only its own canned ones.

## 4. Scope decisions

- **Two categories, one in scope.** User-facing controlled diagnostics (the 16)
  are enforced; serial-channel debug strings (`EXEC-SAW-FAIL`, `BI-BAD ...`, PRD
  Appendix B) are NOT controlled vocabulary and were left untouched.
- **Normal program output is not a diagnostic.** The DIR header, version string,
  `ECHO is on`, and the `A:\>` prompt are program output, not part of the 16; left
  as-is.
- **One uncontrolled diagnostic flagged, not forced.** `command.c:530`
  "Subdirectory traversal not yet supported" is a user-facing notice absent from
  the catalogue. Routing it in would require a deliberate locked-spec change
  (a new approved MSG-DOS entry, Rule 8), so it was filed as **`2nv`** (P3) rather
  than silently invented.

## 5. Phase Disposition — NEXT AGENT

**M2 capability cluster: 3/3 of the cited next items now done** (509.2, yv9, 509.1).
`509.1` being closed **unblocks `509.8` and `509.10`** (both deps `509.1`+`509.4`
are ✓). Resume at the cluster finish per WL-0011 §5:

1. **`509.8` + `4tw` — INT 22h/23h/24h handlers + PSP vector save (DEC-10).**
   Termination/control-break/critical-error vectors; critical-error presents
   **MSG-DOS-0001** (now available verbatim as `MSG_DOS_0001` from the catalogue —
   wire it through, and the new `test-dosmsg` presence tooth will pick it up into R
   automatically) and processes Abort/Retry/Fail; save/restore vectors in PSP
   0Ah-15h across EXEC/EXIT. Then `4tw`: CON input 01h/08h/0Ah detect `^C` (0x03)
   and invoke INT 23h.
2. Then `x0i` (tri-emulator — the principal quality debt), `u0a` (build-time
   `_kernel_end < PROGRAM_BASE` guard), `509.7/.6/.9/.10`, `kod`, `k6x` (the M2
   finale: COMMAND.COM default boot).

**Working discipline (reconfirmed):** re-run the full 61-gate vector after anything
touching the kernel image; verify every subagent "green" yourself; the new catalogue
oracle means any new controlled message MUST be added to `spec/dos_messages.json`
first (the codegen + `test-dosmsg` flow from there).

## 6. Verification of Record

At close, `make test` passes all 61 gates on QEMU (`triple_fault=0`). `test-dosmsg`
+ `test-dosmsg-mutant` are mutation-proven (scripted + orchestrator-independent).
The catalogue is the single source of truth; `command.c` carries no inline
controlled literals; the kernel image carries the referenced messages verbatim.
Tree clean.

---

*— End of Record —*

<!-- Tedium certified compliant with NFR-7. -->
