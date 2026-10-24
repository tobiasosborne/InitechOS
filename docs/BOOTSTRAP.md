# BOOTSTRAP -- rebuilding the InitechOS workbench on a new machine

Written 2026-10-24 (WL-0178 session) so that a fresh clone reaches "full
certificate green + able to mint goldens + able to run the PM lanes" with no
knowledge that lives only on the original laptop. Follow it top to bottom; the
end state is `tools/bootstrap/doctor.sh` printing `DOCTOR: ready` and a full
`make clean && make test` certificate.

Everything below assumes the layout the Makefile and the harness expect:

```
~/Projects/
  initech-os/              this repo (public, github.com/tobiasosborne/InitechOS)
  dbase3-decomp/           ground truth: real dBASE III+ (PRIVATE repo)
  lotus123-decomp/         ground truth: real Lotus 1-2-3 R2.2 (PRIVATE)
  system7-decomp/          ground truth: System 7 / Mac OS 8.1 chrome, fonts, ROMs (PRIVATE)
  win31-decomp/            ground truth: Windows 3.1 (PRIVATE)
  wordperfect51-decomp/    ground truth: real WordPerfect 5.1 (PRIVATE)
  turbopascal-decomp/      ground truth: real Turbo Pascal 5.5 (PRIVATE)
  initech-manuals-ref/     scans of the period manuals (PRIVATE; copyrighted, never public)
  quota-app/               the quota app (/usr/bin/quota), github.com/tobiasosborne/quota-app
```

The sibling paths are overridable (`SYSTEM7_DECOMP=... make ...`, see Makefile
lines ~43-47) but nothing else in the docs assumes anything but `../<name>`.

## 0. Machine

- x86-64 Linux (the original is Ubuntu 24.04, kernel 6.8). `gcc -m32` and
  `qemu-system-i386` are assumed; an ARM host is not supported by the interim
  CDR-0001 toolchain.
- Disk: ~6 GB for the repos + goldens, plus build output (a full certificate
  builds ~300 kernels under build/). 16 GB RAM is comfortable; the gates run
  one emulator at a time by default.
- An X display is NOT needed for the gates (QEMU runs `-display none`, Bochs
  headless, mints on a private Xvfb). The operator's desktop display is never
  used by tooling.

## 1. Packages (apt, Ubuntu 24.04)

```sh
sudo apt install build-essential gcc-multilib nasm make git jq p7zip-full unzip \
     qemu-system-x86 bochs bochs-x bochsbios vgabios mtools dosfstools \
     fpc dosbox-x basilisk2 xvfb imagemagick ffmpeg poppler-utils \
     python3 python3-numpy python3-pil texlive-full gh git-lfs
```

Why each matters (so you can tell a hard requirement from a nicety):

| Package | Used by | If missing |
|---|---|---|
| build-essential, gcc-multilib, nasm | the artifact (C, freestanding -m32) + boot stubs | nothing builds |
| qemu-system-x86 | every emu gate (`test-*-emu`, clips) | `make test` red |
| bochs + bochs-x + bochsbios | the Bochs legs (`test-boot-bochs`, FLAIR desktop/appswitch, SAMIR) | `make test` FAILS LOUD; `SKIP_BOCHS=1` is the only opt-out |
| vgabios | every Bochs leg: the stock LGPL ROM via the `/usr/share/bochs/VGABIOS-lgpl-latest` symlink (dangling without it; bochs only Recommends it) AND the ISA Cirrus ROM `/usr/share/vgabios/vgabios.cirrus.bin` (`BOCHS_CIRRUS_VGABIOS` in harness/bochs_legs.mk, the vga13 legs) | Bochs panics "couldn't open ROM image file", `serial_len=0`, `make test` red |
| mtools, dosfstools | FAT differential, fixture volumes, mtools-differential Finder ops | `make test` red |
| fpc | the INDEPENDENT golden for the seed/TPS differential | FAILS LOUD; `SKIP_FPC=1` is the only opt-out |
| dosbox-x | minting real dBASE/Lotus/WP/TP goldens; some gates replay DOSBox transcripts | mint lanes impossible |
| basilisk2 | Mac OS 8.1 / System 7 captures in ../system7-decomp | FLAIR capture-gated beads blocked |
| xvfb, imagemagick, ffmpeg | mint screenshots, record-flair clips (Rule 14) | clips/mints impossible |
| texlive-full, poppler-utils | pubs (`test-pubs*`, the shipped manuals) | `make test` red (pubs are in the vector) |
| gh | private repos + release-asset restore below | cannot fetch goldens |

86Box (period-authenticity legs; its driver `harness/emu/box86.c` is still planned,
bead initech-x0i, but the machine + ROMs are expected by research lanes):

```sh
flatpak install flathub net._86box._86Box net._86box._86Box.ROMs
```

Check `gcc -m32 -ffreestanding -nostdlib` compiles a trivial file; on Ubuntu
that is what `gcc-multilib` provides.

## 2. Agent / PM tooling

- **bd (beads)** -- the ONLY cross-session tracker (CLAUDE.md Rule 9). Binary at
  `~/.local/bin/bd` (version 1.0.0 on the original box) with the `dolt` engine
  at `/usr/local/bin/dolt` (1.83.8). Install per the beads project's README
  (github.com/steveyegge/beads); then in the clone:
  `bd import .beads/issues.jsonl` if `bd ready` is empty. The export carries
  the issues AND the `bd remember` memories (the ~97 rows without an
  issue_type); run `bd memories` afterwards to confirm they came through.
  `bd dolt pull` has no remote configured on purpose; the JSONL in git is the
  transport.
- **Claude Code** -- `~/.local/bin/claude` (2.1.310). Project settings and
  skills travel in the user's `~/.claude` (not in this repo): the
  `familiarize-project`, `quota` and `codex-vision` skills were used by the PM
  sessions; re-create or copy `~/.claude/skills/` from the old machine.
- **Codex CLI** -- `~/.local/bin/codex` (0.163.0, standalone package under
  `~/.codex`). The PM lanes run `codex exec -m gpt-6.1-sol -c
  model_reasoning_effort=xhigh -s workspace-write`. Log in once (`codex login`).
- **quota app** -- `/usr/bin/quota` is built from `../quota-app` (Rust, Cargo;
  clone github.com/tobiasosborne/quota-app and `cargo build --release`,
  install the binary to /usr/bin/quota). Every PM dispatch reads it first.
- **gh** -- `gh auth login` as tobiasosborne; the goldens below are release
  assets of PRIVATE repos.

## 3. Clone and restore the ground truth (Law 1)

```sh
mkdir -p ~/Projects && cd ~/Projects
git clone https://github.com/tobiasosborne/InitechOS initech-os
cd initech-os
tools/bootstrap/fetch-siblings.sh        # clones the six *-decomp repos and restores their goldens
cd .. && git clone https://github.com/tobiasosborne/initech-manuals-ref && cd initech-manuals-ref && ./fetch.sh
cd ../quota-app 2>/dev/null || git clone https://github.com/tobiasosborne/quota-app ../quota-app
```

What `fetch-siblings.sh` does, in case it has to be done by hand: for each
sibling it clones the repo, downloads the release `goldens-2026-10-24` (or the
latest) assets `<repo>-ignored.tar` + `<repo>-ignored.sha256`, untars into the
repo root and verifies every file. Those tarballs are the repos' gitignored
content: real software disk images and installed trees (`goldens/`,
`mint/work/`), Mac ROMs (`system7-decomp/goldens/roms`), the Mac OS 8.1
captures the FLAIR pixel gates grade against (`system7-decomp/goldens/captures`),
NFNT/PAT# resources, the WP List Files mint image, the DOS 6.22 mint disk.
Sixteen of those files are read directly by the Makefile (grep `_DECOMP)/`),
more by mint scripts, so a sibling without its tarball will turn gates red.

Snapshot sizes (2026-10-24, dbase3 refreshed with mint/work the same day): dbase3 591 MB, win31 679 MB, system7 1.2 GB,
wordperfect51 43 MB, lotus123 17 MB, turbopascal 5.5 MB; manuals 1.1 GB.

Re-acquiring from the original sources instead: every sibling's `SOURCES.md`
lists the archive.org / WinWorld / vendor URL and sha256 of each golden master
(e.g. Turbo Pascal 5.5 = Embarcadero museum `tp55.zip`,
sha256 a04ff655...), and `initech-manuals-ref/SOURCES.md` does the same for
every scan. The release assets are a convenience, not the only route.

Refreshing a snapshot after new mints land in a sibling:

```sh
tools/bootstrap/snapshot-sibling.sh dbase3-decomp            # re-tar + re-upload to goldens-2026-10-24
TAG=goldens-2026-11-01 tools/bootstrap/snapshot-sibling.sh lotus123-decomp   # or a new tag
```

The script tars every gitignored path, writes the sha256 manifest, uploads
both with `--clobber`, and **REFUSES while the sibling has untracked (`??`)
files**. That rule exists because of the first machine move: a snapshot only
sees ignored paths, so anything neither committed nor ignored stays on the old
laptop. `dbase3-decomp/mint/work` (the installed DBASE.EXE tree seven Makefile
sites read) was in exactly that state on 2026-10-24 and had to be re-snapshotted
after the new box's certificate went red on `test-samir-use` (initech-e8s3).
Every sibling's rule of thumb: our work (`.steps`, confs, `.PRG`, transcripts,
screenshots, scripts, mint-results) is committed; copyrighted or installed
material (`goldens/`, `mint/work/`, `mint/samirctl/`, ROMs, captures) is
ignored and travels as the asset. `git status --short` in a sibling must be
empty apart from `!!` lines before a snapshot.

`tools/bootstrap/doctor.sh` checks the exact sibling files the Makefile and
gate rules read (grep `_DECOMP)/` in `Makefile` + `*.mk`), so a sibling that is
"restored" but missing a gate input is MISSING there before `make test` is.

## 4. Verify the machine

```sh
cd ~/Projects/initech-os
tools/bootstrap/doctor.sh            # every tool, every sibling, the manuals manifest
```

Then the certificate, in a separate worktree so lanes can keep using the main
checkout (this is also the first job of ANY session after a machine move --
it is the only thing that catches environment-isms):

```sh
git worktree add ../initech-os-cert main
cd ../initech-os-cert && make clean && make test 2>&1 | tee /tmp/cert.log
```

Expect: all host gates + all emu gates green, serially, in well under an hour
on a 2024 laptop. Known-red gates are NAMED (`test-bochs-pending`,
`test-samir-fns-pending`), never silent. If a gate is red only on the new box,
that is an emulator-ism or a toolchain delta: stop and compare versions with
the table in Sec 1 before touching an oracle (CLAUDE.md Stop conditions).

Smoke the pieces individually if you want faster signal:

```sh
make image && make run          # boots the flagship in QEMU, serial on stdout
make test-region test-fat       # pure host oracles, seconds
make test-flair-desktop         # one emu gate with its screendump oracle
make test-seed-fpc-diff         # proves fpc is wired
make test-pubs-lint             # proves TeX is wired
```

## 5. Resume the programme

Read order is CLAUDE.md -> InitechOS-PRD.md -> `docs/HANDOFF.md` Sec 5 (CURRENT
STATE, newest block first) -> the latest `docs/worklog/WL-*.md` -> `bd ready`.
HANDOFF Sec 6 "Where things live" and Sec 7 "Gotchas" are the parts a new box
needs most. The lane-dispatch pattern the PM sessions use (Codex worktree lanes,
the flock'd serial merge runner, the one-review-one-fix rule, the quota bands)
is recorded in `bd memories` (search `bd memories dispatch`, `bd memories merge`).

Things that do NOT travel and must be re-created on the new machine:

- `~/.claude/` (skills, settings, the auto-memory directory) and `~/.codex/`.
- The PM scratchpad (`/tmp/claude-.../scratchpad`): volatile by design; every
  durable fact from it is in a worklog, a bead note or `bd remember`.
- Lane worktrees (`../initech-os-cx-*`, `../initech-os-wt-*`): recreate per lane.
- `build/` in every repo: regenerated by make; mint `work/` trees are in the
  release tarballs but can also be re-created from the goldens by each
  sibling's `mint/*.steps` + `tools/remint.sh` / `mint_run.sh`.
