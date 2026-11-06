# Local post-push sentinel

Ref: PRD Sec 8/9/14; CLAUDE.md Laws 1-4, Rules 1/6/10; initech-3nc3v.
The PM merges lanes serially, pushes, then runs this on the local host:

```sh
. tools/lane-env.sh
git push && tools/sentinel.sh
# One cycle, returning to the shell:
tools/sentinel.sh --once --jobs 8
# Quick range check without the full certificate:
tools/sentinel.sh --once --no-cert
```

Default jobs: 8. Without --once, a green cycle polls origin/main every
60 seconds and checks each new head. Red or deferred ends the process.
An optional local alias is `git config alias.push-sentinel
'!git push && tools/sentinel.sh --once'` (enter as one shell command).
Git has no built-in post-push hook; a local push wrapper can run the same
command. No remote service, scheduled job or GitHub workflow is installed.
Rule 10: this is a PM-invoked local sentinel, not GitHub CI.

It fetches origin/main and creates/reuses a DETACHED sibling worktree at
../initech-os-sentinel. It refuses an attached, dirty or foreign worktree.
The PM checkout and main are never edited; the sentinel never pushes.
Both sentinel.sh and standalone culprit.sh unset inherited BUILD,
PUBS_BUILD, KERNEL_REPRO_DIR, OVL_REPRO_B, SCREEN_OUT and
FIXTURE_VOLUMES_SCAN before any make.
They also clear writable ccache/config paths: CCACHE_DIR, CCACHE_BASEDIR,
CCACHE_TEMPDIR, CCACHE_CONFIGPATH, CCACHE_LOGFILE, CCACHE_DEBUGDIR and
CCACHE_STATSLOG. Make options/overrides are cleared too. Thus an exported
PM build/cache path cannot redirect detached builds or certificate workers
into that checkout. PUBS_BUILD is normally derived from BUILD by make.
Every worktree command guards cd and checks git's top-level path against
the expected worktree before make (including recursive wrappers, reruns
and the stage-2 runner). A missing/replaced worktree ends with setup error
2; it never falls back to making in the caller's checkout.
Export absolute *_DECOMP corpus overrides when the sibling layout differs.
The last green SHA lives in the PM checkout's build/sentinel/last-green.
Absent: announce and use origin/main~1. Stage 1 runs F8's real
`make -k test-for DIFF=<base>..HEAD`, with every recursive make bounded by
--jobs, including F8's explicit -j16. F8 completion files identify blocked
gates as red too. Each red is rerun once alone in the same worktree;
FLAKY records a solo green and NEVER excuses the original red.

Stage 2 runs tools/cert-sharded.sh -j N. At its start, a 1-minute loadavg
above nproc * 0.75 defers by 60 seconds, up to ten retries. Still busy:
record deferred. Exit 3 only when stage 1 is green; a red stage 1 keeps
result red and exits 1. This protects other lanes on the shared host.
Every certificate FAIL/FAIL-CONFIRMED/FLAKY remains red. --no-cert records
the skip explicitly; a green range-only cycle also advances last-green.

Artifacts stay in the PM checkout at build/sentinel/<head>/:

```text
base <last-green SHA>
base-source last-green
head <fetched SHA>
stage-1 gates 23
stage-1 result red
stage-1 red test-example
stage-1 wall-seconds 12
stage-2 result skipped (--no-cert)
stage-2 wall-seconds 0
result red
CULPRIT test-example <merge SHA> Merge example lane
tools/revert-merge.sh -y -r 'sentinel: test-example' -l 'build/sentinel/<head>/gates.log' <merge SHA>
wall-seconds 18
```

summary is ASCII; gates.log contains both stages and solo reruns. Per-run
directories keep dispatcher data and certificate logs across repeated runs.
Green atomically updates last-green; red/deferred preserves its old value.
Exit codes: 0 green, 1 red, 2 setup error, 3 deferred-and-green-so-far.
Red takes precedence over deferred; both stage results remain in summary.

On CULPRIT lines, inspect the logs, then run the printed F10 command from
the PM main checkout. -y also undoes newer merges first; use -n to inspect
F10's plan. Only F10 performs reverts, smoke checks and a push.
Standalone: `tools/culprit.sh test-example <base>` scans first-parent
merges oldest first through origin/main, in temporary detached worktrees
under build/culprit/. At each revision, a guarded `make -n <gate>` checks
whether that target exists. An exact missing-rule diagnostic for that gate
reports `UNTESTABLE <gate> <merge SHA> <subject>` and continues; it never
blames the older revision for a gate introduced later. Missing prerequisites
and other make failures remain red. It keeps preflight/attempt/solo logs
and reports the first existing red gate's merge and subject.
A solo green reports FLAKY; all green says "no culprit in range (flake class)".

Trade: main may be red for one push cycle; naming and reverting the culprit
is mechanical. Tests use throwaway repositories and a fake certificate:
`make test-sentinel test-sentinel-mutant`. SENTINEL_CERT_CMD selects an
executable path; SENTINEL_LOADAVG overrides the load sample for fixtures.
The fixture checks inherited BUILD against the PM build listing/checksum,
lost/replaced worktrees with a make cwd recorder, a gate introduced red at
M2, and red-plus-deferred exit precedence. Mutants cover newest-first,
flake-excused, BUILD-not-unset and missing-gate-counted-as-culprit.
