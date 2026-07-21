#!/usr/bin/env python3
"""Drive the live FLAIR tenants desktop through interaction storyboards.

DEV AID, NOT AN ORACLE (rfb_unblock.py precedent): reproduces the 2026-07-21
solidity drive battery (docs/FLAIR-solidity-drive-2026-07-21.md; epic
initech-av7s). The oracle versions of these scenarios are the LOCKED traces in
spec/flair_solid_traces.mk graded by tools/ppm_flair_solid_check.c.

Each scenario = one deterministic QEMU boot of build/flair_tenants.img with a
mouse/keys trace injected after FLAIR-LIVE-READY, one screendump (gated on a
serial marker), serial log kept.  Waypoints are ABSOLUTE screen coords; the
driver tracks the cursor (starts 320,240) and splits moves into int8-safe hops.
QEMU rel->cursor: x right-positive, y DOWN-positive (post-rgt8 convention).

Scene geometry (spec/flair_tenants_demo.h):
  HELLO struct (60,60)-(360,260)   title y 60..78,  close ~(74,70), zoom ~(345,70)
  NOTES struct (260,120)-(560,340) title visible x>=360 y 120..139, zoom ~(545,130)
  band1 menubar y 0..19 (System-7), band2 y 20..39 (active app's menu)
"""
import os, subprocess, sys, shutil

REPO = "/home/tobias/Projects/initech-os"
SCRATCH = os.path.dirname(os.path.abspath(__file__))
OUT = "/tmp/claude-1000/fd"  # short: QMP unix socket sun_path is 108-char capped
IMG = os.path.join(REPO, "build", "flair_tenants.img")
HARNESS = os.path.join(REPO, "build", "qemu_harness")

os.makedirs(OUT, exist_ok=True)

START = (320, 240)

def hops(dx, dy):
    """Split (dx,dy) into <=int8-magnitude hops (|c| <= 100 for margin)."""
    toks = []
    while dx or dy:
        hx = max(-100, min(100, dx))
        hy = max(-100, min(100, dy))
        toks.append(f"m{hx}:{hy}")
        dx -= hx
        dy -= hy
    return toks

def trace(actions):
    """actions: list of ('move',x,y) | ('down',) | ('up',) -> spec string."""
    cx, cy = START
    toks = []
    for a in actions:
        if a[0] == "move":
            x, y = a[1], a[2]
            toks += hops(x - cx, y - cy)
            cx, cy = x, y
        elif a[0] == "down":
            toks.append("l1")
        elif a[0] == "up":
            toks.append("l0")
        else:
            raise ValueError(a)
    return ",".join(toks)

def run(name, actions=None, dump_after="FLAIR-LIVE-READY", keys=None,
        timeout_ms=20000):
    spec = trace(actions) if actions else None
    cmd = [HARNESS, "--disk", IMG, "--name", name, "--out", OUT,
           "--screendump", "--screendump-after", dump_after,
           "--timeout-ms", str(timeout_ms)]
    if spec:
        cmd += ["--mouse", spec, "--keys-after", "FLAIR-LIVE-READY"]
    if keys:
        cmd += ["--keys", keys]
        if not spec:
            cmd += ["--keys-after", "FLAIR-LIVE-READY"]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=90)
    ppm = os.path.join(OUT, f"{name}.ppm")
    png = os.path.join(OUT, f"{name}.png")
    ok = os.path.exists(ppm) and os.path.getsize(ppm) > 0
    if ok:
        from PIL import Image
        Image.open(ppm).save(png)
    serial = os.path.join(OUT, f"{name}.serial")
    flair_lines = []
    if os.path.exists(serial):
        with open(serial, errors="replace") as f:
            flair_lines = [l.rstrip() for l in f if l.startswith("FLAIR-")]
    print(f"== {name}: dump={'OK' if ok else 'MISSING'} spec={spec}")
    for l in flair_lines[-12:]:
        print(f"   {l}")
    return ok

# ---------------------------------------------------------------- scenarios
S = {}

# s01: the resting scene (Law-4 baseline).
S["s01_boot"] = dict(actions=None, dump_after="FLAIR-TENANTS-READY")

# s02: the locked O-5 app switch (known good) - click NOTES sliver (460,300).
S["s02_switch"] = dict(actions=[("move",460,300),("down",),("up",)],
                       dump_after="FLAIR-DISPATCH app=NOTES")

# s03: drag the ACTIVE window (HELLO) by its title bar (200,70)->(240,100).
S["s03_drag_hello"] = dict(actions=[("move",200,70),("down",),("move",240,100),("up",)],
                           dump_after="FLAIR-DRAG")

# s04: drag the INACTIVE window (NOTES) by its visible title (450,130)->(510,170).
#      bug haaq: ghost drag - moves while staying behind + inactive?
S["s04_drag_notes"] = dict(actions=[("move",450,130),("down",),("move",510,170),("up",)],
                           dump_after="FLAIR-DRAG")

# s05: close the foreground HELLO via its close box (74,70).
S["s05_close_hello"] = dict(actions=[("move",74,70),("down",),("up",)],
                            dump_after="FLAIR-CLOSE")

# s06: press-and-hold on band2 (Photoshop bar, HELLO active) leftmost menu (20,30).
#      No release -> menu should stay dropped in the dump.
S["s06_menu_band2"] = dict(actions=[("move",20,30),("down",)],
                           dump_after="FLAIR-MENU-DROP")

# s07: app-switch to NOTES, then press band2 leftmost menu. bug t1rv: dispatches
#      to bar_sys instead of NOTES's bar?
S["s07_menu_after_switch"] = dict(
    actions=[("move",460,300),("down",),("up",),("move",20,30),("down",)],
    dump_after="FLAIR-MENU-DROP")

# s08: switch to NOTES then click HELLO's still-visible content (150,150) to
#      switch BACK. bug 7tjp: transient two-System-7 look while NOTES active.
S["s08_switch_back"] = dict(
    actions=[("move",460,300),("down",),("up",),("move",150,150),("down",),("up",)],
    dump_after="FLAIR-DISPATCH app=HELLO")

# s09: click HELLO's zoom box (345,70).
S["s09_zoom_hello"] = dict(actions=[("move",345,70),("down",),("up",)],
                           dump_after="FLAIR-EVT")

# s10: grow-drag from HELLO's bottom-right grow corner (355,255)->(420,320).
#      bug cjfr: grow hit zone is 1x1 px, so this may do nothing.
S["s10_grow_hello"] = dict(actions=[("move",355,255),("down",),("move",420,320),("up",)],
                           dump_after="FLAIR-EVT")

# s11: drag HELLO far off the top-left screen edge - clamping/garbage check.
S["s11_drag_offedge"] = dict(actions=[("move",200,70),("down",),("move",5,5),("up",)],
                             dump_after="FLAIR-DRAG")

# s12: open band1 (System-7) menu then release OUTSIDE (no selection) - does the
#      desktop under the dropped menu restore?
S["s12_menu_cancel"] = dict(
    actions=[("move",40,10),("down",),("move",300,300),("up",),("move",600,400)],
    dump_after="FLAIR-MENU menu=0")

if __name__ == "__main__":
    want = sys.argv[1:] or sorted(S)
    for name in want:
        run(name, **S[name])
