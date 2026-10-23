from pathlib import Path
import subprocess,sys,json,time
E=Path(__file__).resolve().parent
for name in sys.argv[1:]:
 print('START',name,flush=True)
 with (E/(name+'-runner.log')).open('w') as f:
  rc=subprocess.run([sys.executable,'-u',str(E/'run_suite.py'),str(E/(name+'.json'))],stdout=f,stderr=subprocess.STDOUT).returncode
 print('END',name,rc,flush=True)
 if rc:raise SystemExit(rc)
