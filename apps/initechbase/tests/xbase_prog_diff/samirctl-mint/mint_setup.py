#!/usr/bin/env python3
"""samirctl mint: build the dosbox-x C: drive + per-case confs for the real
dBASE III PLUS 1.1 program-control transcripts (DO nesting, Esc interrupt,
ON ESCAPE, RETURN TO MASTER, QUIT in a program, RETRY from ON ERROR)."""
import os, shutil, subprocess, sys

# Working C: drive for dosbox-x (holds DBASE.EXE etc.; not committed):
# $SAMIRCTL_MINT_DIR, default $DBASE3_DECOMP/mint/samirctl.
D = os.environ.get("SAMIRCTL_MINT_DIR") or os.path.join(
    os.environ.get("DBASE3_DECOMP", "../dbase3-decomp"), "mint", "samirctl")
X = os.path.join(os.environ.get("DBASE3_DECOMP", "../dbase3-decomp"),
                 "goldens", "dbase-iii-plus-1.1", "extracted")
os.makedirs(D, exist_ok=True)
for src in ("disk1/DBASE.EXE", "disk1/DBASEINL.OVL", "disk1/DBASE.MSG",
            "disk2/DBASE.OVL"):
    shutil.copy(os.path.join(X, src), D)

def w(name, lines):
    with open(os.path.join(D, name), "wb") as f:
        f.write(("\r\n".join(lines) + "\r\n").encode("ascii"))

w("CONFIG.DB", ["STATUS = OFF", "BELL = OFF", "SAFETY = OFF", "TALK = OFF"])

ALT = lambda t: ["SET TALK OFF", "SET ALTERNATE TO %s" % t, "SET ALTERNATE ON"]

# 1. DO nesting depth with an ON ERROR trap capturing depth + code + text.
w("NEST.PRG", ALT("NEST.TXT") + [
    "PUBLIC N", "N=0",
    'ON ERROR ? "TRAP",N,ERROR(),MESSAGE()',
    "SET PROCEDURE TO NESTP",
    "DO REC",
    '? "UNWOUND",N',
    "CLOSE PROCEDURE",
    "ON ERROR", "SET ALTERNATE TO", "QUIT"])
w("NESTP.PRG", ["PROCEDURE REC", "N=N+1", "DO REC", "RETURN"])

# 2. DO nesting with NO trap: the raw error screen + prompt (cancel, quit).
w("RAW.PRG", ALT("RAW.TXT") + ["PUBLIC N", "N=0", "SET PROCEDURE TO RAWP",
    "RUN AUTOTYPE -w 0 -p 0.1 c q u i t enter", "DO RREC",
                                 '? "AFTER",N', "QUIT"])
w("RAWP.PRG", ["PROCEDURE RREC", "N=N+1", "DO RREC", "RETURN"])

# 3. Esc during a DO WHILE loop: Ignore once, then Cancel, then QUIT.
w("ESC.PRG", ALT("ESC.TXT") + ["N=0",
    "RUN AUTOTYPE -w 0 -p 0.1 esc i esc c q u i t enter",
    "DO WHILE .T.", "N=N+1", "ENDDO", '? "NOTREACHED"'])

# 3b. Esc, Suspend, RESUME at the suspended dot prompt, Esc, Cancel, QUIT.
w("ESCS.PRG", ALT("ESCS.TXT") + ["N=0",
    "RUN AUTOTYPE -w 0 -p 0.1 esc s r e s u m e enter esc c q u i t enter",
    "DO WHILE .T.", "N=N+1", "ENDDO", '? "NOTREACHED"'])

# 4. ON ESCAPE trap ends the loop.
w("ESCTRAP.PRG", ALT("ESCTRAP.TXT") + [
    "STOP=.F.", "N=0", "ON ESCAPE STOP=.T.",
    "RUN AUTOTYPE -w 0 -p 0.1 esc",
    "DO WHILE .NOT. STOP", "N=N+1", "ENDDO",
    '? "TRAPPED",STOP', "ON ESCAPE", "SET ALTERNATE TO", "QUIT"])

# 5. RETURN TO MASTER from two levels down (audit MASTER2 shape, files).
w("MASTER.PRG", ALT("MASTER.TXT") + ['? "MASTER"', "DO FIRST",
                                       '? "MASTER RESUMED"',
                                       "SET ALTERNATE TO", "QUIT"])
w("FIRST.PRG", ['? "FIRST"', "DO SECOND", '? "FIRST RESUMED"', "RETURN"])
w("SECOND.PRG", ['? "SECOND"', "RETURN TO MASTER", '? "SECOND AFTER"',
                 "RETURN"])

# 6. QUIT inside a program (audit QUITPRG shape).
w("QUITP.PRG", ALT("QUITP.TXT") + ['? "BEFORE QUIT"', "QUIT",
                                     '? "AFTER QUIT"', "RETURN"])

# 7. RETRY from an ON ERROR handler (audit RETRYFIX shape, files).
w("RETRYM.PRG", ALT("RETRYM.TXT") + [
    "PUBLIC XX,TRYCNT", 'XX="bad"', "TRYCNT=0", "ON ERROR DO ERH",
    "? XX+1", '? "DONE",TRYCNT', "ON ERROR", "SET ALTERNATE TO", "QUIT"])
w("ERH.PRG", ["XX=6", "TRYCNT=TRYCNT+1", "RETRY"])

subprocess.check_call(["nasm", "-f", "bin", "-o", os.path.join(D, "SCRDUMP.COM"),
                       os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                    "scrdump.asm")])

CASES = {
    "nest":    ("NEST", None),
    "raw":     ("RAW", None),
    "esc":     ("ESC", None),
    "esctrap": ("ESCTRAP", None),
    "escs":    ("ESCS", None),
    "master":  ("MASTER", None),
    "quitp":   ("QUITP", None),
    "retry":   ("RETRYM", None),
}
for case, (prg, keys) in CASES.items():
    auto = ("AUTOTYPE %s\n" % keys) if keys else ""
    conf = """[sdl]
output=surface
[dosbox]
machine=svga_s3
memsize=8
[cpu]
core=auto
cputype=386
cycles=max
[autoexec]
mount c %s
c:
DATE 07/24/1986
%sDBASE %s
SCRDUMP
COPY SCREEN.BIN %s.SCR
exit
""" % (D, auto, prg, prg[:8])
    with open(os.path.join(D, case + ".conf"), "w") as f:
        f.write(conf)
print("ok", D)
