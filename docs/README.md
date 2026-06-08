# docs/ — worklog and design records

Project documentation that is not a locked spec (those live in `spec/`).

- `worklog/` — sharded session log, `NNN-*.md`, one shard per meaningful
  chunk of work: Context -> What changed -> Why -> Frictions -> Acceptance
  -> Pointers (CLAUDE.md "Session close").
- `adr/` — architecture decision records, `NNNN-title.md`, capturing
  contested design choices.

Cross-session *task* tracking does NOT live here — that is beads (`bd`)
only (CLAUDE.md Rule 9). Docs are prose; Unicode is fine in `.md` prose,
but code stays ASCII (CLAUDE.md Rule 12).

Governed by: **CLAUDE.md "Session close", Rule 9.**
