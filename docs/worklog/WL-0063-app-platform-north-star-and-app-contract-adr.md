<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0063 -- The app-platform north star (ADR-0012) + the FLAIR App Contract (ADR-0013)

**Type:** strategy / governance session (orchestrated: two analysis workflows +
one ADR-by-committee, orchestrator-authored docs, operator-ratified via in-session
decision). **No artifact code changed** -- this session ratifies charter + design;
implementation follows in the filed beads.
**Date:** 2026-06-27.
**Beads:** new epic `initech-49ez` (Phase 4.5 Platform Services) + 5 children
(`initech-0w45`/`b2vk`/`77dj`/`gymo`/`o5vm`); `initech-4e35` (App Contract)
re-pointed ADR-0010 -> ADR-0013 + retitled. New ADRs: `ADR-0012`, `ADR-0013`.

## Context

Operator question: "what does FLAIR look like to a consumer -- is there an API, a
sensible architecture to build apps against?" -> then the steer: "the north star is
that FLAIR would be shippable in the late 90s as a real competitor to System 7/8 or
Win 3.1. If that gets achieved with our current planning, this is all good."

Two questions, answered by workflow (Law 1 ground-truth, adversarially verified):

1. **Consumer API map** (`wf_7ed4793a-48c`): FLAIR has a real, oracle-green
   Mac-Toolbox-shaped Manager API (Window/Menu/Control/Dialog/Event/Region/Text),
   but NO app-development contract -- `os/apps/` + `os/tps/` are empty, the only
   live loop is kmain's hardcoded scene behind `-DBOOT_FLAIR_LIVE`, `grafProcs` is
   NULL (drawing verbs declared-only), and the "App Contract" was undrafted. "The
   chrome is there; the app PLATFORM is thin."

2. **North-star gap analysis** (`wf_d1222888-838`): established the historical
   table-stakes bar of System 7/8 + Win 3.1 as app platforms, audited current FLAIR
   coverage, subtracted sanctioned non-goals, and graded. VERDICT: executed to plan,
   FLAIR lands a faithful, self-hosting desktop (its OWN ratified north star) but NOT
   a credible competitor platform. Load-bearing gaps: app process/loader + multi-app
   co-residency; the (undrafted) App Contract; resource model; clipboard/TextEdit;
   Standard File; printing. Two bar items are sanctioned non-goals (v8086/real-binary;
   real-iron) -- escalated, not silently built.

## The decision (operator, in session)

Operator ratified: **adopt the platform bar as a first-class (lowest-priority)
north star**, and do all three follow-ups -- recharter the App Contract ADR, file a
Phase 4.5 platform-services epic, record a worklog/decision shard. The two
competitor-bar items that stay non-goals are recorded as **accepted shortfalls**
(CLAUDE.md Stop Conditions), not closed silently.

## What changed (docs + tracker only)

**ADR-0012 -- North-Star Expansion (charter; RATIFIED).** Adds PRD Sec 1.4 (the
app-platform bar) as the 4th, lowest-priority commitment (D-1/D-5: never overrides
Fidelity/Self-host). D-2: the path = App Contract (ADR-0013) + a Phase-4.5 services
epic, sequenced before the app suite. D-3: accepted shortfalls -- v8086/real-binary
(honest substitute = "Initech versions", re-implementation parity NOT binary compat)
and real-iron (bar = "could plausibly have run on period hardware", tri-emulator-
graded). D-4: era ceiling ~1991-93 (`win95ism_guardrails.md`); TrueType/OLE2/VM/sound
are recorded competitive shortfalls, not anachronisms.

**ADR-0013 -- FLAIR App Contract (ADR-by-committee `wf_42352963-a57`; RATIFIED).**
Spine = Apple Process Manager model (Seat A); mechanics = minimal-cooperative (Seat C,
grafted); Win16 dispatch-shape only (Seat B, noted). A tenant = a `FlairApp` record
of C entry-points + a child arena, bound to its windows via `WindowRecord.refCon`
(the locked 4-byte slot, no new field). The shell INVERTS WaitNextEvent and demuxes
by refCon through a HOST-BUILDABLE Layer-5 dispatcher (`os/flair/process.{c,h}`,
`flair_app_dispatch`) called by BOTH kmain and the `test-process` oracle (the
adversarial verifier's key fix: keeps ADR-0006 E-D2 single-spine AND makes the host
oracle real -- kmain is kernel-only, cannot run hosted). Delivers MULTI-APP
CO-RESIDENCY (MultiFinder) over the existing pump + WindowMgr z-order + FLAIR heap,
within flat-32 + no-isolation + cooperative. Native windowed apps via
`FlairProcess_launch`; character-mode tenants (SAMIR/Turbo Initech) keep the
UNCHANGED single-program loader (suspend-desktop/save-under/rebuild-on-return).
**Fork ruled:** Initech 123 / InitechWord are NATIVE Toolbox tenants (Law 4: the
frame shows them as Mac document windows; co-residency needs the one compositor);
SAMIR/Turbo Initech stay character-mode (the dBASE/Turbo-Pascal full-screen
precedent). 8 oracles O-1..O-8 (host routing/activation/teardown/launch-budget +
emu app-switch + cross-emu + char-tenant suspend/rebuild + non-yield detector).

**PRD:** Sec 1 "Three commitments" -> "Four"; added Sec 1.4; annotated the Sec 2
non-goals (real-iron; preemption/isolation/VM; +a new explicit real-16-bit-binary
non-goal) with the accepted-shortfall pointers.

**Plan (`FLAIR-implementation-plan.md`):** corrected the stale "ADR-0010 App
Contract" (line 9 + Phase 4 header) -> ADR-0013 with a numbering note; inserted a
full **Phase 4.5 Platform Services** section (Resource Manager / Scrap / TextEdit+List
/ Standard File+common dialogs / Print Manager), landing before the app suite.

**Beads:** Phase 4.5 epic `initech-49ez` + 5 feature children, labelled
`flair`/`phase4.5`/`platform-services`, parent-child-linked to the epic; epic depends
on `4e35`; Phase 5 (`ib6`) + Phase 6 (`t4hp`) now depend on `49ez`; `4e35` retitled
+ re-described to ADR-0013 with the first-cut staging.

## Frictions / lessons

- One mapper in `wf_d1222888-838` returned a degenerate stub (`"a"/"b"/...`); the
  synthesizer DETECTED it and reconstructed coverage from the repo, and the
  adversarial verifier independently re-grounded it -- the two-stage (synthesize +
  adversarially verify) shape caught a dead input. **Lesson: never trust a single
  mapper; the verify pass is load-bearing.**
- ADR numbering drift: the plan reserved "ADR-0010" for the App Contract but 0010
  shipped as Grading-and-Goldens; 0011 is reserved-pending for Phase-6 hosting. App
  Contract = ADR-0013, charter = ADR-0012. Corrected the plan + epic refs.
- beads: a feature CANNOT depend-on (be blocked-by) an epic ("tasks can only block
  other tasks, not epics"); epic membership is the `parent-child` dep TYPE, not a
  `blocks` edge. Children linked via `bd dep add <child> <epic> -t parent-child`.
- House style: the PRD natively uses em-dash + section-sign (75/28); the ADRs/plan
  use `--`/"Sec". Edits matched each file's local convention (Rule 12 doc latitude),
  normalized the 2 plan outliers.

## Acceptance

This is a governance session -- the "oracle" is operator ratification + adversarial
doc-consistency, not a `make` gate. Both ADRs repo-verified against HEAD by the
committee chair + adversarial reviewer ("ratifiable-with-fixes"; all 4 fixes folded:
host-buildable dispatcher; teardown-oracle reframed to the bump+free-list heap
contract; +launch-budget +suspend/rebuild oracles; citation fixes). New ADRs are
ASCII-clean (Rule 12). `make test` UNCHANGED (no code touched) -- still the WL-0062
273 host + 53 emu green baseline.

## Pointers / next

- `docs/adr/ADR-0012-*.md`, `docs/adr/ADR-0013-*.md`; PRD Sec 1.4 + Sec 2; plan
  Phase 4.5.
- **Next implementation step (ADR-0013 Sec 8 minimal first cut, epic `initech-4e35`):**
  `os/flair/process.{c,h}` dispatcher + `activateEvt` synthesis (oracle-first: O-1/O-2
  host, mutation-proven, BEFORE routing is trusted) -> raise `SHELL_MAX_WINDOWS`
  (`shell.h:104`) + tenant registry -> reference 'hello window' + a 2nd co-resident
  tenant + emu app-switch gate (O-5) -> migrate SAMIR as the first text-tenant (O-7).
- Then Phase 4.5 (`initech-49ez`) services, each oracle-backed, before the canonical
  suite (Phase 5/6).
