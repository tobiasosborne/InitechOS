# docs/ — worklog and design records

Project documentation that is not a locked spec (those live in `spec/`).

- `worklog/` — sharded session log, `NNN-*.md`, one shard per meaningful
  chunk of work: Context -> What changed -> Why -> Frictions -> Acceptance
  -> Pointers (CLAUDE.md "Session close").
- `adr/` — architecture decision records, `NNNN-title.md`, capturing
  contested design choices.
  [ADR-0013 Amendment AC-3](adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md)
  ratifies disk tenants and the locked Toolbox gate;
  [ADR-0003 Amendment DEC-08a](adr/ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md)
  carries the matching 2026-10-20 placement/address amendment.

Cross-session *task* tracking does NOT live here — that is beads (`bd`)
only (CLAUDE.md Rule 9). Docs are prose; Unicode is fine in `.md` prose,
but code stays ASCII (CLAUDE.md Rule 12).

Governed by: **CLAUDE.md "Session close", Rule 9.**
