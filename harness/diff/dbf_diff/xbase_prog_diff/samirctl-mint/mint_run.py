#!/usr/bin/env python3
"""Run one samirctl mint case under dosbox-x headless; print the ALTERNATE
transcript and the decoded 80x25 screen dump."""
import os, subprocess, sys

# Working C: drive for dosbox-x (holds DBASE.EXE etc.; not committed):
# $SAMIRCTL_MINT_DIR, default $DBASE3_DECOMP/mint/samirctl.
D = os.environ.get("SAMIRCTL_MINT_DIR") or os.path.join(
    os.environ.get("DBASE3_DECOMP", "../dbase3-decomp"), "mint", "samirctl")
case = sys.argv[1]
tmo = sys.argv[2] if len(sys.argv) > 2 else "40"
prg = {"nest": "NEST", "raw": "RAW", "esc": "ESC", "esctrap": "ESCTRAP",
       "master": "MASTER", "quitp": "QUITP", "retry": "RETRYM", "escs": "ESCS"}.get(case, "ZZNONE")
for stale in (prg + ".TXT", prg[:8] + ".SCR", "SCREEN.BIN"):
    p = os.path.join(D, stale)
    if os.path.exists(p):
        os.remove(p)
env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
with open(os.path.join(D, "dosbox_" + case + ".txt"), "w") as log:
    rc = subprocess.call(["timeout", "-k", "5", tmo, "dosbox-x", "-conf",
                          os.path.join(D, case + ".conf"), "-fastlaunch",
                          "-exit"], stdout=log, stderr=subprocess.STDOUT,
                         env=env, cwd=D)
print("rc", rc)
t = os.path.join(D, prg + ".TXT")
if os.path.exists(t):
    print("---- ALTERNATE %s ----" % t)
    print(open(t, "rb").read().decode("latin-1").replace("\r", ""))
s = os.path.join(D, prg[:8] + ".SCR")
if os.path.exists(s):
    b = open(s, "rb").read()
    rows = []
    for r in range(25):
        rows.append(bytes(b[(r * 80 + c) * 2] for c in range(80))
                    .decode("latin-1").rstrip())
    while rows and not rows[-1]:
        rows.pop()
    with open(os.path.join(D, case + "-screen.txt"), "w") as f:
        f.write("\n".join(rows) + "\n")
    print("---- SCREEN ----")
    print("\n".join(rows))
