# build/ — images and intermediates (gitignored)

Disk images (`build/initech.img`), object files, the seed compiler and
factory tools, and emulator artifacts land here. Everything under `build/`
is **gitignored** except this README — `make clean` empties it.

Reproducible-build discipline applies (CLAUDE.md Rule 11, PRD §7): no
timestamps, no host paths, deterministic symbol ordering baked into
outputs, so the self-host certificate (`K2 == K3`) and DDC stay meaningful.

Governed by: **PRD §7, §11; CLAUDE.md "Build & test".**
