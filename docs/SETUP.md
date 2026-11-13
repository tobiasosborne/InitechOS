# Setting up a machine

How to get from a fresh clone to a green `make test` and a booting
product. Written 2026-11-13, when the work moved between machines.

## 1. Tools (Ubuntu or Debian)

```sh
sudo apt install build-essential gcc-multilib nasm make mtools \
  qemu-system-x86 qemu-utils bochs bochsbios vgabios fpc ccache
```

- `gcc-multilib`: the OS is built with the host gcc at `-m32`.
- `fpc` (Free Pascal): the independent reference for Turbo Initech.
- `bochs` needs the legacy BIOS image (`BIOS-bochs-legacy`) and a VGA
  BIOS; on Ubuntu these come from `bochsbios` and `vgabios`.
- Python 3 (standard library only) is used by a few tests as an
  independent reader of foreign file formats. It builds nothing.

## 2. The repository and the legacy reference

```sh
cd ~/Projects
git clone git@github.com:tobiasosborne/InitechOS.git
cd InitechOS
git worktree add --detach ../InitechOS-legacy legacy-final
```

Clone to a short path such as `~/Projects/InitechOS`. One InitechWord
test (`test_iword_listfiles_ops`) fails when the checkout's absolute
path is very long; that is a defect in the test, tracked in `bd`.

`../InitechOS-legacy` is the frozen old tree. It is the reference for
behaviour and is never edited. To build it:
`cd ../InitechOS-legacy && . tools/lane-env.sh && make -j32 factory image`.
Nothing on `main` needs it to be built.

## 3. The reference corpora

Tier 0 compares against files captured from real period software. They
live in sibling checkouts next to this repository (`mk/config.mk`:
`INITECH_CORPUS ?= $(HOME)/Projects`):

| Variable | Repository | Branch | Pinned files | Of which not in git |
|---|---|---|---:|---:|
| `SYSTEM7_DECOMP` | `tobiasosborne/system7-decomp` | master | 19 | 16 |
| `WIN31_DECOMP` | `tobiasosborne/win31-decomp` | master | 1 | 1 |
| `DBASE3_DECOMP` | `tobiasosborne/dbase3-decomp` | mint-005-samir | 769 | 769 |
| `LOTUS123_DECOMP` | `tobiasosborne/lotus123-decomp` | master | 66 | 39 |
| `WORDPERFECT51_DECOMP` | `tobiasosborne/wordperfect51-decomp` | master | 64 | 43 |
| `TURBOPASCAL_DECOMP` | `tobiasosborne/turbopascal-decomp` | master | 0 | - |

`goldens/CORPORA.lock` pins the git HEAD of each checkout and the
SHA-256 of every file the tests read. `make corpus` verifies only that;
`make test` runs it first.

Two things can go wrong on a new machine, and both fail loudly with the
name of the file or variable:

- **Wrong commit.** Check out the pinned HEAD: the lock's `HEAD` lines
  give the commit for each corpus (all of them are pushed).
- **Missing or different files.** Most pinned files are copyrighted
  period software and are deliberately not in git. They have to exist
  in the checkout already (the laptop has them) or be restored the way
  the legacy tree documented: `git show legacy-final:docs/BOOTSTRAP.md`
  and `tools/bootstrap/` at that tag. If a file on this machine is the
  genuine capture and the lock disagrees, do not edit the lock to get
  green: find out which side changed.

To use corpora somewhere else: `make test INITECH_CORPUS=/path`, or set
the individual variables.

## 4. First run

```sh
make corpus        # the pinned reference files are present and correct
make -j16 test     # every component's host tests, about 75 s
make image         # the product disk set
make smoke         # boots it on QEMU and Bochs, about 3 minutes
make status        # which components are imported, and why not
make run           # QEMU with serial on stdio
```

The machine needs 8 MiB. In QEMU that is `-m 8 -machine acpi=off`;
plain `-m 8` loses memory to ACPI tables and the loader refuses it.

## 5. The tracker

Issues are in `bd` (beads). The committed export is
`.beads/issues.jsonl`, including stored memories. On a new machine
`bd` builds its database from that file; run `bd prime`, then
`bd ready`. If it reports a schema migration against dirty tables, run
`bd dolt commit` and retry (this happened once on 2026-11-13).

## 6. Where to start

Read `CLAUDE.md` (one page of rules), `GOLDEN-MASTER.md` (the closed
scope) and `STATUS.md` (what is true today and what is next).
